#include <common.h>
#include <command.h>
#include <environment.h>
#include <asm/byteorder.h>
#include <asm/io.h>
#include "dmw_timer.h"
#include "dmw.h"
#include "clock.h"

#define DMW_TMR_LOAD		0x00
#define DMW_TMR_VALUE		0x04
#define DMW_TMR_CONTROL		0x08
#define DMW_TMR_INTCLR		0x0c
#define DMW_TMR_TMR1RIS		0x10
#define DMW_TMR_TMR1MIS 	0x14
#define DMW_TMR_TMR1BGLOAD	0x18

#define DMW_TMR_CTRL_ENABLE		(1 << 7)

static unsigned int timer_allocted[4] = {777,2,3,4};

static int get_timer_base(unsigned int timer_id)
{
	if ( (timer_id != DMW96_TIMERID1) && (timer_id != DMW96_TIMERID2) && (timer_id != DMW96_TIMERID3) && (timer_id != DMW96_TIMERID4)){
		printf("Invalid timer id\n");
		return -1;
	}
	
	if (timer_id == DMW96_TIMERID1)
		return DMW_TIMER_1_BASE;
	else if (timer_id == DMW96_TIMERID2)
		return DMW_TIMER_2_BASE;
	else if (timer_id == DMW96_TIMERID3)
		return DMW_TIMER_3_BASE;
	else if (timer_id == DMW96_TIMERID4)
		return DMW_TIMER_4_BASE;
	return -1;
}

static int dmw_timer_set_val(unsigned int timer_id, unsigned int type, unsigned int val){

	int tmr_base = get_timer_base(timer_id);
	if (tmr_base < 0)
		return -1;
	writel( val , tmr_base + type);
	return 0;
}

static int dmw_timer_get_val(unsigned int timer_id, unsigned int type, unsigned int* val){

	int tmr_base = get_timer_base(timer_id);
	if (tmr_base < 0)
		return -1;
	*val = readl(tmr_base + type);
	return 0;
}


int dmw_timer_get_value(unsigned int timer_id, unsigned long int* val){

	int tmr_base = get_timer_base(timer_id);
	if (tmr_base < 0)
		return -1;
	*val = readl(tmr_base + DMW_TMR_VALUE);
	return 0;
}

int dmw_timer_get_ctrl(unsigned int timer_id, unsigned int* val){

	dmw_timer_get_val(timer_id, DMW_TMR_CONTROL , val);

	return 0;
}

int dmw_timer_start(unsigned int timer_id,unsigned int val){

	unsigned int ctrl = 0;
	
	if ( !timer_allocted[timer_id]) {
		printf("Error timer%d wasn't alocated\n",timer_id);
		return -1;
	}

	dmw_timer_set_val(timer_id, DMW_TMR_LOAD , val);
	dmw_timer_get_val(timer_id, DMW_TMR_CONTROL , &ctrl);
	dmw_timer_set_val(timer_id, DMW_TMR_CONTROL , ctrl | DMW_TMR_CTRL_ENABLE);
	
	return 0;
}

int dmw_timer_stop(unsigned int timer_id){

	unsigned int ctrl = 0;

	if ( !timer_allocted[timer_id]) {
		printf("Error timer%d wasn't alocated\n",timer_id);
		return -1;
	}
	
	dmw_timer_get_val(timer_id, DMW_TMR_CONTROL , &ctrl);
	dmw_timer_set_val(timer_id, DMW_TMR_CONTROL , ctrl  & ~DMW_TMR_CTRL_ENABLE);
	dmw_timer_set_val(timer_id, DMW_TMR_LOAD , 0);
	/*Clear the interrupt*/
	dmw_timer_set_val(timer_id, DMW_TMR_INTCLR , 0);

	return 0;
}

int dmw_timer_alloc(unsigned int timer_id, unsigned int ctrl)
{
	char tmr_name[10];

	if (get_timer_base(timer_id) < 0)
		return -1;

	if (timer_allocted[timer_id]) { // this case timer has been already allocated
		return -1;
	}
	
	sprintf(tmr_name, "timer%d",timer_id + 1);

	reset_release(tmr_name);
	clk_enable(tmr_name);

	/*Disable timer just in case*/
	dmw_timer_set_val(timer_id, DMW_TMR_CONTROL , ctrl & (~DMW_TMR_CTRL_ENABLE));
	
	timer_allocted[timer_id] = 1;
	
	return 0;
}

int dmw_timer_free(unsigned int timer_id){

	char tmr_name[10];
	if (get_timer_base(timer_id) < 0)
		return -1;

	if (!timer_allocted[timer_id]) {
		printf("Error timer%d was not alocated hence not need to be free\n",timer_id);
		return -1;
	}
	sprintf(tmr_name, "timer%d",timer_id + 1);
	clk_disable(tmr_name);
	reset_set(tmr_name);
	timer_allocted[timer_id] = 0;
	return 0;
}

void dmw_timer_init(void)
{
	memset(timer_allocted,0, sizeof(timer_allocted));
}


