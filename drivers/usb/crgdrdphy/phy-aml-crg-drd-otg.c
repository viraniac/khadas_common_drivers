// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/module.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/irqreturn.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/pm_runtime.h>
#include <linux/delay.h>
#include <linux/usb/phy.h>
#include <linux/amlogic/usb-v2.h>
#include <linux/amlogic/aml_gpio_consumer.h>
#include <linux/workqueue.h>
#include <linux/notifier.h>
#include <linux/suspend.h>
#include <linux/amlogic/usbtype.h>
#include "../phy/phy-aml-new-usb-v2.h"
#include "../usb_main.h"

#include <linux/amlogic/gki_module.h>
#define HOST_MODE	0
#define DEVICE_MODE	1

static int UDC_exist_flag = -1;
static char crg_UDC_name[128];

struct amlogic_crg_otg {
	struct device           *dev;
	void __iomem    *phy3_cfg;
	void __iomem    *phy3_cfg_r1;
	void __iomem    *phy3_cfg_r2;
	void __iomem    *phy3_cfg_r4;
	void __iomem    *phy3_cfg_r5;
	void __iomem    *usb2_phy_cfg;
	void __iomem    *m31_phy_cfg;
	struct delayed_work     work;
	struct delayed_work     set_mode_work;
	/*otg_mutex should be taken amlogic_crg_otg_work and
	 *amlogic_crg_otg_set_m_work
	 */
	struct mutex		*otg_mutex;
	struct gpio_desc *usb_gpio_desc;
	int vbus_power_pin;
	int mode_work_flag;
	int controller_type;
	struct pm_reg {
		/* host/device mode. */
		union u2p_r0_v2 u2p_r0;
		/* iddig irq state. */
		union u2p_r2_v2 u2p_r2;
		union u2p_r1_v2 usb_r1;
	} pm_buf;
	struct notifier_block pm_notifier;
};

static void set_mode
	(unsigned long reg_addr, int mode, unsigned long phy3_addr);

static int amlogic_crg_otg_pm_cb(struct notifier_block *notifier,
			      unsigned long pm_event,
			      void *unused);

static void set_usb_vbus_power
	(struct gpio_desc *usb_gd, int pin, char is_power_on)
{
	if (is_power_on)
		/*set vbus on by gpio*/
		gpiod_direction_output(usb_gd, 1);
	else
		/*set vbus off by gpio first*/
		gpiod_direction_output(usb_gd, 0);
}

static void amlogic_m31_set_vbus_power
		(struct amlogic_crg_otg *phy, char is_power_on)
{
	if (phy->vbus_power_pin != -1)
		set_usb_vbus_power(phy->usb_gpio_desc,
				   phy->vbus_power_pin, is_power_on);
}

static int amlogic_crg_otg_init(struct amlogic_crg_otg *phy)
{
	union usb_r1_v2 r1 = {.d32 = 0};
	union u2p_r2_v2 reg2 = {.d32 = 0};
	struct usb_aml_regs_v2 usb_crg_otg_aml_regs;

	usb_crg_otg_aml_regs.usb_r_v2[1] = phy->phy3_cfg + 4;

	r1.d32 = readl(usb_crg_otg_aml_regs.usb_r_v2[1]);
	r1.b.u3h_fladj_30mhz_reg = 0x20;
	writel(r1.d32, usb_crg_otg_aml_regs.usb_r_v2[1]);

	reg2.d32 = readl((void __iomem *)((unsigned long)phy->usb2_phy_cfg + 8));
	reg2.b.iddig_en0 = 1;
	reg2.b.iddig_en1 = 1;
	reg2.b.iddig_th = 255;
	writel(reg2.d32, (void __iomem *)((unsigned long)phy->usb2_phy_cfg + 8));

	return 0;
}

static void set_mode(unsigned long reg_addr, int mode, unsigned long phy3_addr)
{
	struct u2p_aml_regs_v2 u2p_aml_regs;
	union u2p_r0_v2 reg0;

	u2p_aml_regs.u2p_r_v2[0] = (void __iomem *)((unsigned long)reg_addr);

	reg0.d32 = readl(u2p_aml_regs.u2p_r_v2[0]);
	if (mode == DEVICE_MODE) {
		reg0.b.host_device = 0;
		reg0.b.POR = 0;
	} else {
		reg0.b.host_device = 1;
		reg0.b.POR = 0;
	}
	writel(reg0.d32, u2p_aml_regs.u2p_r_v2[0]);

	usleep_range(500, 600);
}

static void amlogic_crg_otg_work(struct work_struct *work)
{
	struct amlogic_crg_otg *phy =
		container_of(work, struct amlogic_crg_otg, work.work);
	union u2p_r2_v2 reg2;
	unsigned long reg_addr = ((unsigned long)phy->usb2_phy_cfg);
	unsigned long phy3_addr = ((unsigned long)phy->phy3_cfg);
	int ret;

	if (phy->mode_work_flag == 1) {
		cancel_delayed_work_sync(&phy->set_mode_work);
		phy->mode_work_flag = 0;
	}
	mutex_lock(phy->otg_mutex);
	reg2.d32 = readl((void __iomem *)(reg_addr + 8));
	if (reg2.b.iddig_curr == 0) {
		crg_gadget_exit();
		amlogic_m31_set_vbus_power(phy, 1);
		set_mode(reg_addr, HOST_MODE, phy3_addr);
		crg_init();
	} else {
		crg_exit();
		set_mode(reg_addr, DEVICE_MODE, phy3_addr);
		amlogic_m31_set_vbus_power(phy, 0);
		crg_gadget_init();
		if (UDC_exist_flag != 1) {
			ret = crg_otg_write_UDC(crg_UDC_name);
			if (ret == 0 || ret == -EBUSY)
				UDC_exist_flag = 1;
		}
	}
	reg2.b.usb_iddig_irq = 0;
	writel(reg2.d32, (void __iomem *)(reg_addr + 8));
	mutex_unlock(phy->otg_mutex);
}

static void amlogic_crg_otg_set_m_work(struct work_struct *work)
{
	struct amlogic_crg_otg *phy =
		container_of(work, struct amlogic_crg_otg, set_mode_work.work);
	union u2p_r2_v2 reg2;
	unsigned long reg_addr = ((unsigned long)phy->usb2_phy_cfg);
	unsigned long phy3_addr = ((unsigned long)phy->phy3_cfg);

	mutex_lock(phy->otg_mutex);
	phy->mode_work_flag = 0;
	reg2.d32 = readl((void __iomem *)(reg_addr + 8));
	if (reg2.b.iddig_curr == 0) {
		amlogic_m31_set_vbus_power(phy, 1);
		set_mode(reg_addr, HOST_MODE, phy3_addr);
		crg_init();
	} else {
		set_mode(reg_addr, DEVICE_MODE, phy3_addr);
		amlogic_m31_set_vbus_power(phy, 0);
		crg_gadget_init();
	}
	reg2.b.usb_iddig_irq = 0;
	writel(reg2.d32, (void __iomem *)(reg_addr + 8));
	mutex_unlock(phy->otg_mutex);
}

static irqreturn_t amlogic_crgotg_detect_irq(int irq, void *dev)
{
	struct amlogic_crg_otg *phy = (struct amlogic_crg_otg *)dev;
	union u2p_r2_v2 reg2;

	reg2.d32 = readl((void __iomem *)((unsigned long)phy->usb2_phy_cfg + 8));
	reg2.b.usb_iddig_irq = 0;
	writel(reg2.d32, (void __iomem *)((unsigned long)phy->usb2_phy_cfg + 8));

	schedule_delayed_work(&phy->work, msecs_to_jiffies(100));

	return IRQ_HANDLED;
}

static int amlogic_crg_otg_probe(struct platform_device *pdev)
{
	struct amlogic_crg_otg			*phy;
	struct device *dev = &pdev->dev;
	void __iomem *usb2_phy_base;
	unsigned int usb2_phy_mem;
	unsigned int usb2_phy_mem_size = 0;
	void __iomem *usb3_phy_base;
	unsigned int usb3_phy_mem;
	unsigned int usb3_phy_mem_size = 0;
	void __iomem *m31_phy_base;
	unsigned int m31_phy_mem;
	unsigned int m31_phy_mem_size = 0;
	const void *prop;
	int irq;
	int retval;
	int len, otg = 0;
	int controller_type = USB_NORMAL;
	const char *udc_name = NULL;
	const char *gpio_name = NULL;
	int gpio_vbus_power_pin = -1;
	struct gpio_desc *usb_gd = NULL;
	u32 val;

	gpio_name = of_get_property(dev->of_node, "gpio-vbus-power", NULL);
	if (gpio_name) {
		gpio_vbus_power_pin = 1;
		usb_gd = devm_gpiod_get_index
			(&pdev->dev, NULL, 0, GPIOD_OUT_LOW);
		if (IS_ERR(usb_gd))
			return -1;
	}

	prop = of_get_property(dev->of_node, "controller-type", NULL);
	if (prop)
		controller_type = of_read_ulong(prop, 1);

	retval = of_property_read_u32
				(dev->of_node, "usb2-phy-reg", &usb2_phy_mem);
	if (retval < 0)
		return -EINVAL;

	retval = of_property_read_u32
		(dev->of_node, "usb2-phy-reg-size", &usb2_phy_mem_size);
	if (retval < 0)
		return -EINVAL;

	usb2_phy_base = devm_ioremap
				(&pdev->dev, (resource_size_t)usb2_phy_mem,
				(unsigned long)usb2_phy_mem_size);
	if (!usb2_phy_base)
		return -ENOMEM;

	retval = of_property_read_u32
				(dev->of_node, "usb3-phy-reg", &usb3_phy_mem);
	if (retval < 0)
		return -EINVAL;

	retval = of_property_read_u32
		(dev->of_node, "usb3-phy-reg-size", &usb3_phy_mem_size);
	if (retval < 0)
		return -EINVAL;

	usb3_phy_base = devm_ioremap
				(&pdev->dev, (resource_size_t)usb3_phy_mem,
				(unsigned long)usb3_phy_mem_size);
	if (!usb3_phy_base)
		return -ENOMEM;

	phy = devm_kzalloc(&pdev->dev, sizeof(*phy), GFP_KERNEL);
	if (!phy)
		return -ENOMEM;

	udc_name = of_get_property(pdev->dev.of_node, "udc-name", NULL);
	if (!udc_name)
		udc_name = "fdd00000.crgudc2";
	len = strlen(udc_name);
	if (len >= 128) {
		dev_info(&pdev->dev, "udc_name is too long: %d\n", len);
		return -EINVAL;
	}
	strncpy(crg_UDC_name, udc_name, len);
	crg_UDC_name[len] = '\0';

	if (controller_type == USB_OTG)
		otg = 1;

	dev_info(&pdev->dev, "controller_type is %d\n", controller_type);
	dev_info(&pdev->dev, "crg_force_device_mode is %d\n", get_otg_mode());
	dev_info(&pdev->dev, "otg is %d\n", otg);
	dev_info(&pdev->dev, "udc_name: %s\n", crg_UDC_name);

	dev_info(&pdev->dev, "phy2_mem:0x%lx, iomap phy2_base:0x%lx\n",
		 (unsigned long)usb2_phy_mem,
		 (unsigned long)usb2_phy_base);

	dev_info(&pdev->dev, "phy3_mem:0x%lx, iomap phy3_base:0x%lx\n",
		 (unsigned long)usb3_phy_mem,
		 (unsigned long)usb3_phy_base);

	phy->dev		= dev;
	phy->usb2_phy_cfg	= usb2_phy_base;
	phy->phy3_cfg = usb3_phy_base;
	phy->vbus_power_pin = gpio_vbus_power_pin;
	phy->usb_gpio_desc = usb_gd;
	phy->mode_work_flag = 0;
	phy->controller_type = controller_type;
	phy->otg_mutex = kmalloc(sizeof(*phy->otg_mutex),
				GFP_KERNEL);
	mutex_init(phy->otg_mutex);

	INIT_DELAYED_WORK(&phy->work, amlogic_crg_otg_work);

	platform_set_drvdata(pdev, phy);

	pm_runtime_enable(phy->dev);

	phy->pm_notifier.notifier_call = amlogic_crg_otg_pm_cb;
	register_pm_notifier(&phy->pm_notifier);

	if (otg) {
		retval = of_property_read_u32
			(dev->of_node, "m31-phy-reg", &m31_phy_mem);
		if (retval < 0)
			goto NO_M31;

		retval = of_property_read_u32
			(dev->of_node, "m31-phy-reg-size", &m31_phy_mem_size);
		if (retval < 0)
			goto NO_M31;

		m31_phy_base = devm_ioremap
			(&pdev->dev, (resource_size_t)m31_phy_mem,
			(unsigned long)m31_phy_mem_size);
		if (!m31_phy_base)
			goto NO_M31;

		phy->m31_phy_cfg = m31_phy_base;

		val = readl(phy->m31_phy_cfg + 0x8);
		val |= 1;
		writel(val, phy->m31_phy_cfg + 0x8);
NO_M31:
		irq = platform_get_irq(pdev, 0);
		if (irq < 0)
			return -ENODEV;
		retval = request_irq(irq, amlogic_crgotg_detect_irq,
				 IRQF_SHARED, "amlogic_botg_detect", phy);

		if (retval) {
			dev_err(&pdev->dev, "request of irq%d failed\n", irq);
			retval = -EBUSY;
			return retval;
		}
	}

	amlogic_crg_otg_init(phy);

	if (otg == 0) {
		if (get_otg_mode() || controller_type == USB_DEVICE_ONLY) {
			set_mode((unsigned long)phy->usb2_phy_cfg,
				DEVICE_MODE, (unsigned long)phy->phy3_cfg);
			amlogic_m31_set_vbus_power(phy, 0);
			crg_gadget_init();
		} else if (controller_type == USB_HOST_ONLY) {
			set_mode((unsigned long)phy->usb2_phy_cfg,
				HOST_MODE, (unsigned long)phy->phy3_cfg);
			amlogic_m31_set_vbus_power(phy, 1);
			crg_init();
		}
	} else {
		/* The usb2_phy_cfg iddig bit is default 0 and may not change to 1 instantly
		 * with otg connecter plugged at boot time. Defer the mode init here to avoid
		 * excess host mode init.
		 */
		phy->mode_work_flag = 1;
		INIT_DELAYED_WORK(&phy->set_mode_work, amlogic_crg_otg_set_m_work);
		schedule_delayed_work(&phy->set_mode_work, msecs_to_jiffies(500));
	}

	return 0;
}

static int amlogic_crg_otg_remove(struct platform_device *pdev)
{
	return 0;
}

static int amlogic_crg_otg_freeze(struct device *dev)
{
	return 0;
}

static int amlogic_crg_otg_thaw(struct device *dev)
{
	struct amlogic_crg_otg *phy = platform_get_drvdata(to_platform_device(dev));

	writel(phy->pm_buf.usb_r1.d32, phy->phy3_cfg + 4);
	writel(phy->pm_buf.u2p_r2.d32, phy->usb2_phy_cfg + 8);
	writel(phy->pm_buf.u2p_r0.d32, phy->usb2_phy_cfg);

	/* Don't resync here because device_block_probing is still holded.
	 * if hibernation fails,  PM_POST_HIBERNATION will handle.
	 */
	return 0;
}

static int amlogic_crg_otg_restore(struct device *dev)
{
	return 0;
}

static int amlogic_crg_otg_hibernation_prepare(struct amlogic_crg_otg *phy)
{
	union u2p_r2_v2 reg2;

	reg2.d32 = readl((void __iomem *)((unsigned long)phy->usb2_phy_cfg + 8));
	phy->pm_buf.u2p_r2.d32 = reg2.d32;
	/* Quiesce otg func: clear&disable hwirq, cancel works. */
	if (phy->controller_type == USB_OTG) {
		reg2.b.usb_iddig_irq = 0;
		reg2.b.iddig_en0 = 0;
		reg2.b.iddig_en1 = 0;
		reg2.b.iddig_th = 0;
		writel(reg2.d32, (void __iomem *)((unsigned long)phy->usb2_phy_cfg + 8));
		cancel_delayed_work_sync(&phy->work);
	}

	phy->pm_buf.u2p_r0.d32 = readl(phy->usb2_phy_cfg);
	phy->pm_buf.usb_r1.d32 = readl(phy->phy3_cfg + 4);
	return 0;
}

static int amlogic_crg_otg_post_hibernation(struct amlogic_crg_otg *phy)
{
	//union u2p_r2_v2 reg2;

	//reg2.d32 = readl((void __iomem *)((unsigned long)phy->usb2_phy_cfg + 8));
	//pr_err("linestate: %d recover to %d\n",
	//	reg2.b.iddig_curr, phy->pm_buf.u2p_r2.b.iddig_curr);

	writel(phy->pm_buf.usb_r1.d32, phy->phy3_cfg + 4);
	writel(phy->pm_buf.u2p_r2.d32, phy->usb2_phy_cfg + 8);
	writel(phy->pm_buf.u2p_r0.d32, phy->usb2_phy_cfg);

	/* During STD the idpin line state could lose sync with controller state even the
	 * register has been restored. This happens when linestate changes after std but
	 * before h-prepare which disables the irq, records linesate then cancels the unscheduled
	 * role-switch irq work.
	 * e.g.
	 * If the otg plug is removed before h-prepare, the restored state remains as host
	 * even when the otg plug is still removed.
	 * Try to resync after restore.
	 */
	if (phy->controller_type == USB_OTG)
		schedule_delayed_work(&phy->work, msecs_to_jiffies(100));

	return 0;
}

/* crg_gadget_exit&crg_gadget_init should be synchronous to prevent racing with crg_udc_pm_cb.
 * However dpm_prepare defer all probes until dpm_complete, the probe inside is still asynchronous.
 * Avoid this by using pm_cb.
 */
static int amlogic_crg_otg_pm_cb(struct notifier_block *notifier,
			      unsigned long pm_event,
			      void *unused)
{
	struct amlogic_crg_otg *phy =
		container_of(notifier, struct amlogic_crg_otg, pm_notifier);

	pr_info("%s called. pm_event:%lu.\n", __func__, pm_event);

	switch (pm_event) {
	case PM_HIBERNATION_PREPARE:
		amlogic_crg_otg_hibernation_prepare(phy);
		break;
	case PM_POST_HIBERNATION:
		amlogic_crg_otg_post_hibernation(phy);
		break;
	case PM_SUSPEND_PREPARE:
	case PM_POST_SUSPEND:
	case PM_RESTORE_PREPARE:
	case PM_POST_RESTORE:
	default:
		break;
	}
	return NOTIFY_DONE;
}

#ifdef CONFIG_PM_RUNTIME
static int amlogic_crg_otg_runtime_suspend(struct device *dev)
{
	return 0;
}

static int amlogic_crg_otg_runtime_resume(struct device *dev)
{
	u32 ret = 0;

	return ret;
}
#endif

static const struct dev_pm_ops amlogic_crg_otg_pm_ops = {
#ifdef CONFIG_PM_RUNTIME
	SET_RUNTIME_PM_OPS(amlogic_crg_otg_runtime_suspend,
			   amlogic_crg_otg_runtime_resume,
			   NULL)
#endif
	.freeze = amlogic_crg_otg_freeze,
	.thaw = amlogic_crg_otg_thaw,
	.restore = amlogic_crg_otg_restore,
};

#define DEV_PM_OPS     (&amlogic_crg_otg_pm_ops)

#ifdef CONFIG_OF
static const struct of_device_id amlogic_crg_otg_id_table[] = {
	{ .compatible = "amlogic, amlogic-crg-otg" },
	{}
};
MODULE_DEVICE_TABLE(of, amlogic_crg_otg_id_table);
#endif

static struct platform_driver amlogic_crg_otg_driver = {
	.probe		= amlogic_crg_otg_probe,
	.remove		= amlogic_crg_otg_remove,
	.driver		= {
		.name	= "amlogic-crg-otg",
		.owner	= THIS_MODULE,
		.pm	= DEV_PM_OPS,
		.of_match_table = of_match_ptr(amlogic_crg_otg_id_table),
	},
};

int __init crg_otg_init(void)
{
	platform_driver_register(&amlogic_crg_otg_driver);

	return 0;
}
