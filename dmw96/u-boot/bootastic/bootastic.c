#include "version.h"
#include "common.h"
#include "bootrom.h"
#include "hardware.h"
#include "romapi.h"
#include "serial.h"
#include "syscfg.h"
#include "cmu.h"
#include "spi.h"
#include "pmu.h"
#include "ddr.h"
#include "load.h"

void die(void)
{
	serial_puts("error\n");
	serial_exit();

	writel(0x900db00d, DMW96_ROM_BASE);
	while (1);
}

void bootastic(int bootsel)
{
	void (*loader)(void);

	/*
	 * debug: enable this to wait for jtag attach before continuing to
	 * boot. in the debugger, simply set the PC to the next instruction
	 * and then go/singlestep.
	 */
#if 0
	asm("1: b 1b");
#endif

	/* allow serial to flush remaining data */
	serial_exit();

	cmu_setup();

	//spi_init();
	//pmu_init();
	lpddr2_init();

	serial_init(115200);
	serial_puts("Bootastic " VERSION "\n");

	/* load the bootloader to its destination */
	switch (bootsel) {
		case BOOTSEL_UART:
			loader = (void *)0x41000000;
			if (romapi->xmodem_rcv(loader, 0x1000000) < 0)
				die();
			break;

		default:
			loader = load_loader();
			if (IS_ERR(loader))
				die();
	}

	serial_exit();

	/* we do not return from this call */
	loader();
}

void __entry _start(uint32_t rom_version, const struct bootrom_calls *rom_calls, int bootsel)
{
	romapi_init(rom_version, rom_calls);

	if (rom_is_legacy(rom_version))
		bootsel = BOOTSEL_NAND;

	bootastic(bootsel);
}
