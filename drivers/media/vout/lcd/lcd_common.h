/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *
 * Copyright (C) 2019 Amlogic, Inc. All rights reserved.
 *
 */

#ifndef __AML_LCD_COMMON_H__
#define __AML_LCD_COMMON_H__
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/amlogic/media/vout/lcd/lcd_vout.h>
#include "lcd_clk_config.h"

/* 20220430: initial version*/
/* 20220610: add c3 support*/
/* 20220619: c3 mipi-dsi display ok*/
/* 20220622: c3 support bt656/1120*/
/* 20220916: port k5.4 code to k5.15*/
/* 20221012: correct t5w vbyone reset reg*/
/* 20221028: fix lane lock && fix t7 mipi lprx reg set*/
/* 20221111: modify edp transmit_unit_size to 48(temporary)*/
/* 20221115: support force unfit mipi-dsi bit_rate_max*/
/* 20221116: add pinmux lock for c3*/
#define LCD_DRV_VERSION    "20221116"

extern struct mutex lcd_vout_mutex;
extern spinlock_t lcd_reg_spinlock;
extern int lcd_vout_serve_bypass;

static inline unsigned int lcd_do_div(unsigned long long num, unsigned int den)
{
	unsigned long long ret = num;

	do_div(ret, den);

	return (unsigned int)ret;
}

/* lcd common */
void lcd_delay_us(int us);
void lcd_delay_ms(int ms);
unsigned char aml_lcd_i2c_bus_get_str(const char *str);
int lcd_type_str_to_type(const char *str);
char *lcd_type_type_to_str(int type);
unsigned char lcd_mode_str_to_mode(const char *str);
char *lcd_mode_mode_to_str(int mode);
u8 *lcd_vmap(ulong addr, u32 size);
void lcd_unmap_phyaddr(u8 *vaddr);

void lcd_cpu_gpio_probe(struct aml_lcd_drv_s *pdrv, unsigned int index);
void lcd_cpu_gpio_set(struct aml_lcd_drv_s *pdrv, unsigned int index, int value);
unsigned int lcd_cpu_gpio_get(struct aml_lcd_drv_s *pdrv, unsigned int index);
void lcd_rgb_pinmux_set(struct aml_lcd_drv_s *pdrv, int status);
void lcd_bt656_pinmux_set(struct aml_lcd_drv_s *pdrv, int status);
void lcd_bt1120_pinmux_set(struct aml_lcd_drv_s *pdrv, int status);
void lcd_vbyone_pinmux_set(struct aml_lcd_drv_s *pdrv, int status);
void lcd_mlvds_pinmux_set(struct aml_lcd_drv_s *pdrv, int status);
void lcd_p2p_pinmux_set(struct aml_lcd_drv_s *pdrv, int status);
void lcd_edp_pinmux_set(struct aml_lcd_drv_s *pdrv, int status);
void lcd_mipi_pinmux_set(struct aml_lcd_drv_s *pdrv, int status);

int lcd_base_config_load_from_dts(struct aml_lcd_drv_s *pdrv);
int lcd_get_config(struct aml_lcd_drv_s *pdrv);
void lcd_optical_vinfo_update(struct aml_lcd_drv_s *pdrv);

void lcd_vbyone_config_set(struct aml_lcd_drv_s *pdrv);
void lcd_mlvds_config_set(struct aml_lcd_drv_s *pdrv);
void lcd_p2p_config_set(struct aml_lcd_drv_s *pdrv);
void lcd_mipi_dsi_config_set(struct aml_lcd_drv_s *pdrv);
void lcd_edp_config_set(struct aml_lcd_drv_s *pdrv);
void lcd_vrr_config_update(struct aml_lcd_drv_s *pdrv);
void lcd_basic_timing_range_init(struct aml_lcd_drv_s *pdrv);
void lcd_timing_init_config(struct aml_lcd_drv_s *pdrv);

int lcd_fr_is_fixed(struct aml_lcd_drv_s *pdrv);
int lcd_vmode_change(struct aml_lcd_drv_s *pdrv);
void lcd_clk_change(struct aml_lcd_drv_s *pdrv);
void lcd_if_enable_retry(struct aml_lcd_drv_s *pdrv);
void lcd_vout_notify_mode_change_pre(struct aml_lcd_drv_s *pdrv);
void lcd_vout_notify_mode_change(struct aml_lcd_drv_s *pdrv);
void lcd_vinfo_update(struct aml_lcd_drv_s *pdrv);
unsigned int lcd_vrr_lfc_switch(void *dev_data, int fps);
int lcd_vrr_disable_cb(void *dev_data);
void lcd_vrr_dev_update(struct aml_lcd_drv_s *pdrv);

void lcd_queue_work(struct work_struct *work);
inline void lcd_queue_delayed_work(struct delayed_work *delayed_work, int ms);
unsigned int cal_crc32(unsigned int crc, const unsigned char *buf, int buf_len);

/* lcd phy */
void lcd_phy_tcon_chpi_bbc_init_tl1(struct lcd_config_s *pconf);
void lcd_phy_set(struct aml_lcd_drv_s *pdrv, int status);
int lcd_phy_probe(struct aml_lcd_drv_s *pdrv);
int lcd_phy_config_init(struct aml_lcd_drv_s *pdrv);
unsigned int lcd_phy_vswing_level_to_value(struct aml_lcd_drv_s *pdrv, unsigned int level);
unsigned int lcd_phy_preem_level_to_value(struct aml_lcd_drv_s *pdrv, unsigned int level);

/*lcd vbyone*/
void lcd_vbyone_enable_dft(struct aml_lcd_drv_s *pdrv);
void lcd_vbyone_disable_dft(struct aml_lcd_drv_s *pdrv);
void lcd_vbyone_enable_t7(struct aml_lcd_drv_s *pdrv);
void lcd_vbyone_disable_t7(struct aml_lcd_drv_s *pdrv);
void lcd_vbyone_link_maintain_clear(void);
void lcd_vbyone_wait_timing_stable(struct aml_lcd_drv_s *pdrv);
void lcd_vbyone_cdr_training_hold(struct aml_lcd_drv_s *pdrv, int flag);
void lcd_vbyone_wait_hpd(struct aml_lcd_drv_s *pdrv);
void lcd_vbyone_power_on_wait_stable(struct aml_lcd_drv_s *pdrv);
void lcd_vbyone_wait_stable(struct aml_lcd_drv_s *pdrv);
void lcd_vbyone_interrupt_enable(struct aml_lcd_drv_s *pdrv, int flag);
int lcd_vbyone_interrupt_up(struct aml_lcd_drv_s *pdrv);
void lcd_vbyone_interrupt_down(struct aml_lcd_drv_s *pdrv);
void lcd_vbyone_debug_cdr(struct aml_lcd_drv_s *pdrv);
void lcd_vbyone_debug_lock(struct aml_lcd_drv_s *pdrv);
void lcd_vbyone_debug_reset(struct aml_lcd_drv_s *pdrv);

/* lcd tcon */
unsigned int lcd_tcon_reg_read(struct aml_lcd_drv_s *pdrv, unsigned int addr);
void lcd_tcon_reg_write(struct aml_lcd_drv_s *pdrv,
			unsigned int addr, unsigned int val);
int lcd_tcon_probe(struct aml_lcd_drv_s *pdrv);
void lcd_tcon_global_reset(struct aml_lcd_drv_s *pdrv);
int lcd_tcon_gamma_set_pattern(struct aml_lcd_drv_s *pdrv,
			       unsigned int bit_width, unsigned int gamma_r,
			       unsigned int gamma_g, unsigned int gamma_b);
unsigned int lcd_tcon_table_read(unsigned int addr);
unsigned int lcd_tcon_table_write(unsigned int addr, unsigned int val);
int lcd_tcon_core_update(struct aml_lcd_drv_s *pdrv);
int lcd_tcon_od_set(struct aml_lcd_drv_s *pdrv, int flag);
int lcd_tcon_od_get(struct aml_lcd_drv_s *pdrv);
int lcd_tcon_core_reg_get(struct aml_lcd_drv_s *pdrv,
			  unsigned char *buf, unsigned int size);
int lcd_tcon_enable(struct aml_lcd_drv_s *pdrv);
int lcd_tcon_reload(struct aml_lcd_drv_s *pdrv);
int lcd_tcon_reload_pre(struct aml_lcd_drv_s *pdrv);
void lcd_tcon_disable(struct aml_lcd_drv_s *pdrv);
void lcd_tcon_vsync_isr(struct aml_lcd_drv_s *pdrv);

/* lcd debug */
int lcd_debug_info_len(int num);
void lcd_test_pattern_init(struct aml_lcd_drv_s *pdrv, unsigned int num);
int lcd_debug_probe(struct aml_lcd_drv_s *pdrv);
int lcd_debug_remove(struct aml_lcd_drv_s *pdrv);

/* lcd venc */
unsigned int lcd_get_encl_lint_cnt(struct aml_lcd_drv_s *pdrv);
void lcd_wait_vsync(struct aml_lcd_drv_s *pdrv);

void lcd_gamma_debug_test_en(struct aml_lcd_drv_s *pdrv, int flag);
void lcd_debug_test(struct aml_lcd_drv_s *pdrv, unsigned int num);
void lcd_set_venc_timing(struct aml_lcd_drv_s *pdrv);
void lcd_set_venc(struct aml_lcd_drv_s *pdrv);
void lcd_venc_change(struct aml_lcd_drv_s *pdrv);
void lcd_venc_vrr_recovery(struct aml_lcd_drv_s *pdrv);
void lcd_venc_enable(struct aml_lcd_drv_s *pdrv, int flag);
void lcd_mute_set(struct aml_lcd_drv_s *pdrv, unsigned char flag);
int lcd_get_venc_init_config(struct aml_lcd_drv_s *pdrv);
int lcd_venc_probe(struct aml_lcd_drv_s *pdrv);
void lcd_screen_black(struct aml_lcd_drv_s *pdrv);
void lcd_screen_restore(struct aml_lcd_drv_s *pdrv);

/* lcd driver */
#ifdef CONFIG_AMLOGIC_LCD_TV
void lcd_tv_vout_server_init(struct aml_lcd_drv_s *pdrv);
void lcd_tv_vout_server_remove(struct aml_lcd_drv_s *pdrv);
void lcd_tv_clk_config_change(struct aml_lcd_drv_s *pdrv);
void lcd_tv_clk_update(struct aml_lcd_drv_s *pdrv);
int lcd_mode_tv_init(struct aml_lcd_drv_s *pdrv);
int lcd_mode_tv_remove(struct aml_lcd_drv_s *pdrv);
#endif
#ifdef CONFIG_AMLOGIC_LCD_TABLET
int lcd_mipi_dsi_init_table_detect(struct aml_lcd_drv_s *pdrv,
				   struct device_node *m_node, int on_off);
void mipi_dsi_config_init(struct aml_lcd_drv_s *pdrv);
int lcd_mipi_test_read(struct aml_lcd_drv_s *pdrv, struct dsi_read_s *dread);
int dsi_set_operation_mode(struct aml_lcd_drv_s *pdrv, unsigned char op_mode);
int dptx_edid_dump(struct aml_lcd_drv_s *pdrv);
int dptx_edid_timing_probe(struct aml_lcd_drv_s *pdrv);
int dptx_dpcd_read(struct aml_lcd_drv_s *pdrv, unsigned char *buf,
		   unsigned int reg, int len);
int dptx_dpcd_write(struct aml_lcd_drv_s *pdrv, unsigned int reg, unsigned char val);
void dptx_dpcd_dump(struct aml_lcd_drv_s *pdrv);
void lcd_tablet_vout_server_init(struct aml_lcd_drv_s *pdrv);
void lcd_tablet_vout_server_remove(struct aml_lcd_drv_s *pdrv);
void lcd_tablet_clk_config_change(struct aml_lcd_drv_s *pdrv);
void lcd_tablet_clk_update(struct aml_lcd_drv_s *pdrv);
int lcd_mode_tablet_init(struct aml_lcd_drv_s *pdrv);
int lcd_mode_tablet_remove(struct aml_lcd_drv_s *pdrv);
#endif

int lcd_drm_add(struct device *dev);
void lcd_drm_remove(struct device *dev);

#endif
