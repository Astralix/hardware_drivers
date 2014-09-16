#ifndef __BOARD_DSPG_DW_H
#define __BOARD_DSPG_DW_H

/*
 * exported functions
 */
#ifndef __ASSEMBLY__
unsigned int dw_get_pclk(void);
int dw_detect_board(void);
unsigned long get_timer(unsigned long base);
#endif

/*
 * chip and board defines
 */
#define DW_CHIP_UNKNOWN			-1
#define DW_CHIP_DW52			52
#define DW_CHIP_DW74			74
#define DW_BOARD_UNKNOWN		-1
#define DW_BOARD_EXPEDIBLUE		0
#define DW_BOARD_EXPEDITOR1		1
#define DW_BOARD_EXPEDITOR2		2
#define DW_BOARD_EXPEDIWAU_BASIC	3
#define DW_BOARD_IMB			4
#define DW_BOARD_IFRAME			5
#define DW_BOARD_IMB2			6
#define DW_BOARD_IMH			7

/*
 * common defines
 */
#define IO_ADDRESS(x)			((void *)x)

/*
 * hardware registers
 */

/* System Control Registers */

/* Internal On-chip RAM */

#define DW_ONCHIP_RAM			0x16000000

/* Dynamic memory controller */

#define MPMC_BASE			0x09100000
#define DW_SDRAM_BASE			0x60000000

#define MPMCDynamicConfig0		0x0100
#define MPMCDynamicConfig1		0x0120

#ifndef __ASSEMBLY__
enum MPMC_CHIPSELECT {
	MPMC_CHIPSELECT_0 = 0,
	MPMC_CHIPSELECT_1,
	MPMC_CHIPSELECT_2,
	MPMC_CHIPSELECT_3,
	MPMC_CHIPSELECT_4,
	MPMC_CHIPSELECT_5,
	MPMC_CHIPSELECT_6,
	MPMC_CHIPSELECT_7,
};
#endif /* __ASSEMBLY__ */


#define NOR_FLASH_EXP			0x30000000
#define NOR_FLASH			0x34000000
#define EXP_FLASH			0x3C000000

/* UART definitions */
#define DW_UART1_BASE			0x05B00000	/* UART1 */
#define DW_UART2_BASE			0x05C00000	/* UART1 */

#define DW_UART_CTL			0x0000		/* UART control register.		Default = 0x0 */
#define DW_UART_CFG			0x0004		/* UART configuration register.		Default = 0x0 */
#define DW_UART_DIV			0x0008		/* UART divider register.		Default = 0x0 */
#define DW_UART_FIFO_AE_LEVEL		0x000C		/* FIFO Tx wateramrk register.		Default = (FIFO size)/2 */
#define DW_UART_FIFO_AF_LEVEL		0x0010		/* FIFO Rx wateramrk register.		Default = (FIFO size)/2 */
#define DW_UART_FIFO_ICR		0x001C		/* FIFO interrrupt clear register.	Default = 0x0 */

/* System Config definitions */
#define DW_SYSCFG_BASE			0x05200000
#define DW_SYSCFG_GCR0			0x00
#define DW_SYSCFG_GCR1			0x04
#define DW_SYSCFG_CHIP_ID		0x08
#define DW_SYSCFG_CHIP_REV		0x0C
#define DW_SYSCFG_IO_LOC		0x20
#define DW_SYSCFG_MCU_ISOLATION0	0x40
#define DW_SYSCFG_MCU_POWER_EN0		0x44

/* CMU definitions */
#define DW_CMU_BASE			0x05300000
#define DW_CMU_CMR			0x0000
#define DW_CMU_CSR0			0x0004
#define DW_CMU_CSR1			0x0008
#define DW_CMU_LPCR			0x0018
#define DW_CMU_CDR0			0x001C
#define DW_CMU_CDR1			0x0020
#define DW_CMU_CDR2			0x0024
#define DW_CMU_CDR3			0x0028
#define DW_CMU_COMR			0x002C
#define DW_CMU_PLL2CR0			0x0034
#define DW_CMU_PLL2CR1			0x0038
#define DW_CMU_RSTSR			0x0040
#define DW_CMU_BSWRSTR			0x0050
#define DW_CMU_DMR			0x0054
#define DW_CMU_DSPCR			0x0058
#define DW_CMU_DSPRST			0x005C
#define DW_CMU_OSCR			0x0060

/* new for DW74 */
#define DW_CMU_CSR2			0x0064
#define DW_CMU_CDR4			0x0078
#define DW_CMU_CDR5			0x007C
#define DW_CMU_CDR6			0x0080
#define DW_CMU_PLL3CR0			0x0068
#define DW_CMU_PLL3CR1			0x006C
#define DW_CMU_PLL4CR0			0x0070
#define DW_CMU_PLL4CR1			0x0074
#define DW_CMU_CDR6			0x0080

/* Interrupt controller */

#define DW_SIC_BASE			0x05100000

/* USB Device controller */
#define DW_SIC_IRQNR_USB		5

/* Misc definitions */

#define DW_MPMC_BASE			0x09100000	/* MPMC */
#define DW_MPMC_DYN_CTRL		(DW_MPMC_BASE +0x20)	/* MPMC dynamic control*/
#define DW_MPMC_STATUS			(DW_MPMC_BASE +0x4)	/* MPMC Status*/


#define DW_NAND_LCD_BASE		0x05400000
#define DW_DMA_BASE			0x05500000
#define DW_FIFO_BASE			0x05600000
#define DW_TIMER0_1_BASE		0x05900000
#define DW_TIMER2_3_BASE		0x05A00000
#define DW_SBUS_BASE			0x05D00000

#define DW_MACA_BASE			0x06100000
#define DW_MACA_CONTROL			0x0000
#define DW_MACA_PHYMAN			0x0034

#define BRD_UNLOCK_KEY			0xA05F

#define MAC_ADDR_BASE			0x50C21B4800ULL

#define DW_SSRAM_BASE			0x16000000

/* SBUS registers */
#define SBUS_TX				0x0000
#define SBUS_RX				0x0004
#define SBUS_CFG			0x0008
#define SBUS_CTL			0x000C
#define SBUS_STAT			0x0010
#define SBUS_FREQ			0x0014

#endif /* __BOARD_DSPG_DW_H */
