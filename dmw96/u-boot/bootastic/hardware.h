#ifndef __HARDWARE_H
#define __HARDWARE_H

/* modules memory map */
#define DMW96_SDC_BASE		0x00100000	/* sdram controller */
#define DMW96_PSRAM_BASE	0x00200000	/* 64k on-chip ram */
#define DMW96_OTG1_BASE		0x00300000	/* usb1 otg controller */
#define DMW96_OTG0_BASE		0x00400000	/* usb0 otg controller */
#define DMW96_SMSS_BASE		0x00500000	/* sony memory stick slave */
#define DMW96_2D3D_BASE		0x00600000	/* 2d/3d accelerator */
#define DMW96_VIDENC_BASE	0x00700000	/* video encoder */
#define DMW96_VIDDEC_BASE	0x00800000	/* video decoder */
#define DMW96_SDMMC_BASE	0x00900000	/* sd/mmc controller */
#define DMW96_SEC_BASE		0x00a00000	/* security */
#define DMW96_WIFIAHB_BASE	0x00b00000	/* wifi ahb registers */
#define DMW96_ROM_BASE		0x01200000	/* on-chip rom */
#define DMW96_CSS_BASE		0x08000000	/* communication subsystem */
#define DMW96_DDR_BASE		0x40000000	/* ddr memory base */

/* peripheral base addresses */
#define DMW96_GPIO_BASE		0x05000000	/* gpio */
#define DMW96_INTC_BASE		0x05100000	/* interrupt controller */
#define DMW96_SCON_BASE		0x05200000	/* system configuration */
#define DMW96_CMU_BASE		0x05300000	/* clock management unit */
#define DMW96_PMU_BASE		DMW96_CMU_BASE	/* power management unit */
#define DMW96_NFLC_BASE		0x05400000	/* nand flash controller */
#define DMW96_RCU_BASE		0x05500000	/* remote control unit */
#define DMW96_RTC_BASE		0x05600000	/* real time clock */
#define DMW96_WTD_BASE		0x05700000	/* watchdog */
#define DMW96_TIMER0_BASE	0x05800000
#define DMW96_TIMER1_BASE	0x05800020
#define DMW96_TIMER2_BASE	0x05900000
#define DMW96_TIMER3_BASE	0x05900020
#define DMW96_UART1_BASE	0x05a00000
#define DMW96_UART2_BASE	0x05b00000
#define DMW96_UART3_BASE	0x05c00000
#define DMW96_SPI1_BASE		0x05d00000
#define DMW96_SPI2_BASE		0x05e00000
#define DMW96_TDM1_BASE		0x05f00000
#define DMW96_TDM2_BASE		0x06000000
#define DMW96_TDM3_BASE		0x06100000
#define DMW96_I2C1_BASE		0x06200000
#define DMW96_I2C2_BASE		0x06300000
#define DMW96_GDMA_BASE		0x06400000	/* generic dma controller */
#define DMW96_MII_BASE		0x06500000	/* slave mii/rmii */
#define DMW96_PACP_BASE		0x06600000
#define DMW96_CIU_BASE		0x06700000	/* camera interface unit */
#define DMW96_SMSD_BASE		0x06800000	/* sony memory stick dma */
#define DMW96_OSDM_BASE		0x06a00000	/* on screen display manager */
#define DMW96_LCDC_BASE		0x06b00000	/* lcd controller */
#define DMW96_CC_BASE		0x06c00000	/* cryptocell */
#define DMW96_WIFIAPB_BASE	0x06d00000	/* wifi apb registers */
#define DMW96_ETH_BASE		0x06e00000
#define DMW96_CCU0_BASE		0x07000000	/* coherency control unit 0 */
#define DMW96_CCU1_BASE		0x07100000	/* coherency control unit 1 */

/* gpio defiitions */
#define DMW96_GPIO_BANKOFF	0x60

#define DMW96_AGPIO_BASE	(DMW96_GPIO_BASE + DMW96_GPIO_BANKOFF * 0)
#define DMW96_BGPIO_BASE	(DMW96_GPIO_BASE + DMW96_GPIO_BANKOFF * 1)
#define DMW96_CGPIO_BASE	(DMW96_GPIO_BASE + DMW96_GPIO_BANKOFF * 2)
#define DMW96_DGPIO_BASE	(DWM96_GPIO_BASE + DMW96_GPIO_BANKOFF * 3)
#define DMW96_EGPIO_BASE	(DMW96_GPIO_BASE + DMW96_GPIO_BANKOFF * 4)
#define DMW96_FGPIO_BASE	(DMW96_GPIO_BASE + DMW96_GPIO_BANKOFF * 5)
#define DMW96_GGPIO_BASE	(DMW96_GPIO_BASE + DMW96_GPIO_BANKOFF * 6)

#define DMW96_GPIO_DATA		0x00
#define DMW96_GPIO_DATA_SET	0x04
#define DMW96_GPIO_DATA_CLR	0x08
#define DMW96_GPIO_DIR		0x0c
#define DMW96_GPIO_DIR_OUT	0x10
#define DMW96_GPIO_DIR_IN	0x14
#define DMW96_GPIO_EN		0x18
#define DMW96_GPIO_EN_SET	0x1c
#define DMW96_GPIO_EN_CLR	0x20
#define DMW96_GPIO_PULL_CTRL	0x24
#define DMW96_GPIO_PULL_EN	0x28
#define DMW96_GPIO_PULL_DIS	0x2c
#define DMW96_GPIO_PULL_TYPE	0x30
#define DMW96_GPIO_PULL_UP	0x34
#define DMW96_GPIO_PULL_DN	0x38
#define DMW96_GPIO_KEEP		0x3c
#define DMW96_GPIO_KEEP_EN	0x40
#define DMW96_GPIO_KEEP_DIS	0x44
#define DMW96_GPIO_OD		0x48
#define DMW96_GPIO_OD_SET	0x4c
#define DMW96_GPIO_OD_CLR	0x50
#define DMW96_GPIO_IN		0x54

/* system configuration definitions */
#define DMW96_SCON_GCR0		0x00
#define DMW96_SCON_GCR1		0x04
#define DMW96_SCON_GCR2		0x08
#define DMW96_SCON_CORTEX_IR	0x0c

#define DMW96_SCON_SYSOPT_CFG	0x14
#define DMW96_SCON_VIDDEC_PP	0x18
#define DMW96_SCON_CHIP_ID	0x1c
#define DMW96_SCON_CHIP_REV	0x20
#define DMW96_SCON_BMP_CTL	0x24
#define DMW96_SCON_ISOLATION	0x2c
#define DMW96_SCON_POWER_EN	0x30

#define DMW96_SCON_SPCR1	0x54
#define DMW96_SCON_SPCR2	0x58
#define DMW96_SCON_USBOTG1_GCR1	0x5c
#define DMW96_SCON_USBOTG1_GCR2	0x60
#define DMW96_SCON_USBOTG2_GCR1	0x64
#define DMW96_SCON_USBOTG2_GCR2	0x68
#define DMW96_SCON_DRAMCTL_GCR1	0x6c
#define DMW96_SCON_DRAMCTL_GCR2	0x70
#define DMW96_SCON_MEMCFG	0x74
#define DMW96_SCON_MIPICTL1	0x78
#define DMW96_SCON_MIPICTL2	0x7c
#define DMW96_SCON_SA_SIG	0x80
#define DMW96_SCON_SA_MON1	0x84
#define DMW96_SCON_SA_MON2	0x88
#define DMW96_SCON_SA_EFUSE_LCS	0x8c
#define DMW96_SCON_SA_EFUSE_SV  0x90
#define DMW96_SCON_SA_EFUSE_DID	0x94
#define DMW96_SCON_SA_DCU	0x98
#define DMW96_SCON_SA_LCS	0x9c
#define DMW96_SCON_EFUSEC_STATE	0xa0
#define DMW96_SCON_EFUSE_RESRVD 0xa4
#define DMW96_SCON_MEMBIST	0xa8
#define DMW96_SCON_WIFI_MBISTC	0xac
#define DMW96_SCON_WIFI_MBISTS1	0xb0
#define DMW96_SCON_WIFI_MBISTS2	0xb4
#define DMW96_SCON_ARM9_MBIST	0xb8
#define DMW96_SCON_ARMSYS_MBIST	0xbc
#define DMW96_SCON_MS_MBIST	0xc0
#define DMW96_SCON_OSDM_BIST	0xc4
#define DMW96_SCON_MBIST1	0xc8
#define DMW96_SCON_MBIST2	0xcc

#define DMW96_SCON_DRAM_CFG0	0x130
#define DMW96_SCON_DRAM_CFG1	0x134
#define DMW96_SCON_DRAM_CFG2	0x138
#define DMW96_SCON_DRAM_CFG3	0x13c
#define DMW96_SCON_DRAM_CFG4	0x140

/* clock management unit definitions */
#define DMW96_CMU_PLL1		0x00
#define DMW96_CMU_PLL2		0x04
#define DMW96_CMU_PLL3		0x08
#define DMW96_CMU_PLL4		0x0c
#define DMW96_CMU_WIFIPLL	0x10
#define DMW96_CMU_CPUCLK	0x14
#define DMW96_CMU_COMCLKSEL	0x18
#define DMW96_CMU_CLKSWCTL	0x1c
#define DMW96_CMU_CLKDIV1	0x20
#define DMW96_CMU_CLKDIV2	0x24
#define DMW96_CMU_CLKDIV3	0x28
#define DMW96_CMU_CLKDIV4	0x2c
#define DMW96_CMU_CLKDIV5	0x30
#define DMW96_CMU_CLKDIV6	0x34
#define DMW96_CMU_LPTMRR	0x38
#define DMW96_CMU_CLKOSC	0x3c
#define DMW96_CMU_SWSYSRST	0x40
#define DMW96_CMU_SWWRMRST	0x44
#define DMW96_CMU_RSTSR		0x48
#define DMW96_CMU_WRPR		0x4c
#define DMW96_CMU_SWRST1	0x50
#define DMW96_CMU_SWRST2	0x54
#define DMW96_CMU_SWCLKEN1	0x58
#define DMW96_CMU_SWCLKEN2	0x5c
#define DMW96_CMU_CUSTATR	0x60
#define DMW96_CMU_SWOVRCTL	0x64

#endif /* __HARDWARE_H */
