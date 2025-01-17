// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/vmalloc.h>
#include <linux/kfifo.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_tracer.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_event_mgr.h>
#include "hdmitx_log.h"

#define HDMI_TRACE_SIZE (BIT(12)) /* 4k */

struct hdmitx_tracer {
	int previous_event;
	int hdr10plus_event;
	struct kfifo log_fifo;
	struct hdmitx_event_mgr *eventmgr;
	/*to trigger userspace read fifo logs.*/
	struct work_struct uevent_work;
};

const char *hdmitx_event_to_str(enum hdmitx_event_log_bits event)
{
	switch (event) {
	case HDMITX_HPD_PLUGOUT:
		return "hdmitx_hpd_plugout\n";
	case HDMITX_HPD_PLUGIN:
		return "hdmitx_hpd_plugin\n";
	case HDMITX_EDID_HDMI_DEVICE:
		return "hdmitx_edid_hdmi_device\n";
	case HDMITX_EDID_DVI_DEVICE:
		return "hdmitx_edid_dvi_device\n";
	case HDMITX_EDID_HDR_SUPPORT:
		return "hdmitx_edid_hdr_device\n";
	case HDMITX_EDID_DV_SUPPORT:
		return "hdmitx_edid_dv_device\n";
	case HDMITX_HDR_MODE_SDR:
		return "hdmitx_hdr_mode_sdr\n";
	case HDMITX_HDR_MODE_SMPTE2084:
		return "hdmitx_hdr_mode_smpte2084\n";
	case HDMITX_HDR_MODE_HLG:
		return "hdmitx_hdr_mode_hlg\n";
	case HDMITX_HDR_MODE_HDR10PLUS:
		return "hdmitx_hdr_mode_hdr10plus\n";
	case HDMITX_HDR_MODE_CUVA:
		return "hdmitx_hdr_mode_cuva\n";
	case HDMITX_HDR_MODE_DV_STD:
		return "hdmitx_hdr_mode_dv_std\n";
	case HDMITX_HDR_MODE_DV_LL:
		return "hdmitx_hdr_mode_dv_ll\n";
	case HDMITX_KMS_DISABLE_OUTPUT:
		return "output_disable\n";
	case HDMITX_KMS_ENABLE_OUTPUT:
		return "output_enable\n";
	case HDMITX_EDID_HEAD_ERROR:
		return "hdmitx_edid_head_error\n";
	case HDMITX_EDID_CHECKSUM_ERROR:
		return "hdmitx_edid_checksum_error\n";
	case HDMITX_EDID_I2C_ERROR:
		return "hdmitx_edid_i2c_error\n";
	case HDMITX_HDCP_AUTH_SUCCESS:
		return "hdmitx_hdcp_auth_success\n";
	case HDMITX_HDCP_AUTH_FAILURE:
		return "hdmitx_hdcp_auth_failure\n";
	case HDMITX_HDCP_HDCP_1_ENABLED:
		return "hdmitx_hdcp_hdcp1_enabled\n";
	case HDMITX_HDCP_HDCP_2_ENABLED:
		return "hdmitx_hdcp_hdcp2_enabled\n";
	case HDMITX_HDCP_NOT_ENABLED:
		return "hdmitx_hdcp_not_enabled\n";
	case HDMITX_HDCP_DEVICE_NOT_READY_ERROR:
		return "hdmitx_hdcp_device_not_ready_error\n";
	case HDMITX_HDCP_AUTH_NO_14_KEYS_ERROR:
		return "hdmitx_hdcp_auth_no_14_keys_error\n";
	case HDMITX_HDCP_AUTH_NO_22_KEYS_ERROR:
		return "hdmitx_hdcp_auth_no_22_keys_error\n";
	case HDMITX_HDCP_AUTH_READ_BKSV_ERROR:
		return "hdmitx_hdcp_auth_read_bksv_error\n";
	case HDMITX_HDCP_AUTH_VI_MISMATCH_ERROR:
		return "hdmitx_hdcp_auth_vi_mismatch_error\n";
	case HDMITX_HDCP_AUTH_TOPOLOGY_ERROR:
		return "hdmitx_hdcp_auth_topology_error\n";
	case HDMITX_HDCP_AUTH_R0_MISMATCH_ERROR:
		return "hdmitx_hdcp_auth_r0_mismatch_error\n";
	case HDMITX_HDCP_AUTH_REPEATER_DELAY_ERROR:
		return "hdmitx_hdcp_auth_repeater_delay_error\n";
	case HDMITX_HDCP_I2C_ERROR:
		return "hdmitx_hdcp_i2c_error\n";
	case HDMITX_KMS_ERROR:
		return "KMS_ERROR\n";
	case HDMITX_KMS_SKIP:
		return "KMS_SKIP\n";
	case HDMITX_AUDIO_MODE_SETTING:
		return "hdmitx_audio_mode_setting\n";
	case HDMITX_AUDIO_MUTE:
		return "hdmitx_audio_mute\n";
	case HDMITX_AUDIO_UNMUTE:
		return "hdmitx_audio_unmute\n";
	default:
		return "Unknown event\n";
	}
}

static void hdmitx_logevents_handler(struct work_struct *work)
{
	static u32 cnt;
	struct hdmitx_tracer *tracer =
		container_of(work, struct hdmitx_tracer, uevent_work);

	hdmitx_event_mgr_send_uevent(tracer->eventmgr,
		HDMITX_CUR_ST_EVENT, ++cnt, false);
}

struct hdmitx_tracer *hdmitx_tracer_create(struct hdmitx_event_mgr *event_mgr)
{
	struct hdmitx_tracer *instance = vmalloc(sizeof(*instance));
	int ret = 0;

	if (!instance) {
		HDMITX_ERROR("%s FAIL\n", __func__);
	} else {
		instance->eventmgr = event_mgr;
		instance->previous_event = -1;
		instance->hdr10plus_event = -1;
		ret = kfifo_alloc(&instance->log_fifo, HDMI_TRACE_SIZE, GFP_KERNEL);
		if (ret)
			HDMITX_ERROR("alloc hdmi_log_kfifo fail [%d]\n", ret);
		INIT_WORK(&instance->uevent_work, hdmitx_logevents_handler);
	}

	return instance;
}

int hdmitx_tracer_destroy(struct hdmitx_tracer *tracer)
{
	if (tracer) {
		kfifo_free(&tracer->log_fifo);
		vfree(tracer);
	}

	return 0;
}

/* When hdr10plus mode ends, clear hdr10plus_event flag */
int hdmitx_tracer_clean_hdr10plus_event(struct hdmitx_tracer *tracer,
	enum hdmitx_event_log_bits event)
{
	if (event == HDMITX_HDR_MODE_HDR10PLUS)
		tracer->hdr10plus_event = -1;

	return 0;
}

int hdmitx_tracer_write_event(struct hdmitx_tracer *tracer,
	enum hdmitx_event_log_bits event)
{
	const char *log_str;
	int ret = 0;

	// Found, skip duplicate logging.
	// For example, UEvent spamming of HDCP support (b/220687552).
	if (event == tracer->previous_event)
		return 0;
	tracer->previous_event = event;

	/* Play HDR10+ videos, HDMITX_HDR_MODE_HDR10PLUS only writes once */
	if (tracer->hdr10plus_event == HDMITX_HDR_MODE_HDR10PLUS)
		return 0;

	if (event & HDMITX_HDMI_ERROR_MASK)
		HDMITX_INFO("Record HDMI error: %s\n", hdmitx_event_to_str(event));

	log_str = hdmitx_event_to_str(event);
	ret = kfifo_in(&tracer->log_fifo, log_str, strlen(log_str));
	if (!ret)
		HDMITX_ERROR("%s fifo error %d\n", __func__, ret);
	else
		tracer->hdr10plus_event = tracer->previous_event;

	/*after write trace, trigger uevent to save trace log.*/
	schedule_work(&tracer->uevent_work);

	return ret;
}

int hdmitx_tracer_read_event(struct hdmitx_tracer *tracer,
	char *buf, int read_len)
{
	return kfifo_out(&tracer->log_fifo, buf, read_len);
}

