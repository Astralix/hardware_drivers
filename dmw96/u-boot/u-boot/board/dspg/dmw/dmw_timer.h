#ifndef __BOARD_DSPG_DMW_TIMERS_H
#define __BOARD_DSPG_DMW_TIMERS_H

/*Control*/
#define DMW_TMR_CTRL_ONESHOT	(1 << 0)
#define DMW_TMR_CTRL_32BIT		(1 << 1)
#define DMW_TMR_CTRL_DIV1		(0 << 2)
#define DMW_TMR_CTRL_DIV16		(1 << 2)
#define DMW_TMR_CTRL_DIV256		(2 << 2)
#define DMW_TMR_CTRL_IE			(1 << 5)
#define DMW_TMR_CTRL_PERIODIC	(1 << 6)

enum {DMW96_TIMERID1, DMW96_TIMERID2 ,DMW96_TIMERID3 ,DMW96_TIMERID4};

void dmw_timer_init(void);

int dmw_timer_get_value(unsigned int timer_id, unsigned long int* val);
int dmw_timer_get_ctrl(unsigned int timer_id, unsigned int* val);

int dmw_timer_alloc(unsigned int timer_id, unsigned int ctrl);
int dmw_timer_free(unsigned int timer_id);

int dmw_timer_start(unsigned int timer_id,unsigned int val);
int dmw_timer_stop(unsigned int timer_id);

#endif 
