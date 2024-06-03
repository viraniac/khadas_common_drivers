// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/math64.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_mode.h>
#include "hdmitx_log.h"

#define INVALID_HDMI_TIMING (&edid_cea_modes_0[0])

/* *
 * Used to traverse full timing table, idx start from 0.
 * Function will return NULL, when idx > timing_table_size.
 */
const struct hdmi_timing *hdmitx_mode_index_to_hdmi_timing(u32 idx);

/*
 * From CEA/CTA-861 spec.
 */
static const struct hdmi_timing edid_cea_modes_0[] = {
	{HDMI_0_UNKNOWN, "invalid", NULL},
	{HDMI_1_640x480p60_4x3, "640x480p60hz", NULL, 1, 31469, 59940, 25175,
		800, 160, 16, 96, 48, 640, 525, 45, 10, 2, 33, 480, 1, 0, 0, 4, 3, 1, 1},
	{HDMI_2_720x480p60_4x3, "720x480p60hz", "480p60hz", 1, 31469, 59940, 27000,
		858, 138, 16, 62, 60, 720, 525, 45, 9, 6, 30, 480, 7, 0, 0, 4, 3, 8, 9},
	{HDMI_3_720x480p60_16x9, "720x480p60hz", "480p60hz", 1, 31469, 59940, 27000,
		858, 138, 16, 62, 60, 720, 525, 45, 9, 6, 30, 480, 7, 0, 0, 16, 9, 32, 27},
	{HDMI_4_1280x720p60_16x9, "1280x720p60hz", "720p60hz", 1, 45000, 60000, 74250,
		1650, 370, 110, 40, 220, 1280, 750, 30, 5, 5, 20, 720, 1, 1, 1, 16, 9, 1, 1},
	{HDMI_5_1920x1080i60_16x9, "1920x1080i60hz", "1080i60hz", 0, 33750, 60000, 74250,
		2200, 280, 88, 44, 148, 1920, 1125, 22, 2, 5, 15, 1080, 1, 1, 1, 16, 9, 1, 1},
	{HDMI_6_720x480i60_4x3, "720x480i60hz", "480i60hz_4x3", 0, 15734, 59940, 27000,
		1716, 276, 38, 124, 114, 1440, 525, 22, 4, 3, 15, 480, 4, 0, 0, 4, 3, 8, 9, 1},
	{HDMI_7_720x480i60_16x9, "720x480i60hz", "480i60hz", 0, 15734, 59940, 27000,
		1716, 276, 38, 124, 114, 1440, 525, 22, 4, 3, 15, 480, 4, 0, 0, 16, 9, 32, 27, 1},
	{HDMI_8_720x240p60_4x3, "720x240p60hz", NULL, 1, 15734, 59826, 27000,
		1716, 276, 38, 124, 114, 1440, 263, 23, 5, 3, 15, 240, 4, 0, 0, 4, 3, 4, 9},
	{HDMI_9_720x240p60_16x9, "720x240p60hz", NULL, 1, 15734, 59826, 27000,
		1716, 276, 38, 124, 114, 1440, 263, 23, 5, 3, 15, 240, 4, 0, 0, 16, 9, 16, 27},
	{HDMI_10_2880x480i60_4x3, "2880x480i60hz", NULL, 0, 15734, 59940, 54000,
		3432, 552, 76, 248, 228, 2880, 525, 22, 4, 3, 15, 480, 4, 0, 0, 4, 3, 2, 9},
	{HDMI_11_2880x480i60_16x9, "2880x480i60hz", NULL, 0, 15734, 59940, 54000,
		3432, 552, 76, 248, 228, 2880, 525, 22, 4, 3, 15, 480, 4, 0, 0, 16, 9, 8, 27},
	{HDMI_12_2880x240p60_4x3, "2880x240p60hz", NULL, 1, 15734, 59826, 54000,
		3432, 552, 76, 248, 228, 2880, 263, 23, 5, 3, 15, 240, 4, 0, 0, 4, 3, 1, 9},
	{HDMI_13_2880x240p60_16x9, "2880x240p60hz", NULL, 1, 15734, 59826, 54000,
		3432, 552, 76, 248, 228, 2880, 263, 23, 5, 3, 15, 240, 4, 0, 0, 16, 9, 4, 27},
	{HDMI_14_1440x480p60_4x3, "1440x480p60hz", NULL, 1, 31469, 59940, 54000,
		1716, 276, 32, 124, 120, 1440, 525, 45, 9, 6, 30, 480, 7, 0, 0, 4, 3, 4, 9},
	{HDMI_15_1440x480p60_16x9, "1440x480p60hz", NULL, 1, 31469, 59940, 54000,
		1716, 276, 32, 124, 120, 1440, 525, 45, 9, 6, 30, 480, 7, 0, 0, 16, 9, 16, 27},
	{HDMI_16_1920x1080p60_16x9, "1920x1080p60hz", "1080p60hz", 1, 67500, 60000, 148500,
		2200, 280, 88, 44, 148, 1920, 1125, 45, 4, 5, 36, 1080, 1, 1, 1, 16, 9, 1, 1},
	{HDMI_17_720x576p50_4x3, "720x576p50hz", "576p50hz", 1, 31250, 50000, 27000,
		864, 144, 12, 64, 68, 720, 625, 49, 5, 5, 39, 576, 1, 0, 0, 4, 3, 16, 15},
	{HDMI_18_720x576p50_16x9, "720x576p50hz", "576p50hz", 1, 31250, 50000, 27000,
		864, 144, 12, 64, 68, 720, 625, 49, 5, 5, 39, 576, 1, 0, 0, 16, 9, 64, 65},
	{HDMI_19_1280x720p50_16x9, "1280x720p50hz", "720p50hz", 1, 37500, 50000, 74250,
		1980, 700, 440, 40, 220, 1280, 750, 30, 5, 5, 20, 720, 1, 1, 1, 16, 9, 1, 1},
	{HDMI_20_1920x1080i50_16x9, "1920x1080i50hz", "1080i50hz", 0, 28125, 50000, 74250,
		2640, 720, 528, 44, 148, 1920, 1125, 22, 2, 5, 15, 1080, 1, 1, 1, 16, 9, 1, 1},
	{HDMI_21_720x576i50_4x3, "720x576i50hz", "576i50hz_4x3", 0, 15625, 50000, 27000,
		1728, 288, 24, 126, 138, 1440, 625, 24, 2, 3, 19, 576, 1, 0, 0, 4, 3, 16, 15, 1},
	{HDMI_22_720x576i50_16x9, "720x576i50hz", "576i50hz", 0, 15625, 50000, 27000,
		1728, 288, 24, 126, 138, 1440, 625, 24, 2, 3, 19, 576, 1, 0, 0, 16, 9, 64, 65, 1},
	{HDMI_23_720x288p_4x3, "720x288p50hz", NULL, 1, 15625, 49761, 27000,
		1728, 288, 24, 126, 138, 1440, 314, 26, 4, 3, 19, 288, 1, 0, 0, 4, 3, 8, 15},
	{HDMI_24_720x288p_16x9, "720x288p50hz", NULL, 1, 15625, 49761, 27000,
		1728, 288, 24, 126, 138, 1440, 314, 26, 4, 3, 19, 288, 1, 0, 0, 16, 9, 32, 45},
	{HDMI_25_2880x576i50_4x3, "2880x576i50hz", NULL, 0, 15625, 50000, 54000,
		3456, 576, 48, 252, 276, 2880, 625, 24, 2, 3, 19, 576, 1, 0, 0, 4, 3, 2, 15},
	{HDMI_26_2880x576i50_16x9, "2880x576i50hz", NULL, 0, 15625, 50000, 54000,
		3456, 576, 48, 252, 276, 2880, 625, 24, 2, 3, 19, 576, 1, 0, 0, 16, 9, 16, 45},
	{HDMI_27_2880x288p50_4x3, "2880x288p50hz", NULL, 1, 15625, 49761, 54000,
		3456, 576, 48, 252, 276, 2880, 314, 26, 4, 3, 19, 288, 1, 0, 0, 4, 3, 1, 15},
	{HDMI_28_2880x288p50_16x9, "2880x288p50hz", NULL, 1, 15625, 49761, 54000,
		3456, 576, 48, 252, 276, 2880, 314, 26, 4, 3, 19, 288, 1, 0, 0, 16, 9, 8, 45},
	{HDMI_29_1440x576p_4x3, "1440x576p50hz", NULL, 1, 31250, 50000, 54000,
		1728, 288, 24, 128, 136, 1440, 625, 49, 5, 5, 39, 576, 1, 0, 0, 4, 3, 8, 15},
	{HDMI_30_1440x576p_16x9, "1440x576p50hz", NULL, 1, 31250, 50000, 54000,
		1728, 288, 24, 128, 136, 1440, 625, 49, 5, 5, 39, 576, 1, 0, 0, 16, 9, 32, 45},
	{HDMI_31_1920x1080p50_16x9, "1920x1080p50hz", "1080p50hz", 1, 56250, 50000, 148500,
		2640, 720, 528, 44, 148, 1920, 1125, 45, 4, 5, 36, 1080, 1, 1, 1, 16, 9, 1, 1},
	{HDMI_32_1920x1080p24_16x9, "1920x1080p24hz", "1080p24hz", 1, 27000, 24000, 74250,
		2750, 830, 638, 44, 148, 1920, 1125, 45, 4, 5, 36, 1080, 1, 1, 1, 16, 9, 1, 1},
	{HDMI_33_1920x1080p25_16x9, "1920x1080p25hz", "1080p25hz", 1, 28125, 25000, 74250,
		2640, 720, 528, 44, 148, 1920, 1125, 45, 4, 5, 36, 1080, 1, 1, 1, 16, 9, 1, 1},
	{HDMI_34_1920x1080p30_16x9, "1920x1080p30hz", "1080p30hz", 1, 33750, 30000, 74250,
		2200, 280, 88, 44, 148, 1920, 1125, 45, 4, 5, 36, 1080, 1, 1, 1, 16, 9, 1, 1},
	{HDMI_35_2880x480p60_4x3, "2880x480p60hz", NULL, 1, 31469, 59940, 108000,
		3432, 552, 64, 248, 240, 2880, 525, 45, 9, 6, 30, 480, 7, 0, 0, 4, 3, 2, 9},
	{HDMI_36_2880x480p60_16x9, "2880x480p60hz", NULL, 1, 31469, 59940, 108000,
		3432, 552, 64, 248, 240, 2880, 525, 45, 9, 6, 30, 480, 7, 0, 0, 16, 9, 8, 27},
	{HDMI_37_2880x576p50_4x3, "2880x576p50hz", NULL, 1, 31250, 50000, 108000,
		3456, 576, 48, 256, 272, 2880, 625, 49, 5, 5, 39, 576, 1, 0, 0, 4, 3, 4, 15},
	{HDMI_38_2880x576p50_16x9, "2880x576p50hz", NULL, 1, 31250, 50000, 108000,
		3456, 576, 48, 256, 272, 2880, 625, 49, 5, 5, 39, 576, 1, 0, 0, 16, 9, 16, 45},
	{HDMI_39_1920x1080i_t1250_50_16x9, "1920x1080i50hz", NULL, 0, 31250, 50000, 72000,
		2304, 384, 32, 168, 184, 1920, 1250, 85, 23, 5, 57, 1080, 1, 1, 0, 16, 9, 1, 1},
	{HDMI_40_1920x1080i100_16x9, "1920x1080i100hz", NULL, 0, 56250, 100000, 148500,
		2640, 720, 528, 44, 148, 1920, 1125, 22, 2, 5, 15, 1080, 1, 1, 1, 16, 9, 1, 1},
	{HDMI_41_1280x720p100_16x9, "1280x720p100hz", NULL, 1, 75000, 100000, 148500,
		1980, 700, 440, 40, 220, 1280, 750, 30, 5, 5, 20, 720, 1, 1, 1, 16, 9, 1, 1},
	{HDMI_42_720x576p100_4x3, "720x576p100hz", NULL, 1, 62500, 100000, 54000,
		864, 144, 12, 64, 68, 720, 625, 49, 5, 5, 39, 576, 1, 0, 0, 4, 3, 16, 15},
	{HDMI_43_720x576p100_16x9, "720x576p100hz", NULL, 1, 62500, 100000, 54000,
		864, 144, 12, 64, 68, 720, 625, 49, 5, 5, 39, 576, 1, 0, 0, 16, 9, 64, 45},
	{HDMI_44_720x576i100_4x3, "720x576i100hz", NULL, 0, 31250, 100000, 54000,
		1728, 288, 24, 126, 138, 1440, 625, 24, 2, 3, 19, 576, 1, 0, 0, 4, 3, 16, 15},
	{HDMI_45_720x576i100_16x9, "720x576i100hz", NULL, 0, 31250, 100000, 54000,
		1728, 288, 24, 126, 138, 1440, 625, 24, 2, 3, 19, 576, 1, 0, 0, 16, 9, 64, 45},
	{HDMI_46_1920x1080i120_16x9, "1920x1080i120hz", NULL, 0, 67500, 120000, 148500,
		2200, 280, 88, 44, 148, 1920, 1125, 22, 2, 5, 15, 1080, 1, 1, 1, 16, 9, 1, 1},
	{HDMI_47_1280x720p120_16x9, "1280x720p120hz", NULL, 1, 90000, 120000, 148500,
		1650, 370, 110, 40, 220, 1280, 750, 30, 5, 5, 20, 720, 1, 1, 1, 16, 9, 1, 1},
	{HDMI_48_720x480p120_4x3, "720x480p120hz", NULL, 1, 62937, 119880, 54000,
		858, 138, 16, 62, 60, 720, 525, 45, 9, 6, 30, 480, 7, 0, 0, 4, 3, 8, 9},
	{HDMI_49_720x480p120_16x9, "720x480p120hz", NULL, 1, 62937, 119880, 54000,
		858, 138, 16, 62, 60, 720, 525, 45, 9, 6, 30, 480, 7, 0, 0, 16, 9, 32, 27},
	{HDMI_50_720x480i120_4x3, "720x480i120hz", NULL, 0, 31469, 119880, 54000,
		1716, 276, 38, 124, 114, 1440, 525, 22, 4, 3, 15, 480, 4, 0, 0, 4, 3, 8, 9},
	{HDMI_51_720x480i120_16x9, "720x480i120hz", NULL, 0, 31469, 119880, 54000,
		1716, 276, 38, 124, 114, 1440, 525, 22, 4, 3, 15, 480, 4, 0, 0, 16, 9, 32, 27},
	{HDMI_52_720x576p200_4x3, "720x576p200hz", NULL, 1, 125000, 200000, 108000,
		864, 144, 12, 64, 68, 720, 625, 49, 5, 5, 39, 576, 1, 0, 0, 4, 3, 16, 15},
	{HDMI_53_720x576p200_16x9, "720x576p200hz", NULL, 1, 125000, 200000, 108000,
		864, 144, 12, 64, 68, 720, 625, 49, 5, 5, 39, 576, 1, 0, 0, 16, 9, 64, 45},
	{HDMI_54_720x576i200_4x3, "720x576i200hz", NULL, 0, 62500, 200000, 108000,
		1728, 288, 24, 126, 138, 1440, 625, 24, 2, 3, 19, 576, 1, 0, 0, 4, 3, 16, 15},
	{HDMI_55_720x576i200_16x9, "720x576i200hz", NULL, 0, 62500, 200000, 108000,
		1728, 288, 24, 126, 138, 1440, 625, 24, 2, 3, 19, 576, 1, 0, 0, 16, 9, 64, 45},
	{HDMI_56_720x480p240_4x3, "720x480p240hz", NULL, 1, 125874, 239760, 108000,
		858, 138, 16, 62, 60, 720, 525, 45, 9, 6, 30, 480, 7, 0, 0, 4, 3, 8, 9},
	{HDMI_57_720x480p240_16x9, "720x480p240hz", NULL, 1, 125874, 239760, 108000,
		858, 138, 16, 62, 60, 720, 525, 45, 9, 6, 30, 480, 7, 0, 0, 16, 9, 32, 27},
	{HDMI_58_720x480i240_4x3, "720x480i240hz", NULL, 0, 62937, 239760, 108000,
		1716, 276, 38, 124, 114, 1440, 525, 22, 4, 3, 15, 480, 4, 0, 0, 4, 3, 8, 9},
	{HDMI_59_720x480i240_16x9, "720x480i240hz", NULL, 0, 62937, 239760, 108000,
		1716, 276, 38, 124, 114, 1440, 525, 22, 4, 3, 15, 480, 4, 0, 0, 16, 9, 32, 27},
	{HDMI_60_1280x720p24_16x9, "1280x720p24hz", NULL, 1, 18000, 24000, 59400,
		3300, 2020, 1760, 40, 220, 1280, 750, 30, 5, 5, 20, 720, 1, 1, 1, 16, 9, 1, 1},
	{HDMI_61_1280x720p25_16x9, "1280x720p25hz", NULL, 1, 18750, 25000, 74250,
		3960, 2680, 2420, 40, 220, 1280, 750, 30, 5, 5, 20, 720, 1, 1, 1, 16, 9, 1, 1},
	{HDMI_62_1280x720p30_16x9, "1280x720p30hz", NULL, 1, 22500, 30000, 74250,
		3300, 2020, 1760, 40, 220, 1280, 750, 30, 5, 5, 20, 720, 1, 1, 1, 16, 9, 1, 1},
	{HDMI_63_1920x1080p120_16x9, "1920x1080p120hz", "1080p120hz", 1, 135000, 120000, 297000,
		2200, 280, 88, 44, 148, 1920, 1125, 45, 4, 5, 36, 1080, 1, 1, 1, 16, 9, 1, 1},
	{HDMI_64_1920x1080p100_16x9, "1920x1080p100hz", NULL, 1, 112500, 100000, 297000,
		2640, 720, 528, 44, 148, 1920, 1125, 45, 4, 5, 36, 1080, 1, 1, 1, 16, 9, 1, 1},
	{HDMI_65_1280x720p24_64x27, "1280x720p24hz", NULL, 1, 18000, 24000, 59400,
		3300, 2020, 1760, 40, 220, 1280, 750, 30, 5, 5, 20, 720, 1, 1, 1, 64, 27, 4, 3},
	{HDMI_66_1280x720p25_64x27, "1280x720p25hz", NULL, 1, 18750, 25000, 74250,
		3960, 2680, 2420, 40, 220, 1280, 750, 30, 5, 5, 20, 720, 1, 1, 1, 64, 27, 4, 3},
	{HDMI_67_1280x720p30_64x27, "1280x720p30hz", NULL, 1, 22500, 30000, 74250,
		3300, 2020, 1760, 40, 220, 1280, 750, 30, 5, 5, 20, 720, 1, 1, 1, 64, 27, 4, 3},
	{HDMI_68_1280x720p50_64x27, "1280x720p50hz", NULL, 1, 37500, 50000, 74250,
		1980, 700, 440, 40, 220, 1280, 750, 30, 5, 5, 20, 720, 1, 1, 1, 64, 27, 4, 3},
	{HDMI_69_1280x720p60_64x27, "1280x720p60hz", NULL, 1, 45000, 60000, 74250,
		1650, 370, 110, 40, 220, 1280, 750, 30, 5, 5, 20, 720, 1, 1, 1, 64, 27, 4, 3},
	{HDMI_70_1280x720p100_64x27, "1280x720p100hz", NULL, 1, 75000, 100000, 148500,
		1980, 700, 440, 40, 220, 1280, 750, 30, 5, 5, 20, 720, 1, 1, 1, 64, 27, 4, 3},
	{HDMI_71_1280x720p120_64x27, "1280x720p120hz", NULL, 1, 90000, 120000, 148500,
		1650, 370, 110, 40, 220, 1280, 750, 30, 5, 5, 20, 720, 1, 1, 1, 64, 27, 4, 3},
	{HDMI_72_1920x1080p24_64x27, "1920x1080p24hz", NULL, 1, 27000, 24000, 74250,
		2750, 830, 638, 44, 148, 1920, 1125, 45, 4, 5, 36, 1080, 1, 1, 1, 64, 27, 4, 3},
	{HDMI_73_1920x1080p25_64x27, "1920x1080p25hz", NULL, 1, 28125, 25000, 74250,
		2640, 720, 528, 44, 148, 1920, 1125, 45, 4, 5, 36, 1080, 1, 1, 1, 64, 27, 4, 3},
	{HDMI_74_1920x1080p30_64x27, "1920x1080p30hz", NULL, 1, 33750, 30000, 74250,
		2200, 280, 88, 44, 148, 1920, 1125, 45, 4, 5, 36, 1080, 1, 1, 1, 64, 27, 4, 3},
	{HDMI_75_1920x1080p50_64x27, "1920x1080p50hz", NULL, 1, 56250, 50000, 148500,
		2640, 720, 528, 44, 148, 1920, 1125, 45, 4, 5, 36, 1080, 1, 1, 1, 64, 27, 4, 3},
	{HDMI_76_1920x1080p60_64x27, "1920x1080p60hz", NULL, 1, 67500, 60000, 148500,
		2200, 280, 88, 44, 148, 1920, 1125, 45, 4, 5, 36, 1080, 1, 1, 1, 64, 27, 4, 3},
	{HDMI_77_1920x1080p100_64x27, "1920x1080p100hz", NULL, 1, 112500, 100000, 297000,
		2640, 720, 528, 44, 148, 1920, 1125, 45, 4, 5, 36, 1080, 1, 1, 1, 64, 27, 4, 3},
	{HDMI_78_1920x1080p120_64x27, "1920x1080p120hz", NULL, 1, 135000, 120000, 297000,
		2200, 280, 88, 44, 148, 1920, 1125, 45, 4, 5, 36, 1080, 1, 1, 1, 64, 27, 4, 3},
	{HDMI_79_1680x720p24_64x27, "1680x720p24hz", NULL, 1, 18000, 24000, 59400,
		3300, 1620, 1360, 40, 220, 1680, 750, 30, 5, 5, 20, 720, 1, 1, 1, 64, 27, 64, 63},
	{HDMI_80_1680x720p25_64x27, "1680x720p25hz", NULL, 1, 18750, 25000, 59400,
		3168, 1488, 1228, 40, 220, 1680, 750, 30, 5, 5, 20, 720, 1, 1, 1, 64, 27, 64, 63},
	{HDMI_81_1680x720p30_64x27, "1680x720p30hz", NULL, 1, 22500, 30000, 59400,
		2640, 960, 700, 40, 220, 1680, 750, 30, 5, 5, 20, 720, 1, 1, 1, 64, 27, 64, 63},
	{HDMI_82_1680x720p50_64x27, "1680x720p50hz", NULL, 1, 37500, 50000, 82500,
		2200, 520, 260, 40, 220, 1680, 750, 30, 5, 5, 20, 720, 1, 1, 1, 64, 27, 64, 63},
	{HDMI_83_1680x720p60_64x27, "1680x720p60hz", NULL, 1, 45000, 60000, 99000,
		2200, 520, 260, 40, 220, 1680, 750, 30, 5, 5, 20, 720, 1, 1, 1, 64, 27, 64, 63},
	{HDMI_84_1680x720p100_64x27, "1680x720p100hz", NULL, 1, 82500, 100000, 165000,
		2000, 320, 60, 40, 220, 1680, 825, 105, 5, 5, 95, 720, 1, 1, 1, 64, 27, 64, 63},
	{HDMI_85_1680x720p120_64x27, "1680x720p120hz", NULL, 1, 99000, 120000, 198000,
		2000, 320, 60, 40, 220, 1680, 825, 105, 5, 5, 95, 720, 1, 1, 1, 64, 27, 64, 63},
	{HDMI_86_2560x1080p24_64x27, "2560x1080p24hz", NULL, 1, 26400, 24000, 99000,
		3750, 1190, 998, 44, 148, 2560, 1100, 20, 4, 5, 11, 1080, 1, 1, 1, 64, 27, 1, 1},
	{HDMI_87_2560x1080p25_64x27, "2560x1080p25hz", NULL, 1, 28125, 25000, 90000,
		3200, 640, 448, 44, 148, 2560, 1125, 45, 4, 5, 36, 1080, 1, 1, 1, 64, 27, 1, 1},
	{HDMI_88_2560x1080p30_64x27, "2560x1080p30hz", NULL, 1, 33750, 30000, 118800,
		3520, 960, 768, 44, 148, 2560, 1125, 45, 4, 5, 36, 1080, 1, 1, 1, 64, 27, 1, 1},
	{HDMI_89_2560x1080p50_64x27, "2560x1080p50hz", NULL, 1, 56250, 50000, 185625,
		3300, 740, 548, 44, 148, 2560, 1125, 45, 4, 5, 36, 1080, 1, 1, 1, 64, 27, 1, 1},
	{HDMI_90_2560x1080p60_64x27, "2560x1080p60hz", NULL, 1, 66000, 60000, 198000,
		3000, 440, 248, 44, 148, 2560, 1100, 20, 4, 5, 11, 1080, 1, 1, 1, 64, 27, 1, 1},
	{HDMI_91_2560x1080p100_64x27, "2560x1080p100hz", NULL, 1, 125000, 100000, 371250,
		2970, 410, 218, 44, 148, 2560, 1250, 170, 4, 5, 161, 1080, 1, 1, 1, 64, 27, 1, 1},
	{HDMI_92_2560x1080p120_64x27, "2560x1080p120hz", NULL, 1, 150000, 120000, 495000,
		3300, 740, 548, 44, 148, 2560, 1250, 170, 4, 5, 161, 1080, 1, 1, 1, 64, 27, 1, 1},
	{HDMI_93_3840x2160p24_16x9, "3840x2160p24hz", "2160p24hz", 1, 54000, 24000, 297000,
		5500, 1660, 1276, 88, 296, 3840, 2250, 90, 8, 10, 72, 2160, 1, 1, 1, 16, 9, 1, 1},
	{HDMI_94_3840x2160p25_16x9, "3840x2160p25hz", "2160p25hz", 1, 56250, 25000, 297000,
		5280, 1440, 1056, 88, 296, 3840, 2250, 90, 8, 10, 72, 2160, 1, 1, 1, 16, 9, 1, 1},
	{HDMI_95_3840x2160p30_16x9, "3840x2160p30hz", "2160p30hz", 1, 67500, 30000, 297000,
		4400, 560, 176, 88, 296, 3840, 2250, 90, 8, 10, 72, 2160, 1, 1, 1, 16, 9, 1, 1},
	{HDMI_96_3840x2160p50_16x9, "3840x2160p50hz", "2160p50hz", 1, 112500, 50000, 594000,
		5280, 1440, 1056, 88, 296, 3840, 2250, 90, 8, 10, 72, 2160, 1, 1, 1, 16, 9, 1, 1},
	{HDMI_97_3840x2160p60_16x9, "3840x2160p60hz", "2160p60hz", 1, 135000, 60000, 594000,
		4400, 560, 176, 88, 296, 3840, 2250, 90, 8, 10, 72, 2160, 1, 1, 1, 16, 9, 1, 1},
	{HDMI_98_4096x2160p24_256x135, "4096x2160p24hz", "smpte24hz", 1, 54000, 24000, 297000, 5500,
		1404, 1020, 88, 296, 4096, 2250, 90, 8, 10, 72, 2160, 1, 1, 1, 256, 135, 1, 1},
	{HDMI_99_4096x2160p25_256x135, "4096x2160p25hz", "smpte25hz", 1, 56250, 25000, 297000,
		5280, 1184, 968, 88, 128, 4096, 2250, 90, 8, 10, 72, 2160, 1, 1, 1, 256, 135, 1, 1},
	{HDMI_100_4096x2160p30_256x135, "4096x2160p30hz", "smpte30hz", 1, 67500, 30000, 297000,
		4400, 304, 88, 88, 128, 4096, 2250, 90, 8, 10, 72, 2160, 1, 1, 1, 256, 135, 1, 1},
	{HDMI_101_4096x2160p50_256x135, "4096x2160p50hz", "smpte50hz", 1, 112500, 50000, 594000,
		5280, 1184, 968, 88, 128, 4096, 2250, 90, 8, 10, 72, 2160, 1, 1, 1, 256, 135, 1, 1},
	{HDMI_102_4096x2160p60_256x135, "4096x2160p60hz", "smpte60hz", 1, 135000, 60000, 594000,
		4400, 304, 88, 88, 128, 4096, 2250, 90, 8, 10, 72, 2160, 1, 1, 1, 256, 135, 1, 1},
	{HDMI_103_3840x2160p24_64x27, "3840x2160p24hz", NULL, 1, 54000, 24000, 297000,
		5500, 1660, 1276, 88, 296, 3840, 2250, 90, 8, 10, 72, 2160, 1, 1, 1, 64, 27, 4, 3},
	{HDMI_104_3840x2160p25_64x27, "3840x2160p25hz", NULL, 1, 56250, 25000, 297000,
		5280, 1440, 1056, 88, 296, 3840, 2250, 90, 8, 10, 72, 2160, 1, 1, 1, 64, 27, 4, 3},
	{HDMI_105_3840x2160p30_64x27, "3840x2160p30hz", NULL, 1, 67500, 30000, 297000,
		4400, 560, 176, 88, 296, 3840, 2250, 90, 8, 10, 72, 2160, 1, 1, 1, 64, 27, 4, 3},
	{HDMI_106_3840x2160p50_64x27, "3840x2160p50hz", NULL, 1, 112500, 50000, 594000,
		5280, 1440, 1056, 88, 296, 3840, 2250, 90, 8, 10, 72, 2160, 1, 1, 1, 64, 27, 4, 3},
	{HDMI_107_3840x2160p60_64x27, "3840x2160p60hz", NULL, 1, 135000, 60000, 594000,
		4400, 560, 176, 88, 296, 3840, 2250, 90, 8, 10, 72, 2160, 1, 1, 1, 64, 27, 4, 3},
	{HDMI_108_1280x720p48_16x9, "1280x720p48hz", NULL, 1, 36000, 48000, 90000,
		2500, 1220, 960, 40, 220, 1280, 750, 30, 5, 5, 20, 720, 1, 1, 1, 16, 9, 1, 1},
	{HDMI_109_1280x720p48_64x27, "1280x720p48hz", NULL, 1, 36000, 48000, 90000,
		2500, 1220, 960, 40, 220, 1280, 750, 30, 5, 5, 20, 720, 1, 1, 1, 64, 27, 4, 3},
	{HDMI_110_1680x720p48_64x27, "1680x720p48hz", NULL, 1, 36000, 48000, 99000,
		2750, 1070, 810, 40, 220, 1680, 750, 30, 5, 5, 20, 720, 1, 1, 1, 64, 27, 64, 63},
	{HDMI_111_1920x1080p48_16x9, "1920x1080p48hz", NULL, 1, 54000, 48000, 148500,
		2750, 830, 638, 44, 148, 1920, 1125, 45, 4, 5, 36, 1080, 1, 1, 1, 16, 9, 1, 1},
	{HDMI_112_1920x1080p48_64x27, "1920x1080p48hz", NULL, 1, 54000, 48000, 148500,
		2750, 830, 638, 44, 148, 1920, 1125, 45, 4, 5, 36, 1080, 1, 1, 1, 64, 27, 4, 3},
	{HDMI_113_2560x1080p48_64x27, "2560x1080p48hz", NULL, 1, 52800, 48000, 198000,
		3750, 1190, 998, 44, 148, 2560, 1100, 20, 4, 5, 11, 1080, 1, 1, 1, 64, 27, 1, 1},
	{HDMI_114_3840x2160p48_16x9, "3840x2160p48hz", NULL, 1, 108000, 48000, 594000,
		5500, 1660, 1276, 88, 296, 3840, 2250, 90, 8, 10, 72, 2160, 1, 1, 1, 16, 9, 1, 1},
	{HDMI_115_4096x2160p48_256x135, "4096x2160p48hz", "smpte48hz",
		1, 108000, 48000, 594000, 5500,
		1404, 1020, 88, 296, 4096, 2250, 90, 8, 10, 72, 2160, 1, 1, 1, 256, 135, 1, 1},
	{HDMI_116_3840x2160p48_64x27, "3840x2160p48hz", NULL, 1, 108000, 48000, 594000,
		5500, 1660, 1276, 88, 296, 3840, 2250, 90, 8, 10, 72, 2160, 1, 1, 1, 64, 27, 4, 3},
	{HDMI_117_3840x2160p100_16x9, "3840x2160p100hz", NULL, 1, 225000, 100000, 1188000,
		5280, 1440, 1056, 88, 296, 3840, 2250, 90, 8, 10, 72, 2160, 1, 1, 1, 16, 9, 1, 1},
	{HDMI_118_3840x2160p120_16x9, "3840x2160p120hz", NULL, 1, 270000, 120000, 1188000,
		4400, 560, 176, 88, 296, 3840, 2250, 90, 8, 10, 72, 2160, 1, 1, 1, 16, 9, 1, 1},
	{HDMI_119_3840x2160p100_64x27, "3840x2160p100hz", NULL, 1, 225000, 100000, 1188000,
		5280, 1440, 1056, 88, 296, 3840, 2250, 90, 8, 10, 72, 2160, 1, 1, 1, 64, 27, 4, 3},
	{HDMI_120_3840x2160p120_64x27, "3840x2160p120hz", NULL, 1, 270000, 120000, 1188000,
		4400, 560, 176, 88, 296, 3840, 2250, 90, 8, 10, 72, 2160, 1, 1, 1, 64, 27, 4, 3},
	{HDMI_121_5120x2160p24_64x27, "5120x2160p24hz", NULL, 1, 52800, 24000, 396000,
		7500, 2380, 1996, 88, 296, 5120, 2200, 40, 8, 10, 22, 2160, 1, 1, 1, 64, 27, 1, 1},
	{HDMI_122_5120x2160p25_64x27, "5120x2160p25hz", NULL, 1, 55000, 25000, 396000,
		7200, 2080, 1696, 88, 296, 5120, 2200, 40, 8, 10, 22, 2160, 1, 1, 1, 64, 27, 1, 1},
	{HDMI_123_5120x2160p30_64x27, "5120x2160p30hz", NULL, 1, 66000, 30000, 396000,
		6000, 880, 664, 88, 128, 5120, 2200, 40, 8, 10, 22, 2160, 1, 1, 1, 64, 27, 1, 1},
	{HDMI_124_5120x2160p48_64x27, "5120x2160p48hz", NULL, 1, 118800, 48000, 742500,
		6250, 1130, 746, 88, 296, 5120, 2475, 315, 8, 10, 297, 2160, 1, 1, 1, 64, 27, 1, 1},
	{HDMI_125_5120x2160p50_64x27, "5120x2160p50hz", NULL, 1, 112500, 50000, 742500,
		6600, 1480, 1096, 88, 296, 5120, 2250, 90, 8, 10, 72, 2160, 1, 1, 1, 64, 27, 1, 1},
	{HDMI_126_5120x2160p60_64x27, "5120x2160p60hz", NULL, 1, 135000, 60000, 742500,
		5500, 380, 164, 88, 128, 5120, 2250, 90, 8, 10, 72, 2160, 1, 1, 1, 64, 27, 1, 1},
	{HDMI_127_5120x2160p100_64x27, "5120x2160p100hz", NULL, 1, 225000, 100000, 1485000,
		6600, 1480, 1096, 88, 296, 5120, 2250, 90, 8, 10, 72, 2160, 1, 1, 1, 64, 27, 1, 1},
};

static const struct hdmi_timing edid_cea_modes_193[] = {
	{HDMI_193_5120x2160p120_64x27, "5120x2160p120hz", NULL, 1, 270000, 120000, 1485000,
		5500, 380, 164, 88, 128, 5120, 2250, 90, 8, 10, 72, 2160, 1, 1, 1, 64, 27, 1, 1},
	{HDMI_194_7680x4320p24_16x9, "7680x4320p24hz", NULL, 1, 108000, 24000, 1188000, 11000,
		3320, 2552, 176, 592, 7680, 4500, 180, 16, 20, 144, 4320, 1, 1, 1, 16, 9, 1, 1},
	{HDMI_195_7680x4320p25_16x9, "7680x4320p25hz", NULL, 1, 110000, 25000, 1188000, 10800,
		3120, 2352, 176, 592, 7680, 4400, 80, 16, 20, 44, 4320, 1, 1, 1, 16, 9, 1, 1},
	{HDMI_196_7680x4320p30_16x9, "7680x4320p30hz", NULL, 1, 132000, 30000, 1188000,
		9000, 1320, 552, 176, 592, 7680, 4400, 80, 16, 20, 44, 4320, 1, 1, 1, 16, 9, 1, 1},
	{HDMI_197_7680x4320p48_16x9, "7680x4320p48hz", NULL, 1, 216000, 48000, 2376000, 11000,
		3320, 2552, 176, 592, 7680, 4500, 180, 16, 20, 144, 4320, 1, 1, 1, 16, 9, 1, 1},
	{HDMI_198_7680x4320p50_16x9, "7680x4320p50hz", NULL, 1, 220000, 50000,
		2376000, 10800, 3120, 2352, 176, 592, 7680, 4400, 80, 16, 20, 44, 4320,
		1, 1, 1, 16, 9, 1, 1},
	{HDMI_199_7680x4320p60_16x9, "7680x4320p60hz", NULL, 1, 264000, 60000, 2376000,
		9000, 1320, 552, 176, 592, 7680, 4400, 80, 16, 20, 44, 4320, 1, 1, 1, 16, 9, 1, 1},
	{HDMI_200_7680x4320p100_16x9, "7680x4320p100hz", NULL, 1, 450000, 100000, 4752000, 10560,
		2880, 2112, 176, 592, 7680, 4500, 180, 16, 20, 144, 4320, 1, 1, 1, 16, 9, 1, 1},
	{HDMI_201_7680x4320p120_16x9, "7680x4320p120hz", NULL, 1, 540000, 120000, 4752000, 8800,
		1120, 352, 176, 592, 7680, 4500, 180, 16, 20, 144, 4320, 1, 1, 1, 16, 9, 1, 1},
	{HDMI_202_7680x4320p24_64x27, "7680x4320p24hz", NULL, 1, 108000, 24000, 1188000, 11000,
		3320, 2552, 176, 592, 7680, 4500, 180, 16, 20, 144, 4320, 1, 1, 1, 64, 27, 4, 3},
	{HDMI_203_7680x4320p25_64x27, "7680x4320p25hz", NULL, 1, 110000, 25000, 1188000, 10800,
		3120, 2352, 176, 592, 7680, 4400, 80, 16, 20, 44, 4320, 1, 1, 1, 64, 27, 4, 3},
	{HDMI_204_7680x4320p30_64x27, "7680x4320p30hz", NULL, 1, 132000, 30000, 1188000,
		9000, 1320, 552, 176, 592, 7680, 4400, 80, 16, 20, 44, 4320, 1, 1, 1, 64, 27, 4, 3},
	{HDMI_205_7680x4320p48_64x27, "7680x4320p48hz", NULL, 1, 216000, 48000, 2376000, 11000,
		3320, 2552, 176, 592, 7680, 4500, 180, 16, 20, 144, 4320, 1, 1, 1, 64, 27, 4, 3},
	{HDMI_206_7680x4320p50_64x27, "7680x4320p50hz", NULL, 1, 220000, 50000, 2376000, 10800,
		3120, 2352, 176, 592, 7680, 4400, 80, 16, 20, 44, 4320, 1, 1, 1, 64, 27, 4, 3},
	{HDMI_207_7680x4320p60_64x27, "7680x4320p60hz", NULL, 1, 264000, 60000, 2376000,
		9000, 1320, 552, 176, 592, 7680, 4400, 80, 16, 20, 44, 4320, 1, 1, 1, 64, 27, 4, 3},
	{HDMI_208_7680x4320p100_64x27, "7680x4320p100hz", NULL, 1, 450000, 100000, 4752000, 10560,
		2880, 2112, 176, 592, 7680, 4500, 180, 16, 20, 144, 4320, 1, 1, 1, 64, 27, 4, 3},
	{HDMI_209_7680x4320p120_64x27, "7680x4320p120hz", NULL, 1, 540000, 120000, 4752000, 8800,
		1120, 352, 176, 592, 7680, 4500, 180, 16, 20, 144, 4320, 1, 1, 1, 64, 27, 4, 3},
	{HDMI_210_10240x4320p24_64x27, "10240x4320p24hz", NULL, 1, 118800, 24000, 1485000, 12500,
		2260, 1492, 176, 592, 10240, 4950, 630, 16, 20, 594, 4320, 1, 1, 1, 64, 27, 1, 1},
	{HDMI_211_10240x4320p25_64x27, "10240x4320p25hz", NULL, 1, 110000, 25000, 1485000, 13500,
		3260, 2492, 176, 592, 10240, 4400, 80, 16, 20, 44, 4320, 1, 1, 1, 64, 27, 1, 1},
	{HDMI_212_10240x4320p30_64x27, "10240x4320p30hz", NULL, 1, 135000, 30000, 1485000, 11000,
		760, 288, 176, 296, 10240, 4500, 180, 16, 20, 144, 4320, 1, 1, 1, 64, 27, 1, 1},
	{HDMI_213_10240x4320p48_64x27, "10240x4320p48hz", NULL, 1, 237600, 48000, 2970000, 12500,
		2260, 1492, 176, 592, 10240, 4950, 630, 16, 20, 594, 4320, 1, 1, 1, 64, 27, 1, 1},
	{HDMI_214_10240x4320p50_64x27, "10240x4320p50hz", NULL, 1, 220000, 50000, 2970000, 13500,
		3260, 2492, 176, 592, 10240, 4400, 80, 16, 20, 44, 4320, 1, 1, 1, 64, 27, 1, 1},
	{HDMI_215_10240x4320p60_64x27, "10240x4320p60hz", NULL, 1, 270000, 60000, 2970000, 11000,
		760, 288, 176, 296, 10240, 4500, 180, 16, 20, 144, 4320, 1, 1, 1, 64, 27, 1, 1},
	{HDMI_216_10240x4320p100_64x27, "10240x4320p100hz", NULL, 1, 450000, 100000, 5940000, 13200,
		2960, 2192, 176, 592, 10240, 4500, 180, 16, 20, 144, 4320, 1, 1, 1, 64, 27, 1, 1},
	{HDMI_217_10240x4320p120_64x27, "10240x4320p120hz", NULL, 1, 540000, 120000, 5940000, 11000,
		760, 288, 176, 296, 10240, 4500, 180, 16, 20, 144, 4320, 1, 1, 1, 64, 27, 1, 1},
	{HDMI_218_4096x2160p100_256x135, "4096x2160p100hz", "smpte100hz",
		1, 225000, 100000, 1188000,
		5280, 1184, 800, 88, 296, 4096, 2250, 90, 8, 10, 72, 2160, 1, 1, 1, 256, 135, 1, 1},
	{HDMI_219_4096x2160p120_256x135, "4096x2160p120hz", "smpte120hz",
		1, 270000, 120000, 1188000,
		4400, 304, 88, 88, 128, 4096, 2250, 90, 8, 10, 72, 2160, 1, 1, 1, 256, 135, 1, 1},
};

/*VESA Timing, from legacy tx20/hdmi_parameters;
 *All names end with "V" to identify from cea modes.
 *!!! PLEASE UPDATE VESA_TIMING_END also when add new timing in vesa_modes.
 */
static const struct hdmi_timing vesa_modes[] = {
	{HDMIV_0_640x480p60hz, "640x480p60hz", NULL, 1, 26218, 59940, 25175,
		800, 160, 16, 96, 48, 640, 525, 45, 10, 2, 33, 480, 1, 0, 0, 4, 3, 1, 1},
	{HDMIV_1_800x480p60hz, "800x480p60hz", NULL, 1, 30000, 60000, 29230,
		928, 128, 40, 48, 40, 800, 525, 45, 13, 3, 29, 480, 1, 0, 0, 5, 3, 1, 1},
	{HDMIV_2_800x600p60hz, "800x600p60hz", NULL, 1, 37879, 60317, 40000,
		1056, 256, 40, 128, 88, 800, 628, 28, 1, 4, 23, 600, 1, 1, 1, 4, 3},
	{HDMIV_3_852x480p60hz, "852x480p60hz", NULL, 1, 31900, 59960, 30240,
		948, 96, 40, 16, 40, 852, 532, 52, 10, 2, 40, 480, 1, 1, 1, 213, 120},
	{HDMIV_4_854x480p60hz, "854x480p60hz", NULL, 1, 31830, 59950, 30240,
		950, 96, 40, 16, 40, 854, 531, 51, 10, 2, 39, 480, 1, 1, 1, 427, 240},
	{HDMIV_5_1024x600p60hz, "1024x600p60hz", NULL, 1, 37320, 60000, 44580,
		1152, 128, 40, 48, 40, 1024, 645, 45, 13, 3, 29, 600, 1, 1, 1, 17, 10, 1, 1},
	{HDMIV_6_1024x768p60hz, "1024x768p60hz", NULL, 1, 48360, 60004, 79500,
		1344, 320, 24, 136, 160, 1024, 806, 38, 3, 6, 29, 768, 1, 1, 1, 4, 3},
	{HDMIV_7_1152x864p75hz, "1152x864p75hz", NULL, 1, 67500, 75000, 108000,
		1600, 448, 64, 128, 256, 1152, 900, 36, 1, 3, 32, 864, 0, 1, 1, 4, 3},
	{HDMIV_8_1280x768p60hz, "1280x768p60hz", NULL, 1, 47776, 59870, 79500,
		1664, 384, 64, 128, 192, 1280, 798, 30, 3, 7, 20, 768, 0, 1, 1, 5, 3},
	{HDMIV_9_1280x800p60hz, "1280x800p60hz", NULL, 1, 49702, 59810, 83500,
		1680, 400, 72, 128, 200, 1280, 831, 31, 3, 6, 22, 800, 1, 0, 1, 8, 5},
	{HDMIV_10_1280x960p60hz, "1280x960p60hz", NULL, 1, 60000, 60000, 108000,
		1800, 520, 96, 112, 312, 1280, 1000, 40, 1, 3, 36, 960, 0, 1, 1, 4, 3},
	{HDMIV_11_1280x1024p60hz, "1280x1024p60hz", NULL, 1, 63981, 60020, 108000,
		1688, 408, 48, 112, 248, 1280, 1066, 42, 1, 3, 38, 1024, 1, 1, 1, 5, 4},
	{HDMIV_12_1360x768p60hz, "1360x768p60hz", NULL, 1, 47700, 60015, 85500,
		1792, 432, 64, 112, 256, 1360, 795, 27, 3, 6, 18, 768, 1, 1, 1, 16, 9},
	{HDMIV_13_1366x768p60hz, "1366x768p60hz", NULL, 1, 47880, 59790, 85500,
		1792, 426, 70, 143, 213, 1366, 798, 30, 3, 3, 24, 768, 1, 1, 1, 16, 9},
	{HDMIV_14_1400x1050p60hz, "1400x1050p60hz", NULL, 1, 65317, 59978, 121750,
		1862, 464, 88, 144, 232, 1400, 1089, 39, 3, 4, 32, 1050, 1, 1, 1, 4, 3},
	{HDMIV_15_1440x900p60hz, "1440x900p60hz", NULL, 1, 56040, 59887, 106500,
		1904, 464, 80, 152, 232, 1440, 934, 34, 3, 6, 25, 900, 1, 1, 1, 8, 5},
	{HDMIV_16_1440x2560p60hz, "1440x2560p60hz", NULL, 1, 155760, 59999, 244850,
		1572, 132, 64, 4, 64, 1440, 2596, 36, 16, 4, 16, 2560, 1, 1, 1, 16, 9},
	{HDMIV_17_1600x900p60hz, "1600x900p60hz", NULL, 1, 60000, 60000, 108000,
		1800, 200, 24, 80, 96, 1600, 1000, 100, 1, 3, 96, 900, 1, 1, 1, 16, 9},
	{HDMIV_18_1600x1200p60hz, "1600x1200p60hz", NULL, 1, 75000, 60000, 162000,
		2160, 560, 64, 192, 304, 1600, 1250, 50, 1, 3, 46, 1200, 1, 1, 1, 4, 3},
	{HDMIV_19_1680x1050p60hz, "1680x1050p60hz", NULL, 1, 65290, 59954, 146250,
		2240, 560, 104, 176, 280, 1680, 1089, 39, 3, 6, 30, 1050, 1, 1, 1, 8, 5},
	{HDMIV_20_1920x1200p60hz, "1920x1200p60hz", NULL, 1, 74700, 59885, 193250,
		2592, 672, 136, 200, 336, 1920, 1245, 45, 3, 6, 36, 1200, 1, 1, 1, 8, 5},
	{HDMIV_21_2048x1080p24hz, "2048x1080p24hz", NULL, 1, 27000, 24000, 74250,
		2750, 702, 148, 44, 510, 2048, 1125, 45, 4, 5, 36, 1080, 1, 1, 1, 256, 135},
	{HDMIV_22_2160x1200p90hz, "2160x1200p90hz", NULL, 1, 109080, 90000, 268550,
		2462, 302, 190, 32, 80, 2160, 1212, 12, 6, 3, 3, 1200, 1, 1, 1, 9, 5},
	{HDMIV_23_2560x1600p60hz, "2560x1600p60hz", NULL, 1, 99458, 59987, 348500,
		3504, 944, 192, 280, 472, 2560, 1658, 58, 3, 6, 49, 1600, 1, 1, 1, 8, 5},
	{HDMIV_24_3440x1440p60hz, "3440x1440p60hz", NULL, 1, 88858, 59999, 319890,
		3600, 160, 48, 32, 80, 3440, 1481, 41, 3, 5, 33, 1440, 1, 1, 1, 43, 18, 1, 1},
	{HDMIV_25_2400x1200p90hz, "2400x1200p90hz", NULL, 1, 112812, 90106, 280000,
		2482, 82, 20, 30, 32, 2400, 1252, 52, 17, 5, 30, 1200, 1, 1, 1, 2, 1},
	{HDMIV_26_3840x1080p60hz, "3840x1080p60hz", NULL, 1, 67500, 60000, 297000,
		4400, 560, 176, 88, 296, 3840, 1125, 45, 4, 5, 36, 1080, 1, 1, 1, 32, 6},
	{HDMIV_27_2560x1440p60hz, "2560x1440p60hz", NULL, 1, 88787, 59950, 241500,
		2720, 160, 48, 32, 80, 2560, 1481, 41, 3, 5, 33, 1440, 1, 1, 1, 16, 9, 1, 1},
};

#define VESA_TIMING_END HDMIV_27_2560x1440p60hz

/*return NULL for invalid hdmi_timing.*/
const struct hdmi_timing *hdmitx_mode_index_to_hdmi_timing(u32 idx)
{
	u32 i = 0;
	struct {
		int size;
		const struct hdmi_timing *start;
	} all_timing_list[] = {
		{ARRAY_SIZE(edid_cea_modes_0), edid_cea_modes_0},
		{ARRAY_SIZE(edid_cea_modes_193), edid_cea_modes_193},
		{ARRAY_SIZE(vesa_modes), vesa_modes},
	};

	for (i = 0; i < ARRAY_SIZE(all_timing_list); i++) {
		if (idx >= all_timing_list[i].size)
			idx -= all_timing_list[i].size;
		else
			return &all_timing_list[i].start[idx];
	}
	return NULL;
}

/*always return valid struct hdmi_timing, for invalid vic, return timing with vic=1.*/
const struct hdmi_timing *hdmitx_mode_vic_to_hdmi_timing(enum hdmi_vic vic)
{
	int i, offset;
	const struct hdmi_timing *timing = NULL;
	struct {
		enum hdmi_vic start;
		enum hdmi_vic end;
		const struct hdmi_timing *timing_start;
	} all_timing_list[] = {
		{HDMI_0_UNKNOWN, HDMI_127_5120x2160p100_64x27, edid_cea_modes_0},
		{HDMI_193_5120x2160p120_64x27, HDMI_219_4096x2160p120_256x135, edid_cea_modes_193},
		{HDMITX_VESA_OFFSET, VESA_TIMING_END, vesa_modes},
	};

	for (i = 0; i < ARRAY_SIZE(all_timing_list); i++) {
		if (vic >= all_timing_list[i].start && vic <= all_timing_list[i].end) {
			/*double confirm*/
			offset = vic - all_timing_list[i].start;
			timing = &all_timing_list[i].timing_start[offset];
			if (timing->vic != vic)
				HDMITX_ERROR("vic %d, offset %d in table %d not Match!\n",
						vic, offset, i);

			return timing;
		}
	}

	HDMITX_ERROR("unknown vic [%d] in timing table\n", vic);
	return NULL;
}
EXPORT_SYMBOL(hdmitx_mode_vic_to_hdmi_timing);

const struct hdmi_timing *hdmitx_mode_match_dtd_timing(struct dtd *t)
{
	u32 i = 0;
	const struct hdmi_timing *timing;

	if (!t)
		return INVALID_HDMI_TIMING;

	/* interlace mode, all vertical timing parameters
	 * are halved, while vactive/vtotal is doubled
	 * in timing table. need double vactive before compare
	 */
	if (t->flags >> 7 == 0x1)
		t->v_active = t->v_active * 2;

	while (1) {
		timing = hdmitx_mode_index_to_hdmi_timing(i);
		if (!timing)
			break;

		if ((abs(timing->pixel_freq / 10 - t->pixel_clock) <=
			(t->pixel_clock + 1000) / 1000) &&
		    t->h_active == timing->h_active &&
		    t->h_blank == timing->h_blank &&
		    t->v_active == timing->v_active &&
		    t->v_blank == timing->v_blank &&
		    t->h_sync_offset == timing->h_front &&
		    t->h_sync == timing->h_sync &&
		    t->v_sync_offset == timing->v_front &&
		    t->v_sync == timing->v_sync)
			return timing;

		i++;
	}

	return INVALID_HDMI_TIMING;
}

const struct hdmi_timing *hdmitx_mode_match_vesa_timing(struct vesa_standard_timing *t)
{
	int i;
	const struct hdmi_timing *timing;

	if (!t)
		return INVALID_HDMI_TIMING;

	for (i = 0; i < ARRAY_SIZE(vesa_modes); i++) {
		timing = &vesa_modes[i];

		if (t->hactive == timing->h_active &&
		    t->vactive == timing->v_active) {
			if (t->vsync) {
				unsigned int vsync = hdmi_timing_vrefresh(timing);

				if (t->vsync == vsync)
					return timing;
			}
			if (t->hblank &&
			    t->hblank == timing->h_blank &&
			    t->vblank &&
			    t->vblank == timing->v_blank &&
			    t->tmds_clk &&
			    t->tmds_clk == timing->pixel_freq / 10)
				return timing;
		}
	}

	return INVALID_HDMI_TIMING;
}

/*!!!!Internal use!!!!
 *Return timing in mode list, but may not support by driver.
 *mostly for edid parser usage.
 *Call hdmitx_dev_get_valid_vic() to get vic have been validated.
 */
const struct hdmi_timing *hdmitx_mode_match_timing_name(const char *name)
{
	u32 i = 0;
	const struct hdmi_timing *timing;

	if (!name)
		return INVALID_HDMI_TIMING;

	while (1) {
		timing = hdmitx_mode_index_to_hdmi_timing(i);
		if (!timing)
			break;

		/* check sname first */
		if (timing->sname && !strncmp(timing->sname, name, strlen(timing->sname)))
			return timing;

		/* check full name first */
		if (!strncmp(timing->name, name, strlen(timing->name)))
			return timing;

		i++;
	}

	return INVALID_HDMI_TIMING;
}
EXPORT_SYMBOL(hdmitx_mode_match_timing_name);

bool hdmitx_mode_validate_y420_vic(enum hdmi_vic vic)
{
	const struct hdmi_timing *timing;

	/* In Spec2.1 Table 7-34, greater than 2160p30hz will support y420 */
	timing = hdmitx_mode_vic_to_hdmi_timing(vic);
	if (!timing)
		return false;
	if (timing->v_active >= 2160 && timing->v_freq > 30000)
		return true;
	if (timing->v_active >= 4320)
		return true;
	return false;
}
EXPORT_SYMBOL(hdmitx_mode_validate_y420_vic);

const char *hdmitx_mode_get_timing_name(enum hdmi_vic vic)
{
	const struct hdmi_timing *timing =
		hdmitx_mode_vic_to_hdmi_timing(vic);
	const char *modename;

	if (!timing)
		timing = INVALID_HDMI_TIMING;

	modename = timing->sname ? timing->sname : timing->name;

	return modename;
}

/**
 * sync function drm_mode_vrefresh()
 */
int hdmi_timing_vrefresh(const struct hdmi_timing *t)
{
	unsigned int num, den;

	if (t->h_total == 0 || t->v_total == 0)
		return 0;

	num = t->pixel_freq;
	den = t->h_total * t->v_total;

	/*interlace mode*/
	if (t->pi_mode == 0)
		num *= 2;

	return DIV_ROUND_CLOSEST_ULL(mul_u32_u32(num, 1000), den);
}

bool hdmitx_mode_have_alternate_clock(const struct hdmi_timing *t)
{
	/*to be confirm if VESA can support frac rate.*/
	if (t->vic == HDMI_0_UNKNOWN || t->vic >= HDMI_CEA_VIC_END)
		return false;

	if (hdmi_timing_vrefresh(t) % 6 != 0)
		return false;

	return true;
}

int hdmitx_mode_update_timing(struct hdmi_timing *t,
	bool to_frac_mode)
{
	unsigned int alternate_clock = 0;
	bool frac_timing = t->v_freq % 1000 == 0 ? false : true;

	if (!hdmitx_mode_have_alternate_clock(t))
		return -EINVAL;

	if (!frac_timing && to_frac_mode)
		alternate_clock = DIV_ROUND_CLOSEST_ULL(mul_u32_u32(t->pixel_freq, 1000), 1001);
	else if (frac_timing && !to_frac_mode)
		alternate_clock = DIV_ROUND_CLOSEST_ULL(mul_u32_u32(t->pixel_freq, 1001), 1000);

	if (alternate_clock) {
		t->pixel_freq = alternate_clock;
		/*update vsync/hsync*/
		t->v_freq = DIV_ROUND_CLOSEST_ULL(mul_u32_u32(t->pixel_freq, 1000000),
						mul_u32_u32(t->h_total, t->v_total));
		t->h_freq = DIV_ROUND_CLOSEST_ULL(mul_u32_u32(t->pixel_freq, 1000), t->h_total);

		/*HDMITX_INFO("Timing %s update frac_mode(%d):\n", t->name, to_frac_mode);
		 *HDMITX_INFO("\tPixel_freq(%d), h_freq (%d), v_freq(%d).\n",
		 *	t->pixel_freq, t->h_freq, t->v_freq);
		 */
	}

	return alternate_clock;
}

void hdmitx_mode_print_hdmi_timing(const struct hdmi_timing *t)
{
	struct hdmi_timing alternate_t;

	pr_warn("VIC Hactive Vactive I/P Htotal Hblank Vtotal Vblank Hfreq Vfreq Pfreq\n");
	pr_warn("%s[%d] %d %d %c %d %d %d %d %d %d %d\n",
		t->name, t->vic,
		t->h_active, t->v_active,
		(t->pi_mode) ? 'P' : 'I',
		t->h_total, t->h_blank, t->v_total, t->v_blank,
		t->h_freq, t->v_freq, t->pixel_freq);

	if (hdmitx_mode_have_alternate_clock(t)) {
		alternate_t = *t;
		if (hdmitx_mode_update_timing(&alternate_t, 0) > 0 ||
			hdmitx_mode_update_timing(&alternate_t, 1) > 0) {
			pr_warn("Fraction mode:\n");
			pr_warn("%d %d %d\n",
				t->h_freq, t->v_freq, t->pixel_freq);
		}
	}

	pr_warn("\nVIC Hfront Hsync Hback Hpol Vfront Vsync Vback Vpol Ln\n");
	pr_warn("%s[%d] %d %d %d %c %d %d %d %c %d\n",
		t->name, t->vic,
		t->h_front, t->h_sync, t->h_back,
		(t->h_pol) ? 'P' : 'N',
		t->v_front, t->v_sync, t->v_back,
		(t->v_pol) ? 'P' : 'N',
		t->v_sync_ln);

	pr_warn("\nCheck Horizon parameter\n");
	if (t->h_total != (t->h_active + t->h_blank))
		pr_warn("VIC[%d] Ht[%d] != (Ha[%d] + Hb[%d])\n",
			t->vic, t->h_total,
			t->h_active, t->h_blank);
	if (t->h_blank != (t->h_front + t->h_sync + t->h_back))
		pr_warn("VIC[%d] Hb[%d] != (Hf[%d] + Hs[%d] + Hb[%d])\n",
			t->vic, t->h_blank,
			t->h_front, t->h_sync, t->h_back);

	pr_warn("\nCheck Vertical parameter\n");
	if (t->v_total != (t->v_active + t->v_blank))
		pr_warn("VIC[%d] Vt[%d] != (Va[%d] + Vb[%d]\n",
			t->vic, t->v_total, t->v_active,
			t->v_blank);

	if ((t->v_blank != (t->v_front + t->v_sync + t->v_back)) &&
	    t->pi_mode == 1)
		pr_warn("VIC[%d] Vb[%d] != (Vf[%d] + Vs[%d] + Vb[%d])\n",
			t->vic, t->v_blank,
			t->v_front, t->v_sync, t->v_back);

	if ((t->v_blank / 2 != (t->v_front + t->v_sync + t->v_back)) &&
	    t->pi_mode == 0)
		pr_warn("VIC[%d] Vb[%d] != (Vf[%d] + Vs[%d] + Vb[%d])\n",
			t->vic, t->v_blank, t->v_front,
			t->v_sync, t->v_back);
}

void hdmitx_mode_print_all_mode_table(void)
{
	u32 idx = 0;
	const struct hdmi_timing *timing = NULL;

	HDMITX_INFO("-----------------%s start --------------\n", __func__);

	do {
		timing = hdmitx_mode_index_to_hdmi_timing(idx);
		if (!timing)
			break;

		hdmitx_mode_print_hdmi_timing(timing);
	} while (idx++);

	HDMITX_INFO("-----------------%s end --------------\n", __func__);
}

