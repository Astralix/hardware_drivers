/*
 * Board specific setup info
 *
 * (C) Copyright 2003, ARM Ltd.
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

#include <config.h>
#include <version.h>
#include "broadtile.h"

/* Set up the platform, once the cpu has been initialized */
.globl lowlevel_init
lowlevel_init:
	mov	r13, lr
	
	bl	init_fpga
	bl	init_mpmc
	bl	init_ssmc

	mov	lr, r13
	mov	pc, lr

/*
 * mpmc initialization
 */
init_mpmc:
	ldr	r6, =0x10110000 /* mpmc base */

	/* skip the init if we are already initialized */
	ldr	r0, [r6, #0x24]
	cmp	r0, #0
	bne	mpmc_done

	/* set static memory to 32bit */
	mov	r0, #2
	str	r0, [r6, #0x200]
	str	r0, [r6, #0x220]
	str	r0, [r6, #0x240]

	mov	r1, #0
	str	r1, [r6, #0x008]
	str	r0, [r6, #0x408]

	/* set ahbtimeouts */
	str	r1, [r6, #0x428]
	str	r1, [r6, #0x448]
	str	r1, [r6, #0x468]
	str	r1, [r6, #0x488]
	str	r1, [r6, #0x4a8]

	ldr	r0, =0x111
	str	r0, [r6, #0x028]
	ldr	r0, =0xc183
	str	r0, [r6, #0x020]
	ldr	r0, =0xc103
	str	r0, [r6, #0x020]
	mov	r0, #1
	str	r0, [r6, #0x024]

	/* wait a few cycles */
	mov	r0, #10
1:	subs	r0, #1
	bne	1b

	/*
	 * this is somehow calculated in VersatileBoardInit, but we just
	 * hardcode it here for convenience
	 */
	ldr	r0, =0x22
	str	r0, [r6, #0x024]

	/* set ras/cas */
	ldr	r0, =0x202
	str	r0, [r6, #0x104]
	str	r0, [r6, #0x124]
	str	r0, [r6, #0x144]
	str	r0, [r6, #0x164]

	/* set dynamicconfig */
	ldr	r0, =0x5880
	str	r0, [r6, #0x100]
	str	r0, [r6, #0x120]
	str	r0, [r6, #0x140]
	str	r0, [r6, #0x160]

	/* set timings */
	mov	r0, #2
	str	r0, [r6, #0x030]
	mov	r0, #3
	str	r0, [r6, #0x034]
	mov	r0, #5
	str	r0, [r6, #0x038]
	mov	r0, #4
	str	r0, [r6, #0x044]
	mov	r0, #5
	str	r0, [r6, #0x048]
	mov	r0, #5
	str	r0, [r6, #0x04c]
	mov	r0, #5
	str	r0, [r6, #0x050]
	mov	r0, #1
	str	r0, [r6, #0x054]
	mov	r0, #2
	str	r0, [r6, #0x058]
	mov	r0, #1
	str	r0, [r6, #0x05c]

	ldr	r0, =0x83
	str	r0, [r6, #0x020]

	/* read mode registers */
	ldr	r1, =0x04020000
	ldr	r0, [r1]
	ldr	r1, =0x08020000
	ldr	r0, [r1]
	ldr	r1, =0x70020000
	ldr	r0, [r1]
	ldr	r1, =0x78020000
	ldr	r0, [r1]

	ldr	r0, =0x103
	str	r0, [r6, #0x020]
	mov	r0, #3
	str	r0, [r6, #0x020]

mpmc_done:
	mov	pc, lr

/*
 * fpga initialization
 */
init_fpga:
	/*
	 * carry out the FPGA remap. this is done by first unlocking the sys
	 * register, then doing the remap, then locking the sys register again
	 */
	ldr	r1, =0x10000000 /* fpga system register base */
	ldr	r0, =0xa05f     /* unlock key */
	str	r0, [r1, #0x20] /* store to lock register */

	ldr	r0, [r1, #0x60] /* misc register */
	bic	r0, r0, #4	/* clear remap bit */
	str	r0, [r1, #0x60] /* store to misc register */

	mov	r0, #0
	str	r0, [r1, #0x20] /* lock up again */

	/* wait for fpga to fully reset */
	ldr	r0, =0x01800000
1:	subs	r0, #1
	bne	1b

	mov	pc, lr

/*
 * ssmc initialization
 */
init_ssmc:
	ldr	r0, =SSMC_BASE
	ldr	r1, =0x303021
	str	r1, [r0, #SSMC_STATICCS0_CONFIG]
	str	r1, [r0, #SSMC_STATICCS1_CONFIG]
	str	r1, [r0, #SSMC_STATICCS2_CONFIG]

	ldr	r0, =MPMC_BASE
	ldr	r1, =0x2
	str	r1, [r0, #MPMC_STATICCS0_CONFIG]
	str	r1, [r0, #MPMC_STATICCS1_CONFIG]
	str	r1, [r0, #MPMC_STATICCS2_CONFIG]

	/* intel nor */
	ldr	r0, =SSMC_BANK7
	ldr	r1, =0x303021
	str	r1, [r0, #SSMC_CR]
	mov	r1, #0
	str	r1, [r0, #SSMC_IDCYR]
	mov	r1, #4
	str	r1, [r0, #SSMC_WSTRDR]
	mov	r1, #3
	str	r1, [r0, #SSMC_WSTWRR]
	mov	r1, #0
	str	r1, [r0, #SSMC_WSTOENR]
	mov	r1, #1
	str	r1, [r0, #SSMC_WSTWENR]
	mov	r1, #2
	str	r1, [r0, #SSMC_WSTBRDR]

	/* sram init */
	ldr	r0, =MPMC_STATIC_BANK1
	ldr	r1, =0x8a
	str	r1, [r0, #MPMC_STATIC_CONFIG]
	mov	r1, #1
	str	r1, [r0, #MPMC_STATIC_WAIT_WEN]
	mov	r1, #0
	str	r1, [r0, #MPMC_STATIC_WAIT_OEN]
	mov	r1, #9
	str	r1, [r0, #MPMC_STATIC_WAIT_RD]
	mov	r1, #1
	str	r1, [r0, #MPMC_STATIC_WAIT_PAGE]
	mov	r1, #5
	str	r1, [r0, #MPMC_STATIC_WAIT_WR]
	mov	r1, #0
	str	r1, [r0, #MPMC_STATIC_WAIT_TURN]

	mov	pc, lr

