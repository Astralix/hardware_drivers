/*
 * firetux board specific setup
 *
 * (C) Copyright 2007-2009, emlix GmbH, Germany
 * Juergen Schoew, <js@emlix.com>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <config.h>
#include <version.h>

#define RESET_ADDRESS	0xffff0000

.global reset_cpu
reset_cpu:
	ldr	lr, =RESET_ADDRESS
	mov	pc, lr

/* address/register map */

#define RAM_BASE	0x90000000

/* EBI */
#define REG_EBI1_BASE	0xBC000000

#define REG_EBI1_CS0_V1	(REG_EBI1_BASE + 0x00)
#define REG_EBI1_CS1_V1	(REG_EBI1_BASE + 0x10)
#define REG_EBI1_CS2_V1	(REG_EBI1_BASE + 0x20)
#define REG_EBI1_CS3_V1	(REG_EBI1_BASE + 0x30)
#define REG_EBI1_CS4_V1	(REG_EBI1_BASE + 0x40)
#define REG_EBI1_CS5_V1	(REG_EBI1_BASE + 0x50)
#define REG_EBI1_IPID_V1	(REG_EBI1_BASE + 0xF0)

#define REG_EBI1_CS0	(REG_EBI1_BASE + 0x00)
#define REG_EBI1_CS1	(REG_EBI1_BASE + 0x20)
#define REG_EBI1_CS2	(REG_EBI1_BASE + 0x40)
#define REG_EBI1_CS3	(REG_EBI1_BASE + 0x60)
#define REG_EBI1_CS4	(REG_EBI1_BASE + 0x80)
#define REG_EBI1_CS5	(REG_EBI1_BASE + 0xA0)
#define REG_EBI1_IPID	(REG_EBI1_BASE + 0xFFC)

/* read-only values identifying silicon version */
#define REG_SCON_SYSVER	0xC2204000
#define VAL_SYSVER_1	0x4111
#define VAL_SYSVER_2A	0x4121
#define VAL_SYSVER_2B	0x4122		    /* 2B is a metalfix version */

#define VAL_EBI1_IPID_V1	0xB0503101
#define VAL_EBI1_IPID_V2	0x20165100  /* value of datasheet */
#define VAL_EBI1_IPID_V2a	0x20165000  /* value of hardware */

#define REG_EBI2_BASE	0xC0380000

#define REG_EBI2_CS0_V1	(REG_EBI2_BASE + 0x00)
#define REG_EBI2_CS1_V1	(REG_EBI2_BASE + 0x10)
#define REG_EBI2_CS2_V1	(REG_EBI2_BASE + 0x20)
#define REG_EBI2_CS3_V1	(REG_EBI2_BASE + 0x30)
#define REG_EBI2_CS4_V1	(REG_EBI2_BASE + 0x40)
#define REG_EBI2_CS5_V1	(REG_EBI2_BASE + 0x50)
#define REG_EBI2_IPID_V1	(REG_EBI2_BASE + 0xF0)

#define REG_EBI2_CS0	(REG_EBI2_BASE + 0x00)
#define REG_EBI2_CS1	(REG_EBI2_BASE + 0x20)
#define REG_EBI2_CS2	(REG_EBI2_BASE + 0x40)
#define REG_EBI2_CS3	(REG_EBI2_BASE + 0x60)
#define REG_EBI2_CS4	(REG_EBI2_BASE + 0x80)
#define REG_EBI2_CS5	(REG_EBI2_BASE + 0xA0)
#define REG_EBI2_IPID	(REG_EBI2_BASE + 0xFFC)

#define EBI_MAIN_OFF	0x00
#define EBI_READ_OFF	0x04
#define EBI_WRITE_OFF	0x08
#define EBI_BURST_OFF	0x0C

/* CGU */
/* Register definitions */
#define REG_CGU_BASE	0xC2200000
#define REG_CGU_DSPCON	0xC2200000
#define REG_CGU_SCCON	0xC2200004
#define REG_CGU_GATESC	0xC2200008
#define REG_CGU_SLEEPSC	0xC220000C
#define REG_CGU_FDIV	0xC2200010
#define REG_CGU_PER1CON	0xC2200014
#define REG_CGU_PER2CON	0xC2200018
#define REG_CGU_PER2BWCON	0xC220001C
#define REG_CGU_FIXCON	0xC2200020
#define REG_CGU_DIVCON	0xC2200024


#ifdef CONFIG_FIRETUX_HAS_CGU_SCCON_MSC_BUG
/* on some early PNX8181 V1 CPUs a sporadic bug on the silicon doesn't allow
 * writing values > 3 to the msc field of SCCON, still the reset value is 5
 */
# define SCCON		0x0011005b
#else
# define SCCON		0x0011005d
#endif
/* default reset value */
#define PER1CON		0x00601be2
#define PER2CON		0x01801ff3
#define PER2BWCON	0x0000038d
#define FIXCON		0x000f07cf
#define DIVCON		0x102335e8
/* clock gate:
 * disabled:	ebi2 fir usb fci usim pwm[123] kbs uart1 i2s ipint spi[12]
 *  enabled:	ebi1 sdi adpcm etn[12] daif dmau drt gpio bmp uart2 i2c
 *		sctu extint intc
 */
#define GATESC		0x7ef803a7
#define SLEEPSC		0x00000000
#define FDIV		0x25ed097c


/* Set up the platform, once the cpu has been initialized */
.globl lowlevel_init
lowlevel_init:
	/* first make sure interrupts are disabled */
	mrs	r0, cpsr
	orr	r0, r0, #128
	msr	cpsr_c, r0

/* with nandflash we have to skip the lowlevelinit */
/* all setup is done with preloader of nandboot */
#if !defined(CONFIG_NANDFLASH) && !defined(CONFIG_SKIP_RELOCATE_UBOOT)

	/* currently we run from flash, via EBI1/CS0 */

	/* accesses to addresses not decoded result in hang  */
	/* check for V2 CPU */
check_for_v1_core:
	ldr	r2, =REG_SCON_SYSVER
	ldr	r0, [r2]
	ldr	r1, =VAL_SYSVER_1
	cmp	r0, r1
	bne	check_for_v2_core	/* if not v1 core check for v2 */

	/* validate EBI1 silicon version */
	ldr	r2, =REG_EBI1_IPID_V1
	ldr	r0, [r2]
	ldr	r1, =VAL_EBI1_IPID_V1
	cmp	r0, r1
	beq	pnx8181_v1_init		/* v1 core has only one ebi version */
	b	reset_cpu

check_for_v2_core:
	ldr	r2, =REG_SCON_SYSVER
	ldr	r0, [r2]
	ldr	r1, =VAL_SYSVER_2A
	cmp	r0, r1
	beq	is_v2_core

	ldr	r1, =VAL_SYSVER_2B
	cmp	r0, r1
	beq	is_v2_core
	b	reset_cpu		/* if not v2 core jump to reset */

is_v2_core:
	ldr	r2, =REG_EBI1_IPID
	ldr	r0, [r2]
	ldr	r1, =VAL_EBI1_IPID_V2
	cmp	r0, r1
	beq	pnx8181_v2_init		/* v2 core has only one ebi version */
	ldr	r1, =VAL_EBI1_IPID_V2a
	cmp	r0, r1
	beq	pnx8181_v2_init		/* v2 core has only one ebi version */
	b	reset_cpu

pnx8181_v1_init:
	/* configure flash (CS0) for minimum wait states */
	ldr	r2, =(REG_EBI1_CS0_V1 + EBI_MAIN_OFF)
	ldr	r1, =0x0001
	str	r1, [r2]
	ldr	r2, =(REG_EBI1_CS0_V1 + EBI_READ_OFF)
	ldr	r1, =0x22D8C
	str	r1, [r2]
	ldr	r2, =(REG_EBI1_CS0_V1 + EBI_WRITE_OFF)
	ldr	r1, =0x22D8C
	str	r1, [r2]
	ldr	r2, =(REG_EBI1_CS0_V1 + EBI_BURST_OFF)
	ldr	r1, =0x05b5
	str	r1, [r2]

	/* configure pSRAM (CS1) for minimum wait states */
	ldr	r2, =(REG_EBI1_CS1_V1 + EBI_MAIN_OFF)
	ldr	r1, =0x0001
	str	r1, [r2]
	ldr	r2, =(REG_EBI1_CS1_V1 + EBI_READ_OFF)
	ldr	r1, =0x22D88
	str	r1, [r2]
	ldr	r2, =(REG_EBI1_CS1_V1 + EBI_WRITE_OFF)
	ldr	r1, =0x22D88
	str	r1, [r2]
	ldr	r2, =(REG_EBI1_CS1_V1 + EBI_BURST_OFF)
	ldr	r1, =0x0000
	str	r1, [r2]
	b	ez_mcp_memtest

pnx8181_v2_init:
	/* configure flash (CS0) for minimum wait states */
	ldr	r2, =(REG_EBI1_CS0 + EBI_MAIN_OFF)
	ldr	r1, =0x0001
	str	r1, [r2]
	ldr	r2, =(REG_EBI1_CS0 + EBI_READ_OFF)
	ldr	r1, =0x22D8C
	str	r1, [r2]
	ldr	r2, =(REG_EBI1_CS0 + EBI_WRITE_OFF)
	ldr	r1, =0x22D8C
	str	r1, [r2]
	ldr	r2, =(REG_EBI1_CS0 + EBI_BURST_OFF)
	ldr	r1, =0x05b5
	str	r1, [r2]

	/* configure pSRAM (CS1) for minimum wait states */
	ldr	r2, =(REG_EBI1_CS1 + EBI_MAIN_OFF)
	ldr	r1, =0x0001
	str	r1, [r2]
	ldr	r2, =(REG_EBI1_CS1 + EBI_READ_OFF)
	ldr	r1, =0x22D88
	str	r1, [r2]
	ldr	r2, =(REG_EBI1_CS1 + EBI_WRITE_OFF)
	ldr	r1, =0x22D88
	str	r1, [r2]
	ldr	r2, =(REG_EBI1_CS1 + EBI_BURST_OFF)
	ldr	r1, =0x0000
	str	r1, [r2]

	/* try to determine if we are on a EZ_MCP_PNX8181 */
	/* we assume that all EZ boards have a PSRAM @0x90000000 */
ez_mcp_memtest:
	mov	r1, #100
	mov	r2, #0x90000000
1:
	str	r2, [r2]
	add	r2, r2, #4
	sub	r1, r1, #1
	cmp	r1, #0
	bgt	1b

	mov	r1, #100
	mov	r2, #0x90000000
1:
	ldr	r0, [r2]
	cmp	r0, r2
	bne	vega_detected
	add	r2, r2, #4
	sub	r1, r1, #1
	cmp	r1, #0
	bgt	1b
	b	ez_mcp_detected

vega_detected:
	mov	r0, #1
	b	1f

ez_mcp_detected:
	mov	r0, #0
	b	1f
	nop
	nop

1:
	/* setup Clock Generation Unit CGU */
	/* setting default values */
	ldr	r2, =REG_CGU_FDIV
	ldr	r1, =FDIV
	str	r1, [r2]
	ldr	r2, =REG_CGU_FIXCON
	ldr	r1, =FIXCON
	str	r1, [r2]
	ldr	r2, =REG_CGU_DIVCON
	ldr	r1, =DIVCON
	str	r1, [r2]
	ldr	r2, =REG_CGU_PER1CON
	ldr	r1, =PER1CON
	str	r1, [r2]
	ldr	r2, =REG_CGU_GATESC
	ldr	r1, =GATESC
	str	r1, [r2]
	ldr	r2, =REG_CGU_SLEEPSC
	ldr	r1, =SLEEPSC
	str	r1, [r2]

	/* in case we already run on PLL_SC (e.g. when we don't start from
	 * a power cycle) we need to enable and switch back to PLL_FIX */
	ldr	r2, =REG_CGU_SCCON
	ldr	r1, [r2]		/* read sccon */
	ands	r1, r1, #(1<<16)	/* mask scby */
	bne	sccon_reset

switch_to_pll_fix:
	ldr	r1, [r2]		/* read sccon */
	orr	r1, r1, #(1<<16)	/* switch to pll_fix */
	str	r1, [r2]

sccon_reset:
	/* reset value, see CONFIG_FIRETUX_HAS_CGU_SCCON_MSC_BUG */
	ldr	r1, =SCCON
	str	r1, [r2]


	/* check for vega or ez_mcp */
	cmp	r0, #0
	beq	ez_mcp_settings

vega_settings:
	/* vega settings */
	/* SC: armclk=208Mhz, hclk,pclk=104MHz */
	orr	r1, r1, #(1<<16)	/* use pll_fix */
	str	r1, [r2]
	bic	r1, r1, #(0x7f<<3)	/* clear nsc */
	bic	r1, r1, #(3<<10)	/* clear ksc */
	orr	r1, r1, #(14<<3)	/* nsc=14, ksc=0 */
	str	r1, [r2]
	orr	r1, r1, #(1<<18)	/* enable pll_sc */
	str	r1, [r2]
	b	speed_ok

ez_mcp_settings:
	/* ez_mcp settings */
	/* reduced clock because of hardware constrains */
	/* SC: armclk=165MHz, hclk,pclk=83MHz */
	orr	r1, r1, #(1<<16)	/* use pll_fix */
	str	r1, [r2]
	bic	r1, r1, #(0x7f<<3)	/* clear nsc */
	bic	r1, r1, #(3<<10)	/* clear ksc */
	orr	r1, r1, #(11<<3)	/* nsc=11, ksc=0 */
	str	r1, [r2]
	orr	r1, r1, #(1<<18)	/* enable pll_sc */
	str	r1, [r2]

speed_ok:
	mov	r0, #(0x29<<8)		/* wait max 250us */
					/* roughly calculated 208MHz,
					   -> 52000ticks,
					   5intructions -> 10400 */
wait_scpll:				/* wait for pll lock */
	ldr	r1, [r2]
	ands	r1, r1, #(1<<19)
	bne	scpll_ok
	subs	r0, r0, #1
	bne	wait_scpll

scpll_ok:
	ldr	r1, [r2]
	bic	r1, r1, #(0xf<<12)	/* clear mahb */
	orr	r1, r1, #(1<<12)	/* mahb=1 hclk,pclk1=armclk/2 */
	bic	r1, r1, #(0x7)		/* clear msc */
	str     r1, [r2]
	bic	r1, r1, #(1<<16)	/* switch to SC */
	str	r1, [r2]

	/* per2: etnrefclk 50MHz */
	ldr	r2, =REG_CGU_PER2CON
	ldr	r1, =PER2CON		/* reset value */
	str	r1, [r2]
	mov	r1, #0
	orr	r1, r1, #(15<<9)
	orr	r1, r1, #(63<<3)
	orr	r1, r1, #(3)
	orr	r1, r1, #(1<<19)
	bic	r1, r1, #(1<<21)
	str	r1, [r2]
	orr	r1, r1, #(1<<16)	/* enable pll_per2 */
	str	r1, [r2]
	mov	r0, #(0x29<<8)		/* wait max 250us */
wait_per2pll:				/* wait for pll lock */
	ldr	r1, [r2]
	ands	r1, r1, #(1<<17)
	bne	per2pll_ok
	subs	r0, r0, #1
	bne	wait_per2pll
per2pll_ok:
	ldr	r1, [r2]
	orr	r1, r1, #(1<<23)
	orr	r1, r1, #(1<<13)
	str	r1, [r2]

	ldr	r2, =REG_CGU_PER2BWCON	/* set default value */
	ldr	r1, =PER2BWCON
	str	r1, [r2]


#endif
	/* setup memory controller */
	b	memsetup		/* jump to the next assembler code */
					/* board/firetux/memsetup.S */

