/*
 * (C) Copyright 2010
 * DSPG Technologies GmbH
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

#ifndef __CONFIG_DMW_H
#define __CONFIG_DMW_H

#include <asm/sizes.h>

/*
 * Physical Memory Map
 */
#define CONFIG_NR_DRAM_BANKS		1
#define PHYS_SDRAM_1				0x40000000	/* SDRAM Bank #1 */
#define CONFIG_SYS_SDRAM_BASE		PHYS_SDRAM_1

#define CONFIG_SYS_TEXT_BASE		0x41000000
#define CONFIG_SYS_LOAD_ADDR		0x42000000

#define DMW96_SDC_BASE				0x00100000	/* sdram controller */
#define DMW96_SDC_SIZE_OFFSET				38

/*
 * Bit mask on Denali DDR controller register 38 (configured in bootastic)
 */
#define DMW96_DDR_4Gb_MASK			0x00000002
#define DMW96_DDR_2Gb_MASK			0x00000001

/*
 * Size of malloc() pool
 */
#define CONFIG_SYS_MALLOC_LEN		(2 * SZ_1M)

/*
 * Stack sizes - set up in start.S using sizes below
 */
#define CONFIG_SYS_INIT_SP_ADDR		(CONFIG_SYS_SDRAM_BASE + SZ_4K - \
                                         GENERATED_GBL_DATA_SIZE)
#define CONFIG_STACKSIZE		(SZ_128K)	/* regular stack */

/* #define CONFIG_L2_OFF */
#define CONFIG_SYS_NO_DCACHE

#define CONFIG_BOARD_EARLY_INIT_F
#define BOARD_LATE_INIT			1
#define CONFIG_BOOTMENU
#define CFG_PBSIZE			200

#define _DEBUG

#define CONFIG_ARMV7			1
#define CONFIG_DSPG_DMW			1
#define CONFIG_HAVE_CCU			1

#define CONFIG_DISPLAY_CPUINFO		1
#define CONFIG_DISPLAY_BOARDINFO	1

#define CONFIG_VERSION_VARIABLE		1
#define CONFIG_IDENT_STRING		""

#define CONFIG_SYS_ALT_MEMTEST		1
#define CONFIG_SYS_MEMTEST_START	0x100000
#define CONFIG_SYS_MEMTEST_END		0x10000000

#define CONFIG_INITRD_TAG		1

#define CONFIG_CMDLINE_TAG		1 /* pass cmdline  */
#define CONFIG_REVISION_TAG		1 /* pass cpu revision */
#define CONFIG_SETUP_MEMORY_TAGS	1
#define CONFIG_MISC_INIT_R		1 /* call misc_init_r during start up */

#define CONFIG_VIDEO			1
#define CONFIG_LCD			1
#define LCD_BPP				LCD_COLOR16
#define CONFIG_CMD_BMP			1
#define CONFIG_BMP_16BPP		1
#define CONFIG_CFB_CONSOLE		1
#define CONFIG_VGA_AS_SINGLE_DEVICE	1

#define VIDEO_KBD_INIT_FCT	0
#define VIDEO_TSTC_FCT		serial_tstc
#define VIDEO_GETC_FCT		serial_getc

/* #define VIDEO_FB_LITTLE_ENDIAN */
/* #define VIDEO_FB_16BPP_PIXEL_SWAP */
#define VIDEO_FB_16BPP_WORD_SWAP

#define CONFIG_SYS_HZ			1000

/*
 * Hardware drivers
 */

#define CONFIG_MACG
#define CONFIG_NET_MULTI		1

#define CONFIG_MMC
#define CONFIG_DOS_PARTITION
#define CONFIG_EFI_PARTITION
#define CONFIG_GENERIC_MMC
#define CONFIG_SYNDWC_MMC
#define CONFIG_SYS_SYNDWC_BASE		0x00900000

/* SPI */
#define CONFIG_DMW96_SPI

/*
 * NS16550 Configuration
 */
#define CFG_DSPG_DMW_SERIAL

#define CONFIG_CMDLINE_EDITING		1
#define CONFIG_AUTO_COMPLETE		1
#define CONFIG_MX_CYCLIC		1

#define CONFIG_CONS_INDEX		0

#define CONFIG_BAUDRATE			115200
#define CONFIG_SYS_BAUDRATE_TABLE	{ 9600, 19200, 38400, 57600, 115200 }
/* #define CONFIG_SYS_SERIAL0		0x101F1000
#define CONFIG_SYS_SERIAL1		0x101F2000 */

#define CONFIG_BOOTP_MASK		CONFIG_BOOTP_DEFAULT

#define	CONFIG_SYS_HUSH_PARSER		1
#define	CONFIG_SYS_PROMPT_HUSH_PS2	"> "

/*
 * USB
 */
#define CONFIG_USB_DEVICE		1
#define CONFIG_USB_TTY			1
#define CONFIG_SYS_CONSOLE_IS_IN_ENV	1
#define CONFIG_USBD_DFU			1

#define CONFIG_USB_HIGH_SPEED		1
#define CONFIG_DWC_OTG_UDC		1

/* DWC driver settings */
#define CONFIG_DWC_U_BOOT		1
#define CONFIG_USB_DWC_DEBUG		1
#define CONFIG_USB_GADGET_DWC_HDRC	1
#define CONFIG_USB_DWC_DEVICE_ONLY	1

#define CONFIG_USB_DWC_PORT		1

#if CONFIG_USB_DWC_PORT==1
#define CONFIG_SYS_DWC_OTG_BASE		0x00300000
#else
#define CONFIG_SYS_DWC_OTG_BASE		0x00400000
#endif

#define CONFIG_USBD_VENDORID		0x1d50    /* Linux/NetChip */
#define CONFIG_USBD_PRODUCTID_GSERIAL	0x5120    /* gserial */
#define CONFIG_USBD_PRODUCTID_CDCACM	0x5119
#define CONFIG_USBD_MANUFACTURER	"DSPG"
#define CONFIG_USBD_PRODUCT_NAME	"DFU/USB"
#define CONFIG_USBD_DFU_XFER_SIZE	4096
#define CONFIG_USBD_DFU_INTERFACE	2

#ifdef CONFIG_USBD_DFU
  #define CONFIG_USBD_DFUF		1
  #ifdef CONFIG_USBD_DFUF
    #define CONFIG_USBD_DFUF_SI		1
  #else
    #undef  CONFIG_USBD_DFUF_SI
  #endif
#endif

#if 1
#define CONFIG_SYS_NO_FLASH
#else
#define CONFIG_SYS_FLASH_BASE          0x01100000
#define CONFIG_SYS_MAX_FLASH_BANKS     1
#define CONFIG_SYS_MAX_FLASH_SECT      256
#define CONFIG_SYS_MONITOR_BASE        CONFIG_SYS_FLASH_BASE
#define CONFIG_SYS_FLASH_CFI           1
#define CONFIG_FLASH_CFI_DRIVER        1
#define CONFIG_FLASH_CFI_LEGACY        1
#define CONFIG_SYS_FLASH_LEGACY_256kx16 1
//#define CONFIG_SYS_FLASH_USE_BUFFER_WRITE      1       /* ~10x faster */
#endif


#include <config_cmd_default.h>
#define CONFIG_CMD_IMI
#define CONFIG_CMD_BDI
#define CONFIG_CMD_MEMORY
#define CONFIG_CMD_ENV
#define CONFIG_CMD_RUN
#define CONFIG_CMD_NET
#define CONFIG_CMD_ECHO
#define CONFIG_CMD_DHCP
#define CONFIG_CMD_PING
#define CONFIG_CMD_ITEST
#define CONFIG_CMD_FLASH
#define CONFIG_CMD_REV
#define CONFIG_CMD_DPCALIBRATION
#define CONFIG_CMD_MMC
#define CONFIG_CMD_FAT
#define CONFIG_CMD_GPTPART
#define CONFIG_CMD_DISK
#define CONFIG_CMD_GREPENV
#define CONFIG_CMD_DEFENV

#define CONFIG_CONTROL_BOOT

#define DEFAULT_MIN_OPERATION_MV 3700

#define CONFIG_LZO    /* needed for UBI */
#define CONFIG_RBTREE /* needed for UBI */

#define MTDIDS_DEFAULT		"nand0=dmw_nand"
#define MTDPARTS_DEFAULT	"mtdparts="                                   \
					"dmw_nand:"                           \
						"2m(u-boot),"                 \
						"1m(u-boot-env),"             \
						"2m(factory)ro,"              \
						"2m(ubootlogo),"              \
						"5m(boot),"                   \
						"5m(recovery),"               \
						"150m(userdata),"             \
						"80m(cache),"                 \
						"-(system)"

/* Environment settings */
#define CONFIG_ENV_OVERWRITE
#if defined(CONFIG_NANDBOOT)
#define CONFIG_ENV_IS_IN_NAND
#define CONFIG_ENV_OFFSET		(  2 << 20) /* 2M */
#define CONFIG_ENV_SIZE			(256 << 10) /* 256k */
#define CONFIG_ENV_RANGE		(  1 << 20) /* 1M */
#elif defined(CONFIG_EMMCBOOT)
#define CONFIG_ENV_IS_IN_MMC		1
#define CONFIG_SYS_MMC_ENV_DEV		0
#define CONFIG_ENV_OFFSET		0x00200 /* skip MBR */
#define CONFIG_ENV_SIZE			0x40000 /* 256k */
#else
#define CONFIG_ENV_IS_NOWHERE
#endif

/* I2C settings */
#define CONFIG_HARD_I2C
#define CONFIG_PNX8181_I2C
#define CONFIG_PNX8181_I2C_ADDRESS_1	0x06200000
#define CONFIG_PNX8181_I2C_ADDRESS_2	0x06300000
#define CONFIG_PNX8181_I2C_BUS_MAX 2
#define CONFIG_SYS_I2C_SPEED	100000
#define CONFIG_SYS_I2C_SLAVE	0xfe
#define CONFIG_CMD_I2C
#define CONFIG_I2C_MULTI_BUS

#define CONFIG_BOOTDELAY		4
#define CONFIG_BOOTABORTKEY		0x1b /* ascii for escape */
#define CONFIG_TFTP_PORT		1

#define CONFIG_NET_RETRY_COUNT		7

#ifdef CONFIG_DMW_LCDCTYPE
#define LCD_ENV_SETTINGS "lcdctype=" CONFIG_DMW_LCDCTYPE "\0"
#else
#define LCD_ENV_SETTINGS
#endif

#ifdef CONFIG_EMMCBOOT
	#define CONFIG_BOOTARGS "console=ttyS0 root=/dev/mmcblk0p5 rootwait"
	#define BOOTMEDIUM_ENV_SETTINGS \
		"loadkernel=disk read ${loadaddr} mmc 0:3\0"                  \
		"chk_parts=true\0"                                            \
		"dfu_manifest_mmcblk0p1=disk write $loadaddr mmc 0:1 $filesize\0" \
		"dfu_manifest_mmcblk0p2=disk write $loadaddr mmc 0:2 $filesize\0" \
		"dfu_manifest_mmcblk0p3=disk write $loadaddr mmc 0:3 $filesize\0" \
		"u-boot-upgrade=disk write ${loadaddr} mmc 0:2 "              \
			"&& save "                                            \
			"&& echo Done. Please do a power cycle to reset board\0"
	#define SDCARD_NUM "1"
#else
	#define CONFIG_BOOTARGS \
		"console=ttyS0 ubi.mtd=rootfs root=ubi0:rootfs rootfstype=ubifs"
	#define BOOTMEDIUM_ENV_SETTINGS \
		"loadkernel=nand read ${loadaddr} kernel\0"                   \
		"chk_parts=test -n ${mtdparts} || mtdparts default\0"         \
		"dfu_manifest_u-boot-env=run dfu_manifest_2\0"                \
		"dfu_manifest_2=nand erase.part u-boot-env "                  \
			"&& nand write ${loadaddr} u-boot-env "               \
			"&& reset\0"                                          \
		"u-boot-upgrade=run chk_parts; "                              \
			"nand erase.part u-boot "                             \
			"&& nand write ${loadaddr} u-boot "                   \
			"&& save "                                            \
			"&& echo Done. Please do a power cycle to reset board\0"
	#define SDCARD_NUM "0"
#endif

#define CONFIG_BOOTCOMMAND \
	"if mmc rescan " SDCARD_NUM "; then " \
		"if run loadbootscript; then " \
			"echo \"Boot script found on SD card. Do 'run bootscript' to execute\"; " \
		"else " \
			"run test1; " \
		"fi; " \
	"else run test1; fi"

#ifdef DW_USBTTY_AS_DEFAULT
#define USBTTY_ENV_SETTINGS \
	"stdin=usbtty\0"  \
	"stdout=usbtty\0" \
	"stderr=usbtty\0"
#else
#define USBTTY_ENV_SETTINGS
#endif

/**
 * define keypad row/column for next and enter key
 */
#ifdef CONFIG_BOOTMENU
#define MENU_ENV_SETTINGS \
	"menu_next_keypad_row=2\0"		\
	"menu_next_keypad_col=0\0"		\
	"menu_enter_keypad_row=3\0"		\
	"menu_enter_keypad_col=0\0"		\
	"menu_next_key_text=Press [Back]\0"	\
	"menu_enter_key_text=[Menu]\0"		\
	"bootmenu_on=0\0"
#else
#define MENU_ENV_SETTINGS
#endif

#ifdef CONFIG_DMW_BOARDFEATURES
#define BOARD_ENV_SETTINGS "board=" CONFIG_DMW_BOARDID ":" CONFIG_DMW_BOARDFEATURES "\0"
#else
#define BOARD_ENV_SETTINGS "board=" CONFIG_DMW_BOARDID "\0"
#endif

#define CONFIG_EXTRA_ENV_SETTINGS \
	"test1=if test $ethaddr != ; then run linux_boot; else "              \
	        "echo ethaddr not found!; fi\0"                               \
	"loadbootscript=fatload mmc " SDCARD_NUM " ${loadaddr} boot.scr\0"    \
	"bootscript=echo Running bootscript from mmc ...; "                   \
		"source ${loadaddr}\0"                                        \
	"linux_boot=run chk_parts; run loadkernel && run bootm\0"             \
	"recovery=if mmc rescan " SDCARD_NUM "; then if run loadbootscript;"  \
             "then run bootscript; else run test1; fi; else run test1; fi\0"  \
	"norupgrade=protect off all && "                                      \
		"erase all && "                                               \
		"cp.b ${loadaddr} 0x01100000 0x10000 && "                     \
		"echo Done. Please do a power cycle to reset board\0"         \
	"dfu_manifest=run ${bootm}\0"                                         \
	"bootm=set bootargs ${bootargs} board=${board} ${mtdparts} "          \
	        "dp52.calibration=${dp52calibration} "                        \
	        "dp52-battery.spec=${dp52bat} && "                            \
		"bootm\0"                                                     \
	LCD_ENV_SETTINGS                                                      \
	BOOTMEDIUM_ENV_SETTINGS                                               \
	USBTTY_ENV_SETTINGS                                                   \
	MENU_ENV_SETTINGS                                                     \
	BOARD_ENV_SETTINGS

/*
 * Static configuration when assigning fixed address
 */
#define CONFIG_ETHADDR			54:89:07:9B:8A:0D
#define CONFIG_NETMASK			255.255.255.0
#define CONFIG_IPADDR			192.168.0.240
#define CONFIG_SERVERIP			192.168.0.1
#define CONFIG_BOOTFILE			uImage
#define CONFIG_LOADADDR			CONFIG_SYS_LOAD_ADDR

/*
 * Flash configuration
 */
#ifdef CONFIG_NANDBOOT
#define CONFIG_NAND_DMW96
#define CONFIG_CMD_NAND
#define CONFIG_CMD_MTDPARTS
#define CONFIG_CMD_MTDPROTECT
#define CONFIG_CMD_UBI
#define CONFIG_CMD_UBIFS
#define CONFIG_MTD_DEVICE
#define CONFIG_MTD_PARTITIONS
#define CONFIG_SYS_NAND_BASE		0x05400000
#define CONFIG_SYS_MAX_NAND_DEVICE	1
#define NAND_MAX_CHIPS			1 /* needed for u-boot bbt */
#endif

/*
 * Miscellaneous configurable options
 */
#define CONFIG_SYS_LONGHELP		/* undef to save memory */
#define CONFIG_SYS_PROMPT		"DMW # "
#define CONFIG_SYS_CBSIZE		1024

/* Print Buffer Size */
#define CONFIG_SYS_PBSIZE		(CONFIG_SYS_CBSIZE + \
					 sizeof(CONFIG_SYS_PROMPT) + 16)
/* max number of command args */
#define CONFIG_SYS_MAXARGS		64
/* boot argument buffer size */
#define CONFIG_SYS_BARGSIZE		CONFIG_SYS_CBSIZE

#define CONFIG_RECOVERY

#endif /* __CONFIG_DMW_H */

