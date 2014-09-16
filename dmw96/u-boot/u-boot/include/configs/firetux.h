/*
 * firetux board configuration
 *
 * (C) Copyright 2007-2009 emlix GmbH
 * Juergen Schoew <js@emlix.com>
 *
 * (C) Copyright 2008, DSPG Technologies GmbH, Germany
 * (C) Copyright 2007, NXP Semiconductors Germany GmbH
 * Matthias Wenzel, <nxp@mazzoo.de>
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

#define CONFIG_FIRETUX			1
#define CONFIG_ARCH_FIRETUX		1
#define CONFIG_ARM926EJS		1
#define CONFIG_IP3912_ETHER		1
#define CONFIG_IP3912_ETN1_BASE		0xC1600000
#define CONFIG_IP3912_ETN2_BASE		0xC1700000

/* we can have nand _or_ nor flash */
/* #define CONFIG_NANDFLASH		1 */
#define CONFIG_MTD_DEVICE
#define CONFIG_MTD_PARTITIONS

/* #define CONFIG_SHOW_BOOT_PROGRESS	1 */
#define CONFIG_DISPLAY_CPUINFO		1
#define CONFIG_DISPLAY_BOARDINFO	1

#define CONFIG_BOOT_LINUX		1

#ifdef CONFIG_NANDFLASH
  #define VECTOR_TABLE                  0x20780000
#else
  #define VECTOR_TABLE                  0x80000000
#endif

#define CONFIG_USBD_DFU
#ifdef CONFIG_USBD_DFU
  #define CONFIG_USE_IRQ			1
  #define CONFIG_USB_DEVICE			1
  #define CONFIG_USBD_VENDORID			0x145F    /* Linux/NetChip */
  #define CONFIG_USBD_PRODUCTID_GSERIAL		0x5120    /* gserial */
  #define CONFIG_USBD_PRODUCTID_CDCACM		0x511F    /* CDC ACM */
  #define CONFIG_USBD_MANUFACTURER		"DSPG"
  #define CONFIG_USBD_PRODUCT_NAME		"DFU/USB"
  #define CONFIG_USBD_DFU_XFER_SIZE	4096
  #define CONFIG_USBD_DFU_INTERFACE	2
#endif

/* #define CONFIG_SKIP_LOWLEVEL_INIT	1 */
/* #define CONFIG_SKIP_RELOCATE_UBOOT	1 */

#define CONFIG_SYS_ARM_DEBUG_MEM	1

/* MWe has a buggy piece of silicon */
/* #define CONFIG_FIRETUX_HAS_CGU_SCCON_MSC_BUG	1 */

/*
 * High Level Configuration Options
 * (easy to change)
 */
/* Timer 1 is clocked at 1Mhz */
#define CONFIG_SYS_HZ			1000000

/* enable passing of ATAGs  */
#define CONFIG_CMDLINE_TAG		1
#define CONFIG_SETUP_MEMORY_TAGS	1
/* call misc_init_r during start up */
#define CONFIG_MISC_INIT_R		1
#define CONFIG_INITRD_TAG		1
/* mount rootfs read-only */
#define CONFIG_ROOT_RDONLY		1
#define CONFIG_STATUS_LED		1
#define CONFIG_BOARD_SPECIFIC_LED	1
#define STATUS_LED_BIT			1
#define STATUS_LED_PERIOD		(CONFIG_SYS_HZ / 2)
#define STATUS_LED_STATE		STATUS_LED_BLINKING
#define STATUS_LED_ACTIVE		1
#define STATUS_LED_BOOT			1

/* BZIP2 needs 4MB Malloc Size */
/* #define CONFIG_BZIP2			1 */
#define CONFIG_LZMA			1

/*
 * ethernet
 */

/* #define ET_DEBUG			1 */
#define CONFIG_NET_RETRY_COUNT		7
#define CONFIG_NET_MULTI		1
#define CONFIG_MII			1
/* #define CONFIG_DISCOVER_PHY		1 */

/* allow to overwrite serial and ethaddr */
#define CONFIG_ENV_OVERWRITE		1
#ifdef CONFIG_NANDFLASH
#define CONFIG_EXTRA_ENV_SETTINGS     \
		"ethaddr=00:60:37:C0:FF:EE\0"	\
		"netmask=255.255.255.0\0"	\
		"ipaddr=192.168.3.183\0"	\
		"serverip=192.168.3.60\0"	\
		"bootfile=firetux.kernel\0"	\
		"bootcmd=run bootcmd1\0"	\
		"bootcmd1=nboot 0x20200000 0 0x80000 " \
			"&& bootm 0x20200000\0" \
		"bootcmd2=tftpboot 0x20200000 firetux.kernel " \
			"&& bootm 0x20200000\0" \
		"ethact=ETN1\0"			\
		"phymode=auto\0"		\
		"mtdids=nor0=firetux\0"		\
		"mtdparts=mtdparts=firetux:" \
			"384k(U-Boot)," \
			"128k(U-Bootenv)," \
			"1536k(kernel)," \
			"1M(config)," \
			"2M(data)," \
			"-(rootfs)\0" \
		"partition=nor0,5\0"		\
		"unlock=yes\0"			\
		"update.eeprom=echo Update eeprom " \
			"&& tftpboot 0x20000000 firetux.eeprom " \
			"&& eeprom write 0x20000000 0x0 ${filesize}\0" \
		"update.uboot=echo Update u-boot " \
			"&& tftpboot 0x20000000 firetux.nandboot.flash " \
			"&& nand erase 0x0 0x03ffff " \
			"&& nand write.jffs2 0x20000000 0x0 ${filesize}\0" \
		"update.kernel=echo Update kernel " \
			"&& tftpboot 0x20000000 firetux.kernel " \
			"&& nand erase 0x80000 0x17ffff " \
			"&& nand write.jffs2 20000000 0x80000 ${filesize}\0" \
		"update.config=echo Update config " \
			"&& tftpboot 0x20000000 firetux.config " \
			"&& nand erase clean 0x200000 0xfffff " \
			"&& nand write.jffs2 20000000 0x200000 ${filesize}\0" \
		"update.data=echo Update data " \
			"&& tftpboot 0x20000000 firetux.data " \
			"&& nand erase clean 0x300000 0x1fffff " \
			"&& nand write.jffs2 20000000 0x300000 ${filesize}\0" \
		"update.rootfs=echo Update rootfs " \
			"&& tftpboot 0x20000000 firetux.targetfs " \
			"&& nand erase clean 0x500000 " \
			"&& nand write.jffs2 20000000 0x500000 ${filesize}\0" \
		"update.user=run update.rootfs " \
			"&& run update.data " \
			"&& run update.config\0" \
		"update.linux=run update.kernel " \
			"&& run update.user\0" \
		"update.all=run update.uboot " \
			"&& run update.linux\0" \
		""
#else
#define CONFIG_EXTRA_ENV_SETTINGS     \
		"ethaddr=00:60:37:C0:FF:EE\0"	\
		"netmask=255.255.255.0\0"	\
		"ipaddr=192.168.3.183\0"	\
		"serverip=192.168.3.60\0"	\
		"bootfile=firetux.kernel\0"	\
		"bootcmd=run bootcmd1\0"	\
		"bootcmd1=bootm 0x80080000\0" \
		"bootcmd2=tftp 20200000 firetux.kernel  " \
			"&&  bootm 20200000\0" \
		"ethact=ETN1\0"			\
		"phymode=auto\0"		\
		"mtdids=nor0=firetux\0"		\
		"mtdparts=mtdparts=firetux:" \
			"384k(U-Boot)," \
			"128k(U-Bootenv)," \
			"1536k(kernel)," \
			"1M(config)," \
			"2M(data)," \
			"-(rootfs)\0" \
		"partition=nor0,5\0"		\
		"unlock=yes\0"			\
		"update.uboot=echo Update u-boot " \
			"&& tftpboot 0x20000000 firetux.uboot " \
			"&& protect off 0x80000000 0x8005ffff " \
			"&& erase 0x80000000 0x8005ffff " \
			"&& cp.b 0x20000000 0x80000000 ${filesize}\0" \
		"update.kernel=echo Update kernel " \
			"&& tftpboot 0x20000000 firetux.kernel " \
			"&& erase 0x80080000 0x801fffff " \
			"&& cp.b 20000000 0x80080000 ${filesize}\0" \
		"update.config=echo Update config " \
			"&& tftpboot 0x20000000 firetux.config " \
			"&& erase 0x80200000 0x802fffff " \
			"&& cp.b 20000000 0x80200000 ${filesize}\0" \
		"update.data=echo Update data " \
			"&& tftpboot 0x20000000 firetux.data " \
			"&& erase 0x80300000 0x804fffff " \
			"&& cp.b 20000000 0x80300000 ${filesize}\0" \
		"update.rootfs=echo Update rootfs " \
			"&& tftpboot 0x20000000 firetux.targetfs " \
			"&& erase 0x80500000 0x81ffffff " \
			"&& cp.b 20000000 0x80500000 ${filesize}\0" \
		"update.user=run update.rootfs " \
			"&& run update.data " \
			"&& run update.config\0" \
		"update.linux=run update.kernel " \
			"&& run update.user\0" \
		"update.all=run update.uboot " \
			"&& run update.linux\0" \
		""
#endif
				/* better not set "gatewayip" */

/* #define CONFIG_SYS_ETHER_FAIR_SCHEDULE	1 */	/* Use fair or eth schedule */
#define CONFIG_SYS_ETN_RX_DESCRIPTOR_NUMBER	16
#define CONFIG_SYS_ETN_TX_DESCRIPTOR_NUMBER	16

#define MAX_ETH_FRAME_SIZE		1536

#define CONFIG_ENV_SIZE			0x20000		/* 128KB */

/* Size of malloc() pool */
#define CONFIG_SYS_MALLOC_LEN_ETN	(CONFIG_SYS_ETN_RX_DESCRIPTOR_NUMBER \
						* MAX_ETH_FRAME_SIZE)
#define CONFIG_SYS_MALLOC_LEN		(4*128*1024 + CONFIG_ENV_SIZE + \
						CONFIG_SYS_MALLOC_LEN_ETN)
/* min 4MB for bzip2 */
/* #define CONFIG_SYS_MALLOC_LEN		(4 * 1024 * 1024)*/
/* #define CONFIG_SYS_MALLOC_LEN		0x70000 */

/* size in bytes reserved for initial data */
#define CONFIG_SYS_GBL_DATA_SIZE	128


/*
 * Hardware drivers
 */

/*
 * NS16550 Configuration
 */
#define CONFIG_SYS_NS16550		1
#define CONFIG_SYS_NS16550_SERIAL	1
#define CONFIG_CONS_INDEX		2
#define CONFIG_BAUDRATE			115200
#define CONFIG_SYS_BAUDRATE_TABLE	{ 9600, 19200, 38400, 57600, 115200 }
#define CONFIG_SYS_SERIAL0		0xC2004000
#define CONFIG_SYS_SERIAL1		0xC2005000
#define CONFIG_SYS_NS16550_COM1		CONFIG_SYS_SERIAL0
#define CONFIG_SYS_NS16550_COM2		CONFIG_SYS_SERIAL1
#define CONFIG_SYS_NS16550_REG_SIZE	4
/* fclk=27.648==clk26m, uclk=14.756 */
#define CONFIG_SYS_NS16550_CLK		14745600

#define CONFIG_CRC32_VERIFY		1

#define CONFIG_CMDLINE_EDITING		1
#define CONFIG_AUTO_COMPLETE		1
#define CONFIG_MX_CYCLIC		1



/*
 * command line
 */
#include <config_cmd_default.h>
#define CONFIG_CMD_IMI
#define CONFIG_CMD_XIMG
#define CONFIG_CMD_BDI
#define CONFIG_CMD_MEMORY
#define CONFIG_CMD_SAVEENV
#define CONFIG_CMD_CACHE
#define CONFIG_CMD_RUN
#define CONFIG_CMD_MISC
#define CONFIG_CMD_LOADB
#define CONFIG_CMD_LOADS
#define CONFIG_CMD_ELF
#define CONFIG_CMD_CONSOLE
#define CONFIG_CMD_AUTOSCRIPT
#define CONFIG_CMD_ASKENV
#define CONFIG_CMD_NET
#define CONFIG_CMD_NFS
#define CONFIG_CMD_PING
#define CONFIG_CMD_MII
#define CONFIG_CMD_ECHO
#define CONFIG_CMD_BOOTD
#define CONFIG_CMD_DHCP
#define CONFIG_CMD_ITEST
#define CONFIG_CMD_I2C
#define CONFIG_CMD_EEPROM
#ifdef CONFIG_NANDFLASH
#define CONFIG_CMD_NAND
#undef CONFIG_CMD_IMLS
#undef CONFIG_CMD_FLASH
#else
#define CONFIG_CMD_IMLS
#define CONFIG_CMD_FLASH
#define CONFIG_CMD_JFFS2
#define CONFIG_CMD_CRAMFS
#endif
#define CONFIG_CMD_MTDPARTS

/* #define CONFIG_BOOTP_MASK	CONFIG_BOOTP_DEFAULT */


#define CONFIG_BOOTDELAY		10
#define CONFIG_BOOTARGS			"console=ttyS1,115200n8 " \
					"root=/dev/mtdblock5 rootfstype=jffs2" \
					" noalign"
#define CONFIG_BOOTCOMMAND		"run bootcmd1"
#define CONFIG_TIMESTAMP		1
#define CONFIG_TFTP_PORT		1

/*
 * Physical Memory Map
 */
/* if we have psRAM and SDRAM */
/* #define CONFIG_SYS_PSRAM		1 */
#undef  CONFIG_SYS_PSRAM

#define CONFIG_NR_DRAM_BANKS		1
#ifdef CONFIG_SYS_PSRAM
  #define CONFIG_SYS_SDRAM_BASE		0x90000000
#else
  #define CONFIG_SYS_SDRAM_BASE		0x20000000
#endif

#define PHYS_SDRAM_1			CONFIG_SYS_SDRAM_BASE
#define PHYS_SDRAM_1_SIZE		(8<<20)		/* min 8 MB */
#define CONFIG_SYS_MEMTEST_START	PHYS_SDRAM_1
#define CONFIG_SYS_MEMTEST_END		(PHYS_SDRAM_1 + PHYS_SDRAM_1_SIZE - \
					    (1<<20) - 1) /* ramsize - 1MB */
#define CONFIG_SYS_ALT_MEMTEST		1

/*
 * FLASH and environment organization
 */
#ifdef CONFIG_NANDFLASH

#define NAND_MAX_CHIPS			1
#define CONFIG_SYS_MAX_NAND_DEVICE	1
#define CONFIG_SYS_NAND_BASE		0x80000000
#define CONFIG_SYS_NAND_ALE_ADDR	(1<<10)
#define CONFIG_SYS_NAND_CLE_ADDR	(1<<12)
#define CONFIG_SYS_NAND_RB_PORT		0xC2104200
#define CONFIG_SYS_NAND_RB_BIT		18
#define CONFIG_SYS_NO_FLASH		1

#define CONFIG_ENV_IS_IN_NAND		1
#define CONFIG_ENV_OFFSET		0x40000
#define CONFIG_ENV_OFFSET_REDUND	0x60000
#define CONFIG_ENV_OVERWRITE		1


#else /* NOR-FLASH */
/* set optimized access throug board_init */
/* CONFIG_SYS_OPTIMIZE_NORFLASH		1 */

#define CONFIG_ENV_IS_IN_FLASH		1
#define CONFIG_ENV_ADDR			0x80060000
#define CONFIG_ENV_SECT_SIZE		0x20000		/* 128KB */

/* Use drivers/cfi_flash.c */
#define CONFIG_FLASH_CFI_MTD		1
#define CONFIG_FLASH_CFI_DRIVER		1
/* Flash memory is CFI compliant */
#define CONFIG_SYS_FLASH_CFI		1
/* Use buffered writes (~20x faster) */
#define CONFIG_SYS_FLASH_USE_BUFFER_WRITE	1
/* Use hardware sector protection */
#define CONFIG_SYS_FLASH_PROTECTION	1
/* Use Spansion hardware sector protection */
/* #define CONFIG_SYS_AMD_PPB_PROTECTION	1 */

#define CONFIG_SYS_FLASH_BASE		0x80000000
#define CONFIG_SYS_FLASH_BANKS_LIST	{ CONFIG_SYS_FLASH_BASE }
#define CONFIG_SYS_FLASH_EMPTY_INFO	1
/* see include/flash.h for details FLASH_CFI_8BIT */
#define CONFIG_SYS_FLASH_CFI_WIDTH	0x1
/* 4* 32K + 254* 128k + 4* 32k = 262 Sectors */
#define CONFIG_SYS_MAX_FLASH_SECT	262
/* max number of memory banks */
#define CONFIG_SYS_MAX_FLASH_BANKS	1

/* Flash banks JFFS2 should use */
#define CONFIG_SYS_JFFS_CUSTOM_PART	1
#define CONFIG_SYS_JFFS2_SORT_FRAGMENTS	1
#define CONFIG_JFFS2_CMDLINE		1
#define CONFIG_JFFS2_LZO_LZARI		1
#define CONFIG_JFFS2_DEV		"firetux"
#define CONFIG_FLASH_SHOW_PROGRESS	45
#define MTDIDS_DEFAULT			"nor0=firetux"
#define MTDPARTS_DEFAULT		"mtdparts=firetux:" \
						"384k(U-Boot)," \
						"128k(U-Bootenv)," \
						"1536k(kernel)," \
						"1M(config)," \
						"2M(data)," \
						"-(rootfs)"

#ifndef CONFIG_SYS_JFFS_CUSTOM_PART
#define CONFIG_SYS_JFFS2_FIRST_BANK	0
#define CONFIG_SYS_JFFS2_NUM_BANKS	1
/* start addr 0x80500000 */
#define CONFIG_SYS_JFFS2_FIRST_SECTOR	40
#define CONFIG_SYS_JFFS_SINGLE_PART	1
#endif

#endif /* NOR-FLASH */

#define CONFIG_SYS_MONITOR_BASE		CONFIG_SYS_FLASH_BASE
#define CONFIG_SYS_MONITOR_LEN		0x0005ffff

/*
 * Miscellaneous configurable options
 */
/* undef to save memory */
#define CONFIG_SYS_LONGHELP		1
#define CONFIG_SYS_HUSH_PARSER		1
#define CONFIG_SYS_PROMPT_HUSH_PS2	"> "
/* Monitor Command Prompt */
#define CONFIG_SYS_PROMPT		"firetux # "
/* Console I/O Buffer Size*/
#define CONFIG_SYS_CBSIZE		512

/* Print Buffer Size */
#define CONFIG_SYS_PBSIZE		(CONFIG_SYS_CBSIZE + \
						sizeof(CONFIG_SYS_PROMPT)+16)
/* max number of command args */
#define CONFIG_SYS_MAXARGS		16
/* Boot Argument Buffer Size*/
#define CONFIG_SYS_BARGSIZE		CONFIG_SYS_CBSIZE

/* default load address, keep 2 MByte for u-boot/kernel */
#define CONFIG_SYS_LOAD_ADDR		(PHYS_SDRAM_1 + (2<<20))

#ifdef CONFIG_CMD_EEPROM
#define CONFIG_PNX8181_I2C		1
#define CONFIG_SYS_EEPROM_WREN		1
#undef CONFIG_SYS_I2C_MULTI_EEPROMS
#define CONFIG_SYS_I2C_EEPROM_ADDR_LEN	2
#define CONFIG_SYS_I2C_EEPROM_ADDR	0x50
#define CONFIG_SYS_EEPROM_PAGE_WRITE_DELAY_MS	1
#define CONFIG_SYS_I2C_SPEED		100000
#define CONFIG_SYS_I2C_SLAVE		0xfe
#define CONFIG_HARD_I2C			1
#endif
/*
 * Stack sizes
 *
 * The stack sizes are set up in start.S using the settings below
 */
/* regular stack */
#define CONFIG_STACKSIZE	(128*1024)
#ifdef CONFIG_USE_IRQ
/* IRQ stack */
#define CONFIG_STACKSIZE_IRQ	(4*1024)
/* FIQ stack */
#define CONFIG_STACKSIZE_FIQ	(4*1024)
#endif

#endif /* __CONFIG_H */
