/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 * Copyright (c) 2019-2022 Amlogic Inc.
 */

#ifndef _MESON_DRM_H_
#define _MESON_DRM_H_

#include <drm/drm.h>
#include <drm/drm_fourcc.h>

/* Use flags */
#define MESON_USE_NONE			0
#define MESON_USE_SCANOUT			(1ull << 0)
#define MESON_USE_CURSOR			(1ull << 1)
#define MESON_USE_RENDERING		(1ull << 2)
#define MESON_USE_LINEAR			(1ull << 3)
#define MESON_USE_PROTECTED		(1ull << 11)
#define MESON_USE_HW_VIDEO_ENCODER		(1ull << 12)
#define MESON_USE_CAMERA_WRITE		(1ull << 13)
#define MESON_USE_CAMERA_READ		(1ull << 14)
#define MESON_USE_TEXTURE			(1ull << 17)
#define MESON_USE_VIDEO_PLANE           (1ull << 18)
#define MESON_USE_VIDEO_AFBC            (1ull << 19)

#define FBIOPUT_OSD_WINDOW_AXIS          0x4513
#define FBIOGET_DISPLAY_MODE             0x4580

#define MAX_VRR_MODE_GROUP 12
/* 40 bpp RGB */
#define DRM_FORMAT_ABGR10101010	fourcc_code('A', 'B', '4', '0')
		/* [39:0] A:B:G:R 10:10:10:10 little endian */

/**
 * User-desired buffer creation information structure.
 *
 * @size: user-desired memory allocation size.
 * @flags: user request for setting memory type or cache attributes.
 * @handle: returned a handle to created gem object.
 *     - this handle will be set by gem module of kernel side.
 */
struct drm_meson_gem_create {
	__u64 size;
	__u32 flags;
	__u32 handle;
};

struct drm_mode_test_attr {
	char modename[DRM_DISPLAY_MODE_LEN];
	char attr[DRM_DISPLAY_MODE_LEN];
	__u32 valid;
};

struct drm_meson_fbdev_rect {
	__u32 xstart;
	__u32 ystart;
	__u32 width;
	__u32 height;
	__u32 mask;
};

struct drm_vrr_mode_group {
	__u32 brr_vic;
	__u32 width;
	__u32 height;
	__u32 vrr_min;
	__u32 vrr_max;
	__u32 brr;
	char modename[DRM_DISPLAY_MODE_LEN];
};

struct drm_vrr_mode_groups {
	__u32 conn_id;
	__u32 num;
	struct drm_vrr_mode_group groups[MAX_VRR_MODE_GROUP];
};

/**
 * struct drm_meson_dma_buf_export_sync_file - Get a sync_file from a dma-buf
 *
 * Userspace can perform a DMA_BUF_IOCTL_EXPORT_SYNC_FILE to retrieve the
 * current set of fences on a dma-buf file descriptor as a sync_file.  CPU
 * waits via poll() or other driver-specific mechanisms typically wait on
 * whatever fences are on the dma-buf at the time the wait begins.  This
 * is similar except that it takes a snapshot of the current fences on the
 * dma-buf for waiting later instead of waiting immediately.  This is
 * useful for modern graphics APIs such as Vulkan which assume an explicit
 * synchronization model but still need to inter-operate with dma-buf.
 */
struct drm_meson_dma_buf_export_sync_file {
	/**
	 * @flags: Read/write flags
	 *
	 * Must be DMA_BUF_SYNC_READ, DMA_BUF_SYNC_WRITE, or both.
	 *
	 * If DMA_BUF_SYNC_READ is set and DMA_BUF_SYNC_WRITE is not set,
	 * the returned sync file waits on any writers of the dma-buf to
	 * complete.  Waiting on the returned sync file is equivalent to
	 * poll() with POLLIN.
	 *
	 * If DMA_BUF_SYNC_WRITE is set, the returned sync file waits on
	 * any users of the dma-buf (read or write) to complete.  Waiting
	 * on the returned sync file is equivalent to poll() with POLLOUT.
	 * If both DMA_BUF_SYNC_WRITE and DMA_BUF_SYNC_READ are set, this
	 * is equivalent to just DMA_BUF_SYNC_WRITE.
	 */
	__u32 flags;
	__u32 dmabuf_fd;
	/** @fd: Returned sync file descriptor */
	__s32 fd;
};

struct drm_meson_present_fence {
	__u32 crtc_idx;
	__u32 fd;
};

struct drm_meson_plane_mute {
	__u32 plane_type; /* 0:osd plane, 1:video plane */
	__u32 plane_mute; /* 0:umute plane, 1:mute plane */
};

/*Memory related.*/
#define DRM_IOCTL_MESON_GEM_CREATE	DRM_IOWR(DRM_COMMAND_BASE + \
		0x00, struct drm_meson_gem_create)
#define DRM_IOCTL_MESON_RMFB	DRM_IOWR(DRM_COMMAND_BASE + \
		0x01, unsigned int)
#define DRM_IOCTL_MESON_DMABUF_EXPORT_SYNC_FILE	DRM_IOWR(DRM_COMMAND_BASE + \
		0x02, struct drm_meson_dma_buf_export_sync_file)
#define DRM_IOCTL_MESON_ADDFB2	DRM_IOWR(DRM_COMMAND_BASE + \
		0x03, struct drm_mode_fb_cmd2)

/*KMS related.*/
#define DRM_IOCTL_MESON_ASYNC_ATOMIC    DRM_IOWR(DRM_COMMAND_BASE + \
		0x10, struct drm_mode_atomic)
#define DRM_IOCTL_MESON_TESTATTR DRM_IOWR(DRM_COMMAND_BASE + \
		0x11, struct drm_mode_test_attr) /*hdmitx related.*/
#define DRM_IOCTL_MESON_MUTE_PLANE DRM_IOWR(DRM_COMMAND_BASE + \
		0x12, struct drm_meson_plane_mute)
#define DRM_IOCTL_MESON_GET_VRR_RANGE DRM_IOWR(DRM_COMMAND_BASE + \
		0x13, struct drm_vrr_mode_groups)/*hdmitx relatde*/

/*present fence*/
#define DRM_IOCTL_MESON_CREAT_PRESENT_FENCE	DRM_IOWR(DRM_COMMAND_BASE + \
		0x20, struct drm_meson_present_fence)

#endif /* _MESON_DRM_H_ */
