#include <common.h>
#include <asm/byteorder.h>
#include <asm/io.h>

#include "dmw.h"
#include "pads.h"

struct pads {
	char *name;
	int enable_bit;
};

#define __stringify_1(x)	#x
#define __stringify(x)		__stringify_1(x)

#define PADS(_name, _bit)                   \
	struct pads pads_##_name = {               \
		.name = __stringify(_name), \
		.enable_bit = _bit,         \
	}

PADS(btif, 31);
PADS(memboot, 30);
PADS(bb, 29);
PADS(ir_remctl, 28);
PADS(fc, 27);
PADS(uart1, 26);
PADS(uart2, 25);
PADS(uart3, 24);
PADS(tdm1, 23);
PADS(tdm2, 22);
PADS(tdm3, 21);
PADS(spi1, 20);
PADS(spi2, 19);
PADS(slvrmii, 18);
PADS(i2c1, 17);
PADS(i2c2, 16);
PADS(sdmmc, 15);
PADS(emaca, 14);
PADS(ms, 13);
PADS(emmc, 12);
PADS(slvmii, 11);
PADS(lcdc, 10);
PADS(lcdgp8, 8);
PADS(lcdgp7, 7);
PADS(lcdgp6, 6);
PADS(lcdgp5, 5);
PADS(lcdgp4, 4);
PADS(lcdgp3, 3);
PADS(lcdgp2, 2);
PADS(lcdgp1, 1);
PADS(lcdgp0, 0);
PADS(wifi_ser_keep, 48);
PADS(bmp, 38);
PADS(ciu, 35);
PADS(dect_if, 34);
PADS(dp_clk, 33);
PADS(sim, 32);

static struct pads *pads[] = {
	&pads_btif,
	&pads_memboot,
	&pads_bb,
	&pads_ir_remctl,
	&pads_fc,
	&pads_uart1,
	&pads_uart2,
	&pads_uart3,
	&pads_tdm1,
	&pads_tdm2,
	&pads_tdm3,
	&pads_spi1,
	&pads_spi2,
	&pads_slvrmii,
	&pads_i2c1,
	&pads_i2c2,
	&pads_sdmmc,
	&pads_emaca,
	&pads_ms,
	&pads_emmc,
	&pads_slvmii,
	&pads_lcdc,
	&pads_lcdgp8,
	&pads_lcdgp7,
	&pads_lcdgp6,
	&pads_lcdgp5,
	&pads_lcdgp4,
	&pads_lcdgp3,
	&pads_lcdgp2,
	&pads_lcdgp1,
	&pads_lcdgp0,
	&pads_wifi_ser_keep,
	&pads_bmp,
	&pads_ciu,
	&pads_dect_if,
	&pads_dp_clk,
	&pads_sim,
};

static struct pads *
pads_get(const char *id)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(pads); i++) {
		if (!strcmp(id, pads[i]->name))
			return pads[i];
	}

	return NULL;
}

#define DMW_SYSCFG_SPCR1 0x54
#define DMW_SYSCFG_SPCR2 0x58

static void
pads_set_onoff(struct pads *pads, int on)
{
	int tmp, enable_bit, reg = DMW_SYSCFG_SPCR2;

	enable_bit = pads->enable_bit;

	if (pads->enable_bit < 32)
		reg = DMW_SYSCFG_SPCR1;
	else
		enable_bit -= 32;

	tmp = readl(DMW_SYSCFG_BASE + reg);
	if (on)
		tmp |=  (1 << enable_bit);
	else
		tmp &= ~(1 << enable_bit);
	writel(tmp, DMW_SYSCFG_BASE + reg);
}

void
pads_enable(const char *id)
{
	struct pads *pads = pads_get(id);

	if (pads)
		pads_set_onoff(pads, 1);
}

void
pads_disable(const char *id)
{
	struct pads *pads = pads_get(id);

	if (pads)
		pads_set_onoff(pads, 0);
}
