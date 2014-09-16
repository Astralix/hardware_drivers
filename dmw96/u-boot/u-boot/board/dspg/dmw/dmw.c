/*
 * (C) Copyright 2010 DSP Group
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
#include <flash.h>
#include <u-boot/zlib.h>
#include <linux/mtd/nand.h>
#include <syndwc_mmc.h>
#include <spi.h>
#include <asm/arch/gpio.h>
#include <mmc.h>
#include <i2c.h>
#include <part.h>

#include "dmw.h"
#include "clock.h"
#include "pads.h"
#include "dp52.h"
#include "keypad.h"
#include "lcd.h"
#include "pnx8181_i2c.h"
#include "irq.h"

#ifdef CONFIG_BOOTMENU
#include "bootmenu.h"
#endif

#define EVB96_CPLD_I2C_ADDR	0x37
#define EVB96_CPLD_I2C_BUS	1

DECLARE_GLOBAL_DATA_PTR;

unsigned long board_i2c_pnx_get_pclk(void)
{
	return dmw_get_pclk();
}

static int board_i2c_init(void)
{
	gpio_disable(CGPIO(28));
	gpio_disable(CGPIO(29));

	gpio_pull_ctrl(CGPIO(28), 0, 1);
	gpio_pull_ctrl(CGPIO(29), 0, 1);

	reset_release("i2c1");
	clk_enable("i2c1");
	pads_enable("i2c1");

#ifdef	CONFIG_I2C_MULTI_BUS
	gpio_disable(CGPIO(30));
	gpio_disable(CGPIO(31));

	gpio_pull_ctrl(CGPIO(30), 0, 1);
	gpio_pull_ctrl(CGPIO(31), 0, 1);

	reset_release("i2c2");
	clk_enable("i2c2");
	pads_enable("i2c2");
#endif

	return 0;
}

#ifdef CONFIG_MACG
int gmac_eth_initialize(int id, void *regs, unsigned int phy_addr);
#endif

#if defined(DMW_BOARD_EVB) || defined(DMW_BOARD_TFEVB)
static void phy_set_straps(void)
{
	// Reset the PHY
	gpio_enable(DGPIO(26));
	gpio_direction_output(DGPIO(26), 0);

	// Enable the strap pins gpios
	gpio_enable(EGPIO(3));
	gpio_enable(EGPIO(4));
	gpio_enable(EGPIO(6));
	gpio_enable(EGPIO(13));
	gpio_enable(EGPIO(14));
	gpio_enable(EGPIO(15));
	gpio_enable(EGPIO(17));

	// Configure strap pins for SMSC 8710A on EVB
	gpio_direction_output(EGPIO(13), 0); /* MIISEL = 0 (MII) */

	gpio_direction_output(EGPIO(3),  1); /* MODE0  = 1 (Enable autoneg) */
	gpio_direction_output(EGPIO(4),  1); /* MODE1  = 1 (Enable autoneg) */
	gpio_direction_output(EGPIO(17), 1); /* MODE2  = 1 (Enable autoneg) */

	gpio_direction_output(EGPIO(6),  1); /* PHYAD0 = 1 (PHY address 3) */
	gpio_direction_output(EGPIO(15), 1); /* PHYAD1 = 1 (PHY address 3) */
	gpio_direction_output(EGPIO(14), 0); /* PHYAD2 = 0 (PHY address 3) */

	udelay(1000);

	// Take the PHY out of reset
	gpio_set_value(DGPIO(26), 1);

	udelay(1000);

	// Re-configure strap pins as MII signals (and not gpios)
	gpio_disable(EGPIO(3));
	gpio_disable(EGPIO(4));
	gpio_disable(EGPIO(6));
	gpio_disable(EGPIO(13));
	gpio_disable(EGPIO(14));
	gpio_disable(EGPIO(15));
	gpio_disable(EGPIO(17));
}
#elif defined(DMW_BOARD_IMH3)
static void phy_set_straps(void)
{
	gpio_enable(AGPIO(26));
	gpio_direction_output(AGPIO(26), 0);

	// Enable the strap pins gpios
	gpio_enable(EGPIO(3));
	gpio_enable(EGPIO(4));
	gpio_enable(EGPIO(5));
	gpio_enable(EGPIO(6));
	gpio_enable(EGPIO(13));
	gpio_enable(EGPIO(14));
	gpio_enable(EGPIO(16));
	gpio_enable(EGPIO(17));

	// Configure strap pins for VIA VT6103 on IMH3 + BDB
	                                     /* PHYAD0 = 1 (PHY address 7) */
	gpio_direction_output(EGPIO(14), 1); /* PHYAD1 = 1 (PHY address 7) */
	gpio_direction_output(EGPIO(13), 1); /* PHYAD2 = 1 (PHY address 7) */
	gpio_direction_output(EGPIO(3),  0); /* PHYAD3 = 0 (PHY address 7) */
	gpio_direction_output(EGPIO(4),  0); /* PHYAD4 = 0 (PHY address 7) */

	gpio_direction_output(EGPIO(17), 0); /* SYMB   = 0 (Symbol mode) */
	gpio_direction_output(EGPIO(6),  0); /* ISO    = 0 (Isolate off) */
	gpio_direction_output(EGPIO(16), 0); /* RPTR   = 0 (Repeater mode off) */

	gpio_direction_output(EGPIO(5),  0); /* BYPOSC = 0 */

	udelay(10000);

	// Take the PHY out of reset
	gpio_set_value(AGPIO(26), 1);

	udelay(1000);

	// Re-configure strap pins as MII signals (and not gpios)
	gpio_disable(EGPIO(3));
	gpio_disable(EGPIO(4));
	gpio_disable(EGPIO(5));
	gpio_disable(EGPIO(6));
	gpio_disable(EGPIO(13));
	gpio_disable(EGPIO(14));
	gpio_disable(EGPIO(16));
	gpio_disable(EGPIO(17));
}
#else
static void phy_set_straps(void) { }
#endif

int board_eth_init(bd_t *bis)
{
	int rc = 0;

#ifdef CONFIG_MACG
	reset_release("ethmac");
	clk_enable("ethmac");

	/* MII */
	gpio_disable(EGPIO(0));  /* TXD0 */
	gpio_disable(EGPIO(1));  /* TXD1 */
	gpio_disable(EGPIO(2));  /* TXEN */
	gpio_disable(EGPIO(3));  /* RXD0 */
	gpio_disable(EGPIO(4));  /* RXD1 */
	gpio_disable(EGPIO(5));  /* CRS_DV */
	gpio_disable(EGPIO(6));  /* RXER */
	gpio_disable(EGPIO(7));  /* REFCK */
	gpio_disable(EGPIO(8));  /* MDIO */
	gpio_disable(EGPIO(9));  /* MDC */
	gpio_disable(EGPIO(10)); /* TXER */
	gpio_disable(EGPIO(11)); /* TXD2 */
	gpio_disable(EGPIO(12)); /* TXD3 */
	gpio_disable(EGPIO(13)); /* RXD2 */
	gpio_disable(EGPIO(14)); /* RXD3 */
	gpio_disable(EGPIO(15)); /* RXCLK */
	gpio_disable(EGPIO(16)); /* RXCRS */
	gpio_disable(EGPIO(17)); /* COL */

	phy_set_straps();

	pads_enable("emaca");

	rc = gmac_eth_initialize(0, (void *)0x06E00000, 0);
#endif

	return rc;
}

static int dmw_lcd_init(void)
{
	int rc = 0;

	reset_release("lcdc");
	clk_enable("lcdc");

	gpio_disable(DGPIO(0));  /* LCDD0 */
	gpio_disable(DGPIO(1));  /* LCDD1 */
	gpio_disable(DGPIO(2));  /* LCDD8 */
	gpio_disable(DGPIO(3));  /* LCDD9 */
	gpio_disable(DGPIO(4));  /* LCDD16 */
	gpio_disable(DGPIO(5));  /* LCDD17 */

	gpio_disable(CGPIO(0));  /* LCDD2 */
	gpio_disable(CGPIO(1));  /* LCDD3 */
	gpio_disable(CGPIO(2));  /* LCDD4 */
	gpio_disable(CGPIO(3));  /* LCDD5 */
	gpio_disable(CGPIO(4));  /* LCDD6 */
	gpio_disable(CGPIO(5));  /* LCDD7 */
	gpio_disable(CGPIO(6));  /* LCDD10*/
	gpio_disable(CGPIO(7));  /* LCDD11 */
	gpio_disable(CGPIO(8));  /* LCDD12 */
	gpio_disable(CGPIO(9));  /* LCDD13 */
	gpio_disable(CGPIO(10)); /* LCDD14 */
	gpio_disable(CGPIO(11)); /* LCDD15 */
	gpio_disable(CGPIO(12)); /* LCDD18 */
	gpio_disable(CGPIO(13)); /* LCDD19 */
	gpio_disable(CGPIO(14)); /* LCDD20 */
	gpio_disable(CGPIO(15)); /* LCDD21 */
	gpio_disable(CGPIO(16)); /* LCDD22 */
	gpio_disable(CGPIO(17)); /* LCDD23 */
	gpio_disable(CGPIO(18)); /* LPCLK */
	gpio_disable(CGPIO(19)); /* LCDGP0 */
	gpio_disable(CGPIO(20)); /* LCDGP1 */
	gpio_disable(CGPIO(21)); /* LCDGP2 */

	pads_enable("lcdc");
	pads_enable("lcdgp0");
	pads_enable("lcdgp1");
	pads_enable("lcdgp2");

	return rc;
}

int board_nand_init(struct nand_chip *nand)
{
	reset_release("nfc");
	clk_enable("nfc");

	/* NFC data (0-15) */
	gpio_disable(AGPIO(0));
	gpio_disable(AGPIO(1));
	gpio_disable(AGPIO(2));
	gpio_disable(AGPIO(3));
	gpio_disable(AGPIO(4));
	gpio_disable(AGPIO(5));
	gpio_disable(AGPIO(6));
	gpio_disable(AGPIO(7));
	gpio_disable(AGPIO(8));
	gpio_disable(AGPIO(9));
	gpio_disable(AGPIO(10));
	gpio_disable(AGPIO(11));
	gpio_disable(AGPIO(12));
	gpio_disable(AGPIO(13));
	gpio_disable(AGPIO(14));
	gpio_disable(AGPIO(15));

	gpio_disable(AGPIO(16)); /* write-enable */
	gpio_disable(AGPIO(17)); /* read-enable */
	gpio_disable(AGPIO(18)); /* ready1 */
	gpio_disable(AGPIO(19)); /* cle */
	gpio_disable(AGPIO(20)); /* ale */
	gpio_disable(AGPIO(21)); /* cs0 */
	gpio_disable(EGPIO(31)); /* ready2 */

	pads_disable("emmc");
	pads_enable("fc");

	return 0;
}

static int dmw_chip = DMW_CHIP_UNKNOWN;

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

int dmw_detect_chip(void)
{
	int chip, chip_id;

#define DMW_SYSCFG_CHIP_ID 0x1c
	chip_id = readl(DMW_SYSCFG_BASE + DMW_SYSCFG_CHIP_ID);

	switch (chip_id) {
	case 0x0960:
		chip = DMW_CHIP_DMW96; break;
	default:
		chip = -1;
	}

	return chip;
}

int dmw_chip_revision(void)
{
	int ret;

#define DMW_SYSCFG_CHIP_REV 0x20
#define DMW_BOOTROM_BASE		0x01200000
	ret = readl(DMW_SYSCFG_BASE + DMW_SYSCFG_CHIP_REV) & 0xffff;
	if (ret > 0)
		return ret + 1;

	switch (readb(DMW_BOOTROM_BASE + 68)) {
	case 0x30: ret = DMW_CHIP_DMW96_REV_A; break;
	case 0x90: ret = DMW_CHIP_DMW96_REV_B; break;
	default:   ret = -1;
	}

	return ret;
}

#ifdef CONFIG_RECOVERY
int is_recovery(void){
	return !strcmp("recovery", (char *)(DMW_INTRAM_BASE+0x10000 - DMW_RECOVERY_INFOSIZE));
}
#endif

/*****************************************************************************/


void board_spi_init(void)
{
	reset_release("spi1");
	clk_enable("spi1");

	gpio_disable(BGPIO(28)); /* SDO */
	gpio_disable(BGPIO(29)); /* SDI */
	gpio_disable(BGPIO(31)); /* SCLK */

	gpio_disable(BGPIO(30));  /* CS1 */
	gpio_disable(GGPIO(21));  /* CS2 */

	pads_enable("spi1");
}

/*****************************************************************************/

/*****************************************************************************/

static void dmw_usb_init(void)
{
#ifdef CONFIG_USB_DEVICE
#if CONFIG_USB_DWC_PORT==1
	/* take USB1 out of reset */
	reset_release("usb1_phy_por");
	udelay(40);
	reset_release("usb1_phy_port");
	udelay(10);
	reset_release("usb1_mac_phyif");
	udelay(10);
	reset_release("usb_mac");

	/* enable clocks */
	clk_enable("usb_phy");
	clk_enable("usb_mac");
#else
	/* take USB2 out of reset */
	reset_release("usb2_phy_por");
	udelay(40);
	reset_release("usb2_phy_port");
	udelay(10);
	reset_release("usb2_mac_phyif");
	udelay(10);
	reset_release("usb2_mac");

	/* enable clocks */
	clk_enable("usb2_phy");
	clk_enable("usb2_mac");
#endif

	/* activate VBUS_EN */
	gpio_enable(FGPIO(26));
	gpio_direction_output(FGPIO(26), 1);
#endif
}

static void enable_32khz_dp( void )
{
	int reg_val;

	/* Turn on CLK32K_EN bit */
	reg_val = dp52_direct_read(DP52_CMU_SYSCFG);
	reg_val |= 0x0010;
	dp52_direct_write(DP52_CMU_SYSCFG, reg_val);
}

static void dp52_power_setup( void )
{
	int reg_val;

	reg_val = dp52_direct_read(DP52_PMU_EN_SW);

	/* switch wifi power from LDO to DCDC */
	reg_val |= 0x0010;

	/* turn on vcca (analog voltage supply) */
	reg_val |= 0x0004;

	/* turn on vcam (analog voltage supply for camera) */
	reg_val |= 0x0001;

	/* turn off core domain (not used) */
	reg_val &= ~0x0300;

	dp52_direct_write(DP52_PMU_EN_SW, reg_val);
}

/* FIXME: move to the USB driver once it is merged */
static int charger_connected(void)
{
#if defined(CONFIG_DMW_USB_TYPE_CTRL) && defined(CONFIG_DMW_USB_PORT_DETECT)
	gpio_enable(CONFIG_DMW_USB_TYPE_CTRL);
	gpio_direction_output(CONFIG_DMW_USB_TYPE_CTRL, 1);

	gpio_enable(CONFIG_DMW_USB_PORT_DETECT);
	gpio_direction_input(CONFIG_DMW_USB_PORT_DETECT);
	gpio_pull_ctrl(CONFIG_DMW_USB_PORT_DETECT,1, 0);

	mdelay(10);
	
	return (gpio_get_value(CONFIG_DMW_USB_PORT_DETECT));
#else
	return 1;
#endif
}

static void rtc_init(void){

	reset_release("rtc");
	clk_enable("rtc");
}

void timer__init(void);

/*
 * Miscellaneous platform dependent initialisations
 */
int board_early_init_f(void)
{
	dmw_chip = dmw_detect_chip();

	gpio_enable(CONFIG_DMW_OVERDRIVE_GPIO);
#ifdef CONFIG_DMW_OVERDRIVE
	gpio_direction_output(CONFIG_DMW_OVERDRIVE_GPIO, 1);
#else
	gpio_direction_output(CONFIG_DMW_OVERDRIVE_GPIO, 0);
#endif
	clk_init();

	return 0;
}

#if defined(DMW_BOARD_EVB)
static void dmw_board_evb_check_cpld(void)
{
#if defined(CONFIG_HARD_I2C) || defined(CONFIG_SOFT_I2C)
	char str[128];
	u8 cpld_version;
	uchar status;

	i2c_set_bus_num(EVB96_CPLD_I2C_BUS);
	status = i2c_read(EVB96_CPLD_I2C_ADDR, 0, 7, &cpld_version, 1);

	/* in case we can't read - version = 0 */
	if (status) {
		cpld_version = 0;
		sprintf(str, "CPLD:  can't read version! please burn new CPLD version\n");
	}
	else
		sprintf(str, "CPLD:  version: %d\n", cpld_version);

	puts(str);

	/* at version 9 and up - need to write register 0x28 in order to enable bt-coexistence */
	if (cpld_version >= 9) {
		u8 dummy = 0;
		i2c_write(EVB96_CPLD_I2C_ADDR, 0x28, 7, &dummy, 1);
	}

	/* only cpld version 9 and up support btcoexist */
	setenv("btcoexist_feature", cpld_version >= 9 ? ",btcoexist" : "");
#else
	#error I2C is not enabled!!!
#endif
}
#endif

int board_init(void)
{
	ccu_init();

	/* audio PA on newer CPLD versions and IMH3-B */
	gpio_enable(CGPIO(26));
	gpio_direction_output(CGPIO(26), 0);

	gpio_direction_output(CONFIG_DMW_GPIO_CAM_FRONT_RESET, 0);
	gpio_direction_output(CONFIG_DMW_GPIO_CAM_FRONT_PWDN,  1);

	board_spi_init();
	spi_init();

	if (dp52_init() < 0)
		printf("\nPlease press dp52 on/off switch ...\n\n");
	do {
		mdelay(100);
	} while (dp52_init() < 0);

	/* enable 32Khz by DP52 */
	enable_32khz_dp();

	/* enable capacitator backing the dp52-rtc */
	dp52_indirect_write(DP52_PMU_IND_BAKPORLVL, 0x92);

	/* dp52 power setting */
	dp52_power_setup();

	/* make sure charger is off */
	dp52_direct_write_field(DP52_PWM0_CFG2, 13, 1, 0); /* disable PWM0 */
	dp52_direct_write_field(DP52_PWM0_CRGTMR_CFG, 7, 1, 0); /* disable timer */

#ifdef CONFIG_PNX8181_I2C
	board_i2c_init();
#endif

#ifdef CONFIG_DMW_VIBRATOR
	/* enable vibrator */
	gpio_direction_output(CONFIG_DMW_VIBRATOR_GPIO, 1);
	gpio_enable(CONFIG_DMW_VIBRATOR_GPIO);
#endif

	gd->bd->bi_arch_number = MACH_TYPE_DMW96;

	/* adress of boot parameters */
	gd->bd->bi_boot_params = gd->bd->bi_dram[0].start + 0x100;

	icache_enable();

	dmw_keypad_setup();

	dmw_lcd_init();

	dmw_usb_init();

	irq_init();

	rtc_init();

	return 0;
}

#ifdef CONFIG_CONTROL_BOOT
#ifdef CONFIG_USBD_DFU
int dfu_key_pressed = 0;
int dfu_key_press_duration = 0;
int dfu_enter_timeout;
int dfu_enter_keyboard_row, dfu_enter_keyboard_col;
#endif

int recovery_key_pressed = 0;
int recovery_key_press_duration = 0;
int recovery_enter_timeout;
int recovery_enter_keyboard_row, recovery_enter_keyboard_col;
#endif

int board_late_init(void)
{
	int res;
#ifdef CONFIG_CONTROL_BOOT
	char *tmp;

#ifdef CONFIG_USBD_DFU
	tmp = getenv("dfu_enter_timeout");
	dfu_enter_timeout = tmp ? simple_strtoul(tmp, NULL, 10) : 0;
	dfu_enter_timeout *= 1000; /* convert to ms */

	tmp = getenv("dfu_enter_keyboard_row");
	dfu_enter_keyboard_row = tmp ? simple_strtoul(tmp, NULL, 10) : -1;

	tmp = getenv("dfu_enter_keyboard_col");
	dfu_enter_keyboard_col = tmp ? simple_strtoul(tmp, NULL, 10) : -1;
#endif

	tmp = getenv("recovery_enter_timeout");
	recovery_enter_timeout = tmp ? simple_strtoul(tmp, NULL, 10) : 0;
	recovery_enter_timeout *= 1000; /* convert to ms */

	tmp = getenv("recovery_enter_keyboard_row");
	recovery_enter_keyboard_row = tmp ? simple_strtoul(tmp, NULL, 10) : -1;

	tmp = getenv("recovery_enter_keyboard_col");
	recovery_enter_keyboard_col = tmp ? simple_strtoul(tmp, NULL, 10) : -1;
#endif

#if defined(DMW_BOARD_EVB)
	dmw_board_evb_check_cpld();
#endif

	res = dp52_late_init();

	if (!res) {
		/*
		 * Check if external power is connected and that it's indeed a charger
		 * (and not a plain USB host). Start charging in this case, otherwise
		 * try to continue booting.
		 */
		if (dp52_charger_connected()) {
			unsigned int is_fast = charger_connected();
			printf("External power is connected. Entering charging %s mode...\n", (is_fast ? "fast" : "slow") );
			dp52_charge(1,is_fast);	/* turn off if charger is disconnected */
		} else {
			int state = dp52_check_battery();

			if (state) {
				switch (state) {
				case 1:
					printf("Battery thermal limit!");
					break;
				case 2:
					printf("Battery capacity too low!");
					break;
				}

				printf(" Shutdown...\n");
				dp52_power_off();
			}
		}
	}

#ifdef CONFIG_BOOTMENU
	dmw_bootmenu();
#endif
	dmw_keypad_late_init();

	return 0;
}

int dram_init (void)
{
	/* calculate from denali registers */
	uint32_t reg;
	reg = readl(DMW96_SDC_BASE + DMW96_SDC_SIZE_OFFSET*4);
	
	if (reg & DMW96_DDR_4Gb_MASK) {
		gd->ram_size = 256 * SZ_1M;
	}
	else if (reg & DMW96_DDR_2Gb_MASK) {
		gd->ram_size = 512 * SZ_1M;
	}
	else {
		printf("DRAM size is wrong!\n");
		return -1;
	}
	return 0;
}

#ifndef CONFIG_SYS_NO_FLASH
ulong board_flash_get_legacy(ulong base, int banknum, flash_info_t *info)
{
	info->portwidth = FLASH_CFI_16BIT;
	info->chipwidth = FLASH_CFI_BY16;
	info->interface = FLASH_CFI_X16;
	return 1;
}
#endif

#ifdef CONFIG_MMC
static int mmc_get_card_detect(void)
{
	return !gpio_get_value(CONFIG_DMW_GPIO_MMC_DETECT);
}

static void mmc_init_gpio(int gpio)
{
	gpio_disable(gpio);
	gpio_pull_ctrl(gpio, 1, 1);
}

static struct syndwc_pdata syndwc_pdata = {
	.card_num    = 0,
	.mclk        = 48000000,
	.f_max       = 24000000,
	.host_caps   = MMC_MODE_4BIT | MMC_MODE_HS,
	.voltages    = MMC_VDD_32_33 | MMC_VDD_33_34 | MMC_VDD_165_195,
	.card_detect = &mmc_get_card_detect,
};

#ifdef CONFIG_EMMCBOOT
int mmc_get_env_addr(struct mmc *mmc, u32 *env_addr)
{
	int ret, i = 1;

	/* try to find the u-boot environment partition */
	do {
		disk_partition_t info;

		ret = get_partition_info(&mmc->block_dev, i++, &info);
		if (!ret && !strcmp(info.type, BOOT_PART_ENV)) {
			if (info.size * 512 < CONFIG_ENV_SIZE) {
				printf("ERROR: Environment partition too small!\n");
				return 1;
			}
			*env_addr = info.start * 512;
			return 0;
		}
	} while (ret == 0);

	printf("ERROR: No environment partition found! "
	       "Use gptpart to create one...\n");

	return 1;
}

static int mmc_card_detect_always(void)
{
	return 1;
}

static struct syndwc_pdata syndwc_pdata_emmc = {
	.card_num    = 1,
	.mclk        = 48000000,
	.f_max       = 10000000,
	.host_caps   = MMC_MODE_4BIT | MMC_MODE_8BIT | MMC_MODE_HS,
	.voltages    = MMC_VDD_32_33 | MMC_VDD_33_34 | MMC_VDD_165_195,
	.card_detect = &mmc_card_detect_always,
};
#endif

int board_mmc_init(bd_t *bis)
{
	unsigned long value;

	reset_release("sdmmc");
	clk_enable("sdmmc");

	/* set output to 3.3v */
	value = dp52_direct_read(DP52_PMU_LVL2_SW);
	value &= ~(0x1f << 5);
	value |= (18 << 5);
	dp52_direct_write(DP52_PMU_LVL2_SW, value);

	/* enable SD power (LdoSp2) */
	value = dp52_direct_read(DP52_PMU_EN_SW);
	dp52_direct_write(DP52_PMU_EN_SW, value | DP52_PMU_LDO_SP2_EN);

	/* configure GPIOs */
	mmc_init_gpio(DGPIO(18)); /* D0 */
	mmc_init_gpio(DGPIO(19)); /* D1 */
	mmc_init_gpio(DGPIO(20)); /* D2 */
	mmc_init_gpio(DGPIO(21)); /* D3 */
	mmc_init_gpio(DGPIO(22)); /* CKL */
	mmc_init_gpio(DGPIO(23)); /* CMD */

	pads_enable("sdmmc");

	gpio_direction_input(CONFIG_DMW_GPIO_MMC_DETECT);
	gpio_pull_ctrl(CONFIG_DMW_GPIO_MMC_DETECT, 1, 1);
	gpio_enable(CONFIG_DMW_GPIO_MMC_DETECT);

#ifdef CONFIG_EMMCBOOT
	mmc_init_gpio(AGPIO(0)); /* D1 */
	mmc_init_gpio(AGPIO(1)); /* D3 */
	mmc_init_gpio(AGPIO(2)); /* D5 */
	mmc_init_gpio(AGPIO(3)); /* D7 */
	mmc_init_gpio(AGPIO(8)); /* D0 */
	mmc_init_gpio(AGPIO(9)); /* D2 */
	mmc_init_gpio(AGPIO(10)); /* D4 */
	mmc_init_gpio(AGPIO(11)); /* D6 */
	mmc_init_gpio(AGPIO(18)); /* CLK */
	mmc_init_gpio(AGPIO(19)); /* RST */
	mmc_init_gpio(EGPIO(31)); /* CMD */

	pads_disable("fc");
	pads_enable("emmc");

	syndwc_initialize(&syndwc_pdata_emmc);
#endif

	return syndwc_initialize(&syndwc_pdata);
}
#endif

#ifdef CONFIG_DISPLAY_CPUINFO
static void print_core_freq(void)
{
	char str[32];

	sprintf(str, "%lu Mhz (PCLK@%lu)",
	        dmw_get_cpufreq() / 1000000, dmw_get_pclk()/1000000);
	puts(str);
}

int print_cpuinfo(void)
{
	puts("CPU:   ");

	switch (dmw_chip) {
	case 96:
		puts("DMW96: ");
		print_core_freq();
		puts("\n");
		break;
	default: puts("unknown\n");
	}

	return 0;
}
#endif

#ifdef CONFIG_DISPLAY_BOARDINFO
int checkboard(void)
{
	int dram_class = (readl(DMW96_SDC_BASE) >> 8) & 0xf;
	char str[32];

	puts("MEM:   ");
	sprintf(str, "%sDDR%d (%luMHz)\n",
	        (dram_class & 0x1) ? "LP" : "",
	        (dram_class & 0x4) ? 2 : 1,
		dmw_get_dramfreq()/1000000);
	puts(str);

	puts("BOARD: " CONFIG_DMW_BOARDID "\n");
	return 0;
}
#endif

extern char *simple_itoa(unsigned int i);

#ifdef CONFIG_USBD_DFU
int dfu_enter_dfu_mode(void);

int do_dfu(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
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
	setenv("verify", "n");

#ifdef CONFIG_DMW_VIBRATOR
	/* disable vibrator */
	gpio_set_value(CONFIG_DMW_VIBRATOR_GPIO, 0);
#endif

	return 0;
}

/*
 * Write the CMU S/W reset register to cause reset
 */
#define DMW_CMU_RSTSR    0x48
#define DMW_CMU_SWSYSRST 0x40

void reset_cpu(unsigned long addr)
{
	uint32_t reg;

	/* clear SWSYSRSTIND and SWWRMRSTIND before SW resetting the DMW */
	reg = readl(DMW_CMU_BASE + DMW_CMU_RSTSR);
	reg |= ((1<<5) | (1<<4));
	writel(reg, DMW_CMU_BASE + DMW_CMU_RSTSR);

	writel(0x7496398b, DMW_CMU_BASE + DMW_CMU_SWSYSRST);
	writel(0xb5c9c674, DMW_CMU_BASE + DMW_CMU_SWSYSRST);
}

#ifdef CONFIG_REVISION_TAG
u32 get_board_rev(void)
{
	switch (dmw_chip) {
	case DMW_CHIP_DMW96:
		return 0x9600;
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

int do_cfg_mac_addr(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char mac[20];
	unsigned long long addr = 0x009696969600ULL + simple_strtol(argv[1], NULL, 10);

	sprintf(mac, "%02llX:%02llX:%02llX:%02llX:%02llX:%02llX",
		(addr >> 40) & 0xff, (addr >> 32) & 0xff, (addr >> 24) & 0xff,
		(addr >> 16) & 0xff, (addr >>  8) & 0xff,  addr        & 0xff);
	setenv ("ethaddr", mac);

#ifndef CONFIG_ENV_IS_NOWHERE
	saveenv();
#endif

	return 0;
}

U_BOOT_CMD(cfg_mac_addr, 2, 0, do_cfg_mac_addr,
	"cfg_mac_addr - Configure Ethernet MAC address.\n",
	""
);

#ifdef CONFIG_CONTROL_BOOT
void control_boot(int *abort, int *bootdelay)
{
#ifdef CONFIG_USBD_DFU
	if (dfu_enter_timeout &&
	    (dmw_keypad_scan(dfu_enter_keyboard_row,
	                     dfu_enter_keyboard_col))) {
		if (dfu_key_pressed == 0) {
			dfu_key_press_duration = 0;
			dfu_key_pressed = 1;
		} else if (dfu_key_pressed > 0) {
			dfu_key_press_duration += 10;
			if (*bootdelay <= 1)
				(*bootdelay)++;
			if (dfu_key_press_duration >= dfu_enter_timeout) {
				dfu_enter_dfu_mode();
				*abort = 1;
				*bootdelay = 0;
			}
		}
	} else {
		if (dfu_key_pressed > 0)
			dfu_key_pressed = -1;
	}
#endif

	if (recovery_enter_timeout &&
		(dmw_keypad_scan(recovery_enter_keyboard_row,
		                 recovery_enter_keyboard_col))) {
		if (recovery_key_pressed == 0) {
			recovery_key_press_duration = 0;
			recovery_key_pressed = 1;
		} else if (recovery_key_pressed > 0) {
			recovery_key_press_duration += 10;
			if (*bootdelay <= 1)
				(*bootdelay)++;
			if (recovery_key_press_duration >= recovery_enter_timeout) {
				setenv("boot_mode", "recovery");
				*bootdelay = 0;
			}
		}
	} else {
		if (recovery_key_pressed > 0)
			recovery_key_pressed = -1;
	}
}
#endif
