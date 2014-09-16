#include <common.h>
#include <asm/byteorder.h>
#include <asm/io.h>

#include "dw.h"
#include "clock.h"

struct clk {
	char *name;
	int enable_bit;
};

#define __stringify_1(x)	#x
#define __stringify(x)		__stringify_1(x)

#define CLK_GATED(_name, _bit)              \
	struct clk _name = {                \
		.name = __stringify(_name), \
		.enable_bit = _bit,         \
	}

CLK_GATED(usb2, 39);
CLK_GATED(sdmmc, 38);
CLK_GATED(lcdc, 36);
CLK_GATED(ahb3, 31);
CLK_GATED(ethmacb, 30);
CLK_GATED(ethmaca, 29);
CLK_GATED(spi, 27);
CLK_GATED(tmr2, 24);
CLK_GATED(tmr1, 23);
CLK_GATED(sbus, 22);
CLK_GATED(uart2, 21);
CLK_GATED(uart1, 20);
CLK_GATED(fc, 14);
CLK_GATED(dma, 13);
CLK_GATED(dmafifo, 12);
CLK_GATED(usb, 8);
CLK_GATED(mpmc, 4);
CLK_GATED(shmem, 3);
CLK_GATED(intmem, 2);
CLK_GATED(etm, 1);

static struct clk *onchip_clks[] = {
	&usb2,
	&sdmmc,
	&lcdc,
	&ahb3,
	&ethmacb,
	&ethmaca,
	&spi,
	&tmr2,
	&tmr1,
	&sbus,
	&uart2,
	&uart1,
	&fc,
	&dma,
	&dmafifo,
	&usb,
	&mpmc,
	&shmem,
	&intmem,
	&etm,
};

static struct clk *
clk_get(const char *id)
{
	int i;
	struct clk *clock;

	for (i = 0; i < ARRAY_SIZE(onchip_clks); i++) {
		clock = onchip_clks[i];

		if (!strcmp(id, clock->name))
			return clock;
	}

	return NULL;
}

static void
clock_set_onoff(struct clk *clk, int on)
{
	int tmp, enable_bit, reg = DW_CMU_CSR2;

	enable_bit = clk->enable_bit;

	if (clk->enable_bit < 16) {
		reg = DW_CMU_CSR0;
	} else if (clk->enable_bit < 32) {
		reg = DW_CMU_CSR1;
		enable_bit -= 16;
	} else {
		enable_bit -= 32;
	}

	tmp = readl(DW_CMU_BASE + reg);
	if (on)
		tmp |=  (1 << enable_bit);
	else
		tmp &= ~(1 << enable_bit);
	writel(tmp, DW_CMU_BASE + reg);
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
