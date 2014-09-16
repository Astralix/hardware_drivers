#ifndef __BOARD_DSPG_DMW_H
#define __BOARD_DSPG_DMW_H

/*
 * exported functions
 */
#ifndef __ASSEMBLY__
unsigned long dmw_get_pclk(void);
unsigned long dmw_get_cpufreq(void);
unsigned long dmw_get_dramfreq(void);
void mdelay(unsigned long msec);
void ccu_init(void);
#endif

#ifdef CONFIG_RECOVERY
int is_recovery(void);
#endif

/*
 * chip and board defines
 */
#define DMW_RECOVERY_INFOSIZE 64

#define DMW_CHIP_UNKNOWN		-1
#define DMW_CHIP_DMW96			96

#define DMW_ONCHIP_RAM			0x16000000

#define DMW_MPMC_BASE			0x00100000
#define DMW_SDRAM0_BASE			0x40000000
#define DMW_SDRAM1_BASE			0x80000000
#define DMW_INTRAM_BASE			0x00200000

#define DMW_USB1_OTG_BASE		0x00300000
#define DMW_USB2_OTG_BASE		0x00400000

#define DMW_UART1_BASE			0x05A00000
#define DMW_UART2_BASE			0x05B00000
#define DMW_UART3_BASE			0x05C00000
#define DMW_SYSCFG_BASE			0x05200000
#define DMW_CMU_BASE			0x05300000
#define DMW_NAND_LCD_BASE		0x05400000
#define DMW_TIMER_1_BASE		0x05800000
#define DMW_TIMER_2_BASE		0x05800020
#define DMW_TIMER_3_BASE		0x05900000
#define DMW_TIMER_4_BASE		0x05900020
#define DMW_SPI1_BASE			0x05D00000
#define DMW_SPI2_BASE			0x05E00000
#define DMW_I2C1_BASE			0x06200000
#define DMW_I2C2_BASE			0x06300000
#define DMW_ETHERMAC_BASE		0x06E00000
#define DMW_CCU0_BASE                   0x07000000

#define DMW_CHIP_DMW96_REV_A		0
#define DMW_CHIP_DMW96_REV_B		1

int dmw_chip_revision(void);
int dmw_detect_chip(void);

#endif /* __BOARD_DSPG_DMW_H */
