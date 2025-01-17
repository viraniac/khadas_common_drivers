/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __AM_MESON_DRV_H
#define __AM_MESON_DRV_H

#include <linux/platform_device.h>
#include <linux/kthread.h>
#include <linux/of.h>
#include <drm/drmP.h>
#include <drm/drm_writeback.h>
/*CONFIG_AMLOGIC_DRM_EMULATE_FBDEV*/
#include <meson_fbdev.h>

#include <drm/amlogic/meson_drm_bind.h>

#include <linux/amlogic/media/vfm/vframe.h>
#include <linux/amlogic/media/vfm/vframe_provider.h>
#include <linux/amlogic/media/vfm/vframe_receiver.h>
#include <linux/amlogic/media/vfm/vfm_ext.h>
#include "meson_logo.h"
#include "meson_print.h"

#define MESON_MAX_CRTC		3
#define MESON_MAX_OSD		4
#define MESON_MAX_VIDEO		3

extern struct am_hdmi_tx am_hdmi_info;

struct meson_drm_thread {
	struct kthread_worker worker;
	struct drm_device *dev;
	struct task_struct *thread;
	unsigned int crtc_id;
};

struct meson_connector {
	struct drm_connector connector;
	struct meson_drm *drm_priv;
	void (*update)(struct drm_connector_state *new_state,
		struct drm_connector_state *old_state);
};

#define connector_to_meson_connector(x) container_of(x, struct meson_connector, connector)

enum vpu_enc_type {
	ENCODER_HDMI = 0,
	ENCODER_LCD,
	ENCODER_CVBS,
	ENCODER_MAX
};

enum atomic_mode_type {
	BLOCK_MODE = 0,
	NONBLOCK_MODE,
	ASYNC_MODE
};

struct meson_of_conf {
	/*for encoder: 0:hdmi 1:lcd 2:cvbs*/
	u32 crtc_masks[ENCODER_MAX];

	u32 vfm_mode;

	u32 osd_afbc_mask;

	u32 crtcmask_osd[MESON_MAX_OSD];
	u32 crtcmask_video[MESON_MAX_VIDEO];

	/*
	 * this will be used as drm usage, one item occupy one bit.
	 * bit0:
	 * gfcd afbc flag, 1:enable 0:disable;
	 */
	u64 drm_policy_mask;

	char *pref_mode;

	/* force osd slice_mode: 1*/
	u32 force_slice;
};

struct meson_drm {
	struct device *dev;

	struct drm_device *drm;
	struct device_node *topo_node;
	struct device_node *blocks_node;
	struct drm_plane *primary_plane;
	struct drm_plane *cursor_plane;
	struct drm_property_blob *gamma_lut_blob;

#ifdef CONFIG_AMLOGIC_DRM_USE_ION
	struct ion_client *gem_client;
#endif

	struct meson_vpu_pipeline *pipeline;
	struct meson_vpu_funcs *funcs;
	struct meson_vpu_data *vpu_data;
	struct am_meson_logo *logo;
	struct drm_atomic_state *state;

	u32 num_crtcs;
	u32 primary_plane_index[MESON_MAX_CRTC];
	struct am_meson_crtc *crtcs[MESON_MAX_CRTC];
	struct meson_drm_thread commit_thread[MESON_MAX_CRTC];

	u32 num_planes;
	struct am_osd_plane *osd_planes[MESON_MAX_OSD];
	struct am_video_plane *video_planes[MESON_MAX_VIDEO];

	/* During a vsync of pan display, an async commit already ran */
	bool pan_async_commit_ran;
	u32 disable_video_plane;

	struct meson_of_conf of_conf;

	/*CONFIG_AMLOGIC_DRM_EMULATE_FBDEV*/
	struct meson_drm_fbdev_config ui_config;
	struct meson_drm_fbdev *osd_fbdevs[MESON_MAX_OSD];
	u32 fbdev_zorder[MESON_MAX_OSD];

	struct meson_drm_bound_data bound_data;

	bool irq_enabled;
	wait_queue_head_t wq_shut_ctrl;
	bool shutdown_on;
	bool compat_mode;
	bool logo_show_done;
	bool recovery_mode;
	u32 osd_occupied_index;
	u8 dummyl_from_hdmitx;
	u8 remove_get_vblank_timestamp;

	u32 pxp_mode;
};

/*component bind functions*/
int meson_connector_dev_bind(struct drm_device *drm,
	int type, struct meson_connector_dev *intf);
int meson_connector_dev_unbind(struct drm_device *drm,
	int type, int connector_id);

/*meson mode config atomic func*/
int meson_atomic_commit(struct drm_device *dev,
			     struct drm_atomic_state *state,
			     bool nonblock);
void meson_atomic_helper_commit_tail(struct drm_atomic_state *old_state);

/*meson of parse*/
void meson_of_init(struct device *vpu_dev, struct drm_device *dev,
	struct meson_drm *priv);
/*******************************/

#ifdef CONFIG_DEBUG_FS
void meson_debugfs_init(struct drm_minor *minor);
#endif

#endif /* __AM_MESON_DRV_H */
