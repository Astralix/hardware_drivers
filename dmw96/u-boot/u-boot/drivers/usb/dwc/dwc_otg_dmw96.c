/*
 * DSPG DMW96 DWC OTG USB platform module.
 *
 * Copyright (c) 2011 Intelligraphics, Inc.
 * Author: Stephen Smith stephen.smith@intelligraphics.com
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
 */

#include <common.h>
#include <dspg_otg_udc.h>

#include "dwc_otg_driver.h"
#include "dwc_otg_platform.h"

int dwc_otg_platform_init(void)
{
	return dwc_otg_driver_init(UDC_BASE);
}
