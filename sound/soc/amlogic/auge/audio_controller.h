/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __AML_AUDIO_CONTROLLER_H_
#define __AML_AUDIO_CONTROLLER_H_

struct aml_audio_controller;
int aml_return_chip_id(void);
#define CLK_NOTIFY_CHIP_ID    (0x41)
#define CLK_NOTIFY_CHIP_ID_T3X    (0x42)

#define MPLL_HBR_FIXED_FREQ   (491520000)
#define MPLL_CD_FIXED_FREQ    (451584000)
#define THRESHOLD_HIFI0 (491520000 / 1000)
#define THRESHOLD_HIFI1 (451584000 / 1000)
#endif
