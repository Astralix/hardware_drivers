#ifndef DMW_CLOCK_H
#define DMW_CLOCK_H

void clk_init(void);
void clk_enable(char *id);
void clk_disable(char *id);
void clk_set_rate(char *id, unsigned long rate);

void reset_set(char *id);
void reset_release(char *id);

unsigned long dmw_get_slowclkfreq(void);

#endif
