/*
 * firetux board detection
 *
 * (C) Copyright 2007-2009, emlix GmbH, Germany
 * Juergen Schoew <js@emlix.com>
 * (C) Copyright 2008, Sebastian Hess, emlix GmbH <sh@emlix.com>
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

/*
 * These VEGA boards are very similair to each other. The main difference
 * is the phy address of the network, CPU revision, number of leds and keys.
 */

#include <common.h>
#include <asm/io.h>
#include "firetux.h"

#ifndef CONFIG_IP3912_ETN1_BASE
#define CONFIG_IP3912_ETN1_BASE		0xC1600000
#endif
#ifndef CONFIG_IP3912_ETN2_BASE
#define CONFIG_IP3912_ETN2_BASE		0xC1700000
#endif

/* ip3912 / pnx8181 ETN registers */
#define ETN1_BASE			CONFIG_IP3912_ETN1_BASE
#define ETN2_BASE			CONFIG_IP3912_ETN2_BASE

#define ETN_MAC1			0x0000
#define ETN_SUPP			0x0018
#define ETN_MCFG			0x0020
#define ETN_MADR			0x0028
#define ETN_MCMD			0x0024
#define ETN_MWTD			0x002c
#define ETN_MRDD			0x0030
#define ETN_MIND			0x0034
#define ETN_COMMAND			0x0100

static char firetux_boardrevision = UNKNOWN_PNX8181;

char get_boardrevision(void)
{
	return firetux_boardrevision;
}

static void detect_etn_start(void)
{
	debug("board detect: initialize ETN\n");
	/* reset MII mgmt, set MII clock */
	writel(0x0000801c, (void *)(ETN1_BASE + ETN_MCFG));
	writel(0x0000001c, (void *)(ETN1_BASE + ETN_MCFG));

	/* enable RMMI */
	writel(0x00000600, (void *)(ETN1_BASE + ETN_COMMAND));

	/* reset MAC layer */
	writel(0x0000cf00, (void *)(ETN1_BASE + ETN_MAC1));
	/* release MAC soft reset */
	writel(0x00000000, (void *)(ETN1_BASE + ETN_MAC1));
	/* reset rx-path, tx-path, host registers */
	writel(0x00000638, (void *)(ETN1_BASE + ETN_COMMAND));
	/* reset RMII, 100Mbps MAC, 10Mbps MAC */
	writel(0x1888, (void *)(ETN1_BASE + ETN_SUPP));
	writel(0x1000, (void *)(ETN1_BASE + ETN_SUPP));
}

/* Wait for the ETN engine to be ready */
static void detect_phy_wait(void)
{
	int i, status;
	for (i = 0; i < 1000; i++) {
		status = readl((void *)(ETN1_BASE + ETN_MIND)) & 0x7;
		if (!status)
			return;
		udelay(1);
	}
	debug("board detect: wait PHY timed out!\n");
}

/* write to phy */
static void detect_phy_write(u8 address, u8 reg, u16 data)
{
	writel((address << 8) | reg, (void *)(ETN1_BASE + ETN_MADR));
	writel(data, (ETN1_BASE + ETN_MWTD));
	detect_phy_wait();
}

/* read from phy */
static u16 detect_phy_read(u8 address, u8 reg)
{
	u16 value;
	writel((address << 8) | reg, (void *)(ETN1_BASE + ETN_MADR));
	writel(0x00000001, (void *)(ETN1_BASE + ETN_MCMD));
	detect_phy_wait();

	value = readl((void *)(ETN1_BASE + ETN_MRDD));
	writel(0x00000000, (void *)(ETN1_BASE + ETN_MCMD));
	return value;
}

/* do a software reset on the phy */
static int detect_phy_softreset(u8 phyaddr)
{
	int i, rc;

	debug("board detect: softreset PHY @0x%02x ", phyaddr);

	/*
	 *  reset power down bit and set autonegotiation in
	 *  phy basic control register 0
	 */
	detect_phy_write(phyaddr, 0x00, 0x1200);

	/* set reset bit in phy basic control register */
	detect_phy_write(phyaddr, 0x00, detect_phy_read(phyaddr, 0x00)
								| (1 << 15));
	udelay(260); /* HW ref (pg. 51): 256us, IEEE <= 500ms */

	/* Check for bit reset */
	for (i = 0; i < 1000; i++) {
		rc = detect_phy_read(phyaddr, 0x00);
		if (!(rc & (1 < 11))) {
			debug("OK\n");
			return 0;
		}
	}

	debug("timed out!\n");
	return -1;
}

/* pull the hardware reset line of the phy */
static int detect_phy_hardreset(int gpio)
{
	/* strobe the reset pin */
	debug("board detect: hardreset PHY with GPIO %d\n", gpio);
	clear_gpioa(gpio);
	udelay(120);		/* min 100us */
	set_gpioa(gpio);
	udelay(1);		/* min 800ns */

	return 0;
}

/* write the phy address to the special function register */
static int detect_phy_init(int phyaddr)
{
	int i;
	u16 status, check_status = 0x40e0 | (phyaddr & 0x1f);

	debug("board detect: PHY @0x%02x init ", phyaddr);

	for (i = 0; i < 1000; i++) {
		/* Set the special function register (Reg 18) */
		detect_phy_write(phyaddr, 0x12, check_status);

		/* Read back the Register */
		status = detect_phy_read(phyaddr, 0x12);
		if ((status & 0x40ff) == check_status) {
			debug("powerdown disabled\n");
			return detect_phy_softreset(phyaddr);
		}
		udelay(10);
	}
	debug("disable powerdown failed\n");
	return -1;
}

static int detect_eth_check_vega_phy(void)
{
	int status, scon_sysver, cpuversion;

	detect_etn_start();

	/* Check for the CPU revision */
	scon_sysver = readw(PNX8181_REG_SC_SYSVER);

	if (scon_sysver == 0x4111) {
		cpuversion = 1;		/* a revision 1 CPU */
		debug("board detect: PNX8181 V1 CPU found\n");
	} else if ((scon_sysver == 0x4121) || (scon_sysver == 0x4122)) {
		cpuversion = 2;		/* a revision 2 CPU */
		debug("board detect: PNX8181 V2 CPU found\n");

		/* a V2 cpu can only be on IIIb or IIIc boards */
		goto cpu_v2;
	} else {
		cpuversion = 0;
		debug("board detect: unknown PNX8181 CPU revision\n");

		return UNKNOWN_PNX8181;
	}

cpu_v1:
	/* phy reset for IIIa and IIIb is on GPIO A12 */
	config_gpioa(12, SYSMUX_GPIOA, GPIOA_DIR_OUTPUT, SYSPAD_PULL_DOWN, 1);
	detect_phy_hardreset(12);

	/* If both phy 1 and 2 are ok: III-a Board*/
	status = detect_phy_init(0x01);
	if (!status && !detect_phy_init(0x02))
		return VEGA_BASESTATION_IIIA;

	/*
	 *  A V1 cpu can only be on EZ-MCP or IIIa boards, but this board uses
	 *  an unknown phys configuration, therefore it is an unknown board
	 */
	debug("board detect: unknown board with V1 CPU\n");
	return UNKNOWN_PNX8181;

cpu_v2:
	/* set gpioA3 to 0 to hold possible IIIc phy in reset */
	config_gpioa(3, SYSMUX_GPIOA, GPIOA_DIR_OUTPUT, SYSPAD_PULL_DOWN, 0);

	/* Phy 29 and 30  are ok: III-b Board */
	config_gpioa(12, SYSMUX_GPIOA, GPIOA_DIR_OUTPUT, SYSPAD_PULL_DOWN, 1);
	detect_phy_hardreset(12);
	debug("board detect: checking for VEGA Basestation IIIb\n");
	status = detect_phy_init(0x1d);
	if (!status && !detect_phy_init(0x1e))
		return VEGA_BASESTATION_IIIB;

	/* for IIIC we need to set some pull-up / pull-down */
	writel((readl((void *)(PNX8181_SCON_BASE + PNX8181_SYSPAD4))
					& 0xf7f5755f) | 0x080a8aa0,
				 (void *)PNX8181_SCON_BASE + PNX8181_SYSPAD4);

	/* phy reset for IIIc is on GPIO A3, hold GPIO A12 low */
	clear_gpioa(12);
	detect_phy_hardreset(3);

	/* Phy 29 and 30  are ok: III-c Board */
	debug("board detect: checking for VEGA Basestation IIIc\n");
	status = detect_phy_init(0x1d);
	if (!status && !detect_phy_init(0x1e))
		return VEGA_BASESTATION_IIIC;

	debug("board detect: unknown board with V2 CPU\n");
	return UNKNOWN_PNX8181;
}

void board_detect(void)
{
	int cgu_sccon;

	debug("board detection ");
	/* simple board detection */
	cgu_sccon = readw((void *)PNX8181_REG_CGU_SCCON);
	cgu_sccon = ((cgu_sccon >> 3) & 0x3f);

	/* The easy case, only the EZ_MCP ist clocked slow */
	if (cgu_sccon == 11) {
		/* ez_mcp_pnx8181 */
		firetux_boardrevision = EZ_MCP_PNX8181;
		debug("found EZ_MCP_PNX8181\n");
		/* just be sure to set the phy to a known state */
		config_gpioa(12, SYSMUX_GPIOA, GPIOA_DIR_OUTPUT,
						SYSPAD_PULL_DOWN, 1);
		detect_etn_start();
		detect_phy_hardreset(12);
		detect_phy_init(0x01);
		detect_phy_init(0x02);
		return;
	}

	if (cgu_sccon == 14) {
		/*
		 * vega_pnx8181_basestation Platform III a, b, c
		 * we need futher inevestigation by network test
		 */
		firetux_boardrevision = VEGA_BASESTATION_IIIA;
		debug("clock 208MHz, need further investigation\n");
	}

	/* ... check the phy to differ between V1 and V2 Boards */
	if (firetux_boardrevision == VEGA_BASESTATION_IIIA)
		firetux_boardrevision = detect_eth_check_vega_phy();
}
