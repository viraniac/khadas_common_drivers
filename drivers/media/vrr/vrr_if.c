// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2021 Amlogic, Inc. All rights reserved.
 */

#include <linux/init.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/notifier.h>
#include <linux/reboot.h>
#include <linux/clk.h>
#include <linux/of_device.h>
#include <linux/amlogic/iomap.h>
#include <linux/amlogic/media/vout/vinfo.h>
#include <linux/amlogic/media/vout/vout_notify.h>
#include <linux/amlogic/media/vrr/vrr.h>
#include "vrr_drv.h"
#include "vrr_reg.h"

static struct aml_vrr_drv_s *vrr_drv_active_sel(void)
{
	struct vinfo_s *vinfo = NULL;
	struct aml_vrr_drv_s *vdrv = NULL;
	int index = 0;

	vinfo = get_current_vinfo();
	if (!vinfo)
		return NULL;

	index = (vinfo->viu_mux >> 4) & 0xf;
	vdrv = vrr_drv_get(index);
	return vdrv;
}

struct aml_vrr_drv_s *aml_vrr_drv_active_sel(void)
{
	struct aml_vrr_drv_s *vdrv = NULL;

	vdrv = vrr_drv_active_sel();
	return vdrv;
}

static void vrr_drv_state_active_update(int index)
{
	struct aml_vrr_drv_s *vdrv = NULL;
	int i;

	for (i = 0; i < VRR_MAX_DRV; i++) {
		vdrv = vrr_drv_get(i);
		if (!vdrv)
			continue;
		if (i == index)
			vdrv->state |= VRR_STATE_ACTIVE;
		else
			vdrv->state &= ~VRR_STATE_ACTIVE;
	}
}

ssize_t vrr_active_status_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct aml_vrr_drv_s *vdrv = vrr_drv_active_sel();
	unsigned int offset;
	ssize_t len = 0;

	if (!vdrv) {
		len = sprintf(buf, "active vrr is null\n");
		return len;
	}

	offset = vdrv->data->offset[vdrv->index];

	len += sprintf(buf + len, "active vrr [%d] info\n", vdrv->index);
	if (vdrv->vrr_dev) {
		len += sprintf(buf + len, "dev->name:       %s\n",
				vdrv->vrr_dev->name);
		len += sprintf(buf + len, "dev->output_src: %d\n",
				vdrv->vrr_dev->output_src);
		len += sprintf(buf + len, "dev->enable:     %d\n",
				vdrv->vrr_dev->enable);
		len += sprintf(buf + len, "dev->vline:      %d\n",
				vdrv->vrr_dev->vline);
		len += sprintf(buf + len, "dev->vline_max:  %d\n",
				vdrv->vrr_dev->vline_max);
		len += sprintf(buf + len, "dev->vline_min:  %d\n",
				vdrv->vrr_dev->vline_min);
		len += sprintf(buf + len, "dev->vfreq_max:  %d\n",
				vdrv->vrr_dev->vfreq_max);
		len += sprintf(buf + len, "dev->vfreq_min:  %d\n\n",
				vdrv->vrr_dev->vfreq_min);
	}
	len += sprintf(buf + len, "vrr_policy:      %d\n", vdrv->policy);
	len += sprintf(buf + len, "lfc_en:          %d\n", vdrv->lfc_en);
	len += sprintf(buf + len, "adj_vline_max:   %d\n", vdrv->adj_vline_max);
	len += sprintf(buf + len, "adj_vline_min:   %d\n", vdrv->adj_vline_min);
	len += sprintf(buf + len, "line_dly:        %d\n", vdrv->line_dly);
	len += sprintf(buf + len, "state:           0x%08x\n", vdrv->state);
	len += sprintf(buf + len, "enable:          %d\n\n", vdrv->enable);

	/** vrr reg info **/
	len += sprintf(buf + len, "VENC_VRR_CTRL: 0x%x\n",
			vrr_reg_read(VENC_VRR_CTRL + offset));
	len += sprintf(buf + len, "VENC_VRR_ADJ_LMT: 0x%x\n",
			vrr_reg_read(VENC_VRR_ADJ_LMT + offset));
	len += sprintf(buf + len, "VENC_VRR_CTRL1: 0x%x\n",
			vrr_reg_read(VENC_VRR_CTRL1 + offset));
	len += sprintf(buf + len, "VENP_VRR_CTRL: 0x%x\n",
			vrr_reg_read(VENP_VRR_CTRL + offset));
	len += sprintf(buf + len, "VENP_VRR_ADJ_LMT: 0x%x\n",
			vrr_reg_read(VENP_VRR_ADJ_LMT + offset));
	len += sprintf(buf + len, "VENP_VRR_CTRL1: 0x%x\n\n",
			vrr_reg_read(VENP_VRR_CTRL1 + offset));

	/** vrr timer **/
	len += sprintf(buf + len, "sw_timer info\n");
	len += sprintf(buf + len, "sw_timer_cnt: %d\n", vdrv->sw_timer_cnt);
	len += sprintf(buf + len, "sw_timer_cnt state: %d\n",
		       vdrv->sw_timer_flag);

	return len;
}

int aml_vrr_state(void)
{
	struct aml_vrr_drv_s *vdrv_active = vrr_drv_active_sel();
	int ret = 0;

	if (!vdrv_active)
		return 0;

	if (vdrv_active->enable)
		ret = 1;
	else
		ret = 0;

	return ret;
}

int aml_vrr_func_en(int flag)
{
	struct aml_vrr_drv_s *vdrv_active = vrr_drv_active_sel();
	int ret;

	if (!vdrv_active)
		return -1;

	if (vrr_debug_print & VRR_DBG_PR_ADV)
		dump_stack();

	if (flag)
		ret = vrr_drv_func_en(vdrv_active, 1);
	else
		ret = vrr_drv_func_en(vdrv_active, 0);

	return ret;
}

static int aml_vrr_lfc_update(int flag, int fps)
{
	struct aml_vrr_drv_s *vdrv_active = vrr_drv_active_sel();
	int ret;

	if (!vdrv_active)
		return -1;

	if (flag)
		ret = vrr_drv_lfc_update(vdrv_active, 1, fps);
	else
		ret = vrr_drv_lfc_update(vdrv_active, 0, 0);

	return ret;
}

static int aml_vrr_drv_update(void)
{
	struct aml_vrr_drv_s *vdrv_active = vrr_drv_active_sel();
	struct vrr_notifier_data_s vdata;

	vdrv_active = vrr_drv_active_sel();
	if (!vdrv_active)
		return -1;

	if (vdrv_active->state & VRR_STATE_TRACE)
		vrr_drv_trace(vdrv_active, "vrr drv update for mode_change\n");

	vrr_drv_state_active_update(vdrv_active->index);
	if (vdrv_active->vrr_dev) {
		vdata.dev_vfreq_max = vdrv_active->vrr_dev->vfreq_max;
		vdata.dev_vfreq_min = vdrv_active->vrr_dev->vfreq_min + 5;
		if (vrr_debug_print & VRR_DBG_PR_NORMAL) {
			VRRPR("%s: dev_vfreq max:%d, min:%d\n",
				__func__, vdata.dev_vfreq_max, vdata.dev_vfreq_min);
		}
		aml_vrr_atomic_notifier_call_chain(VRR_EVENT_UPDATE, &vdata);
	}

	return 0;
}

static int aml_vrr_vout_notify_callback(struct notifier_block *block,
					unsigned long event, void *para)
{
	if (event != VOUT_EVENT_MODE_CHANGE)
		return 0;

	aml_vrr_drv_update();
	return 0;
}

static int aml_vrr_switch_notify_callback(struct notifier_block *block,
					  unsigned long event, void *para)
{
	struct aml_vrr_drv_s *vdrv_active = vrr_drv_active_sel();
	struct vrr_notifier_data_s *vdata;

	switch (event) {
	case FRAME_LOCK_EVENT_VRR_ON_MODE:
		if (!para)
			break;
		vdata = (struct vrr_notifier_data_s *)para;
		if (vdrv_active) {
			vdrv_active->line_dly = vdata->line_dly;
			vdrv_active->policy = vdata->vrr_policy;
			vrr_dly_new = vdata->line_dly;
		}
		aml_vrr_func_en(1);
		break;
	case FRAME_LOCK_EVENT_VRR_OFF_MODE:
		aml_vrr_func_en(0);
		break;
	default:
		break;
	}

	return 0;
}

static int aml_vrr_lfc_notify_callback(struct notifier_block *block,
				       unsigned long event, void *para)
{
	int fps;

	switch (event) {
	case VRR_EVENT_LFC_ON:
		if (!para) {
			VRRERR("%s: para is null\n", __func__);
			return 0;
		}
		fps = *(int *)para;
		aml_vrr_lfc_update(1, fps);
		break;
	case VRR_EVENT_LFC_OFF:
		aml_vrr_lfc_update(0, 0);
		break;
	default:
		break;
	}

	return 0;
}

static int aml_vrr_get_state_notify_callback(struct notifier_block *block,
					     unsigned long event, void *para)
{
	if (event != VRR_EVENT_GET_STATE)
		return 0;

	*(int *)para = aml_vrr_state();

	return 0;
}

static struct notifier_block aml_vrr_vout_notifier = {
	.notifier_call = aml_vrr_vout_notify_callback,
};

static struct notifier_block aml_vrr_switch_notifier = {
	.notifier_call = aml_vrr_switch_notify_callback,
};

static struct notifier_block aml_vrr_lfc_notifier = {
	.notifier_call = aml_vrr_lfc_notify_callback,
};

static struct notifier_block aml_vrr_get_state_notifier = {
	.notifier_call = aml_vrr_get_state_notify_callback,
};

int aml_vrr_if_probe(void)
{
	vout_register_client(&aml_vrr_vout_notifier);
	aml_vrr_atomic_notifier_register(&aml_vrr_switch_notifier);
	aml_vrr_atomic_notifier_register(&aml_vrr_lfc_notifier);
	aml_vrr_atomic_notifier_register(&aml_vrr_get_state_notifier);

	return 0;
}

int aml_vrr_if_remove(void)
{
	aml_vrr_atomic_notifier_unregister(&aml_vrr_switch_notifier);
	aml_vrr_atomic_notifier_unregister(&aml_vrr_lfc_notifier);
	aml_vrr_atomic_notifier_unregister(&aml_vrr_get_state_notifier);
	vout_unregister_client(&aml_vrr_vout_notifier);

	return 0;
}
