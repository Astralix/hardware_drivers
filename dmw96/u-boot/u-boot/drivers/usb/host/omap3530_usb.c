/*
 * TI's OMAP3 platform specific USB functions.
 *
 * Copyright (c) 2009 Texas Instruments
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
 *
 * Author: Thomas Abraham t-abraham@ti.com, Texas Instruments
 */

#include <common.h>

#include "omap3530_usb.h"
#include <asm/arch/cpu.h>
#include <asm/arch/sys_proto.h>
#include <asm/io.h>

/* MUSB platform configuration */
struct musb_config musb_cfg = {
	(struct	musb_regs *)MENTOR_USB0_BASE,
	OMAP3_USB_TIMEOUT,
	0
};

/* MUSB module register overlay */
struct omap3_usb_regs *regs;

/*
 * This function performs OMAP3 platform specific initialization for Mentor
 * USB OTG controller.
 */
int musb_platform_init(void)
{
	regs = (struct omap3_usb_regs *)OMAP3_USB0_BASE;
	/* Disable force standby */
	sr32((void *)&regs->forcestdby, 0 , 1 , 0);
	/* Set configuration for clock and interface clock to be always on */
	writel(0x1008, &regs->sysconfig);
	/* Configure the PHY as PHY interface is 12-pin, 8-bit SDR ULPI */
	sr32((void *)&regs->interfsel, 0, 1, 1);
	return 0;
}

/*
 * This function performs OMAP3 platform specific deinitialization for
 * Mentor USB OTG controller.
 */
void musb_platform_deinit(void)
{
	/* MUSB soft-reset */
	writel(2, &regs->sysconfig);
}
