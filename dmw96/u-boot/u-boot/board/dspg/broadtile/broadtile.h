#ifndef __BROADTILE_H___
#define __BROADTILE_H___

/* for the dspg_nand driver */
#define MACH_VERSATILE_BROADTILE

/*
 * hardware defines
 */
#define SYS_BASE		0x10000000
#define SYS_ID_OFFSET		0x00
#define SYS_FPGA_REMAP_OFFSET	0x04
#define SYS_LOCK_OFFSET		0x20
#define SYS_CFGDATA1_OFFSET	0x28
#define SYS_CFGDATA2_OFFSET	0x2C
#define SYS_RESETCTL_OFFSET	0x40
#define SYS_ID			(SYS_BASE + SYS_ID_OFFSET)
#define SYS_FPGA_REMAP		(SYS_BASE + SYS_FPGA_REMAP_OFFSET)
#define SYS_LOCK		(SYS_BASE + SYS_LOCK_OFFSET)
#define SYS_CFGDATA1		(SYS_BASE + SYS_CFGDATA1_OFFSET)
#define SYS_CFGDATA2		(SYS_BASE + SYS_CFGDATA2_OFFSET)
#define SYS_RESETCTL		(SYS_BASE + SYS_RESETCTL_OFFSET)

#define SYS_UNLOCK_KEY		0xa05f

#define SSMC_BASE		0x10100000
#define SSMC_STATICCS0_CONFIG	0x14 /* SMSC control register for Bank 0 */
#define SSMC_STATICCS1_CONFIG	0xF4 /* SMSC control register for Bank 7 */
#define SSMC_STATICCS2_CONFIG	0x54 /* SMSC control register for Bank 2 */

#define MPMC_BASE		0x10110000
#define MPMC_STATICCS0_CONFIG	0x200 /* MPMC Static Bank 0 */
#define MPMC_STATICCS1_CONFIG	0x220 /* MPMC Static Bank 1 */
#define MPMC_STATICCS2_CONFIG	0x240 /* MPMC Static Bank 2 */

/* SSMC register definitions */
#define SSMC_BANK0              (SSMC_BASE+0x00)
#define SSMC_BANK1              (SSMC_BASE+0x20)
#define SSMC_BANK2              (SSMC_BASE+0x40)
#define SSMC_BANK3              (SSMC_BASE+0x60)
#define SSMC_BANK4              (SSMC_BASE+0x80)
#define SSMC_BANK5              (SSMC_BASE+0xA0)
#define SSMC_BANK6              (SSMC_BASE+0xC0)
#define SSMC_BANK7              (SSMC_BASE+0xE0)

#define SSMC_IDCYR		0x00
#define SSMC_WSTRDR		0x04
#define SSMC_WSTWRR		0x08
#define SSMC_WSTOENR		0x0C
#define SSMC_WSTWENR		0x10
#define SSMC_CR			0x14
#define SSMC_WSTBRDR		0x1C

/* MPMC static memory register definitions */
#define MPMC_STATIC_BANK0	(MPMC_BASE+0x200)
#define MPMC_STATIC_BANK1	(MPMC_BASE+0x220)
#define MPMC_STATIC_BANK2	(MPMC_BASE+0x240)
#define MPMC_STATIC_BANK3	(MPMC_BASE+0x260)

#define MPMC_STATIC_CONFIG	0x00
#define MPMC_STATIC_WAIT_WEN	0x04
#define MPMC_STATIC_WAIT_OEN	0x08
#define MPMC_STATIC_WAIT_RD	0x0C
#define MPMC_STATIC_WAIT_PAGE	0x10
#define MPMC_STATIC_WAIT_WR	0x14
#define MPMC_STATIC_WAIT_TURN	0x18

/* peripherals on the fpga */
#define DW_TIMER0_1_BASE	0x85900000
#define DW_TIMER2_3_BASE	0x85a00000

#endif /* __BROADTILE_H___ */
