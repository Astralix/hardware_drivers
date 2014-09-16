#include <common.h>
#include <command.h>
#include <environment.h>
#include <asm/byteorder.h>
#include <asm/io.h>

#include "dmw.h"
#include "clock.h"
#include "dmw_timer.h"

#define TMR_LOAD	0x00
#define TMR_VALUE	0x04
#define TMR_CONTROL	0x08
DECLARE_GLOBAL_DATA_PTR;

/*
 * We're using the gd pointer to store the timer state because the timer is
 * initialized before relocation and all global variables are re-initialized by
 * the relocation. The following gd variables are used by the timer:
 *
 * timer_rate_hz: timer rate in hz
 * lastinc:       last timer counter value (note: DMW timers are decrementing!)
 * tbu:           ticks per jiffie unit
 * tbl:           last jiffie
 */

void reset_timer_masked(void)
{
	unsigned long int val;
	if (dmw_timer_get_value(DMW96_TIMERID1, &val))
		return;
	gd->lastinc = val; /* capture current decrementer value time */
	gd->tbl = 0;              /* start "advancing" last time stamp from 0 */
}

void reset_timer(void)
{
	reset_timer_masked();
}

unsigned long get_timer_masked(void)
{
	unsigned long elapsed, now;
	unsigned long int val;
	if (dmw_timer_get_value(DMW96_TIMERID1, &val))
		return -1;

	now = val;
	elapsed = (gd->lastinc - now) / gd->tbu;

	gd->tbl += elapsed;
	gd->lastinc -= elapsed * gd->tbu;

	return gd->tbl;
}

unsigned long get_timer(unsigned long base)
{
	return get_timer_masked() - base;
}

/* delay x useconds AND preserve advance timestamp value */
void __udelay(unsigned long usec)
{
	/* Fine grained or coarse/long delay? */
	if (usec >= 10000) {
		/* do it with CONFIG_SYS_HZ resolution */
		unsigned long tmo, endtime;
		signed long diff;

		tmo = usec / 1000;	/* start to normalize for usec to ticks per sec */
		tmo *= CONFIG_SYS_HZ;	/* find number of "ticks" to wait to achieve target */
		tmo /= 1000;		/* finish normalize. */

		endtime = get_timer_masked() + tmo;
		do {
			unsigned long now = get_timer_masked();
			diff = endtime - now;
		} while (diff >= 0);
	} else {
		/* do it with hardware resolution */
		long tmo = usec * (gd->timer_rate_hz / 1000) / 1000;
		unsigned long now, last;
		unsigned long int val;
		if (dmw_timer_get_value(DMW96_TIMERID1, &val)){
			return;
		}
		last = val;
		while (tmo > 0) {
			if (dmw_timer_get_value(DMW96_TIMERID1, &val))
				return;
			now = val;
			tmo -= last - now;
			last = now;
		}
	}
}

unsigned long long get_ticks(void)
{
	return get_timer(0);
}

ulong get_tbclk(void)
{
	return CONFIG_SYS_HZ;
}

int timer_init(void)
{	
	dmw_timer_init();

	dmw_timer_alloc(DMW96_TIMERID1 , DMW_TMR_CTRL_DIV16 | DMW_TMR_CTRL_32BIT );
	dmw_timer_start(DMW96_TIMERID1,0xffffffff);

	gd->timer_rate_hz = dmw_get_pclk() / 16;
	gd->tbu = gd->timer_rate_hz / CONFIG_SYS_HZ;

	/* init the timestamp and lastdec value */
	reset_timer_masked();

	return 0;
}

void mdelay(unsigned long msec)
{
	int i;

	for (i = 0; i < msec; i++)
		udelay(1000);
}

int do_dmw_test_timers(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int count = 10;

	printf("Counting with mdelay: %2d ", count);
	while (count > 0) {
		mdelay(1000);
		printf("\b\b\b%2d ", --count);
	}
	printf("\n");

	count = 10;
	printf("Counting with udelay: %2d ", count);
	while (count > 0) {
		udelay(1000000);
		printf("\b\b\b%2d ", --count);
	}
	printf("\n");

	return 0;
}

U_BOOT_CMD(test_timers, 1, 0, do_dmw_test_timers,
	"run timers test (countdown for 10 seconds)",
	""
);
