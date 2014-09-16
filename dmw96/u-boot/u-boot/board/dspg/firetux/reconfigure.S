/*
 * Memory reconfiguration for PNX8181
 *
 * (C) Copyright 2009, DSPG Technologies GmbH, Germany
 * Andreas Weissel <andreas.weissel@dspg.com>
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

/* SDI */
#define REG_SDI_RFRSH1	0xC1200008	/* Refresh parameters 1 */
#define REG_SDI_SW_CTRL	0xC1200400	/* Software control register */

/* CGU */
#define REG_CGU_SCCON	0xC2200004	/* CGU SCCON register */

.globl set_cpu_speed_internal_NOT_USED
set_cpu_speed_internal_NOT_USED:
	mov	r3, r0			/* r3 holds the target CPU speed */

	/* memcpy(0x0, set_cpu_speed_internal_start) */
	ldr	r2, set_cpu_speed_internal
	ldr	r4, set_cpu_speed_internal_end
	mov	r1, #0
copy_loop:
	ldr	r0, [r2]
	str	r0, [r1]
	add	r2, r2, #1
	add	r1, r1, #1
	cmp	r2, r4
	ble	copy_loop

	mov	lr, pc
	add	lr, #8
	mov	pc, #0
	mov	pc, lr

.globl set_cpu_speed_internal
set_cpu_speed_internal:
	mov	r3, r0			/* r3 holds the target CPU speed */

	ldr	r2, =REG_SDI_RFRSH1
	ldr	r1, [r2]
	orr	r1, #(1<<25)		/* Activate hold self refresh */ 
	str	r1, [r2]
	mov	r0, #(0x50<<8)		/* wait 200us */
1:
	subs	r0, r0, #1
	bne	1b

	/*mov	r1, #0x09*/		/* load extended mode register */

	mov	r1, #0x01		/* precharge all register */
	ldr	r2, =REG_SDI_SW_CTRL
	str	r1, [r2]

	mov	r1, #0x02		/* self refresh register */
	ldr	r2, =REG_SDI_SW_CTRL
	str	r1, [r2]

	/* CGU speed configuration */

	ldr	r2, =REG_CGU_SCCON
	ldr	r1, [r2]		/* read sccon */
	orr	r1, r1, #(1<<16)	/* switch to pll_fix */
	str	r1, [r2]

	mov	r4, r1			/* store original setting */
	bic	r1, r1, #(0x7f<<3)	/* clear nsc */
	bic	r1, r1, #(3<<10)	/* clear ksc */
	bic	r1, r1, #(0x7)		/* clear msc */
	bic	r1, r1, #(0xf<<12)	/* clear mahb */
	orr	r1, r1, #(1<<12)	/* mahb=1 hclk,pclk1=armclk/2 */

	orr	r1, r1, #(1<<20)	/* bypass output clock divider */

	cmp	r3, #208
	beq	speed_208
	cmp	r3, #194
	beq	speed_194
	cmp	r3, #180
	beq	speed_180
	cmp	r3, #165
	beq	speed_165
	cmp	r3, #104
	beq	speed_104
	cmp	r3, #97
	beq	speed_97
	cmp	r3, #90
	beq	speed_90
	cmp	r3, #83
	beq	speed_83
	mov	r1, r4			/* restore original setting */
	b	speed_ok

	/* hclk,pclk=104MHz */
speed_104:
	orr	r1, r1, #0x1		/* msc=1 */
speed_208:
	orr	r1, r1, #(14<<3)	/* nsc=14, ksc=0 */
	b	speed_ok

	/* hclk,pclk=97MHz */
speed_97:
	orr	r1, r1, #0x1		/* msc=1 */
speed_194:
	orr	r1, r1, #(13<<3)	/* nsc=13, ksc=0 */
	b	speed_ok

	/* hclk,pclk=90MHz */
speed_90:
	orr	r1, r1, #0x1		/* msc=1 */
speed_180:
	orr	r1, r1, #(12<<3)	/* nsc=12, ksc=0 */
	b	speed_ok

	/* hclk,pclk=83MHz */
speed_83:
	orr	r1, r1, #0x1		/* msc=1 */
speed_165:
	orr	r1, r1, #(11<<3)	/* nsc=11, ksc=0 */

speed_ok:
	str	r1, [r2]
	orr	r1, r1, #(1<<18)	/* enable pll_sc */
	str	r1, [r2]

	ldr	r1, [r2]		/* read sccon */
	bic	r1, r1, #(1<<16)	/* switch to pll_sc */
	str	r1, [r2]

	/* CGU speed configuration finished */

	ldr	r2, =REG_SDI_RFRSH1
	ldr	r1, [r2]
	bic	r1, #(1<<25)		/* De-activate hold self refresh */
	str	r1, [r2]
	orr	r1, #(1<<24)		/* activate auto refresh */
	str	r1, [r2]
	mov	r0, #(0x50<<8)		/* wait 200us */
1:
	subs	r0, r0, #1
	bne	1b

	mov	r1, #0x00		/* NOP command */
	ldr	r2, =REG_SDI_SW_CTRL
	str	r1, [r2]

	mov	pc, lr

.globl set_cpu_speed_internal_end
set_cpu_speed_internal_end:
