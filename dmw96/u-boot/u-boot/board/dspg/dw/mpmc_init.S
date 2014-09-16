/*
 *  Memory setup for the DW
 *
 *  Copyright (C) 2009, DSPG Technologies GmbH
 *
 *  Based on board/dspg/firetux/memsetup.S
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

#include <config.h>
#include "dw.h"

#define MPMC_READCONFIG          0x00
#define MPMC_RASCAS              0x04
#define MPMC_CONFIG              0x08
#define MPMC_TRP                 0x0C
#define MPMC_TRAS                0x10
#define MPMC_TSREX               0x14
#define MPMC_TWR                 0x18
#define MPMC_TRC                 0x1C
#define MPMC_TRFC                0x20
#define MPMC_TXSR                0x24
#define MPMC_TRRD                0x28
#define MPMC_TMRD                0x2C
#define MPMC_TCDLR               0x30
#define MPMC_MODE_REG            0x34

/* memory map table (chip select <-> address) */
	.align	4
memory_map:
	.long	0x30000000       /* MPMC CS0: Static Mem 0 */
	.long	0x3C000000       /* MPMC CS1: Static Mem 1 */
	.long	0x34000000       /* MPMC CS2: Static Mem 2 */
	.long	0                /* MPMC CS3: Static Mem 3 */
	.long	0x34000000       /* MPMC CS4: Dynamic Mem 0 */
	.long	0x60000000       /* MPMC CS5: Dynamic Mem 1 */
	.long	0                /* MPMC CS6: Dynamic Mem 2 */
	.long	0                /* MPMC CS7: Dynamic Mem 3 */

/* tables for memory settings */
	.align	4
memsetting_EXPEDIBLUE:
	/*
	 * 128mb low-power SDR-SDRAM:
	 *   32bit, 512Mb (32Mx16), 4 banks, row = 13, col = 10
	 */
	.long	0x12             /* MPMCDynamicReadConfig */
	.long	0x202            /* MPMCDynamicRasCas0-3 */
	.long	0x5882           /* MPMCDynamicConfig0-3 */
	.long	4                /* MPMCDynamictRP */
	.long	8                /* MPMCDynamictRAS */
	.long	15               /* MPMCDynamictSREX */
	.long	15               /* MPMCDynamictWR */
	.long	12               /* MPMCDynamictRC */
	.long	15               /* MPMCDynamictRFC */
	.long	15               /* MPMCDynamictXSR */
	.long	4                /* MPMCDynamictRRD */
	.long	2                /* MPMCDynamictMRD */
	.long	4                /* MPMCDynamictCDLR */
	.long	0x88000          /* mode register address */

	.align	4
memsetting_EXPEDITOR_64M:
	/* 64mb SDR-SDRAM: 4 banks, row = 13, col = 9 */
	.long	0x11             /* MPMCDynamicReadConfig */
	.long	0x303            /* MPMCDynamicRasCas0-3 */
	.long	0x4680           /* MPMCDynamicConfig0-3 */
	.long	4                /* MPMCDynamictRP */
	.long	8                /* MPMCDynamictRAS */
	.long	15               /* MPMCDynamictSREX */
	.long	15               /* MPMCDynamictWR */
	.long	12               /* MPMCDynamictRC */
	.long	15               /* MPMCDynamictRFC */
	.long	15               /* MPMCDynamictXSR */
	.long	4                /* MPMCDynamictRRD */
	.long	2                /* MPMCDynamictMRD */
	.long	4                /* MPMCDynamictCDLR */
	.long	0x60000          /* mode register address */

	.align	4
memsetting_EXPEDITOR_128M:
	/* 128mb SDR-SDRAM: 4 banks, row = 13, col = 9 */
	.long	0x11             /* MPMCDynamicReadConfig */
	.long	0x303            /* MPMCDynamicRasCas0-3 */
	.long	0x4880           /* MPMCDynamicConfig0-3 */
	.long	4                /* MPMCDynamictRP */
	.long	8                /* MPMCDynamictRAS */
	.long	15               /* MPMCDynamictSREX */
	.long	15               /* MPMCDynamictWR */
	.long	12               /* MPMCDynamictRC */
	.long	15               /* MPMCDynamictRFC */
	.long	15               /* MPMCDynamictXSR */
	.long	4                /* MPMCDynamictRRD */
	.long	2                /* MPMCDynamictMRD */
	.long	4                /* MPMCDynamictCDLR */
	.long	0xC0000          /* mode register address */

.globl __platform_mpmc_init
__platform_mpmc_init:
	/*
	 * stack is not setup yet, we have to backup the link register
	 *
	 * since r11 is used for the board type, we use r10 so we can jump
	 * into and return from subroutines
	 */
	mov	r10, lr

	ldr	r6, =MPMC_BASE

	/* set little endian */
	mov	r0, #0
	str	r0, [r6, #MPMCConfig]

	/*
	 * if refresh is not zero then we must be already initialised. 
	 */
	ldr	r0, [r6, #MPMCDynamicRefresh]
	cmp	r0, #0
	bne	return

	/* set timeouts */
	mov	r0, #25
	str	r0, [r6, #MPMCAHBTimeout0] /* LCDC */
	mov	r0, #150
	str	r0, [r6, #MPMCAHBTimeout1] /* Wifi */
	mov	r0, #100
	str	r0, [r6, #MPMCAHBTimeout2] /* Scaler, OSDM, JPEG, Ethernet */
	mov	r0, #100
	str	r0, [r6, #MPMCAHBTimeout3] /* NAND, SD/MMC, Sony MS, USB2.0 */
	mov	r0, #200
	str	r0, [r6, #MPMCAHBTimeout4] /* CPU data */
	mov	r0, #200
	str	r0, [r6, #MPMCAHBTimeout5] /* CPU instructions */

	cmp	r11, #0
	bne	setup_expeditor

setup_expediblue:
	mov	r4, #5
	adr	r3, memsetting_EXPEDIBLUE
	bl	mem_init
	mov	r3, #(128<<20) /* check for 128mb */
	bl	mem_test
	cmp	r3, #0
	bne	return
	beq	_start          /* unrecoverable; start over */

setup_expeditor:
	mov	r4, #5
	adr	r3, memsetting_EXPEDITOR_128M
	bl	mem_init
	mov	r3, #(128<<20) /* check for 128mb */
	bl	mem_test
	cmp	r3, #0
	bne	setup_expeditor_bank2

	adr	r3, memsetting_EXPEDITOR_64M
	bl	mem_init
	mov	r3, #(64<<20)   /* check for 64mb */
	bl	mem_test
	cmp	r3, #0
	beq	_start          /* unrecoverable; start over */

setup_expeditor_bank2:
	mov	r4, #4
	adr	r3, memsetting_EXPEDITOR_64M
	bl	mem_init
	mov	r3, #(64<<20)   /* check for 64mb */
	bl	mem_test
	cmp	r3, #0          /* if no memory has been found... */
		                /* ... clear the dynamic config entry */
	subeq	r4, #4
	moveq	r4, r4, lsl #5
	addeq	r4, r4, #MPMCDynamicConfig0
	streq	r3, [r6, +r4]

return:
	mov	pc, r10


/*
 * initialize the mpmc with settings from the memsetting table
 *
 * r0: used for temporary values
 * r1: used for temporary values
 * r3: address of memsetting table
 * r4: chip select to configure
 * r6: mpmc base
 */
mem_init:
	ldr	r0, [r3, #MPMC_READCONFIG]
	str	r0, [r6, #MPMCDynamicReadConfig]

	/* sdram initialization value to NOP */
	ldr	r0, =0x183
	str	r0, [r6, #MPMCDynamicControl]

	/* perform refresh cycles */
	mov	r0, #0xF000
1:	nop
	nop
	subs	r0, #1
	bne	1b

	/* sdram initialization value to PALL */
	ldr	r0, =0x103
	str	r0, [r6, #MPMCDynamicControl]

	/* write small value into refresh register */
	mov	r0, #4
	str	r0, [r6, #MPMCDynamicRefresh]

	/* perform refresh cycles */
	mov	r0, #0xF000
1:	nop
	nop
	subs	r0, #1
	bne	1b

	/* set refresh rate */
	mov	r0, #50
	str	r0, [r6, #MPMCDynamicRefresh]

	/* perform refresh cycles */
	mov	r0, #0xF000
1:	nop
	nop
	subs	r0, #1
	bne	1b

	sub	r1, r4, #4
	mov	r1, r1, lsl #5
	add	r1, r1, #MPMCDynamicRasCas0
	ldr	r0, [r3, #MPMC_RASCAS]
	str	r0, [r6, +r1]

	sub	r1, r4, #4
	mov	r1, r1, lsl #5
	add	r1, r1, #MPMCDynamicConfig0
	ldr	r0, [r3, #MPMC_CONFIG]
	str	r0, [r6, +r1]

	ldr	r0, [r3, #MPMC_TRP]
	str	r0, [r6, #MPMCDynamictRP]

	ldr	r0, [r3, #MPMC_TRAS]
	str	r0, [r6, #MPMCDynamictRAS]

	ldr	r0, [r3, #MPMC_TSREX]
	str	r0, [r6, #MPMCDynamictSREX]

	ldr	r0, [r3, #MPMC_TWR]
	str	r0, [r6, #MPMCDynamictWR]

	ldr	r0, [r3, #MPMC_TRC]
	str	r0, [r6, #MPMCDynamictRC]

	ldr	r0, [r3, #MPMC_TRFC]
	str	r0, [r6, #MPMCDynamictRFC]

	ldr	r0, [r3, #MPMC_TXSR]
	str	r0, [r6, #MPMCDynamictXSR]

	ldr	r0, [r3, #MPMC_TRRD]
	str	r0, [r6, #MPMCDynamictRRD]

	ldr	r0, [r3, #MPMC_TMRD]
	str	r0, [r6, #MPMCDynamictMRD]

	ldr	r0, [r3, #MPMC_TCDLR]
	str	r0, [r6, #MPMCDynamictCDLR]

	/* sdram initialization value to MODE */
	mov	r0, #0x083
	str	r0, [r6, #MPMCDynamicControl]

	/* perform refresh cycles */
	mov	r0, #0xF000
1:	nop
	nop
	subs	r0, #1
	bne	1b

	/* program mode registers (do a read from memory base + addr) */
	ldr	r0, [r3, #MPMC_MODE_REG]
	adr	r1, memory_map
	ldr	r1, [r1, +r4, lsl #2]
	ldr	r0, [r1, +r0]

	/* sdram initialization value to PALL */
	ldr	r0, =0x103
	str	r0, [r6, #MPMCDynamicControl]

	/* sdram initialization value to NORMAL */
	mov	r0, #3
	str	r0, [r6, #MPMCDynamicControl]

	mov	pc, lr

/*
 * check if memory is working fine for the given size
 *
 * r1: used for temporary values
 * r2: used for temporary values
 * r3: memory size
 * r4: chip select
 * 
 * returns 0 in r3 if check has failed
 */
mem_test:
	mov	r1, r3
1:
	/* fetch base address from memory map table */
	adr	r2, memory_map
	ldr	r2, [r2, +r4, lsl #2]

	/* write test values */
	add	r2, r2, r1
	sub	r2, r2, #4
	str	r1, [r2]
	mov	r1, r1, LSR #1
	cmp	r1, #4
	bgt	1b
	mov	r1, r3
1:
	/* fetch base address from memory map table */
	adr	r2, memory_map
	ldr	r2, [r2, +r4, lsl #2]

	/* read back and check test values */
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

