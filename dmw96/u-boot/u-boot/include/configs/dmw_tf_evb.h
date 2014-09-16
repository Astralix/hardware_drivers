#ifndef __BOARD_DSPG_DMW_CONFIGS_BOARD_H
#define __BOARD_DSPG_DMW_CONFIGS_BOARD_H

#include <asm/arch/gpio.h>

/* define board type */
#define DMW_BOARD_TFEVB
#define CONFIG_DMW_BOARDID "tf-evb"

/* general options */
#define CONFIG_DMW_OVERDRIVE
#define CONFIG_DMW_OVERDRIVE_GPIO	(GGPIO( 9))
#undef  CONFIG_DMW_LCDONLY
#define CONFIG_DMW_DP52
#define CONFIG_DMW_CHARGER
#define CONFIG_DMW_VIBRATOR
#define CONFIG_DMW_VIBRATOR_GPIO	(FGPIO(25))
#define CONFIG_DMW_DISPLAY_LOGO
#define CONFIG_DMW_USB_PORT_DETECT	(AGPIO(28))
#define CONFIG_DMW_USB_TYPE_CTRL	(AGPIO(29))

/* default lcd */
#define CONFIG_DMW_LCDCTYPE "innolux_at070tn90"

/* spi */
#define CONFIG_DMW_AMOLED_SPI_UNIT	0		/* SPI1 */
#define CONFIG_DMW_DP52_SPI_UNIT	0		/* SPI1 */
#define CONFIG_DMW_GPIO_DP52_CS		0		/* BGPIO(30) */
#define CONFIG_DMW_GPIO_AMOLED_CS	1		/* GGPIO(21) */

/* gpio mapping */
#define CONFIG_DMW_GPIO_MMC_DETECT      (FGPIO(19))
#define CONFIG_DMW_GPIO_AMOLED_RESET    (FGPIO(23))
#define CONFIG_DMW_GPIO_MII_RESET       (AGPIO(26))
#define CONFIG_DMW_GPIO_USB_INT         (FGPIO(20))
#define CONFIG_DMW_GPIO_USB_5VEN        (FGPIO(26))
#define CONFIG_DMW_GPIO_USB_PTYPE_DET   (AGPIO(28))
#define CONFIG_DMW_GPIO_USB_PTYPE_FET   (AGPIO(29))

/* Front Camera GPIO mappig */
#define CONFIG_DMW_GPIO_CAM_FRONT_RESET		(GGPIO(14))
#define CONFIG_DMW_GPIO_CAM_FRONT_PWDN		(FGPIO(5))

/* DP52 wiring */
#define CONFIG_DP52_ICHG_DCIN  1
#define CONFIG_DP52_ICHG_R     50	/* in mOhm */
#define CONFIG_DP52_VBAT_DCIN  3
#define CONFIG_DP52_VBAT_R1    1000
#define CONFIG_DP52_VBAT_R2    330
#define CONFIG_DP52_TEMP_DCIN  0
#define CONFIG_DP52_TEMP_R     39000	/* in Ohm */

/* Default battery parameters */
#define DP52_BATTERY_DEFAULT \
	.name = "VARTA EasyPack EZPack L", \
	.vmin = 3200, \
	.vmax = 4130, \
	.temp_min_charge = 0, \
	.temp_max_charge = 50, \
	.temp_min_discharge = -20, \
	.temp_max_discharge = 60, \
	.chrg_voltage = 4200, \
	.chrg_current = 350, \
	.chrg_cutoff_curr = 30, \
	.chrg_cutoff_time = 90, \
	.ntc_r = 10000, \
	.ntc_b = 3435, \
	.ntc_t = 25, \
	.v1 = 3580, \
	.c1 = 7, \
	.v2 = 3770, \
	.c2 = 54,

#include <configs/dmw.h>

#endif
