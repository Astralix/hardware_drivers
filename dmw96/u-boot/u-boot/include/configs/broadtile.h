/*
 * (C) Copyright 2003
 * Texas Instruments.
 * Kshitij Gupta <kshitij@ti.com>
 * Configuation settings for the TI OMAP Innovator board.
 *
 * (C) Copyright 2004
 * ARM Ltd.
 * Philippe Robin, <philippe.robin@arm.com>
 * Configuration for Versatile PB.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __CONFIG_H
#define __CONFIG_H

/*
 * High Level Configuration Options
 * (easy to change)
 */
#define CONFIG_ARM926EJS	1	/* This is an arm926ejs CPU core */
#define CONFIG_VERSATILE	1	/* in Versatile Platform Board	*/
#define CONFIG_ARCH_VERSATILE	1	/* Specifically, a Versatile	*/

#define CONFIG_ARCH_VERSATILE_BROADTILE 1 /* with a broadtile board */

#define CONFIG_SYS_MEMTEST_START	0x100000
#define CONFIG_SYS_MEMTEST_END		0x10000000
#define CONFIG_SYS_HZ			(1000000 / 256)
#define CONFIG_SYS_TIMERBASE		0x101E2000	/* Timer 0 and 1 base */

#define CONFIG_SYS_TIMER_INTERVAL	10000
#define CONFIG_SYS_TIMER_RELOAD		(CONFIG_SYS_TIMER_INTERVAL >> 4)
#define CONFIG_SYS_TIMER_CTRL		0x84		/* Enable, Clock / 16 */

/*
 * control registers
 */
#define VERSATILE_SCTL_BASE		0x101E0000	/* System controller */

/*
 * System controller bit assignment
 */
#define VERSATILE_REFCLK	0
#define VERSATILE_TIMCLK	1

#define VERSATILE_TIMER1_EnSel	15
#define VERSATILE_TIMER2_EnSel	17
#define VERSATILE_TIMER3_EnSel	19
#define VERSATILE_TIMER4_EnSel	21

#define CONFIG_CMDLINE_TAG		1	/* enable passing of ATAGs */
#define CONFIG_SETUP_MEMORY_TAGS	1
#define CONFIG_MISC_INIT_R		1
/*
 * Size of malloc() pool
 */
#define CONFIG_ENV_SIZE			8192
#define CONFIG_SYS_MALLOC_LEN		(CONFIG_ENV_SIZE + (2 << 20))

/*
 * Hardware drivers
 */

#define CONFIG_NET_MULTI
#define CONFIG_SMC91111
#define CONFIG_SMC_USE_32_BIT
#define CONFIG_SMC91111_BASE	0x10010000
#undef CONFIG_SMC91111_EXT_PHY

/*
In order to operate the GMAC driver please comment it out

#define CONFIG_MACG
#define CONFIG_NET_MULTI
*/

/*
 * NS16550 Configuration
 */
#define CONFIG_PL011_SERIAL
#define CONFIG_PL011_CLOCK	24000000
#define CONFIG_PL01x_PORTS				\
			{(void *)CONFIG_SYS_SERIAL0,	\
			 (void *)CONFIG_SYS_SERIAL1 }
#define CONFIG_CONS_INDEX	0

#define CONFIG_BAUDRATE			115200
#define CONFIG_SYS_BAUDRATE_TABLE	{ 9600, 19200, 38400, 57600, 115200 }
#define CONFIG_SYS_SERIAL0		0x101F1000
#define CONFIG_SYS_SERIAL1		0x101F2000

/*
 * Command line configuration.
 */
#define CONFIG_CMD_BDI
#define CONFIG_CMD_DHCP
#define CONFIG_CMD_FLASH
#define CONFIG_CMD_IMI
#define CONFIG_CMD_MEMORY
#define CONFIG_CMD_NET
#define CONFIG_CMD_PING
#define CONFIG_CMD_SAVEENV
#define CONFIG_CMD_RUN
#define CONFIG_CMD_MTDPARTS
#define CONFIG_CMD_DEFENV

#ifdef CONFIG_NAND
# define CONFIG_CMD_NAND
#endif

/*
 * BOOTP options
 */
#define CONFIG_BOOTP_BOOTPATH
#define CONFIG_BOOTP_GATEWAY
#define CONFIG_BOOTP_HOSTNAME
#define CONFIG_BOOTP_SUBNETMASK

#define CONFIG_BOOTDELAY	2
#define CONFIG_BOOTARGS		"root=/dev/nfs mem=128M ip=dhcp "\
				"netdev=25,0,0xf1010000,0xf1010010,eth0"

/*
 * Static configuration when assigning fixed address
 */
#define CONFIG_BOOTFILE		"uImage" /* file to load */

/*
 * Miscellaneous configurable options
 */
#define CONFIG_SYS_LONGHELP	/* undef to save memory */
#define CONFIG_SYS_CBSIZE	256		/* Console I/O Buffer Size */
#define CONFIG_SYS_PROMPT	"Broadtile # "	/* Monitor Command Prompt   */
/* Print Buffer Size */
#define CONFIG_SYS_PBSIZE	\
			(CONFIG_SYS_CBSIZE + sizeof(CONFIG_SYS_PROMPT) + 16)
#define CONFIG_SYS_MAXARGS	16		/* max number of command args */
#define CONFIG_SYS_BARGSIZE	CONFIG_SYS_CBSIZE /* Boot Argument Buffer Size */

#define CONFIG_SYS_LOAD_ADDR	0x7fc0	/* default load address */

/*-----------------------------------------------------------------------
 * Stack sizes
 *
 * The stack sizes are set up in start.S using the settings below
 */
#define CONFIG_STACKSIZE	(128 * 1024)	/* regular stack */
#ifdef CONFIG_USE_IRQ
#define CONFIG_STACKSIZE_IRQ	(4 * 1024)	/* IRQ stack */
#define CONFIG_STACKSIZE_FIQ	(4 * 1024)	/* FIQ stack */
#endif

/*-----------------------------------------------------------------------
 * Physical Memory Map
 */
#define CONFIG_NR_DRAM_BANKS             1
#ifdef CONFIG_BROADTILE_HIGH_BANK
# define PHYS_SDRAM_1                    0xE0000000  /* SDRAM Bank #2 */
# ifdef CONFIG_BROADTILE_HIGH_BANK_128M
#  define PHYS_SDRAM_1_SIZE              (128 << 20) /* 128 MB */
# else
#  define PHYS_SDRAM_1_SIZE              ( 32 << 20) /* 32 MB */
# endif
#else
# define PHYS_SDRAM_1                    0x00000000  /* SDRAM Bank #1 */
# define PHYS_SDRAM_1_SIZE               (128 << 20)  /* 128 MB */
#endif

#define CONFIG_SYS_SDRAM_BASE		PHYS_SDRAM_1
#define CONFIG_SYS_INIT_RAM_ADDR	0x04000000 /* dram, end of nor mapping */
#define CONFIG_SYS_INIT_RAM_SIZE	(2 << 20)  /* 2mb */
#define CONFIG_SYS_GBL_DATA_OFFSET	(CONFIG_SYS_INIT_RAM_SIZE - \
						GENERATED_GBL_DATA_SIZE)
#define CONFIG_SYS_INIT_SP_ADDR	(CONFIG_SYS_INIT_RAM_ADDR + \
						CONFIG_SYS_GBL_DATA_OFFSET)

#define CONFIG_BOARD_EARLY_INIT_F

#define PHYS_FLASH_SIZE		0x04000000	/* 64MB */

/*-----------------------------------------------------------------------
 * FLASH and environment organization
 */
#define CONFIG_MTD_DEVICE
#define CONFIG_MTD_PARTITIONS
#define CONFIG_SYS_TEXT_BASE  0x0
/*
 * Use the CFI flash driver for ease of use
 */
#define CONFIG_SYS_FLASH_CFI
#define CONFIG_SYS_FLASH_USE_BUFFER_WRITE

#define CONFIG_FLASH_CFI_MTD
#define CONFIG_FLASH_CFI_DRIVER
#define CONFIG_FLASH_SHOW_PROGRESS 45

#define CONFIG_ENV_IS_IN_FLASH	1
/*
 *	System control register
 */
#define VERSATILE_SYS_BASE		0x10000000
#define VERSATILE_SYS_FLASH_OFFSET	0x4C
#define VERSATILE_FLASHCTRL		\
		(VERSATILE_SYS_BASE + VERSATILE_SYS_FLASH_OFFSET)
/* Enable writing to flash */
#define VERSATILE_FLASHPROG_FLVPPEN	(1 << 0)

/* timeout values are in ticks */
#define CONFIG_SYS_FLASH_ERASE_TOUT	(2 * CONFIG_SYS_HZ) /* Erase Timeout */
#define CONFIG_SYS_FLASH_WRITE_TOUT	(2 * CONFIG_SYS_HZ) /* Write Timeout */

/*
 * Note that CONFIG_SYS_MAX_FLASH_SECT allows for a parameter block
 * i.e.
 *	the bottom "sector" (bottom boot), or top "sector"
 *	(top boot), is a seperate erase region divided into
 *	4 (equal) smaller sectors. This, notionally, allows
 *	quicker erase/rewrire of the most frequently changed
 *	area......
 *	CONFIG_SYS_MAX_FLASH_SECT is padded up to a multiple of 4
 */

#define FLASH_SECTOR_SIZE		0x00040000	/* 256 KB sectors */
#define CONFIG_ENV_SECT_SIZE		FLASH_SECTOR_SIZE
#define CONFIG_SYS_MAX_FLASH_SECT	(260)

#define CONFIG_SYS_FLASH_BASE		0x34000000
#define CONFIG_SYS_MAX_FLASH_BANKS	1

#define CONFIG_SYS_MONITOR_LEN		(4 * CONFIG_ENV_SECT_SIZE)

/* The ARM Boot Monitor is shipped in the lowest sector of flash */

#define FLASH_TOP			(CONFIG_SYS_FLASH_BASE + PHYS_FLASH_SIZE)
#define CONFIG_ENV_ADDR			(FLASH_TOP - CONFIG_ENV_SECT_SIZE)
#define CONFIG_ENV_OFFSET		(CONFIG_ENV_ADDR - CONFIG_SYS_FLASH_BASE)
#define CONFIG_SYS_MONITOR_BASE		(CONFIG_ENV_ADDR - CONFIG_SYS_MONITOR_LEN)

#define CONFIG_SYS_FLASH_PROTECTION	/* The devices have real protection */
#define CONFIG_SYS_FLASH_EMPTY_INFO	/* flinfo indicates empty blocks */

/*
 * NAND flash organization
 */
#ifdef CONFIG_NAND
  #define CONFIG_NAND_DMW96
  #define CONFIG_NAND_DMW96_REV1
  #define CONFIG_JFFS2_NAND
  #define CONFIG_JFFS2_DEV		"nand0" // Needed to read jffs2 file system from nand0
  #define MTDIDS_DEFAULT		"nor0=dwflash.0,nand0=dwnand"
  #define MTDPARTS_DEFAULT		"mtdparts=dwflash.0:512k(u-boot);dwnand:3m(kernel1),3m(kernel2),-(rootfs)"
  #define CONFIG_SYS_NAND_BASE		0x85400000
  #define CONFIG_SYS_MAX_NAND_DEVICE	1
  #define CONFIG_SYS_MAX_NAND_DEVICE	1
  #define NAND_MAX_CHIPS		1 // needed for u-boot bbt
  #define CONFIG_SYS_64BIT_VSPRINTF	1
#endif

#define CONFIG_EXTRA_ENV_SETTINGS \
	"swupgrade=protect off all " \
		"&& erase 0x34000000 0x3407ffff " \
		"&& cp.b 0x02000000 0x34000000 0x80000 " \
		"&& save && reset\0" \
	"nandupgrade=nand erase 0 0x60000 " \
		"&& nand write 0x02000000 0 0x60000\0"

#define CONFIG_CMDLINE_EDITING		1
#define CONFIG_AUTO_COMPLETE		1
#define CONFIG_SYS_HUSH_PARSER		1
#define CONFIG_SYS_PROMPT_HUSH_PS2	"> "

#define CONFIG_PANIC_HANG		1

#endif	/* __CONFIG_H */
