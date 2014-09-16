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
#include <asm/byteorder.h>
#include <asm/io.h>

#include <image.h>
#include <u-boot/zlib.h>

#include "dw.h"
#include "dp52.h"
#include "charger.h"

/*****************************************************************************/
#define DP_DEFAULT_SAMPLERATE		0x7

#ifdef RUN_CHARGER_FROM_SRAM
  #define DEBUG(x...)
  #define CLOCK_FREQ	32000
  #define dp52_direct_write(x...) charger_dp52_direct_write(x)
  #define dp52_direct_read(x...) charger_dp52_direct_read(x)
#else
  #define DEBUG(x...) printf(x)
  #define CLOCK_FREQ	240000000
#endif

static inline void private_mdelay(unsigned long msec)
{
	unsigned long msec_loops = CLOCK_FREQ/4000;

	__asm__ volatile (
		"	mov %[loops_counter], %[loops]\n"
		"	adds %[loops_counter], %[loops_counter], #1\n"
		"1:	mov %[inloops_cnt], %[inloops]\n"
		"	adds %[inloops_cnt], %[inloops_cnt], #1\n"
		"2:	subs %[inloops_cnt], %[inloops_cnt], #1\n"
		"	bne 2b\n"
		"	subs %[loops_counter], %[loops_counter], #1\n"
		"	bne	1b\n"
		: [loops_counter]"=&r" (msec), [inloops_cnt]"=&r" (msec_loops)
		: [loops]"r" (msec), [inloops]"r" (msec_loops)
	);
}

typedef enum charger_state_e
{
	/* charging disabled */
	CS_DISABLED = 0,
	/* force 3.8v charging */
	CS_START38,
	/* charging completely stopped due to reaching thermal limits */
	CS_THERMAL,
	/* fast charging to 4.2v, under normal conditions */
	CS_FAST42,
	/* fast charging to 3.8v, under high/low temps */
	CS_FAST38,
	/* topoff in 4.2v */
	CS_TOPOFF42,
	/* topoff in 3.8v */
	CS_TOPOFF38,
	/* monitoring in 4.2v */
	CS_MONITOR42,
	/* monitoring in 3.8v */
	CS_MONITOR38
} charger_state;

/*****************************************************************************/

void dp_clr_bits(uint16_t addr, uint16_t bits)
{
	uint16_t value;

	value = dp52_direct_read(addr);
	dp52_direct_write(addr, value & ~bits);
}

void dp_set_bits(uint16_t addr, uint16_t bits)
{
	bits |= dp52_direct_read(addr);
	dp52_direct_write(addr, bits);
}

/*****************************************************************************/

/* Motorola BF5X battery thermistor table *
 *
 * This table was created with the following assumptions:
 * 1. DPLDIO = 3.3V
 * 2. DCIN0 Constant resistor value = 68 kOhm
 * */
struct battery_thermistor battery_thermistor_BF5X = {
	.voltage_mul_10000 =       {27348, 25652, 23697, 21539, 19261, 18331, 16943, 14691, 12565, 10634,
								 8920,  7439,  6174,  5114,  4231,  3502,  2902,  2409,  2006,  1675,
								 1403,  1264,  1180,   995,   842,   716,   612,   524,   450,   388,
								  337,   292,   255,   223,   196,   173},
	.temperature_in_c_mul_10 = { -400,  -350,  -300,  -250,  -200,  -180,  -150,  -100,   -50,     0,
								   50,   100,   150,   200,   250,   300,   350,   400,   450,   500,
								  550,   580,   600,   650,   700,   750,   800,   850,   900,   950,
								 1000,  1050,  1100,  1150,  1200,  1250},
	.table_length = 36,
};

signed long charger_v_to_c( u16 voltage_mul_10000 )
{
	int index = 0;

	struct battery_thermistor *bat_therm = &battery_thermistor_BF5X;

	// go over the thermistor table and find the correct range we are in
	while (index < bat_therm->table_length)
	{
		if (voltage_mul_10000 > bat_therm->voltage_mul_10000[index])
		{
			if (index == 0) {
				// report temp below the temp of the current index (for now just do temp-10 (-1c))
				return bat_therm->temperature_in_c_mul_10[index]-10;
			}
			else {
				// return the middle point of the current temperature range we are in
				signed long temp_range = bat_therm->temperature_in_c_mul_10[index] - bat_therm->temperature_in_c_mul_10[index-1];
				return bat_therm->temperature_in_c_mul_10[index-1] + (temp_range >> 1);
			}
		}

		index++;
	}

	// report temp above the temp of the last element (for now just do temp+10 (+1c))
	return bat_therm->temperature_in_c_mul_10[index-1]+10;
}


/* check for hard thermal limit (<-20c or >60c) */
u8 dp_charger_check_hard_thermal_limit(signed int temp)
{
	/*-----------------------------
	 - below -20c, or above 60c,
	 - stop the charge and wait for
	 - temp to return to normal
	 -----------------------------*/

	if ((temp < -200) || (temp > 600))
	{
		return 1;
	}

	return 0;
}

/* check for hard thermal unlimit point (-20c or 60c) */
u8 dp_charger_check_hard_thermal_unlimit(signed int temp)
{
	/*-----------------------------
	 - above -20c and below 60c,
	 - reenable charging
	 -----------------------------*/

	if ((temp >= -200) && (temp <= 600))
	{
		return 1;
	}

	return 0;
}

/* check for soft thermal limit (between -20c and 0c, or 45c and 60c) */
u8 dp_charger_check_soft_thermal_limit(signed int temp)
{
	/*-----------------------------
	 - below 0c, or above 45c,
	 - limit to 3.8v
	 -----------------------------*/

	if ((temp < 0) || (temp > 450))
	{
		return 1;
	}

	return 0;
}

/* check for soft thermal unlimit point (0 or 45c) */
u8 dp_charger_check_soft_thermal_unlimit(signed int temp)
{
	/*-----------------------------
	 - above 0c and below 45c,
	 - reenable charging
	 -----------------------------*/

	if ((temp >= 0) && (temp <= 450))
	{
		return 1;
	}

	return 0;
}

void do_dpm_fastcharge(unsigned int auxbgp, unsigned int atten42, unsigned int atten38, unsigned int tempref, unsigned int min_mv)
{
	u16 		  pushCounter = 0;
	u32 		  temp_counter = 0;
	charger_state state;
	signed int    temp;

	DEBUG("entering fastcharge: (auxbgp = %d, atten42 = %d, atten38 = %d, tempref = %d, min_mv = %d)\n",
			auxbgp,
			atten42,
			atten38,
			tempref,
			min_mv);

	/* reset */
	state = CS_DISABLED;

	/* initial temperature read */
	temp = get_temperature(auxbgp, tempref);

	// Main charger loop
	while ( 1 )
	{
		/*-----------------------------
		 - read temperature
		 -----------------------------*/

		/* temp measurement activation counter */
		if (50000 == temp_counter)
		{
			/* read temperature */
			temp = get_temperature(auxbgp, tempref);

			temp_counter = 0;
		}

		temp_counter++;

		/*-----------------------------
		 - enforce hard thermal limit,
		 - below -20c and above 60c
		 -----------------------------*/

		if ( dp_charger_check_hard_thermal_limit( temp ) )
		{
			/* if not already disabled or thermaled */
			switch ( state )
			{
				/* if disabled, move to thermal */
				case CS_DISABLED:
				case CS_START38:
					DEBUG("temp %d, hard thermal stop\n", temp);

					state = CS_THERMAL;
					break;

				/* if in thermal, do nothing */
				case CS_THERMAL:
					break;

				/* any other state, disable charging and move to thermal */
				default:
					DEBUG("temp %d, hard thermal stop\n", temp);

					/* disable charging */
					state = CS_THERMAL;
					dp_charger_reset();

					break;
			}
		}

		/*-----------------------------
		 - state machine
		 -----------------------------*/

		switch ( state )
		{
			/* disabled, start charging */
			case CS_DISABLED:

				/* check soft limit, if encountered start charging to 3.8v */
				if ( dp_charger_check_soft_thermal_limit( temp ) )
				{
					DEBUG("CS_DISABLED: switching to fastcharge 3.8v mode (auxbgp = %d, atten38 = %d)\n", auxbgp, atten38);

					state = CS_START38;
				}
				/* otherwise normal start to 4.2v */
				else
				{
					DEBUG("CS_DISABLED: switching to fastcharge 4.2v mode (auxbgp = %d, atten42 = %d)\n", auxbgp, atten42);

					/* start charging with comparator at 4.2v */
					dp_charger_init(auxbgp, atten42, 1);
					dp_charger_set();

					state = CS_FAST42;
				}

				break;

			/* start 3.8v charging sequence */
			case CS_START38:

				/* start charging with the comparator gain set to trigger at 3.8v.
				 * we need to use 1.25x gain to trigger at 3.8v, as opposed to 1x at 4.2v */
				dp_charger_init(auxbgp, atten38, 2);
				dp_charger_set();

				/* move to 3.8v fast charge */
				state = CS_FAST38;

				break;

			/* thermal hard shutoff */
			case CS_THERMAL:

				/* check hard thermal unlimit point */
				if ( dp_charger_check_hard_thermal_unlimit( temp ) )
				{
					DEBUG("CS_THERMAL: temp %d, hard thermal cancel, restarting\n", temp);

					/* move into disabled state and let the state machine work normally */
					state = CS_DISABLED;
				}

				// TODO sleep

				break;

			/*------------------------
			 - 4.2v, full charging
			 ------------------------*/

			/* fast charging to 4.2v */
			case CS_FAST42:

				/* if got to thermal limit, move to charging at 3.8v */
				if ( dp_charger_check_soft_thermal_limit( temp ) )
				{
					DEBUG("CS_FAST42: temp %d, soft thermal, restarting at 3.8v\n", temp);

					/* stop charger */
					dp_charger_reset();

					state = CS_START38;
				}
				else
				{
					/* if comparator hit value, move to top off */
					if (dp52_direct_read(DP52_AUX_RAWPMINTSTAT) & (1<<3))
					{
						DEBUG("CS_FAST42: switching to 4.2v top-off mode\n");

						state = CS_TOPOFF42;
					}
				}

				break;

			/* 4.2v topoff */
			case CS_TOPOFF42:

				/* if got to thermal limit, move to charging at 3.8v */
				if ( dp_charger_check_soft_thermal_limit( temp ) )
				{
					DEBUG("CS_TOPOFF42: temp %d, soft thermal, restarting at 3.8v\n", temp);

					/* stop charger */
					dp_charger_reset();

					state = CS_START38;
				}
				else
				{
					/* check timer */
					if (dp52_direct_read(DP52_PWM0_CRGTMR_CFG) & (1<<4))
					{
						// Timer expired, charger disabled - go to monitor mode
						DEBUG("CS_TOPOFF42: switching to monitor mode\n");
						state = CS_MONITOR42;
					}
				}

				break;

			/* monitoring at 4.2v */
			case CS_MONITOR42:

				/* if got to thermal limit, move to charging at 3.8v */
				if ( dp_charger_check_soft_thermal_limit( temp ) )
				{
					DEBUG("CS_MONITOR42: temp %d, soft thermal, restarting at 3.8v\n", temp);

					/* stop charger */
					dp_charger_reset();

					state = CS_START38;
				}
				else
				{
					/* check voltage level */
					if (get_mVoltage(auxbgp, atten42) < 4000)
					{
						// Voltage dropped below threshold, start charger.
						DEBUG("CS_MONITOR42: voltage dropped below 4.0v, starting charger again\n");

						// disable timer
						dp_clr_bits(DP52_PWM0_CRGTMR_CFG, 0x80);
						state = CS_DISABLED;
					}
				}

				break;

			/*------------------------
			 - 3.8v, thermally limited
			 - at temps between -20c
			 - and -10c or 45c and 60c
			 ------------------------*/

			/* fast charging to 3.8v */
			case CS_FAST38:

				/* if found soft unlimit point, disable charger and let the process restart
				 * at 4.2v */
				if ( dp_charger_check_soft_thermal_unlimit( temp ) )
				{
					DEBUG("CS_FAST38: temp %d, soft thermal unlimit, restarting at 4.2v\n", temp);

					dp_charger_reset();
					state = CS_DISABLED;
				}
				else
				{
					/* if comparator hit value, move to top off */
					if (dp52_direct_read(DP52_AUX_RAWPMINTSTAT) & (1<<3))
					{
						DEBUG("CS_FAST38: switching to 3.8v top-off mode\n");

						state = CS_TOPOFF38;
					}
				}

				break;

			/* 3.8v topoff */
			case CS_TOPOFF38:

				/* if found soft unlimit point, disable charger and let the process restart
				 * at 4.2v */
				if ( dp_charger_check_soft_thermal_unlimit( temp ) )
				{
					DEBUG("CS_TOPOFF38: temp %d, soft thermal unlimit, restarting at 4.2v\n", temp);

					dp_charger_reset();
					state = CS_DISABLED;
				}
				else
				{
					/* check timer */
					if (dp52_direct_read(DP52_PWM0_CRGTMR_CFG) & (1<<4))
					{
						// Timer expired, charger disabled - go to monitor mode
						DEBUG("CS_TOPOFF38: switching to monitor mode\n");
						state = CS_MONITOR38;
					}
				}

				break;

			/* monitoring at 3.8v */
			case CS_MONITOR38:

				/* if found soft unlimit point, disable charger and let the process restart
				 * at 4.2v */
				if ( dp_charger_check_soft_thermal_unlimit( temp ) )
				{
					DEBUG("CS_MONITOR38: temp %d, soft thermal unlimit, restarting at 4.2v\n", temp);

					dp_charger_reset();
					state = CS_DISABLED;
				}
				else
				{
					/* check voltage level */
					if (get_mVoltage(auxbgp, atten38) < 3750)
					{
						// Voltage dropped below threshold, start charger.
						DEBUG("CS_MONITOR38: voltage dropped below 3.7v, starting charger again\n");

						// disable timer
						dp_clr_bits(DP52_PWM0_CRGTMR_CFG, 0x80);

						/* restart at 3.8v */
						state = CS_START38;
					}
				}

				break;
		}

		/*----------------------------------
		 - check external power disconnect
		 - or button press
		 ----------------------------------*/

		// Check for external power
		if (!(dp52_direct_read(DP52_PMURAWSTAT) & (1 << 1)))	// power plugged out
		{
			DEBUG("External power out, shutdown.\n");
			power_off();
		}

		// Check for ON button
		if (dp52_direct_read(DP52_PMURAWSTAT) & 1)
		{
			if (pushCounter++ >= 5)
			{
				if (get_mVoltage(auxbgp, atten42) < min_mv)
				{
					// Cannot power the device on (do nothing)
					DEBUG("Power too low, continue charging.\n");
					pushCounter=0;
					continue;
				}
				else
				{
					DEBUG("Power ok, boot kernel.\n");
					//dp_charger_reset();
					return; // start kernel
				}
			}
		}
		else pushCounter = 0;
	}
}

int dp_charger_init(int auxbgp, int atten, int gain)
{
	// Enable auxilliary module
	dp_aux_enable(auxbgp);

	// set interrupt masks for DCIN3 and DCINS
	dp_clr_bits(DP52_PMUSTAT, 0xffff);
	dp_clr_bits(DP52_AUX_INTSTAT, 0xffff);
	dp_clr_bits(DP52_ICU_STAT, 0x8); // can be removed (RO)?

	// Open the analog init
	dp_set_bits(DP52_AUX_EN, 0xa0);

	// Set gain compare and current limit
	dp52_direct_write(DP52_AUX_DC3GAINCNT1, (atten << 5) | gain);
	dp52_direct_write(DP52_AUX_DC1GAINCNT1, 0x05cd);		// current limit

	// Set bandgap tune value
	dp_clr_bits(DP52_AUX_ADCTRL, 0xff00);
	dp_set_bits(DP52_AUX_ADCTRL, auxbgp<<8);

	// reset pwm0
	dp52_direct_write(DP52_PWM0_CFG2, 0x0000);
	dp52_direct_write(DP52_PWM0_CFG1, 0xaa00);

	// init timer to 4 hours (4*60*60 / ~64 = 0xe1)
	dp_clr_bits(DP52_PWM0_CRGTMR_CFG, 0x80);
	dp52_direct_write(DP52_PWM0_CRG_TIMEOUT, 0xe1);
	// count every ~64 seconds (2^(10+11) / 32576)
	dp52_direct_write(DP52_PWM0_CRGTMR_CFG, 0x80b);

	return 0;
}

int dp_charger_set()
{
	u16 val;

	dp52_direct_write(DP52_PWM0_CFG1, 0xaa1b);

	// check chip id (ECN #1078) and confiure CFG2 according to ECN #1068
	val = dp52_direct_read(DP52_CMU_CSTSR);
	if (((val & 0x01e0) >> 5) > 0)		// DP-TO2
	{
		dp52_direct_write(DP52_PWM0_CFG2,0x2800);	// enable pwm0 with modulo 32
	}
	else dp52_direct_write(DP52_PWM0_CFG2, 0x2000); // original TO, enable pwm0 with modulo 32

	// Enable timer
	dp_set_bits(DP52_PWM0_CRGTMR_CFG, 0x80);

	return 0;
}

void dp_charger_reset()
{
	dp_set_bits(DP52_AUX_EN, 0x0);
	dp_clr_bits(DP52_PWM0_CFG2, 0x2000);
	dp_clr_bits(DP52_PWM0_CRGTMR_CFG, 0x80);
}

void dp_aux_enable(u16 bandgap)
{
	dp52_direct_write(DP52_PMU_EN_SW, 0x7ac);
	dp52_direct_write(DP52_CMU_CCR2, 0x2);
	dp52_direct_write(DP52_CMU_CCR0, 0x800);
	dp_set_bits(DP52_AUX_EN, 0x3);
	dp52_direct_write(DP52_AUX_ADCTRL, (bandgap<<8) | (DP_DEFAULT_SAMPLERATE << 1));
//	dp_aux_mux(DP52_AUX_MUX_DCIN1_16V);
}

void dp_aux_mux(unsigned short mux)
{
	dp_clr_bits(DP52_AUX_ADCFG, 0x1f);
	dp_set_bits(DP52_AUX_ADCFG, mux);
}

/*****************************************************************************/

u16 dp_read_aux(void)
{
	dp_set_bits(DP52_AUX_ADCTRL, 0x1);
	while (dp52_direct_read(DP52_AUX_ADCTRL) & 0x1);

	return dp52_direct_read(DP52_AUX_ADDATA) & 0x7fff;
}

u16 getADCMeaurement (u16 auxcfg)
{
	int valueADC = 0;
	u16 i;

	dp_aux_mux(auxcfg);

	for ( i=0; i < 4; i++)
	{
		valueADC += dp_read_aux();
	}
	valueADC = valueADC >> 2;	// divide by 4

	return valueADC;
}

/* get raw NTC reading */
int get_batt_ntc(u16 auxbgp)
{
	u16 val;

	// read NTC
	dp_aux_enable(auxbgp);

	// read DCIN0
	val = getADCMeaurement(DP52_AUX_MUX_DCIN0_16V);

	return val;
}

/* get battery temperature, 3 digit. return temp*10 */
signed int get_temperature(u16 auxbgp, u16 tempref)
{
	u32	       val;
	u32 	   voltage;
	u32		   cal_ratio;
	signed int temp;

/* If no temperature support - report 25C */
#ifndef CONFIG_BATTERY_TEMP_SUPPORT
	return 250;
#endif

	/* get raw sample from DCIN0 */
	val = get_batt_ntc(auxbgp);

	/* calculate calibration ratio */
	cal_ratio = ((tempref * 10000) >> 14);

	/* calculate voltage */
	voltage = (((val * cal_ratio) >> 15) * 2);

	/* resolve into voltage */
	temp = charger_v_to_c(voltage);

	DEBUG("DCIN0: %ld voltage: %ld temp: %d\n", val, voltage, temp);

	/* return 3 digit integer */
	return temp;
}

int get_mVoltage(u16 auxbgp, u16 atten)
{
	u16 uValueADC;
	int mVoltage;
	u16 dc3gain_control_reg;
	u16 old_dc3gain;
	u16 old_dc3atten;

	int charger_is_on = dp52_direct_read(DP52_PWM0_CFG2) & 0x2000;
	if (charger_is_on) {
		DEBUG("Charger is on\n");

		// save DC3 comperator gain and attenuation for the charger init later
		dc3gain_control_reg = dp52_direct_read(DP52_AUX_DC3GAINCNT1);
		old_dc3gain = (dc3gain_control_reg) & 0x1f;
		old_dc3atten = (dc3gain_control_reg >> 5) & 0x3f;
		DEBUG("DP52_AUX_DC3GAINCNT1 = 0x%x\n", dc3gain_control_reg);
		DEBUG("old_dc3atten = 0x%x\n", old_dc3atten);
		DEBUG("old_dc3gain = 0x%x\n", old_dc3gain);

		dp_charger_reset();
		// let the voltage stabilize
		private_mdelay(1000);
	}

	dp_aux_enable(auxbgp);

	/* disable touchpad; it might interfere with sampling (auto-mode) */
	dp52_direct_write(DP52_TP_CTL1, 0);
	dp52_direct_write(DP52_TP_CTL2, 0);

	uValueADC = getADCMeaurement(DP52_AUX_MUX_DCIN3_16V);
	DEBUG("uValueADC = %d\n", uValueADC);

	mVoltage = ((int)uValueADC * 20 * 403) >> 15; // shift right 15: divide by 0x8000 - max-read (15 bit)

	DEBUG("mVoltage = %d\n", mVoltage);

	if (charger_is_on) {
		dp_charger_init(auxbgp, old_dc3atten, old_dc3gain);
		dp_charger_set();
	}

	return mVoltage;
}

void power_off()
{
	// Power off
	while(1)
	{
		dp52_direct_write( DP52_PMU_CTRL, 0x1);
		dp52_direct_write( DP52_PMU_EN_SW, 0x7a8);
		dp52_direct_write( DP52_PMU_OFF, 0xaaaa);
		dp52_direct_write( DP52_PMU_OFF, 0x5555);
		dp52_direct_write( DP52_PMU_OFF, 0xaaaa);
	}
}


/***************************************************************************************************/
/* dp52 read/write code (this code is copied into the SRAM, should be inside the same object file) */
/***************************************************************************************************/
#ifdef RUN_CHARGER_FROM_SRAM

/********************* taken as it is from gpio.c ****************/
#include "gpio.h"

#define AGPDATA		0x0

static int charger_gpio_register_offset(int gpio)
{
	return (DW_GPIO_BASE + ((gpio >> 8) & 0xff));
}

static int charger_gpio_mask(int gpio)
{
	return (1 << (gpio & 0xff));
}

void charger_gpio_set_value(int gpio, int value)
{
	unsigned int tmp;

	tmp = readl(charger_gpio_register_offset(gpio) + AGPDATA);
	if (value)
		tmp |= charger_gpio_mask(gpio);
	else
		tmp &= ~charger_gpio_mask(gpio);
	writel(tmp, charger_gpio_register_offset(gpio) + AGPDATA);
}

/*****************************************************************/


/********************** taken as it is from spi.c ****************/
#include "spi.h"

void charger_spi_read(unsigned char *buf, int len)
{
	int i;

	writel(0x03, DW_SPI_INT_CLR);
	writel(0x01, DW_SPI_INT_EN);

	for (i = 0; i < len; i++) {
		writel(0x00, DW_SPI_TXBUF);
		while (!(readl(DW_SPI_INTSTAT) & 0x1))
			;
		buf[i] = readl(DW_SPI_RXBUF);
		writel(0x03, DW_SPI_INT_CLR);
	}
}

void charger_spi_write(unsigned char *buf, int len)
{
	int i;

	writel(0x03, DW_SPI_INT_CLR);
	writel(0x01, DW_SPI_INT_EN);

	for (i = 0; i < len; i++) {
		writel(buf[i], DW_SPI_TXBUF);
		while (!(readl(DW_SPI_INTSTAT) & 0x1))
			;
		writel(0x3, DW_SPI_INT_CLR);
	}
}
/*****************************************************************/


/******************* taken as it is from dp52.c ******************/

void charger_dp52_select(void)
{
	charger_gpio_set_value(AGPIO(26), 0);
}

void charger_dp52_deselect(void)
{
	charger_gpio_set_value(AGPIO(26), 1);
}

int charger_dp52_direct_write(unsigned int reg, unsigned int value)
{
	unsigned char tmp[3];

	charger_dp52_select();

	tmp[0] = reg;
	tmp[1] = (value >> 8) & 0xff;
	tmp[2] = value & 0xff;
	charger_spi_write(tmp, 3);

	charger_dp52_deselect();

	return 0;
}

int charger_dp52_direct_read(unsigned int reg)
{
	unsigned char tmp[2];

	charger_dp52_select();

	tmp[0] = (reg & 0x7f) | 0x80;
	charger_spi_write(tmp, 1);

	charger_spi_read(tmp, 2);

	charger_dp52_deselect();

	return (tmp[0] << 8) | tmp[1];
}

int charger_dp52_indirect_read(unsigned int reg)
{
	unsigned char tmp[2];

	charger_dp52_select();

	tmp[0] = DP52_SPI_INDADD;
	tmp[1] = reg;
	charger_spi_write(tmp, 2);

	tmp[0] = DP52_SPI_INDDAT | 0x80;
	charger_spi_write(tmp, 1);

	charger_spi_read(tmp, 2);

	charger_dp52_deselect();

	return (tmp[0] << 8) | tmp[1];
}

int charger_dp52_indirect_write(unsigned int reg, unsigned int value)
{
	unsigned char tmp[3];

	charger_dp52_select();

	tmp[0] = DP52_SPI_INDADD;
	tmp[1] = reg;
	charger_spi_write(tmp, 2);

	tmp[0] = DP52_SPI_INDDAT;
	tmp[1] = (value >> 8) & 0xff;
	tmp[2] = value & 0xff;
	charger_spi_write(tmp, 3);

	charger_dp52_deselect();

	return 0;
}
/*****************************************************************/

#endif

