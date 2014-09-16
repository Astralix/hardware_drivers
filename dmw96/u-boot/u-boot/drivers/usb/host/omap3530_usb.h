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

#ifndef __OMAP3_MUSB_H__
#define __OMAP3_MUSB_H__

#include "../musb/musb_core.h"

/* Base address of OMAP3 usb0 wrapper */
#define OMAP3_USB0_BASE 0x480AB400
/* Base address of OMAP3 musb core */
#define MENTOR_USB0_BASE 0x480AB000
/* Timeout for OMAP3 USB module */
#define OMAP3_USB_TIMEOUT 0x3FFFFFF

/*
 * OMAP3 platform USB register overlay.
 */
struct omap3_usb_regs {
	u32 revision;
	u32 sysconfig;
	u32 sysstatus;
	u32 interfsel;
	u32 simenable;
	u32 forcestdby;
};

#endif	/* __OMAP3_MUSB_H__ */
