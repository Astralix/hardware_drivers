#ifndef __DDRCONF_DDRCONF_H
#define __DDRCONF_DDRCONF_H

#include "../types.h"

uint32_t ddrconf_get_pll3mult(void);
uint32_t ddrconf_get_pll3outdiv(void);

extern uint32_t denali_setup[92];
extern uint32_t denali_phy_setup[39];

#endif
