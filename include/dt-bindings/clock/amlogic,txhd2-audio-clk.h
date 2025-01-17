/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __T5_AUDIO_CLK_H__
#define __T5_AUDIO_CLK_H__

/*
 * CLKID audio index values
 */

#define CLKID_AUDIO_GATE_DDR_ARB                0
#define CLKID_AUDIO_GATE_PDM                    1
#define CLKID_AUDIO_GATE_TDMINA                 2
#define CLKID_AUDIO_GATE_TDMINB                 3
#define CLKID_AUDIO_GATE_TDMINC                 4
#define CLKID_AUDIO_GATE_TDMINLB                5
#define CLKID_AUDIO_GATE_TDMOUTA                6
#define CLKID_AUDIO_GATE_TDMOUTB                7
#define CLKID_AUDIO_GATE_TDMOUTC                8
#define CLKID_AUDIO_GATE_FRDDRA                 9
#define CLKID_AUDIO_GATE_FRDDRB                 10
#define CLKID_AUDIO_GATE_FRDDRC                 11
#define CLKID_AUDIO_GATE_TODDRA                 12
#define CLKID_AUDIO_GATE_TODDRB                 13
#define CLKID_AUDIO_GATE_TODDRC                 14
#define CLKID_AUDIO_GATE_LOOPBACKA              15
#define CLKID_AUDIO_GATE_SPDIFIN                16
#define CLKID_AUDIO_GATE_SPDIFOUT_A             17
#define CLKID_AUDIO_GATE_RESAMPLEA              18
#define CLKID_AUDIO_GATE_RESERVED0              19
#define CLKID_AUDIO_GATE_RESERVED1              20
#define CLKID_AUDIO_GATE_SPDIFOUT_B             21
#define CLKID_AUDIO_GATE_EQDRC                  22
#define CLKID_AUDIO_GATE_RESERVED2              23
#define CLKID_AUDIO_GATE_RESERVED3              24
#define CLKID_AUDIO_GATE_RESERVED4              25
#define CLKID_AUDIO_GATE_RESAMPLEB              26
#define CLKID_AUDIO_GATE_TOVAD                  27
#define CLKID_AUDIO_GATE_AUDIOLOCKER            28
#define CLKID_AUDIO_GATE_SPDIFIN_LB             29
#define CLKID_AUDIO_GATE_FRATV                  30
#define CLKID_AUDIO_GATE_FRHDMIRX               31

/* Gate En1 */
#define CLKID_AUDIO_GATE_FRDDRD                 32
#define CLKID_AUDIO_GATE_TODDRD                 33
#define CLKID_AUDIO_GATE_LOOPBACKB              34
#define CLKID_AUDIO_GATE_FRDDRE                 35
#define CLKID_AUDIO_GATE_TODDRE                 36
#define CLKID_AUDIO_GATE_EARCTX                 37
#define CLKID_AUDIO_GATE_EARCRX                 38
#define CLKID_AUDIO_GATE_RESAMPLEB_OLD          39
#define CLKID_AUDIO_GATE_LOCKER                 40

#define CLKID_AUDIO_GATE_MAX                    41

#define MCLK_BASE                               CLKID_AUDIO_GATE_MAX
#define CLKID_AUDIO_MCLK_A                      (MCLK_BASE + 0)
#define CLKID_AUDIO_MCLK_B                      (MCLK_BASE + 1)
#define CLKID_AUDIO_MCLK_C                      (MCLK_BASE + 2)
#define CLKID_AUDIO_MCLK_D                      (MCLK_BASE + 3)
#define CLKID_AUDIO_MCLK_E                      (MCLK_BASE + 4)
#define CLKID_AUDIO_MCLK_F                      (MCLK_BASE + 5)

#define CLKID_AUDIO_SPDIFIN                     (MCLK_BASE + 6)
#define CLKID_AUDIO_SPDIFOUT_A                  (MCLK_BASE + 7)
#define CLKID_AUDIO_RESAMPLE_A                  (MCLK_BASE + 8)
#define CLKID_AUDIO_LOCKER_OUT                  (MCLK_BASE + 9)
#define CLKID_AUDIO_LOCKER_IN                   (MCLK_BASE + 10)
#define CLKID_AUDIO_PDMIN0                      (MCLK_BASE + 11)
#define CLKID_AUDIO_PDMIN1                      (MCLK_BASE + 12)
#define CLKID_AUDIO_SPDIFOUT_B                  (MCLK_BASE + 13)
#define CLKID_AUDIO_RESAMPLE_B                  (MCLK_BASE + 14)
#define CLKID_AUDIO_SPDIFIN_LB                  (MCLK_BASE + 15)
#define CLKID_AUDIO_EQDRC                       (MCLK_BASE + 16)
#define CLKID_AUDIO_VAD                         (MCLK_BASE + 17)
#define CLKID_EARCTX_CMDC                       (MCLK_BASE + 18)
#define CLKID_EARCTX_DMAC                       (MCLK_BASE + 19)
#define CLKID_EARCRX_CMDC                       (MCLK_BASE + 20)
#define CLKID_EARCRX_DMAC                       (MCLK_BASE + 21)

#define CLKID_AUDIO_MCLK_PAD0                   (MCLK_BASE + 22)
#define CLKID_AUDIO_MCLK_PAD1                   (MCLK_BASE + 23)
#define CLKID_AUDIO_MCLK_PAD2                   (MCLK_BASE + 24)

#define NUM_AUDIO_CLKS                          (MCLK_BASE + 25)
#endif /* __T5_AUDIO_CLK_H__ */
