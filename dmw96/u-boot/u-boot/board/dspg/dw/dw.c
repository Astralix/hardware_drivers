/*
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * (C) Copyright 2002
 * David Mueller, ELSOFT AG, <d.mueller@elsoft.ch>
 *
 * (C) Copyright 2003
 * Texas Instruments, <www.ti.com>
 * Kshitij Gupta <Kshitij@ti.com>
 *
 * (C) Copyright 2004
 * ARM Ltd.
 * Philippe Robin, <philippe.robin@arm.com>
 *
 * (C) Copyright 2006-2007
 * Oded Golombek, DSP Group, odedg@dsp.co.il
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <command.h>
#include <environment.h>
#include <asm/byteorder.h>
#include <asm/io.h>

#include <image.h>
#include <u-boot/zlib.h>
#include <video_fb.h>

/* for software upgrade */
#include <jffs2/jffs2.h>

#ifdef CONFIG_USB_DEVICE
#include <usbdevice.h>
#ifndef CONFIG_MUSB
#include <usbdcore_dspg.h>
#endif
#include <dspg_udc.h>
#endif
#include <syndwc_mmc.h>

#include "dw.h"
#include "clock.h"
#include "gpio.h"
#include "spi.h"
#include "dp52.h"
#include "charger.h"
#include "keypad.h"

#ifdef CONFIG_BOOTMENU
#include "bootmenu.h"
#endif


#define ADJUST_VALUE_RESET		0x66
#define DP_DEFAULT_SAMPLERATE		0x7
// MACRO to change measurement to mVolt value


DECLARE_GLOBAL_DATA_PTR;
unsigned int sdram_size = 0;

void timer__init (void);
void mpmc__init (int bus_width);
void udc__init(void);
void set_backlight(int onoff);
static int hatoi(char *p);

void peripheral_power_enable (void);

void dw_init_i2c_eeprom(void);
unsigned char dw_read_i2c_eeprom(unsigned char dev_id, unsigned short addr);
void dw_read_seq_i2c_eeprom(unsigned char dev_id, unsigned short addr);

unsigned long board_flash_get_legacy(unsigned long base, int banknum, flash_info_t *info)
{
	return 0;
}

#ifdef DW_UBOOT_NAND_SUPPORT
//#include <linux/mtd/nand.h>
//int board_nand_init(struct nand_chip *nand);
#endif

#ifdef CONFIG_DRIVER_MACB
int dw_macb_initialize(void);
#endif

int board_eth_init(bd_t *bis)
{
#ifdef CONFIG_DRIVER_MACB
	clk_enable("ethmaca");

	return dw_macb_initialize();
#endif
	return 0;
}

int dw_chip = DW_CHIP_UNKNOWN;
int dw_board = DW_BOARD_UNKNOWN;

#define mdelay(n)	udelay((n)*1000)

#if defined(CONFIG_SHOW_BOOT_PROGRESS)
void show_boot_progress(int progress)
{
	printf("Boot reached stage %d\n", progress);
}
#endif

#define COMP_MODE_ENABLE ((unsigned int)0x0000EAEF)

static inline void delay (unsigned long loops)
{
	__asm__ volatile ("adds %0, %1, #1\n"
		"1:\n"
		"subs %0, %1, #1\n"
		"bne 1b":"=r" (loops):"0" (loops));
}

static struct
{
	unsigned int magic_number;
	unsigned char version[2];
	unsigned char burn_date[14];
	unsigned char board_version[2];
	unsigned int board_serial;
	unsigned char eth0_mac[6];
	unsigned char eth1_mac[6];
	unsigned char wifi_mac[6];
	unsigned char reserved_mac[6];
	unsigned char padding[10];
	unsigned int crc32;
} __attribute__((__packed__)) dw_eeprom_record;
static int dw_eeprom_valid = 0;

int dw_detect_board(void)
{
	return dw_board;
}

static int dw_detect_chip(void)
{
	int chip, chip_id;

	chip_id = readl(DW_SYSCFG_BASE + DW_SYSCFG_CHIP_ID);

	switch (chip_id) {
	case 0x5200:
		chip = 52; break;
	case 0x0740:
		chip = 74; break;
	default:
		chip = -1;
	}

	return chip;
}

static int dw_chip_revision(void)
{
	/* determine chip revision by comparing values in the bootrom */
	if (readl(0x10000000) == 0xea000014)
		return 0;

	if (readl(0x10000050) == 0x0000a2d0)
		return 1;

	return 2;
}

#define MPMCDynamicConfigOffset (MPMCDynamicConfig1 - MPMCDynamicConfig0)
static unsigned int dw_get_ramsize(enum MPMC_CHIPSELECT cs)
{
	u32 reg;
	u32 conf_offset = (cs - 4) * MPMCDynamicConfigOffset;

	/* extract configured sdram size from mpmc */
	reg = readl(MPMC_BASE + MPMCDynamicConfig0 + conf_offset);
	reg = (reg & 0x0F80) >> 7;

	/*
	 * reg now contains bits [11:7] shifted to the left
	 *
	 * values can be compared to table 3-21 (address mapping) of the
	 * PrimeCell MultiPort Memory Controller (PL175) Technical Reference
	 * Manual
	 */
	switch (reg) {
	case 0x0D:
		return (64 << 20);
	case 0x11:
		return (128 << 20);
	}

	return 0;
}

/*****************************************************************************/

void board_spi_init(void)
{
	unsigned int tmp;

	if (dw_detect_chip() != DW_CHIP_DW74)
		return;

	clk_enable("spi");

	tmp = readl(DW_SYSCFG_BASE + DW_SYSCFG_IO_LOC);
	tmp &= ~(1 << 4); /* SPI_LOC=0 */
	writel(tmp, DW_SYSCFG_BASE + DW_SYSCFG_IO_LOC);

	gpio_disable(AGPIO(23)); /* SDO */
	gpio_disable(AGPIO(24)); /* SDI */
	gpio_disable(AGPIO(27)); /* SCLK */

	gpio_enable(AGPIO(25));  /* CS1 */
	gpio_enable(AGPIO(26));  /* CS2 */
	gpio_direction_output(AGPIO(25), 1);
	gpio_direction_output(AGPIO(26), 1);

	writel(0x012A, DW_SPI_CFG);
}

/*****************************************************************************/

int do_dp_write(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	unsigned int reg;
	unsigned int val;

	if (argc != 3)
		return -1;

	reg = simple_strtoul(argv[1], NULL, 16);
	val = simple_strtoul(argv[2], NULL, 16);

	dp52_direct_write(reg, val);

	return 0;
}

U_BOOT_CMD(dp_write,	3,	1,	do_dp_write,
	"dp_write - write value to register of DP52\n",
	"register value"
);

int do_dp_read(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	unsigned int reg;
	unsigned int val;

	if (argc != 2)
		return -1;

	reg = simple_strtoul(argv[1], NULL, 16);

	val = dp52_direct_read(reg);

	printf("0x%x\n", val);

	return 0;
}

U_BOOT_CMD(dp_read,	2,	1,	do_dp_read,
	"dp_read - read register from DP52\n",
	"register"
);

/*************************************************************************/

#ifdef CONFIG_CMD_DPCALIBRATION

unsigned int get_auxbgp(void)
{
	unsigned int auxbgp = 0;
	char *str_auxbgp;

	str_auxbgp = getenv("auxbgp");
	if (str_auxbgp) auxbgp = simple_strtoul(str_auxbgp, NULL, 10);
	printf("auxbgp = %d\n", auxbgp);

	return auxbgp;
}

unsigned int get_cmpgain(void)
{
	unsigned int cmpgain = 0;
	char *str_cmpgain;

	str_cmpgain = getenv("cmpgain");
	if (str_cmpgain) cmpgain = simple_strtoul(str_cmpgain, NULL, 10);
	printf("cmpgain = %d\n", cmpgain);

	return cmpgain;
}

unsigned int get_cmpgain38(void)
{
	unsigned int cmpgain38 = 0;
	char *str_cmpgain38;

	str_cmpgain38 = getenv("cmpgain38");
	if (str_cmpgain38) cmpgain38 = simple_strtoul(str_cmpgain38, NULL, 10);
	printf("cmpgain38 = %d\n", cmpgain38);

	return cmpgain38;
}

unsigned int get_tempref(void)
{
	unsigned int tempref = 0;
	char *str_tempref;

	str_tempref = getenv("tempref");
	if (str_tempref) tempref = simple_strtoul(str_tempref, NULL, 10);
	printf("tempref = %d\n", tempref);

	return tempref;
}

/* get_min_mv:
 * returns the minimum voltage required to boot from the variable 'min_mv' (in millivolts).
 * If the var 'min_mv' is missing the default value DEFAULT_MIN_OPERATION_MV will be returned.
 */
unsigned int get_min_mv(void)
{
	unsigned int min_mv = DEFAULT_MIN_OPERATION_MV;		// the default
	char *str_min_mv;

	str_min_mv = getenv("min_mv");
	if (str_min_mv) min_mv = simple_strtoul(str_min_mv, NULL, 10);
	printf("min_mv = %d\n", min_mv);

	return min_mv;
}

void bandgap_calibrate(u16 refVoltage)
{
	u16 auxbgp;
	int minBandgap = 0;
	int maxBandgap = 255;
	int delta;
	int currentReading;
	char strBandgap[10];
	u16 cmpgain = get_cmpgain();

	// Binary search
	do {
		auxbgp = (minBandgap + maxBandgap) / 2;

		// Read voltage
		currentReading = get_mVoltage(auxbgp, cmpgain);

		printf("bandgap 0x%x voltage %i\n", auxbgp, currentReading);

		if (refVoltage < currentReading)
			minBandgap = auxbgp+1;
		else
			maxBandgap = auxbgp-1;
	} while ((refVoltage != currentReading) && (minBandgap <= maxBandgap));

	delta = refVoltage - currentReading;
	if (delta > 20 || delta < -20) {
		// The delta is too big
		printf("calibration error: could not perform bandgap calibration, wrong reading from DCIN3\n");
		return;
	}

	sprintf (strBandgap, "%i", auxbgp);
	setenv("auxbgp", strBandgap);
	printf("setting auxbgp = %s\n to adopt permanently type \"saveenv\"\n", strBandgap);
}

/* calibrate temperature */
int do_dp_caltemp( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[] )
{
	int reading;
	char tempref_str[10];
	u16 auxbgp;

	auxbgp = get_auxbgp();

	/* calibration is reading DCIN0 with a 1.00v reference in place of the NTC */
	reading = get_batt_ntc(auxbgp);

	/* store */
	sprintf(tempref_str, "%i", reading);
	setenv("tempref", tempref_str);

	printf("setting tempref = %s\n", tempref_str);

	return 0;
}

/* calculate comparator attenuation for 3.8v triggering; value we receive is 4.2v setting */
unsigned int calc_38v_atten(unsigned int atten)
{
	float resistor_ratio;
	float real_atten38;
	unsigned int atten38;

	/* calculate resistor ratio */
	resistor_ratio = 256.0f / (4.2f * (atten+193));
	printf("[%s] resistor_ratio*10000 = %ld\n", __func__, (unsigned long)(resistor_ratio*10000));

	/* calculate attenuation for 3.8V using 1.25 as the gain */
	real_atten38 = (1.00f / (resistor_ratio * 3.8f * 1.25f));
	printf("[%s] real_atten38*1000 = %ld\n", __func__, (unsigned long)(real_atten38*1000));

	/* compute the register value to get real_atten38 attenuation */
	atten38 = (unsigned int)(256*(real_atten38*100-75)/100-1);

	printf("[%s] atten38 = %d\n", __func__,  atten38);

	return atten38;
}

int do_tunecomperator(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]){

	volatile unsigned short i, prev_reading, current_reading, comp_calibration_value42, comp_calibration_value38, comp3_status, reg_value = 0;
	char strcmpgain42[10];
	char strcmpgain38[10];
	u16 value;

	current_reading = 0;
	prev_reading = 0;
	comp_calibration_value42 = 0;

	dp52_direct_write(DP52_PMU_EN_SW, 0x7ac);

	value = 0x83;
	value |= dp52_direct_read(DP52_AUX_EN);
	dp52_direct_write(DP52_AUX_EN, value);

	dp52_direct_write(DP52_AUX_DC3GAINCNT1, 0x001);

	for (i=0;i<64;i++) 
	{
		reg_value = (i << 5) + 1;
		dp52_direct_write(DP52_AUX_DC3GAINCNT1, reg_value);
		mdelay(5);
		
		comp3_status = dp52_direct_read(DP52_AUX_RAWPMINTSTAT) & 0x8;

		if (comp3_status > 0) {
			comp_calibration_value42 = i;
			break;
		}
	}

	reg_value = (comp_calibration_value42 << 5) + 1;

	dp52_direct_write(DP52_AUX_DC3GAINCNT1, reg_value);

	/* compute comperator attenuation value for 3.8v based on the comperator attenuation value for 4.2v */
	comp_calibration_value38 = calc_38v_atten(comp_calibration_value42);

	sprintf (strcmpgain42, "%i", comp_calibration_value42);
	sprintf (strcmpgain38, "%i", comp_calibration_value38);
	setenv("cmpgain", strcmpgain42);
	setenv("cmpgain38", strcmpgain38);

	printf("\n\ncmpgain = %s\n", strcmpgain42);
	printf("cmpgain38 = %s\n", strcmpgain38);
	printf("To adopt permanently type \"saveenv\"\n", strcmpgain42);

	return 0;
}

void dspg_dpm_idle(unsigned int, unsigned int, unsigned int, unsigned int, unsigned int);

int do_fastcharge(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	unsigned int auxbgp, cmpgain, cmpgain38, tempref, min_mv;

	auxbgp = get_auxbgp();
	cmpgain = get_cmpgain();
	cmpgain38 = get_cmpgain38();
	tempref = get_tempref();
	min_mv = get_min_mv();

#ifdef RUN_CHARGER_FROM_SRAM
	mdelay(500);
	dspg_dpm_idle(auxbgp, cmpgain, cmpgain38, tempref, min_mv);
#else
	do_dpm_fastcharge(auxbgp, cmpgain, cmpgain38, tempref, min_mv);
#endif

	return 0;
}

/* dp_board_up:
 * If board was up due to external power connection or
 * battery voltage is below DEFAULT_MIN_OPERATION_MV or 'min_mv' env. variable - start charger.
 * else - return
 */
int dp_board_up(void)
{
	u16 val;

	// get PMURAWSTAT and check if external power is connected
	val = dp52_direct_read(DP52_PMURAWSTAT);

	// if external power is connected start charging (do not boot the kernel)
	if (val & (1 << 1)) {
		printf("External power is connected (0x%x). Entering charging mode...\n", val);
		do_fastcharge(NULL,0,0,NULL);
	} else {
		unsigned int auxbgp, cmpgain, min_mv;
		printf("User pressed power button (0x%x)\n", val);
		auxbgp = get_auxbgp();
		cmpgain = get_cmpgain();
		min_mv = get_min_mv();
		if (get_mVoltage(auxbgp, cmpgain) < min_mv) {
			printf("Battery current too low, not connected to charger, shutdown.\n");
			power_off();
		}
	}

	// Start kernel
	return 0;
}

int do_calibrate (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	unsigned long VREF = 0;

	if (argc <2)
		return 0;

	VREF = simple_strtoul(argv[1], NULL, 10);

	printf("assuming external power is %lu mV \n", VREF);
	dp52_direct_write(DP52_PMU_EN_SW, 0x7ac);
	bandgap_calibrate(VREF);

	return 0;
}

int do_dp_get_voltage(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int mVoltage;
	u16 auxbgp = 0, cmpgain = 0;

	auxbgp = get_auxbgp();
	cmpgain = get_cmpgain();

	mVoltage = get_mVoltage(auxbgp, cmpgain);

	printf("Battery voltage = %i\n", mVoltage);

	return 0;
}

int do_dp_get_temp(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	signed int temperature = 0;
	signed int temperature_abs = 0;
	u16 tempref = 0, auxbgp = 0;

	auxbgp = get_auxbgp();
	tempref = get_tempref();

	temperature = get_temperature(auxbgp, tempref);

	/* get an absolute temp value */
	if (temperature < 0)
	{
		temperature_abs = (temperature * -1);
	}
	else
	{
		temperature_abs = temperature;
	}

	printf("Battery temperature = %i.%iC\n", (temperature / 10), temperature_abs - ((temperature_abs / 10) * 10));

	return 0;
}

U_BOOT_CMD(
	dp_get_voltage,	1,	0,	do_dp_get_voltage,
	"Get battery voltage",
	""
);

U_BOOT_CMD(
	dp_get_temp,	1,	0,	do_dp_get_temp,
	"Get battery temperature",
	""
);

U_BOOT_CMD(
	dp_caltemp,	1,	0,	do_dp_caltemp,
	"Calibrate battery temperature",
	""
);

U_BOOT_CMD(
	dp_tunecmp,	2,	1,	do_tunecomperator,
	"calibrate comperator",
	"referenceVoltage"
);


U_BOOT_CMD(
	dp_calibration,	2,	1,	do_calibrate,
	"calibrate bandgap",
	"referenceVoltage"
);

U_BOOT_CMD(
	dp_fastcharge,	1,	0,	do_fastcharge,
	"Start fastcharge",
	""
);

#endif	// CONFIG_CMD_DPCHARGE

/*****************************************************************************/
/****************** DW LOGO **************************************************/

#ifdef DW_DISPLAY_LOGO

/* TODO: here we can display different logos by including a different header file */
#include "android_logo.h"

void dw_display_logo(void)
{
	unsigned int startX, startY, y, bitmap_pos, screen_pos;
	GraphicDevice *imhGD = (GraphicDevice*)video_hw_init();
	unsigned short *frame_buffer;

	if (!imhGD)
		return;

	frame_buffer = (unsigned short*)imhGD->frameAdrs;

	/* Clear buffer (this buffer was allocated to 800x480 screen)*/
	memset(frame_buffer,0,800*480*2*2);


	/* Copy logo1 to buffer 1 */
	startX = imhGD->winSizeX / 2 - logo1_width / 2;
	startY = imhGD->winSizeY / 3 - logo1_height / 2;
	screen_pos = startX + startY * imhGD->winSizeX;
	bitmap_pos = 0;
	for (y=0 ; y<logo1_height ; y++) {
		memcpy(&frame_buffer[screen_pos],&logo1_bitmap[bitmap_pos], logo1_width*2);
		bitmap_pos+=logo1_width;
		screen_pos+=imhGD->winSizeX;
	}

	/* Copy logo2 to buffer 2 */
	startX = imhGD->winSizeX / 2 - logo2_width / 2;
	startY = imhGD->winSizeY / 3 - logo2_height / 2;
	screen_pos = startX + startY * imhGD->winSizeX + 800*480;	// double buffer was allocated for iframe/expediblue screens (800x480)
	bitmap_pos = 0;
	for (y=0 ; y<logo2_height ; y++) {
		memcpy(&frame_buffer[screen_pos],&logo2_bitmap[bitmap_pos], logo2_width*2);
		bitmap_pos+=logo2_width;
		screen_pos+=imhGD->winSizeX;
	}
}

#endif

/*****************************************************************************/

void set_backlight(int onoff)
{
	int reg_val;

	reg_val = dp52_direct_read(DP52_CMU_SYSCFG);
	reg_val |= 0x2;
	dp52_direct_write(DP52_CMU_SYSCFG, reg_val);

	/* 0x6F below is mask of PWM intesity */
	dp52_direct_write(0x66, 0x4000 | 0x6f);

	reg_val = dp52_direct_read(0x67);

	if (onoff != 0)
		reg_val |= 0x2000;
	else
		reg_val &= ~0x2000;

	dp52_direct_write(0x67, reg_val);
}

/*****************************************************************************/

#ifdef CONFIG_USB_DEVICE
void dw_usb_pullup(int enable)
{
	int gpio = CGPIO(8);

	if (dw_board == DW_BOARD_EXPEDIBLUE)
		gpio = BGPIO(17);

	/* USB Pull-up */
	gpio_set_value(gpio, enable);
	gpio_direction_output(gpio, enable);
	gpio_pullen(gpio, 0);
	gpio_pod(gpio, 0);
	gpio_enable(gpio);
}

void udc__init(void)
{
	int val;

	dw_usb_pullup(1);

	/* enable IRQ */
	//writel((1<<DW_SIC_IRQNR_USB), DW_SIC_INTC_IMASK);

	clk_enable("usb");

	val = readl(DW_SYSCFG_BASE);
	val |= (1<<0); /* Power up PHY */
	writel(val, DW_SYSCFG_BASE);

	/* clear interrupt */
	val = readb(0x0950a006);
	val = readb(0x0950a002);
	val = readb(0x0950a003);
	val = readb(0x0950a004);
	val = readb(0x0950a005);
}
#endif

#ifdef CONFIG_MUSB

#define DW_BSWRSTR_USB2MAC_SWRST	(1 << 9)
#define DW_BSWRSTR_USB2PHY_SWRST	(1 << 10)

static void musb_set_clock(int enable)
{
	unsigned long value = readl(DW_SYSCFG_BASE + 0x2c);

	if (enable) {
		value |=   1<<4;
		clk_enable("usb2");
	} else {
		value &= ~(1<<4);
		clk_disable("usb2");
	}

	writel(value, DW_SYSCFG_BASE + 0x2c);
}

static void musb_set_reset(int enable)
{
	unsigned long bswrstr = readl(DW_CMU_BASE + DW_CMU_BSWRSTR);
	unsigned long usb2_bits;

	usb2_bits = DW_BSWRSTR_USB2PHY_SWRST | DW_BSWRSTR_USB2MAC_SWRST;

	if (enable)
		bswrstr |= usb2_bits;
	else
		bswrstr &= ~usb2_bits;

	writel(bswrstr, DW_CMU_BASE + DW_CMU_BSWRSTR);
}

/*
 *  reset musb and activate clock
 */
void udc_hs_init(void)
{
	musb_set_reset(0);
	musb_set_clock(1);

	if (dw_board == DW_BOARD_IMH) {
		/* The DMW74 OTG_VBUS pad is subject to be injured while the
		 * unit is off and the USB is connected. A protection switch
		 * was added that can be controlled through DGPIO23. */
		int gpio = DGPIO(23);
		gpio_direction_output(gpio, 1);
		gpio_pullen(gpio, 0);
		gpio_enable(gpio);

		/* Configure USBDETECT as input with pull-up/down */
		gpio = CGPIO(8);
		gpio_direction_input(gpio);
		gpio_pullen(gpio, 0);
		gpio_enable(gpio);
	}
}
#endif

#ifdef CONFIG_DPCHARGE_EN
extern void del_all_mtd_devices(void);
#endif

static void enable_32khz_dp( void )
{
	int reg_val;

	// Turn on CLK32K_EN bit
	reg_val = dp52_direct_read(DP52_CMU_SYSCFG);
	reg_val |= 0x0010;
	dp52_direct_write(DP52_CMU_SYSCFG, reg_val);
}

extern void lowlevel(void);
extern int platform_board_detection(void);

/*
 * Miscellaneous platform dependent initialisations
 */
int board_init(void)
{
	u32 val;

	timer__init();

	lowlevel();

	dw_chip = dw_detect_chip();

	/* Run board detection always to set up the ethernet MAC & PHY,
	 * overwrite the result with DW_BOARD_ID (if defined).
	 */
	dw_board = platform_board_detection();
#ifdef DW_BOARD_ID
	dw_board = DW_BOARD_ID;
#endif

	if (dw_chip == DW_CHIP_DW74) {
		board_spi_init();

		/* enable 32Khz by DP52 */
		enable_32khz_dp();

		/* enable capacitator backing the dp52-rtc */
		dp52_indirect_write(DP52_PMU_IND_BAKPORLVL, 0x92);
	}

	//timer__init();

#ifdef CONFIG_DPCHARGE_EN
	// Configure environment before charger starts
	env_init();
	init_baudrate();
	serial_init();
	nand_init();
	env_relocate();

	dp_board_up();

	del_all_mtd_devices();
#endif

#ifdef CONFIG_VIBRATOR
	if (dw_board == DW_BOARD_IMH) {
		/* enable vibrator */
		val = dp52_direct_read(DP52_PMU_EN_SW);
		dp52_direct_write(DP52_PMU_EN_SW, val | DP52_PMU_LDO_SP3_EN);

		/* the voltage ("volume") for the vibratore can be changed by
		 * modifying bits 0..4 of DP52_PMU_LVL2_SW. The default value
		 * set by the preloader is 0x12 (1.5 + 1.8 = 3.3V) */
		gpio_direction_output(DGPIO(26), 1);
		gpio_enable(DGPIO(26));
	}
#endif

	if (dw_board == DW_BOARD_IMH) {
		/* Enable DECT LDO - only on boards on which LDO is not constanly ON */
		/* On other boards DGPIO(24) is not connected at all, so it should not make problems */
		gpio_direction_output(DGPIO(24), 1);
		gpio_enable(DGPIO(24));
	}

	if (dw_board == DW_BOARD_EXPEDITOR2) {
		/* Turn off backlight of CHI_MEI LCD on Expeditor 2 (AGPIO15) */
		/* enable */
		gpio_enable(AGPIO(15));
		gpio_direction_output(AGPIO(15), 0);
	}

	gd->bd->bi_arch_number = MACH_TYPE_DSPG_DW;

	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = dw_get_ramsize(DW_SDRAM_BANK1_CS);

	if (dw_get_ramsize(DW_SDRAM_BANK2_CS) > 0) {
		gd->bd->bi_dram[1].start = PHYS_SDRAM_2;
		gd->bd->bi_dram[1].size = dw_get_ramsize(DW_SDRAM_BANK2_CS);
	}

	/* set memcs3 to nfl data again if we haven't found memory */
	if (!gd->bd->bi_dram[1].size) {
		val = readl(DW_SYSCFG_BASE + DW_SYSCFG_GCR0);
		val &= ~0x8000;
		writel(val, DW_SYSCFG_BASE + DW_SYSCFG_GCR0);
	}

	/* adress of boot parameters */
	if (gd->bd->bi_dram[1].size) {
		gd->bd->bi_boot_params = min(gd->bd->bi_dram[0].start,
		                             gd->bd->bi_dram[1].start);
	} else {
		gd->bd->bi_boot_params = gd->bd->bi_dram[0].start;
	}
	gd->bd->bi_boot_params += 0x100;

	icache_enable();

#ifdef CONFIG_MUSB
	if (dw_chip != DW_CHIP_DW52) {
		gpio_direction_output(BGPIO(20), 1);

		udc_hs_init();
	}
#elif defined(CONFIG_USB_DEVICE)
	udc__init();
#endif


	dw_keypad_setup();

	return 0;
}

int board_late_init(void)
{
	int backlight_en;

#ifdef CONFIG_BOOTMENU
	dw_bootmenu();
#endif
	dw_keypad_late_init();

#ifdef DW_DISPLAY_LOGO
	/* draw initial logo on screen */
	dw_display_logo();
#endif

	/* handle backlight */
	if (dw_chip == DW_CHIP_DW74) {
		backlight_en = hatoi(getenv("backlight_en"));

		/* if the variable is not set, or contain wrong value - disable backlight */
		if (backlight_en == -1)
			backlight_en = 0;

		set_backlight(backlight_en);
	}

	return 0;
}

int dram_init (void)
{
	return 0;
}

#ifdef CONFIG_MMC
int board_mmc_init(bd_t *bis)
{
	unsigned long value;

	/* only on DW74 */
	if (dw_chip != DW_CHIP_DW74)
		return -1;

	/* enable SD power */
	value = dp52_direct_read(DP52_PMU_EN_SW);
	dp52_direct_write(DP52_PMU_EN_SW, value | DP52_PMU_LDO_SP2_EN);

	/* configure GPIOs */
	gpio_disable(AGPIO(2));  /* CKL */
	gpio_disable(DGPIO(27)); /* D0 */
	gpio_disable(DGPIO(28)); /* D1 */
	gpio_disable(DGPIO(29)); /* D2 */
	gpio_disable(DGPIO(30)); /* D3 */
	gpio_disable(DGPIO(31)); /* CMD */

	gpio_direction_input(BGPIO(22));
	gpio_enable(BGPIO(22)); /* Card detection */

	/* fixup clocks when running from DECT clock */
	if ((dw_chip == DW_CHIP_DW74) &&
	    ((readl(DW_CMU_BASE + DW_CMU_DMR) & 0x1) == 0x1) &&
	    (readl(DW_CMU_BASE + DW_CMU_PLL4CR1) == 0x0000)) {
		/*
		 * Use clk_usb as clock source:
		 *   - enable SDMMC clock divider
		 *   - Bypass clock divider (use clk_usb)
		 *   - no clock division
		 */
		writel(0x0090, DW_CMU_BASE + DW_CMU_CDR5);
		clk_enable("usb");
	}

	/* enable clocks */
	clk_enable("sdmmc");

	return syndwc_initialize(bis, NULL);
}
#endif

#ifdef CONFIG_MUSB
int udc_musb_platform_init(void)
{
	return 0;
}
#endif

#ifdef CONFIG_DISPLAY_CPUINFO
static char *dw_core_freq(void)
{
	if (	(dw_chip == DW_CHIP_DW74) &&
		((readl(DW_CMU_BASE + DW_CMU_DMR) & 0x1) == 0x1) &&
		(readl(DW_CMU_BASE + DW_CMU_PLL4CR0) == 0x0026) &&
		(readl(DW_CMU_BASE + DW_CMU_PLL4CR1) == 0x0103))
		return "240Mhz@OSC12";

	if (	(dw_chip == DW_CHIP_DW74) &&
		((readl(DW_CMU_BASE + DW_CMU_DMR) & 0x1) == 0x0) &&
		(readl(DW_CMU_BASE + DW_CMU_PLL3CR0) == 0x0c12) &&
		(readl(DW_CMU_BASE + DW_CMU_PLL3CR1) == 0x0101))
		return "240Mhz@WIFI";
/* what if DMR & 0x1 == 0x0 but PLL4CRx and PLL3CRx are set to 0? */
	if (	(dw_chip == DW_CHIP_DW74) &&
		((readl(DW_CMU_BASE + DW_CMU_DMR) & 0x1) == 0x1) &&
		(readl(DW_CMU_BASE + DW_CMU_PLL4CR1) == 0x0000))
		return "143.7696Mhz@DECT";

	if (	(dw_chip == DW_CHIP_DW52) &&
		((readl(DW_CMU_BASE + DW_CMU_DMR) & 0x2) == 0x2) &&
		(readl(DW_CMU_BASE + DW_CMU_PLL2CR0) == 0x4320))
		return "147.546Mhz@DECT";

	if (	(dw_chip == DW_CHIP_DW52) &&
		((readl(DW_CMU_BASE + DW_CMU_DMR) & 0x2) == 0x0) &&
		(readl(DW_CMU_BASE + DW_CMU_PLL2CR0) == 0x4524))
		return "144Mhz@WIFI";

	return "unknown freq.";
}

int print_cpuinfo(void)
{
	puts("CPU:   ");

	switch (dw_chip) {
	case 52:
		puts("DW52: ");
		puts(dw_core_freq());
		puts("\n");
		break;
	case 74:
	{
		int revision = dw_chip_revision();

		puts("DW74 ");
		if (revision == 0)
			puts("(ZBC - bootROM v1.0): ");
		else if (revision == 1)
			puts("(YBC - bootROM v1.1): ");
		else if (revision == 2)
			puts("(ABC - bootROM v1.2): ");
		else
			puts("(unknown revision): ");

		puts(dw_core_freq());
		puts("\n");

		break;
	}
	default: puts("unknown\n");
	}

	return 0;
}
#endif

#ifdef CONFIG_DISPLAY_BOARDINFO
int checkboard(void)
{
	int val;

	puts("Board: ");

	switch (dw_board) {
	case DW_BOARD_EXPEDIBLUE:	puts("Expediblue\n");		break;
	case DW_BOARD_EXPEDITOR1:	puts("Expeditor 1\n");		break;
	case DW_BOARD_EXPEDITOR2:	puts("Expeditor 2\n");		break;
	case DW_BOARD_EXPEDIWAU_BASIC:	puts("ExpediWAU (basic)\n");	break;
	case DW_BOARD_IMB:		puts("IMB\n");			break;
	case DW_BOARD_IFRAME:		puts("IFrame\n");		break;
	case DW_BOARD_IMB2:		puts("IMB 2\n");		break;
	case DW_BOARD_IMH:		puts("IMH\n");			break;
	default:			puts("unknown\n");
	}

	puts("Oscillators: ");
	val = readl(DW_CMU_BASE + DW_CMU_OSCR);
	if (val & 0x40)
		puts("WIFI ");
	if (val & 0x20)
		puts("DECT ");
	if (val & 0x80)
		puts("OSC12M ");
	puts("\n");

	return 0;
}
#endif

static int hatoi(char *p)
{
	int res = 0;

	while (1) {
		switch (*p) {
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
			res |= (*p - 'a' + 10);
			break;
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
			res |= (*p - 'A' + 10);
			break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			res |= (*p - '0');
			break;
		default:
			return -1;
		}
		p++;
		if (*p == 0)
		{
			break;
		}
		res <<= 4;
	}

	return res;
}

static unsigned char *gethwaddr(char *in, unsigned char *out)
{
	char tmp[3];
	int i;
	int myint;

	for (i=0;i<6;i++) {
		if ((in[i*3+2] == 0) && (i == 5))
		{
			if ( (myint = hatoi(&in[i*3])) < 0 )
			{
				return NULL;
			}
			out[i] = (unsigned char)myint;
		}
		else if ((in[i*3+2] == ':') && (i < 5))
		{
			tmp[0] = in[i*3];
			tmp[1] = in[i*3+1];
			tmp[2] = 0;
			if ( (myint = hatoi(tmp)) < 0 )
			{
				return NULL;
			}
			out[i] = (unsigned char)myint;
		}
		else
		{
			return NULL;
		}
	}

	return out;
}

extern char *simple_itoa(unsigned int i);

#ifdef CONFIG_USBD_DFU
int dfu_enter_dfu_mode(void);

int do_dfu(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	dfu_enter_dfu_mode();

	return 0;
}

U_BOOT_CMD(dfu, 2, 0, do_dfu,
	"dfu - Enter DFU mode.\n",
	""
);
#endif

int misc_init_r (void)
{
	char *s;
	unsigned char *eth_addr;
#ifdef CONFIG_VIBRATOR
	unsigned long val;
#endif

	setenv("verify", "n");

	if ( dw_board == DW_CHIP_UNKNOWN )
		s = "-1";
	else
		s = simple_itoa(dw_board);

	setenv("board", s);

	s = getenv("ethaddr0");
	eth_addr = gethwaddr(s, dw_eeprom_record.eth0_mac);
	if (NULL == eth_addr)
	{
		printf ("Can not parse ETH0 MAC address\n");
		return 1;
	}

	if (dw_board == DW_BOARD_EXPEDIBLUE) {
		s = getenv("ethaddr1");
		eth_addr = gethwaddr(s, dw_eeprom_record.eth1_mac);
		if (NULL == eth_addr) {
			printf ("Can not parse ETH1 MAC address\n");
			return 1;
		}
	}

	s = getenv("wifiaddr");
	eth_addr = gethwaddr(s, dw_eeprom_record.wifi_mac);
	if (NULL == eth_addr)
	{
		printf ("Can not parse WiFi MAC address\n");
		return 1;
	}

	dw_eeprom_valid = 1;

#ifdef CONFIG_VIBRATOR
	if (dw_board == DW_BOARD_IMH) {
		/* disable vibrator */
		gpio_set_value(DGPIO(26), 0);

		val = dp52_direct_read(DP52_PMU_EN_SW);
		val &= ~(DP52_PMU_LDO_SP3_EN);
		dp52_direct_write(DP52_PMU_EN_SW, val);
	}
#endif

	return 0;
}

static unsigned long timestamp;
static unsigned long lastdec;
/*
 * timer without interrupts
 */

void reset_timer(void)
{
	reset_timer_masked();
}

unsigned long get_timer(unsigned long base)
{
	return (get_timer_masked() / TICKS_PER_mSEC) - base;
}

/* delay x useconds AND perserve advance timstamp value */
void udelay(unsigned long usec)
{
	unsigned long tmo, tmp;
	unsigned long r;

	// Should be 5 in DW - There are 5 ticks per uSec
	// Should be 1 in Versatile - There is 1 ticks per uSec
	r = TICKS_PER_uSEC; //CONFIG_SYS_HZ / (1000 * 1000);
	tmo = usec * r;

	tmp = get_timer_masked();	/* get current timestamp */
	if ((tmo + tmp + 1) < tmp)	/* if setting this fordward will roll time stamp */
		reset_timer_masked();	/* reset "advancing" timestamp to 0, set lastdec value */
	else
		tmo += tmp;		/* else, set advancing stamp wake up time */

	while (get_timer_masked() < tmo)/* loop till event */
		/*NOP*/;
}

void reset_timer_masked(void)
{
	/* reset time */
	lastdec = READ_TIMER;	/* capure current decrementer value time */
	timestamp = 0;		/* start "advancing" time stamp from 0 */
}

unsigned long get_timer_masked(void)
{
	unsigned long now = READ_TIMER;		/* current tick value */

	if (lastdec >= now) {
		/* normal mode (non roll) */
		timestamp += lastdec - now; /* move stamp fordward with absoulte diff ticks */
	} else {/* we have overflow of the count down timer */
		/* nts = ts + ld + (TLV - now)
		 * ts=old stamp, ld=time that passed before passing through -1
		 * (TLV-now) amount of time after passing though -1
		 * nts = new "advancing time stamp"...it could also roll and cause problems.
		 */
		timestamp += lastdec + CFG_TIMER_RELOAD - now;
	}
	lastdec = now;

	return timestamp;
}

/* waits specified delay value and resets timestamp */
void udelay_masked(unsigned long usec)
{
	unsigned long tmo;
	unsigned long endtime;
	signed long diff;
	unsigned long r;

	// Should be 5 in DW - There are 5 ticks per uSec
	// Should be 1 in Versatile - There is 1 ticks per uSec
	r = TICKS_PER_uSEC; //CONFIG_SYS_HZ / (1000 * 1000);
	tmo = usec * r;

	endtime = get_timer_masked () + tmo;

	do {
		unsigned long now = get_timer_masked();
		diff = endtime - now;
	} while (diff >= 0);
}

int timer_init(void)
{
	return 0;
}

void timer__init(void)
{
	clk_enable("tmr1");

	/* Try to find out timer frequency */
	/* Configure the DW timer */
	writel(CFG_TIMER_RELOAD, DW_TIMER0_1_BASE + 0); /* timer load */
	writel(CFG_TIMER_RELOAD, DW_TIMER0_1_BASE + 4); /* timer value */
	writel(CFG_DW_TIMER_CONTROL, DW_TIMER0_1_BASE + 8);

	/* init the timestamp and lastdec value */
	reset_timer_masked();
}

/*
 * Write the CMU S/W reset register to cause reset
 */
void reset_cpu(unsigned long addr)
{
	//Clear WDRSTIND and SWRSTIND before SW resetting the DW
	*(volatile unsigned int *)(DW_CMU_BASE + 0x40) |= ((1<<5) | (1<<4));

	*(volatile unsigned int *)(DW_CMU_BASE + 0x3C) = 0x0000398B;
	*(volatile unsigned int *)(DW_CMU_BASE + 0x3C) = 0x0000C674;
}

int do_dw_test_timers(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int count = 10;

	printf("Counting: %2d ", count);

	while(count > 0) {
		mdelay(1000);
		printf ("\b\b\b%2d ", --count);
	}
	printf("\n");

	return 0;
}

U_BOOT_CMD(test_timers,	2,	0,	do_dw_test_timers,
	"test_timers - Run Timers test (countdwon for 10 seconds).\n",
	"0/1\n"
);

int do_dw_test_uart(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int key;

	printf("Type any character (Press ESC stops the test)\n");

	while((key = getc()) != 0x1B)
	{
		printf("%c", key);
	}

	printf("\n");
	return 0;
}

U_BOOT_CMD(test_uart,	2,	0,	do_dw_test_uart,
	"test_uart\t - Run UART test (characters loop-back).\n",
	"0/1\n"
);

//EEPROM
#define SBUS_STOP_MASK 0x100
#define SBUS_RESTART_MASK 0x200

#define _writel(v,p) writel(v,p);delay(100000)

/* Init I2C SBUS so we can access the eeprom */
void dw_init_i2c_eeprom(void)
{
	clk_disable("fc"); /* FC_EN=0 disable NFLC (same I/O with SCL) */

	_writel(0, DW_SBUS_BASE + SBUS_CFG); /* Disable SBUS */
	_writel(0x600, DW_SBUS_BASE + SBUS_FREQ); /* Set divider to xx */
	_writel(0x8000, DW_SBUS_BASE + SBUS_CFG); /* Enable SBUS */
	_writel(SBUS_STOP_MASK, DW_SBUS_BASE + SBUS_TX);
}

/* Read I2C device regs */
unsigned char dw_read_i2c_eeprom(unsigned char dev_id, unsigned short addr)
{
	unsigned char res;
	_writel(dev_id, DW_SBUS_BASE + SBUS_TX);
	_writel((addr & 0xFF00) >> 8, DW_SBUS_BASE + SBUS_TX);
	_writel((addr & 0x00FF), DW_SBUS_BASE + SBUS_TX);
	//_writel(SBUS_RESTART_MASK, DW_SBUS_BASE + SBUS_TX);
	_writel(SBUS_RESTART_MASK | dev_id | 0x1, DW_SBUS_BASE + SBUS_TX);
	_writel(SBUS_STOP_MASK, DW_SBUS_BASE + SBUS_TX);
	udelay(100);
	res = readl(DW_SBUS_BASE + SBUS_RX);
	udelay(100);
	printf("[0] %x\n", res);
	return res;
}


void dw_read_seq_i2c_eeprom(unsigned char dev_id, unsigned short addr)
{
	int i;

	_writel(dev_id, DW_SBUS_BASE + SBUS_TX);
	_writel((4096 & 0xFF00) >> 8, DW_SBUS_BASE + SBUS_TX);
	_writel((4096 & 0x00FF), DW_SBUS_BASE + SBUS_TX);
	_writel(SBUS_RESTART_MASK | dev_id | 0x1, DW_SBUS_BASE + SBUS_TX);

	//_writel(dev_id | 0x1, DW_SBUS_BASE + 0x00);
	for (i = 0; i < 63; i++) {
		udelay(100);
		((unsigned char*)&dw_eeprom_record)[i] = readl(DW_SBUS_BASE + SBUS_RX);
		udelay(100);
	}
	_writel(SBUS_STOP_MASK, DW_SBUS_BASE + SBUS_TX);
	udelay(100);
	((unsigned char*)&dw_eeprom_record)[i] = readl(DW_SBUS_BASE + SBUS_RX);
	udelay(100);
}


unsigned char* dw_get_mac(int mac_id)
{
	if (!dw_eeprom_valid)
		return NULL;

	switch (mac_id) {
	case DW_MACID_MACB0:
		return dw_eeprom_record.eth0_mac;
	case DW_MACID_MACB1:
		return dw_eeprom_record.eth1_mac;
	case DW_MACID_WIFI:
		return dw_eeprom_record.wifi_mac;
	case DW_MACID_RESERVED:
		return dw_eeprom_record.reserved_mac;
	default:
		return NULL;
	}
}

int do_dw_cfg_mac_addr(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	unsigned long long addr;
	int board_ser;
	char s_env_mac[48];
	int max_board_num;
	int min_board_num;
	unsigned int num_addr_per_board;
	unsigned long long base_mac_addr;

	if (dw_board != DW_BOARD_EXPEDIBLUE) {
		// Expeditor
		min_board_num = 300;
		max_board_num = 1000-1;
		num_addr_per_board = 2;
		base_mac_addr = (unsigned long long)MAC_ADDR_BASE + 300ULL*4; // There are max 300 ExpediBlue boards with 4 addresses each
	} else {
		// ExpediBlue
		min_board_num = 0;
		max_board_num = 300-1;
		num_addr_per_board = 4;
		base_mac_addr = (unsigned long long)MAC_ADDR_BASE;
	}

	if ((argc < 2) || (argc > 3)) {
		printf ("Usage:\n%s\n", cmdtp->help);
		return 1;
	}

	board_ser = simple_strtol(argv[1], NULL, 10) + min_board_num;

	if ((board_ser < min_board_num) || (board_ser > max_board_num)) {
		printf ("Usage:\ncfg_mac_addr <board serial number in the range of 0 to %d>\n", (max_board_num-min_board_num));
		return 1;
	}

	addr = base_mac_addr + ((board_ser-min_board_num) * num_addr_per_board); //ETH0
	dw_eeprom_record.eth0_mac[0] = (addr >> 0) & 0xFF;
	dw_eeprom_record.eth0_mac[1] = (addr >> 8) & 0xFF;
	dw_eeprom_record.eth0_mac[2] = (addr >> 16) & 0xFF;
	dw_eeprom_record.eth0_mac[3] = (addr >> 24) & 0xFF;
	dw_eeprom_record.eth0_mac[4] = (addr >> 32) & 0xFF;
	dw_eeprom_record.eth0_mac[5] = (addr >> 40) & 0xFF;
	sprintf(s_env_mac, "%02X:%02X:%02X:%02X:%02X:%02X",
			dw_eeprom_record.eth0_mac[5], dw_eeprom_record.eth0_mac[4],
			dw_eeprom_record.eth0_mac[3], dw_eeprom_record.eth0_mac[2],
			dw_eeprom_record.eth0_mac[1], dw_eeprom_record.eth0_mac[0]);
	setenv ("ethaddr0", s_env_mac);

	if (dw_board == DW_BOARD_EXPEDIBLUE) {
		addr += 1; //ETH1
		dw_eeprom_record.eth1_mac[0] = (addr >> 0) & 0xFF;
		dw_eeprom_record.eth1_mac[1] = (addr >> 8) & 0xFF;
		dw_eeprom_record.eth1_mac[2] = (addr >> 16) & 0xFF;
		dw_eeprom_record.eth1_mac[3] = (addr >> 24) & 0xFF;
		dw_eeprom_record.eth1_mac[4] = (addr >> 32) & 0xFF;
		dw_eeprom_record.eth1_mac[5] = (addr >> 40) & 0xFF;
		sprintf(s_env_mac, "%02X:%02X:%02X:%02X:%02X:%02X",
				dw_eeprom_record.eth1_mac[5], dw_eeprom_record.eth1_mac[4],
				dw_eeprom_record.eth1_mac[3], dw_eeprom_record.eth1_mac[2],
				dw_eeprom_record.eth1_mac[1], dw_eeprom_record.eth1_mac[0]);
		setenv ("ethaddr1", s_env_mac);
	}

	addr += 1; //WIFI
	dw_eeprom_record.wifi_mac[0] = (addr >> 0) & 0xFF;
	dw_eeprom_record.wifi_mac[1] = (addr >> 8) & 0xFF;
	dw_eeprom_record.wifi_mac[2] = (addr >> 16) & 0xFF;
	dw_eeprom_record.wifi_mac[3] = (addr >> 24) & 0xFF;
	dw_eeprom_record.wifi_mac[4] = (addr >> 32) & 0xFF;
	dw_eeprom_record.wifi_mac[5] = (addr >> 40) & 0xFF;
	sprintf(s_env_mac, "%02X:%02X:%02X:%02X:%02X:%02X",
			dw_eeprom_record.wifi_mac[5], dw_eeprom_record.wifi_mac[4],
			dw_eeprom_record.wifi_mac[3], dw_eeprom_record.wifi_mac[2],
			dw_eeprom_record.wifi_mac[1], dw_eeprom_record.wifi_mac[0]);
	setenv ("wifiaddr", s_env_mac);

	if (dw_board == DW_BOARD_EXPEDIBLUE) {
		addr += 1; //RESERVED
		dw_eeprom_record.reserved_mac[0] = (addr >> 0) & 0xFF;
		dw_eeprom_record.reserved_mac[1] = (addr >> 8) & 0xFF;
		dw_eeprom_record.reserved_mac[2] = (addr >> 16) & 0xFF;
		dw_eeprom_record.reserved_mac[3] = (addr >> 24) & 0xFF;
		dw_eeprom_record.reserved_mac[4] = (addr >> 32) & 0xFF;
		dw_eeprom_record.reserved_mac[5] = (addr >> 40) & 0xFF;
		sprintf(s_env_mac, "%02X:%02X:%02X:%02X:%02X:%02X",
				dw_eeprom_record.reserved_mac[5], dw_eeprom_record.reserved_mac[4],
				dw_eeprom_record.reserved_mac[3], dw_eeprom_record.reserved_mac[2],
				dw_eeprom_record.reserved_mac[1], dw_eeprom_record.reserved_mac[0]);
		setenv ("resvaddr", s_env_mac);
	}

	if (saveenv()) {
		printf("\nFailed saving environment.\n");
		return 1;
	} else {
		printf("\nDone configuring MAC addresses. Type <printenv> to verify.\n");
		return 0;
	}
}

U_BOOT_CMD(cfg_mac_addr, 2, 0, do_dw_cfg_mac_addr,
	"cfg_mac_addr - Configure MAC addresses for ETH and WiFi interfaces.\n",
	""
);

////////////////////////////////////////////////////////////////////////////
// THE FOLLOWING code is an addition to support SAFE UPGRADE from user space
////////////////////////////////////////////////////////////////////////////
//test kernel uImage integrity
extern int image_info (unsigned long addr);

#define DSPG_DW_NOR_KEREL_START_ENV_NAME    "kernel_start"

#define DSPG_DW_NOR_KEREL1_START_ENV_NAME   "kernel1_start"
#define DSPG_DW_NOR_KEREL2_START_ENV_NAME   "kernel2_start"

#define DSPG_DW_NOR_ROOTFS1_START_ENV_NAME  "rootfs1_start"
#define DSPG_DW_NOR_ROOTFS2_START_ENV_NAME  "rootfs2_start"

#define DSPG_DW_ENV_ROOTFS1_SIZE_NAME       "rootfs_size"
#define DSPG_DW_ENV_ROOTFS2_SIZE_NAME       "rootfs2_size"

#define DSPG_DW_ENV_ROOTFS1_CRC_NAME        "rootfs_crc"
#define DSPG_DW_ENV_ROOTFS2_CRC_NAME        "rootfs2_crc"

#define DSPG_DW_ENV_ROOT_BLOCK_NAME         "rootblock"


///
///	@brief		Calculate CRC32 for specified rootfs start address
///
///	@pRootfsStart   [in]	start address of root file system
///	@pRootfsSize    [in]	pointer to uboot env string containing rootfs size
///	@pRootfsCrc     [in]	pointer to uboot env string containing rootfs crc
///
///	@return		1 - main rootfs/2 - second rootfs/ -1 failure
///
static int dspg_dw_rootfs_crc( char *pRootfsStart, char *pRootfsSize,char *pRootfsCrc)
{
    char *pSize;
    char *pCRC;
    char *pAddress;

    //get second rootfs size and crc
    pSize = getenv(pRootfsSize);
    pCRC =  getenv(pRootfsCrc);
    pAddress = getenv(pRootfsStart);
    //Test secondary rootfs address

    if( NULL != pSize && NULL != pCRC && NULL != pAddress )
    {
        unsigned long Crc;
        unsigned long Size;
        unsigned long RootfsStart;

        RootfsStart = simple_strtoul(pAddress,NULL,16);
        Size = simple_strtoul(pSize,NULL,16);
        Crc = simple_strtoul(pCRC,NULL,16);

        //test CRC32 for secondary rootfs address
        if (crc32(0,(const uchar *)RootfsStart,Size) != Crc)
        {
            printf("WRONG CRC on 0x%8lx rootfs\n",RootfsStart);
            return -1;
        }

    }

    return 0;
}


///
///	@brief		Set "rootblock" environment varaible
///
///	@pRootfs   [in]	rootfs partition name string
///
///	@return		0 - success / -1 failure
///
int dspg_dw_set_root_block( char *pRootfs )
{
    char *pRootBlock = getenv(DSPG_DW_ENV_ROOT_BLOCK_NAME);
    char *pMtdParts = getenv(DSPG_DW_ENV_MTDPARTS_NAME);
    int i=0;

    if ( NULL != pRootBlock )
    {
        //remove the unmber from 'rootblock' parameter
        while ( 0 != pRootBlock[i] )
        {
            //find the first occurence of digit
            if ( pRootBlock[i] > '0' && pRootBlock[i] < '9' )
            {
                pRootBlock[i] = 0;
                break;
            }
            i++;
        }

        if( NULL != pMtdParts )
        {
            int count = 0;
            char *pTmp;
            char buf[32];

            //search for rootfs/rootfs2 inside mtdparts
            pTmp = strstr(pMtdParts,pRootfs);
            if ( NULL != pTmp )
            {
                //search backwards to the beginning of 'mtdparts'
                //and count the number of ',' delimiters
                while( pTmp != pMtdParts )
                {
                    if ( ',' == *pTmp || ';' == *pTmp)
                    {
                        count++;
                    }
                    pTmp--;
                }
            }

            sprintf(buf,"%s%d",pRootBlock,count);
            setenv("rootblock",buf);

            return 0;
        }
    }

    return -1;
}
///
///	@brief		Check file system for integrity
///
///	@return		1 - main rootfs/2 - second rootfs/ -1 failure
///
static int dspg_dw_check_rootfs( void )
{
    //check if rootfs verification was done at least once
    char *pRootFsVerify = getenv(DSPG_DW_ENV_ROOTFS_VERIFY_NAME);

    //get current active kernel number 1-MAIN/2-SECONDARY
    char *pRootFsActive = getenv(DSPG_DW_NOR_ROOTFS_ACTIVE_ENV_NAME);

    if ( NULL == pRootFsVerify || NULL == pRootFsActive )
    {
        return -1;
    }
    else if ( '1' == *pRootFsVerify )
    {
        //set 'rootblock' parameter
        if( '1' == *pRootFsActive )
            dspg_dw_set_root_block(DSPG_DW_ENV_ROOTFS_NAME);
        else
            dspg_dw_set_root_block(DSPG_DW_ENV_ROOTFS2_NAME);

        return 0;
    }

    if( '2' == *pRootFsActive )
    {
        if (dspg_dw_rootfs_crc(DSPG_DW_NOR_ROOTFS2_START_ENV_NAME,
                               DSPG_DW_ENV_ROOTFS2_SIZE_NAME,
                               DSPG_DW_ENV_ROOTFS2_CRC_NAME) != 0)
        {
            if (dspg_dw_rootfs_crc(DSPG_DW_NOR_ROOTFS1_START_ENV_NAME,
                               DSPG_DW_ENV_ROOTFS1_SIZE_NAME,
                               DSPG_DW_ENV_ROOTFS1_CRC_NAME) == 0)
            {
                //Set roofs 1 as active
                setenv(DSPG_DW_ENV_ROOTFS_VERIFY_NAME,"1");
                dspg_dw_set_root_block(DSPG_DW_ENV_ROOTFS_NAME);
            }
            else
            {
                return -1;
            }
        }
        else
        {
            //switch rootblock to start from rootfs2
            dspg_dw_set_root_block(DSPG_DW_ENV_ROOTFS2_NAME);
        }

    }
    else
    {
        if (dspg_dw_rootfs_crc(DSPG_DW_NOR_ROOTFS1_START_ENV_NAME,
                               DSPG_DW_ENV_ROOTFS1_SIZE_NAME,
                               DSPG_DW_ENV_ROOTFS1_CRC_NAME) != 0)
        {
            if (dspg_dw_rootfs_crc(DSPG_DW_NOR_ROOTFS2_START_ENV_NAME,
                               DSPG_DW_ENV_ROOTFS2_SIZE_NAME,
                               DSPG_DW_ENV_ROOTFS2_CRC_NAME) == 0)
            {
                //Set roofs 2 as active
                setenv(DSPG_DW_ENV_ROOTFS_VERIFY_NAME,"2");
                dspg_dw_set_root_block(DSPG_DW_ENV_ROOTFS2_NAME);
            }
            else
            {
                return -1;
            }
        }
        else
        {
            //switch rootblock to start from rootfs
            dspg_dw_set_root_block(DSPG_DW_ENV_ROOTFS_NAME);
        }
    }

    saveenv();
    return 0;
}

static int dspg_dw_check_kernel( void )
{
    char    *pTmp,*pKernel1Start,*pKernel2Start;
    unsigned long   kernel1Start,kernel2Start;

    //get current active kernel number 1-MAIN/2-SECONDARY
	pTmp = getenv(DSPG_DW_NOR_KEREL_ACTIVE_ENV_NAME);
    pKernel1Start = getenv(DSPG_DW_NOR_KEREL1_START_ENV_NAME);
    pKernel2Start = getenv(DSPG_DW_NOR_KEREL2_START_ENV_NAME);

    if( NULL == pTmp || NULL == pKernel1Start || NULL == pKernel2Start)
    {
        return -1;
    }

    kernel1Start = simple_strtoul(pKernel1Start, NULL, 16);
    kernel2Start = simple_strtoul(pKernel2Start, NULL, 16);

    if( 0x0 == kernel1Start || 0x0 == kernel2Start)
    {
        return -1;
    }

    //default start is main
    if(pTmp != NULL && *pTmp == '2')
    {
        //try to load from secondary flash address
        if(image_info(kernel2Start) != 0)
        {
            if(image_info(kernel1Start) == 0)
            {
                setenv(DSPG_DW_NOR_KEREL_ACTIVE_ENV_NAME,"1");
                setenv(DSPG_DW_NOR_KEREL_START_ENV_NAME,pKernel1Start);
                saveenv();
            }
            else
            {
                return -1;
            }
        }
    }
    else
    {
        //try to load from main flash address
        if(image_info(kernel1Start) != 0)
        {
            if(image_info(kernel2Start) == 0)
            {
                setenv(DSPG_DW_NOR_KEREL_ACTIVE_ENV_NAME,"2");
                setenv(DSPG_DW_NOR_KEREL_START_ENV_NAME,pKernel2Start);
                saveenv();
            }
            else
            {
                return -1;
            }
        }
    }

    return 0;
}

typedef enum
{
    FT_NONE,
    FT_SAFE,
    FT_NOT_SAFE,
} EFlashType;

///
///	@brief		Determine flash layout (safe/not safe)
///
///	@return
///
EFlashType dspg_dw_get_fs_type( void )
{
    char    *pTmp;
    //get current active kernel number 1-MAIN/2-SECONDARY
	pTmp = getenv(DSPG_DW_ENV_MTDPARTS_NAME);

    if( NULL == pTmp )
    {
        return FT_NONE;
    }

    if ( NULL == strstr(pTmp,DSPG_DW_ENV_ROOTFS2_NAME) )
    {
        return FT_NOT_SAFE;
    }

    return FT_SAFE;
}

///
///	@brief		Safe boot command
///
///	@Address        [in]	start address for crc calculation
///	@pRootfsSize    [in]	pointer to uboot env string containing rootfs size
///	@pRootfsCrc     [in]	pointer to uboot env string containing rootfs crc
///
///	@return
///
int dspg_dw_do_safe_bootm ( cmd_tbl_t *pCmdTbl, int Flag, int Argc, char *pArgv[] )
{
    int     rc;
    EFlashType flashType;

    flashType = dspg_dw_get_fs_type();

    if ( FT_NONE == flashType )
    {
        printf("Please set mtdparts\n");
        return 0;
    }
    else if ( FT_NOT_SAFE == flashType )
    {
        //no safe upgrade, set rootblock parameter
        dspg_dw_set_root_block(DSPG_DW_ENV_ROOTFS_NAME);
        return 0;
    }

    //test kernel for integrity
    rc = dspg_dw_check_kernel();
    if ( -1 == rc )
    {
        printf("No active kernel found\n");
        return 0;
    }

    //test file system for integrity
    rc = dspg_dw_check_rootfs();
    if ( -1 == rc )
    {
        printf("No active root file system found\n");
        return 0;
    }

    return 0;
}

U_BOOT_CMD(
	safe_bootm,	1,	0,	dspg_dw_do_safe_bootm,
	"safe_bootm   - safe application boot from flash\n",
	""
);

#ifdef CONFIG_REVISION_TAG
u32 get_board_rev(void)
{
	switch (dw_chip) {
	case DW_CHIP_DW52:
		return 0x5200;
	case DW_CHIP_DW74:
		return 0x7400 + dw_chip_revision();
	}

	return -1;
}
#endif

#ifdef CONFIG_USE_IRQ
int arch_interrupt_init(void)
{
	return 0;
}
#endif

unsigned int dw_get_pclk(void)
{
#define RSTSR_OSC12  0x8
#define RSTSR_WIFIEN 0x4
#define RSTSR_DECTEN 0x2
#define RSTSR_CLKMSK (RSTSR_OSC12 | RSTSR_WIFIEN | RSTSR_DECTEN)

	u16 clk_straps = readl(DW_CMU_BASE + DW_CMU_RSTSR) & RSTSR_CLKMSK;
	u16 src_sel = readl(DW_CMU_BASE + DW_CMU_CDR6);

	/* dw52 which does not have pll4 and no wifi 20MHz osc */
	if ((dw_chip == DW_CHIP_DW52) && !(clk_straps & RSTSR_WIFIEN))
		return 72000000;

	/* dw74, but only dect oscillator available, no 12Mhz */
	if ((dw_chip == DW_CHIP_DW74) && (clk_straps == RSTSR_DECTEN))
		return 72000000;

	/* dw74, has 12MHz and dect, no wifi, but we use pll2 as hclk */
	if ((dw_chip == DW_CHIP_DW74) &&
	    (clk_straps == (RSTSR_DECTEN | RSTSR_OSC12)) &&
	    !(src_sel & 0x2))
		return 72000000;

	return 80000000;
}

int do_mmu(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	enable_mmu();
	return 0;
}

U_BOOT_CMD(
	mmu, 1, 0, do_mmu,
	"mmu\t- activate MMU and data cache\n",
	"\n"
);
