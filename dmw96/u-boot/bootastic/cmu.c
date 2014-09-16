/*
 * (C) Copyright 2010
 * DSPG Technologies GmbH
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

#include "common.h"
#include "hardware.h"
#include "romapi.h"

#include "cmu.h"
#include "ddrconf/ddrconf.h"

/* CPU frequency is always 756Mhz on bootastic, since for higher frequencies */
/* we have to enable GPIO for power-drive */
#define CPU_FREQ	756

#define WRPR_RSTEN       (0x6 << 0)
#define WRPR_CLKEN       (0x9 << 4)

void reset_release(unsigned int block)
{
	unsigned int off = (block / 32) * sizeof(uint32_t);
	uint32_t tmp;

	block &= 0x1f;

	tmp = readl(DMW96_CMU_BASE + DMW96_CMU_SWRST1 + off);
	tmp &= ~(1 << block);

	writel(WRPR_RSTEN, DMW96_CMU_BASE + DMW96_CMU_WRPR);
	writel(tmp, DMW96_CMU_BASE + DMW96_CMU_SWRST1 + off);
}

void clk_enable(unsigned int block)
{
	unsigned int off = (block / 32) * sizeof(uint32_t);
	uint32_t tmp;

	block &= 0x1f;

	tmp = readl(DMW96_CMU_BASE + DMW96_CMU_SWCLKEN1 + off);
	tmp |= (1 << block);

	writel(WRPR_CLKEN, DMW96_CMU_BASE + DMW96_CMU_WRPR);
	writel(tmp, DMW96_CMU_BASE + DMW96_CMU_SWCLKEN1 + off);
}

static void pll_setup(void)
{
	uint32_t reg;

	/* system clock setup */
	reg = ( 0 <<  0) |		/* pre-div:         1 */
	      (39 <<  5) |		/* multiplicator:  40 */
	      ( 0 << 12) |		/* output-div:      1 */
	      ( 0 << 14) |		/* band:          300 - 600 Mhz */
	      ( 0 << 16) |		/* bypass:          0 */
	      ( 0 << 19);		/* input:          12 MHz */
	writel(reg, DMW96_CMU_BASE + DMW96_CMU_PLL1);

	reg = readl(DMW96_CMU_BASE + DMW96_CMU_CPUCLK);
	reg &= ~(1 << 11);		/* pll1 power-down: 0 */
	writel(reg, DMW96_CMU_BASE + DMW96_CMU_CPUCLK);

	do {
		reg = readl(DMW96_CMU_BASE + DMW96_CMU_PLL1);
	} while ((reg & (1 << 18)) == 0); /* wait for pll to lock */

	/* cpu clock setup */
	reg = ( 0 <<  0) |					/* pre-div:         1 */
	      ((((CPU_FREQ)/12)-1) <<  5) |	/* multiplicator:  (CPU_FREQ/12)-1, so output freq is CPU_FREQ */
	      ( 0 << 12) |					/* output-div:      1 */
	      ( 1 << 14) |					/* band:          500 - 1000 Mhz */
	      ( 1 << 15) |					/* power-down:      1 */
	      ( 0 << 16) |					/* bypass:          0 */
	      ( 0 << 19);					/* input:          12 MHz */

	writel(reg, DMW96_CMU_BASE + DMW96_CMU_PLL2);


	/* wait atleast 500 ns */
	delay(1);

	reg &= ~(1 << 15);		/* power-down:      0 */
	writel(reg, DMW96_CMU_BASE + DMW96_CMU_PLL2);

	do {
		reg = readl(DMW96_CMU_BASE + DMW96_CMU_PLL2);
	} while ((reg & (1 << 18)) == 0); /* wait for pll to lock */

	/* switch clocks and dividers */
	reg = ( 1 <<  4) |		/* sysbusdiv:       4 */
	      ( 1 <<  8) |		/* cpu feeder:      pll2 */
	      ( 0 << 11) |		/* pll1 power-down: 0 */
	      ( 2 << 12);		/* sys feeder:      pll1 */

	writel(reg, DMW96_CMU_BASE + DMW96_CMU_CPUCLK);

	/* dram clock setup */
	reg = ( 0 <<  0) |		/* pre-div:         1 */
	      ( 0 << 14) |		/* band:          300 - 600 Mhz */
	      ( 1 << 15) |		/* power-down:      1 */
	      ( 0 << 16) |		/* bypass:          0 */
	      ( 0 << 19);		/* input:          12 MHz */
	reg |= (ddrconf_get_pll3mult() << 5);
	reg |= (ddrconf_get_pll3outdiv() << 12);
	writel(reg, DMW96_CMU_BASE + DMW96_CMU_PLL3);

	/* wait atleast 500 ns */
	delay(100);

	reg &= ~(1 << 15);		/* power-down:      0 */
	writel(reg, DMW96_CMU_BASE + DMW96_CMU_PLL3);
}

static void per_setup(void)
{
	uint32_t reg;

	/* dram clocksource */
	reg = readl(DMW96_CMU_BASE + DMW96_CMU_CLKSWCTL);
	reg |=  (1 <<  0);             /* swdramsrc:     pll3 */
	writel(reg, DMW96_CMU_BASE + DMW96_CMU_CLKSWCTL);
}

void cmu_setup(void)
{
	if (romapi->clkchg_pre) {
		romapi->clkchg_pre(CLKCHG_PCLK,   120000000);
		romapi->clkchg_pre(CLKCHG_SYSCLK, 480000000);
	}

	pll_setup();
	per_setup();

	if (romapi->clkchg_post) {
		romapi->clkchg_post(CLKCHG_PCLK,   120000000);
		romapi->clkchg_post(CLKCHG_SYSCLK, 480000000);
	}
}

static unsigned long
cmu_calc_pllfreq(unsigned long input_freq, int pll_ctrl)
{
	uint32_t r, f, od;
	unsigned long pll_out_freq;

	if (pll_ctrl & (1<<16)) {
		/* bypassed */
		pll_out_freq = input_freq;
	} else {
		r = (pll_ctrl & 0x1f) + 1;
		f = ((pll_ctrl >> 5) & 0x7f) + 1;
		od = 1 << ((pll_ctrl >> 12) & 3);

		pll_out_freq = input_freq * f / r / od;
	}

	return pll_out_freq;
}

unsigned long cmu_get_sysclk(void)
{
	uint32_t pll1, clkcntrl = readl(DMW96_CMU_BASE + DMW96_CMU_CPUCLK);

	switch ((clkcntrl >> 12) & 3) {
		case 0: /* CPUICLK */
			switch ((clkcntrl >> 16) & 3) {
				case 0: return 12000000ul;
				case 1: return 20000000ul;
				case 2: return 13824000ul;
			}
			break;
		case 1: /* ALTSYSSRC */
			switch ((clkcntrl >> 16) & 3) {
				case 0: return 12000000ul;
				case 1: return 20000000ul;
				case 2: return 13824000ul;
				case 3: return    32768ul;
			}
			break;
		case 2: /* PLL1 */
			pll1 = readl(DMW96_CMU_BASE + DMW96_CMU_PLL1);
			return cmu_calc_pllfreq(12000000ul, pll1);
	}

	return 0;
}

unsigned long cmu_get_pclk(void)
{
	uint32_t clkcntrl = readl(DMW96_CMU_BASE + DMW96_CMU_CPUCLK);

	/* divide by SYSBUSDIV */
	return cmu_get_sysclk() / (((clkcntrl >> 4) & 0x0f) + 1) / 2;
}
