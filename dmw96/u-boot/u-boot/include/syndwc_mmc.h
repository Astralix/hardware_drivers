#ifndef __SYNDWC_MMC_H
#define __SYNDWC_MMC_H

#include <common.h>

typedef int (*syndwc_cd_t)(void);

struct syndwc_pdata {
	unsigned int card_num;
	unsigned int mclk;
	unsigned int f_max;
	unsigned int host_caps;
	unsigned int voltages;

	syndwc_cd_t card_detect;
};

int syndwc_initialize(struct syndwc_pdata *pdata);

#endif

