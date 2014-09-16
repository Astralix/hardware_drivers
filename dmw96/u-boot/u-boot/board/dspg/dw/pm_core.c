#include "dw.h"

extern char __End_of_idle[], __Start_of_idle[];
extern char charger_start, charger_end;
extern void do_dpm_fastcharge(unsigned int, unsigned int, unsigned int, unsigned int, unsigned int);
#define IDLE_CODE_SIZE (__End_of_idle - __Start_of_idle)
#define SRAM_IDLE_TASK_OFFSET	0x100

typedef void (*ram_idle_task_cb)(unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int);
ram_idle_task_cb* ram_idle_task;

/* The unit of the values below is 32Khz clock cycles */
#define PLL_STABILIZATION_32KHZ_CYCLES	33
#define XTAL_WAKEUP_TIME (197)		/* This is the value we put into XTALOKDURATION. It must be 8 bits value */
#define WAKEUP_OVERHEAD (PLL_STABILIZATION_32KHZ_CYCLES)	/* PLL stabilization plus 0 cycles for other activities */
#define SLEEP_WORTH_PERIOD ((XTAL_WAKEUP_TIME) + (WAKEUP_OVERHEAD) + 1)



#define DPM_FLAG_PLL3_EN	1
#define DPM_FLAG_PLL4_EN	2
#define DPM_FLAG_12_OSC_EN	4
typedef struct _dpm_state
{
	unsigned int csr0;
	unsigned int csr1;
	unsigned int csr2;
	unsigned int flags;
}
dpm_state;

static void dspg_dpm_enter_sequence(dpm_state *state)
{
	/* store scr2 register */
	state->csr0 = *(volatile unsigned int *)(DW_CMU_BASE + 0x04);
	state->csr1 = *(volatile unsigned int *)(DW_CMU_BASE + 0x08);
	state->csr2 = *(volatile unsigned int *)(DW_CMU_BASE + 0x64);
	state->flags = 0;

	/* turn off shared memory and ETM */
	*(volatile unsigned int *)(DW_CMU_BASE + 0x04) &= ~0x0003;

	/* turn off AHB3 and MACA (CSR1) */
	*(volatile unsigned int *)(DW_CMU_BASE + 0x08) &= ~0xa000;

	/* turn off sdmmc, usb2.0, osdm, bmp, etc (CSR2) */
	*(volatile unsigned int *)(DW_CMU_BASE + 0x64) = 1;

	/* set cpu clock to 20 Mhz */
	*(volatile unsigned int *)(DW_CMU_BASE + 0x00) = 0;

	/* turn off 240_EN mode */
	*(volatile unsigned int *)(DW_CMU_BASE + 0x80) &= ~0x1;

	if ( *(volatile unsigned int *)(DW_CMU_BASE + 0x74) & 0x0001 ) {
		/* turn off PLL4 */
		*(volatile unsigned int *)(DW_CMU_BASE + 0x74) &= ~0x0001;
		state->flags |= DPM_FLAG_PLL4_EN;
	}

	if ( *(volatile unsigned int *)(DW_CMU_BASE + 0x6c) & 0x0001 ) {
		/* turn off PLL3 */
		*(volatile unsigned int *)(DW_CMU_BASE + 0x6c) &= ~0x0001;
		state->flags |= DPM_FLAG_PLL3_EN;
	}
#if 0 
	if ( *(volatile unsigned int *)(DW_CMU_BASE + 0x60) & (1<<7) ) {
		/* turn off 12Mhz oscillator */
		*(volatile unsigned int *)(DW_CMU_BASE + 0x60) = 0x9400 | (*(volatile unsigned int *)(DW_CMU_BASE + 0x60) & 0x60);
		state->flags |= DPM_FLAG_12_OSC_EN;
	}
#endif	
//	*(volatile unsigned int *)(DW_CMU_BASE + 0x0c) = XTAL_WAKEUP_TIME;
	
	/* Enable low power mode */
	*(volatile unsigned int *)(DW_CMU_BASE + DW_CMU_LPCR) |= 1 << 4;

	/* turn off shared memory power */
//	*(volatile unsigned int *)(DW_SYSCFG_BASE + 0x40) = 0xe;
//	*(volatile unsigned int *)(DW_SYSCFG_BASE + 0x44) = 0xe;

}

static void dspg_dpm_leave_sequence( dpm_state *state )
{
	/* Disable low power mode */
	*(volatile unsigned int *)(DW_CMU_BASE + DW_CMU_LPCR) &= ~(1 << 4);

	/* turn on shared memory power */
//	*(volatile unsigned int *)(DW_SYSCFG_BASE + 0x40) = 0x0;
//	*(volatile unsigned int *)(DW_SYSCFG_BASE + 0x44) = 0x0;


#if 0	
	if ( state->flags & DPM_FLAG_12_OSC_EN ) {
		/* turn on 12Mhz oscillator */
		*(volatile unsigned int *)(DW_CMU_BASE + 0x60) = 0x9480 | (*(volatile unsigned int *)(DW_CMU_BASE + 0x60) & 0x60);
	}
#endif



	*(volatile unsigned int *)(DW_CMU_BASE + 0x60) |= 0x94c0;

	if ( state->flags & DPM_FLAG_PLL4_EN ) {
		/* turn on PLL4 */
		*(volatile unsigned int *)(DW_CMU_BASE + 0x74) |= 0x0001;
	}

	if ( state->flags & DPM_FLAG_PLL3_EN ) {
		/* turn on PLL3 */
		*(volatile unsigned int *)(DW_CMU_BASE + 0x6c) |= 0x0001;
	}

	/* wait one ms - RTC counts at 32Khz, and we wait 40 cycles so it is more than 1ms */
	udelay(10000);

	/* turn on 240_EN mode */
	*(volatile unsigned int *)(DW_CMU_BASE + 0x80) |= 1;

	/* set cpu clock to run from PLL3/PLL4 */
	*(volatile unsigned int *)(DW_CMU_BASE + 0x00) = 2;

	/* restore CSR2 */
	*(volatile unsigned int *)(DW_CMU_BASE + 0x64) = state->csr2;

	/* restore CSR1 */
	*(volatile unsigned int *)(DW_CMU_BASE + 0x08) = state->csr1;

	/* restore CSR0 */
	*(volatile unsigned int *)(DW_CMU_BASE + 0x04) = state->csr0;


//	/* turn on shared memory and ETM */
//	*(volatile unsigned int *)(DW_CMU_BASE + 0x04) |= 0x000a;
}

extern void cpu_arm926_do_idle_selfrefresh(void);

void dspg_dpm_idle(unsigned int auxbgp, unsigned int cmpval, unsigned int cmpval38, unsigned int tempref, unsigned int min_mv)
{
	unsigned int offset, startptr = (0x16000000 + SRAM_IDLE_TASK_OFFSET) + ((IDLE_CODE_SIZE + 0xff) & ~0xff) ;
	dpm_state sleep_state;

	ram_idle_task = (ram_idle_task_cb*)(0x16000000 + SRAM_IDLE_TASK_OFFSET);

	// Copy the Idle task to the internal RAM
	memcpy(ram_idle_task, &cpu_arm926_do_idle_selfrefresh, IDLE_CODE_SIZE); 
	memcpy(startptr, &charger_start, (&charger_end - &charger_start));
	offset = (unsigned int)do_dpm_fastcharge - (unsigned int)&charger_start;
	offset += startptr;

	dspg_dpm_enter_sequence(&sleep_state);

	((ram_idle_task_cb)(ram_idle_task))(auxbgp, cmpval, cmpval38, tempref, min_mv, offset);

	dspg_dpm_leave_sequence(&sleep_state);
}
