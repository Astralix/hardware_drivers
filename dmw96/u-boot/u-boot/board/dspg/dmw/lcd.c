/*
 * (C) Copyright 2009
 * Shlomi Mor, DSP Group, shlomi.mor@dspg.com
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

#include <common.h>
#include <asm/io.h>
#include <config.h>
#include <video_fb.h>
#include <lcd.h>
#include <asm/arch/gpio.h>

#include "dmw.h"
#include "pads.h"
#include "clock.h"
#include "lcd.h"
#include "amoled.h"

DECLARE_GLOBAL_DATA_PTR;

GraphicDevice gdev;

const struct dmw_lcdc_panel *dmw_panel = NULL;

static ushort colormap[256];

vidinfo_t panel_info = {
	.vl_col = 800,
	.vl_row = 480,
	.vl_bpix = LCD_COLOR16,
	.cmap = colormap,
};

void *lcd_base;			/* Start of framebuffer memory	*/
void *lcd_console_address;	/* Start of console buffer	*/

int lcd_line_length;
int lcd_color_fg;
int lcd_color_bg;

short console_col;
short console_row;

static struct dmw_lcdc_seq dmw_lcdc_hw_init_seq[] = {
	{ LCDC_REG_GCER,        0x0000 }, /* disable gamma correction */
	{ LCDC_REG_CLPER,       0x0000 }, /* disable clipping */
	{ LCDC_REG_INTMR,       0x0000 }, /* 5:6:5 rgb input */
	{ LCDC_REG_OSDMCR,      0x0000 },
	{ LCDC_REG_DISPIDXR,    0x0000 },

	{ LCDC_REG_BACKCPR,     0x0000 }, /* black background */
	{ LCDC_REG_RBCR,        0x0080 }, /* no color clipping */
	{ LCDC_REG_GBCR,        0x0080 },
	{ LCDC_REG_BBCR,        0x0080 },

	{ LCDC_REG_PALR0,       0x0000 },
	{ LCDC_REG_PALR1,       0x0000 },
	{ LCDC_REG_PALR2,       0x0000 },
	{ LCDC_REG_PALR3,       0x0000 },

	{ LCDC_REG_DISPYSPOS2R, 0x0000 }, /* only used for CCIR656 */
	{ LCDC_REG_DISPYEPOS2R, 0x0000 }, /* only used for CCIR656 */

	{ LCDC_REG_GPSELR,      0x0000 },

	{ LCDC_REG_DISPCR,      0x0001 }, /* enable display */

	{ LCDC_SEQ_END,         0x0000 },
};

static void amoled_standby(void)
{
	dmw_amoled_standby();
}

static void amoled_resume(void)
{
	dmw_amoled_resume();
}

static const struct dmw_lcdc_panel dmw_amoled =
{
	.name = "amoled",
	.xres = 480,
	.yres = 800,
	.bits_per_pixel = LCD_COLOR16,
	.clock_rate = 27000000,
	.lcdc_gp_en = 0x1ff,
	.init_fn = dmw_amoled_init,
	.standby_fn = amoled_standby,
	.resume_fn = amoled_resume,
	.init_seq = {
		/* Display panel */
		{ LCDC_REG_DISPIR,   0x0001 },
		{ LCDC_REG_PANCSR,   0x000f },

		/* Vertical Timing register */
		{ LCDC_REG_VSTR,          0 },
		{ LCDC_REG_VFTR,          4 },
		{ LCDC_REG_VETR,          4 },

		/* Horizontal Timing register*/
		{ LCDC_REG_HSTR,          1 },
		{ LCDC_REG_HFTR,          7 },
		{ LCDC_REG_HETR,          4 },

		{ LCDC_REG_YCLPCR,   0xff00 },
		{ LCDC_REG_CCLPCR,   0xff00 },

		{ LCDC_REG_ISTAR4,   0x4009 },

		{ LCDC_REG_GP3CNTR,  0x0001 }, /* GP3 set high (RESET) */

		{ LCDC_SEQ_END,      0x0000 }
	},
};

static void dmw_at080tn64_wvga_standby(void)
{
	/*TODO have to write the standby sequence*/
}

static void dmw_at080tn64_wvga_resume(void)
{
	/*TODO have to write the standby sequence*/
}

static const struct dmw_lcdc_panel dmw_at080tn64_wvga =
{
	.name = "che_wvga",
	.xres = 800,
	.yres = 480,
	.bits_per_pixel = LCD_COLOR16,
	.clock_rate = 34000000,
	.lcdc_gp_en = 0x1ff,
	.standby_fn = dmw_at080tn64_wvga_standby,
	.resume_fn = dmw_at080tn64_wvga_resume,
	.init_seq = {
		/* Display panel */
		{ LCDC_REG_DISPIR,   0x0000 },
		{ LCDC_REG_PANCSR,   0x0016 },

		/* Vertical Timing register */
		{ LCDC_REG_VSTR,          0 },
		{ LCDC_REG_VFTR,         23 },
		{ LCDC_REG_VETR,         22 },

		/* Horizontal Timing register*/
		{ LCDC_REG_HSTR,          1 },
		{ LCDC_REG_HFTR,         45 },
		{ LCDC_REG_HETR,        209 },

		{ LCDC_REG_YCLPCR,   0xff00 },
		{ LCDC_REG_CCLPCR,   0xff00 },

		{ LCDC_REG_ISTAR4,   0x4009 },

		{ LCDC_REG_GP3CNTR,  0x0001 }, /* GP3 set high (RESET) */

		{ LCDC_SEQ_END,      0x0000 }
	},
};

static void dmw_innolux_init(void)
{
	gpio_enable(GGPIO(21)); /* LCDGP2 */
	gpio_enable(CGPIO(25)); /* LCDGP6 */
	gpio_enable(CGPIO(26)); /* LCDGP7 */

	gpio_direction_output(GGPIO(21), 1); /* left-right direction */
	gpio_direction_output(CGPIO(25), 0); /* up-down direction */
	gpio_direction_output(CGPIO(26), 1); /* backlight on */
}

static void dmw_at070tn90_wvga_standby(void)
{
	/*TODO have to write the standby sequence*/
}

static void dmw_at070tn90_wvga_resume(void)
{
	/*TODO have to write the standby sequence*/
}

static const struct dmw_lcdc_panel dmw_at070tn90_wvga =
{
	.name = "innolux_at070tn90",
	.xres = 800,
	.yres = 480,
	.bits_per_pixel = LCD_COLOR16,
	.clock_rate = 37125000,
	.lcdc_gp_en = 0x1ff,
	.init_fn = dmw_innolux_init,
	.standby_fn = dmw_at070tn90_wvga_standby,
	.resume_fn = dmw_at070tn90_wvga_resume,
	.init_seq = {
		/* Display panel */
		{ LCDC_REG_DISPIR,   0x0001 },
		{ LCDC_REG_PANCSR,   0x0006 },

		/* Vertical Timing register */
		{ LCDC_REG_VSTR,          0 },
		{ LCDC_REG_VFTR,         23 },
		{ LCDC_REG_VETR,         22 },

		/* Horizontal Timing register*/
		{ LCDC_REG_HSTR,          1 },
		{ LCDC_REG_HFTR,         45 },
		{ LCDC_REG_HETR,        209 },

		{ LCDC_REG_YCLPCR,   0xff00 },
		{ LCDC_REG_CCLPCR,   0xff00 },

		{ LCDC_REG_ISTAR4,   0x4009 },

		{ LCDC_REG_GP3CNTR,  0x0001 }, /* GP3 set high (RESET) */

		{ LCDC_SEQ_END,      0x0000 }
	},
};

static void dmw_chimei_init(void)
{
	gpio_disable(CGPIO(22)); /* LCDGP3 */
	gpio_disable(CGPIO(23)); /* LCDGP4 */
	gpio_disable(CGPIO(24)); /* LCDGP5 */
	gpio_disable(CGPIO(25)); /* LCDGP6 */
	gpio_disable(CGPIO(26)); /* LCDGP7 */
	gpio_disable(CGPIO(27)); /* LCDGP8 */

	pads_enable("lcdgp3");
	pads_enable("lcdgp4");
	pads_enable("lcdgp5");
	pads_enable("lcdgp6");
	pads_enable("lcdgp7");
	pads_enable("lcdgp8");
}

static void dmw_chimei_standby(void)
{
	/*TODO have to write the standby sequence*/
}

static void dmw_chimei_resume(void)
{
	/*TODO have to write the standby sequence*/
}


static const struct dmw_lcdc_panel dmw_chimei =
{
	.name = "chimei_wvga",
	.xres = 800,
	.yres = 480,
	.bits_per_pixel = LCD_COLOR16,
	.clock_rate = 34000000,
	.lcdc_gp_en = 0x1ff,
	.init_fn = dmw_chimei_init,
	.standby_fn = dmw_chimei_standby,
	.resume_fn = dmw_chimei_resume,
	.init_seq = {
		/* data driven on falling edge */
		{ LCDC_REG_PANCSR,     0x0001 },

		/* vertical timing */
		{ LCDC_REG_VSTR,            4 },
		{ LCDC_REG_VFTR,           35 },
		{ LCDC_REG_VETR,           41 },

		/* horizontal timing */
		{ LCDC_REG_HSTR,            9 },
		{ LCDC_REG_HFTR,           65 },
		{ LCDC_REG_HETR,           73 },

		/* GP00(LD) */
		{ LCDC_REG_GP0B_H_HI,  0x037e },  /* H_ST */
		{ LCDC_REG_GP0B_V_ST,  0x0028 },  /* V_ST */
		{ LCDC_REG_GP0B_H_LO,  0x0388 },  /* H_END */
		{ LCDC_REG_GP0B_V_END, 0x0207 },  /* V_END */
		{ LCDC_REG_GP0BCNTR,   0x8050 },

		{ LCDC_REG_GP0A_H_HI,  0x0000 },
		{ LCDC_REG_GP0A_V_ST,  0x0000 },
		{ LCDC_REG_GP0A_H_LO,  0x0000 },
		{ LCDC_REG_GP0A_V_END, 0x0000 },
		{ LCDC_REG_GP0ACNTR,   0x0000 },

		/* GP01(POL) */
		{ LCDC_REG_GP1B_H_HI,  0x031f },
		{ LCDC_REG_GP1B_V_ST,  0x0028 },
		{ LCDC_REG_GP1B_H_LO,  0x037a },
		{ LCDC_REG_GP1B_V_END, 0x0208 },
		{ LCDC_REG_GP1BCNTR,   0x8030 },

		/* GP02(CKV) */
		{ LCDC_REG_GP2_H_HI,   0x01da },
		{ LCDC_REG_GP2_V_ST,   0x0000 },
		{ LCDC_REG_GP2_H_LO,   0x0388 },
		{ LCDC_REG_GP2_V_END,  0x0000 },
		{ LCDC_REG_GP2CNTR,    0xe050 },

		/* GP03(OE) */
		{ LCDC_REG_GP3_H_HI,   0x0000 },
		{ LCDC_REG_GP3_V_ST,   0x0029 },
		{ LCDC_REG_GP3_H_LO,   0x0354 },
		{ LCDC_REG_GP3_V_END,  0x0208 },
		{ LCDC_REG_GP3CNTR,    0xa050 },

		/* GP04(STH) */
		{ LCDC_REG_GP4_H_HI,   0x004a },
		{ LCDC_REG_GP4_V_ST,   0x0028 },
		{ LCDC_REG_GP4_H_LO,   0x004b },
		{ LCDC_REG_GP4_V_END,  0x0207 },
		{ LCDC_REG_GP4CNTR,    0x8050 },

		/* GP05(STH) */
		{ LCDC_REG_GP5_H_HI,   0x004a },
		{ LCDC_REG_GP5_V_ST,   0x0028 },
		{ LCDC_REG_GP5_H_LO,   0x004b },
		{ LCDC_REG_GP5_V_END,  0x0207 },
		{ LCDC_REG_GP5CNTR,    0x8050 },

		/* GP06(STV) */
		{ LCDC_REG_GP6_H_HI,   0x034c },
		{ LCDC_REG_GP6_V_ST,   0x0028 },
		{ LCDC_REG_GP6_H_LO,   0x034b },
		{ LCDC_REG_GP6_V_END,  0x0029 },
		{ LCDC_REG_GP6CNTR,    0x4050 },

		/* GP07(STV) */
		{ LCDC_REG_GP7_H_HI,   0x034c },
		{ LCDC_REG_GP7_V_ST,   0x0028 },
		{ LCDC_REG_GP7_H_LO,   0x034b },
		{ LCDC_REG_GP7_V_END,  0x002a },
		{ LCDC_REG_GP7CNTR,    0x40d0 },

		{ LCDC_REG_GP_HMAXR,   0x03b5 },
		{ LCDC_REG_GP_VMAXR,   0x0230 },

		{ LCDC_REG_GPSELR,     0x0001 },

		{ LCDC_SEQ_END,        0x0000 },
	}
};

static const struct dmw_lcdc_panel *dmw_panels[] = {
	&dmw_amoled,
	&dmw_at080tn64_wvga,
	&dmw_at070tn90_wvga,
	&dmw_chimei,
};

static inline void dmw_lcdc_writel(unsigned long addr, unsigned long val)
{
	writel(val, addr);
}

static void dmw_lcdc_run_sequence(const struct dmw_lcdc_seq *seq)
{
	struct dmw_lcdc_seq *cmd = (struct dmw_lcdc_seq *)seq;

	while (cmd->addr != LCDC_SEQ_END) {
		writel(cmd->value, cmd->addr);
		cmd++;
	}
}

static void dmw_lcdc_hw_disable(void)
{
	dmw_lcdc_writel(LCDC_REG_LCDCCR, 0);
}

static void dmw_lcdc_hw_enable(void)
{
	dmw_lcdc_writel(LCDC_REG_LCDCCR, 1);
}

static void dmw_lcdc_hw_param_update(void)
{
	dmw_lcdc_writel(LCDC_REG_PARUP, 1);
}

static void dmw_lcdc_set_clock_rate(const struct dmw_lcdc_panel *panel)
{
	if (panel->clock_rate)
		clk_set_rate("lcdc", panel->clock_rate);
}

static void dmw_lcdc_hw_init_panel(const struct dmw_lcdc_panel *panel)
{
	/* input data transfer sizes */
	dmw_lcdc_writel(LCDC_REG_INDTR, (panel->xres >> 1) - 1);
	dmw_lcdc_writel(LCDC_REG_INDXSR, panel->xres - 1);
	dmw_lcdc_writel(LCDC_REG_INDYSR, panel->yres - 1);

	dmw_lcdc_writel(LCDC_REG_VATR,   panel->yres - 1);
	dmw_lcdc_writel(LCDC_REG_HADSTR, panel->xres - 1);
	dmw_lcdc_writel(LCDC_REG_HAPWR,  panel->xres - 1);

	/* display position */
	dmw_lcdc_writel(LCDC_REG_DISPXSPOSR,  0);
	dmw_lcdc_writel(LCDC_REG_DISPXEPOSR,  panel->xres - 1);
	dmw_lcdc_writel(LCDC_REG_DISPYSPOS1R, 0);
	dmw_lcdc_writel(LCDC_REG_DISPYEPOS1R, panel->yres - 1);

	/* input buffer */
	dmw_lcdc_writel(LCDC_REG_MSBAHBA0R, ((unsigned long)lcd_base) >> 16);
	dmw_lcdc_writel(LCDC_REG_LSBAHBA0R, ((unsigned long)lcd_base) & 0xffff);
	dmw_lcdc_writel(LCDC_REG_OFFAR0, panel->xres * sizeof(unsigned short));

	dmw_lcdc_writel(LCDC_REG_MSBAHBA1R, 0);
	dmw_lcdc_writel(LCDC_REG_LSBAHBA1R, 0);
	dmw_lcdc_writel(LCDC_REG_OFFAR1,    0);

	dmw_lcdc_run_sequence(panel->init_seq);

	if (panel->init_fn)
		panel->init_fn();
}

void dmw_lcdc_init_colors(const struct dmw_lcdc_panel *panel)
{
	int i, num_words;
	unsigned long *buf = lcd_base;

	num_words = (panel->xres * panel->yres * (NBITS(panel->bits_per_pixel) / 8)) /
	            sizeof(unsigned long);

	for (i = 0; i < num_words/3; i++)
		*buf++ = 0xf800f800; /* red */

	for (i = 0; i < num_words/3; i++)
		*buf++ = 0x07e007e0; /* green */

	for (i = 0; i < num_words/3; i++)
		*buf++ = 0x001f001f; /* blue */
}

void lcd_initcolregs(void)
{
}

void lcd_setcolreg(ushort regno, ushort red, ushort green, ushort blue)
{
}

void lcd_enable(void)
{
	if (dmw_panel) {
		unsigned long image_size = dmw_panel->xres * dmw_panel->yres *
		                           (NBITS(dmw_panel->bits_per_pixel) / 8) * 2;

		memset(lcd_base, 0, image_size);

		dmw_lcdc_init_colors(dmw_panel);

		dmw_lcdc_set_clock_rate(dmw_panel);

		dmw_lcdc_hw_disable();

		dmw_lcdc_run_sequence(dmw_lcdc_hw_init_seq);

		dmw_lcdc_hw_init_panel(dmw_panel);

		dmw_lcdc_hw_enable();

		dmw_lcdc_hw_param_update();
	}
}

void lcdc_suspend_mode (unsigned int on)
{
	static unsigned int is_standby = 0;

	if (on && !is_standby) {
		dmw_panel->standby_fn();
		clk_disable("lcdc");
		is_standby = 1;
	} else if (!on && is_standby){
		clk_enable("lcdc");
		dmw_panel->resume_fn();
		is_standby = 0;
	}

}

void lcd_ctrl_init(void *lcdbase)
{
	int i;
	char *lcdc_type;

	if (!lcdbase)
		return;

	lcdc_type = getenv("lcdctype");
	if (!lcdc_type)
		return;

	for (i = 0; i < ARRAY_SIZE(dmw_panels); i++) {
		if (!strcmp(lcdc_type, dmw_panels[i]->name)) {
			dmw_panel = dmw_panels[i];

			panel_info.vl_col  = dmw_panel->xres;
			panel_info.vl_row  = dmw_panel->yres;
			panel_info.vl_bpix = dmw_panel->bits_per_pixel;

			lcd_line_length = (panel_info.vl_col * NBITS(panel_info.vl_bpix)) / 8;

			return;
		}
	}
}

ulong calc_fbsize(void)
{
	return ((panel_info.vl_col * panel_info.vl_row *
		NBITS(panel_info.vl_bpix)) / 8) + PAGE_SIZE;
}

void *video_hw_init(void)
{
	if (dmw_panel) {
		printf("LCD:   %s\n", dmw_panel->name);

		gdev.frameAdrs = (unsigned long)lcd_base;
		gdev.winSizeX = dmw_panel->xres;
		gdev.winSizeY = dmw_panel->yres;
		gdev.gdfBytesPP = NBITS(dmw_panel->bits_per_pixel) / 8;
		gdev.gdfIndex = GDF_16BIT_565RGB;

		return (void *)&gdev;
	}

	return NULL;
}

void video_set_lut(unsigned int index, /* color number */
                   unsigned char r,    /* red */
                   unsigned char g,    /* green */
                   unsigned char b)    /* blue */
{
	/* nothing to do here */
}
