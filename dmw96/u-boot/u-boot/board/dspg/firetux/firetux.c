/*
 * firetux board specific setup
 *
 * (C) Copyright 2007-2009, emlix GmbH, Germany
 * Juergen Schoew <js@emlix.com>
 *
 * (C) Copyright 2008, DSPG Technologies GmbH, Germany
 * (C) Copyright 2007, NXP Semiconductors Germany GmbH
 * Matthias Wenzel, <nxp@mazzoo.de>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <asm/io.h>
#include <status_led.h>
#include "firetux.h"

DECLARE_GLOBAL_DATA_PTR;

/* functions for ethernet initialization */
extern int ip3912_eth_initialize(unsigned char nr, unsigned int etn_base,
	unsigned int phy_base, unsigned char phy_addr, unsigned char rmii);
extern int ip3912_miiphy_initialize(bd_t *bis);

#ifdef CONFIG_SHOW_BOOT_PROGRESS
void show_boot_progress(int progress)
{
	printf("Boot reached stage %d\n", progress);
}
#endif

void firetux_gpio_init(void)
{
	if (get_boardrevision() < 2)		/* EZ_MCP has no keys or leds */
		return;

	/* set GPIOA10 and GPIOA2 for key input */
	config_gpioa(10, SYSMUX_GPIOA, GPIOA_DIR_INPUT, SYSPAD_PULL_UP, -1);
	config_gpioa(2, SYSMUX_GPIOA, GPIOA_DIR_INPUT, SYSPAD_PULL_UP, -1);

	switch (get_boardrevision()) {
	case 2:
		/* Baseboard IIIa: set GPIO A3 A9 A16 A31 for LED output */
		config_gpioa(3, SYSMUX_GPIOA, GPIOA_DIR_OUTPUT,
							SYSPAD_PULL_UP, 1);
		config_gpioa(9, SYSMUX_GPIOA, GPIOA_DIR_OUTPUT,
							SYSPAD_PULL_UP, 1);
		config_gpioa(16, SYSMUX_GPIOA, GPIOA_DIR_OUTPUT,
							SYSPAD_PULL_UP, 1);
		config_gpioa(31, SYSMUX_GPIOA, GPIOA_DIR_OUTPUT,
							SYSPAD_PULL_UP, 1);
		break;
	case 3:
	case 4:
		/* set GPIOA9 and GPIOA0 for LED output */
		config_gpioa(9, SYSMUX_GPIOA, GPIOA_DIR_OUTPUT,
							SYSPAD_PULL_UP, 1);
		config_gpioa(0, SYSMUX_GPIOA, GPIOA_DIR_OUTPUT,
							SYSPAD_PLAIN_INPUT, 1);
		break;
	default:
		break;
	}
}

#ifdef CONFIG_CMD_NAND
void firetux_nandflash_init(void)
{
	/*
	 * Hardware configuration
	 * setup GPIOB18 / FMP40 to GPIO input with internal pullup
	 */
	writel(readl((void *)(PNX8181_SCON_BASE + PNX8181_SYSMUX3)) & ~(0x30),
				(void *)(PNX8181_SCON_BASE + PNX8181_SYSMUX3));
	/* set FPM40 to GPIOb18 */
	writel(readl((void *)(PNX8181_SCON_BASE + PNX8181_SYSMUX3)) |= 0x10,
				(void *)(PNX8181_SCON_BASE + PNX8181_SYSMUX3));
	/* set to input */
	writel(readl((void *)PNX8181_GPIOB_DIRECTION)
					& ~(1 << CONFIG_SYS_NAND_RB_BIT),
					(void *)PNX8181_GPIOB_DIRECTION);
	/* set pull up */
	writel(readl((void *)(PNX8181_SCON_BASE + PNX8181_SYSPAD2))
								& ~(3 << 20),
				(void *)(PNX8181_SCON_BASE + PNX8181_SYSPAD2));

	/* setup access timing for CS0 / 16bit */
	writel(0x0001, (void *)(PNX8181_REG_EBI1_CS0 + PNX8181_EBI_MAIN_OFF));
	writel(0x1FFFF, (void *)(PNX8181_REG_EBI1_CS0 + PNX8181_EBI_READ_OFF));
	writel(0x1FFFF, (void *)(PNX8181_REG_EBI1_CS0 + PNX8181_EBI_WRITE_OFF));
	writel(0x0CF8, (void *)(PNX8181_REG_EBI1_CS0 + PNX8181_EBI_BURST_OFF));
}
#else
void firetux_norflash_init(void)
{
#ifdef CONFIG_SYS_OPTIMIZE_NORFLASH
	/* optimize access speed */
	writel(0x9, (void *)(PNX8181_REG_EBI1_CS0 + PNX8181_EBI_MAIN_OFF));
	writel(0x22D88, (void *)(PNX8181_REG_EBI1_CS0 + PNX8181_EBI_READ_OFF));
	writel(0x22D88, (void *)(PNX8181_REG_EBI1_CS0 + PNX8181_EBI_WRITE_OFF));
	writel(0x0000, (void *)(PNX8181_REG_EBI1_CS0 + PNX8181_EBI_BURST_OFF));
#endif
}
#endif

void firetux_irq_disable(void)
{
	int i;

	writel(31, (void *)PNX8181_INTC_PRIOMASK_IRQ);
	writel(31, (void *)PNX8181_INTC_PRIOMASK_FIQ);

	for (i = 1; i < 67; i++)
		PNX8181_DISABLEIRQ(i);

	writel(readl((void *)PNX8181_CGU_GATESC) & 0xfdd7becd,
						(void *)PNX8181_CGU_GATESC);
}

/* Ethernet */
void firetux_sysmux_config_rmii(void)
{
	/* config gpio c15 and c16 for RMII */
	writel((readl((void *)(PNX8181_SCON_BASE + PNX8181_SYSMUX5))
		& ~(0x3 << 0)), (void *)(PNX8181_SCON_BASE + PNX8181_SYSMUX5));
	writel((readl((void *)(PNX8181_SCON_BASE + PNX8181_SYSMUX4))
		& ~(0x3 << 30)), (void *)(PNX8181_SCON_BASE + PNX8181_SYSMUX4));
}

void firetux_sysmux_config_etn1(void)
{
	/* config gpio c0, c1, ..., c6 for ETN1 */
	writel((readl((void *)(PNX8181_SCON_BASE + PNX8181_SYSMUX4))
		 & 0xffffc000), (void *)(PNX8181_SCON_BASE + PNX8181_SYSMUX4));
}

void firetux_sysmux_config_etn2(void)
{
	/* config gpio c7, c8, ..., c13 for ETN2 (alternative config for sd-card) */
	writel((readl((void *)(PNX8181_SCON_BASE + PNX8181_SYSMUX4))
		& 0xf0003fff), (void *)(PNX8181_SCON_BASE + PNX8181_SYSMUX4));
}

int board_eth_init(bd_t *bis)
{
	int rc = 0;
	char ethact[8];

/*
 * priority setting for ETN over CPU for SDRAM access
 * enable port-aging with priority for ETN
 */
#ifdef CONFIG_SYS_ETHER_FAIR_SCHEDULE
	/*
	 * be fair: ETN and CPU are both high priority, but ETN gets
	 * higher priority and a much lower ageing penatly to get
	 * much more accesses to RAM
	 */
	writel(0x80000121, (void *)(PNX8181_SDI_SCHED + PNX8181_SCHED0));
	writel(0xffff0100, (void *)(PNX8181_SDI_SCHED + PNX8181_SCHED1));
	writel(0x00030131, (void *)(PNX8181_SDI_SCHED + PNX8181_SCHED2));
#else
	/*
	 * highly prioritise the ETN engine and lower CPU priority
	 * to archive a better network thoughput
	 */
	writel(0xffff0020, (void *)(PNX8181_SDI_SCHED + PNX8181_SCHED0));
	writel(0xffff0000, (void *)(PNX8181_SDI_SCHED + PNX8181_SCHED1));
	writel(0xffff0031, (void *)(PNX8181_SDI_SCHED + PNX8181_SCHED2));
#endif

	/* increase priority for SCRAM access at AHB of descriptors */
	writew(7, (void *)(PNX8181_SC_ARB_BASE + PNX8181_SC_ARB_CFG2));
	writew(7, (void *)(PNX8181_SC_ARB_BASE + PNX8181_SC_ARB_CFG3));

	/*
	 * init ETN clocks
	 *
	 * set ETNREFCLK to internal CGU clock, assuming a 13.824MHz crystal
	 * for other xtals see NXP's validation tests,
	 * lib/tools/source/swift_tools.c
	 */
	writel((15 << 9) | (62 << 3) | 3, (void *)PNX8181_CGU_PER2CON);

	/* turn on PLL */
	writel(readl((void *)PNX8181_CGU_PER2CON) | 0x00010000,
						(void *)PNX8181_CGU_PER2CON);
	/* wait for PLL lock */
	while (!(readl((void *)PNX8181_CGU_PER2CON) & 0x00020000))
		;

	/* ungate ETN clocks */
	writel(readl((void *)PNX8181_CGU_PER2CON) | 0x00802000,
						(void *)PNX8181_CGU_PER2CON);

	/* register ip3912 mii interface */
	ip3912_miiphy_initialize(gd->bd);

	/* register ethernet devices */
	switch (get_boardrevision()) {
	case 1:
	case 2:
		memset(ethact, 0, 8);
		if (getenv("ethact") != NULL)
			strcpy(ethact, getenv("ethact"));
		firetux_sysmux_config_rmii();
		firetux_sysmux_config_etn1();
		firetux_sysmux_config_etn2();
		/* phys are at addr 0x1 & 0x2 at RMII phybus */
		rc |= ip3912_eth_initialize(0, CONFIG_IP3912_ETN1_BASE,
					CONFIG_IP3912_ETN1_BASE, 0x01, 1);
		rc |= ip3912_eth_initialize(1, CONFIG_IP3912_ETN2_BASE,
					CONFIG_IP3912_ETN1_BASE, 0x02, 1);
		if (ethact[0])
			setenv("ethact", ethact);
		else
			setenv("ethact", "ETN1");
		eth_set_current();
		break;
	case 3:
	case 4:
		memset(ethact, 0, 8);
		if (getenv("ethact") != NULL)
			strcpy(ethact, getenv("ethact"));
		firetux_sysmux_config_rmii();
		firetux_sysmux_config_etn1();
		firetux_sysmux_config_etn2();
		/* phys are at addr 0x1e & 0x1d at RMII phybus */
		rc |= ip3912_eth_initialize(0, CONFIG_IP3912_ETN1_BASE,
					CONFIG_IP3912_ETN1_BASE, 0x1e, 1);
		rc |= ip3912_eth_initialize(1, CONFIG_IP3912_ETN2_BASE,
					CONFIG_IP3912_ETN1_BASE, 0x1d, 1);
		if (ethact[0])
			setenv("ethact", ethact);
		else
			setenv("ethact", "ETN1");
		eth_set_current();
		break;
	default:
		return -1;
	}
	return rc;
}

void firetux_usb_init(void)
{
#define CGU_PER1CON         0xC2200014
#define PER1CON_PER1BY2     22
#define PER1CON_PLLPER1LOCK 17
#define PER1CON_PLLPER1EN   16
#define PER1CON_PER1CLKEN   13

	*(vu_long *)CGU_PER1CON &= ~(1 << PER1CON_PER1BY2); /* use mper1 */
	*(vu_long *)CGU_PER1CON |= (1 << PER1CON_PLLPER1EN);

	/* wait for pll lock after enable */
	while ((*(vu_long *)CGU_PER1CON & (1 << PER1CON_PLLPER1LOCK)) == 0);

	/* enable per1 clocks */
	*(vu_long *)CGU_PER1CON |= (1 << PER1CON_PER1CLKEN);
}

/*
 * Miscellaneous platform dependent initialisations
 */

int board_init(void)
{
	/* arch number of Firetux Platform Boards */
	gd->bd->bi_arch_number = MACH_TYPE_PNX8181;

	/* adress of boot parameters / atags */
	gd->bd->bi_boot_params = PHYS_SDRAM_1 + 0x100;

	/* release the RSTAPU and RSTEXT signal which keeps the PHYs in reset */
	writel(readw((void *)PNX8181_WDRUCON) | 0x0009,
						(void *)PNX8181_WDRUCON);
	/* lower VDDIO to Vbgp * 2.44 (typ. 3.05V) */
	writel(0x0079, (void *)PNX8181_DAIF_RVDDC);

	/* select TXD2 for GPIOa11 and select RXD2 for GPIOa1 */
	writel(((readl((void *)(PNX8181_SCON_BASE + PNX8181_SYSMUX0))
						& 0xff3ffff3) | 0x00400004),
				(void *)(PNX8181_SCON_BASE + PNX8181_SYSMUX0));
	/* select ETN for GPIOc0-16 */
	writel(readl((void *)(PNX8181_SCON_BASE + PNX8181_SYSMUX5))
								& 0xfffffffc,
				(void *)(PNX8181_SCON_BASE + PNX8181_SYSMUX5));
	writel(readl((void *)(PNX8181_SCON_BASE + PNX8181_SYSMUX4))
								& 0x30000000,
				(void *)(PNX8181_SCON_BASE + PNX8181_SYSMUX4));

	/* use clk26m and fract-n for UART2 */
	writew(((1 << 7) | (1 << 1)),
			(void *)(PNX8181_UART2_BASE + PNX8181_UART_FDIV_CTRL));
	writew(0x5F37, (void *)(PNX8181_UART2_BASE + PNX8181_UART_FDIV_M));
	writew(0x32C8, (void *)(PNX8181_UART2_BASE + PNX8181_UART_FDIV_N));

	board_detect();

	firetux_usb_init();

	firetux_gpio_init();

	icache_enable();
	dcache_enable();

#ifdef CONFIG_CMD_NAND
	firetux_nandflash_init();
#else
	firetux_norflash_init();
#endif

	firetux_irq_disable();

	return 0;
}

int firetux_keystatus(int key)
{
	int ret = 0;

	switch (key) {
	case 1:	/* GPIOA2 */
		ret = get_gpioa(2);
		break;
	case 2:	/* GPIOA10 */
		ret = get_gpioa(10);
		break;
	default:
		ret = -1;
		break;
	}

	return ret;
}

void firetux_check_bootkey(void)
{
	int i = 0, abort = 0;

	while ((abort == 0) && (i < 500)) {
		if (firetux_keystatus(2) == 1) {
			setenv("bootcmd", "run bootcmd1");
			firetux_set_led(2, 0);
			abort = 1;
		}
		i++;
		udelay(10000);
	}
	if (abort == 0) {
		setenv("bootcmd", "run bootcmd2");
		puts("using alternative bootcmd\n");
		firetux_set_led(2, 1);
	}
}

#define NR_SPEEDS 8
int supported_cpu_speeds[NR_SPEEDS] = {
	83, 90, 97, 104, 165, 180, 194, 208
};

void set_cpu_speed_internal(int armclk);
typedef void (*set_cpu_speed_cb)(int armclk);
set_cpu_speed_cb* set_cpu_speed_intram;

static void set_cpu_speed(int mhz)
{
	int i;
	int armclk;

	i = 0;
	while ((i < NR_SPEEDS) && (mhz > supported_cpu_speeds[i]))
		i++;
	armclk = supported_cpu_speeds[i];

	set_cpu_speed_intram = (set_cpu_speed_cb*)(0x0);
	memcpy(set_cpu_speed_intram, &set_cpu_speed_internal, 0x1000); 

	((set_cpu_speed_cb)(set_cpu_speed_intram))(armclk);

	print_cpuinfo();
}

int misc_init_r(void)
{
	char *cpu_string;

	if (get_boardrevision() > 1)		/* ez_mcp_pnx8181 has no keys */
		firetux_check_bootkey();

	setenv("verify", "n");

	/* CPU speed setting */
	if ((cpu_string = getenv("cpu_speed")) != NULL)
		set_cpu_speed(simple_strtol(cpu_string, NULL, 10));

	return 0;
}

#if defined(CONFIG_DISPLAY_CPUINFO)
int print_cpuinfo(void)
{
	unsigned int sysver, cgu_sccon, armclk, hclk;

	puts("CPU: ");
	sysver = readw((void *)PNX8181_REG_SC_SYSVER);
	printf("PNX8181-%d%c %s-ARM926EJ-S(ARMv5TEJ) ",
		((sysver >> 4) & 0xf), ((sysver & 0xf) + 64),
		(((sysver >> 8) & 0xf) == 1) ? "OM6xxx" : "unknown");

	cgu_sccon = readw((void *)PNX8181_CGU_SCCON);

	/* armclk = bbcki * nsc / msc / ksc */
	armclk = 13824 * (((cgu_sccon >> 3) & 0x3f) + 1)
		/ ((cgu_sccon & 0x7) + 1)
		/ (((cgu_sccon >> 10) & 0x3) + 1);
	hclk = armclk / (((cgu_sccon >> 12) & 0xf) + 1);

	printf("@ %dMHz(armclk), %dMHz(hclk)\n",
		(armclk / 1000), (hclk / 1000));
	return 0;
}
#endif

#if defined(CONFIG_DISPLAY_BOARDINFO)
int checkboard(void)
{
	puts("Board: ");

	switch (get_boardrevision()) {
	case 1:
		puts("EZ_MCP_PNX8181\n");
		break;
	case 2:
		puts("Vega_PNX8181_BaseStation Platform III-a\n");
		break;
	case 3:
		puts("Vega_PNX8181_BaseStation Platform III-b\n");
		break;
	case 4:
		puts("Vega_PNX8181_BaseStation Platform III-c\n");
		break;
	case 0:
	default:
		puts("unknown PNX8181 board\n");
		break;
	}

	return 0;
}
#endif

int dram_init(void)
{
	int reg;

	reg = readl((void *)PNX8181_SDI_CFG_1);
	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = 1 << (
		(reg & 0x3) +			/* dram width 8, 16, 32, 64 */
		(((reg & 0x30) >> 4) + 8) +	/* columns 8, 9, 10, 11 */
		(((reg & 0x700) >> 8) + 9) +	/* rows 9 .. 16 */
		((reg & 0x3000) >> 12));	/* nr banks 1, 2, 4 */

	return 0;
}

#ifdef CONFIG_USE_IRQ
int arch_interrupt_init(void)
{
	return 0;
}
#endif

