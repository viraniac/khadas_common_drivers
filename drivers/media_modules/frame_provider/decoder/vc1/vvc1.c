/*
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Description:
 */
#define DEBUG
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/kfifo.h>
#include <linux/platform_device.h>
#include <linux/amlogic/media/utils/amstream.h>
#include <linux/amlogic/media/frame_sync/ptsserv.h>
#include <linux/amlogic/media/canvas/canvas.h>
#include <linux/amlogic/media/canvas/canvas_mgr.h>
#include <linux/amlogic/media/vfm/vframe.h>
#include <linux/amlogic/media/vfm/vframe_provider.h>
#include <linux/amlogic/media/vfm/vframe_receiver.h>
#include <linux/amlogic/media/registers/register.h>
#include <linux/amlogic/media/codec_mm/codec_mm.h>
#include <linux/amlogic/media/codec_mm/configs.h>
#include <linux/delay.h>
#include <uapi/linux/tee.h>

#include "../utils/amvdec.h"
#include "../utils/vdec.h"
#include "../utils/decoder_mmu_box.h"
#include "../utils/decoder_bmmu_box.h"
#include "../utils/vdec_feature.h"
#include "../utils/firmware.h"
#include "../../../common/chips/decoder_cpu_ver_info.h"
#include "../../../stream_input/amports/amports_priv.h"

#define DRIVER_NAME "amvdec_vc1"
#define MODULE_NAME "amvdec_vc1"

#define DEBUG_PTS
#if 1	/* //MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6 */
#define NV21
#endif

#define VC1_MAX_SUPPORT_SIZE (1920*1088)

#define I_PICTURE   0
#define P_PICTURE   1
#define B_PICTURE   2

#define ORI_BUFFER_START_ADDR   0x01000000

#define INTERLACE_FLAG          0x80
#define BOTTOM_FIELD_FIRST_FLAG 0x40

/* protocol registers */
#define VC1_PIC_RATIO       AV_SCRATCH_0
#define VC1_ERROR_COUNT    AV_SCRATCH_6
#define VC1_SOS_COUNT     AV_SCRATCH_7
#define VC1_BUFFERIN       AV_SCRATCH_8
#define VC1_BUFFEROUT      AV_SCRATCH_9
#define VC1_REPEAT_COUNT    AV_SCRATCH_A
#define VC1_TIME_STAMP      AV_SCRATCH_B
#define VC1_OFFSET_REG      AV_SCRATCH_C
#define MEM_OFFSET_REG      AV_SCRATCH_F

#define CANVAS_BUF_REG      AV_SCRATCH_D
#define ANC0_CANVAS_REG     AV_SCRATCH_E
#define ANC1_CANVAS_REG     AV_SCRATCH_5
#define DECODE_STATUS       AV_SCRATCH_H
#define VC1_PIC_INFO        AV_SCRATCH_J
#define DEBUG_REG1          AV_SCRATCH_M
#define DEBUG_REG2          AV_SCRATCH_N

#define DECODE_STATUS_SEQ_HEADER_DONE 0x1
#define DECODE_STATUS_PIC_HEADER_DONE 0x2
#define DECODE_STATUS_PIC_SKIPPED     0x3
#define DECODE_STATUS_BUF_INVALID     0x4
#define DECODE_STATUS_PARAM_CHECK     0x5

/*bit0:1 support vc1 new version, 0: old version
  bit1:1 support DECODE_STATUS_PARAM_CHECK*/
#define NEW_DRV_VER         3

#define VF_POOL_SIZE		16
#define DECODE_BUFFER_NUM_MAX	4
#define WORKSPACE_SIZE		(2 * SZ_1M)
#define MAX_BMMU_BUFFER_NUM	(DECODE_BUFFER_NUM_MAX + 1)
#define VF_BUFFER_IDX(n)	(1 + n)
#define DCAC_BUFF_START_ADDR	0x01f00000

 #define PUT_INTERVAL        (HZ/100)

#if 1	/* /MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6 */
/* TODO: move to register headers */
#define VPP_VD1_POSTBLEND       (1 << 10)
#define MEM_FIFO_CNT_BIT        16
#define MEM_LEVEL_CNT_BIT       18
#endif
static struct vdec_info *gvs;
static struct vdec_s *vdec = NULL;

static struct vframe_s *vvc1_vf_peek(void *);
static struct vframe_s *vvc1_vf_get(void *);
static void vvc1_vf_put(struct vframe_s *, void *);
static int vvc1_vf_states(struct vframe_states *states, void *);
static int vvc1_event_cb(int type, void *data, void *private_data);

static int vvc1_prot_init(void);
static void vvc1_local_init(bool is_reset);

static const char vvc1_dec_id[] = "vvc1-dev";

#define PROVIDER_NAME   "decoder.vc1"
static const struct vframe_operations_s vvc1_vf_provider = {
	.peek = vvc1_vf_peek,
	.get = vvc1_vf_get,
	.put = vvc1_vf_put,
	.event_cb = vvc1_event_cb,
	.vf_states = vvc1_vf_states,
};
static void *mm_blk_handle;
static struct vframe_provider_s vvc1_vf_prov;

static DECLARE_KFIFO(newframe_q, struct vframe_s *, VF_POOL_SIZE);
static DECLARE_KFIFO(display_q, struct vframe_s *, VF_POOL_SIZE);
static DECLARE_KFIFO(recycle_q, struct vframe_s *, VF_POOL_SIZE);

static struct vframe_s vfpool[VF_POOL_SIZE];
static struct vframe_s vfpool2[VF_POOL_SIZE];
static int cur_pool_idx;
static struct timer_list recycle_timer;
static u32 stat;
static u32 buf_size = 32 * 1024 * 1024;
static u32 buf_offset;
static u32 avi_flag;
static u32 unstable_pts_debug;
static u32 unstable_pts;
static u32 vvc1_ratio;
static u32 vvc1_format;

static u32 intra_output;
static u32 frame_width, frame_height, frame_dur;
static u32 saved_resolution;
static u32 pts_by_offset = 1;
static u32 total_frame;
static u32 next_pts;
static u64 next_pts_us64;
static bool is_reset;
static struct work_struct set_clk_work;
static struct work_struct error_wd_work;
static struct canvas_config_s vc1_canvas_config[DECODE_BUFFER_NUM_MAX][3];
spinlock_t vc1_rp_lock;

#ifdef DEBUG_PTS
static u32 pts_hit, pts_missed, pts_i_hit, pts_i_missed;
#endif
static DEFINE_SPINLOCK(lock);

static struct dec_sysinfo vvc1_amstream_dec_info;

struct frm_s {
	int state;
	u32 start_pts;
	int num;
	u32 end_pts;
	u32 rate;
	u32 trymax;
};

static struct frm_s frm;

enum {
	RATE_MEASURE_START_PTS = 0,
	RATE_MEASURE_END_PTS,
	RATE_MEASURE_DONE
};
#define RATE_MEASURE_NUM 8
#define RATE_CORRECTION_THRESHOLD 5
#define RATE_24_FPS  3755	/* 23.97 */
#define RATE_30_FPS  3003	/* 29.97 */
#define DUR2PTS(x) ((x)*90/96)
#define PTS2DUR(x) ((x)*96/90)

#define I_PICTURE 0
#define P_PICTURE 1
#define B_PICTURE 2
#define BI_PICTURE 3

#define VC1_DEBUG_DETAIL                   0x01

#define INVALID_IDX -1  /* Invalid buffer index.*/
#define MAX_SIZE_2K (1920 * 1088)

static u32 udebug_flag;
static int debug;
unsigned int debug_mask = 0xff;
static u32 wait_time = 5;

bool process_busy;

struct pic_info_t {
	u32 buffer_info;
	u32 index;
	u32 offset;
	u32 width;
	u32 height;
	u32 pts;
	u64 pts64;
	bool pts_valid;
	ulong v4l_ref_buf_addr;
	u32 hw_decode_time;
	u32 frame_size; // For frame base mode
	u64 timestamp;
	u32 picture_type;
	unsigned short decode_pic_count;
	u32 repeat_cnt;
};

struct vdec_vc1_hw_s {
	s32 vfbuf_use[DECODE_BUFFER_NUM_MAX];
	unsigned char again_flag;
	unsigned char recover_flag;
	u32 frame_width;
	u32 frame_height;
	u32 frame_dur;
	u32 frame_prog;
	u32 saved_resolution;
	u32 avi_flag;
	u32 vavs_ratio;
	u32 pic_type;

	u32 vf_buf_num_used;
	u32 total_frame;
	u32 next_pts;
	unsigned char throw_pb_flag;

	/*debug*/
	u32 ucode_pause_pos;
	u32 decode_pic_count;
	u8 reset_decode_flag;
	u32 display_frame_count;
	u32 buf_status;
	u32 pre_parser_wr_ptr;

	s32 refs[2];

	s32 ref_use[DECODE_BUFFER_NUM_MAX];
	s32 buf_use[DECODE_BUFFER_NUM_MAX];
	u32 decoding_index;
	struct pic_info_t pics[DECODE_BUFFER_NUM_MAX];
	u32 interlace_flag;
	u32 new_type;
	u32 canvas_mode;
};

struct vdec_vc1_hw_s vc1_hw;

#undef pr_info
#define pr_info pr_cont
int vc1_print(int index, int debug_flag, const char *fmt, ...)
{
	if ((debug_flag == 0) ||
		((debug & debug_flag) &&
		((1 << index) & debug_mask))) {
		unsigned char *buf = kzalloc(512, GFP_ATOMIC);
		int len = 0;
		va_list args;

		if (!buf)
			return 0;

		va_start(args, fmt);
		len = sprintf(buf, "%d: ", index);
		vsnprintf(buf + len, 512-len, fmt, args);
		pr_info("%s", buf);
		va_end(args);
		kfree(buf);
	}
	return 0;
}

static inline int pool_index(struct vframe_s *vf)
{
	if ((vf >= &vfpool[0]) && (vf <= &vfpool[VF_POOL_SIZE - 1]))
		return 0;
	else if ((vf >= &vfpool2[0]) && (vf <= &vfpool2[VF_POOL_SIZE - 1]))
		return 1;
	else
		return -1;
}

static inline bool close_to(int a, int b, int m)
{
	return abs(a - b) < m;
}

static inline u32 index2canvas(u32 index)
{
	const u32 canvas_tab[DECODE_BUFFER_NUM_MAX] = {
#if 1	/* ALWAYS.MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6 */
	0x010100, 0x030302, 0x050504, 0x070706/*,
	0x090908, 0x0b0b0a, 0x0d0d0c, 0x0f0f0e*/
#else
		0x020100, 0x050403, 0x080706, 0x0b0a09
#endif
	};

	return canvas_tab[index];
}

static void set_aspect_ratio(struct vframe_s *vf, unsigned int pixel_ratio)
{
	int ar = 0;

	if (vvc1_ratio == 0) {
		/* always stretch to 16:9 */
		vf->ratio_control |= (0x90 << DISP_RATIO_ASPECT_RATIO_BIT);
	} else if (pixel_ratio > 0x0f) {
		ar = (vvc1_amstream_dec_info.height * (pixel_ratio & 0xff) *
			  vvc1_ratio) / (vvc1_amstream_dec_info.width *
							 (pixel_ratio >> 8));
	} else {
		switch (pixel_ratio) {
		case 0:
			ar = (vvc1_amstream_dec_info.height * vvc1_ratio) /
				 vvc1_amstream_dec_info.width;
			break;
		case 1:
			vf->sar_width = 1;
			vf->sar_height = 1;
			ar = (vf->height * vvc1_ratio) / vf->width;
			break;
		case 2:
			vf->sar_width = 12;
			vf->sar_height = 11;
			ar = (vf->height * 11 * vvc1_ratio) / (vf->width * 12);
			break;
		case 3:
			vf->sar_width = 10;
			vf->sar_height = 11;
			ar = (vf->height * 11 * vvc1_ratio) / (vf->width * 10);
			break;
		case 4:
			vf->sar_width = 16;
			vf->sar_height = 11;
			ar = (vf->height * 11 * vvc1_ratio) / (vf->width * 16);
			break;
		case 5:
			vf->sar_width = 40;
			vf->sar_height = 33;
			ar = (vf->height * 33 * vvc1_ratio) / (vf->width * 40);
			break;
		case 6:
			vf->sar_width = 24;
			vf->sar_height = 11;
			ar = (vf->height * 11 * vvc1_ratio) / (vf->width * 24);
			break;
		case 7:
			vf->sar_width = 20;
			vf->sar_height = 11;
			ar = (vf->height * 11 * vvc1_ratio) / (vf->width * 20);
			break;
		case 8:
			vf->sar_width = 32;
			vf->sar_height = 11;
			ar = (vf->height * 11 * vvc1_ratio) / (vf->width * 32);
			break;
		case 9:
			vf->sar_width = 80;
			vf->sar_height = 33;
			ar = (vf->height * 33 * vvc1_ratio) / (vf->width * 80);
			break;
		case 10:
			vf->sar_width = 18;
			vf->sar_height = 11;
			ar = (vf->height * 11 * vvc1_ratio) / (vf->width * 18);
			break;
		case 11:
			vf->sar_width = 15;
			vf->sar_height = 11;
			ar = (vf->height * 11 * vvc1_ratio) / (vf->width * 15);
			break;
		case 12:
			vf->sar_width = 64;
			vf->sar_height = 33;
			ar = (vf->height * 33 * vvc1_ratio) / (vf->width * 64);
			break;
		case 13:
			vf->sar_width = 160;
			vf->sar_height = 99;
			ar = (vf->height * 99 * vvc1_ratio) /
				(vf->width * 160);
			break;
		default:
			vf->sar_width = 1;
			vf->sar_height = 1;
			ar = (vf->height * vvc1_ratio) / vf->width;
			break;
		}
	}

	ar = min(ar, DISP_RATIO_ASPECT_RATIO_MAX);

	vf->ratio_control = (ar << DISP_RATIO_ASPECT_RATIO_BIT);
	/*vf->ratio_control |= DISP_RATIO_FORCECONFIG | DISP_RATIO_KEEPRATIO;*/
}

static void vc1_set_rp(void) {
	unsigned long flags;

	spin_lock_irqsave(&vc1_rp_lock, flags);
	STBUF_WRITE(&vdec->vbuf, set_rp,
		READ_VREG(VLD_MEM_VIFIFO_RP));
	spin_unlock_irqrestore(&vc1_rp_lock, flags);
}

static int vvc1_config_ref_buf(struct vdec_vc1_hw_s *hw)
{
	vc1_print(0, VC1_DEBUG_DETAIL,"%s: new_type %d\n", __func__, hw->new_type);

	if (hw->new_type != B_PICTURE) {
		if (hw->refs[1] == -1) {
			WRITE_VREG(ANC0_CANVAS_REG, 0xffffffff);
		} else {
			WRITE_VREG(ANC0_CANVAS_REG, index2canvas(hw->refs[1]));
		}

		if (hw->refs[0] == -1) {
			WRITE_VREG(ANC1_CANVAS_REG, 0xffffffff);
		} else {
			WRITE_VREG(ANC1_CANVAS_REG, index2canvas(hw->refs[0]));
		}
	} else {
		if (hw->refs[0] == -1) {
			WRITE_VREG(ANC0_CANVAS_REG, 0xffffffff);
		} else {
			WRITE_VREG(ANC0_CANVAS_REG, index2canvas(hw->refs[0]));
		}

		if (hw->refs[1] == -1) {
			WRITE_VREG(ANC1_CANVAS_REG, 0xffffffff);
		} else {
			WRITE_VREG(ANC1_CANVAS_REG, index2canvas(hw->refs[1]));
		}
	}

	return 0;
}

static int update_reference(struct vdec_vc1_hw_s *hw,	int index)
{
	hw->ref_use[index]++;
	if (hw->refs[1] == -1) {
		hw->refs[1] = index;
		/*
		* first pic need output to show
		* usecnt do not decrease.
		*/
	} else if (hw->refs[0] == -1) {
		hw->refs[0] = hw->refs[1];
		hw->refs[1] = index;
		/* second pic do not output */
		index = hw->vf_buf_num_used;
	} else {
		hw->ref_use[hw->refs[0]]--; 	//old ref0 unused
		hw->refs[0] = hw->refs[1];
		hw->refs[1] = index;
		index = hw->refs[0];
	}
	vc1_print(0, VC1_DEBUG_DETAIL,"%s: hw->refs[0] = %d, hw->refs[1] = %d\n", __func__, hw->refs[0], hw->refs[1]);
	return index;
}

static int find_free_buffer(struct vdec_vc1_hw_s *hw)
{
	int i;

	for (i = 0; i < hw->vf_buf_num_used; i++) {
		vc1_print(0, VC1_DEBUG_DETAIL,"%s: i %d, vfbuf_use %d, ref_use %d, buf_use %d\n", __func__,
			i, hw->vfbuf_use[i], hw->ref_use[i], hw->buf_use[i]);
		if ((hw->vfbuf_use[i] == 0) &&
			(hw->ref_use[i] == 0) &&
			(hw->buf_use[i] == 0)) {
			break;
		}
	}

	if ((i == hw->vf_buf_num_used) &&
		(hw->vf_buf_num_used != 0)) {
		vc1_print(0, VC1_DEBUG_DETAIL, "%s: buf INVALID_IDX\n", __func__);
		return INVALID_IDX;
	}

	return i;
}

static int vvc1_config_buf(struct vdec_vc1_hw_s *hw)
{
	u32 index = -1;
	u32 canvas1_info = 0;
	index = find_free_buffer(hw);
	if (index == INVALID_IDX) {
		WRITE_VREG(CANVAS_BUF_REG, 0xff);
		return -1;
	}
	hw->decoding_index = index;
	hw->buf_use[hw->decoding_index]++;
	canvas1_info = (index2canvas(index) << 8) | index;
	vc1_print(0, VC1_DEBUG_DETAIL,"%s: decoding_index %d, buf_use %d, canvas1_info 0x%x\n",
		__func__, index, hw->buf_use[hw->decoding_index], canvas1_info);
	WRITE_VREG(CANVAS_BUF_REG, canvas1_info);

	vvc1_config_ref_buf(hw);
	return 0;
}

static int prepare_display_buf(struct vdec_vc1_hw_s *hw,	struct pic_info_t *pic)
{

	struct vframe_s *vf = NULL;
	u32 reg = pic->buffer_info;
	u32 picture_type = pic->picture_type;
	u32 repeat_count = pic->repeat_cnt;
	unsigned int pts = pic->pts;
	unsigned int pts_valid = pic->pts_valid;
	//unsigned int offset = pic->offset;
	u64 pts_us64 = pic->pts64;
	u32 buffer_index = pic->index;

	vc1_print(0, VC1_DEBUG_DETAIL, "%s: buffer_info 0x%x, index %d, picture_type %d\n",
					__func__, reg, buffer_index, picture_type);

	if (hw->interlace_flag) { /* interlace */
		vc1_print(0, VC1_DEBUG_DETAIL, "%s: interlace reg 0x%x\n", __func__, reg);
		hw->throw_pb_flag = 0;
		if (kfifo_get(&newframe_q, &vf) == 0) {
			pr_info
			("fatal error, no available buffer slot.");
			return IRQ_HANDLED;
		}
		vf->signal_type = 0;
		vf->index = buffer_index;
		vf->width = vvc1_amstream_dec_info.width;
		vf->height = vvc1_amstream_dec_info.height;
		vf->bufWidth = 1920;
		vf->flag = 0;

		if (pts_valid) {
			vf->pts = pts;
			vf->pts_us64 = pts_us64;
			if ((repeat_count > 1) && avi_flag) {
				vf->duration =
					vvc1_amstream_dec_info.rate *
					(repeat_count >> 1);
				next_pts = pts +
					(vvc1_amstream_dec_info.rate *
					 repeat_count >> 1) * 15 / 16;
				next_pts_us64 = pts_us64 +
					((vvc1_amstream_dec_info.rate *
					repeat_count >> 1) * 15 / 16) *
					100 / 9;
			} else {
				vf->duration =
				vvc1_amstream_dec_info.rate >> 1;
				next_pts = 0;
				next_pts_us64 = 0;
				if (picture_type != I_PICTURE &&
					unstable_pts) {
					vf->pts = 0;
					vf->pts_us64 = 0;
				}
			}
		} else {
			vf->pts = next_pts;
			vf->pts_us64 = next_pts_us64;
			if ((repeat_count > 1) && avi_flag) {
				vf->duration =
					vvc1_amstream_dec_info.rate *
					(repeat_count >> 1);
				if (next_pts != 0) {
					next_pts += ((vf->duration) -
					((vf->duration) >> 4));
				}
				if (next_pts_us64 != 0) {
					next_pts_us64 +=
					div_u64((u64)((vf->duration) -
					((vf->duration) >> 4)) *
					100, 9);
				}
			} else {
				vf->duration =
				vvc1_amstream_dec_info.rate >> 1;
				next_pts = 0;
				next_pts_us64 = 0;
				if (picture_type != I_PICTURE &&
					unstable_pts) {
					vf->pts = 0;
					vf->pts_us64 = 0;
				}
			}
		}

		vf->duration_pulldown = 0;
		vf->type = (reg & BOTTOM_FIELD_FIRST_FLAG) ?
		VIDTYPE_INTERLACE_BOTTOM : VIDTYPE_INTERLACE_TOP;
#ifdef NV21
		vf->type |= VIDTYPE_VIU_NV21;
#endif
		vf->canvas0Addr = vf->canvas1Addr =
					index2canvas(buffer_index);
		vf->orientation = 0;
		vf->type_original = vf->type;
		set_aspect_ratio(vf, READ_VREG(VC1_PIC_RATIO));

		hw->vfbuf_use[buffer_index]++;
		vf->mem_handle =
			decoder_bmmu_box_get_mem_handle(
				mm_blk_handle,
				buffer_index);
		//if (is_support_vdec_canvas()) {
		vf->canvas0Addr = vf->canvas1Addr = -1;
		vf->canvas0_config[0] = vc1_canvas_config[buffer_index][0];
		vf->canvas0_config[1] = vc1_canvas_config[buffer_index][1];
#ifdef NV21
		vf->plane_num = 2;
#else
		vf->canvas0_config[2] = vc1_canvas_config[buffer_index][2];
		vf->plane_num = 3;
#endif
		//}
		if (get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_T5D && vdec->use_vfm_path &&
			vdec_stream_based(vdec)) {
			vf->type |= VIDTYPE_FORCE_SIGN_IP_JOINT;
		}

		decoder_do_frame_check(vdec, vf);
		kfifo_put(&display_q, (const struct vframe_s *)vf);
		ATRACE_COUNTER(MODULE_NAME, vf->pts);

		vf_notify_receiver(
			PROVIDER_NAME,
			VFRAME_EVENT_PROVIDER_VFRAME_READY,
			NULL);

		if (kfifo_get(&newframe_q, &vf) == 0) {
			pr_info
			("fatal error, no available buffer slot.");
			return IRQ_HANDLED;
		}
		vf->signal_type = 0;
		vf->index = buffer_index;
		vf->width = vvc1_amstream_dec_info.width;
		vf->height = vvc1_amstream_dec_info.height;
		vf->bufWidth = 1920;
		vf->flag = 0;

		vf->pts = next_pts;
		vf->pts_us64 = next_pts_us64;
		if ((repeat_count > 1) && avi_flag) {
			vf->duration =
				vvc1_amstream_dec_info.rate *
				(repeat_count >> 1);
			if (next_pts != 0) {
				next_pts +=
					((vf->duration) -
					 ((vf->duration) >> 4));
			}
			if (next_pts_us64 != 0) {
				next_pts_us64 += div_u64((u64)((vf->duration) -
				((vf->duration) >> 4)) * 100, 9);
			}
		} else {
			vf->duration =
				vvc1_amstream_dec_info.rate >> 1;
			next_pts = 0;
			next_pts_us64 = 0;
			if (picture_type != I_PICTURE &&
				unstable_pts) {
				vf->pts = 0;
				vf->pts_us64 = 0;
			}
		}

		vf->duration_pulldown = 0;
		vf->type = (reg & BOTTOM_FIELD_FIRST_FLAG) ?
		VIDTYPE_INTERLACE_TOP : VIDTYPE_INTERLACE_BOTTOM;
#ifdef NV21
		vf->type |= VIDTYPE_VIU_NV21;
#endif
		vf->canvas0Addr = vf->canvas1Addr =
				index2canvas(buffer_index);
		vf->orientation = 0;
		vf->type_original = vf->type;
		set_aspect_ratio(vf, READ_VREG(VC1_PIC_RATIO));

		hw->vfbuf_use[buffer_index]++;
		vf->mem_handle =
			decoder_bmmu_box_get_mem_handle(
				mm_blk_handle,
				buffer_index);

		//if (is_support_vdec_canvas()) {
		vf->canvas0Addr = vf->canvas1Addr = -1;
		vf->canvas0_config[0] = vc1_canvas_config[buffer_index][0];
		vf->canvas0_config[1] = vc1_canvas_config[buffer_index][1];
#ifdef NV21
		vf->plane_num = 2;
#else
		vf->canvas0_config[2] = vc1_canvas_config[buffer_index][2];
		vf->plane_num = 3;
#endif
		//}
		if (get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_T5D && vdec->use_vfm_path &&
			vdec_stream_based(vdec)) {
			vf->type |= VIDTYPE_FORCE_SIGN_IP_JOINT;
		}
		kfifo_put(&display_q, (const struct vframe_s *)vf);
		ATRACE_COUNTER(MODULE_NAME, vf->pts);

		vf_notify_receiver(
				PROVIDER_NAME,
				VFRAME_EVENT_PROVIDER_VFRAME_READY,
				NULL);
	} else {	/* progressive */
		hw->throw_pb_flag = 0;
		if (kfifo_get(&newframe_q, &vf) == 0) {
			pr_info
			("fatal error, no available buffer slot.");
			return IRQ_HANDLED;
		}
		vf->signal_type = 0;
		vf->index = buffer_index;
		vf->width = vvc1_amstream_dec_info.width;
		vf->height = vvc1_amstream_dec_info.height;
		vf->bufWidth = 1920;
		vf->flag = 0;

		if (pts_valid) {
			vf->pts = pts;
			vf->pts_us64 = pts_us64;
			if ((repeat_count > 1) && avi_flag) {
				vf->duration =
					vvc1_amstream_dec_info.rate *
					repeat_count;
				next_pts =
					pts +
					(vvc1_amstream_dec_info.rate *
					 repeat_count) * 15 / 16;
				next_pts_us64 = pts_us64 +
					((vvc1_amstream_dec_info.rate *
					repeat_count) * 15 / 16) *
					100 / 9;
			} else {
				vf->duration =
					vvc1_amstream_dec_info.rate;
				next_pts = 0;
				next_pts_us64 = 0;
				if (picture_type != I_PICTURE &&
					unstable_pts) {
					vf->pts = 0;
					vf->pts_us64 = 0;
				}
			}
		} else {
			vf->pts = next_pts;
			vf->pts_us64 = next_pts_us64;
			if ((repeat_count > 1) && avi_flag) {
				vf->duration =
					vvc1_amstream_dec_info.rate *
					repeat_count;
				if (next_pts != 0) {
					next_pts += ((vf->duration) -
						((vf->duration) >> 4));
				}
				if (next_pts_us64 != 0) {
					next_pts_us64 +=
					div_u64((u64)((vf->duration) -
					((vf->duration) >> 4)) *
					100, 9);
				}
			} else {
				vf->duration =
					vvc1_amstream_dec_info.rate;
				next_pts = 0;
				next_pts_us64 = 0;
				if (picture_type != I_PICTURE &&
					unstable_pts) {
					vf->pts = 0;
					vf->pts_us64 = 0;
				}
			}
		}

		vf->duration_pulldown = 0;
#ifdef NV21
		vf->type =
			VIDTYPE_PROGRESSIVE | VIDTYPE_VIU_FIELD |
			VIDTYPE_VIU_NV21;
#else
		vf->type = VIDTYPE_PROGRESSIVE | VIDTYPE_VIU_FIELD;
#endif
		vf->canvas0Addr = vf->canvas1Addr =
					index2canvas(buffer_index);
		vf->orientation = 0;
		vf->type_original = vf->type;
		set_aspect_ratio(vf, READ_VREG(VC1_PIC_RATIO));

		hw->vfbuf_use[buffer_index]++;
		vc1_print(0, VC1_DEBUG_DETAIL, "%s:  progressive vfbuf_use[%d] %d\n",
			__func__, buffer_index, hw->vfbuf_use[buffer_index]);
		vf->mem_handle =
			decoder_bmmu_box_get_mem_handle(
				mm_blk_handle,
				buffer_index);
		//if (is_support_vdec_canvas()) {
		vf->canvas0Addr = vf->canvas1Addr = -1;
		vf->canvas0_config[0] = vc1_canvas_config[buffer_index][0];
		vf->canvas0_config[1] = vc1_canvas_config[buffer_index][1];
#ifdef NV21
		vf->plane_num = 2;
#else
		vf->canvas0_config[2] = vc1_canvas_config[buffer_index][2];
		vf->plane_num = 3;
#endif
		//}
		if (get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_T5D && vdec->use_vfm_path &&
			vdec_stream_based(vdec)) {
			vf->type |= VIDTYPE_FORCE_SIGN_IP_JOINT;
		}
		vc1_print(0, VC1_DEBUG_DETAIL, "%s: display_q index %d, pts 0x%x/0x%x\n", __func__, vf->index, vf->pts, vf->pts_us64);
		decoder_do_frame_check(vdec, vf);
		kfifo_put(&display_q, (const struct vframe_s *)vf);
		ATRACE_COUNTER(MODULE_NAME, vf->pts);

		vf_notify_receiver(PROVIDER_NAME,
				VFRAME_EVENT_PROVIDER_VFRAME_READY,
				NULL);
	}
	return 0;
}

static int is_oversize(int w, int h)
{
	int max = MAX_SIZE_2K;

	if (w <= 0 || h <= 0)
		return true;

	if (h != 0 && (w > max / h))
		return true;

	if (w > h) {
		if (w > 1920 || h > 1088)
			return true;
	} else if (w < h) {
		if (w > 1088 || h > 1920)
			return true;
	} else {
		if (w*h > 1920 *1088)
			return true;
	}

	return false;
}

static irqreturn_t vvc1_isr_thread_handler(int irq, void *dev_id)
{
	struct vdec_vc1_hw_s *hw = &vc1_hw;
	u32 reg;
	u32 repeat_count;
	u32 picture_type;
	u32 buffer_index;
	unsigned int pts, pts_valid = 0, offset = 0;
	u32 v_width, v_height;
	u64 pts_us64 = 0;
	u32 frame_size;
	u32 debug_tag;
	u32 status_reg;
	int ret;
	bool is_bi_type;

	debug_tag = READ_VREG(DEBUG_REG1);
	if (debug_tag != 0) {
		vc1_print(0, 0, "%s: dbg%x: %x, wp 0x%x, rp 0x%x\n", __func__,
			debug_tag, READ_VREG(DEBUG_REG2),
			READ_VREG(VLD_MEM_VIFIFO_WP), READ_VREG(VLD_MEM_VIFIFO_RP));
		WRITE_VREG(DEBUG_REG1, 0);
		return IRQ_HANDLED;
	}

	status_reg = READ_VREG(DECODE_STATUS);

	if (status_reg == DECODE_STATUS_SEQ_HEADER_DONE) {//seq heard done
		reg = READ_VREG(VC1_PIC_INFO);
		hw->frame_width = READ_VREG(VC1_PIC_INFO) & 0x3fff;
		hw->frame_height = (READ_VREG(VC1_PIC_INFO) >> 14) & 0x3fff;
		hw->interlace_flag = (READ_VREG(VC1_PIC_INFO) >> 28) & 0x1;
		vc1_print(0, 0, "%s: SEQ_HEADER_DONE frame_width %d/%d, interlace_flag %d\n", __func__,
			hw->frame_width, hw->frame_height, hw->interlace_flag);
		WRITE_VREG(DECODE_STATUS, 0);
		return IRQ_HANDLED;
	} else if (status_reg == DECODE_STATUS_PARAM_CHECK) {
		v_width = READ_VREG(VC1_PIC_INFO) & 0x3fff;
		v_height = (READ_VREG(VC1_PIC_INFO) >> 14) & 0x3fff;

		if (is_oversize(v_width, v_height)) {
			WRITE_VREG(VC1_PIC_INFO, 0);
			vc1_print(0, VC1_DEBUG_DETAIL, "%s: oversize v_width %d, v_height %d\n",
				__func__, v_width, v_height);
		}
		WRITE_VREG(DECODE_STATUS, 0);
		return IRQ_HANDLED;
	} else if (status_reg == DECODE_STATUS_PIC_SKIPPED) {//PIC Skipped
		vc1_print(0, VC1_DEBUG_DETAIL, "%s: PIC_SKIPPED picture_type invalid\n", __func__);
		WRITE_VREG(DECODE_STATUS, 0);
		return IRQ_HANDLED;
	} else if (status_reg == DECODE_STATUS_BUF_INVALID) {
		vc1_print(0, VC1_DEBUG_DETAIL, "%s: BUF_INVALID \n", __func__);
		WRITE_VREG(DECODE_STATUS, 0);
		return IRQ_HANDLED;
	} else if (status_reg == DECODE_STATUS_PIC_HEADER_DONE) {//PIC header done
		hw->new_type = READ_VREG(AV_SCRATCH_K);
		vc1_print(0, VC1_DEBUG_DETAIL, "%s: PIC_HEADER_DONE picture_type %d(%s)\n", __func__, hw->new_type,
			((hw->new_type == I_PICTURE) ? "I" : ((hw->new_type == P_PICTURE) ? "P" : "B")));
	}

	reg = READ_VREG(VC1_BUFFEROUT);
	if (reg) {
		v_width = hw->frame_width;//READ_VREG(AV_SCRATCH_J);
		v_height = hw->frame_height;//READ_VREG(AV_SCRATCH_K);
		vc1_print(0, VC1_DEBUG_DETAIL, "%s: v_width %d, v_height %d\n",
					__func__, v_width, v_height);

		vc1_set_rp();
		if (v_width && v_width <= 4096
			&& (v_width != vvc1_amstream_dec_info.width)) {
			pr_info("frame width changed %d to %d\n",
				   vvc1_amstream_dec_info.width, v_width);
			vvc1_amstream_dec_info.width = v_width;
			frame_width = v_width;
		}
		if (v_height && v_height <= 4096
			&& (v_height != vvc1_amstream_dec_info.height)) {
			pr_info("frame height changed %d to %d\n",
				   vvc1_amstream_dec_info.height, v_height);
			vvc1_amstream_dec_info.height = v_height;
			frame_height = v_height;
		}

		if (pts_by_offset) {
			offset = READ_VREG(VC1_OFFSET_REG);
			if (pts_lookup_offset_us64(
					PTS_TYPE_VIDEO,
					offset, &pts, &frame_size,
					0, &pts_us64) == 0) {
				hw->pics[hw->decoding_index].pts_valid = 1;
				hw->pics[hw->decoding_index].pts = pts;
				hw->pics[hw->decoding_index].pts64 = pts_us64;

#ifdef DEBUG_PTS
				pts_hit++;
#endif
			} else {
				hw->pics[hw->decoding_index].pts_valid = 0;
				hw->pics[hw->decoding_index].pts = 0;
				hw->pics[hw->decoding_index].pts64 = 0;

#ifdef DEBUG_PTS
				pts_missed++;
#endif
			}
		}

		repeat_count = READ_VREG(VC1_REPEAT_COUNT);
		buffer_index = ((reg & 0x7) - 1) & 3;
		is_bi_type = (reg >> 8) & 0x1;
		if (is_bi_type)
			picture_type = B_PICTURE + 1;//BI, should set as B_PICTURE
		else
			picture_type = (reg >> 3) & 7;//I:0,P:1,B:2

		hw->interlace_flag = (reg & INTERLACE_FLAG) ? 1 : 0;
		hw->pics[hw->decoding_index].offset = READ_VREG(VC1_OFFSET_REG);
		hw->pics[hw->decoding_index].repeat_cnt = READ_VREG(VC1_REPEAT_COUNT);
		hw->pics[hw->decoding_index].buffer_info = reg;
		hw->pics[hw->decoding_index].index = hw->decoding_index;
		//hw->pics[hw->decoding_index].decode_pic_count = decode_pic_count;
		hw->pics[hw->decoding_index].picture_type = picture_type;
		vc1_print(0, VC1_DEBUG_DETAIL, "%s: get buffer_index %d, decoding_index %d, buffer_info 0x%x, index %d, picture_type %d\n",
					__func__, buffer_index, hw->decoding_index,
					hw->pics[hw->decoding_index].buffer_info,
					hw->pics[hw->decoding_index].index,
					hw->pics[hw->decoding_index].picture_type);

#ifdef DEBUG_PTS
		if (picture_type == I_PICTURE) {
			/* pr_info("I offset 0x%x,
			 *pts_valid %d\n", offset, pts_valid);
			 */
			if (!pts_valid)
				pts_i_missed++;
			else
				pts_i_hit++;
		}
#endif

		if ((pts_valid) && (frm.state != RATE_MEASURE_DONE)) {
			if (frm.state == RATE_MEASURE_START_PTS) {
				frm.start_pts = pts;
				frm.state = RATE_MEASURE_END_PTS;
				frm.trymax = RATE_MEASURE_NUM;
			} else if (frm.state == RATE_MEASURE_END_PTS) {
				if (frm.num >= frm.trymax) {
					frm.end_pts = pts;
					frm.rate = (frm.end_pts -
						frm.start_pts) / frm.num;
					pr_info("frate before=%d,%d,num=%d\n",
					frm.rate,
					DUR2PTS(vvc1_amstream_dec_info.rate),
					frm.num);
					/* check if measured rate is same as
					 * settings from upper layer
					 * and correct it if necessary
					 */
					if ((close_to(frm.rate, RATE_30_FPS,
						RATE_CORRECTION_THRESHOLD) &&
						close_to(
						DUR2PTS(
						vvc1_amstream_dec_info.rate),
						RATE_24_FPS,
						RATE_CORRECTION_THRESHOLD))
						||
						(close_to(
						frm.rate, RATE_24_FPS,
						RATE_CORRECTION_THRESHOLD)
						&&
						close_to(DUR2PTS(
						vvc1_amstream_dec_info.rate),
						RATE_30_FPS,
						RATE_CORRECTION_THRESHOLD))) {
						pr_info(
						"vvc1: frate from %d to %d\n",
						vvc1_amstream_dec_info.rate,
						PTS2DUR(frm.rate));

						vvc1_amstream_dec_info.rate =
							PTS2DUR(frm.rate);
						frm.state = RATE_MEASURE_DONE;
					} else if (close_to(frm.rate,
						DUR2PTS(
						vvc1_amstream_dec_info.rate),
						RATE_CORRECTION_THRESHOLD))
						frm.state = RATE_MEASURE_DONE;
					else {

/* maybe still have problem,
 *						try next double frames....
 */
						frm.state = RATE_MEASURE_DONE;
						frm.start_pts = pts;
						frm.state =
						RATE_MEASURE_END_PTS;
						/*60 fps*60 S */
						frm.num = 0;
					}
				}
			}
		}

		if (frm.state != RATE_MEASURE_DONE)
			frm.num += (repeat_count > 1) ? repeat_count : 1;
		if (vvc1_amstream_dec_info.rate == 0)
			vvc1_amstream_dec_info.rate = PTS2DUR(frm.rate);

		if (hw->throw_pb_flag && picture_type != I_PICTURE) {
				vc1_print(0, VC1_DEBUG_DETAIL, "%s: WRITE_VREG(VC1_BUFFERIN, 0x%x) decoding_index %d, for throwing picture with type of %d\n",
					__func__, ~(1 << hw->decoding_index), hw->decoding_index, picture_type);

				WRITE_VREG(VC1_BUFFERIN, ~(1 << hw->decoding_index));
				hw->buf_use[hw->decoding_index]--;
				vc1_print(0, VC1_DEBUG_DETAIL,"%s: index %d, buf_use %d, buffer_index %d\n",
					__func__, hw->decoding_index, hw->buf_use[hw->decoding_index], buffer_index);
			} else {
				if ((picture_type == I_PICTURE) ||
					(picture_type == P_PICTURE)) {
					buffer_index = update_reference(hw, hw->decoding_index);
				} else {
					/* drop b frame before reference pic ready */
					if (hw->refs[0] == -1) {
						buffer_index = hw->vf_buf_num_used;
						WRITE_VREG(VC1_BUFFERIN, ~(1 << hw->decoding_index));
						hw->buf_use[hw->decoding_index]--;
						vc1_print(0, VC1_DEBUG_DETAIL,"%s: decoding_index %d, buf_use %d\n",
							__func__, hw->decoding_index, hw->buf_use[hw->decoding_index]);
					}
				}

				if (buffer_index < hw->vf_buf_num_used) {
					vc1_print(0, VC1_DEBUG_DETAIL, "%s: show buffer_index %d\n",	__func__, buffer_index);
					prepare_display_buf(hw, &hw->pics[buffer_index]);
				} else {
					vc1_print(0, VC1_DEBUG_DETAIL, "%s: drop buffer_index %d\n",	__func__, buffer_index);
				}
			}

			frame_dur = vvc1_amstream_dec_info.rate;
			total_frame++;

			/*count info*/
			gvs->frame_dur = frame_dur;
			vdec_count_info(gvs, 0, offset);
	}

	ret = vvc1_config_buf(hw);
	if (ret != 0) {
		do {
			msleep(wait_time);
			ret = vvc1_config_buf(hw);
			vc1_print(0, VC1_DEBUG_DETAIL, "%s: ret %d, wait_time %d, is_reset %d\n",
						__func__, ret, wait_time, is_reset);

		} while ((ret == -1) && (!is_reset));
	}
	WRITE_VREG(DECODE_STATUS, 0);

	return IRQ_HANDLED;
}

static irqreturn_t vvc1_isr_thread_fn(int irq, void *dev_id)
{
	irqreturn_t ret;

	ret = vvc1_isr_thread_handler(irq, dev_id);

	process_busy = false;

	return ret;
}

static irqreturn_t vvc1_isr(int irq, void *dev_id)
{
	if (process_busy)
		return IRQ_HANDLED;

	process_busy = true;

	WRITE_VREG(ASSIST_MBOX1_CLR_REG, 1);

	return IRQ_WAKE_THREAD;
}

static struct vframe_s *vvc1_vf_peek(void *op_arg)
{
	struct vframe_s *vf;

	if (kfifo_peek(&display_q, &vf))
		return vf;

	return NULL;
}

static struct vframe_s *vvc1_vf_get(void *op_arg)
{
	struct vframe_s *vf;

	if (kfifo_get(&display_q, &vf))
		return vf;

	return NULL;
}

static void vvc1_vf_put(struct vframe_s *vf, void *op_arg)
{
	if (pool_index(vf) == cur_pool_idx)
		kfifo_put(&recycle_q, (const struct vframe_s *)vf);
}

static int vvc1_vf_states(struct vframe_states *states, void *op_arg)
{
	unsigned long flags;

	spin_lock_irqsave(&lock, flags);

	states->vf_pool_size = VF_POOL_SIZE;
	states->buf_free_num = kfifo_len(&newframe_q);
	states->buf_avail_num = kfifo_len(&display_q);
	states->buf_recycle_num = kfifo_len(&recycle_q);

	spin_unlock_irqrestore(&lock, flags);

	return 0;
}

static int vvc1_event_cb(int type, void *data, void *private_data)
{
	if (type & VFRAME_EVENT_RECEIVER_RESET) {
		unsigned long flags;

		amvdec_stop();
#ifndef CONFIG_AMLOGIC_POST_PROCESS_MANAGER
		vf_light_unreg_provider(&vvc1_vf_prov);
#endif
		spin_lock_irqsave(&lock, flags);
		vvc1_local_init(true);
		vvc1_prot_init();
		spin_unlock_irqrestore(&lock, flags);
#ifndef CONFIG_AMLOGIC_POST_PROCESS_MANAGER
		vf_reg_provider(&vvc1_vf_prov);
#endif
		amvdec_start();
	}

	if (type & VFRAME_EVENT_RECEIVER_REQ_STATE) {
		struct provider_state_req_s *req =
			(struct provider_state_req_s *)data;
		if (req->req_type == REQ_STATE_SECURE && vdec)
			req->req_result[0] = vdec_secure(vdec);
		else
			req->req_result[0] = 0xffffffff;
	}
	return 0;
}

int vvc1_dec_status(struct vdec_s *vdec, struct vdec_info *vstatus)
{
	if (!(stat & STAT_VDEC_RUN))
		return -1;

	vstatus->frame_width = vvc1_amstream_dec_info.width;
	vstatus->frame_height = vvc1_amstream_dec_info.height;
	if (vvc1_amstream_dec_info.rate != 0)
		vstatus->frame_rate = 96000 / vvc1_amstream_dec_info.rate;
	else
		vstatus->frame_rate = -1;
	vstatus->error_count = READ_VREG(AV_SCRATCH_C);
	vstatus->status = stat;
	vstatus->bit_rate = gvs->bit_rate;
	vstatus->frame_dur = vvc1_amstream_dec_info.rate;
	vstatus->frame_data = gvs->frame_data;
	vstatus->total_data = gvs->total_data;
	vstatus->frame_count = gvs->frame_count;
	vstatus->error_frame_count = gvs->error_frame_count;
	vstatus->drop_frame_count = gvs->drop_frame_count;
	vstatus->total_data = gvs->total_data;
	vstatus->samp_cnt = gvs->samp_cnt;
	vstatus->offset = gvs->offset;
	snprintf(vstatus->vdec_name, sizeof(vstatus->vdec_name),
		"%s", DRIVER_NAME);

	return 0;
}

int vvc1_set_isreset(struct vdec_s *vdec, int isreset)
{
	vc1_print(0, VC1_DEBUG_DETAIL,"%s: isreset 0x%x\n", __func__, isreset);

	is_reset = isreset;
	return 0;
}

static int vvc1_vdec_info_init(void)
{

	gvs = kzalloc(sizeof(struct vdec_info), GFP_KERNEL);
	if (NULL == gvs) {
		pr_info("the struct of vdec status malloc failed.\n");
		return -ENOMEM;
	}
	return 0;
}

/****************************************/
static int vvc1_canvas_init(void)
{
	int i, ret;
	u32 canvas_width, canvas_height;
	u32 alloc_size, decbuf_size, decbuf_y_size, decbuf_uv_size;
	unsigned long buf_start;
	struct vdec_vc1_hw_s *hw = &vc1_hw;
	int endian = (hw->canvas_mode == CANVAS_BLKMODE_LINEAR) ? 7 : 0;

	if (buf_size <= 0x00400000) {
		/* SD only */
		canvas_width = 768;
		canvas_height = 576;
		decbuf_y_size = 0x80000;
		decbuf_uv_size = 0x20000;
		decbuf_size = 0x100000;
	} else {
		/* HD & SD */
		canvas_width = 1920;
		canvas_height = 1088;
		if (vvc1_amstream_dec_info.width < vvc1_amstream_dec_info.height) {
			canvas_width = 1088;
			canvas_height = 1920;
			if (vvc1_amstream_dec_info.width > 1088) {
				canvas_width = ALIGN(vvc1_amstream_dec_info.width, 64);
				canvas_height = ALIGN(vvc1_amstream_dec_info.height, 64);
			}
		}
		decbuf_y_size = 0x200000;
		decbuf_uv_size = 0x80000;
		decbuf_size = 0x300000;
	}

	for (i = 0; i < MAX_BMMU_BUFFER_NUM; i++) {
		/* workspace mem */
		if (i == (MAX_BMMU_BUFFER_NUM - 1))
			alloc_size = WORKSPACE_SIZE;
		else
			alloc_size = decbuf_size;
		vc1_print(0, VC1_DEBUG_DETAIL, "%s: i %d, alloc_size %d\n", __func__, i, alloc_size);

		ret = decoder_bmmu_box_alloc_buf_phy(mm_blk_handle, i,
				alloc_size, DRIVER_NAME, &buf_start);
		if (ret < 0)
			return ret;
		if (i == (MAX_BMMU_BUFFER_NUM - 1)) {
			buf_offset = buf_start - DCAC_BUFF_START_ADDR;
			break;
		}

#ifdef NV21
		config_cav_lut_ex(2 * i + 0,
			buf_start,
			canvas_width, canvas_height,
			CANVAS_ADDR_NOWRAP, hw->canvas_mode, endian, VDEC_1);
		vc1_canvas_config[i][0].endian = endian;
		vc1_canvas_config[i][0].width = canvas_width;
		vc1_canvas_config[i][0].height = canvas_height;
		vc1_canvas_config[i][0].block_mode = hw->canvas_mode;
		vc1_canvas_config[i][0].phy_addr = buf_start;

		config_cav_lut_ex(2 * i + 1,
			buf_start +
			decbuf_y_size, canvas_width,
			canvas_height / 2, CANVAS_ADDR_NOWRAP,
			hw->canvas_mode, 0, VDEC_1);
		vc1_canvas_config[i][1].endian = endian;
		vc1_canvas_config[i][1].width = canvas_width;
		vc1_canvas_config[i][1].height = canvas_height >> 1;
		vc1_canvas_config[i][1].block_mode = hw->canvas_mode;
		vc1_canvas_config[i][1].phy_addr = buf_start + decbuf_y_size;
#else
		config_cav_lut_ex(3 * i + 0,
			buf_start,
			canvas_width, canvas_height,
			CANVAS_ADDR_NOWRAP, hw->canvas_mode, endian, VDEC_1);
		vc1_canvas_config[i][0].endian = endian;
		vc1_canvas_config[i][0].width = canvas_width;
		vc1_canvas_config[i][0].height = canvas_height;
		vc1_canvas_config[i][0].block_mode = hw->canvas_mode;
		vc1_canvas_config[i][0].phy_addr = buf_start;
		config_cav_lut_ex(3 * i + 1,
			buf_start +
			decbuf_y_size, canvas_width / 2,
			canvas_height / 2, CANVAS_ADDR_NOWRAP,
			hw->canvas_mode, endian, VDEC_1);
		vc1_canvas_config[i][1].endian = endian;
		vc1_canvas_config[i][1].width = canvas_width >> 1;
		vc1_canvas_config[i][1].height = canvas_height >> 1;
		vc1_canvas_config[i][1].block_mode = hw->canvas_mode;
		vc1_canvas_config[i][1].phy_addr = buf_start + decbuf_y_size;
		config_cav_lut_ex(3 * i + 2,
			buf_start +
			decbuf_y_size + decbuf_uv_size,
			canvas_width / 2, canvas_height / 2,
			CANVAS_ADDR_NOWRAP, hw->canvas_mode, endian, VDEC_1);
		vc1_canvas_config[i][2].endian = endian;
		vc1_canvas_config[i][2].width = canvas_width >> 1;
		vc1_canvas_config[i][2].height = canvas_height >> 1;
		vc1_canvas_config[i][2].block_mode = hw->canvas_mode;
		vc1_canvas_config[i][2].phy_addr = buf_start +
			decbuf_y_size + decbuf_uv_size;
#endif

	}
	return 0;
}

static int vvc1_prot_init(void)
{
	int r;
#if 1	/* /MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6 */
	WRITE_VREG(DOS_SW_RESET0, (1 << 7) | (1 << 6) | (1 << 4));
	WRITE_VREG(DOS_SW_RESET0, 0);

	READ_VREG(DOS_SW_RESET0);

	WRITE_VREG(DOS_SW_RESET0, (1 << 7) | (1 << 6) | (1 << 4));
	WRITE_VREG(DOS_SW_RESET0, 0);

	WRITE_VREG(DOS_SW_RESET0, (1 << 9) | (1 << 8));
	WRITE_VREG(DOS_SW_RESET0, 0);

#else
	WRITE_RESET_REG(RESET0_REGISTER,
				   RESET_IQIDCT | RESET_MC | RESET_VLD_PART);
	READ_RESET_REG(RESET0_REGISTER);
	WRITE_RESET_REG(RESET0_REGISTER,
				   RESET_IQIDCT | RESET_MC | RESET_VLD_PART);

	WRITE_RESET_REG(RESET2_REGISTER, RESET_PIC_DC | RESET_DBLK);
#endif

	WRITE_VREG(POWER_CTL_VLD, 0x10);
	WRITE_VREG_BITS(VLD_MEM_VIFIFO_CONTROL, 2, MEM_FIFO_CNT_BIT, 2);
	WRITE_VREG_BITS(VLD_MEM_VIFIFO_CONTROL, 8, MEM_LEVEL_CNT_BIT, 6);

	r = vvc1_canvas_init();

	/* index v << 16 | u << 8 | y */
#ifdef NV21
	WRITE_VREG(AV_SCRATCH_0, 0x010100);
	WRITE_VREG(AV_SCRATCH_1, 0x030302);
	WRITE_VREG(AV_SCRATCH_2, 0x050504);
	WRITE_VREG(AV_SCRATCH_3, 0x070706);
/*	WRITE_VREG(AV_SCRATCH_G, 0x090908);
	WRITE_VREG(AV_SCRATCH_H, 0x0b0b0a);
	WRITE_VREG(AV_SCRATCH_I, 0x0d0d0c);
	WRITE_VREG(AV_SCRATCH_J, 0x0f0f0e);*/
#else
	WRITE_VREG(AV_SCRATCH_0, 0x020100);
	WRITE_VREG(AV_SCRATCH_1, 0x050403);
	WRITE_VREG(AV_SCRATCH_2, 0x080706);
	WRITE_VREG(AV_SCRATCH_3, 0x0b0a09);
	WRITE_VREG(AV_SCRATCH_G, 0x090908);
	WRITE_VREG(AV_SCRATCH_H, 0x0b0b0a);
	WRITE_VREG(AV_SCRATCH_I, 0x0d0d0c);
	WRITE_VREG(AV_SCRATCH_J, 0x0f0f0e);
#endif

	/* notify ucode the buffer offset */
	WRITE_VREG(AV_SCRATCH_F, buf_offset);

	/* disable PSCALE for hardware sharing */
	WRITE_VREG(PSCALE_CTRL, 0);

	WRITE_VREG(VC1_SOS_COUNT, 0);
	WRITE_VREG(VC1_BUFFERIN, 0);
	WRITE_VREG(VC1_BUFFEROUT, NEW_DRV_VER);//identify new driver version
	vc1_print(0, VC1_DEBUG_DETAIL,"%s VC1_BUFFEROUT %d\n", __func__, NEW_DRV_VER);

	/* clear mailbox interrupt */
	WRITE_VREG(ASSIST_MBOX1_CLR_REG, 1);

	/* enable mailbox interrupt */
	WRITE_VREG(ASSIST_MBOX1_MASK, 1);

#ifdef NV21
	SET_VREG_MASK(MDEC_PIC_DC_CTRL, 1 << 17);
#endif
	CLEAR_VREG_MASK(MDEC_PIC_DC_CTRL, 1 << 16);

	WRITE_VREG(CANVAS_BUF_REG, 0);
	WRITE_VREG(ANC0_CANVAS_REG, 0);
	WRITE_VREG(ANC1_CANVAS_REG, 0);
	WRITE_VREG(DECODE_STATUS, 0);
	WRITE_VREG(VC1_PIC_INFO, 0);
	WRITE_VREG(DEBUG_REG1, 0);
	WRITE_VREG(DEBUG_REG2, 0);

	WRITE_VREG(AV_SCRATCH_L, udebug_flag);
	return r;
}

static void vvc1_local_init(bool is_reset)
{
	struct vdec_vc1_hw_s *hw = &vc1_hw;
	int i;
	vc1_print(0, VC1_DEBUG_DETAIL,"%s \n", __func__);

	/* vvc1_ratio = 0x100; */
	vvc1_ratio = vvc1_amstream_dec_info.ratio;

	avi_flag = (unsigned long) vvc1_amstream_dec_info.param & 0x01;

	unstable_pts = (((unsigned long) vvc1_amstream_dec_info.param & 0x40) >> 6);
	if (unstable_pts_debug == 1) {
		unstable_pts = 1;
		pr_info("vc1 init , unstable_pts_debug = %u\n",unstable_pts_debug);
	}
	total_frame = 0;

	next_pts = 0;

	next_pts_us64 = 0;
	saved_resolution = 0;
	frame_width = frame_height = frame_dur = 0;
	process_busy = false;
#ifdef DEBUG_PTS
	pts_hit = pts_missed = pts_i_hit = pts_i_missed = 0;
#endif

	memset(&frm, 0, sizeof(frm));

	if (!is_reset) {
		hw->refs[0] = -1;
		hw->refs[1] = -1;
		hw->throw_pb_flag = 1;
		hw->vf_buf_num_used = DECODE_BUFFER_NUM_MAX;
		if (hw->vf_buf_num_used > DECODE_BUFFER_NUM_MAX)
			hw->vf_buf_num_used = DECODE_BUFFER_NUM_MAX;

		for (i = 0; i < hw->vf_buf_num_used; i++) {
			hw->vfbuf_use[i] = 0;
			hw->buf_use[i] = 0;
			hw->ref_use[i] = 0;
		}

		INIT_KFIFO(display_q);
		INIT_KFIFO(recycle_q);
		INIT_KFIFO(newframe_q);
		cur_pool_idx ^= 1;
		for (i = 0; i < VF_POOL_SIZE; i++) {
			const struct vframe_s *vf;

			if (cur_pool_idx == 0) {
				vf = &vfpool[i];
				vfpool[i].index = DECODE_BUFFER_NUM_MAX;
			} else {
				vf = &vfpool2[i];
				vfpool2[i].index = DECODE_BUFFER_NUM_MAX;
			}
			kfifo_put(&newframe_q, (const struct vframe_s *)vf);
		}
	}

	if (mm_blk_handle) {
		decoder_bmmu_box_free(mm_blk_handle);
		mm_blk_handle = NULL;
	}

		mm_blk_handle = decoder_bmmu_box_alloc_box(
			DRIVER_NAME,
			0,
			MAX_BMMU_BUFFER_NUM,
			4 + PAGE_SHIFT,
			CODEC_MM_FLAGS_CMA_CLEAR |
			CODEC_MM_FLAGS_FOR_VDECODER,
			BMMU_ALLOC_FLAGS_WAITCLEAR);
}

#ifdef CONFIG_AMLOGIC_POST_PROCESS_MANAGER
static void vvc1_ppmgr_reset(void)
{
	vc1_print(0, VC1_DEBUG_DETAIL,"%s: ", __func__);

	vf_notify_receiver(PROVIDER_NAME, VFRAME_EVENT_PROVIDER_RESET, NULL);

	vvc1_local_init(true);

	/* vf_notify_receiver(PROVIDER_NAME,
	 * VFRAME_EVENT_PROVIDER_START,NULL);
	 */

	pr_info("vvc1dec: vf_ppmgr_reset\n");
}
#endif

static void vvc1_set_clk(struct work_struct *work)
{
		int fps = 96000 / frame_dur;

		saved_resolution = frame_width * frame_height * fps;
		vdec_source_changed(VFORMAT_VC1,
			frame_width, frame_height, fps);

}

static void error_do_work(struct work_struct *work)
{
	vc1_print(0, VC1_DEBUG_DETAIL,"%s \n", __func__);

	amvdec_stop();
	msleep(20);
#ifdef CONFIG_AMLOGIC_POST_PROCESS_MANAGER
	vvc1_ppmgr_reset();
#else
	vf_light_unreg_provider(&vvc1_vf_prov);
	vvc1_local_init(true);
	vf_reg_provider(&vvc1_vf_prov);
#endif
	vvc1_prot_init();
	amvdec_start();
}

static void vvc1_put_timer_func(struct timer_list *timer)
{
	struct vdec_vc1_hw_s *hw = &vc1_hw;

	if (READ_VREG(VC1_SOS_COUNT) > 10)
		schedule_work(&error_wd_work);

	vc1_set_rp();

	while (!kfifo_is_empty(&recycle_q)/* && (READ_VREG(VC1_BUFFERIN) == 0)*/) {
		struct vframe_s *vf;

		if (kfifo_get(&recycle_q, &vf)) {
			if ((vf->index < hw->vf_buf_num_used) &&
			 (--hw->vfbuf_use[vf->index] == 0)) {
				hw->buf_use[vf->index]--;
				vc1_print(0, VC1_DEBUG_DETAIL,	"%s WRITE_VREG(VC1_BUFFERIN, 0x%x) for vf index of %d,  buf_use %d\n",
					__func__, ~(1 << vf->index), vf->index, hw->buf_use[vf->index]);
				WRITE_VREG(VC1_BUFFERIN, ~(1 << vf->index));
				vf->index = hw->vf_buf_num_used;
			}
			if (pool_index(vf) == cur_pool_idx)
				kfifo_put(&newframe_q, (const struct vframe_s *)vf);
		}

	}

	if (frame_dur > 0 && saved_resolution !=
		frame_width * frame_height * (96000 / frame_dur))
		schedule_work(&set_clk_work);
	timer->expires = jiffies + PUT_INTERVAL;

	add_timer(timer);
}

static s32 vvc1_init(void)
{
	int ret = -1;
	char *buf = vmalloc(0x1000 * 16);
	int fw_type = VIDEO_DEC_VC1;

	if (IS_ERR_OR_NULL(buf))
		return -ENOMEM;

	pr_info("vvc1_init, format %d\n", vvc1_amstream_dec_info.format);
	timer_setup(&recycle_timer, vvc1_put_timer_func, 0);

	stat |= STAT_TIMER_INIT;

	intra_output = 0;
	amvdec_enable();

	vvc1_local_init(false);

	if (vvc1_amstream_dec_info.format == VIDEO_DEC_FORMAT_WMV3) {
		pr_info("WMV3 dec format\n");
		vvc1_format = VIDEO_DEC_FORMAT_WMV3;
		WRITE_VREG(AV_SCRATCH_4, 0);
	} else if (vvc1_amstream_dec_info.format == VIDEO_DEC_FORMAT_WVC1) {
		pr_info("WVC1 dec format\n");
		vvc1_format = VIDEO_DEC_FORMAT_WVC1;
		WRITE_VREG(AV_SCRATCH_4, 1);
	} else
		pr_info("not supported VC1 format\n");

	if (get_firmware_data(fw_type, buf) < 0) {
		amvdec_disable();
		pr_err("get firmware fail.");
		vfree(buf);
		return -1;
	}

	ret = amvdec_loadmc_ex(VFORMAT_VC1, NULL, buf);
	if (ret < 0) {
		amvdec_disable();
		vfree(buf);
		pr_err("VC1: the %s fw loading failed, err: %x\n",
			fw_tee_enabled() ? "TEE" : "local", ret);
		return -EBUSY;
	}

	vfree(buf);

	stat |= STAT_MC_LOAD;

	/* enable AMRISC side protocol */
	ret = vvc1_prot_init();
	if (ret < 0)
		return ret;

	if (vdec_request_threaded_irq(VDEC_IRQ_1, vvc1_isr,
			vvc1_isr_thread_fn, IRQF_SHARED,
			"vvc1-irq", (void *)vvc1_dec_id)) {
		amvdec_disable();

		pr_info("vvc1 irq register error.\n");
		return -ENOENT;
	}

	stat |= STAT_ISR_REG;
#ifdef CONFIG_AMLOGIC_POST_PROCESS_MANAGER
	vf_provider_init(&vvc1_vf_prov,
		PROVIDER_NAME, &vvc1_vf_provider, NULL);
	vf_reg_provider(&vvc1_vf_prov);
	vf_notify_receiver(PROVIDER_NAME,
		VFRAME_EVENT_PROVIDER_START, NULL);
#else
	vf_provider_init(&vvc1_vf_prov,
		PROVIDER_NAME, &vvc1_vf_provider, NULL);
	vf_reg_provider(&vvc1_vf_prov);
#endif

	if (!is_reset)
		vf_notify_receiver(PROVIDER_NAME,
				VFRAME_EVENT_PROVIDER_FR_HINT,
				(void *)
				((unsigned long)vvc1_amstream_dec_info.rate));

	stat |= STAT_VF_HOOK;

	recycle_timer.expires = jiffies + PUT_INTERVAL;
	add_timer(&recycle_timer);

	stat |= STAT_TIMER_ARM;

	amvdec_start();

	stat |= STAT_VDEC_RUN;

	return 0;
}

static int amvdec_vc1_probe(struct platform_device *pdev)
{
	struct vdec_s *pdata = *(struct vdec_s **)pdev->dev.platform_data;
	vc1_print(0, VC1_DEBUG_DETAIL,"%s \n", __func__);

	if (pdata == NULL) {
		pr_info("amvdec_vc1 memory resource undefined.\n");
		return -EFAULT;
	}

	if (pdata->sys_info) {
		vvc1_amstream_dec_info = *pdata->sys_info;

		if ((vvc1_amstream_dec_info.height != 0) &&
			(vvc1_amstream_dec_info.width >
			(VC1_MAX_SUPPORT_SIZE/vvc1_amstream_dec_info.height))) {
			pr_info("amvdec_vc1: over size, unsupport: %d * %d\n",
				vvc1_amstream_dec_info.width,
				vvc1_amstream_dec_info.height);
			return -EFAULT;
		}
	}
	pdata->dec_status = vvc1_dec_status;
	pdata->set_isreset = vvc1_set_isreset;
	is_reset = 0;
	vdec = pdata;

	vc1_hw.canvas_mode = pdata->canvas_mode;

	vvc1_vdec_info_init();

	INIT_WORK(&error_wd_work, error_do_work);
	INIT_WORK(&set_clk_work, vvc1_set_clk);
	spin_lock_init(&vc1_rp_lock);
	if (vvc1_init() < 0) {
		pr_info("amvdec_vc1 init failed.\n");
		kfree(gvs);
		gvs = NULL;
		pdata->dec_status = NULL;
		return -ENODEV;
	}

	return 0;
}

static int amvdec_vc1_remove(struct platform_device *pdev)
{
	cancel_work_sync(&error_wd_work);
	vc1_print(0, VC1_DEBUG_DETAIL,"%s \n", __func__);
	if (stat & STAT_VDEC_RUN) {
		amvdec_stop();
		stat &= ~STAT_VDEC_RUN;
	}

	if (stat & STAT_ISR_REG) {
		vdec_free_irq(VDEC_IRQ_1, (void *)vvc1_dec_id);
		stat &= ~STAT_ISR_REG;
	}

	if (stat & STAT_TIMER_ARM) {
		del_timer_sync(&recycle_timer);
		stat &= ~STAT_TIMER_ARM;
	}

	cancel_work_sync(&set_clk_work);
	if (stat & STAT_VF_HOOK) {
		if (!is_reset)
			vf_notify_receiver(PROVIDER_NAME,
					VFRAME_EVENT_PROVIDER_FR_END_HINT,
					NULL);

		vf_unreg_provider(&vvc1_vf_prov);
		stat &= ~STAT_VF_HOOK;
	}

	amvdec_disable();

	if (get_cpu_major_id() >= AM_MESON_CPU_MAJOR_ID_TM2)
		vdec_reset_core(vdec);

	if (mm_blk_handle) {
		decoder_bmmu_box_free(mm_blk_handle);
		mm_blk_handle = NULL;
	}

#ifdef DEBUG_PTS
	pr_debug("pts hit %d, pts missed %d, i hit %d, missed %d\n", pts_hit,
		pts_missed, pts_i_hit, pts_i_missed);
	pr_debug("total frame %d, avi_flag %d, rate %d\n",
		total_frame, avi_flag,
		vvc1_amstream_dec_info.rate);
#endif
	kfree(gvs);
	gvs = NULL;
	vdec = NULL;

	return 0;
}

/****************************************/

static struct platform_driver amvdec_vc1_driver = {
	.probe = amvdec_vc1_probe,
	.remove = amvdec_vc1_remove,
#ifdef CONFIG_PM
	.suspend = amvdec_suspend,
	.resume = amvdec_resume,
#endif
	.driver = {
		.name = DRIVER_NAME,
	}
};

static int __init amvdec_vc1_driver_init_module(void)
{
	vc1_print(0, 0, "amvdec_vc1 module init\n");

	if (platform_driver_register(&amvdec_vc1_driver)) {
		pr_err("failed to register amvdec_vc1 driver\n");
		return -ENODEV;
	}

	vcodec_profile_register_v2("vc1", VFORMAT_VC1, 0);
	vcodec_feature_register(VFORMAT_VC1, 0);

	return 0;
}

static void __exit amvdec_vc1_driver_remove_module(void)
{
	pr_debug("amvdec_vc1 module remove.\n");

	platform_driver_unregister(&amvdec_vc1_driver);
}
module_param(unstable_pts_debug, uint, 0664);
MODULE_PARM_DESC(unstable_pts_debug, "\n amvdec_vc1 unstable_pts\n");

module_param(udebug_flag, uint, 0664);
MODULE_PARM_DESC(udebug_flag, "\n amvdec_vc1 udebug_flag\n");

module_param(debug, uint, 0664);
MODULE_PARM_DESC(debug, "\n amvdec_vc1 debug\n");

module_param(debug_mask, uint, 0664);
MODULE_PARM_DESC(debug_mask, "\n amvdec_vc1 debug_mask\n");

module_param(wait_time, uint, 0664);
MODULE_PARM_DESC(wait_time, "\n amvdec_vc1 wait_time\n");

/****************************************/
module_init(amvdec_vc1_driver_init_module);
module_exit(amvdec_vc1_driver_remove_module);

MODULE_DESCRIPTION("AMLOGIC VC1 Video Decoder Driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Qi Wang <qi.wang@amlogic.com>");
