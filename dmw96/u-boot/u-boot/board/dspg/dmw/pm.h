#ifndef __MACH_PM_H
#define __MACH_PM_H

#define SLEEP_DDR_MODE_1	0x04
#define SLEEP_DDR_MODE_2	0x03
#define SLEEP_DDR_MODE_3	0x02
#define SLEEP_DDR_MODE_4	0x01
#define SLEEP_DDR_MODE(_mode)	((_mode) & 0x0f)

#define SLEEP_CHECK_BM		0x0100	/* Don't enter if a bus master is active */
#define SLEEP_DDR_PLL_OFF	0x0200	/* Switch off PLL3 (implies manual entry) */
#define SLEEP_CPU_SLOW		0x0400	/* Run CPU from 12MHz */

#define SLEEP_QUIRK_DIS_INP	0x10000	/* Disable input buffer in self refresh */

#define DMW_MPMC_BASE	0x00100000

void go_to_self_refresh(int req_mode);
unsigned int pm_clk_devs(unsigned int enb);

#endif


