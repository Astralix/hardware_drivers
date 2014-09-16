/*
 * firetux led support
 *
 * (C) Copyright 2008-2009, emlix GmbH, Germany
 * Juergen Schoew <js@emlix.com>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <asm/io.h>
#include <status_led.h>
#include "firetux.h"

void firetux_set_led_VEGA_BASESTATION_IIIA(int led, int brightness)
{
	switch (led) {
	case 1:	/* GPIO A9 */
		if (brightness)
			clear_gpioa(9);
		else
			set_gpioa(9);
		break;
	case 2:	/* GPIO A31 */
		if (brightness)
			clear_gpioa(31);
		else
			set_gpioa(31);
		break;
	case 3:	/* GPIO A16 */
		if (brightness)
			clear_gpioa(16);
		else
			set_gpioa(16);
		break;
	case 4: /* GPIO A3 */
		if (brightness)
			clear_gpioa(3);
		else
			set_gpioa(3);
		break;
	default:
		break;
	}
}

void firetux_led_toggle_VEGA_BASESTATION_IIIA(led_id_t mask)
{
	switch (mask) {
	case 1:
		toggle_gpioa(9);
		break;
	case 2:
		/* GPIO A31 */
		toggle_gpioa(31);
		break;
	case 3:
		/* only Board IIIa has this led */
		toggle_gpioa(16);
		break;
	case 4:
		/* only Board IIIa has this led */
		toggle_gpioa(3);
		break;
	default:
		break;
	}
}

void firetux_set_led_VEGA_BASESTATION_IIIB(int led, int brightness)
{
	switch (led) {
	case 1:	/* GPIO A9 */
		if (brightness)
			clear_gpioa(9);
		else
			set_gpioa(9);
		break;
	case 2:	/* GPIO A0 */
		if (brightness)
			clear_gpioa(0);
		else
			set_gpioa(0);
		break;
	default:
		break;
	}
}


void firetux_led_toggle_VEGA_BASESTATION_IIIB(led_id_t mask)
{
	switch (mask) {
	case 1:
		toggle_gpioa(9);
		break;
	case 2:
		/* GPIO A0 */
		toggle_gpioa(0);
		break;
	default:
		break;
	}
}

void firetux_set_led(int led, int brightness)
{
	switch (get_boardrevision()) {
	case 0:
	case 1:
		/* unknown and EZ_MCP does not have leds */
		break;
	case 2:
		firetux_set_led_VEGA_BASESTATION_IIIA(led, brightness);
		break;
	case 3:
	case 4:
		/* VEGA_BASESTATION_IIIB and VEGA_BASESTATION_IIIC */
		firetux_set_led_VEGA_BASESTATION_IIIB(led, brightness);
		break;
	default:
		break;
	}
}

void __led_toggle(led_id_t mask)
{
	switch (get_boardrevision()) {
	case 0:
	case 1:
		/* unknown and EZ_MCP does not have leds */
		break;
	case 2:
		firetux_led_toggle_VEGA_BASESTATION_IIIA(mask);
		break;
	case 3:
	case 4:
		/* VEGA_BASESTATION_IIIB and VEGA_BASESTATION_IIIC */
		firetux_led_toggle_VEGA_BASESTATION_IIIB(mask);
		break;
	default:
		break;
	}
}

void __led_init(led_id_t mask, int state)
{
}

void __led_set(led_id_t mask, int state)
{
	firetux_set_led(mask, state);
}
