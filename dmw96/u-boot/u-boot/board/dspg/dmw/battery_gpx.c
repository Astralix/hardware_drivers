/*
 * (C) Copyright 2012
 * DSP Group
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
	Raster graphics
	Upper left corner of screen is (0,0)

 */
#include <asm/io.h>
#include <common.h>
#include <lcd.h>
#include "lcd.h"

extern void *lcd_base;
extern const struct dmw_lcdc_panel *dmw_panel;


#define PIXEL			unsigned short
#define COLOR			PIXEL

#define _R(r)			(r & 0x00f8)
#define _G(g)			(g & 0x007e)
#define _B(b)			(b & 0x001f)
#define RGB(r,g,b)		( (_R(r)<<11) | (_G(g)<<5) | _B(b) )
#define RED				RGB(0xf8, 0, 0)
#define GREEN			RGB(0, 0x7e, 0)
#define BLUE			RGB(0, 0, 0x1f)
#define YELLOW			RGB(0xf8, 0x2e, 0)
#define BLACK			RGB(0, 0, 0)
#define GRAY			RGB(0x7e, 0x7e, 0x7e)

#define BAT_WIDTH_SCALE		2
#define BAT_HEIGHT_SCALE	2
#define FRAME_PEN_SIZE		10

struct pen {
	COLOR color;
	unsigned int size;
};

static int line_len(void)
{ 
	return dmw_panel->xres * (NBITS(dmw_panel->bits_per_pixel) / 8) / sizeof(PIXEL);
}

static PIXEL *find_pixel(int x, int y)
{
	PIXEL *buf = lcd_base;
	int pos;

	pos = y * line_len(); 
	pos +=  x * (NBITS(dmw_panel->bits_per_pixel) / 8) / sizeof(PIXEL) ;
	return &buf[pos];
}

static void paint_hline(int x, int y, int length, const struct pen *pen)
{
	int i, width;
	PIXEL *buf;

	for (width = 0; width < pen->size; width++) {
		buf = find_pixel(x, y+width);
		for (i = 0; i < length; i++)
			*buf++ = pen->color;
	}
}

static void paint_vline(int x, int y, int height, const struct pen *pen)
{
	int i, width;
	PIXEL *buf;

	for (width = 0; width < pen->size; width++) {
		buf = find_pixel(x+width, y);
		for (i = 0; i < height; i++) {
			buf += line_len();
			*buf = pen->color;
		}
	}
}

/*
	(x1, y1) = top left corner
	(x2, y2) = bottom right corner
	x2 > x1
	y2 > y1
 */
static void paint_rect(int x1, int y1, int x2, int y2, const struct pen *pen)
{
	paint_hline(x1, y1, x2-x1, pen);
	paint_hline(x1, y2-pen->size+1, x2-x1, pen);
	paint_vline(x1, y1, y2-y1, pen);
	paint_vline(x2-pen->size+1, y1, y2-y1, pen);
}

/*
	(x1, y1) = top left corner
	(x2, y2) = bottom right corner
	x2 > x1
	y2 > y1
 */
static void fill_rect(int x1, int y1, int x2, int y2, const struct pen *pen)
{
	struct pen pen2 = { 
		.color = pen->color,
		.size = y2-y1
	};

	paint_hline(x1, y1, x2-x1, &pen2);
}

static void fill_screen(const struct pen *pen)
{
	fill_rect(0, 0, dmw_panel->xres, dmw_panel->yres, pen);
}


static void paint_battery_frame(const struct pen *pen)
{
	int frame_width = (dmw_panel->xres >> BAT_WIDTH_SCALE);
	int frame_height = dmw_panel->yres >> BAT_HEIGHT_SCALE;
	int start_x = (dmw_panel->xres - frame_width) / 2;
	int start_y = (dmw_panel->yres - frame_height) / 2;
	int top_width, top_height;

	/* Draw battery frame */
	paint_rect(start_x, start_y, start_x+frame_width, start_y+frame_height, pen);

	/* Draw battery top */
	top_height = frame_height / 8;
	top_width = frame_width / 2;
	start_x += ((frame_width - top_width)/2) ;
	fill_rect(start_x, start_y-top_height+1, start_x+top_width , start_y, pen);
}

static void paint_battery_capacity(int capacity, const struct pen *cap_pen, const struct pen *frame_pen)
{
	int frame_width = (dmw_panel->xres >> BAT_WIDTH_SCALE);
	int frame_height = dmw_panel->yres >> BAT_HEIGHT_SCALE;

	int width = frame_width - (2 * frame_pen->size) + 1;
	int height = frame_height - (2 * frame_pen->size) + 1;
	int start_x = (dmw_panel->xres - frame_width) / 2 + frame_pen->size;
	int start_y = (dmw_panel->yres - height) / 2 +1;

	capacity = capacity * ((double)height/100);
	start_y+=(height-capacity);
	fill_rect(start_x, start_y, start_x+width, start_y+capacity, cap_pen);
}

/* 
	Battery graphics

	Charging: 
		frame: BLUE	(capacity>0%), YELLOW (capacity<=0%)
		capacity: GREEN (capacity>20%), YELLOW (capacity>10%), RED (capacity<=10%)
	Not charging:
		frame: BLUE	(capacity>0%), YELLOW (capacity<=0%)
		capacity: GRAY
 */

void dmw_bat_paint(int capacity, int charging)
{
	struct pen cap_pen, frame_pen;
	frame_pen.color = BLACK,
	fill_screen(&frame_pen);

	frame_pen.size = FRAME_PEN_SIZE;
	frame_pen.color = capacity > 0 ? BLUE : YELLOW;
	paint_battery_frame(&frame_pen);

	if (capacity <= 0)
		return;

	cap_pen.size = 1;
	cap_pen.color = capacity > 10 ? YELLOW : RED;
	cap_pen.color = capacity > 20 ? GREEN : cap_pen.color;
	cap_pen.color = charging ? cap_pen.color : GRAY;
	paint_battery_capacity(capacity, &cap_pen, &frame_pen);
}

/* 
	Thermal condition reached
		frame: RED
		capacity: GRAY	
 */
void dmw_bat_paint_thermal(int capacity) 
{
	struct pen cap_pen, frame_pen;
	frame_pen.color = BLACK,
	fill_screen(&frame_pen);

	frame_pen.size = FRAME_PEN_SIZE;
	frame_pen.color = RED;
	paint_battery_frame(&frame_pen);
	if (capacity <= 0)    
		return;

	cap_pen.size = 1;
	cap_pen.color = GRAY;
	paint_battery_capacity(capacity, &cap_pen, &frame_pen);
}

/* 
	Error condition
		frame: GRAY	
 */
void dmw_bat_paint_error(void) 
{
	struct pen frame_pen;
	frame_pen.color = BLACK,
	fill_screen(&frame_pen);

	frame_pen.size = FRAME_PEN_SIZE;
	frame_pen.color = GRAY;
	paint_battery_frame(&frame_pen);
}

/* 
	No battery condition
		frame: thin GRAY	
 */
void dmw_bat_paint_nobat(void)
{
	struct pen cap_pen, frame_pen;
	frame_pen.color = BLACK,
	fill_screen(&frame_pen);

	frame_pen.size = 2;
	frame_pen.color = GRAY;
	paint_battery_frame(&frame_pen);
}

static int do_bat_gpx(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	switch (argc) {
		case 2:
			if (strcmp(argv[1], "error") == 0) {
				dmw_bat_paint_error();
				return 0;
			}
			else if (strcmp(argv[1], "no_bat") == 0) {
				dmw_bat_paint_nobat();
				return 0;
			}
			break;
		case 3:
			if (strcmp(argv[1], "charge") == 0) {
				int capacity = simple_strtol(argv[2], NULL, 10);
				dmw_bat_paint(capacity, 1);
				return 0;
			}
			else if (strcmp(argv[1], "no_charge") == 0) {
				int capacity = simple_strtol(argv[2], NULL, 10);
				dmw_bat_paint(capacity, 0);
				return 0;
			} else if (strcmp(argv[1], "thermal") == 0) {
				int capacity = simple_strtol(argv[2], NULL, 10);
				dmw_bat_paint_thermal(capacity);
				return 0;
			}
			break;
		default:
			break;
	}

usage:
	cmd_usage(cmdtp);
	return 1;
}


U_BOOT_CMD(
	bat_gpx_test, 32, 0, do_bat_gpx, "Test battery graphics",
	"\n"
	"bat_gpx_test error\n"
	"    - paint general battery error condition\n"
	"bat_gpx_test no_bat\n"
	"    - paint no-battery condition\n"
	"bat_gpx_test thermal\n"
	"    - paint battery thermal condition error\n"
	"      <capacity> ::= integer in the range of -1 to 100\n"
	"bat_gpx_test charge <capacity>\n"
	"    - paint battery when charge is <capacity>\n"
	"      <capacity> ::= integer in the range of -1 to 100\n"
	"bat_gpx_test no_charge <capacity>\n"
	"    - paint battery when charging is stopped (no error)\n"
	"      <capacity> ::= integer in the range of -1 to 100\n"
);
