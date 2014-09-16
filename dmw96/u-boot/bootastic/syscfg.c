#include "common.h"
#include "hardware.h"
#include "syscfg.h"

void pad_enable(unsigned int pad)
{
	uint32_t reg;
	int off = pad / 32 * 4;

	pad %= 32;

	reg = readl(DMW96_SCON_BASE + DMW96_SCON_SPCR1 + off);
	reg |= (1 << pad);
	writel(reg, DMW96_SCON_BASE + DMW96_SCON_SPCR1 + off);
}

void syscfg_set_romws(unsigned int waitstates)
{
	uint32_t reg;

	waitstates &= 3;

	reg = readl(DMW96_SCON_BASE + DMW96_SCON_GCR1);
	reg &= ~3;
	reg |= waitstates;
	writel(reg, DMW96_SCON_BASE + DMW96_SCON_GCR1);
}

void syscfg_dram_set_sus(enum dram_pin_groups group, int enable)
{
	int regnum = (group / 3) * 4;
	int off = (group % 3) * 9;
	uint32_t reg, bit;

	bit = (1 << (2 + off));

	reg = readl(DMW96_SCON_BASE + DMW96_SCON_DRAM_CFG0 + regnum);

	if (enable)
		reg |= bit;
	else
		reg &= ~bit;

	writel(reg, DMW96_SCON_BASE + DMW96_SCON_DRAM_CFG0 + regnum);
}

void syscfg_dram_set_coherency_control(void)
{
	uint32_t reg;

	reg = readl(DMW96_SCON_BASE + DMW96_SCON_DRAMCTL_GCR1);
	reg |= 0x1e; /* AXI ports 0..3 */
	writel(reg, DMW96_SCON_BASE + DMW96_SCON_DRAMCTL_GCR1);
}
