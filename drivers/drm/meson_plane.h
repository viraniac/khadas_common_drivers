/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __MESON_PLANE_H
#define __MESON_PLANE_H

#include <linux/kernel.h>
#include <drm/drmP.h>
#include <drm/drm_plane.h>
#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <uapi/amlogic/drm/meson_drm.h>
#include <linux/amlogic/media/vout/vout_notify.h>
#include <linux/amlogic/meson_uvm_core.h>
#include <drm/amlogic/meson_drm_plane.h>
#include "meson_fb.h"

/*legacy video driver issue caused zorder problem.*/
#define OSD_PLANE_BEGIN_ZORDER	65
#define OSD_PLANE_END_ZORDER		128

struct am_meson_plane_state {
	struct drm_plane_state base;
	u32 sec_en;
	bool fbdev_commit;
};

enum meson_max_fb_enum {
	FB_SIZE_1920x1080 = 0,
	FB_SIZE_3840x2160,
};

enum meson_plane_type {
	OSD_PLANE = 0,
	VIDEO_PLANE,
};

struct am_osd_plane {
	/*base struct, same as am_video_plane*/
	struct drm_plane base; //must be first element.
	struct meson_drm *drv; //point to struct parent.
	struct dentry *plane_debugfs_dir;
	int plane_index;
	int plane_type;

	struct drm_property *occupied_property;
	struct drm_property *prop_sec_en;
	struct drm_property *palette;
	struct drm_property *unsupport_nonafbc;
	bool osd_occupied;
	u32 palette_id;
	/*max fb property*/
	struct drm_property *max_fb_property;
	struct drm_property *rotation_reflect_property;

	/*osd extend*/
	u32 osd_reverse;
	u32 osd_blend_bypass;
	u32 osd_read_ports;
	void *vir_addr;
	u32 dump_size;
	bool bflg;
	u32 *receive_palette;
	int osd_permanent_blank;

	/* sysfs debug*/
	u16 pixel_blend_debug;
};

struct am_video_plane {
	/*base struct, same as am_video_plane*/
	struct drm_plane base; //must be first element.
	struct meson_drm *drv; //point to struct parent.
	struct dentry *plane_debugfs_dir;
	int plane_index;
	int plane_type;
	struct meson_vpu_pipeline *pipeline;
	spinlock_t lock; //used for video plane dma_fence
	u32 vfm_mode;
	/*video exted*/
};

struct meson_video_plane_fence_info {
	struct dma_fence *fence;
	struct vframe_s *vf;
	atomic_t refcount;
};

struct vf_node {
	struct meson_video_plane_fence_info *infos;
	struct list_head vfm_node;
};

struct meson_plane_supported_formats {
	u32 *formats;
	u32 format_num;
};

#define to_am_osd_plane(x) container_of(x, \
	struct am_osd_plane, base)
#define to_am_meson_plane_state(x) container_of(x, \
	struct am_meson_plane_state, base)
#define to_am_video_plane(x) container_of(x, \
	struct am_video_plane, base)

int am_meson_plane_create(struct meson_drm *priv);

/*For async commit on kernel 4.9, and can be reused on kernel 5.4.*/
int meson_video_plane_async_check(struct drm_plane *plane,
	struct drm_atomic_state *state);
void meson_video_plane_async_update(struct drm_plane *plane,
	struct drm_atomic_state *state);

int meson_osd_plane_async_check(struct drm_plane *plane,
	struct drm_atomic_state *state);
void meson_osd_plane_async_update(struct drm_plane *plane,
	struct drm_atomic_state *state);

struct drm_property *
meson_create_scaling_filter_prop(struct drm_device *dev,
			       unsigned int supported_filters);

void meson_video_set_vfmmode(struct device_node *of_node,
	struct meson_drm *priv);

#if IS_ENABLED(CONFIG_SYNC_FILE)
int am_meson_dmabuf_export_sync_file_ioctl(struct drm_device *dev,
	void *data, struct drm_file *file_priv);
#endif
int meson_plane_mute_ioctl(struct drm_device *dev,
	void *data, struct drm_file *file_priv);

#endif
