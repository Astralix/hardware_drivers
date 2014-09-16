/*
 * bootmenu.c - Boot menu
 *
 * Copyright (C) 2006-2007 by OpenMoko, Inc.
 * Written by Werner Almesberger <werner@openmoko.org>
 * All Rights Reserved
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */


#include <common.h>
#include <environment.h>
#include <bootmenu.h>
#include <asm/atomic.h>

#if 0
#ifdef CONFIG_USBD_DFU
#include "usbdcore.h"
#include "usb_dfu.h"
#endif
#endif

#include "dw.h"
#include "keypad.h"

#define DEBOUNCE_LOOPS			2	/* wild guess */

#define NEXT_KEY_ACTION_STRING		"Press [SWK1]"
#define ENTER_KEY_ACTION_STRING		"[SWK2]"

#define MENU_KEYPAD_TEXT_SIZE		20

static unsigned long time_base = 0;
static int menu_enter_keypad_row = 0;
static int menu_enter_keypad_col = 0;
static int menu_next_keypad_row = 0;
static int menu_next_keypad_col = 0;
static char menu_next_keypad_text[MENU_KEYPAD_TEXT_SIZE];
static char menu_enter_keypad_text[MENU_KEYPAD_TEXT_SIZE];
static unsigned int enter_bootmenu = 0;

static int debounce(int (*fn)(void), int *last)
{
	int on, i;

again:
	on = fn();
	if (on != *last)
		for (i = DEBOUNCE_LOOPS; i; i--)
			if (on != fn())
				goto again;
	*last = on;
	return on;
}

static int dw_keypad_next_key(void)
{
	return dw_keypad_scan(menu_next_keypad_row, menu_next_keypad_col);
}

static int dw_keypad_enter_key(void)
{
	return dw_keypad_scan(menu_enter_keypad_row, menu_enter_keypad_col);
}

static int aux_key(void *user)
{
	static int last_aux = -1;

	return debounce(dw_keypad_next_key, &last_aux);
}


static int on_key(void *user)
{
	static int last_on = -1;

	return debounce(dw_keypad_enter_key, &last_on);
}

static int no_key(void *user)
{
	return 0;
}

/* return a 1 every new second */
static int seconds(void *user)
{
	unsigned long time = get_timer(time_base);
	if (time >= 1) {
		time_base = time;
		return 1;
	}
	return 0;
}


static void poweroff_if_idle(void *user)
{
	/* TODO: no power off in idle state yet */
}


/* "bootmenu_setup" is extern, so platform can tweak it */

struct bootmenu_setup bootmenu_setup = {
	.next_key = aux_key,
	.enter_key = on_key,
	.seconds = seconds,
	.idle_action = poweroff_if_idle,
	.next_key_action = NEXT_KEY_ACTION_STRING,
	.enter_key_name = ENTER_KEY_ACTION_STRING,
};

extern void bootmenu_hook(int activity);

void dw_bootmenu_hook(int activity)
{
	if (enter_bootmenu == 1) {
		bootmenu_hook(activity);
	}
}

void dw_bootmenu(void)
{
        char *tmp = NULL;

	tmp = getenv("bootmenu_on");
	enter_bootmenu = tmp ? simple_strtoul(tmp, NULL, 10) : 0;
	if (enter_bootmenu != 1) {
		/* no bootmenu */
		return;
	}

	/* init time base */
	time_base = get_timer(0);

	tmp = getenv("menu_enter_keypad_row");
	menu_enter_keypad_row = tmp ? simple_strtoul(tmp, NULL, 10) : -1;
	tmp = getenv("menu_enter_keypad_col");
	menu_enter_keypad_col = tmp ? simple_strtoul(tmp, NULL, 10) : -1;

	tmp = getenv("menu_next_keypad_row");
	menu_next_keypad_row = tmp ? simple_strtoul(tmp, NULL, 10) : -1;
	tmp = getenv("menu_next_keypad_col");
	menu_next_keypad_col = tmp ? simple_strtoul(tmp, NULL, 10) : -1;

	tmp = getenv("menu_next_key_text");
	if (tmp != NULL) {
		strncpy(menu_next_keypad_text, tmp, min(MENU_KEYPAD_TEXT_SIZE, strlen(tmp)));
		bootmenu_setup.next_key_action = &menu_next_keypad_text[0];
	}

	tmp = getenv("menu_enter_key_text");
	if (tmp != NULL) {
		strncpy(menu_enter_keypad_text, tmp, min(MENU_KEYPAD_TEXT_SIZE, strlen(tmp)));
		bootmenu_setup.enter_key_name = &menu_enter_keypad_text[0];
	}

	if ((menu_next_keypad_row == -1) || (menu_next_keypad_col == -1)) {
		bootmenu_setup.next_key_action = "Press [no key defined]";
		bootmenu_setup.next_key = NULL;
	}

	if ((menu_next_keypad_row == -1) || (menu_next_keypad_col == -1)) {
		bootmenu_setup.next_key_action = "Press [no key defined]";
		bootmenu_setup.next_key = no_key;
	}

	if ((menu_enter_keypad_row == -1) || (menu_enter_keypad_col == -1)) {
		bootmenu_setup.enter_key_name = "[no key defined]";
		bootmenu_setup.enter_key = no_key;
	}

	bootmenu_add("Boot Linux", NULL,            "boot");
	bootmenu_add("Set console to USB", NULL,    "setenv stdin=usbtty; setenv stdout=usbtty; setenv stderr=usbtty");
	bootmenu_add("Set console to serial", NULL, "setenv stdin=serial; setenv stdout=serial; setenv stderr=serial");
	bootmenu_add("Reboot", NULL,                "reset");
	bootmenu_add("Power off", NULL,             "reset");
	bootmenu_add("Factory reset", NULL,         "defenv; saveenv");
	bootmenu_init(&bootmenu_setup);
	bootmenu();
}
