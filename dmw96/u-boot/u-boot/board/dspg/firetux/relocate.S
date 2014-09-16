/*
 * firetux board specific setup
 * based on cpu/arm926ejs/start.S
 * modified to relocate the u-boot code at end of RAM
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */


#include <config.h>
#include <version.h>

/* external labels needed for calculation */
.extern _start
.extern _armboot_start
.extern _bss_start
.extern _bss_end

/* these are defined in the board-specific linker script */
.global _relocation_table
_relocation_table:
	.word __relocation_table
.global _got
_got:
	.word _GLOBAL_OFFSET_TABLE_

/* local labels */
_start_armboot:
	.word start_armboot
__start:
	.word _start
_armbootstart:
	.word _armboot_start
_bssstart:
	.word _bss_start
_bssend:
	.word _bss_end
_rel_offset:
	.word .


.global firetux_relocate_code
firetux_relocate_code:

/* input from memsetup:
   r3 and r4 hold the size of the installed memory */


/* with nandflash we skipped the lowlevelinit and memsetup */
/* but we have to provide the memory settings here */
#if defined(CONFIG_NANDFLASH) || defined(CONFIG_SKIP_RELOCATE_UBOOT)

/* read sdram settings from the controller */
#define REG_SDI_CFG1	0xC1200000	/* SDRAM controller configuration 1 */
	ldr	r1, =REG_SDI_CFG1
	ldr	r2, [r1]
	ands	r0, r2, #0x3		/* dram width 8, 16, 32, 64bit */
	lsr	r2, r2, #4
	ands	r4, r2, #0x3		/* columns 8, 9, 10, 11 */
	add	r4, r4, #8
	add	r0, r0, r4
	lsr	r2, r2, #4
	ands	r4, r2, #0x7		/* rows 9 .. 16 */
	add	r4, r4, #9
	add	r0, r0, r4
	lsr	r2, r2, #4
	ands	r4, r2, #0x3		/* nr banks 1, 2, 4 */
	add	r0, r0, r4
	mov	r2, #1
	lsl	r3, r2, r0
	mov	r4, r3			/* save size in r3 and r4 */
#endif

/* calculate address offset */
	adr	r0, _rel_offset
	ldr	r1, =_rel_offset
	sub	r2, r0, r1
	mov	r12, r2			/* offset to _start saved in r12 */

/* if running from sdram, skip relocating */
#ifndef CONFIG_SKIP_RELOCATE_UBOOT
	ldr	r0, =_start		/* original start */
	add	r0, r0, r12		/* r0 <- current position of code */
	mov	r2, r0			/* save in r2 */
	mov	r1, #PHYS_SDRAM_1	/* test if we run from flash or RAM */
	and	r0, r0, #0xf0000000	/* check if segment addr match */
	and	r1, r1, #0xf0000000	/* check if segment addr match */
#ifndef CONFIG_NANDFLASH		/* if coming from NAND we do */
					/* relocate, because loader copied */
					/* us into middle of RAM */
	cmp	r0, r1                  /* don't reloc when already in ram */
	bne	firetux_relocate
	mov	r4, r2			/* if not relocated, start of uboot */
					/* need to be in r4 for stack setup */
	b	firetux_stack_setup
#endif

/* relocation code */
firetux_relocate:			/* relocate U-Boot to RAM */
	ldr	r3, _bssend		/* address of label */
	ldr	r3, [r3, r12]		/* address of _bss_end */
	ldr	r2, __start		/* address of _start */
	sub	r3, r3, r2		/* space needed for uboot with bss */
	mov	r2, #PHYS_SDRAM_1	/* ram start */
	add	r2, r2, r4		/* add ram_size (memsetup.S) */
	sub	r2, r2, #4		/* adjust off by one opcode */
	sub	r2, r2, r3		/* sub size of uboot */
	mvn	r1, #0xff		/* load 0xffffff00 */
	and	r1, r2, r1		/* clear last 8bit to get alignment */
					/* r1 <- destination address */
	ldr	r0, __start
	add	r0, r0, r12		/* r0 <- current position of code */
					/*       source address */
	mov	r4, r1			/* save destination addr */
	ldr	r2, __start		/* address of _start */
	ldr	r3, _bssstart		/* address of label */
	ldr	r3, [r3, r12]		/* address of _bss_start */
	sub	r2, r3, r2		/* r2 <- size of armboot without bss */
	add	r2, r0, r2		/* r2 <- end of source address */

firetux_copy_loop:
	ldmia	r0!, {r5-r10}		/* copy from source address [r0] */
	stmia	r1!, {r5-r10}		/* copy to   target address [r1] */
	cmp	r0, r2			/* until source end addreee [r2] */
	ble	firetux_copy_loop
	b	firetux_stack_setup
#endif	/* CONFIG_SKIP_RELOCATE_UBOOT */
	ldr	r4, =_start		/* if not relocated, start of uboot */
	add	r0, r0, r12		/* r0 <- current position of code */
					/* need to be in r4 for stack setup */

	/* Set up the stack */
firetux_stack_setup:
	mov	r0, r4					/* relocated uboot */
	sub	r0, r0, #CONFIG_SYS_MALLOC_LEN		/* malloc area */
	sub	r0, r0, #CONFIG_SYS_GBL_DATA_SIZE	/* bdinfo */
#ifdef CONFIG_USE_IRQ
	sub	r0, r0, #(CONFIG_STACKSIZE_IRQ+CONFIG_STACKSIZE_FIQ)
#endif
	sub	sp, r0, #12		/* leave 3 words for abort-stack */

firetux_clear_bss:
	mov	r0, r4			/* start address of relocated uboot */
	ldr	r2, __start		/* address of _start */
	ldr	r3, _bssstart		/* address of label */
	ldr	r3, [r3, r12]		/* address of _bss_start */
	sub	r2, r3, r2		/* offset from start */
	add	r0, r0, r2		/* new bss_start in ram */
					/* r0 <- new _bss_start */
	ldr	r1, _bssstart		/* address of label */
	ldr	r1, [r1, r12]		/* address of _bss_start */
	ldr	r2, _bssend		/* address of label */
	ldr	r2, [r2, r12]		/* address of _bss_end */
	sub	r1, r2, r1		/* calc length of bss */
	add	r1, r0, r1		/* r1 <- new bss_end in ram */
	mov 	r2, #0x00000000		/* clear */

firetux_clbss_l:
	str	r2, [r0]		/* clear loop... */
	add	r0, r0, #4
	cmp	r0, r1
	ble	firetux_clbss_l

#ifndef CONFIG_SKIP_RELOCATE_UBOOT
relocate_variables:
	/* we need to relocate the _armboot_start, _bss_start and _bss_end
	 * variables to the new u-boot location, which is needed for
	 * lib_arm/board.c for the caculation of the memory locations for
	 * board setup */
	ldr	r0, __start		/* start of old code */
	sub	r3, r4, r0		/* calculate offset between old/new */
					/* r3 <- value to add */
	ldr	r0, _armbootstart	/* start of old code */
	ldr	r2, [r0, r12]		/* read value */
	add	r2, r2, r3		/* add offset */
	str	r2, [r0, r3]		/* store new value */
	ldr	r0, _bssstart		/* get old address */
	ldr	r2, [r0, r12]		/* read value */
	add	r2, r2, r3		/* add offset */
	str	r2, [r0, r3]		/* store new value */
	ldr	r0, _bssend		/* get old address */
	ldr	r2, [r0, r12]		/* read value */
	add	r2, r2, r3		/* add offset */
	str	r2, [r0, r3]		/* store new value */

	/* OK, we need to fix the whole GLOBAL_OFFSET_TABLE till bss section */
fix_global_offset_table:
	ldr	r0, __start		/* get old starting point */
	mov	r3, r0			/* r3 <- save for compare lower limit */
	sub	r0, r4, r0		/* r0 <- calculate offset to new start*/
	ldr	r1, _relocation_table	/* get old table address */
	add	r1, r1, r0		/* r1 <- calculate new table address */
	ldr	r2, _bssend		/* get label address */
	ldr	r2, [r2, r12]		/* get old table end */
	mov	r5, r2			/* r5 <- save for compare upper limit */
	sub	r2, r2, #4
	add	r2, r2, r0		/* r2 <- calculate new end address */

2:	ldr	r6, [r1]		/* load table value */
	cmp	r6, r3			/* check for lower value */
	blt	1f
	cmp	r6, r5			/* check for upper limit */
	bgt	1f
	add	r6, r6, r0
	str	r6, [r1]
1:	add	r1, r1, #4		/* increase address */
	cmp	r1, r2			/* compare with end address */
	ble	2b
#endif	/* CONFIG_SKIP_RELOCATE_UBOOT */

	/* calculate addr for start_armboot, because jump maybe more than 32MB*/
	ldr	r0, __start		/* start of code */
	ldr	r1, _start_armboot	/* start of c-function */
	sub	r0, r1, r0		/* offset from beginning */
	add	r0, r0, r4		/* calculate new addr in ram */
	mov	lr, r0			/* save start addr in lr */

	/* set PIC offset register (sl/r10) */
	ldr	r1, __start
	add	r1, r1, r12		/* addr of original u-boot */
	adr	r2, _got		/* addr of _got label */
	sub	r1, r2, r1		/* offset from _start */

	ldr	sl, _got		/* get got offset from _got */
	add	sl, sl, r1		/* add offset of _got */
	add	sl, sl, r4		/* add start + offset for new got */

	mov	pc, r0			/* jump into C code start_armboot */
					/* lib_arm/board.c */
