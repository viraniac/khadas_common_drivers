// SPDX-License-Identifier: GPL-2.0-only
/*
 * Pin controller and GPIO driver for Amlogic Meson SoCs
 *
 * Copyright (C) 2014 Beniamino Galvani <b.galvani@gmail.com>
 */

/*
 * The available pins are organized in banks (A,B,C,D,E,X,Y,Z,AO,
 * BOOT,CARD for meson6, X,Y,DV,H,Z,AO,BOOT,CARD for meson8 and
 * X,Y,DV,H,AO,BOOT,CARD,DIF for meson8b) and each bank has a
 * variable number of pins.
 *
 * The AO bank is special because it belongs to the Always-On power
 * domain which can't be powered off; the bank also uses a set of
 * registers different from the other banks.
 *
 * For each pin controller there are 4 different register ranges that
 * control the following properties of the pins:
 *  1) pin muxing
 *  2) pull enable/disable
 *  3) pull up/down
 *  4) GPIO direction, output value, input value
 *
 * In some cases the register ranges for pull enable and pull
 * direction are the same and thus there are only 3 register ranges.
 *
 * Since Meson G12A SoC, the ao register ranges for gpio, pull enable
 * and pull direction are the same, so there are only 2 register ranges.
 *
 * For the pull and GPIO configuration every bank uses a contiguous
 * set of bits in the register sets described above; the same register
 * can be shared by more banks with different offsets.
 *
 * In addition to this there are some registers shared between all
 * banks that control the IRQ functionality. This feature is not
 * supported at the moment by the driver.
 */

#include <linux/device.h>
#include <linux/gpio/driver.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/pinctrl/pinconf-generic.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/seq_file.h>

#include <pinctrl/core.h>
#include <pinctrl/pinctrl-utils.h>
#include "pinctrl-meson.h"

#ifdef CONFIG_AMLOGIC_MODIFY
#define PIN_CONFIG_VIN_THRESHOLD	(PIN_CONFIG_END + 1)

static const struct pinconf_generic_params meson_extr_pinconf[] = {
	{"vin-threshold",	PIN_CONFIG_VIN_THRESHOLD,	0},
};

#ifdef CONFIG_DEBUG_FS
static const struct pin_config_item meson_extr_conf_items[] = {
	PCONFDUMP(PIN_CONFIG_VIN_THRESHOLD, "vin threshold", NULL, true),
};
#endif

static int meson_memory_duplicate(struct platform_device *pdev, void **addr,
				  size_t n, size_t size)
{
	void *mem;

	if (!(*addr))
		return -EINVAL;

	mem = devm_kzalloc(&pdev->dev, size * n, GFP_KERNEL);
	if (!mem)
		return -ENOMEM;

	memcpy(mem, *addr, size * n);
	*addr = mem;

	return 0;
};
#endif

static const unsigned int meson_bit_strides[] = {
#ifndef CONFIG_AMLOGIC_MODIFY
	1, 1, 1, 1, 1, 2, 1
#else
	1, 1, 1, 1, 1, 2, 1, 1
#endif
};

/**
 * meson_get_bank() - find the bank containing a given pin
 *
 * @pc:		the pinctrl instance
 * @pin:	the pin number
 * @bank:	the found bank
 *
 * Return:	0 on success, a negative value on error
 */
static int meson_get_bank(struct meson_pinctrl *pc, unsigned int pin,
			  struct meson_bank **bank)
{
	int i;

	for (i = 0; i < pc->data->num_banks; i++) {
		if (pin >= pc->data->banks[i].first &&
		    pin <= pc->data->banks[i].last) {
			*bank = &pc->data->banks[i];
			return 0;
		}
	}

	return -EINVAL;
}

/**
 * meson_calc_reg_and_bit() - calculate register and bit for a pin
 *
 * @bank:	the bank containing the pin
 * @pin:	the pin number
 * @reg_type:	the type of register needed (pull-enable, pull, etc...)
 * @reg:	the computed register offset
 * @bit:	the computed bit
 */
static void meson_calc_reg_and_bit(struct meson_bank *bank, unsigned int pin,
				   enum meson_reg_type reg_type,
				   unsigned int *reg, unsigned int *bit)
{
	struct meson_reg_desc *desc = &bank->regs[reg_type];

	*bit = (desc->bit + pin - bank->first) * meson_bit_strides[reg_type];
	*reg = (desc->reg + (*bit / 32)) * 4;
	*bit &= 0x1f;
}

static int meson_get_groups_count(struct pinctrl_dev *pcdev)
{
	struct meson_pinctrl *pc = pinctrl_dev_get_drvdata(pcdev);

	return pc->data->num_groups;
}

static const char *meson_get_group_name(struct pinctrl_dev *pcdev,
					unsigned int selector)
{
	struct meson_pinctrl *pc = pinctrl_dev_get_drvdata(pcdev);

	return pc->data->groups[selector].name;
}

static int meson_get_group_pins(struct pinctrl_dev *pcdev, unsigned int selector,
				const unsigned int **pins, unsigned int *num_pins)
{
	struct meson_pinctrl *pc = pinctrl_dev_get_drvdata(pcdev);

	*pins = pc->data->groups[selector].pins;
	*num_pins = pc->data->groups[selector].num_pins;

	return 0;
}

static void meson_pin_dbg_show(struct pinctrl_dev *pcdev, struct seq_file *s,
			       unsigned int offset)
{
	seq_printf(s, " %s", dev_name(pcdev->dev));
}

static const struct pinctrl_ops meson_pctrl_ops = {
	.get_groups_count	= meson_get_groups_count,
	.get_group_name		= meson_get_group_name,
	.get_group_pins		= meson_get_group_pins,
	.dt_node_to_map		= pinconf_generic_dt_node_to_map_all,
	.dt_free_map		= pinctrl_utils_free_map,
	.pin_dbg_show		= meson_pin_dbg_show,
};

int meson_pmx_get_funcs_count(struct pinctrl_dev *pcdev)
{
	struct meson_pinctrl *pc = pinctrl_dev_get_drvdata(pcdev);

	return pc->data->num_funcs;
}
EXPORT_SYMBOL(meson_pmx_get_funcs_count);

const char *meson_pmx_get_func_name(struct pinctrl_dev *pcdev,
				    unsigned int selector)
{
	struct meson_pinctrl *pc = pinctrl_dev_get_drvdata(pcdev);

	return pc->data->funcs[selector].name;
}
EXPORT_SYMBOL(meson_pmx_get_func_name);

int meson_pmx_get_groups(struct pinctrl_dev *pcdev, unsigned int selector,
			 const char * const **groups,
			 unsigned * const num_groups)
{
	struct meson_pinctrl *pc = pinctrl_dev_get_drvdata(pcdev);

	*groups = pc->data->funcs[selector].groups;
	*num_groups = pc->data->funcs[selector].num_groups;

	return 0;
}
EXPORT_SYMBOL(meson_pmx_get_groups);

static int meson_pinconf_set_gpio_bit(struct meson_pinctrl *pc,
				      unsigned int pin,
				      unsigned int reg_type,
				      bool arg)
{
	struct meson_bank *bank;
	unsigned int reg, bit;
	int ret;

	ret = meson_get_bank(pc, pin, &bank);
	if (ret)
		return ret;

	meson_calc_reg_and_bit(bank, pin, reg_type, &reg, &bit);
	return regmap_update_bits(pc->reg_gpio, reg, BIT(bit),
				  arg ? BIT(bit) : 0);
}

static int meson_pinconf_get_gpio_bit(struct meson_pinctrl *pc,
				      unsigned int pin,
				      unsigned int reg_type)
{
	struct meson_bank *bank;
	unsigned int reg, bit, val;
	int ret;

	ret = meson_get_bank(pc, pin, &bank);
	if (ret)
		return ret;

	meson_calc_reg_and_bit(bank, pin, reg_type, &reg, &bit);
	ret = regmap_read(pc->reg_gpio, reg, &val);
	if (ret)
		return ret;

	return BIT(bit) & val ? 1 : 0;
}

static int meson_pinconf_set_output(struct meson_pinctrl *pc,
				    unsigned int pin,
				    bool out)
{
	return meson_pinconf_set_gpio_bit(pc, pin, REG_DIR, !out);
}

static int meson_pinconf_get_output(struct meson_pinctrl *pc,
				    unsigned int pin)
{
	int ret = meson_pinconf_get_gpio_bit(pc, pin, REG_DIR);

	if (ret < 0)
		return ret;

	return !ret;
}

static int meson_pinconf_set_drive(struct meson_pinctrl *pc,
				   unsigned int pin,
				   bool high)
{
	return meson_pinconf_set_gpio_bit(pc, pin, REG_OUT, high);
}

static int meson_pinconf_get_drive(struct meson_pinctrl *pc,
				   unsigned int pin)
{
	return meson_pinconf_get_gpio_bit(pc, pin, REG_OUT);
}

static int meson_pinconf_set_output_drive(struct meson_pinctrl *pc,
					  unsigned int pin,
					  bool high)
{
	int ret;

#ifndef CONFIG_AMLOGIC_MODIFY
	ret = meson_pinconf_set_output(pc, pin, true);
	if (ret)
		return ret;

	return meson_pinconf_set_drive(pc, pin, high);
#else
	ret = meson_pinconf_set_drive(pc, pin, high);
	if (ret)
		return ret;

	return meson_pinconf_set_output(pc, pin, true);
#endif
}

static int meson_pinconf_disable_bias(struct meson_pinctrl *pc,
				      unsigned int pin)
{
	struct meson_bank *bank;
	unsigned int reg, bit = 0;
	int ret;

	ret = meson_get_bank(pc, pin, &bank);
	if (ret)
		return ret;

	meson_calc_reg_and_bit(bank, pin, REG_PULLEN, &reg, &bit);
	ret = regmap_update_bits(pc->reg_pullen, reg, BIT(bit), 0);
	if (ret)
		return ret;

	return 0;
}

static int meson_pinconf_enable_bias(struct meson_pinctrl *pc, unsigned int pin,
				     bool pull_up)
{
	struct meson_bank *bank;
	unsigned int reg, bit, val = 0;
	int ret;

	ret = meson_get_bank(pc, pin, &bank);
	if (ret)
		return ret;

	meson_calc_reg_and_bit(bank, pin, REG_PULL, &reg, &bit);
	if (pull_up)
		val = BIT(bit);

	ret = regmap_update_bits(pc->reg_pull, reg, BIT(bit), val);
	if (ret)
		return ret;

	meson_calc_reg_and_bit(bank, pin, REG_PULLEN, &reg, &bit);
	ret = regmap_update_bits(pc->reg_pullen, reg, BIT(bit),	BIT(bit));
	if (ret)
		return ret;

	return 0;
}

static int meson_pinconf_set_drive_strength(struct meson_pinctrl *pc,
					    unsigned int pin,
					    u16 drive_strength_ua)
{
	struct meson_bank *bank;
	unsigned int reg, bit, ds_val;
	int ret;

	if (!pc->reg_ds) {
		dev_err(pc->dev, "drive-strength not supported\n");
		return -ENOTSUPP;
	}

	ret = meson_get_bank(pc, pin, &bank);
	if (ret)
		return ret;

	meson_calc_reg_and_bit(bank, pin, REG_DS, &reg, &bit);

	if (drive_strength_ua <= 500) {
		ds_val = MESON_PINCONF_DRV_500UA;
	} else if (drive_strength_ua <= 2500) {
		ds_val = MESON_PINCONF_DRV_2500UA;
	} else if (drive_strength_ua <= 3000) {
		ds_val = MESON_PINCONF_DRV_3000UA;
	} else if (drive_strength_ua <= 4000) {
		ds_val = MESON_PINCONF_DRV_4000UA;
	} else {
		dev_warn_once(pc->dev,
			      "pin %u: invalid drive-strength : %d , default to 4mA\n",
			      pin, drive_strength_ua);
		ds_val = MESON_PINCONF_DRV_4000UA;
	}

	ret = regmap_update_bits(pc->reg_ds, reg, 0x3 << bit, ds_val << bit);
	if (ret)
		return ret;

	return 0;
}

#ifdef CONFIG_AMLOGIC_MODIFY
static int  meson_pinconf_set_vthx(struct meson_pinctrl *pc, unsigned int pin,
				   u32 ref_arg)
{
	struct meson_bank *bank;
	unsigned int reg, bit;
	int ret;

	if (!pc->reg_vthx) {
		dev_err(pc->dev, "vin-threshold not supported\n");
		return -EOPNOTSUPP;
	}

	ret = meson_get_bank(pc, pin, &bank);
	if (ret)
		return ret;
	meson_calc_reg_and_bit(bank, pin, REG_VTHX, &reg,
			       &bit);
	return regmap_update_bits(pc->reg_vthx, reg, BIT(bit),
				  !!ref_arg ? BIT(bit) : 0);
}

static int meson_pinconf_get_vthx(struct meson_pinctrl *pc, unsigned int pin,
				  u16 *arg)
{
	struct meson_bank *bank;
	unsigned int reg, bit, val;
	int ret;

	if (!pc->reg_vthx)
		return -EOPNOTSUPP;

	ret = meson_get_bank(pc, pin, &bank);
	if (ret)
		return ret;

	meson_calc_reg_and_bit(bank, pin, REG_VTHX, &reg, &bit);

	ret = regmap_read(pc->reg_pullen, reg, &val);
	if (ret)
		return ret;

	*arg = !!(val & BIT(bit));

	return 0;
}

#endif

static int meson_pinconf_set(struct pinctrl_dev *pcdev, unsigned int pin,
			     unsigned long *configs, unsigned int num_configs)
{
	struct meson_pinctrl *pc = pinctrl_dev_get_drvdata(pcdev);
	unsigned int param;
	unsigned int arg = 0;
	int i, ret;

	for (i = 0; i < num_configs; i++) {
		param = pinconf_to_config_param(configs[i]);

		switch (param) {
		case PIN_CONFIG_DRIVE_STRENGTH_UA:
#ifdef CONFIG_AMLOGIC_MODIFY
		case PIN_CONFIG_INPUT_ENABLE:
		case PIN_CONFIG_VIN_THRESHOLD:
#endif
		case PIN_CONFIG_OUTPUT_ENABLE:
		case PIN_CONFIG_OUTPUT:
			arg = pinconf_to_config_argument(configs[i]);
			break;

		default:
			break;
		}

		switch (param) {
		case PIN_CONFIG_BIAS_DISABLE:
			ret = meson_pinconf_disable_bias(pc, pin);
			break;
		case PIN_CONFIG_BIAS_PULL_UP:
			ret = meson_pinconf_enable_bias(pc, pin, true);
			break;
		case PIN_CONFIG_BIAS_PULL_DOWN:
			ret = meson_pinconf_enable_bias(pc, pin, false);
			break;
		case PIN_CONFIG_DRIVE_STRENGTH_UA:
			ret = meson_pinconf_set_drive_strength(pc, pin, arg);
			break;
		case PIN_CONFIG_OUTPUT_ENABLE:
			ret = meson_pinconf_set_output(pc, pin, arg);
			break;
		case PIN_CONFIG_OUTPUT:
			ret = meson_pinconf_set_output_drive(pc, pin, arg);
			break;
#ifdef CONFIG_AMLOGIC_MODIFY
		case PIN_CONFIG_INPUT_ENABLE:
			ret = meson_pinconf_set_output(pc, pin, !arg);
			break;
		case PIN_CONFIG_VIN_THRESHOLD:
			ret =  meson_pinconf_set_vthx(pc, pin, arg);
			break;
#endif
		default:
			ret = -ENOTSUPP;
		}

		if (ret)
			return ret;
	}

	return 0;
}

static int meson_pinconf_get_pull(struct meson_pinctrl *pc, unsigned int pin)
{
	struct meson_bank *bank;
	unsigned int reg, bit, val;
	int ret, conf;

	ret = meson_get_bank(pc, pin, &bank);
	if (ret)
		return ret;

	meson_calc_reg_and_bit(bank, pin, REG_PULLEN, &reg, &bit);

	ret = regmap_read(pc->reg_pullen, reg, &val);
	if (ret)
		return ret;

	if (!(val & BIT(bit))) {
		conf = PIN_CONFIG_BIAS_DISABLE;
	} else {
		meson_calc_reg_and_bit(bank, pin, REG_PULL, &reg, &bit);

		ret = regmap_read(pc->reg_pull, reg, &val);
		if (ret)
			return ret;

		if (val & BIT(bit))
			conf = PIN_CONFIG_BIAS_PULL_UP;
		else
			conf = PIN_CONFIG_BIAS_PULL_DOWN;
	}

	return conf;
}

static int meson_pinconf_get_drive_strength(struct meson_pinctrl *pc,
					    unsigned int pin,
					    u16 *drive_strength_ua)
{
	struct meson_bank *bank;
	unsigned int reg, bit;
	unsigned int val;
	int ret;

	if (!pc->reg_ds)
		return -ENOTSUPP;

	ret = meson_get_bank(pc, pin, &bank);
	if (ret)
		return ret;

	meson_calc_reg_and_bit(bank, pin, REG_DS, &reg, &bit);

	ret = regmap_read(pc->reg_ds, reg, &val);
	if (ret)
		return ret;

	switch ((val >> bit) & 0x3) {
	case MESON_PINCONF_DRV_500UA:
		*drive_strength_ua = 500;
		break;
	case MESON_PINCONF_DRV_2500UA:
		*drive_strength_ua = 2500;
		break;
	case MESON_PINCONF_DRV_3000UA:
		*drive_strength_ua = 3000;
		break;
	case MESON_PINCONF_DRV_4000UA:
		*drive_strength_ua = 4000;
		break;
	}

	return 0;
}

static int meson_pinconf_get(struct pinctrl_dev *pcdev, unsigned int pin,
			     unsigned long *config)
{
	struct meson_pinctrl *pc = pinctrl_dev_get_drvdata(pcdev);
	unsigned int param = pinconf_to_config_param(*config);

	u16 arg;
	int ret;

	switch (param) {
	case PIN_CONFIG_BIAS_DISABLE:
	case PIN_CONFIG_BIAS_PULL_DOWN:
	case PIN_CONFIG_BIAS_PULL_UP:
		if (meson_pinconf_get_pull(pc, pin) == param)
			arg = 1;
		else
			return -EINVAL;
		break;
	case PIN_CONFIG_DRIVE_STRENGTH_UA:
		ret = meson_pinconf_get_drive_strength(pc, pin, &arg);
		if (ret)
			return ret;
		break;
	case PIN_CONFIG_OUTPUT_ENABLE:
		ret = meson_pinconf_get_output(pc, pin);
		if (ret <= 0)
			return -EINVAL;
		arg = 1;
		break;
	case PIN_CONFIG_OUTPUT:
		ret = meson_pinconf_get_output(pc, pin);
		if (ret <= 0)
			return -EINVAL;

		ret = meson_pinconf_get_drive(pc, pin);
		if (ret < 0)
			return -EINVAL;

		arg = ret;
		break;
#ifdef CONFIG_AMLOGIC_MODIFY
	case PIN_CONFIG_VIN_THRESHOLD:
		if (meson_pinconf_get_vthx(pc, pin, &arg) < 0)
			return -EINVAL;
		break;
#endif
	default:
		return -ENOTSUPP;
	}

	*config = pinconf_to_config_packed(param, arg);
	dev_dbg(pc->dev, "pinconf for pin %u is %lu\n", pin, *config);

	return 0;
}

static int meson_pinconf_group_set(struct pinctrl_dev *pcdev,
				   unsigned int num_group,
				   unsigned long *configs, unsigned int num_configs)
{
	struct meson_pinctrl *pc = pinctrl_dev_get_drvdata(pcdev);
	struct meson_pmx_group *group = &pc->data->groups[num_group];
	int i;

	dev_dbg(pc->dev, "set pinconf for group %s\n", group->name);

	for (i = 0; i < group->num_pins; i++) {
		meson_pinconf_set(pcdev, group->pins[i], configs,
				  num_configs);
	}

	return 0;
}

static int meson_pinconf_group_get(struct pinctrl_dev *pcdev,
				   unsigned int group, unsigned long *config)
{
	return -ENOTSUPP;
}

static const struct pinconf_ops meson_pinconf_ops = {
	.pin_config_get		= meson_pinconf_get,
	.pin_config_set		= meson_pinconf_set,
	.pin_config_group_get	= meson_pinconf_group_get,
	.pin_config_group_set	= meson_pinconf_group_set,
	.is_generic		= true,
};

static int meson_gpio_direction_input(struct gpio_chip *chip, unsigned int gpio)
{
	return meson_pinconf_set_output(gpiochip_get_data(chip), gpio, false);
}

static int meson_gpio_direction_output(struct gpio_chip *chip, unsigned int gpio,
				       int value)
{
	return meson_pinconf_set_output_drive(gpiochip_get_data(chip),
					      gpio, value);
}

static void meson_gpio_set(struct gpio_chip *chip, unsigned int gpio, int value)
{
	meson_pinconf_set_drive(gpiochip_get_data(chip), gpio, value);
}

static int meson_gpio_get(struct gpio_chip *chip, unsigned int gpio)
{
	struct meson_pinctrl *pc = gpiochip_get_data(chip);
	unsigned int reg, bit, val;
	struct meson_bank *bank;
	int ret;

	ret = meson_get_bank(pc, gpio, &bank);
	if (ret)
		return ret;

	meson_calc_reg_and_bit(bank, gpio, REG_IN, &reg, &bit);
	ret = regmap_read(pc->reg_gpio, reg, &val);
	if (ret)
		return ret;

	return !!(val & BIT(bit));
}

#ifdef CONFIG_AMLOGIC_MODIFY
static int meson_gpio_to_irq(struct gpio_chip *chip, unsigned int gpio)
{
	struct meson_pinctrl *pc = gpiochip_get_data(chip);
	struct meson_bank *bank;
	struct irq_fwspec fwspec;
	int hwirq;

	if (meson_get_bank(pc, gpio, &bank))
		return -EINVAL;

	if (bank->irq_first < 0) {
		dev_warn(pc->dev, "no support irq for pin[%d]\n", gpio);
		return -EINVAL;
	}

	if (!pc->of_irq) {
		dev_err(pc->dev, "invalid device node of gpio INTC\n");
		return -EINVAL;
	}

	hwirq = gpio - bank->first + bank->irq_first;

	fwspec.fwnode = of_node_to_fwnode(pc->of_irq);
	fwspec.param_count = 2;
	fwspec.param[0] = hwirq;
	fwspec.param[1] = IRQ_TYPE_NONE;

	return irq_create_fwspec_mapping(&fwspec);
}
#endif

static int meson_gpiolib_register(struct meson_pinctrl *pc)
{
	int ret;
#ifdef FIX_IT_LATER
	/* KASAN BUG Fix It Later */
	const char **names;
	const struct pinctrl_pin_desc *pins;
	int i;
#endif

	pc->chip.label = pc->data->name;
	pc->chip.parent = pc->dev;
	pc->chip.request = gpiochip_generic_request;
	pc->chip.free = gpiochip_generic_free;
	pc->chip.direction_input = meson_gpio_direction_input;
	pc->chip.direction_output = meson_gpio_direction_output;
	pc->chip.get = meson_gpio_get;
	pc->chip.set = meson_gpio_set;
#ifdef CONFIG_AMLOGIC_MODIFY
	pc->chip.to_irq = meson_gpio_to_irq;
	pc->chip.set_config = gpiochip_generic_config;
/* KASAN BUG Fix It Later */
#ifdef FIX_IT_LATER
	names = kcalloc(pc->desc.npins, sizeof(char *), GFP_KERNEL);
	if (!names)
		return -ENOMEM;
	pins = pc->desc.pins;
	for (i = 0; i < pc->desc.npins; i++)
		names[pins[i].number] = pins[i].name;
	pc->chip.names = (const char * const *)names;
#endif
#endif
	pc->chip.base = -1;
	pc->chip.ngpio = pc->data->num_pins;
	pc->chip.can_sleep = false;
	pc->chip.of_node = pc->of_node;
	pc->chip.of_gpio_n_cells = 2;

	ret = gpiochip_add_data(&pc->chip, pc);
/* KASAN BUG Fix It Later */
#ifdef FIX_IT_LATER
	/* pin->chip.names will be assigned to each gpio discriptor' name
	 * member after gpiochip_add_data. To keep node name consistency when
	 * use sysfs to export gpio, pc->chip.name need to be cleared also see
	 * gpiod_export->device_create_with_groups.
	 */
	kfree(names);
	names = NULL;
	pc->chip.names = NULL;
#endif
	if (ret) {
		dev_err(pc->dev, "can't add gpio chip %s\n",
			pc->data->name);
		return ret;
	}

	return 0;
}

static struct regmap_config meson_regmap_config = {
	.reg_bits = 32,
	.val_bits = 32,
	.reg_stride = 4,
};

static struct regmap *meson_map_resource(struct meson_pinctrl *pc,
					 struct device_node *node, char *name)
{
	struct resource res;
	void __iomem *base;
	int i;

	i = of_property_match_string(node, "reg-names", name);
	if (of_address_to_resource(node, i, &res))
		return NULL;

	base = devm_ioremap_resource(pc->dev, &res);
	if (IS_ERR(base))
		return ERR_CAST(base);

	meson_regmap_config.max_register = resource_size(&res) - 4;
	meson_regmap_config.name = devm_kasprintf(pc->dev, GFP_KERNEL,
						  "%pOFn-%s", node,
						  name);
	if (!meson_regmap_config.name)
		return ERR_PTR(-ENOMEM);

	return devm_regmap_init_mmio(pc->dev, base, &meson_regmap_config);
}

static int meson_pinctrl_parse_dt(struct meson_pinctrl *pc,
				  struct device_node *node)
{
	struct device_node *np, *gpio_np = NULL;

	for_each_child_of_node(node, np) {
		if (!of_find_property(np, "gpio-controller", NULL))
			continue;
		if (gpio_np) {
			dev_err(pc->dev, "multiple gpio nodes\n");
			of_node_put(np);
			return -EINVAL;
		}
		gpio_np = np;
	}

	if (!gpio_np) {
		dev_err(pc->dev, "no gpio node found\n");
		return -EINVAL;
	}

	pc->of_node = gpio_np;

#ifdef CONFIG_AMLOGIC_MODIFY
	pc->of_irq = of_find_compatible_node(NULL,
					     NULL,
					     "amlogic,meson-gpio-intc-ext");
	if (!pc->of_irq)
		pc->of_irq = of_find_compatible_node(NULL,
						     NULL,
						     "amlogic,meson-gpio-intc");
#endif

	pc->reg_mux = meson_map_resource(pc, gpio_np, "mux");
	if (IS_ERR_OR_NULL(pc->reg_mux)) {
		dev_err(pc->dev, "mux registers not found\n");
		return pc->reg_mux ? PTR_ERR(pc->reg_mux) : -ENOENT;
	}

	pc->reg_gpio = meson_map_resource(pc, gpio_np, "gpio");
	if (IS_ERR_OR_NULL(pc->reg_gpio)) {
		dev_err(pc->dev, "gpio registers not found\n");
		return pc->reg_gpio ? PTR_ERR(pc->reg_gpio) : -ENOENT;
	}

	pc->reg_pull = meson_map_resource(pc, gpio_np, "pull");
	if (IS_ERR(pc->reg_pull))
		pc->reg_pull = NULL;

	pc->reg_pullen = meson_map_resource(pc, gpio_np, "pull-enable");
	if (IS_ERR(pc->reg_pullen))
		pc->reg_pullen = NULL;

	pc->reg_ds = meson_map_resource(pc, gpio_np, "ds");
	if (IS_ERR(pc->reg_ds)) {
		dev_dbg(pc->dev, "ds registers not found - skipping\n");
		pc->reg_ds = NULL;
	}

#ifdef CONFIG_AMLOGIC_MODIFY
	if (of_property_read_bool(node, "amlogic,vin-threshold-support"))
		pc->reg_vthx = pc->reg_gpio;
#endif

	if (pc->data->parse_dt)
		return pc->data->parse_dt(pc);

	return 0;
}

int meson8_aobus_parse_dt_extra(struct meson_pinctrl *pc)
{
	if (!pc->reg_pull)
		return -EINVAL;

	pc->reg_pullen = pc->reg_pull;

	return 0;
}

#ifdef CONFIG_AMLOGIC_MODIFY
/*g12a/b/sm1/tm2/t5/t5d/ like old platform */
int meson_g12a_aobus_parse_dt_extra(struct meson_pinctrl *pc)
{
	pc->reg_pull = pc->reg_gpio;
	pc->reg_pullen = pc->reg_gpio;

	return 0;
}
EXPORT_SYMBOL(meson_g12a_aobus_parse_dt_extra);

int meson_t5w_workaround_parse_dt_extra(struct meson_pinctrl *pc)
{
	pc->reg_ds = pc->reg_mux;

	return 0;
}
EXPORT_SYMBOL(meson_t5w_workaround_parse_dt_extra);
#endif

int meson_a1_parse_dt_extra(struct meson_pinctrl *pc)
{
	pc->reg_pull = pc->reg_gpio;
	pc->reg_pullen = pc->reg_gpio;
	pc->reg_ds = pc->reg_gpio;

	return 0;
}
EXPORT_SYMBOL(meson_a1_parse_dt_extra);

int meson_pinctrl_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct meson_pinctrl *pc;
	int ret;

	pc = devm_kzalloc(dev, sizeof(struct meson_pinctrl), GFP_KERNEL);
	if (!pc)
		return -ENOMEM;

	pc->dev = dev;
	pc->data = (struct meson_pinctrl_data *)of_device_get_match_data(dev);

	ret = meson_pinctrl_parse_dt(pc, dev->of_node);
	if (ret)
		return ret;
#ifdef CONFIG_AMLOGIC_MODIFY
	ret = meson_memory_duplicate(pdev, (void **)&pc->data->groups, pc->data->num_groups,
				     sizeof(struct meson_pmx_group));
	if (ret)
		return ret;

	ret = meson_memory_duplicate(pdev, (void **)&pc->data->funcs, pc->data->num_funcs,
				     sizeof(struct meson_pmx_func));
	if (ret)
		return ret;
	pc->desc.num_custom_params = ARRAY_SIZE(meson_extr_pinconf);
	pc->desc.custom_params = meson_extr_pinconf;
#ifdef CONFIG_DEBUG_FS
	pc->desc.custom_conf_items = meson_extr_conf_items;
#endif
#endif
	pc->desc.name		= "pinctrl-meson";
	pc->desc.owner		= THIS_MODULE;
	pc->desc.pctlops	= &meson_pctrl_ops;
	pc->desc.pmxops		= pc->data->pmx_ops;
	pc->desc.confops	= &meson_pinconf_ops;
	pc->desc.pins		= pc->data->pins;
	pc->desc.npins		= pc->data->num_pins;

	pc->pcdev = devm_pinctrl_register(pc->dev, &pc->desc, pc);
	if (IS_ERR(pc->pcdev)) {
		dev_err(pc->dev, "can't register pinctrl device");
		return PTR_ERR(pc->pcdev);
	}

	return meson_gpiolib_register(pc);
}
EXPORT_SYMBOL(meson_pinctrl_probe);
