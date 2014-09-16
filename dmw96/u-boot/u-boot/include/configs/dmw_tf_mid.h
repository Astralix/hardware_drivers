#ifndef __BOARD_DSPG_DMW_CONFIGS_BOARD_H
#define __BOARD_DSPG_DMW_CONFIGS_BOARD_H

#include <asm/arch/gpio.h>

/* define board type */
#define DMW_BOARD_TFEVB
#define CONFIG_DMW_BOARDID "tf-mid"

/* general options */
#define CONFIG_DMW_OVERDRIVE
#define CONFIG_DMW_OVERDRIVE_GPIO	(GGPIO( 9))
#undef  CONFIG_DMW_LCDONLY
#define CONFIG_DMW_DP52
#define CONFIG_DMW_CHARGER
#define CONFIG_DMW_VIBRATOR
#define CONFIG_DMW_VIBRATOR_GPIO	(FGPIO(27))
#define CONFIG_DMW_DISPLAY_LOGO
#undef  CONFIG_DMW_USB_PORT_DETECT
#undef  CONFIG_DMW_USB_TYPE_CTRL

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

/* Front Camera GPIO mappig */
#define CONFIG_DMW_GPIO_CAM_FRONT_RESET		(GGPIO(14))
#define CONFIG_DMW_GPIO_CAM_FRONT_PWDN		(AGPIO(15))

/* DP52 wiring */
#define CONFIG_DP52_ICHG_DCIN  1
#define CONFIG_DP52_ICHG_R     50	/* in mOhm */
#define CONFIG_DP52_VBAT_DCIN  3
#define CONFIG_DP52_VBAT_R1    1000
#define CONFIG_DP52_VBAT_R2    330
#undef  CONFIG_DP52_TEMP_DCIN		/* No NTC? */
#define CONFIG_DP52_TEMP_R     10000	/* in Ohm */

/* Default battery parameters */
#define DP52_BATTERY_DEFAULT \
	.name = "PTI PL057193P", \
	.vmin = 3000, \
	.vmax = 4200, \
	.temp_min_charge = 0, \
	.temp_max_charge = 45, \
	.temp_min_discharge = -20, \
	.temp_max_discharge = 60, \
	.chrg_voltage = 4200, \
	.chrg_current = 1975, \
	.chrg_cutoff_curr = 40, \
	.chrg_cutoff_time = 300, \
	.ntc_r = 10000, /* No NTC? */ \
	.ntc_b = 3435, /* No NTC? */ \
	.ntc_t = 25, /* No NTC? */ \
	.v1 = 3580, \
	.c1 = 7, \
	.v2 = 3770, \
	.c2 = 54,

#include <configs/dmw.h>

#endif
