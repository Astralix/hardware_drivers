/*
 * Copyright (c) 2011 DSPG Technologies GmbH
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
 * DMW96 cache code
 */

.align 5
.global invalidate_dcache
.global l2_cache_enable
.global l2_cache_disable
.global get_device_type

/*
 *	invalidate_dcache()
 *
 *	Invalidate the whole D-cache.
 */
invalidate_dcache:
	/* FIXME */
	bx	lr

l2_cache_enable:
	mrc 15, 1, r0, cr9, cr0, 2			@ read L2 cache aux ctrl register
	bic r0, r0, #(7 << 6)
	orr r0, r0, #(2 << 6)				@ set tag RAM latency to 3 cycles
	bic r0, r0, #0xf
	orr r0, r0, #3						@ set data RAM latency to 4 cycles
	mcr 15, 1, r0, cr9, cr0, 2			@ write the L2 cache aux ctrl register

	mrc	15, 0, r0, cr1, cr0, 1
	orr	r0, r0, #2
	mcr	15, 0, r0, cr1, cr0, 1
	bx	lr

l2_cache_disable:
	mrc	15, 0, r0, cr1, cr0, 1
	bic	r0, r0, #2
	mcr	15, 0, r0, cr1, cr0, 1
	bx	lr

get_device_type:
	mov	r0, #0
	bx	lr
