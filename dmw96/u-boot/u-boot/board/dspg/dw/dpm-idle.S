/*
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 * These are the low level assembler for performing dpm idle task
 *
 *  CONFIG_CPU_ARM926_CPU_IDLE -> nohlt
 */
#include "dw.h"
/*
 * This is the maximum size of an area which will be invalidated
 * using the single invalidate entry instructions.  Anything larger
 * than this, and we go for the whole cache.
 *
 * This value should be chosen such that we choose the cheapest
 * alternative.
 */
#define CACHE_DLIMIT	16384

/*
 * the cache line size of the I and D cache
 */
#define CACHE_DLINESIZE	32

	.text

/*
 * cpu_arm926_do_idle_selfrefresh()
 *
 * parameters:
 * 	r0 - aux bandgap
 *	r1 - comparator gain for 4.2
 *  r2 - comperator gain for 3.8
 *  r3 - temperature reference
 *	stack+16 - minimal operation voltage
 *	stack+20 - offset
 * Called with IRQs disabled
 * Note: we don't touch r0-r3, so they are passed to stack+20 callback (look for "blx r7" below)
 */
	.align	10 /* Alligned at least to 8 words  cacheline_aligned*/
.globl __Start_of_idle
__Start_of_idle:
/*ENTRY(cpu_arm926_do_idle_selfrefresh)*/
.globl cpu_arm926_do_idle_selfrefresh
cpu_arm926_do_idle_selfrefresh:

@ push registers we are going to ruin (r0-r3 can be ruined by the function)
	stmdb   r13!,{r4-r7}

@ read 5th and 6th parameters into r6 and r7
	ldr		r6, [sp, #16]
	ldr		r7, [sp, #20]

@SXX move MPMC to self refresh
	ldr	r5, mpmc_dyn_ctrl
	ldr	r4, [r5]
	orr	r4, r4, #0x4
	str	r4, [r5]

@SXX Verify self refresh is active
	ldr	r5, mpmc_status
poll1:
	ldr	r4,[r5]
	and	r4, #0x4
	cmp	r4, #0x4
	bne	poll1

@SXX turn off CE and CS bits
	ldr	r5, mpmc_dyn_ctrl
	ldr	r4, [r5]
	bic	r4, r4, #0x3
	str	r4, [r5]

@do_dpm_fastcharge
	ldr     r5, retaddr
	str     lr, [r5, #0]
	ldr     r5, origstack
	str     sp, [r5, #0]
	ldr     sp, dpmstack

@ r0-r3 already contains the parameters we want to pass
@ push the 5th parameter which we stored at r6
@ jump to r7 which was the 6th parameter of the function (stored at r7)
	stmdb   sp!,{r6}
	blx     r7
	ldmia   sp!,{r6}

	ldr     r5, origstack
	ldr     sp, [r5, #0]

@SXX Add a delay
	mov r0, #0xff
loopdeb1:
	nop
	sub r0, #1
	teq r0, #0
	bne loopdeb1

@SXX turn on CE and CS bits
	ldr	r0, mpmc_dyn_ctrl
	ldr	r1, [r0]
	orr	r1, r1, #0x3
	str	r1, [r0]

@SXX enable MPMC to normal mode
	ldr	r1, [r0]
	bic	r1, r1, #0x4
	str	r1, [r0]

@SXX wait for MPMC to get back to normal mode
	ldr	r0,mpmc_status
poll0:
	ldr	r1, [r0]
	and	r1, #0x4
	cmp	r1, #0x4
	beq	poll0

@SXX Add a delay
	mov r0, #0xff
loopdeb2:
	nop
	sub r0, #1
	teq r0, #0
	bne loopdeb2

	ldr     r5, retaddr
	ldr     lr, [r5, #0]

	ldmia   r13!,{r4-r7}
	mov	pc, lr

mpmc_dyn_ctrl:
	.word	DW_MPMC_DYN_CTRL

mpmc_status:
	.word	DW_MPMC_STATUS

cmu_lpcr:
	.word	DW_CMU_BASE +0x18

cmu_clksel:
	.word	DW_CMU_BASE +0x00

retaddr:
	.word   0x16003FF8

origstack:
	.word   0x16003FF4

dpmstack:
	.word   0x16003F00

.globl __End_of_idle
__End_of_idle:

