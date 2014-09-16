/*
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * (C) Copyright 2002
 * David Mueller, ELSOFT AG, <d.mueller@elsoft.ch>
 *
 * (C) Copyright 2003
 * Texas Instruments, <www.ti.com>
 * Kshitij Gupta <Kshitij@ti.com>
 *
 * (C) Copyright 2004
 * ARM Ltd.
 * Philippe Robin, <philippe.robin@arm.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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

#include <common.h>
#include <netdev.h>
#include <asm/io.h>
#include "broadtile.h"

#define IO_ADDRESS(x) (x)
#define ASYNC_MODE    0x00E20000
#define DW_SDRAM_SIZE (8 << 20)

/* from common/hush.c */
char *simple_itoa(unsigned int i);

DECLARE_GLOBAL_DATA_PTR;

void broadtile_fpga_init(void);
void broadtile_mpmc_init(int mhz, int bus_width);

static inline void delay (unsigned long loops)
{
	__asm__ volatile ("1:\n"
		"subs %0, %1, #1\n"
		"bne 1b":"=r" (loops):"0" (loops));
}

#if defined(CONFIG_SHOW_BOOT_PROGRESS)
void show_boot_progress(int progress)
{
    printf("Boot reached stage %d\n", progress);
}
#endif

/*
 * Miscellaneous platform dependent initialisations
 */

int board_early_init_f (void)
{
	/*
	 * set clock frequency:
	 *	VERSATILE_REFCLK is 32KHz
	 *	VERSATILE_TIMCLK is 1MHz
	 */
	*(volatile unsigned int *)(VERSATILE_SCTL_BASE) |=
	  ((VERSATILE_TIMCLK << VERSATILE_TIMER1_EnSel) | (VERSATILE_TIMCLK << VERSATILE_TIMER2_EnSel) |
	   (VERSATILE_TIMCLK << VERSATILE_TIMER3_EnSel) | (VERSATILE_TIMCLK << VERSATILE_TIMER4_EnSel));

	return 0;
}

int board_init (void)
{
	uint32_t reg;

	/* arch number of Versatile Board */
	gd->bd->bi_arch_number = MACH_TYPE_VERSATILE_PB;

	/* adress of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM_1 + 0x00000100;

	/*
	 * when we come here, u-boot has relocated us to the end of the sdram.
	 * if the NOR flash is still mapped to 0x0 (because we booted from it),
	 * remove that mapping now so we can access the full sdram.
	 */
	reg = readl(VERSATILE_SCTL_BASE);
	if ((reg & 0x100) == 0) {
		reg |= 0x100;
		writel(reg, VERSATILE_SCTL_BASE);
	}

	icache_enable();

	broadtile_fpga_init();

	return 0;
}

#define DW_TIMER_CLOCK		80000000
#define DW_TIMER_PRESCALE	16
#define DW_TIMER_CONTROL	(0x80 | (1 << 2) | (1 << 1))
#define DW_TIMER_RELOAD		0xffffffff

int misc_init_r (void)
{
	setenv("verify", "n");

	/* If BroaTile is supposed to be connected, do some checks (we determine this according to ASYC mode) */
	if (readl(IO_ADDRESS(SYS_CFGDATA1)) == ASYNC_MODE)
	{
		ulong tmr_control;
		ulong dw_ticks;
		/* We assume BroadTile is connected to board and we're already in async mode */
		/* Make sure BT is connected by reading its timer control reg */
		printf("Probing timer to validate BroadTile is connected..\n");
		tmr_control = *(volatile ulong *)(DW_TIMER0_1_BASE+8);
		if (tmr_control != 0x20 && tmr_control != DW_TIMER_CONTROL)
		{
			printf("Failed reading DW timer, BroadTile board might not be functioning properly\n");
		}
		else
		{
			/* BT is connected, try to find out its timer frequency */
			/* Configure the DW timer */
			*(volatile ulong *)(DW_TIMER0_1_BASE + 0) = DW_TIMER_RELOAD; /* TimerLoad */
			*(volatile ulong *)(DW_TIMER0_1_BASE + 4) = DW_TIMER_RELOAD; /* TimerValue */
			*(volatile ulong *)(DW_TIMER0_1_BASE + 8) = DW_TIMER_CONTROL;

#define SAMPLE_TIME_US 500000
			/* Get current of DW timer */
			dw_ticks = *(volatile ulong *)(DW_TIMER0_1_BASE + 4);
			/* Wait 0.5 seconds */
			udelay(SAMPLE_TIME_US);
			/* See how many ticks passed */
			dw_ticks = (dw_ticks - (*(volatile ulong *)(DW_TIMER0_1_BASE + 4))) * DW_TIMER_PRESCALE;
			/* Print result */
			printf("DW timer frequency approx %d.%dMHz\n", dw_ticks / SAMPLE_TIME_US, (((dw_ticks*(1000000/SAMPLE_TIME_US))) % 1000000) / 100000 );
#undef SAMPLE_TIME_US
		}
	}

	return (0);
}

/******************************
 Routine:
 Description:
******************************/
int dram_init (void)
{
	/* initialize broadtile sdram */
	broadtile_mpmc_init(-1, -1);

	/* dram_init must store complete ramsize in gd->ram_size */
	gd->ram_size = PHYS_SDRAM_1_SIZE;

	return 0;
}

struct nand_chip;

int board_nand_init(struct nand_chip *nand)
{
	return 0;
}

void broadtile_fpga_init()
{
	if (readl(SYS_CFGDATA2) & (1 << 22)) {
		if (readl(SYS_CFGDATA1) != ASYNC_MODE) {
			/* unlock system registers */
			writel(0xa05f, SYS_LOCK);
			delay(500000);

			/* set to async */
			writel(ASYNC_MODE, SYS_CFGDATA1);
			delay(500000);

			/* perform soft reset */
			writel(0x2, SYS_RESETCTL);
			delay(500000);

			writel(0x102, SYS_RESETCTL);

			/* chip is reset at this point, system reboots */
		}
	}
}

#define mdelay(x) udelay((x)*1000)
#define mpmc_readl(reg) readl(regs + (reg))
#define mpmc_writel(reg, val) writel((val), regs + (reg))

/* bus_width: desired bus width (16/32) or -1 to read from env / use default. */
/* mhz: may be 35, 56, 80 or -1 to read from env / use default */
void broadtile_mpmc_init(int mhz, int bus_width)
{
	volatile int temp;
	char *mpmc_bus_width, *fpga_mhz;
	unsigned int *ptr;
	void *regs = 0x89100000;
	unsigned int i;

	/* Negative value for mhz means read value stored in env or use default */
	if (mhz < 0) {
		fpga_mhz = getenv("fpga_mhz");
		if (!fpga_mhz) {
			printf("No fpga_mhz stored in environment, MPMC configuration aborted\n");
			return;
		}

		mhz = simple_strtoul(fpga_mhz, NULL, 10);
		if ((mhz != 35) && (mhz != 56) && (mhz != 80)) {
			puts("Invalid fpga_mhz stored in environment, MPMC configuration aborted\n");
			return;
		}

		printf("fpga_mhz of %d read from environment\n", mhz);
	}

	/* Set environment variable according to specification */
	if (gd->flags & GD_FLG_ENV_READY) {
		if (mhz == 0) {
			setenv("fpga_mhz", NULL);
			return;
		} else {
			setenv("fpga_mhz", simple_itoa(mhz));
		}
	}

	if (mhz == 35) {
		/* initialize sdram */
		mpmc_writel(0x028, 0x0012);
		mpmc_writel(0x024, 0x0009); /* refresh */
		mpmc_writel(0x030, 0x0001);
		mpmc_writel(0x034, 0x0004);
		mpmc_writel(0x038, 0x0007);
		mpmc_writel(0x040, 0x0004);
		mpmc_writel(0x044, 0x0001);
		mpmc_writel(0x048, 0x0006);
		mpmc_writel(0x04C, 0x0006);
		mpmc_writel(0x050, 0x0007);
		mpmc_writel(0x054, 0x0001);
		mpmc_writel(0x058, 0x0002);
		mpmc_writel(0x124, 0x0101); /* Cas and Ras latency */

		/* enable write buffer at ARM port (port 0) */
		mpmc_writel(0x408, mpmc_readl(0x408) | 1);

		/* enable write buffer at ARM port (port 4) */
		mpmc_writel(0x488, mpmc_readl(0x488) | 1);

		mpmc_writel(0x020, 0x0003); /* issue normal operation command */
		mpmc_writel(0x020, 0x0183); /* issue normal operation command */
		mpmc_writel(0x020, 0x0103); /* issue normal operation command */

		mpmc_writel(0x024, 0x0010); /* refresh */

		mdelay(30);

#ifdef CONFIG_BROADTILE_HIGH_BANK_128M
		mpmc_writel(0x120, 0x5882); /* 128Mbyte - Config */ 
#else
		mpmc_writel(0x120, 0x4500); /* 32Mbyte - Config  */ 
#endif									
		mpmc_writel(0x020, 0x0083); /* issue normal operation command */ 

		mdelay(30);

		temp = *(volatile int *)(0xe0010800);

		mdelay(30);

		mpmc_writel(0x020, 0x0103); /* issue normal operation command */
		mpmc_writel(0x024, 0x0010); /* refresh */

		mdelay(30);

		mpmc_writel(0x020, 0x0003); /* issue normal operation command */
		mpmc_writel(0x480, 0x0001);

		printf("MPMC for 35MHz\n");
	} else if (mhz == 56) {
		/* initialize sdram */
		mpmc_writel(0x028, 0x0012);
		mpmc_writel(0x024, 0x000d); /* refresh */
		mpmc_writel(0x030, 0x0002);
		mpmc_writel(0x034, 0x0006);
		mpmc_writel(0x038, 0x000a);
		mpmc_writel(0x040, 0x0004);
		mpmc_writel(0x044, 0x0001);
		mpmc_writel(0x048, 0x0006);
		mpmc_writel(0x04C, 0x0006);
		mpmc_writel(0x050, 0x0007);
		mpmc_writel(0x054, 0x0001);
		mpmc_writel(0x058, 0x0002);
		mpmc_writel(0x124, 0x0202); /* Cas and Ras latency */

		mpmc_writel(0x420, mpmc_readl(0x420) | 1);

		mpmc_writel(0x020, 0x0003); /* issue normal operation command */
		mpmc_writel(0x020, 0x0183); /* issue normal operation command */
		mpmc_writel(0x020, 0x0103); /* issue normal operation command */

		mpmc_writel(0x024, 0x000d); /* refresh */

		mdelay(30);

		mpmc_writel(0x120, 0x4500); /* 32bit - Config - (address map) */
		mpmc_writel(0x020, 0x0083); /* issue normal operation command */

		mdelay(30);

		temp = *(volatile int *)(0xe0010800);

		mdelay(30);

		mpmc_writel(0x020, 0x0103); /* issue normal operation command */
		mpmc_writel(0x024, 0x000d); /* refresh */

		mdelay(30);

		mpmc_writel(0x020, 0x0003); /* issue normal operation command */
		mpmc_writel(0x480, 0x0001);

		printf("MPMC for 56MHz\n");
	} else if (mhz == 80) {
		/* Negative value for bus width means read value stored in env or use default */
		if (bus_width < 0) {
			mpmc_bus_width = getenv("mpmc_bus_width");
			if (!mpmc_bus_width) {
				printf("No mpmc_bus_width stored in environment, MPMC configuration aborted (use 'mpmcconf' command to fix this)\n");
				return;
			}

			bus_width = simple_strtoul(mpmc_bus_width, NULL, 10);
			if (bus_width != 32 && bus_width != 16) {
				puts("Invalid mpmc_bus_width stored in environment, MPMC configuration aborted (use 'mpmcconf' command to fix this)\n");
				return;
			}

			printf("mpmc_bus_width of %d read from environment\n",
			       bus_width);
		}

		/* Set environment variable according to specification */
		if (bus_width == 0) {
			setenv("mpmc_bus_width", NULL);
			return;
		} else {
			setenv("mpmc_bus_width", simple_itoa(bus_width));
		}

		mpmc_writel(0x028, 0x0012);
		mpmc_writel(0x020, 0x0003); /* issue normal operation cmd */
		mpmc_writel(0x020, 0x0183); /* issue normal operation cmd */
		mpmc_writel(0x020, 0x0103); /* issue normal operation cmd */
		mpmc_writel(0x024, 0x0002); /* refresh */

		mdelay(30);

		mpmc_writel(0x124, 0x0202); /* Cas and Ras latency */
		if (bus_width == 16)
			mpmc_writel(0x120, 0x0280); /* 16bit */
		else
			mpmc_writel(0x120, 0x4500); /* 32bit */

		mpmc_writel(0x020, 0x0083); /* issue normal operation cmd */

		mdelay(30);

		temp = *(volatile int *)(0xe0010800);

		mdelay(30);

		writel(0x020, 0x0103); /* issue normal operation cmd */
		writel(0x024, 0x0020); /* refresh */

		mdelay(30);

		writel(0x020, 0x0003); /* issue normal operation cmd */
		writel(0x480, 0x0001);

		mdelay(30);

		printf("MPMC for 80MHz\n");
	} else {
		printf("Unsupported FPGA speed setting\n");
	}

	// clear SDRAM
	ptr = (unsigned int *)0xE0000000;
	for (i = 0; i < DW_SDRAM_SIZE/4; i++)
		*ptr++ = 0x0DED0DED;
}

int do_mpmcconf(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int bus_width;

	if (argc < 2) {
		cmd_usage(cmdtp);
		return 1;
	}

	bus_width = simple_strtoul(argv[1], NULL, 10);

	switch (bus_width) {
	case 32:
	case 16:
	case 0:
		broadtile_mpmc_init(80, bus_width);
		return 0;
	default:
		cmd_usage(cmdtp);
		return 1;
	}
}

U_BOOT_CMD( /* @@ this command should be removed in the future (cause mpmc will always be 32bit and will be configured on u-boot init) */
	mpmcconf,	2,	0,	do_mpmcconf,
	"mpmcconf\t- Initialize the DW's MPMC to work in 16 or 32 bit mode\n",
	"16/32/0 (use 0 to disable MPMC configuration)\n"
);

int do_show_rev(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	u32 * rev_reg = (u32*)SYS_ID;
	u32 info = *rev_reg;

	/* Versatile specific
	 * perhaps we can generalize
	 *	[31:24] Read MAN , manufacturer 		: 0x41 = ARM
	 *	[23:16] Read ARCH, architecture		: 0x00 = ARM926
	 *	[15:12] Read FPGA type			: 0x7 = XC2V2000
	 *	[11:04] Read Build value (ARM internal use)
	 *	[03:00] Read REV , Major revision 	: 0x4 = multilayer AHB
	 */
	 printf("===================================================\n");
	 printf("Versatile board info register indicates:-          \n");
	 printf("Manufacturer   0x%08x (0x00000041 == ARM)   \n"		, ((info & 0xFF000000)/0x1000000));
	 printf("Architecture   0x%08x (0x00000000 == ARM926)\n"		, ((info & 0x00FF0000)/0x10000));
#ifdef CONFIG_ARCH_VERSATILE_AB	/* Versatile AB */
	 printf("FPGA type      0x%04x     (0x0008 == XC2S300E)\n"     		, ((info & 0x0000F000)/0x1000));
#else
	 printf("FPGA type      0x%04x     (0x0007 == XC2V2000)\n"     		, ((info & 0x0000F000)/0x1000));
#endif
	 printf("Build number   0x%08x\n"					, ((info & 0x00000FF0)/0x10));
	 printf("Major Revision 0x%04x     (0x0004 == Multi-layer AHB) \n"	,  (info & 0x0000000F));
	 printf("===================================================\n");

	return 0;
}

U_BOOT_CMD(
	show_rev,	1,	0,	do_show_rev,
	"show_rev\t- Parse the board revision info\n",
	"\n"
);

#ifdef CONFIG_CMD_NET
int board_eth_init(bd_t *bis)
{
	int rc = 0;

#ifdef CONFIG_MACG
	/* get PHY out of reset */
	writel(0x80000000, (void*)(0x85000000));

	rc = gmac_eth_initialize(0, (void *)0x86100000, 0);
#endif
#ifdef CONFIG_SMC91111
	rc = smc91111_initialize(0, CONFIG_SMC91111_BASE);
#endif

	return rc;
}
#endif
