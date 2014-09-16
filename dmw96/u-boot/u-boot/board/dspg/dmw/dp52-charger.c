/*
 * (C) Copyright 2011 DSP Group
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

#include <asm/arch/gpio.h>
#include <asm/byteorder.h>
#include <asm/io.h>
#include <common.h>
#include <linux/ctype.h>
#include <spi.h>
#include <asm/arch/gpio.h>
#include <stdio_dev.h>

#include "dmw.h"
#include "dp52.h"
#include "dmw_timer.h"
#include "irq.h"
#include "lcd.h"
#include "pm.h"
#include "clock.h"
#include "battery_gpx.h"

#define INT_MAX	((int)(~0U>>1))

static struct stdio_dev *ch_con;

#if 0
static void init_ch_console(void)
{
	ch_con = stdio_get_by_name("vga");
	if (ch_con && ch_con->start && ch_con->start() < 0)
		ch_con = NULL;
	if (!ch_con) 
		ch_con = stdio_devices[stdout];
	if (!ch_con)
		return;
}

static void charger_printf(const char *fmt, ...)
{
	va_list args;
	char printbuffer[CFG_PBSIZE];

	va_start(args, fmt);
	vsprintf(printbuffer, fmt, args);
	va_end(args);

	printf(printbuffer);
	if (!ch_con)
		init_ch_console();

	if (ch_con) 
		ch_con->puts(printbuffer);
}
#else
#define charger_printf printf
static void init_ch_console(void) {}
#endif

#if 0
#define dbg_printf(format, arg...) \
	printf(format , ## arg)
#else
#define dbg_printf(format, arg...) \
	({ if (0) printf(format, ##arg); 0; })
#endif

/*
 * Charging does not start if the battery capacity is equal or above the
 * following percentage.
 */
#define MIN_CHRG_CAPACITY 96

#define POLL_INTERVAL 3000

struct battery {
	char *name;

	int vmin;
	int vmax;

	int temp_min_charge;
	int temp_max_charge;
	int temp_min_discharge;
	int temp_max_discharge;

	int chrg_voltage;
	int chrg_current;
	int chrg_cutoff_curr;
	int chrg_cutoff_time;

	int ntc_r;
	int ntc_b;
	int ntc_t;

	int v1;
	int c1;
	int v2;
	int c2;
};

enum state {
	CHRG_STATE_CHARGE,
	CHRG_STATE_MONITOR,
	CHRG_STATE_THERMAL
};

struct chrg_state {
	enum state state;
	struct battery bat;
	int vcca;
};

#define Q_SHIFT 12
#define Q_HALF (1 << (Q_SHIFT-1))
#define Q(x) ((x) << Q_SHIFT)

static int q_mul(int a, int b)
{
	return (a * b + Q_HALF) >> Q_SHIFT;
}

static int q_div(int a, int b)
{
	int x = (a << Q_SHIFT) + b / 2;
	return x / b;
}

/*
 * Returns the natural logarithm of a fixed point number.
 *
 * The function approximates the logarithm with the Taylor series which
 * converges for 0 < x < 2. Outside of this range we're using the fact that:
 *
 *   ln(1/x) = -ln(x)
 */
static int ln(int x)
{
	int k, l, xx;

	if (x >= Q(2)) {
		return -ln(q_div(Q(1), x));
	} else {
		x = x - Q(1);
	}


	l = x;
	xx = x;
	for (k=2; k<8; k+=2) {
		xx = q_mul(xx, x);
		l -= q_div(xx, Q(k));
		xx = q_mul(xx, x);
		l += q_div(xx, Q(k + 1));
	}

	return l;
}

static int get_value(char *params, char *tag, int *val)
{
	char *pos = strstr(params, tag);
	int len = strlen(tag);

	if (!pos) {
		charger_printf("Missing '%s' battery parameter!\n", tag);
		return 1;
	}

	if (pos[len] != ':' || (!isdigit(pos[len+1]) && pos[len+1] != '-')) {
		charger_printf("Invalid battery parameter '%s'\n", tag);
		return 1;
	}

	*val = simple_strtol(pos + len + 1, NULL, 10);
	return 0;
}

static int bat_loads(struct battery *bat)
{
	char *settings = getenv("dp52bat");
	int val;

	/* parse calibration data */
	if (!settings)
		goto invalid;

	if (get_value(settings, "vmin", &val))
		goto invalid;
	bat->vmin = val;

	if (get_value(settings, "vmax", &val))
		goto invalid;
	bat->vmax = val;

	if (get_value(settings, "tminc", &val))
		goto invalid;
	bat->temp_min_charge = val;

	if (get_value(settings, "tmind", &val))
		goto invalid;
	bat->temp_min_discharge = val;

	if (get_value(settings, "tmaxc", &val))
		goto invalid;
	bat->temp_max_charge = val;

	if (get_value(settings, "tmaxd", &val))
		goto invalid;
	bat->temp_max_discharge = val;

	if (get_value(settings, "cv", &val))
		goto invalid;
	bat->chrg_voltage = val;

	if (get_value(settings, "ci", &val))
		goto invalid;
	bat->chrg_current = val;

	if (get_value(settings, "cci", &val))
		goto invalid;
	bat->chrg_cutoff_curr = val;

	if (get_value(settings, "cct", &val))
		goto invalid;
	bat->chrg_cutoff_time = val;

	if (get_value(settings, "ntcr", &val))
		goto invalid;
	bat->ntc_r = val;

	if (get_value(settings, "ntcb", &val))
		goto invalid;
	bat->ntc_b = val;

	if (get_value(settings, "ntct", &val))
		goto invalid;
	bat->ntc_t = val;

	if (get_value(settings, "v1", &val))
		goto invalid;
	bat->v1 = val;

	if (get_value(settings, "c1", &val))
		goto invalid;
	bat->c1 = val;

	if (get_value(settings, "v2", &val))
		goto invalid;
	bat->v2 = val;

	if (get_value(settings, "c2", &val))
		goto invalid;
	bat->c2 = val;

	return 0;

invalid:
	charger_printf("Invalid battery data (\"dp52bat\"). Charging disabled!\n");
	return 1;
}

static void bat_puts(struct battery *bat)
{
	char buf[256];

	sprintf(buf, "vmin:%d,vmax:%d,tminc:%d,tmind:%d,tmaxc:%d,tmaxd:%d,"
		"cv:%d,ci:%d,cci:%d,cct:%d,ntcr:%d,ntcb:%d,ntct:%d,"
		"v1:%d,c1:%d,v2:%d,c2:%d",
		bat->vmin, bat->vmax, bat->temp_min_charge, bat->temp_min_discharge,
		bat->temp_max_charge, bat->temp_max_discharge, bat->chrg_voltage,
		bat->chrg_current, bat->chrg_cutoff_curr, bat->chrg_cutoff_time,
		bat->ntc_r, bat->ntc_b, bat->ntc_t,
		bat->v1, bat->c1, bat->v2, bat->c2);

	setenv("dp52bat", buf);
}

static int get_batt_temp(struct chrg_state *state)
{
#ifdef CONFIG_DP52_TEMP_DCIN
	int vtemp, rtemp, temp, off;

	vtemp = dp52_auxadc_measure(CONFIG_DP52_TEMP_DCIN);
	if (vtemp < 50 || vtemp > 1950)
		return INT_MAX; /* invalid temp -> battery not present */

	/* subtract offset of shunt resistor */
	off = dp52_auxadc_measure(CONFIG_DP52_ICHG_DCIN);

	rtemp = (vtemp - off) * CONFIG_DP52_TEMP_R / (state->vcca - off - vtemp);
	temp = state->bat.ntc_b * (state->bat.ntc_t+273) /
		( state->bat.ntc_b +
		  ((ln(Q(rtemp) / state->bat.ntc_r) * (state->bat.ntc_t+273)) >> Q_SHIFT)
		) - 273;

	dbg_printf("temp: %dC (%dmV)\n", temp, vtemp);

	return temp;
#else
	/* No support for temerature readings... */
	return 25;
#endif
}

static int calculate_bat_capacity(struct battery *bat, int vbat)
{
	int capacity;

	/* the voltage<->capacity graph has 3 linear parts:
	 * 1) (bat->vmin,0)		- (bat->v1,bat->c1)
	 * 2) (bat->v1,bat->c1)	- (bat->v2,bat->c2)
	 * 3) (bat->v2,bat->c2)	- (bat->vmax, 100)
	 *
	 * Note: if vbat < bat->vmin, the function will return negative result!
	 *
	 */
	int v[2];
	int c[2];

	if (vbat < bat->v1) {
		/* 1 */
		capacity = 0;
		v[0] = bat->vmin;
		c[0] = 0;
		v[1] = bat->v1;
		c[1] = bat->c1;
	}
	else if (vbat < bat->v2) {
		/* 2 */
		capacity = bat->c1;
		v[0] = bat->v1;
		c[0] = bat->c1;
		v[1] = bat->v2;
		c[1] = bat->c2;
	}
	else {		/* bat->vbat >= bat->v2 */
		/* 3 */
		capacity = bat->c2;
		v[0] = bat->v2;
		c[0] = bat->c2;
		v[1] = bat->vmax;
		c[1] = 100;
	}
	capacity += (vbat - v[0]) * (c[1] - c[0]) / (v[1] - v[0]);

	/* in case the voltage is above bat->vmax, we might get more than 100%... */
	if (capacity > 100)
		capacity = 100;

	return capacity;
}

static int get_batt_capacity(struct chrg_state *state)
{
	int vbat;

	if (state->state != CHRG_STATE_MONITOR) {
		/* disable charging to let voltage stabalize */
		dp52_direct_write_field(DP52_PWM0_CFG2, 13, 1, 0);
		udelay(100000);
	}

	vbat = dp52_auxadc_measure(CONFIG_DP52_VBAT_DCIN) *
		(CONFIG_DP52_VBAT_R1+CONFIG_DP52_VBAT_R2) /
		CONFIG_DP52_VBAT_R2;
	dbg_printf("vbat: %dmV\n", vbat);

	/* resume charging if stopped previously */
	if (state->state != CHRG_STATE_MONITOR)
		dp52_direct_write_field(DP52_PWM0_CFG2, 13, 1, 1);

	return calculate_bat_capacity(&state->bat, vbat);
}

static int get_ichrg(struct chrg_state *state)
{
	int i,v;

	v = dp52_auxadc_measure(CONFIG_DP52_ICHG_DCIN);
	i = v * 1000 / CONFIG_DP52_ICHG_R;

	dbg_printf("ichrg: %d (%dmV)\n", i, v);

	return i;
}

static int charge_timer_expired(struct chrg_state *state)
{
	return !!(dp52_direct_read(DP52_PWM0_CRGTMR_CFG) & 0x10);
}

static int state_init(struct chrg_state *state)
{
	int val;
	int gain;
	int attn;
	
	state->state = CHRG_STATE_MONITOR;
	if (bat_loads(&state->bat)) {
		charger_printf("error: can't load battery parameters\n");
		return 1;
	}

	if (dp52_get_calibrated_parameters(&gain, &attn)) {
		charger_printf("error: can't get calibration parameters\n");
		return 1;
	}

	/*
	 * Get Vcca level
	 */
	val = dp52_direct_read_field(DP52_PMU_LVL3_SW, 0, 4);
	state->vcca = 2400 + 100*val;

	return 0;
}

static int charge_init(struct chrg_state *state, unsigned int is_fast_charge)
{
	int ret;

	if ((ret = state_init(state)))
		return ret;

	/* 
	  TODO add as battery parameter duty cycle for slow charge case 
	  Make sure that the duty cycle / modulo is smaller than a given maximum save value (that given max value will have to be defined) */

	/*
	 * Setup PWM0:
	 * modulo = 256 (counting 0..255)
	 * duty_cycle = 0xfe = 254 (almost maximum DC)
	 *   - cut off if above configured Vbat and Ichg
	 */
	dp52_direct_write(DP52_PWM0_CFG2, 7 << 9);	/* modulo = 256 */

	/* TODO change the code back to following when u-boot support low power mode (Wait For Interrupt, close LCDC ... etc) */
	dp52_direct_write(DP52_PWM0_CFG1, (is_fast_charge ? 0x00fe : 0x010) |
		(is_fast_charge ? (0x1100 << CONFIG_DP52_ICHG_DCIN) : 0) |
		(0x1100 << CONFIG_DP52_VBAT_DCIN));
	
	/*
	 * Init timer, counting every 64 seconds. Counting
	 * starts as soon Vbat rises above cutoff threshold.
	 */
	dp52_direct_write(DP52_PWM0_CRGTMR_CFG, 0x0000);
	dp52_direct_write(DP52_PWM0_CRG_TIMEOUT,
	(is_fast_charge ? 1 : 2) * state->bat.chrg_cutoff_time * 60 / 64);
	dp52_direct_write(DP52_PWM0_CRGTMR_CFG, 0x000b |
		(0x0100 << CONFIG_DP52_VBAT_DCIN));

	return 0;
}

static enum state charge_state(struct chrg_state *state)
{
	return state->state;
}

static int dp52_enable_current_limit(struct chrg_state *state, int capacity, int thermal)
{
	int gain;
	int attn;
	int vcurrent;

	if (thermal) {
		vcurrent = 10; /* that's as small as it can get... */
	}
	else {
		vcurrent = CONFIG_DP52_ICHG_R * state->bat.chrg_current / 1000;
		/* if capacity is very low, we should charge with low current */
		if (capacity < 0) {
			vcurrent /= 4;
		}
	}

	if (dp52_compute_initial_gain_attn(vcurrent, &gain, &attn, 0))
		return 1;

	dp52_auxcmp_enable(CONFIG_DP52_ICHG_DCIN, gain, attn);
	return 0;
}

static int dp52_enable_voltage_limit(struct chrg_state *state)
{
	int gain;
	int attn;
#if 0
	int vmax = state->bat.chrg_voltage * CONFIG_DP52_VBAT_R2 /
		(CONFIG_DP52_VBAT_R1+CONFIG_DP52_VBAT_R2);
#endif

	/* get calibrated parameters which should give EXACT vmax voltage */
	if (dp52_get_calibrated_parameters(&gain, &attn))
		return 1;

	dp52_auxcmp_enable(CONFIG_DP52_VBAT_DCIN, gain, attn);
	return 0;
}

static int charge_start(struct chrg_state *state, int thermal)
{
	int capacity;

	dbg_printf("Start charging\n");

	capacity = get_batt_capacity(state);

	/* limit the charging current */
	if (dp52_enable_current_limit(state, capacity, thermal)) {
		charger_printf("Cannot enable current limiter!\n");
		return 1;
	}

	/* make sure charger will start timer after battery reaches maximum volage */
	if (dp52_enable_voltage_limit(state)) {
		charger_printf("Cannot enable voltage limiter!\n");
		return 1;
	}

	dp52_direct_write_field(DP52_PWM0_CFG2, 13, 1, 1); /* enable PWM0 */
	dp52_direct_write_field(DP52_PWM0_CRGTMR_CFG, 4, 1, 0); /* clear timer done indication */
	dp52_direct_write_field(DP52_PWM0_CRGTMR_CFG, 7, 1, 1); /* enable timer */

#if 0
	/* dump all dp registers */
	{
		int i;

		for (i=0; i<0x80; i++) {
			if (i%4 == 0) {
				printf("\n%3x:", i);
			}
			printf("%5x", dp52_direct_read(i));
		}
		printf("\n");
	}
#endif

	if (thermal)
		state->state = CHRG_STATE_THERMAL;
	else
		state->state = CHRG_STATE_CHARGE;

	return 0;
}

static void charge_stop(struct chrg_state *state)
{
	if (state->state == CHRG_STATE_MONITOR)
		return;

	dbg_printf("Stop charging\n");
	dp52_direct_write_field(DP52_PWM0_CFG2, 13, 1, 0); /* disable PWM0 */
	dp52_direct_write_field(DP52_PWM0_CRGTMR_CFG, 7, 1, 0); /* disable timer */
	dp52_auxcmp_disable(CONFIG_DP52_VBAT_DCIN);
	dp52_auxcmp_disable(CONFIG_DP52_ICHG_DCIN);

	state->state = CHRG_STATE_MONITOR;
}

#define DP52_EXTINT_NUM		2

static void on_off_int(unsigned int enb)
{
	if (enb) {
		/*Set EXTINT2 at input FGPIO18 */
		gpio_direction_input(extint_to_gpio(DP52_EXTINT_NUM));
		gpio_pull_ctrl(extint_to_gpio(DP52_EXTINT_NUM), 0, 1);
		gpio_enable(extint_to_gpio(DP52_EXTINT_NUM));
		/*Enable EXTINT2_MOD as Level and EXTINT2_POL as active high*/
		writel( 0x1 << DP52_EXTINT_NUM * 2 , DMW_GPIO_BASE + DMW_EXTINT_CFG);
		/*Enable PMU interrupt*/
		dp52_direct_write(DP52_ICU_MASK,0x1); 
		/*Enable on/off interrupt button*/
		dp52_direct_write(DP52_PMINTMSK,0x1); 
		/* Unmask  EXTINT2 on PLICU*/
		dmw_irq_unmask(extint_to_idd(DP52_EXTINT_NUM));
	}else {
		dp52_direct_write(DP52_PMUSTAT, 0x0000);
		dmw_irq_mask(extint_to_idd(DP52_EXTINT_NUM));
	}

}
static void enable_timer2_int(unsigned long ms)
{
	dmw_irq_unmask(DMW_IID_TIMER2);

	dmw_timer_alloc(DMW96_TIMERID2 , DMW_TMR_CTRL_DIV16 | DMW_TMR_CTRL_32BIT | DMW_TMR_CTRL_IE );
	/*The pclk is 120000000 and the RTC is devided by 16 (delivering DMW_TMR_CTRL_DIV16 to dmw_timer_alloc)*/
	dmw_timer_start(DMW96_TIMERID2, ms*(dmw_get_pclk()/16000) );

}

static void  disable_timer2_int(void)
{
	/*Clear interrupt on devices*/
	dmw_timer_stop(DMW96_TIMERID2);
	dmw_irq_mask(DMW_IID_TIMER2);
}


void static keypad_int(unsigned int enb)
{
	if (enb) {
		dmw_irq_unmask(DMW_IID_KEYPAD);
	} else {
		dmw_irq_mask(DMW_IID_KEYPAD);
	}
}

static unsigned int was_keypad_int(void){
	return (dmw_is_irq_pending(DMW_IID_KEYPAD));
}

static  unsigned int was_on_off_int(void) {

	return (readl(DMW_GPIO_BASE + DMW_EXTINT_STAT) & (0x1 << 2));
}

#define RTC_TIME 0x14
#define TURN_OFF_LCD_TIMEOUT 10
#define DMW_RTC_BASE 0x05600000
static unsigned int time_press;

static void charge_sleep(unsigned long ms)
{
	
	unsigned int time_now;
	unsigned long flags;

	local_irq_save(flags);
	keypad_int(1);
	enable_timer2_int(ms);
	on_off_int(1);
	pm_clk_devs(0);

	go_to_self_refresh(SLEEP_DDR_MODE_3  | SLEEP_CPU_SLOW);

	/*
	if (was_on_off_int())
		printf("On/Off was pressed\n");

	if (was_keypad_int())
		printf("Keypad was pressed\n");
	*/

	if (was_on_off_int() || was_keypad_int())
	{
		time_press = readl(DMW_RTC_BASE + RTC_TIME);
		lcdc_suspend_mode(0);
	}
	
	time_now = readl(DMW_RTC_BASE + RTC_TIME);

	if (time_now - time_press > (dmw_get_slowclkfreq() * TURN_OFF_LCD_TIMEOUT) ) {
	
		lcdc_suspend_mode(1);
	}

	pm_clk_devs(1);
	keypad_int(0);
	disable_timer2_int();
	on_off_int(0);
	local_irq_restore(flags);
}

void dp52_charge(int turn_off_charger_out, unsigned int is_fast_charge)
{
	struct chrg_state state;
	int temp;
	int capacity;
	int last_capacity = 999;	/* not valid - will never return from get_bat_capacity */

	charger_printf("Entering charging mode...\n");
	
	if (charge_init(&state , is_fast_charge))
		return;

	temp = get_batt_temp(&state);
	if (temp == INT_MAX) {
		charger_printf("Cannot charge. Battery not present!\n");
		dmw_bat_paint_nobat();
		return;
	}

	if (temp < state.bat.temp_min_discharge || temp > state.bat.temp_max_discharge) {
		charger_printf("Hard thermal battery limit. Shutting down...\n");
		dp52_power_off();
	}

	if (temp < state.bat.temp_min_charge || temp > state.bat.temp_max_charge) {
		charger_printf("Thermal problem! Start charging\n");
		if (charge_start(&state, 1)) {
			charger_printf("Error starting charging\n");
			dmw_bat_paint_thermal(-1);
			return;
		}
	} else if (get_batt_capacity(&state) < MIN_CHRG_CAPACITY) {
		charger_printf("Start charging\n");
		if (charge_start(&state, 0)) {
			charger_printf("Error starting charging\n");
			dmw_bat_paint_error();
			return;
		}
	} else
		dbg_printf("Battery ok. Monitoring...");

	time_press = (dmw_get_slowclkfreq() * TURN_OFF_LCD_TIMEOUT) ;
	
	for (;;) {
		int backoff = 0;
		int thermal;

		/* returns immediately if button pressed */
		charge_sleep(POLL_INTERVAL);

		temp = get_batt_temp(&state);
		if (temp == INT_MAX) {
			charger_printf("Battery removed!\n");
			charge_stop(&state);
			dmw_bat_paint_nobat();
			return;
		}

		thermal = temp < state.bat.temp_min_charge || temp > state.bat.temp_max_charge;

		/* Power button pressed? Only check if battery is exptected to be ok. */
		if (!backoff) {
			int i = 0;
			while (dp52_direct_read(DP52_PMURAWSTAT) & 0x01) {
				if (++i > 100) {
					if (get_batt_capacity(&state) <= 0)
						backoff = 100;
					else if (thermal)
						backoff = 50;
					else {
						/* Continue booting... */
						charger_printf("Booting device\n");
						dmw_bat_paint(get_batt_capacity(&state), 0);		
						charge_stop(&state);
						return;
					}
				}
				mdelay(10);
			}
		} else
			backoff--;

		/* Charger removed? */
		if (!(dp52_direct_read(DP52_PMURAWSTAT) & 0x02)) {
			charge_stop(&state);
			if (turn_off_charger_out) {
				charger_printf("Charger removed. Shutting down...\n");
				dp52_power_off();
				/* never gets here... */
			}
			dmw_bat_paint(-1, 0);
			charger_printf("Charger removed\n");
			return;
		}

		/* get capacity, and display if changed */
		capacity = get_batt_capacity(&state);
		if (capacity != last_capacity) {
			dmw_bat_paint(capacity, 1);
			if (capacity > 0) 
				charger_printf("Battery capacity: %d%%\n", capacity);
			else
				charger_printf("Battery capacity: VERY LOW!\n");
			last_capacity = capacity;
		}

		/* Charging state machine */
		switch (charge_state(&state)) {
			case CHRG_STATE_CHARGE:
				if (thermal) {
					/* thermal limit reached */
					charger_printf("Stopping charger because of thermal limit\n");
					charge_stop(&state);
					if (charge_start(&state, 1)) {
						charger_printf("Error starting charging\n");
						dmw_bat_paint_thermal(capacity);
						return;
					}
				} else if ( ( is_fast_charge ? (get_ichrg(&state) < state.bat.chrg_cutoff_curr) : 0) ||
				           charge_timer_expired(&state)) {
					/* charge cutoff condition reached */
					charger_printf("Stopping charger because of cutoff\n");
					dmw_bat_paint(capacity, 0);
					charge_stop(&state);
				} else {
					/* set again the current limit. no thermal condition (checked above) */
					dp52_enable_current_limit(&state, capacity, 0);
				}
				break;

			case CHRG_STATE_MONITOR:
				if (capacity < MIN_CHRG_CAPACITY) {
					if (charge_start(&state, thermal)) {
						charger_printf("Error starting charging\n");
						dmw_bat_paint_error();
						return;
					}
				}
				break;

			case CHRG_STATE_THERMAL:
				/* Check thermal limit */
				if (!thermal) {
					charger_printf("Stopping charger because of !thermal\n");
					charge_stop(&state);
					if (capacity < MIN_CHRG_CAPACITY) {
						if (charge_start(&state, 0)) {
							charger_printf("Error starting charging\n");
							dmw_bat_paint_error();
							return;
						}
					}
				}
				break;
		}
	}
}

int dp52_check_battery(void)
{
	struct chrg_state state;
	int temp;

	if (state_init(&state))
		return 0;

	temp = get_batt_temp(&state);
	if (temp == INT_MAX)
		return 0; /* battery not present */

	if (temp < state.bat.temp_min_discharge || temp > state.bat.temp_max_discharge)
		return 1;

	if (get_batt_capacity(&state) <= 0)
		return 2;

	return 0;
}


static struct battery default_bat = {
	DP52_BATTERY_DEFAULT
};

static int do_battery(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	if (argc == 1) {
		struct battery bat;
		if (bat_loads(&bat))
			return 1;

		printf("Battery parameters:\n");
		printf("    vmin:               %5d mV\n", bat.vmin);
		printf("    vmax:               %5d mV\n", bat.vmax);
		printf("    temp_min_charge:    %5d C\n", bat.temp_min_charge);
		printf("    temp_max_charge:    %5d C\n", bat.temp_max_charge);
		printf("    temp_min_discharge: %5d C\n", bat.temp_min_discharge);
		printf("    temp_max_discharge: %5d C\n", bat.temp_max_discharge);
		printf("    chrg_voltage:       %5d mV\n", bat.chrg_voltage);
		printf("    chrg_current:       %5d mA\n", bat.chrg_current);
		printf("    chrg_cutoff_curr:   %5d mA\n", bat.chrg_cutoff_curr);
		printf("    chrg_cutoff_time:   %5d min\n", bat.chrg_cutoff_time);
		printf("    ntc_r:              %5d Ohm\n", bat.ntc_r);
		printf("    ntc_b:              %5d\n", bat.ntc_b);
		printf("    ntc_t:              %5d C\n", bat.ntc_t);
		printf("    v1:                 %5d mV\n", bat.v1);
		printf("    c1:                 %5d %%\n", bat.c1);
		printf("    v2:                 %5d mV\n", bat.v2);
		printf("    c2:                 %5d %%\n", bat.c2);
	} else if (argc == 2) {
		if (strcmp(argv[1], "default") == 0) {
			bat_puts(&default_bat);
			printf("Battery parameters reset to defaults (%s).\n"
				"Make sure they match YOUR battery!\n", default_bat.name);
		} else if (strcmp(argv[1], "status") == 0) {
			struct chrg_state state;
			int temp;

			if (state_init(&state))
				return 1;

			temp = get_batt_temp(&state);
			if (temp == INT_MAX) {
				printf("Battery not present!\n");
				return 0;
			}

			printf("Temperature: %dC\n", temp);
			printf("Capacity: %d%%\n", get_batt_capacity(&state));
		} else if (strcmp(argv[1], "charge") == 0)
			dp52_charge(0,1);	/* don't shut down if charger is removed */
		else
			return 1;
	} else if (strcmp(argv[1], "set") == 0) {
		int i;
		struct battery bat;

		if (argc & 1)
			return 1;

		if (bat_loads(&bat))
			return 1;

		for (i=2; i<argc; i+=2) {
			int val = simple_strtol(argv[i+1], NULL, 10);

			if (!strcmp(argv[i], "vmin"))
				bat.vmin = val;
			else if (!strcmp(argv[i], "vmax"))
				bat.vmax = val;
			else if (!strcmp(argv[i], "temp_min_charge"))
				bat.temp_min_charge = val;
			else if (!strcmp(argv[i], "temp_max_charge"))
				bat.temp_max_charge = val;
			else if (!strcmp(argv[i], "temp_min_discharge"))
				bat.temp_min_discharge = val;
			else if (!strcmp(argv[i], "temp_max_discharge"))
				bat.temp_max_discharge = val;
			else if (!strcmp(argv[i], "chrg_voltage"))
				bat.chrg_voltage = val;
			else if (!strcmp(argv[i], "chrg_current"))
				bat.chrg_current = val;
			else if (!strcmp(argv[i], "chrg_cutoff_curr"))
				bat.chrg_cutoff_curr = val;
			else if (!strcmp(argv[i], "chrg_cutoff_time"))
				bat.chrg_cutoff_time = val;
			else if (!strcmp(argv[i], "ntc_r"))
				bat.ntc_r = val;
			else if (!strcmp(argv[i], "ntc_b"))
				bat.ntc_b = val;
			else if (!strcmp(argv[i], "ntc_t"))
				bat.ntc_t = val;
			else if (!strcmp(argv[i], "v1"))
				bat.v1 = val;
			else if (!strcmp(argv[i], "c1"))
				bat.c1 = val;
			else if (!strcmp(argv[i], "v2"))
				bat.v2 = val;
			else if (!strcmp(argv[i], "c2"))
				bat.c2 = val;
			else
				return 1;
		}

		bat_puts(&bat);
	} else
		return 1;

	return 0;
}

U_BOOT_CMD(
	dp_battery, 32, 0, do_battery, "DP52 battery handling",
	"\n"
	"    - show battery parameters\n"
	"dp_battery default\n"
	"    - set battery parameters to defaults\n"
	"dp_battery set <param-list>\n"
	"    - change one or more battery parameters\n"
	"      <param-list> ::= <param>[,<param>,...]\n"
	"      <param> ::= <tag> <value>\n"
	"dp_battery charge\n"
	"    - enter charging mode\n"
	"dp_battery status\n"
	"    - measure voltage and temperature"
);

