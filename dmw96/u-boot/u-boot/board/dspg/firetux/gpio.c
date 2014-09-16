/*
 * firetux gpio driver
 *
 * (C) Copyright 2008-2009, emlix GmbH, Germany
 * Juergen Schoew <js@emlix.com>
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

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <asm/io.h>
#include "firetux.h"

/* this library only supports GPIO A, because only this port is linear */

unsigned char get_gpioa(unsigned char gpionr)
{
	if ((gpionr < 0) || (gpionr > 31))
		return -1;
	return (readl((void *)PNX8181_GPIOA_PINS) >> gpionr) & 1;
}

void set_gpioa(unsigned char gpionr)
{
	if ((gpionr < 0) || (gpionr > 31))
		return;
	writel((readl((void *)PNX8181_GPIOA_OUTPUT) | (1 << gpionr)),
					(void *)PNX8181_GPIOA_OUTPUT);
}

void clear_gpioa(unsigned char gpionr)
{
	if ((gpionr < 0) || (gpionr > 31))
		return;
	writel((readl((void *)PNX8181_GPIOA_OUTPUT) & ~(1 << gpionr)),
					(void *)PNX8181_GPIOA_OUTPUT);
}

void toggle_gpioa(unsigned char gpionr)
{
	if ((gpionr < 0) || (gpionr > 31))
		return;
	if (get_gpioa(gpionr))
		clear_gpioa(gpionr);
	else
		set_gpioa(gpionr);
}

void set_direction_gpioa(unsigned char gpionr, unsigned char dir)
{
	if ((gpionr < 0) || (gpionr > 31))
		return;
	if (dir)
		writel(readl((void *)PNX8181_GPIOA_DIRECTION) | (1 << gpionr),
					(void *)PNX8181_GPIOA_DIRECTION);
	else
		writel(readl((void *)PNX8181_GPIOA_DIRECTION) & ~(1 << gpionr),
					(void *)PNX8181_GPIOA_DIRECTION);
}

void set_syspad_gpioa(unsigned char gpionr, unsigned char pad)
{
	if ((gpionr < 0) || (gpionr > 31))
		return;
	pad &= 0x3;
	if (gpionr < 16) {
		writel((readl((void *)(PNX8181_SCON_BASE + PNX8181_SYSPAD0))
				& ~(0x3 << (2 * gpionr)))
				| (pad << (2 * gpionr)),
				(void *)(PNX8181_SCON_BASE + PNX8181_SYSPAD0));
	} else {
		gpionr -= 16;
		writel((readl((void *)(PNX8181_SCON_BASE + PNX8181_SYSPAD1))
				& ~(0x3 << (2 * gpionr)))
				| (pad << (2 * gpionr)),
				(void *)(PNX8181_SCON_BASE + PNX8181_SYSPAD1));
	}
}

void set_sysmux_gpioa(unsigned char gpionr, unsigned char mux)
{
	if ((gpionr < 0) || (gpionr > 31))
		return;
	mux &= 0x3;
	if (gpionr < 16) {
		writel((readl((void *)(PNX8181_SCON_BASE + PNX8181_SYSMUX0))
				& ~(0x3 << (2 * gpionr)))
				| (mux << (2 * gpionr)),
				(void *)(PNX8181_SCON_BASE + PNX8181_SYSMUX0));
	} else {
		gpionr -= 16;
		writel((readl((void *)(PNX8181_SCON_BASE + PNX8181_SYSMUX1))
				& ~(0x3 << (2 * gpionr)))
				| (mux << (2 * gpionr)),
				(void *)(PNX8181_SCON_BASE + PNX8181_SYSMUX1));
	}
}

void config_gpioa(unsigned char gpionr, unsigned char mux, unsigned char dir,
					unsigned char pad, unsigned char value)
{
	if ((gpionr < 0) || (gpionr > 31))
		return;

	if (mux >= 0)
		set_sysmux_gpioa(gpionr, mux);

	if (dir >= 0)
		set_direction_gpioa(gpionr, dir);

	if (pad >= 0)
		set_syspad_gpioa(gpionr, pad);

	if ((dir == GPIOA_DIR_OUTPUT) && (value >= 0)) {
		if (value)
			set_gpioa(gpionr);
		else
			clear_gpioa(gpionr);
	}
}
