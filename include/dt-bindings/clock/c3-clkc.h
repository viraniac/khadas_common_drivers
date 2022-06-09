/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __C3_CLKC_H
#define __C3_CLKC_H

#define CLKID_FCLK_BASE				0
#define CLKID_FIXED_PLL_VCO			(CLKID_FCLK_BASE + 0)
#define CLKID_FIXED_PLL				(CLKID_FCLK_BASE + 1)
#define CLKID_FCLK50M_DIV40			(CLKID_FCLK_BASE + 2)
#define CLKID_FCLK50M				(CLKID_FCLK_BASE + 3)
#define CLKID_FCLK_DIV2_DIV			(CLKID_FCLK_BASE + 4)
#define CLKID_FCLK_DIV2				(CLKID_FCLK_BASE + 5)
#define CLKID_FCLK_DIV2P5_DIV			(CLKID_FCLK_BASE + 6)
#define CLKID_FCLK_DIV2P5			(CLKID_FCLK_BASE + 7)
#define CLKID_FCLK_DIV3_DIV			(CLKID_FCLK_BASE + 8)
#define CLKID_FCLK_DIV3				(CLKID_FCLK_BASE + 9)
#define CLKID_FCLK_DIV4_DIV			(CLKID_FCLK_BASE + 10)
#define CLKID_FCLK_DIV4				(CLKID_FCLK_BASE + 11)
#define CLKID_FCLK_DIV5_DIV			(CLKID_FCLK_BASE + 12)
#define CLKID_FCLK_DIV5				(CLKID_FCLK_BASE + 13)
#define CLKID_FCLK_DIV7_DIV			(CLKID_FCLK_BASE + 14)
#define CLKID_FCLK_DIV7				(CLKID_FCLK_BASE + 15)

#define CLKID_GP_PLL_BASE			(CLKID_FCLK_BASE + 16)
#define CLKID_GP0_PLL_VCO			(CLKID_GP_PLL_BASE + 0)
#define CLKID_GP0_PLL				(CLKID_GP_PLL_BASE + 1)
#define CLKID_GP1_PLL_VCO			(CLKID_GP_PLL_BASE + 2)
#define CLKID_GP1_PLL				(CLKID_GP_PLL_BASE + 3)
#define CLKID_MCLK_PLL_VCO			(CLKID_GP_PLL_BASE + 4)
#define CLKID_MCLK_PLL				(CLKID_GP_PLL_BASE + 5)
#define CLKID_MCLK_PLL_CLK			(CLKID_GP_PLL_BASE + 6)
#define CLKID_MCLK_0_SEL			(CLKID_GP_PLL_BASE + 7)
#define CLKID_MCLK_0_SEL_OUT			(CLKID_GP_PLL_BASE + 8)
#define CLKID_MCLK_0_DIV2			(CLKID_GP_PLL_BASE + 9)
#define CLKID_MCLK_0				(CLKID_GP_PLL_BASE + 10)
#define CLKID_MCLK_1_SEL			(CLKID_GP_PLL_BASE + 11)
#define CLKID_MCLK_1_SEL_OUT			(CLKID_GP_PLL_BASE + 12)
#define CLKID_MCLK_1_DIV2			(CLKID_GP_PLL_BASE + 13)
#define CLKID_MCLK_1				(CLKID_GP_PLL_BASE + 14)

#define CLKID_MIXED_BASE			(CLKID_GP_PLL_BASE + 15)
#define CLKID_HIFI_PLL_VCO			(CLKID_MIXED_BASE + 0)
#define CLKID_HIFI_PLL				(CLKID_MIXED_BASE + 1)
#define CLKID_SYS_PLL_VCO			(CLKID_MIXED_BASE + 2)
#define CLKID_SYS_PLL				(CLKID_MIXED_BASE + 3)

#define CLKID_SYS_B_SEL				(CLKID_MIXED_BASE + 4)
#define CLKID_SYS_B_DIV				(CLKID_MIXED_BASE + 5)
#define CLKID_SYS_B				(CLKID_MIXED_BASE + 6)
#define CLKID_SYS_A_SEL				(CLKID_MIXED_BASE + 7)
#define CLKID_SYS_A_DIV				(CLKID_MIXED_BASE + 8)
#define CLKID_SYS_A				(CLKID_MIXED_BASE + 9)
#define CLKID_SYS_CLK				(CLKID_MIXED_BASE + 10)
#define CLKID_AXI_A_SEL				(CLKID_MIXED_BASE + 11)
#define CLKID_AXI_A_DIV				(CLKID_MIXED_BASE + 12)
#define CLKID_AXI_A				(CLKID_MIXED_BASE + 13)
#define CLKID_AXI_B_SEL				(CLKID_MIXED_BASE + 14)
#define CLKID_AXI_B_DIV				(CLKID_MIXED_BASE + 15)
#define CLKID_AXI_B				(CLKID_MIXED_BASE + 16)
#define CLKID_AXI_CLK				(CLKID_MIXED_BASE + 17)

/* CPUCTRL_CLK_CTRL */
#define CLKID_CPU_BASE				(CLKID_MIXED_BASE + 18)
#define CLKID_CPU_DYN_CLK			(CLKID_CPU_BASE + 6)
#define CLKID_CPU_CLK				(CLKID_CPU_BASE + 7)

/* CLKTREE_SYS_OSCIN_CTRL gates*/
#define XTAL_BASE				(CLKID_CPU_BASE + 8)
#define CLKID_XTAL_CLKTREE			(XTAL_BASE + 0)
#define CLKID_XTAL_DDRPLL			(XTAL_BASE + 1)
#define CLKID_XTAL_DDRPHY			(XTAL_BASE + 2)
#define CLKID_XTAL_PLLTOP			(XTAL_BASE + 3)
#define CLKID_XTAL_USBPLL			(XTAL_BASE + 4)
#define CLKID_XTAL_MIPI_ISP_VOUT		(XTAL_BASE + 5)
#define CLKID_XTAL_MCLKPLL			(XTAL_BASE + 6)
#define CLKID_XTAL_USBCTRL			(XTAL_BASE + 7)
#define CLKID_XTAL_ETHPLL			(XTAL_BASE + 8)

/* CLKTREE_SYS_CLK_EN0 gates*/
#define GATE_BASE0				(XTAL_BASE + 9)
#define CLKID_SYS_RESET_CTRL			(GATE_BASE0 + 0)
#define CLKID_SYS_PWR_CTRL			(GATE_BASE0 + 1)
#define CLKID_SYS_PAD_CTRL			(GATE_BASE0 + 2)
#define CLKID_SYS_CTRL				(GATE_BASE0 + 3)
#define CLKID_SYS_TS_PLL			(GATE_BASE0 + 4)
#define CLKID_SYS_DEV_ARB			(GATE_BASE0 + 5)
#define CLKID_SYS_MMC_PCLK			(GATE_BASE0 + 6)
#define CLKID_SYS_CAPU				(GATE_BASE0 + 7)
#define CLKID_SYS_CPU_CTRL			(GATE_BASE0 + 8)
#define CLKID_SYS_JTAG_CTRL			(GATE_BASE0 + 8)
#define CLKID_SYS_IR_CTRL			(GATE_BASE0 + 10)
#define CLKID_SYS_IRQ_CTRL			(GATE_BASE0 + 11)
#define CLKID_SYS_MSR_CLK			(GATE_BASE0 + 12)
#define CLKID_SYS_ROM				(GATE_BASE0 + 13)
#define CLKID_SYS_UART_F			(GATE_BASE0 + 14)
#define CLKID_SYS_CPU_ARB			(GATE_BASE0 + 15)
#define CLKID_SYS_RSA				(GATE_BASE0 + 16)
#define CLKID_SYS_SAR_ADC			(GATE_BASE0 + 17)
#define CLKID_SYS_STARTUP			(GATE_BASE0 + 18)
#define CLKID_SYS_SECURE			(GATE_BASE0 + 19)
#define CLKID_SYS_SPIFC				(GATE_BASE0 + 20)
#define CLKID_SYS_NNA				(GATE_BASE0 + 21)
#define CLKID_SYS_ETH_MAC			(GATE_BASE0 + 22)
#define CLKID_SYS_GIC				(GATE_BASE0 + 23)
#define CLKID_SYS_RAMA				(GATE_BASE0 + 24)
#define CLKID_SYS_BIG_NIC			(GATE_BASE0 + 25)
#define CLKID_SYS_RAMB				(GATE_BASE0 + 26)
#define CLKID_SYS_AUDIO_PCLK			(GATE_BASE0 + 27)
/* CLKTREE_SYS_CLK_EN1 gates*/
#define GATE_BASE1				(GATE_BASE0 + 28)
#define CLKID_SYS_PWM_KL			(GATE_BASE1 + 0)
#define CLKID_SYS_PWM_IJ			(GATE_BASE1 + 1)
#define CLKID_SYS_USB				(GATE_BASE1 + 2)
#define CLKID_SYS_SD_EMMC_A			(GATE_BASE1 + 3)
#define CLKID_SYS_SD_EMMC_C			(GATE_BASE1 + 4)
#define CLKID_SYS_PWM_AB			(GATE_BASE1 + 5)
#define CLKID_SYS_PWM_CD			(GATE_BASE1 + 6)
#define CLKID_SYS_PWM_EF			(GATE_BASE1 + 7)
#define CLKID_SYS_PWM_GH			(GATE_BASE1 + 8)
#define CLKID_SYS_SPICC_1			(GATE_BASE1 + 9)
#define CLKID_SYS_SPICC_0			(GATE_BASE1 + 10)
#define CLKID_SYS_UART_A			(GATE_BASE1 + 11)
#define CLKID_SYS_UART_B			(GATE_BASE1 + 12)
#define CLKID_SYS_UART_C			(GATE_BASE1 + 13)
#define CLKID_SYS_UART_D			(GATE_BASE1 + 14)
#define CLKID_SYS_UART_E			(GATE_BASE1 + 15)
#define CLKID_SYS_I2C_M_A			(GATE_BASE1 + 16)
#define CLKID_SYS_I2C_M_B			(GATE_BASE1 + 17)
#define CLKID_SYS_I2C_M_C			(GATE_BASE1 + 18)
#define CLKID_SYS_I2C_M_D			(GATE_BASE1 + 19)
#define CLKID_SYS_I2S_S_A			(GATE_BASE1 + 20)
#define CLKID_SYS_RTC				(GATE_BASE1 + 21)
#define CLKID_SYS_GE2D				(GATE_BASE1 + 22)
#define CLKID_SYS_ISP				(GATE_BASE1 + 23)
#define CLKID_SYS_GPV_ISP_NIC			(GATE_BASE1 + 24)
#define CLKID_SYS_GPV_CVE_NIC			(GATE_BASE1 + 25)
#define CLKID_SYS_MIPI_DSI_HOST			(GATE_BASE1 + 26)
#define CLKID_SYS_MIPI_DSI_PHY			(GATE_BASE1 + 27)
#define CLKID_SYS_ETH_PHY			(GATE_BASE1 + 28)
#define CLKID_SYS_ACODEC			(GATE_BASE1 + 29)
#define CLKID_SYS_DWAP				(GATE_BASE1 + 30)
#define CLKID_SYS_DOS				(GATE_BASE1 + 31)

#define CLKID_SYS_CVE				(GATE_BASE1 + 32)
#define CLKID_SYS_VOUT				(GATE_BASE1 + 33)
#define CLKID_SYS_VC9000E			(GATE_BASE1 + 34)
#define CLKID_SYS_PWM_MN			(GATE_BASE1 + 35)
#define CLKID_SYS_SD_EMMC_B			(GATE_BASE1 + 36)
/* CLKTREE_AXI_CLK_EN gates */
#define GATE_BASE2				(GATE_BASE1 + 37)
#define CLKID_AXI_SYS_NIC			(GATE_BASE2 + 0)
#define CLKID_AXI_ISP_NIC			(GATE_BASE2 + 1)
#define CLKID_AXI_CVE_NIC			(GATE_BASE2 + 2)
#define CLKID_AXI_RAMB				(GATE_BASE2 + 3)
#define CLKID_AXI_RAMA				(GATE_BASE2 + 4)
#define CLKID_AXI_CPU_DMC			(GATE_BASE2 + 5)
#define CLKID_AXI_NIC				(GATE_BASE2 + 6)
#define CLKID_AXI_DMA				(GATE_BASE2 + 7)
#define CLKID_AXI_MUX_NIC			(GATE_BASE2 + 8)
#define CLKID_AXI_CAPU				(GATE_BASE2 + 9)
#define CLKID_AXI_CVE				(GATE_BASE2 + 10)
#define CLKID_AXI_DEV1_DMC			(GATE_BASE2 + 11)
#define CLKID_AXI_DEV0_DMC			(GATE_BASE2 + 12)
#define CLKID_AXI_DSP_DMC			(GATE_BASE2 + 13)
/* dsp clks */
#define DSP_BASE				(GATE_BASE2 + 14)
#define CLKID_DSI_PHY_SEL			(DSP_BASE + 0)
#define CLKID_DSI_PHY_DIV			(DSP_BASE + 1)
#define CLKID_DSI_PHY_CLK			(DSP_BASE + 2)
#define CLKID_DSPA_B_SEL			(DSP_BASE + 3)
#define CLKID_DSPA_B_DIV			(DSP_BASE + 4)
#define CLKID_DSPA_B				(DSP_BASE + 5)
#define CLKID_DSPA_SEL				(DSP_BASE + 6)
#define CLKID_DSPA_EN_DSPA			(DSP_BASE + 7)
#define CLKID_DSPA_EN_NIC			(DSP_BASE + 8)
/* 32k: rtc and ceca clock */
#define RTC_32K_BASE				(DSP_BASE + 9)
#define CLKID_RTC_XTAL_CLKIN			(RTC_32K_BASE + 0)
#define CLKID_RTC_32K_DIV			(RTC_32K_BASE + 1)
#define CLKID_RTC_32K_DIV_SEL			(RTC_32K_BASE + 2)
#define CLKID_RTC_32K_SEL			(RTC_32K_BASE + 3)
#define CLKID_RTC_CLK				(RTC_32K_BASE + 4)
/* 12m/24m/gen clks */
#define BASIC_BASE				(RTC_32K_BASE + 5)
#define CLKID_24M				(BASIC_BASE + 0)
#define CLKID_24M_DIV2				(BASIC_BASE + 1)
#define CLKID_12M				(BASIC_BASE + 2)
#define CLKID_DIV2_PRE				(BASIC_BASE + 3)
#define CLKID_FCLK_DIV2_DIVN			(BASIC_BASE + 4)
#define CLKID_GEN_SEL				(BASIC_BASE + 5)
#define CLKID_GEN_DIV				(BASIC_BASE + 6)
#define CLKID_GEN				(BASIC_BASE + 7)
#define CLKID_SARADC_SEL			(BASIC_BASE + 8)
#define CLKID_SARADC_DIV			(BASIC_BASE + 9)
#define CLKID_SARADC_GATE			(BASIC_BASE + 10)
#define CLKID_PWM_A_SEL				(BASIC_BASE + 11)
#define CLKID_PWM_A_DIV				(BASIC_BASE + 12)
#define CLKID_PWM_A				(BASIC_BASE + 13)
#define CLKID_PWM_B_SEL				(BASIC_BASE + 14)
#define CLKID_PWM_B_DIV				(BASIC_BASE + 15)
#define CLKID_PWM_B				(BASIC_BASE + 16)
#define CLKID_PWM_C_SEL				(BASIC_BASE + 17)
#define CLKID_PWM_C_DIV				(BASIC_BASE + 18)
#define CLKID_PWM_C				(BASIC_BASE + 19)
#define CLKID_PWM_D_SEL				(BASIC_BASE + 20)
#define CLKID_PWM_D_DIV				(BASIC_BASE + 21)
#define CLKID_PWM_D				(BASIC_BASE + 22)
#define CLKID_PWM_E_SEL				(BASIC_BASE + 23)
#define CLKID_PWM_E_DIV				(BASIC_BASE + 24)
#define CLKID_PWM_E				(BASIC_BASE + 25)
#define CLKID_PWM_F_SEL				(BASIC_BASE + 26)
#define CLKID_PWM_F_DIV				(BASIC_BASE + 27)
#define CLKID_PWM_F				(BASIC_BASE + 28)
#define CLKID_PWM_G_SEL				(BASIC_BASE + 29)
#define CLKID_PWM_G_DIV				(BASIC_BASE + 30)
#define CLKID_PWM_G				(BASIC_BASE + 31)
#define CLKID_PWM_H_SEL				(BASIC_BASE + 32)
#define CLKID_PWM_H_DIV				(BASIC_BASE + 33)
#define CLKID_PWM_H				(BASIC_BASE + 34)
#define CLKID_PWM_I_SEL				(BASIC_BASE + 35)
#define CLKID_PWM_I_DIV				(BASIC_BASE + 36)
#define CLKID_PWM_I				(BASIC_BASE + 37)
#define CLKID_PWM_J_SEL				(BASIC_BASE + 38)
#define CLKID_PWM_J_DIV				(BASIC_BASE + 39)
#define CLKID_PWM_J				(BASIC_BASE + 40)
#define CLKID_PWM_K_SEL				(BASIC_BASE + 41)
#define CLKID_PWM_K_DIV				(BASIC_BASE + 42)
#define CLKID_PWM_K				(BASIC_BASE + 43)
#define CLKID_PWM_L_SEL				(BASIC_BASE + 44)
#define CLKID_PWM_L_DIV				(BASIC_BASE + 45)
#define CLKID_PWM_L				(BASIC_BASE + 46)
#define CLKID_PWM_M_SEL				(BASIC_BASE + 47)
#define CLKID_PWM_M_DIV				(BASIC_BASE + 48)
#define CLKID_PWM_M				(BASIC_BASE + 49)
#define CLKID_PWM_N_SEL				(BASIC_BASE + 50)
#define CLKID_PWM_N_DIV				(BASIC_BASE + 51)
#define CLKID_PWM_N				(BASIC_BASE + 52)
#define CLKID_SPICC_A_SEL			(BASIC_BASE + 53)
#define CLKID_SPICC_A_DIV			(BASIC_BASE + 54)
#define CLKID_SPICC_A				(BASIC_BASE + 55)
#define CLKID_MMC_PCLK_SEL			(BASIC_BASE + 56)
#define CLKID_MMC_PCLK_DIV			(BASIC_BASE + 57)
#define CLKID_MMC_PCLK_GATE			(BASIC_BASE + 58)
#define CLKID_MMC_PCLK_MUX			(BASIC_BASE + 59)
#define CLKID_TS_DIV				(BASIC_BASE + 60)
#define CLKID_TS				(BASIC_BASE + 61)
#define CLKID_ETH_RMII_DIV			(BASIC_BASE + 62)
#define CLKID_ETH_RMII				(BASIC_BASE + 63)
#define CLKID_ETH_125M_DIV			(BASIC_BASE + 64)
#define CLKID_ETH_125M				(BASIC_BASE + 65)
#define CLKID_SPICC_B_SEL			(BASIC_BASE + 66)
#define CLKID_SPICC_B_DIV			(BASIC_BASE + 67)
#define CLKID_SPICC_B				(BASIC_BASE + 68)
#define BASIC_BASE2				(BASIC_BASE + 69)

#define CLKID_SPIFC_SEL				(BASIC_BASE2 + 0)
#define CLKID_SPIFC_DIV				(BASIC_BASE2 + 1)
#define CLKID_SPIFC_GATE			(BASIC_BASE2 + 2)
#define CLKID_SPIFC				(BASIC_BASE2 + 3)
#define CLKID_USB_BUS_SEL			(BASIC_BASE2 + 4)
#define CLKID_USB_BUS_DIV			(BASIC_BASE2 + 5)
#define CLKID_USB_BUS				(BASIC_BASE2 + 6)
#define CLKID_SD_EMMC_A_SEL			(BASIC_BASE2 + 7)
#define CLKID_SD_EMMC_A_DIV			(BASIC_BASE2 + 8)
#define CLKID_SD_EMMC_A_GATE			(BASIC_BASE2 + 9)
#define CLKID_SD_EMMC_A				(BASIC_BASE2 + 10)
#define CLKID_SD_EMMC_B_SEL			(BASIC_BASE2 + 11)
#define CLKID_SD_EMMC_B_DIV			(BASIC_BASE2 + 12)
#define CLKID_SD_EMMC_B_GATE			(BASIC_BASE2 + 13)
#define CLKID_SD_EMMC_B				(BASIC_BASE2 + 14)
#define CLKID_SD_EMMC_C_SEL			(BASIC_BASE2 + 15)
#define CLKID_SD_EMMC_C_DIV			(BASIC_BASE2 + 16)
#define CLKID_SD_EMMC_C_GATE			(BASIC_BASE2 + 17)
#define CLKID_SD_EMMC_C				(BASIC_BASE2 + 18)

/*mm clk*/

#define CLKID_MM_BASE0				(BASIC_BASE2 + 19)
#define CLKID_HCODEC_A_SEL			(CLKID_MM_BASE0 + 0)
#define CLKID_HCODEC_A_DIV			(CLKID_MM_BASE0 + 1)
#define CLKID_HCODEC_A_CLK			(CLKID_MM_BASE0 + 2)
#define CLKID_HCODEC_A_GATE			(CLKID_MM_BASE0 + 3)
#define CLKID_HCODEC_B_SEL			(CLKID_MM_BASE0 + 4)
#define CLKID_HCODEC_B_DIV			(CLKID_MM_BASE0 + 5)
#define CLKID_HCODEC_B_CLK			(CLKID_MM_BASE0 + 6)
#define CLKID_HCODEC_CLK			(CLKID_MM_BASE0 + 7)
#define CLKID_VC9000E_ACLK_SEL			(CLKID_MM_BASE0 + 8)
#define CLKID_VC9000E_ACLK_DIV			(CLKID_MM_BASE0 + 9)
#define CLKID_VC9000E_ACLK			(CLKID_MM_BASE0 + 10)
#define CLKID_DEWARPA_CLK_SEL			(CLKID_MM_BASE0 + 12)
#define CLKID_DEWARPA_CLK_DIV			(CLKID_MM_BASE0 + 13)
#define CLKID_DEWARPA_CLK			(CLKID_MM_BASE0 + 14)
#define CLKID_VC9000E_CORE_CLK_SEL		(CLKID_MM_BASE0 + 15)
#define CLKID_VC9000E_CORE_CLK_DIV		(CLKID_MM_BASE0 + 16)
#define CLKID_VC9000E_CORE_CLK			(CLKID_MM_BASE0 + 17)

#define CLKID_MM_BASE1				(CLKID_MM_BASE0 + 18)
#define CLKID_CSI_PHY0_CLK_SEL			(CLKID_MM_BASE1 + 0)
#define CLKID_CSI_PHY0_CLK_DIV			(CLKID_MM_BASE1 + 1)
#define CLKID_CSI_PHY0_CLK			(CLKID_MM_BASE1 + 2)
#define CLKID_ISP0_CLK_SEL			(CLKID_MM_BASE1 + 4)
#define CLKID_ISP0_CLK_DIV			(CLKID_MM_BASE1 + 5)
#define CLKID_ISP0_CLK				(CLKID_MM_BASE1 + 6)
#define CLKID_NNA_AXI_SEL			(CLKID_MM_BASE1 + 8)
#define CLKID_NNA_AXI_DIV			(CLKID_MM_BASE1 + 9)
#define CLKID_NNA_AXI_CLK			(CLKID_MM_BASE1 + 10)
#define CLKID_NNA_CORE_SEL			(CLKID_MM_BASE1 + 12)
#define CLKID_NNA_CORE_DIV			(CLKID_MM_BASE1 + 13)
#define CLKID_NNA_CORE_CLK			(CLKID_MM_BASE1 + 14)
#define CLKID_VOUT_MCLK_SEL			(CLKID_MM_BASE1 + 16)
#define CLKID_VOUT_MCLK_DIV			(CLKID_MM_BASE1 + 17)
#define CLKID_VOUT_MCLK				(CLKID_MM_BASE1 + 18)
#define CLKID_VOUT_ENC_CLK_SEL			(CLKID_MM_BASE1 + 20)
#define CLKID_VOUT_ENC_CLK_DIV			(CLKID_MM_BASE1 + 21)
#define CLKID_VOUT_ENC_CLK			(CLKID_MM_BASE1 + 22)
#define CLKID_GE2D_SEL				(CLKID_MM_BASE1 + 24)
#define CLKID_GE2D_DIV				(CLKID_MM_BASE1 + 25)
#define CLKID_GE2D_CLK				(CLKID_MM_BASE1 + 26)

#define CLKID_VAPB_CLK_BASE			(CLKID_MM_BASE1 + 27)
#define CLKID_VAPB_CLK_SEL			(CLKID_VAPB_CLK_BASE + 0)
#define CLKID_VAPB_CLK_DIV			(CLKID_VAPB_CLK_BASE + 1)
#define CLKID_VAPB_CLK				(CLKID_VAPB_CLK_BASE + 2)
#define CLKID_END_BASE				(CLKID_VAPB_CLK_BASE + 3)

#endif /* __C3_CLKC_H */
