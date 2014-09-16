/*
 * Memory setup for PNX8181
 *
 * (C) Copyright 2007-2009, emlix GmbH, Germany
 * Juergen Schoew, <js@emlix.com>
 *
 * (C) Copyright 2008, DSPG Technologies GmbH, Germany
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
 *	Memory setup
 *	- the reset defaults are assumed sufficient
 */

#include <config.h>
#include <version.h>

#ifndef CONFIG_NANDFLASH	/* with nandflash we have to skip memconfig */

/* SDI */
#define REG_SDI_CFG1	0xC1200000	/* SDRAM controller configuration 1 */
#define REG_SDI_PWR_CTL	0xC1200004	/* Power savings parameters */
#define REG_SDI_RFRSH1	0xC1200008	/* Refresh parameters 1 */
#define REG_SDI_RFRSH2	0xC120000C	/* Refresh parameters 2 */
#define REG_SDI_TIM1	0xC1200010	/* Timings parameters 1 */
#define REG_SDI_TIM2	0xC1200014	/* Timings parameters 2 */
#define REG_SDI_MODE	0xC1200018	/* Mode */
#define REG_SDI_EXT_MOD	0xC120001C	/* Extended mode */
#define REG_SDI_CFG2	0xC1200020	/* SDRAM controller configuration 2 */
#define REG_SDI_SCHED0	0xC1200100	/* Data port #0 scheduler */
#define REG_SDI_SCHED1	0xC1200104	/* Data port #1 scheduler */
#define REG_SDI_SCHED2	0xC1200108	/* Data port #2 scheduler */
#define REG_SDI_WTAG0	0xC1200300	/* Data port #0 write tag enable */
#define REG_SDI_WTAG1	0xC1200304	/* Data port #1 write tag enable */
#define REG_SDI_WTAG2	0xC1200308	/* Data port #2 write tag enable */
#define REG_SDI_TAM	0xC1200310	/* Tag address mask */
#define REG_SDI_SW_CTRL	0xC1200400	/* Software control register */
#define REG_SDI_STAT	0xC1200800	/* Status register */
#define REG_SDI_IP_ID	0xC1200FFC	/* IP identification */

/* table offsets */
#define SDI_CFG1	0x00
#define SDI_CFG2	0x04
#define SDI_PWR_CTRL	0x08
#define SDI_RFRSH1	0x0c
#define SDI_RFRSH2	0x10
#define SDI_TIM1	0x14
#define SDI_TIM2	0x18
#define SDI_MODE	0x1c
#define SDI_EXT_MODE	0x20
#define SDI_SCHED0	0x24
#define SDI_SCHED1	0x28
#define SDI_SCHED2	0x2c
#define SDI_WTAG0	0x30
#define SDI_WTAG1	0x34
#define SDI_WTAG2	0x38
#define SDI_TAM		0x3c

#define REG_CGU_SCCON	0xC2200004	/* SC Controller for hclk */

/* table for memory settings */
    .align	4
memsetting_EZ_MCP_PNX8181_89MHZ:
/*
 * settings for Samsung K4M64163PK
 * 8MB @hclk=89.86MHz
 */
	.long	0x22301		/* define SDI_CFG1 */
	.long	0x200		/* define SDI_CFG2 */
	.long	0x1000000	/* define SDI_PWR_CTRL */
	.long	0x579		/* define SDI_RFRSH1 */
	.long	0x0		/* define SDI_RFRSH2 */
	.long	0x1131242	/* define SDI_TIM1 */
	.long	0xb08		/* define SDI_TIM2 */
	.long	0x23		/* define SDI_MODE */
	.long	0x18		/* define SDI_EXT_MODE */
	.long	0x80000121	/* define SDI_SCHED0 */
	.long	0xffff0100	/* define SDI_SCHED1 */
	.long	0x00030131	/* define SDI_SCHED2 */
	.long	0xf		/* define SDI_WTAG0 */
	.long	0xf		/* define SDI_WTAG1 */
	.long	0xf		/* define SDI_WTAG2 */
	.long	0x0		/* define SDI_TAM */

	.align	4
memsetting_EZ_MCP_PNX8181_103MHZ:
/*
 * settings for Samsung K4M64163PK
 * 8MB @hclk=103.68MHz
 */
	.long	0x22301		/* define SDI_CFG1 */
	.long	0x200		/* define SDI_CFG2 */
	.long	0x1000000	/* define SDI_PWR_CTRL */
	.long	0x651		/* define SDI_RFRSH1 */
	.long	0x0		/* define SDI_RFRSH2 */
	.long	0x1131262	/* define SDI_TIM1 */
	.long	0xc0a		/* define SDI_TIM2 */
	.long	0x23		/* define SDI_MODE */
	.long	0x18		/* define SDI_EXT_MODE */
	.long	0x80000121	/* define SDI_SCHED0 */
	.long	0xffff0100	/* define SDI_SCHED1 */
	.long	0x00030131	/* define SDI_SCHED2 */
	.long	0xf		/* define SDI_WTAG0 */
	.long	0xf		/* define SDI_WTAG1 */
	.long	0xf		/* define SDI_WTAG2 */
	.long	0x0		/* define SDI_TAM */

	.align	4
memsetting_VEGA_PNX8181_BASESTATION_III_a:
/*
 * settings for Micron MT48LC8M16A2
 * 16MB @hclk=103.68MHz
 */
	.long	0x22311		/* define SDI_CFG1 */
	.long	0x200		/* define SDI_CFG2 */
	.long	0x1000000	/* define SDI_PWR_CTRL */
	.long	0x651		/* define SDI_RFRSH1 */
	.long	0x0		/* define SDI_RFRSH2 */
	.long	0x1131262	/* define SDI_TIM1 */
	.long	0xc0a		/* define SDI_TIM2 */
	.long	0x23		/* define SDI_MODE */
	.long	0x18		/* define SDI_EXT_MODE */
	.long	0x80000121	/* define SDI_SCHED0 */
	.long	0xffff0100	/* define SDI_SCHED1 */
	.long	0x00030131	/* define SDI_SCHED2 */
	.long	0xf		/* define SDI_WTAG0 */
	.long	0xf		/* define SDI_WTAG1 */
	.long	0xf		/* define SDI_WTAG2 */
	.long	0x0		/* define SDI_TAM */

	.align	4
memsetting_VEGA_PNX8181_BASESTATION_III_b:
/*
 * settings for Micron MT48LC16M16A2
 * 32MB @hclk=103.68MHz
 */
	.long	0x22411		/* define SDI_CFG1 */
	.long	0x200		/* define SDI_CFG2 */
	.long	0x1000000	/* define SDI_PWR_CTRL */
	.long	0x651		/* define SDI_RFRSH1 */
	.long	0x0		/* define SDI_RFRSH2 */
	.long	0x1131262	/* define SDI_TIM1 */
	.long	0xc0a		/* define SDI_TIM2 */
	.long	0x23		/* define SDI_MODE */
	.long	0x18		/* define SDI_EXT_MODE */
	.long	0x80000121	/* define SDI_SCHED0 */
	.long	0xffff0100	/* define SDI_SCHED1 */
	.long	0x00030131	/* define SDI_SCHED2 */
	.long	0xf		/* define SDI_WTAG0 */
	.long	0xf		/* define SDI_WTAG1 */
	.long	0xf		/* define SDI_WTAG2 */
	.long	0x0		/* define SDI_TAM */

#endif

.globl memsetup
memsetup:
/* with nandflash we have to skip memconfig */
#if !defined(CONFIG_NANDFLASH) && !defined(CONFIG_SKIP_RELOCATE_UBOOT)
	/* check 32MB of Vega III-b / III-c */
	adr	r3, memsetting_VEGA_PNX8181_BASESTATION_III_b
	mov	r4, lr
	bl	do_memsetup
	mov	lr, r4
#endif
	mov	r3, #(32<<20)		/* check for 32MB */
/* with nandflash we have to skip memconfig */
#if !defined(CONFIG_NANDFLASH) && !defined(CONFIG_SKIP_RELOCATE_UBOOT)
	bl	memtest
	mov	lr, r4
	cmp	r3, #0
	bne	2f

	/* check 16MB of Vega III-a */
	adr	r3, memsetting_VEGA_PNX8181_BASESTATION_III_a
	mov	r4, lr
	bl	do_memsetup
	mov	lr, r4
	mov	r3, #(16<<20)		/* check for 16MB */
	bl	memtest
	mov	lr, r4
	cmp	r3, #0
	bne	2f

	/* fallback, check for 8MB of EZ_MCP */
	adr	r3, memsetting_EZ_MCP_PNX8181_103MHZ
	ldr	r1, =REG_CGU_SCCON
	ldr	r0, [r1]
	mov	r1, #(0x7f<<3)
	ands	r0, r0, r1
	cmp	r0, #(14<<3)		/* check for hclk=103 */
	bge	1f
	/* OK, we do not have 103MHz, so chosse other setting */
	adr	r3, memsetting_EZ_MCP_PNX8181_89MHZ
1:
	mov	r4, lr
	bl	do_memsetup

	mov	r1, #0x01		/* precharge all */
	ldr	r2, =REG_SDI_SW_CTRL
	str	r1, [r2]
	mov	r1, #0x09		/* load extended mode register */
	ldr	r2, =REG_SDI_SW_CTRL
	str	r1, [r2]

	mov	lr, r4
	mov	r3, #(8<<20)		/* check for 8MB */
	bl	memtest
	mov	lr, r4
	cmp	r3, #0
	bne	2f
	nop
	nop
2:					/* setup done */
#endif
	mov	r4, r3			/* r3 holds the memory size */
					/* save a copy in r4 */
	b	firetux_relocate_code	/* jump to the next assembler code */
					/* board/firetux/relocate.S */



/* Fill the sdram controller with the values from the table */

do_memsetup:
/* with nandflash we have to skip memconfig */
#if !defined(CONFIG_NANDFLASH) && !defined(CONFIG_SKIP_RELOCATE_UBOOT)
	/*
	 * CGU setup
	 * pin multiplexing
	 * SDRAM config
	 */
	mov	r1, #0x00		/* NOP */
	ldr	r2, =REG_SDI_SW_CTRL
	str	r1, [r2]

	mov	r0, #(0x50<<8)		/* wait 200us */
					/*
					 * roughly calculated 208MHz,
					 * 40960ticks,
					 * 2 instructions per loop
					 */
1:
	subs	r0, r0, #1
	bne	1b

	mov	r1, #0x07		/* exit power down */
	ldr	r2, =REG_SDI_SW_CTRL
	str	r1, [r2]

	mov	r0, #(0x50<<8)		/* wait 200us */
1:
	subs	r0, r0, #1
	bne	1b

	ldr	r2, =REG_SDI_CFG1
	ldr	r1, [r3, #SDI_CFG1]
	str	r1, [r2]
	ldr	r2, =REG_SDI_CFG2
	ldr	r1, [r3, #SDI_CFG2]
	str	r1, [r2]
	ldr	r2, =REG_SDI_PWR_CTL
	ldr	r1, [r3, #SDI_PWR_CTRL]
	str	r1, [r2]
	ldr	r2, =REG_SDI_RFRSH1
	ldr	r1, [r3, #SDI_RFRSH1]
	str	r1, [r2]
	mov	r0, #(0x50<<8)		/* wait 200us */
1:
	subs	r0, r0, #1
	bne	1b
	orr	r1, r1, #(1<<24)	/* enable auto refresh generation */
	str	r1, [r2]
	ldr	r2, =REG_SDI_MODE
	ldr	r1, [r3, #SDI_MODE]
	str	r1, [r2]
	ldr	r2, =REG_SDI_EXT_MOD
	ldr	r1, [r3, #SDI_EXT_MODE]
	str	r1, [r2]
	ldr	r2, =REG_SDI_RFRSH2
	ldr	r1, [r3, #SDI_RFRSH2]
	str	r1, [r2]
	ldr	r2, =REG_SDI_TIM1
	ldr	r1, [r3, #SDI_TIM1]
	str	r1, [r2]
	ldr	r2, =REG_SDI_TIM2
	ldr	r1, [r3, #SDI_TIM2]
	str	r1, [r2]
	ldr	r2, =REG_SDI_SCHED0
	ldr	r1, [r3, #SDI_SCHED0]
	str	r1, [r2]
	ldr	r2, =REG_SDI_SCHED1
	ldr	r1, [r3, #SDI_SCHED1]
	str	r1, [r2]
	ldr	r2, =REG_SDI_SCHED2
	ldr	r1, [r3, #SDI_SCHED2]
	str	r1, [r2]
	ldr	r2, =REG_SDI_WTAG0
	ldr	r1, [r3, #SDI_WTAG0]
	str	r1, [r2]
	ldr	r2, =REG_SDI_WTAG1
	ldr	r1, [r3, #SDI_WTAG1]
	str	r1, [r2]
	ldr	r2, =REG_SDI_WTAG2
	ldr	r1, [r3, #SDI_WTAG2]
	str	r1, [r2]
	ldr	r2, =REG_SDI_TAM
	ldr	r1, [r3, #SDI_TAM]
	str	r1, [r2]

	/* send power-up info */
	mov	r1, #0x01		/* precharge all */
	ldr	r2, =REG_SDI_SW_CTRL
	str	r1, [r2]

	mov	r0, #8			/* 8 times */
1:
	mov	r1, #0x03		/* auto refresh */
	ldr	r2, =REG_SDI_SW_CTRL
	str	r1, [r2]
	subs	r0, r0, #1
	bne	1b

	mov	r1, #0x08		/* load mode register */
	ldr	r2, =REG_SDI_SW_CTRL
	str	r1, [r2]
	mov	pc,lr


/* check if the memory is working */

memtest:
	mov	r1, r3
1:
	ldr     r2, =CONFIG_SYS_SDRAM_BASE
	add     r2, r2, r1
	sub	r2, r2, #4
	str	r1, [r2]
	mov	r1, r1, LSR #1
	cmp	r1, #4
	bgt	1b
	mov	r1, r3
1:
	ldr	r2, =CONFIG_SYS_SDRAM_BASE
	add	r2, r2, r1
	sub	r2, r2, #4
	ldr	r0, [r2]
	cmp	r0, r1
	bne	2f
	mov	r1, r1, LSR #1
	cmp	r1, #4
	bgt	1b
	mov	pc, lr
2:
	mov	r3, #0
	mov	pc, lr

	nop
	nop
	nop
	nop

#endif
