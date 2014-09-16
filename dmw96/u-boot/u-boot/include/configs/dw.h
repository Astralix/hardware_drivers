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
 * (C) Copyright 2006-2007
 * Oded Golombek, DSP Group, odedg@dsp.co.il
 * Configuration for DSPG-DW
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

#define BOARD_LATE_INIT			1
#define CONFIG_BOOTMENU
#define CFG_PBSIZE			200

#define _DEBUG

#define CONFIG_ARM926EJS		1
#define CONFIG_DSPG_DW			1
#define CONFIG_ARCH_DSPG_DW		1

#define CONFIG_DISPLAY_CPUINFO		1
#define CONFIG_DISPLAY_BOARDINFO	1

#define CONFIG_VERSION_VARIABLE		1
#define CONFIG_IDENT_STRING		""

#define CONFIG_SYS_SDRAM_BASE		0x60000000

#define CONFIG_SYS_MEMTEST_START	0x100000
#define CONFIG_SYS_MEMTEST_END		0x10000000

#define mSEC_1				1000
#define mSEC_10				(mSEC_1 * 10)

/* TICK_RATE is 5MHz */
#define DW_TIMER_CLOCK			80000000
#define DW_TIMER_PRESCALE		16
#define DW_TIMER_TICK_RATE		(DW_TIMER_CLOCK / DW_TIMER_PRESCALE)
#define TICKS_PER_mSEC			(DW_TIMER_TICK_RATE / 1000)
#define TICKS_PER_uSEC			(DW_TIMER_TICK_RATE / 1000000)
#define TICKS2USECS(x)			((x) / TICKS_PER_uSEC)
#define TICKS2MSECS(x)			((x) / TICKS_PER_mSEC)
#define CFG_TIMER_RELOAD		0xFFFFFFFF /* load max value to track time */
#define CFG_DW_TIMER_CONTROL		(0x80 | (1 << 2) | (1 << 1)) /* Timer Enable, Timer Prescale divide by 16, Timer Size 32 bit*/
#define STANDALONE_DW
#define CFG_TIMERBASE			DW_TIMER0_1_BASE
#define CONFIG_SYS_HZ			DW_TIMER_TICK_RATE
#define TIMER_CTRL			CFG_DW_TIMER_CONTROL

#define READ_TIMER			readl(CFG_TIMERBASE + 4)

#define CONFIG_CMDLINE_TAG		1 /* pass cmdline  */
#define CONFIG_REVISION_TAG		1 /* pass cpu revision */
#define CONFIG_SETUP_MEMORY_TAGS	1
#define CONFIG_MISC_INIT_R		1 /* call misc_init_r during start up */

#define CONFIG_VIDEO
#define CONFIG_CFB_CONSOLE
#define CONFIG_VGA_AS_SINGLE_DEVICE

#define VIDEO_KBD_INIT_FCT	0
#define VIDEO_TSTC_FCT		serial_tstc
#define VIDEO_GETC_FCT		serial_getc

/* #define VIDEO_FB_LITTLE_ENDIAN */
/* #define VIDEO_FB_16BPP_PIXEL_SWAP */
#define VIDEO_FB_16BPP_WORD_SWAP

/*
 * Hardware drivers
 */
#define CONFIG_NET_MULTI		1
#define CONFIG_DRIVER_MACB /* Enable MACB ethernet driver */

#define CONFIG_MMC
#define CONFIG_DOS_PARTITION
#define CONFIG_GENERIC_MMC
#define CONFIG_SYNDWC_MMC
#define CONFIG_SYS_SYNDWC_BASE		0x09A00000

/* Definitions for MACB etherenet driver */
#define MACB_IO_ADDR			DW_MACA_BASE
#define MACB_IO_ADDR0			MACB_IO_ADDR

/*
 * NS16550 Configuration
 */
#define CFG_DSPG_DW_SERIAL

#define CONFIG_CMDLINE_EDITING		1
#define CONFIG_AUTO_COMPLETE		1
#define CONFIG_MX_CYCLIC		1

#define CONFIG_CONS_INDEX		0

#define CONFIG_BAUDRATE			115200
#define CONFIG_SYS_BAUDRATE_TABLE	{ 9600, 19200, 38400, 57600, 115200 }
#define CONFIG_SYS_SERIAL0		0x101F1000
#define CONFIG_SYS_SERIAL1		0x101F2000

#define CONFIG_BOOTP_MASK		CONFIG_BOOTP_DEFAULT

#define	CONFIG_SYS_HUSH_PARSER		1
#define	CONFIG_SYS_PROMPT_HUSH_PS2	"> "

#define CONFIG_USBD_DFU
#ifdef CONFIG_USBD_DFU
  #define CONFIG_USBD_DFUF			1
  #ifdef CONFIG_USBD_DFUF
    #define CONFIG_USBD_DFUF_SI			1
  #else
    #undef  CONFIG_USBD_DFUF_SI
  #endif

  #define CONFIG_USB_DEVICE			1
  #define CONFIG_USB_TTY			1
  #define CONFIG_SYS_CONSOLE_IS_IN_ENV		1
  #ifndef CONFIG_NO_MUSB
    #define CONFIG_MUSB				1 /* Enable USB driver*/
  #endif
  #define CONFIG_USBD_VENDORID			0x1d50    /* Linux/NetChip */
  #define CONFIG_USBD_PRODUCTID_GSERIAL		0x5120    /* gserial */
  #define CONFIG_USBD_PRODUCTID_CDCACM		0x5119
  #define CONFIG_USBD_MANUFACTURER		"DSPG"
  #define CONFIG_USBD_PRODUCT_NAME		"DFU/USB"
  #define CONFIG_USBD_DFU_XFER_SIZE		4096
  #define CONFIG_USBD_DFU_INTERFACE		2
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
#define CONFIG_CMD_MTDPARTS
#define CONFIG_CMD_MTDPROTECT
#define CONFIG_CMD_REV
#define CONFIG_CMD_UBI
#define CONFIG_CMD_UBIFS
#define CONFIG_CMD_DPCALIBRATION
#define CONFIG_CMD_MMC
#define CONFIG_CMD_FAT
#define CONFIG_CMD_DEFENV

#define DEFAULT_MIN_OPERATION_MV 3700

#define CONFIG_LZO    /* needed for UBI */
#define CONFIG_RBTREE /* needed for UBI */

#ifdef CONFIG_ENV_IS_IN_FLASH
# define MTDIDS_DEFAULT		"nor0=dwflash.0,nand0=dwnand"
# define MTDPARTS_DEFAULT	"mtdparts="                                   \
					"dwflash.0:"                          \
						"384k(u-boot),"               \
						"128k(u-boot-env);"           \
					"dwnand:"                             \
						"1m(factory)ro,"              \
						"3m(kernel),"                 \
						"3m(kernel2),"                \
						"-(rootfs)"
#else /* CONFIG_ENV_IS_IN_NAND */
# define MTDIDS_DEFAULT		"nand0=dwnand"

#if DW_BOARD_ID == 7
# define MTDPARTS_DEFAULT	"mtdparts="                                   \
					"dwnand:"                             \
						"768k(u-boot),"               \
						"256k(u-boot-env),"           \
						"1m(factory)ro,"              \
						"3m(kernel),"                 \
						"3m(kernel2),"                \
						"2m(initrd),"                 \
						"40m(data),"                  \
						"-(rootfs)"

#else
# define MTDPARTS_DEFAULT	"mtdparts="                                   \
					"dwnand:"                             \
						"768k(u-boot),"               \
						"256k(u-boot-env),"           \
						"1m(factory)ro,"              \
						"3m(kernel),"                 \
						"3m(kernel2),"                \
						"-(rootfs)"
#endif

#endif


#ifdef DW_UBOOT_NAND_SUPPORT /* Config commands with nand support */
	#define CONFIG_CMD_NAND
#endif

#define CONFIG_BOOTDELAY		4
#define CONFIG_BOOTABORTKEY		0x1b /* ascii for escape */
#define CONFIG_TFTP_PORT		1

#define CONFIG_BOOTARGS "console=ttyDW0,115200n8 root=/dev/nfs ip=dhcp"
#define CONFIG_BOOTCOMMAND \
	"if mmc rescan 0; then " \
		"if run loadbootscript; then " \
			"run bootscript; " \
		"else " \
			"run test1; " \
		"fi; " \
	"else run test1; fi"


#define CONFIG_NET_RETRY_COUNT		7

#define CONFIG_ENV_OVERWRITE		1

#ifdef DW_USBTTY_AS_DEFAULT
#define USBTTY_ENV_SETTINGS \
	"stdin=usbtty\0"  \
	"stdout=usbtty\0" \
	"stderr=usbtty\0"
#else
#define USBTTY_ENV_SETTINGS
#endif

#ifdef CONFIG_ENV_IS_IN_FLASH
#define DFU_MANIFEST_UBOOT_ENV \
	"dfu_manifest_2=protect off all "                                     \
		"&& erase 0x3c080000 0x3c09FFFF "                             \
		"&& cp.b ${loadaddr} 0x3c080000 0x20000 "                     \
		"&& reset\0"                                                  \
	"dfu_manifest_u-boot-env=protect off all "                            \
		"&& erase 0x3c080000 0x3c09FFFF "                             \
		"&& cp.b ${loadaddr} 0x3c080000 0x20000 "                     \
		"&& reset\0"
#else
#define DFU_MANIFEST_UBOOT_ENV \
	"dfu_manifest_2=nand erase 0xc0000 0x40000 "                          \
		"&& nand write ${loadaddr} 0xc0000 0x40000 "                  \
		"&& reset\0"                                                  \
	"dfu_manifest_u-boot-env=nand erase 0xc0000 0x40000 "                 \
		"&& nand write ${loadaddr} 0xc0000 0x40000 "                  \
		"&& reset\0"
#endif

/**
 * define keypad row/column for next and enter key
 */
#ifdef CONFIG_BOOTMENU
#define MENU_NEXT_KEYPAD_ROW		"menu_next_keypad_row=1\0"
#define MENU_NEXT_KEYPAD_COL		"menu_next_keypad_col=4\0"
#define MENU_ENTER_KEYPAD_ROW		"menu_enter_keypad_row=1\0"
#define MENU_ENTER_KEYPAD_COL		"menu_enter_keypad_col=3\0"
#define MENU_NEXT_KEY_ACTION_STRING	"menu_next_key_text=Press [SWK1]\0"
#define MENU_ENTER_KEY_ACTION_STRING	"menu_enter_key_text=[SWK2]\0"
#define MENU_BOOTMENU_ON		"bootmenu_on=0\0"
#else
#define MENU_NEXT_KEYPAD_ROW
#define MENU_NEXT_KEYPAD_COL
#define MENU_ENTER_KEYPAD_ROW
#define MENU_ENTER_KEYPAD_COL
#define MENU_NEXT_KEY_ACTION_STRING
#define MENU_ENTER_KEY_ACTION_STRING
#define MENU_BOOTMENU_ON
#endif

#if DW_BOARD_ID == 7

#define CONFIG_EXTRA_ENV_SETTINGS                                             \
	USBTTY_ENV_SETTINGS                                                   \
	"loadaddr=0x62000000\0"                                               \
	"ethaddr0=00:50:C2:1B:4D:62\0"                                        \
	"wifiaddr=00:50:C2:1B:4D:63\0"                                        \
	"test1=if test $ethaddr0 != ; then run test2; else echo ethaddr0 "    \
		"not found! Use cfg_mac_addr to configure and type reset to " \
		"restart.; fi\0"                                              \
	"test2=if test $wifiaddr != ; then run test_datafs; "                 \
		"run test_boot_mode; run linux_boot; else echo "              \
		"wifiaddr not found! Use cfg_mac_addr to configure and type " \
		"reset to restart.; fi\0"                                     \
	"loadbootscript=fatload mmc 0 ${loadaddr} boot.scr\0"                 \
	"bootscript=echo Running bootscript from mmc ...; "                   \
		"source ${loadaddr}\0"                                        \
	"chk_mtdparts=test -n ${mtdparts} || mtdparts default\0"              \
	"nandupgrade=run chk_mtdparts; "                                      \
		"nand erase 0 0xc0000 "                                       \
		"&& nand write ${loadaddr} 0 0xc0000 "                        \
		"&& saveenv "                                                 \
		"&& echo Done. Please do a power cycle to reset board\0"      \
	"dfu_loadaddr=${loadaddr}\0"                                          \
	"dfu_manifest=set bootargs ${bootargs} "                              \
			"kona.wifiaddr=${wifiaddr} board=${board} "           \
			"${mtdparts} "                                        \
		"&& bootm ${loadaddr}\0"                                      \
	"dfu_manifest_u-boot=run nandupgrade\0"                               \
	"dfu_manifest_kernel=nand erase kernel; "                             \
		"nand erase kernel2; "                                        \
		"nand write.jffs2 0x62000000 kernel 0x200000; "               \
		"nand write.jffs2 0x62000000 kernel2 0x200000\0"              \
	"dfu_manifest_rootfs=nand erase rootfs; "                             \
		"ubi part rootfs; "                                           \
		"ubi create rootfs; "                                         \
		"ubi write 0x62000000 rootfs ${filesize}; "                   \
		"reset\0"                                                     \
	"dfu_manifest_initrd=nand erase initrd; "                             \
		"nand write.jffs2 0x62000000 initrd 0x200000;\0"              \
	"test_boot_mode=if test $boot_mode = do_upgrade ; then setenv "       \
		"linux_boot ${linux_boot_initrd}; else if test "              \
		"$boot_mode != normal; then run incr_fbc_first_boot; "        \
		"run chk_fbc; fi; fi\0"                                       \
	"incr_fbc_first_boot=if test $boot_mode = first_boot; then if "       \
		"test $first_boot_cnt = 0;then setenv first_boot_cnt 1; "     \
		"elif test $first_boot_cnt = 1; "                             \
		"then setenv first_boot_cnt 2; "                              \
		"fi; saveenv; fi\0"                                           \
	"chk_fbc=if test $boot_mode != upgrade_error; then if test "          \
		"$first_boot_cnt = 2; then setenv boot_mode upgrade_error; "  \
		"setenv linux_boot ${linux_boot_initrd}; fi; else setenv "    \
		"linux_boot ${linux_boot_initrd}; fi\0"                       \
	"boot_mode=normal\0"                                                  \
	"test_datafs=if test $create_datafs = 1; "                            \
		"then setenv create_datafs 0; "                               \
		"run create_datafs_partition ; saveenv; fi;\0"                \
	"create_datafs=1\0"                                                   \
	"create_datafs_partition=nand erase data; "                           \
		"ubi part data; "                                             \
		"ubi create data\0"                                           \
	"bootargs_nfs_usb=root=/dev/nfs nfsroot=10.0.0.1:/opt/arm/target "    \
		"console=ttyDW1,115200n8 ubi.mtd=data ubi.mtd=rootfs "        \
		"androidboot.console=ttyDW1\0"                                \
	"bootargs_nfs=root=/dev/nfs ip=dhcp console=ttyDW1,115200n8 "         \
		"ubi.mtd=data ubi.mtd=rootfs androidboot.console=ttyDW1\0"    \
	"bootargs_ubi=root=ubi1:rootfs rootfstype=ubifs "                     \
		"console=ttyDW1,115200n8 ubi.mtd=data ubi.mtd=rootfs "        \
		"androidboot.console=ttyDW1\0"                                \
	"bootargs_initrd=root=/dev/ram0 rw initrd=0x63000000,8M "             \
		"console=ttyDW1,115200n8 ubi.mtd=data ubi.mtd=rootfs\0"       \
	"linux_boot_nfs=bootp 0x62000000; "                                   \
		"set bootargs ${bootargs_nfs} macb.ethaddr0=${ethaddr0} "     \
			"kona.wifiaddr=${wifiaddr} board=${board_id} "        \
			"${mtdparts} ${initcmd} "                             \
			"calibrationmode=${calibrationmode} "                 \
			"cmpgain=${cmpgain} auxbgp=${auxbgp} "                \
			"cmpgain38=${cmpgain38} tempref=${tempref};"          \
		"bootm 0x62000000\0"                                          \
	"linux_boot_nfs_usb=bootp 0x62000000; "                               \
		"set bootargs ${bootargs_nfs_usb} ${usbethcmd} "              \
			"macb.ethaddr0=${ethaddr0} kona.wifiaddr=${wifiaddr} "\
			"board=${board_id} ${mtdparts} ${initcmd} "           \
			"calibrationmode=${calibrationmode} "                 \
			"cmpgain=${cmpgain} auxbgp=${auxbgp} "                \
			"cmpgain38=${cmpgain38} tempref=${tempref}; "         \
		"bootm 0x62000000\0"                                          \
	"linux_boot_ubi=nand_safe_bootm 0x62000000; "                         \
		"set bootargs ${bootargs_ubi} ${usbethcmd} "                  \
			"macb.ethaddr0=${ethaddr0} kona.wifiaddr=${wifiaddr} "\
			"board=${board_id} ${mtdparts} ${initcmd} "           \
			"calibrationmode=${calibrationmode} "                 \
			"cmpgain=${cmpgain} auxbgp=${auxbgp} "                \
			"cmpgain38=${cmpgain38} tempref=${tempref}; "         \
		"bootm 0x62000000\0"                                          \
	"linux_boot=nand_safe_bootm 0x62000000; "                             \
		"set bootargs ${bootargs_ubi} ${usbethcmd} "                  \
			"macb.ethaddr0=${ethaddr0} kona.wifiaddr=${wifiaddr} "\
			"board=${board_id} ${mtdparts} ${initcmd} "           \
			"calibrationmode=${calibrationmode} "                 \
			"cmpgain=${cmpgain} auxbgp=${auxbgp} "                \
			"cmpgain38=${cmpgain38} tempref=${tempref}; "         \
		"bootm 0x62000000\0"                                          \
	"linux_boot_initrd=nand_safe_bootm 0x62000000; "                      \
		"nand read 0x63000000 initrd 0x180000; "                      \
		"set bootargs ${bootargs_initrd} ${mtdparts} "                \
			"board=${board_id} init=/sbin/init; "                 \
		"bootm 0x62000000\0"                                          \
	"initcmd=init=/init\0"                                                \
	"setnormalmode=setenv calibrationmode off "                           \
		"&& setenv mtdparts ${mtdparts_normal} "                      \
		"&& setenv initcmd init=/init "                               \
		"&& saveenv "                                                 \
		"&& echo Boot Mode is: Normal\0"                              \
	"setcalibrationmode=setenv calibrationmode on "                       \
		"&& setenv mtdparts ${mtdparts_calibration} "                 \
		"&& setenv initcmd init=/init "                               \
		"&& saveenv "                                                 \
		"&& echo Boot Mode is: Calibration\0"                         \
	"calibrate_charger=echo Assuming 4200mV as battery power. "           \
		"&& echo Performing calibration... "                          \
		"&& setenv auxbgp "                                           \
		"&& setenv cmpgain "                                          \
		"&& dp_calibration 4200 "                                     \
		"&& dp_tunecmp "                                              \
		"&& saveenv\0"                                                \
	"calibrate_safe_charger=echo Assuming 4200mV as battery power. "      \
		"&& echo Assuming 1000mV in DCIN0. "                          \
		"&& echo Performing calibration... "                          \
		"&& setenv auxbgp "                                           \
		"&& setenv cmpgain "                                          \
		"&& setenv cmpgain38 "                                        \
		"&& setenv tempref "                                          \
		"&& dp_calibration 4200 "                                     \
		"&& dp_tunecmp "                                              \
		"&& dp_caltemp "                                              \
		"&& saveenv\0"                                                \
	"setnfsmode=setenv linux_boot ${linux_boot_nfs}\0"                    \
	"setnfsusbmode=setenv linux_boot ${linux_boot_nfs_usb}\0"             \
	"setubimode=setenv linux_boot ${linux_boot_ubi}\0"                    \
	"toggle_screen=1\0"                                                   \
	"backlight_en=1\0"                                                    \
	"kernel_active=1\0"                                                   \
	"usbethcmd=ip=10.0.0.2:10.0.0.1:10.0.0.1:255.255.255.0:imh:usb0:on\0" \
	"mtdparts=mtdparts=dwnand:768k(u-boot),256k(u-boot-env),"             \
		"1m(factory)ro,3m(kernel),3m(kernel2),2m(initrd),"            \
		"40m(data),-(rootfs)\0"                                       \
	"mtdparts_calibration=mtdparts=dwnand:768k(u-boot),256k(u-boot-env)," \
		"1m(factory),3m(kernel),3m(kernel2),2m(initrd),"              \
		"40m(data),-(rootfs)\0"                                       \
	"mtdparts_normal=mtdparts=dwnand:768k(u-boot),256k(u-boot-env),"      \
		"1m(factory)ro,3m(kernel),3m(kernel2),2m(initrd),"            \
		"40m(data),-(rootfs)\0"                                       \
	"mtdids=nand0=dwnand\0"                                               \
	"calibrationmode=off\0"                                               \
	"auxbgp=104\0"                                                        \
	"cmpgain=50\0"                                                        \
	"board_id=7\0"                                                        \
	"dfu_enter_timeout=3\0"                                               \
	"dfu_idle_timeout=30\0"                                               \
	"dfu_enter_keyboard_row=0\0"                                          \
	"dfu_enter_keyboard_col=0\0"                                          \
	"lcdctype=2\0"                                                        \
	MENU_NEXT_KEYPAD_ROW                                                  \
	MENU_NEXT_KEYPAD_COL                                                  \
	MENU_ENTER_KEYPAD_ROW                                                 \
	MENU_ENTER_KEYPAD_COL                                                 \
	MENU_NEXT_KEY_ACTION_STRING                                           \
	MENU_ENTER_KEY_ACTION_STRING                                          \
	MENU_BOOTMENU_ON                                                      \
	DFU_MANIFEST_UBOOT_ENV

#else

#define CONFIG_EXTRA_ENV_SETTINGS                                             \
	USBTTY_ENV_SETTINGS                                                   \
	"loadaddr=0x62000000\0"                                               \
	"test1=if test $ethaddr0 != ; then run test2; else echo ethaddr0 "    \
		"not found! Use cfg_mac_addr to configure and type reset to " \
		"restart.; fi\0"                                              \
	"test2=if test $wifiaddr != ; then run linux_boot; else echo "        \
		"wifiaddr not found! Use cfg_mac_addr to configure and type " \
		"reset to restart.; fi\0"                                     \
	"loadbootscript=fatload mmc 0 ${loadaddr} boot.scr\0"                 \
	"bootscript=echo Running bootscript from mmc ...; "                   \
		"source ${loadaddr}\0"                                        \
	"linux_boot=bootp 0x60200000 "                                        \
		"&& set bootargs ${bootargs} kona.wifiaddr=${wifiaddr} "      \
			"board=${board} ${mtdparts} "                         \
		"&& bootm 0x60200000\0"                                       \
	"chk_mtdparts=test -n ${mtdparts} || mtdparts default\0"              \
	"swupgrade=run chk_mtdparts; "                                        \
		"protect off all "                                            \
		"&& erase 0x3C000000 0x3C07FFFF "                             \
		"&& cp.b ${loadaddr} 0x3C000000 0x80000 "                     \
		"&& save "                                                    \
		"&& echo Done. Please do a power cycle to reset board\0"      \
	"nandupgrade=run chk_mtdparts; "                                      \
		"nand erase 0 0xc0000 "                                       \
		"&& nand write ${loadaddr} 0 0xc0000 "                        \
		"&& save "                                                    \
		"&& echo Done. Please do a power cycle to reset board\0"      \
	"dfu_loadaddr=${loadaddr}\0"                                          \
	"dfu_manifest=set bootargs ${bootargs} "                              \
			"kona.wifiaddr=${wifiaddr} board=${board} "           \
			"${mtdparts} "                                        \
		"&& bootm ${loadaddr}\0"                                      \
	MENU_NEXT_KEYPAD_ROW                                                  \
	MENU_NEXT_KEYPAD_COL                                                  \
	MENU_ENTER_KEYPAD_ROW                                                 \
	MENU_ENTER_KEYPAD_COL                                                 \
	MENU_NEXT_KEY_ACTION_STRING                                           \
	MENU_ENTER_KEY_ACTION_STRING                                          \
	MENU_BOOTMENU_ON                                                      \
	DFU_MANIFEST_UBOOT_ENV

#endif

/*
 * Static configuration when assigning fixed address
 */
/*
#define CONFIG_NETMASK			255.255.255.0
#define CONFIG_IPADDR				192.168.0.240
#define CONFIG_SERVERIP			192.168.0.151
#define CONFIG_BOOTFILE			"dw.kernel"
*/

/*
 * Stack sizes - set up in start.S using sizes below
 */
#define CONFIG_STACKSIZE		(128*1024)	/* regular stack */

/*
 * Physical Memory Map
 */
#define CONFIG_NR_DRAM_BANKS		2
#define PHYS_SDRAM_1			0x60000000	/* SDRAM Bank #1 */
#define PHYS_SDRAM_2			0x34000000	/* SDRAM Bank #2 */
#define DW_SDRAM_BANK1_CS		MPMC_CHIPSELECT_5
#define DW_SDRAM_BANK2_CS		MPMC_CHIPSELECT_4
#ifdef DW_EXPEDITOR_SMALL_NOR_FLASH
 #define PHYS_FLASH_SIZE		0x80000		/* 0.5 MB */
#else
 #define PHYS_FLASH_SIZE		0x02000000	/* 32MB */
#endif

/*
 * FLASH and environment organization
 */
#define CONFIG_MTD_DEVICE
#define CONFIG_MTD_PARTITIONS
/* setenv mtdids "nor0=dwflash.0,nand0=dwnand" */
/* setenv mtdparts "mtdparts=dwflash.0:384k(u-boot),128k(u-boot-env),3m(kernel),-(rootfs1);dwnand:3m(kernel1),3m(kernel2),-(rootfs2)"*/
/*                                 “dw:384k(u-boot),128k(u-boot-env),1536(kernel),-(rootfs)” */
#ifdef DW_UBOOT_NAND_SUPPORT
  #define CONFIG_MTD_NAND_DW_DMA_THROUGH_SSRAM 1
  #define CONFIG_SYS_NAND_BASE		0x05400000
  #define CONFIG_SYS_MAX_NAND_DEVICE	1
  #define NAND_MAX_CHIPS		1		// needed for u-boot bbt
  #define CONFIG_SYS_64BIT_VSPRINTF	1		// fix nand_util warning
#endif

/*
 * Use the CFI flash driver for ease of use
 */
#define CONFIG_FLASH_CFI_MTD		1
#define CONFIG_FLASH_CFI_DRIVER		1
#define CONFIG_SYS_FLASH_CFI		1
#define CONFIG_SYS_FLASH_USE_BUFFER_WRITE 1
/* 
 *	System control register
 */
#define VERSATILE_SYS_BASE		0x10000000
#define VERSATILE_SYS_FLASH_OFFSET	0x4C
#define VERSATILE_FLASHCTRL		(VERSATILE_SYS_BASE + VERSATILE_SYS_FLASH_OFFSET)
#define VERSATILE_FLASHPROG_FLVPPEN	(1 << 0)	/* Enable writing to flash */

/* timeout values are in ticks */
#define CFG_FLASH_ERASE_TOUT		(2*CONFIG_SYS_HZ)	/* Timeout for Flash Erase */
#define CFG_FLASH_WRITE_TOUT		(2*CONFIG_SYS_HZ)	/* Timeout for Flash Write */

/*
 * Note that CONFIG_SYS_MAX_FLASH_SECT allows for a parameter block 
 * i.e. the bottom "sector" (bottom boot), or top "sector" 
 *      (top boot), is a seperate erase region divided into 
 *      4 (equal) smaller sectors. This, notionally, allows
 *      quicker erase/rewrire of the most frequently changed
 *      area......
 *      CONFIG_SYS_MAX_FLASH_SECT is padded up to a multiple of 4
 */

#define FLASH_SECTOR_SIZE		0x00020000	/* 128 KB sectors */
#define CONFIG_SYS_MAX_FLASH_SECT	(520)

#define CONFIG_SYS_FLASH_BASE		0x3C000000
#define CONFIG_SYS_MAX_FLASH_BANKS	1		/* max memory banks */

#ifdef CONFIG_ENV_IS_IN_FLASH
#define CONFIG_ENV_SECT_SIZE		FLASH_SECTOR_SIZE
#define CONFIG_ENV_SIZE			0x20000
#define CONFIG_ENV_OFFSET		0x80000
#define CONFIG_ENV_ADDR			(CONFIG_SYS_FLASH_BASE + CONFIG_ENV_OFFSET)
#else
#define CONFIG_ENV_SECT_SIZE		0x40000
#define CONFIG_ENV_SIZE			0x40000
#define CONFIG_ENV_OFFSET		0xc0000
#endif

#define CONFIG_SYS_FLASH_PROTECTION	/* The devices have real protection */
#define CONFIG_SYS_FLASH_EMPTY_INFO	/* flinfo indicates empty blocks */

/*
 * Size of malloc() pool
 */
#define CONFIG_SYS_MALLOC_LEN		(2 * 1024 * 1024)
#define CONFIG_SYS_GBL_DATA_SIZE	128 /* bytes reservd for initial data */


#define CONFIG_FLASH_SHOW_PROGRESS	45

/*
 * Miscellaneous configurable options
 */
#define CONFIG_SYS_LONGHELP		/* undef to save memory */
#define CONFIG_SYS_PROMPT		"DW # "
#define CONFIG_SYS_CBSIZE		1024

/* Print Buffer Size */
#define CONFIG_SYS_PBSIZE		(CONFIG_SYS_CBSIZE + \
						sizeof(CONFIG_SYS_PROMPT) + 16)
/* max number of command args */
#define CONFIG_SYS_MAXARGS		64
/* boot argument buffer size */
#define CONFIG_SYS_BARGSIZE		CONFIG_SYS_CBSIZE

#undef	CFG_CLKS_IN_HZ			/* everything, incl board info, in Hz */
#define CONFIG_SYS_LOAD_ADDR		(PHYS_SDRAM_1 + (2<<20))

#define CONFIG_VIBRATOR			1

#ifndef __ASSEMBLY__
/* Enum for different MAC addresses stored in DW eeprom */
enum {
	DW_MACID_MACB0,
	DW_MACID_MACB1,
	DW_MACID_WIFI,
	DW_MACID_RESERVED
};
unsigned char* dw_get_mac(int mac_id);
#endif

#define DSPG_DW_ENV_MTDPARTS_NAME           "mtdparts"
#define DSPG_DW_ENV_ROOTFS_NAME             "rootfs"
#define DSPG_DW_ENV_ROOTFS2_NAME            "rootfs2"
#define DSPG_DW_NOR_KEREL_ACTIVE_ENV_NAME   "kernel_active"
#define DSPG_DW_NOR_ROOTFS_ACTIVE_ENV_NAME  "rootfs_active"
#define DSPG_DW_ENV_ROOTFS_VERIFY_NAME      "rootfs_verify"

#endif /* __CONFIG_H */

