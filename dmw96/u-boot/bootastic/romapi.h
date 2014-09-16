#ifndef __ROMAPI_H
#define __ROMAPI_H

#define CLKCHG_SYSCLK  0
#define CLKCHG_PCLK    1

enum bootmethods {
	BOOTSEL_NAND,
	BOOTSEL_NAND_NOECC,
	BOOTSEL_JTAG,
	BOOTSEL_PRODTEST,
	BOOTSEL_EMMC,
	BOOTSEL_UART,
};

/* Check for legacy bootrom which did not:
 *  - pass version
 *  - implement call tables
 *  - pass boot method
 */
static inline int rom_is_legacy(unsigned int rom_version)
{
	return rom_version == 0x0020c000;
}

struct bootrom_calls {
	int            (*read)(void *dest, unsigned long off, unsigned long size);
	unsigned long (*crc32)(void *buffer, unsigned long size);
	void     (*clkchg_pre)(int clk, unsigned long freq);
	void    (*clkchg_post)(int clk, unsigned long freq);
	int      (*xmodem_rcv)(void *dest, unsigned long size);
	void         (*udelay)(unsigned long usec);
};

extern const struct bootrom_calls *romapi;

void romapi_init(unsigned int rom_version, const struct bootrom_calls *rom_calls);

#endif
