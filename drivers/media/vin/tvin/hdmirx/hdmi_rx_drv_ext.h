/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __HDMI_RX_DRV_EXT_H__
#define __HDMI_RX_DRV_EXT_H__

/* for CEC notify */
typedef int (*cec_callback)(unsigned int pwr5v_sts);
typedef void (*cec_spd_callback)(int port_id, int handle_type);

int hdmirx_get_connect_info(void);
int cec_set_dev_info(u8 dev_idx);
int cec_set_vendor_id(u8 *vendor_id);
int hdmirx_set_cec_cfg(u32 cfg);
int register_cec_callback(cec_callback callback);
void unregister_cec_callback(void);
unsigned int hdmirx_get_base_fps(unsigned int hw_vic);
unsigned int hdmirx_get_hpd_info(void);
void rx_update_edid_callback(u32 tvin_port, u32 hdr_priority);
#endif
