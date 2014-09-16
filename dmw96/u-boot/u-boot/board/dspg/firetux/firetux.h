/*
 * firetux board specific defines
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * Boardrevisions:
 * 0: unknown PNX8181 board
 * 1: EZ_MCP_PNX8181
 * 2: Vega_PNX8181_BaseStation Platform III-a
 * 3: Vega_PNX8181_BaseStation Platform III-b
 * 4: Vega_PNX8181_BaseStation Platform III-c
 */
#define CONFIG_MAX_BOARDREVISIONS       4
enum firetux_revisions {
	Any_PNX8181_BaseStation = -1,
	UNKNOWN_PNX8181 = 0,
	EZ_MCP_PNX8181,
	VEGA_BASESTATION_IIIA,
	VEGA_BASESTATION_IIIB,
	VEGA_BASESTATION_IIIC,
};

/* watchdog control register */
#define PNX8181_WDRUCON		0xc2203000

/* digital to analog interface */
#define PNX8181_DAIF_RVDDC	0xc2000000

/* console port */
#define PNX8181_UART2_BASE	0xc2005000
#define PNX8181_UART_FDIV_CTRL	0xc00
#define PNX8181_UART_FDIV_M	0xc04
#define PNX8181_UART_FDIV_N	0xc08

/* gpio settings */
#define PNX8181_SCON_BASE	0xC2204000
#define PNX8181_REG_SC_SYSVER	(PNX8181_SCON_BASE + 0x0)
#define PNX8181_SYSMUX0		0x0C
#define PNX8181_SYSMUX1		0x10
#define PNX8181_SYSMUX2		0x14
#define PNX8181_SYSMUX3		0x18
#define PNX8181_SYSMUX4		0x1C
#define PNX8181_SYSMUX5		0x20
#define PNX8181_SYSPAD0		0x34
#define PNX8181_SYSPAD1		0x38
#define PNX8181_SYSPAD2		0x3c
#define PNX8181_SYSPAD3		0x40
#define PNX8181_SYSPAD4		0x44
#define PNX8181_SYSPAD5		0x48

#define SYSMUX_GPIOA		0x0

#define SYSPAD_PULL_UP		0x0
#define SYSPAD_REPEATER		0x1
#define SYSPAD_PLAIN_INPUT	0x2
#define SYSPAD_PULL_DOWN	0x3

#define PNX8181_GPIOA_PINS	0xc2104000
#define PNX8181_GPIOA_OUTPUT	0xc2104004
#define PNX8181_GPIOA_DIRECTION	0xc2104008

#define PNX8181_GPIOB_DIRECTION	0xc2104208

#define GPIOA_DIR_OUTPUT	1
#define GPIOA_DIR_INPUT		0

/* timing for CS0 (NAND) */
#define PNX8181_REG_EBI1_BASE	0xBC000000
#define PNX8181_REG_EBI1_CS0	(PNX8181_REG_EBI1_BASE + 0x00)
#define PNX8181_EBI_MAIN_OFF	0x00
#define PNX8181_EBI_READ_OFF	0x04
#define PNX8181_EBI_WRITE_OFF	0x08
#define PNX8181_EBI_BURST_OFF	0x0C

/* irq resetting */
#define PNX8181_INTC_REQUEST	0xC1100400

#define PNX8181_DISABLEIRQ(IRQ)	writel((0x1f << 25), \
				(void *)PNX8181_INTC_REQUEST + (4 * IRQ));

#define PNX8181_INTC_PRIOMASK_IRQ	0xC1100000
#define PNX8181_INTC_PRIOMASK_FIQ	0xC1100004

/* clock speed */
#define PNX8181_CGU_SCCON	0xC2200004
/* clock gate */
#define PNX8181_CGU_GATESC	0xC2200008
#define PNX8181_CGU_PER2CON	0xC2200018

/* priority for ETN over CPU for SDRAM access */
#define PNX8181_SDI_SCHED	0xC1200100
#define PNX8181_SCHED0		0x00
#define PNX8181_SCHED1		0x04
#define PNX8181_SCHED2		0x08

/* priority for SCRAM access at AHB */
#define PNX8181_SC_ARB_BASE	0xc200ea00
#define PNX8181_SC_ARB_CFG2	0x08
#define PNX8181_SC_ARB_CFG3	0x0c

/* clock settings */
#define PNX8181_REG_CGU_SCCON	0xC2200004

/* sdram configuration */
#define PNX8181_SDI_CFG_1	0xC1200000

/* external functions */
extern unsigned char get_gpioa(unsigned char gpionr);
extern void set_gpioa(unsigned char gpionr);
extern void clear_gpioa(unsigned char gpionr);
extern void toggle_gpioa(unsigned char gpionr);
extern void set_direction_gpioa(unsigned char gpionr, unsigned char dir);
extern void set_syspad_gpioa(unsigned char gpionr, unsigned char pad);
extern void set_sysmux_gpioa(unsigned char gpionr, unsigned char mux);
extern void config_gpioa(unsigned char gpionr, unsigned char mux,
		unsigned char dir, unsigned char pad, unsigned char value);

extern char get_boardrevision(void);
extern void board_detect(void);
