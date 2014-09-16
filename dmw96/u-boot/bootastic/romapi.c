#include "common.h"
#include "hardware.h"
#include "legacy_nand.h"
#include "romapi.h"
#include "syscfg.h"

static void legacy_clkchg_pre(int clk, unsigned long freq)
{
	/*
	 * before we switch to fast clocks, we have to assure that the bootrom
	 * is not accessed faster than 80 mhz. we do this by inserting wait
	 * states.
	 */
	syscfg_set_romws(1);
}

static void legacy_udelay(unsigned long usec)
{
	delay(0xf000); /* TODO */
}

static const struct bootrom_calls legacy_calls = {
	.read        = legacy_nand_read,
	.crc32       = (void *)(DMW96_ROM_BASE + 0x0264),
	.clkchg_pre  = legacy_clkchg_pre,
	.clkchg_post = legacy_nand_clkchg,
	.udelay      = legacy_udelay,
};

const struct bootrom_calls *romapi;

void romapi_init(unsigned int rom_version, const struct bootrom_calls *rom_calls)
{
	if (rom_is_legacy(rom_version))
		rom_calls = &legacy_calls;

	/* currently only one known version -> use table directly */
	romapi = rom_calls;
}
