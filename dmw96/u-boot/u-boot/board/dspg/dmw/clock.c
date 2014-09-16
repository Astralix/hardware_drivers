#include <common.h>
#include <asm/byteorder.h>
#include <asm/io.h>

#include "dmw.h"
#include "clock.h"

#define DMW_CMU_PLL1      0x00
#define DMW_CMU_PLL2      0x04
#define DMW_CMU_PLL3      0x08
#define DMW_CMU_PLL4      0x0C
#define DMW_CMU_CPUCLKCTL 0x14
#define DMW_CMU_COMCLKSEL 0x18
#define DMW_CMU_CLKSWCTL  0x1C
#define DMW_CMU_CLKDIV1   0x20
#define DMW_CMU_CLKDIV2   0x24
#define DMW_CMU_CLKDIV3   0x28
#define DMW_CMU_WRPR      0x4C
#define DMW_CMU_SWCHWRST1 0x50
#define DMW_CMU_SWCHWRST2 0x54
#define DMW_CMU_SWCLKENR1 0x58
#define DMW_CMU_SWCLKENR2 0x5C

struct clk {
	char *name;
	int clockbit;
	int resetbit;
	int (*set_rate)(struct clk *clk, unsigned long rate);
};

static void lcdc_set_rate(struct clk *clk, unsigned long rate);

#define __stringify_1(x)	#x
#define __stringify(x)		__stringify_1(x)

#define CLK(_name, _clockbit, _resetbit)    \
	struct clk _name = {                \
		.name = __stringify(_name), \
		.clockbit = _clockbit,      \
		.resetbit = _resetbit,      \
	}

#define CLK_RATE(_name, _clockbit, _resetbit, _set_rate) \
	struct clk _name = {                \
		.name = __stringify(_name), \
		.clockbit = _clockbit,      \
		.resetbit = _resetbit,      \
		.set_rate = _set_rate,      \
	}

CLK(neon,            -1, 32 + 31);
CLK(usb2_phy_por,    -1, 32 + 29);
CLK(usb2_phy_port,   -1, 32 + 28);
CLK(usb2_mac_phyif,  -1, 32 + 27);
CLK(usb1_phy_por,    -1, 32 + 25);
CLK(usb1_phy_port,   -1, 32 + 24);
CLK(usb1_mac_phyif,  -1, 32 + 23);
CLK(ccu1,            -1, 32 + 21);
CLK(ccu0,            -1, 32 + 20);
CLK(coresight,  32 + 20, 32 + 30);
CLK(pll4prediv, 32 + 19,      -1);
CLK(dbm,        32 + 18, 32 + 18);
CLK(usb2_phy,   32 + 17,      -1);
CLK(usb2_mac,   32 + 16, 32 + 26);
CLK(css_gdmac,  32 + 15, 32 + 15);
CLK(css_timer2, 32 + 14, 32 + 14);
CLK(css_timer1, 32 + 13, 32 + 13);
CLK(dram_ctl,   32 + 12, 32 + 12);
CLK(clkout,     32 + 11,      -1);
CLK(smc,        32 + 10, 32 + 10);
CLK(ir_remctl,  32 +  9, 32 +  9);
CLK(slvmii,     32 +  8, 32 +  8);
CLK(spi2,       32 +  7, 32 +  7);
CLK(spi1,       32 +  6, 32 +  6);
CLK(i2c2,       32 +  5, 32 +  5);
CLK(i2c1,       32 +  4, 32 +  4);
CLK(timer4,     32 +  3, 32 +  1);
CLK(timer3,     32 +  2, 32 +  1);
CLK(timer2,     32 +  1, 32 +  0);
CLK(timer1,     32 +  0, 32 +  0);

CLK(css_etm,         31,      31);
CLK(css_bmp,         30,      30);
CLK(css_adpcm,       29,      29);
CLK(dp,              28,      -1);
CLK(sim,             27,      -1);
CLK(viden,           26,      26);
CLK(vidde,           25,      25);
CLK(gpu,             24,      24);
CLK(css,             23,      23);
CLK(sec,             22,      22);
CLK_RATE(lcdc,       21,      21, lcdc_set_rate);
CLK(osdm,            20,      20);
CLK(ciu,             19,      19);
CLK(wifi,            18,      18);
CLK(usb_phy,         17,      -1);
CLK(usb_mac,         16, 32 + 22);
CLK(acp,             15,      15);
CLK(nfc,             14,      14);
CLK(sdmmc,           13,      13);
CLK(msc,             12,      12);
CLK(tdm3,            11,      11);
CLK(tdm2,            10,      10);
CLK(tdm1,             9,       9);
CLK(uart3,            8,       8);
CLK(uart2,            7,       7);
CLK(uart1,            6,       6);
CLK(rmii_if,          5,      -1);
CLK(secure,           4,       4);
CLK(ethmac,           3,       3);
CLK(rtc,              2,       2);
CLK(css_arm926,       1,       1);
CLK(gdmac,            0,       0);

static struct clk *clocks[] = {
	&neon,
	&usb2_phy_por,
	&usb2_phy_port,
	&usb2_mac_phyif,
	&usb1_phy_por,
	&usb1_phy_port,
	&usb1_mac_phyif,
	&ccu1,
	&ccu0,
	&coresight,
	&pll4prediv,
	&dbm,
	&usb2_phy,
	&usb2_mac,
	&css_gdmac,
	&css_timer2,
	&css_timer1,
	&dram_ctl,
	&clkout,
	&smc,
	&ir_remctl,
	&slvmii,
	&spi2,
	&spi1,
	&i2c2,
	&i2c1,
	&timer4,
	&timer3,
	&timer2,
	&timer1,
	&css_etm,
	&css_bmp,
	&css_adpcm,
	&dp,
	&sim,
	&viden,
	&vidde,
	&gpu,
	&css,
	&sec,
	&lcdc,
	&osdm,
	&ciu,
	&wifi,
	&usb_phy,
	&usb_mac,
	&acp,
	&nfc,
	&sdmmc,
	&msc,
	&tdm3,
	&tdm2,
	&tdm1,
	&uart3,
	&uart2,
	&uart1,
	&rmii_if,
	&secure,
	&ethmac,
	&rtc,
	&css_arm926,
	&gdmac,
};

static unsigned long
dmw_calc_pllfreq(unsigned long input_freq, int pll_ctrl)
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

static unsigned long
dmw_get_sysclk(void)
{
	uint32_t pll1, clkcntrl = readl(DMW_CMU_BASE + DMW_CMU_CPUCLKCTL);

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
			pll1 = readl(DMW_CMU_BASE + DMW_CMU_PLL1);
			return dmw_calc_pllfreq(12000000ul, pll1);
	}

	return 0;
}

static unsigned long
dmw_get_pll4(void)
{
	/* FIXME: only valid for CONFIG_DMW_OVERDRIVE_CPU==972MHz or no overdrive */
	return 297000000UL;
}

static unsigned int
clk_divider(unsigned long parent_rate, unsigned long f, int bits)
{
	int div = ((parent_rate + f/2) / f) - 1;

	if (div < 0)
		div = 0;
	else if (div >= (1 << bits))
		div = (1 << bits) - 1;

	return div;
}

static void
lcdc_set_rate(struct clk *clk, unsigned long rate)
{
	unsigned long reg, lcddivsrc;

	if (!rate)
		return;

	reg = readl(DMW_CMU_BASE + DMW_CMU_CLKDIV1);
	reg &= ~(0xff << 8);

	lcddivsrc = readl(DMW_CMU_BASE + DMW_CMU_CLKSWCTL) & (2 << 17);
	if (lcddivsrc) { /* lcddivsrc: pll4 */
		reg |= (clk_divider(dmw_get_pll4(), rate, 8) << 8);   /* lcdpixdiv */
	} else {         /* lcddivsrc: pll1 */
		reg |= (clk_divider(dmw_get_sysclk(), rate, 8) << 8); /* lcdpixdiv */
	}

	writel(reg, DMW_CMU_BASE + DMW_CMU_CLKDIV1);
}

unsigned long
dmw_get_pclk(void)
{
	uint32_t clkcntrl = readl(DMW_CMU_BASE + DMW_CMU_CPUCLKCTL);

	/* divide by SYSBUSDIV */
	return dmw_get_sysclk() / (((clkcntrl >> 4) & 0x0f) + 1) / 2;
}

unsigned long
dmw_get_cpufreq(void)
{
	uint32_t pll2, clkcntrl = readl(DMW_CMU_BASE + DMW_CMU_CPUCLKCTL);

	switch ((clkcntrl >> 8) & 3) {
		case 0: /* CPUICLK */
			switch ((clkcntrl >> 16) & 3) {
				case 0: return 12000000ul;
				case 1: return 20000000ul;
				case 2: return 13824000ul;
			}
			break;

		case 1: /* PLL2 */
			pll2 = readl(DMW_CMU_BASE + DMW_CMU_PLL2);
			return dmw_calc_pllfreq(12000000ul, pll2);

		case 2: /* ALTSYSSRC */
			switch ((clkcntrl >> 16) & 3) {
				case 0: return 12000000ul;
				case 1: return 20000000ul;
				case 2: return 13824000ul;
				case 3: return    32768ul;
			}
			break;
	}

	return 0;
}

unsigned long
dmw_get_dramfreq(void)
{
	uint32_t pll3, clkcntrl = readl(DMW_CMU_BASE + DMW_CMU_CLKSWCTL);

	if (clkcntrl & 1) {
		pll3 = readl(DMW_CMU_BASE + DMW_CMU_PLL3);
		return dmw_calc_pllfreq(12000000ul, pll3);
	} else
		return 12000000ul;
}

unsigned long 
dmw_get_slowclkfreq(void) 
{
	/* feed the rtc with 32k clock */
	if (readl(DMW_CMU_BASE + DMW_CMU_CLKSWCTL) & (1 << 16)) {
		return 32768;
	} else  {
		unsigned int div = 0;
		/*Check if the devider enabled*/
		if ( readl(DMW_CMU_BASE + DMW_CMU_CLKSWCTL) & ( 0x1 << 3) )
			div = (readl(DMW_CMU_BASE + DMW_CMU_CLKSWCTL) >> 0x4 ) & 0x7ff;
		return 12000000 / (div+1);
	}
}

static void
loop(unsigned int loops)
{
	__asm__ volatile (
		"   adds %0, %1, #1\n"
		"1: subs %0, %1, #1\n"
		"   bne  1b"
		:"=r" (loops)
		:"0" (loops)
	);
}

static void
clk_overdrive(int cpu_mhz)
{
	uint32_t reg;

	/* cpu feed: cpuiclk */
	reg = readl(DMW_CMU_BASE + DMW_CMU_CPUCLKCTL);
	reg &= ~(3 << 8);
	writel(reg, DMW_CMU_BASE + DMW_CMU_CPUCLKCTL);

	/* pll2: power down */
	reg = readl(DMW_CMU_BASE + DMW_CMU_PLL2);
	reg |= (1 << 15);
	writel(reg, DMW_CMU_BASE + DMW_CMU_PLL2);

	loop(1);

	/* pll2: multiplicator 72 or 81 */
	reg &= ~(0x7f << 5);
	reg |= (((cpu_mhz)/12)-1) << 5;
	writel(reg, DMW_CMU_BASE + DMW_CMU_PLL2);

	loop(1);

	/* pll2: active */
	reg &= ~(1 << 15);
	writel(reg, DMW_CMU_BASE + DMW_CMU_PLL2);

	/* wait for lock */
	do {
		reg = readl(DMW_CMU_BASE + DMW_CMU_PLL2);
	} while ((reg & (1 << 18)) == 0);

	/* cpu feed: pll2 */
	reg = readl(DMW_CMU_BASE + DMW_CMU_CPUCLKCTL);
	reg |= (1 << 8);
	writel(reg, DMW_CMU_BASE + DMW_CMU_CPUCLKCTL);
}

void
clk_init(void)
{
	uint32_t reg;

#ifdef CONFIG_DMW_OVERDRIVE
/* CPU overdrive frequency should be defined to 864 or 972 Mhz */
#ifndef CONFIG_DMW_OVERDRIVE_CPU
#define CONFIG_DMW_OVERDRIVE_CPU 972
#endif
#if CONFIG_DMW_OVERDRIVE_CPU!=864 && CONFIG_DMW_OVERDRIVE_CPU!=972
#error CONFIG_DMW_OVERDRIVE_CPU should be defined to 864/972
#endif
	clk_overdrive(CONFIG_DMW_OVERDRIVE_CPU);
#endif

	/* arm926 (css) */
#ifdef CONFIG_DMW_LCDONLY
	reg = ( 0 <<  0) |		/* pre-div:         1 */
	      (24 <<  5) |		/* multiplicator:  25 */
	      ( 0 << 12) |		/* output-div:      1 */
	      ( 0 << 14) |		/* band:          300 - 600 Mhz */
	      ( 1 << 15) |		/* power-down:      1 */
	      ( 0 << 16) |		/* bypass:          0 */
	      ( 3 << 19);		/* input:        pll4 prediv (pll2) */
	writel(reg, DMW_CMU_BASE + DMW_CMU_PLL4);

	/* wait atleast 500 ns */
	loop(100);

	reg &= ~(1 << 15);		/* power-down:      0 */
	writel(reg, DMW_CMU_BASE + DMW_CMU_PLL4);

#else /* CONFIG_DMW_LCDONLY */

	clk_enable("pll4prediv");

	reg = readl(DMW_CMU_BASE + DMW_CMU_CLKDIV2);
	reg &= ~(   3 <<  4);		/* pll4altsrc:   pll2 */
	reg &= ~(0xff << 24);

# ifdef CONFIG_DMW_OVERDRIVE
#  if CONFIG_DMW_OVERDRIVE_CPU==864
		reg |= (63 << 24);	/* pll4prediv:     64 */
#  else
		reg |= (71 << 24);	/* pll4prediv:     72 */
#  endif
# else
		reg |= (55 << 24);	/* pll4prediv:     56 */
# endif
	writel(reg, DMW_CMU_BASE + DMW_CMU_CLKDIV2);

	reg = ( 0 <<  0) |		/* pre-div:         1 */
	      (43 <<  5) |		/* multiplicator:  44 */
	      ( 1 << 12) |		/* output-div:      2 */
	      ( 0 << 14) |		/* band:          300 - 600 Mhz */
	      ( 1 << 15) |		/* power-down:      1 */
	      ( 0 << 16) |		/* bypass:          0 */
	      ( 3 << 19);		/* input:        pll4 prediv (pll2) */
	writel(reg, DMW_CMU_BASE + DMW_CMU_PLL4);

	/* wait atleast 500 ns */
	loop(100);

	reg &= ~(1 << 15);		/* power-down:      0 */
	writel(reg, DMW_CMU_BASE + DMW_CMU_PLL4);
#endif

	/* wait for lock */
	do {
		reg = readl(DMW_CMU_BASE + DMW_CMU_PLL4);
	} while ((reg & (1 << 18)) == 0);

	reg = readl(DMW_CMU_BASE + DMW_CMU_COMCLKSEL);
	reg &= ~(0xf <<  0);
	reg |=  (  1 <<  0);             /* comaltdiv:      2 */
	reg &= ~(  3 << 10);
	reg |=  (  1 << 10);             /* swcomsrc:      pll4 output */
	writel(reg, DMW_CMU_BASE + DMW_CMU_COMCLKSEL);

	/* dram, lcdc and ethernet clocksource */
	reg = readl(DMW_CMU_BASE + DMW_CMU_CLKSWCTL);
	reg &= ~(3 << 17);
#ifdef CONFIG_DMW_LCDONLY
	reg |=  (0 << 17);             /* lcddivsrc:     pll1 */
#else
	reg |=  (2 << 17);             /* lcddivsrc:     pll4 */
#endif
	reg |=  (1 << 25);             /* ethrdivsrc:    pll4 */
	reg |=  (1 << 15);             /* mii_clk_src:   off-chip */
	writel(reg, DMW_CMU_BASE + DMW_CMU_CLKSWCTL);

	/* ciu, lcdc, sonyms, sd/mmc dividers */
	reg = (19 <<  0) |             /* ciudiv:          20 */
	      (clk_divider(dmw_get_sysclk(), 26000000ul, 8) <<  8) |    /* lcdpixdiv: 26MHz */
	      (clk_divider(dmw_get_sysclk(), 40000000ul, 8) << 16) |    /* msdiv: 40MHz */
	      (clk_divider(dmw_get_sysclk(), 48000000ul, 8) << 24);     /* sddiv: 48MHz */
	writel(reg, DMW_CMU_BASE + DMW_CMU_CLKDIV1);

	/* vid encoder, vid decoder, gpu, ethernet */
	reg = readl(DMW_CMU_BASE + DMW_CMU_CLKDIV2);
	reg &= ~(0x00ffff0f);          /* leave pll4 intact   */
	reg |=  (1 <<  0) |            /* vidediv:          2 */
	        (1 <<  8) |            /* vidddiv:          2 */
	        (0 << 16) |            /* gpudiv:           1 */
	        (5 << 20);             /* ethrdiv:          6 */
	writel(reg, DMW_CMU_BASE + DMW_CMU_CLKDIV2);

	/* dp, sim */
	reg = (23 << 8);               /* simdiv:          24 */
	writel(reg, DMW_CMU_BASE + DMW_CMU_CLKDIV3);

	/* adjust SLOWDIV to 40kHz in case we're running from it */
	reg = readl(DMW_CMU_BASE + DMW_CMU_CLKSWCTL);
	reg &= ~(0x7ff << 4);
	reg |= (299 << 4);
	writel(reg, DMW_CMU_BASE + DMW_CMU_CLKSWCTL);
}

static struct clk *
clk_get(const char *id)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(clocks); i++) {
		if (!strcmp(id, clocks[i]->name))
			return clocks[i];
	}

	return NULL;
}

static void
clock_set_onoff(struct clk *clk, int on)
{
	int tmp, bit = clk->clockbit, reg = DMW_CMU_SWCLKENR2;

	if (bit == -1)
		return;
	else if (bit < 32)
		reg = DMW_CMU_SWCLKENR1;
	else
		bit -= 32;

	tmp = readl(DMW_CMU_BASE + reg);
	if (on)
		tmp |=  (1 << bit);
	else
		tmp &= ~(1 << bit);
	writel(0x9 << 4, DMW_CMU_BASE + DMW_CMU_WRPR);
	writel(tmp, DMW_CMU_BASE + reg);
}

void
clk_set_rate(char *id, unsigned long rate)
{
	struct clk *clock = clk_get(id);

	if (clock && clock->set_rate)
		clock->set_rate(clock, rate);
}

void
clk_enable(char *id)
{
	struct clk *clock = clk_get(id);

	if (clock)
		clock_set_onoff(clock, 1);
}

void
clk_disable(char *id)
{
	struct clk *clock = clk_get(id);

	if (clock)
		clock_set_onoff(clock, 0);
}

static void
reset_set_onoff(struct clk *clk, int on)
{
	int tmp, bit = clk->resetbit, reg = DMW_CMU_SWCHWRST2;

	if (bit == -1)
		return;
	else if (bit < 32)
		reg = DMW_CMU_SWCHWRST1;
	else
		bit -= 32;

	tmp = readl(DMW_CMU_BASE + reg);
	if (on)
		tmp |=  (1 << bit);
	else
		tmp &= ~(1 << bit);
	writel(0x6, DMW_CMU_BASE + DMW_CMU_WRPR);
	writel(tmp, DMW_CMU_BASE + reg);
}

void
reset_set(char *id)
{
	struct clk *clock = clk_get(id);

	if (clock)
		reset_set_onoff(clock, 1);
}

void
reset_release(char *id)
{
	struct clk *clock = clk_get(id);

	if (clock)
		reset_set_onoff(clock, 0);
}
