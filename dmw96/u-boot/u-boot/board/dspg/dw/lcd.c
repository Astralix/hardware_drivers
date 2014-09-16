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
#include <nand.h>
#include <asm/io.h>
#include <config.h>
#include <jffs2/jffs2.h>
#include <video_fb.h>

#include "dw.h"
#include "clock.h"
#include "gpio.h"
#include "lcd.h"
#include "lcd_bmp.h"
//#include "tuxdspg_16.h"

//
// Extern functions
//
extern int dw_copy_nand_to_ram(nand_info_t *nand, ulong buffer, ulong off, ulong size);
extern int mtdparts_init( void );
extern int find_dev_and_part(const char *id, struct mtd_device **dev, u8 *part_num, struct part_info **part);

//
// Forward functions declerations
//
static void s1d13742_fb_do_hw_sequence(dw_lcd_controller *c);
static void lr38825_fb_do_hw_sequence(dw_lcd_controller *c);
static void solomon1962_do_hw_sequence(dw_lcd_controller *c);
static void lcd_ctlr_fillscreen(dw_lcd_controller *c);

//
// Defines
//
#define LCD_CS(cs)		((cs) << 17)
#define BMP_PARTITION_NAME	"ubootlogo"

//
// Global variables
//
static int		g_is_first_pixel;
static unsigned long	image_buffer[2*800*480*2/sizeof(long)] __attribute__ ((aligned (256)));;	// make sure the array is aligned for 32 bytes
static unsigned char	bmp_cache[4*1024];			// 4KB for bmp reading cache
static unsigned int	bmp_cache_index;
static unsigned int	bmp_file_index;

//
// Functions
//

/* Write long to dma registers */
static inline int writel_dma(int data, u8 offset)
{
	return writel(data, IO_ADDRESS(DW_DMA_BASE+offset));
}

/* Read long from dma registers */
static inline int readl_dma(u8 offset)
{
	return readl(IO_ADDRESS(DW_DMA_BASE+offset));
}

/* Write long  to fifo registers */
static inline int writel_fifo(int data, u8 offset)
{
	return writel(data, IO_ADDRESS(DW_FIFO_BASE+offset));
}

/* Read long from fifo registers */
static inline int readl_fifo(u8 offset)
{
	return readl(IO_ADDRESS(DW_FIFO_BASE+offset));
}

/* Write long to lcdif registers */
static inline int writel_lcdif(int data, u8 offset)
{
	return writel(data, IO_ADDRESS(DW_NAND_LCD_BASE+offset));
}

/* Read long from lcdif registers */
static inline int readl_lcdif(u8 offset)
{
	return readl(IO_ADDRESS(DW_NAND_LCD_BASE+offset));
}

int EPS_READ_REG(dw_lcd_controller *c, unsigned short reg)
{
	int val;

	writel(0x100241 | LCD_CS(c->cs), IO_ADDRESS(DW_LCD_BASE) + DW_FC_SEQUENCE);
	writel(0x0,      IO_ADDRESS(DW_LCD_BASE) + DW_FC_ADDR_COL);
	writel(reg,      IO_ADDRESS(DW_LCD_BASE) + DW_FC_CMD);
	writel(0x2,      IO_ADDRESS(DW_LCD_BASE) + DW_FC_DCOUNT);
	writel(0x0,      IO_ADDRESS(DW_LCD_BASE) + DW_FC_FBYP_CTL);
	writel(0x401,    IO_ADDRESS(DW_LCD_BASE) + DW_FC_CTL);

	// ????
	mdelay(1);

	while (readl(IO_ADDRESS(DW_LCD_BASE) + DW_FC_FBYP_CTL) & 0x1);

	val = readl(IO_ADDRESS(DW_LCD_BASE) + DW_FC_FBYP_DATA);
	val >>= 16;

	return val & 0x00FF;
}

void EPS_WRITE_REG(dw_lcd_controller *c, unsigned short data, unsigned short reg)
{
	writel(0x100043 | LCD_CS(c->cs), IO_ADDRESS(DW_LCD_BASE) + DW_FC_SEQUENCE);
	writel(data, IO_ADDRESS(DW_LCD_BASE) + DW_FC_ADDR_COL);
	writel(reg, IO_ADDRESS(DW_LCD_BASE) + DW_FC_CMD);
	writel(0x0, IO_ADDRESS(DW_LCD_BASE) + DW_FC_DCOUNT);
	writel(0x0, IO_ADDRESS(DW_LCD_BASE) + DW_FC_FBYP_CTL);

	// Start the transaction
	writel(0x401, IO_ADDRESS(DW_LCD_BASE) + DW_FC_CTL);

	// ?????
	udelay(1);

	while (readl(IO_ADDRESS(DW_LCD_BASE) + DW_FC_FBYP_CTL) & 0x1);
}

void EPS_START_WRITE_PIXELS(dw_lcd_controller *c)
{
	writel(0x100601 | LCD_CS(c->cs), IO_ADDRESS(DW_LCD_BASE) + DW_FC_SEQUENCE);
	writel(0x48, IO_ADDRESS(DW_LCD_BASE) + DW_FC_CMD);
	writel(0x2, IO_ADDRESS(DW_LCD_BASE) + DW_FC_DCOUNT);
	writel(0x2, IO_ADDRESS(DW_LCD_BASE) + DW_FC_FBYP_CTL);
	g_is_first_pixel = 1;
}

void EPS_STOP_WRITE_PIXELS(dw_lcd_controller *c)
{
	if ( !g_is_first_pixel ) {
		// Wait 
		while (readl(IO_ADDRESS(DW_LCD_BASE) +DW_FC_FBYP_CTL) & 0x1);
	}
}

inline void EPS_WRITE_PIXEL(dw_lcd_controller *c, unsigned short data)
{
	if ( g_is_first_pixel ) {
		g_is_first_pixel = 0;
	}
	else {
		while (readl(IO_ADDRESS(DW_LCD_BASE) +DW_FC_FBYP_CTL) & 0x1);
	}

	writel(data, IO_ADDRESS(DW_LCD_BASE) + DW_FC_FBYP_DATA);
	writel(0x401, IO_ADDRESS(DW_LCD_BASE) + DW_FC_CTL);

	// Start the writing of the next 2 bytes
	writel(0x6, IO_ADDRESS(DW_LCD_BASE) + DW_FC_FBYP_CTL);
}

//#define NEW_SSD_CODE
/* Write byte to SOLOMON registers */
static int writeb_ssd(int bytes, unsigned char *data, u8 offset, u32 usec)
{
	int i;

	//issue address
	writel_lcdif(SSD1962FB_LCDIF_WAIT, DW_FC_WAIT);
	writel_lcdif(SSD1962FB_LCDIF_PULSETIME, DW_FC_PULSETIME);
	writel_lcdif(SSD1962FB_WRITE_OFFSET_SEQ | SSD1962FB_CS, DW_FC_SEQUENCE);
	writel_lcdif(offset, DW_FC_CMD);
	writel_lcdif(0x0, DW_FC_DCOUNT);
	writel_lcdif(0x0, DW_FC_FBYP_CTL);

#ifdef NEW_SSD_CODE
	writel_lcdif(-1, DW_FC_STS_CLR);
#endif
	writel_lcdif(SSD1962FB_LCDIF_MEM_TRANS, DW_FC_CTL);

#ifdef NEW_SSD_CODE
	while( (readl_lcdif(DW_FC_STATUS) & DW_FC_STATUS_TRANS_DONE) == 0);
#else
	while( readl_lcdif(DW_FC_STATUS) & DW_FC_STATUS_TRANS_BUSY )
		udelay(usec);
#endif
	//write data
	for(i=0; i<bytes; i++) {
		writel_lcdif(SSD1962FB_WRITE_BYTE_SEQ | SSD1962FB_CS, DW_FC_SEQUENCE);
		writel_lcdif(data[i], DW_FC_ADDR_COL);
		writel_lcdif(0x0, DW_FC_DCOUNT);
		writel_lcdif(0x0, DW_FC_FBYP_CTL);
	
#ifdef NEW_SSD_CODE
		writel_lcdif(-1, DW_FC_STS_CLR);
#endif
		writel_lcdif(SSD1962FB_LCDIF_MEM_TRANS, DW_FC_CTL);

#ifdef NEW_SSD_CODE
		while( (readl_lcdif(DW_FC_STATUS) & DW_FC_STATUS_TRANS_DONE) == 0);
#else
		while( readl_lcdif(DW_FC_STATUS) & DW_FC_STATUS_TRANS_BUSY )
			udelay(usec);
#endif
	}

	return (i==0 ? i : (i-1));
}

/* Read byte from SOLOMON registers */
static int readb_ssd(int bytes, unsigned char *data, u8 offset, u32 usec)
{
	int i = 0;
	unsigned int data32;	

	//issue address
	writel_lcdif(SSD1962FB_LCDIF_WAIT, DW_FC_WAIT);
	writel_lcdif(SSD1962FB_LCDIF_PULSETIME, DW_FC_PULSETIME);
	writel_lcdif(SSD1962FB_WRITE_OFFSET_SEQ | SSD1962FB_CS, DW_FC_SEQUENCE);
	writel_lcdif(offset, DW_FC_CMD);
	writel_lcdif(0x0, DW_FC_DCOUNT);
	writel_lcdif(0x0, DW_FC_FBYP_CTL);

#ifdef NEW_SSD_CODE
	writel_lcdif(-1, DW_FC_STS_CLR);
#endif
	writel_lcdif(SSD1962FB_LCDIF_MEM_TRANS, DW_FC_CTL);
#ifdef NEW_SSD_CODE
	while( (readl_lcdif(DW_FC_STATUS) & DW_FC_STATUS_TRANS_DONE) == 0);
#else
	while( readl_lcdif(DW_FC_STATUS) & DW_FC_STATUS_TRANS_BUSY )
		udelay(usec);
#endif

	//read data
	for(i=0; i<bytes; i++) {
		writel_lcdif(SSD1962FB_READ_BYTE_SEQ | SSD1962FB_CS, DW_FC_SEQUENCE);
		writel_lcdif(0x0, DW_FC_FBYP_CTL);

#ifdef NEW_SSD_CODE
		writel_lcdif(-1, DW_FC_STS_CLR);
#endif
		writel_lcdif(SSD1962FB_LCDIF_MEM_TRANS, DW_FC_CTL);
#ifdef NEW_SSD_CODE
		while( (readl_lcdif(DW_FC_STATUS) & DW_FC_STATUS_TRANS_DONE) == 0);
#else
		while( readl_lcdif(DW_FC_STATUS) & DW_FC_STATUS_TRANS_BUSY )
			udelay(usec);
#endif

		data32 = readl_lcdif(DW_FC_FBYP_DATA);
		data[i] = data32 >> 16;
	}

	return (i==0 ? i : (i-1));
}

static void ssd1962fb_setupfifo(void)
{
	writel_fifo(0x1, DW_FIFO_RST); //reset fifo pointers and flags
	writel_fifo(0x0, DW_FIFO_IER); //disable fifo interrupts
	writel_fifo(0x3, DW_FIFO_ICR); //clear overrun, underrun interrupts
	writel_fifo(0x0, DW_FIFO_CTL); //disable
	writel_fifo(0x8, DW_FIFO_AE_LEVEL); //almost empty level
	writel_fifo(0x8, DW_FIFO_AF_LEVEL); //almost full level
}

static void ssd1962fb_setupdma(	unsigned long data_addr, unsigned long num_bytes )
{
	writel_dma(0x0, DW_DMA_IER); //disable dma interrupts
	writel_dma(0x3, DW_DMA_ICR); //clear error, done interrupts
	writel_dma(0x0, DW_DMA_CTL); //disable 
	writel_dma((int)data_addr, DW_DMA_ADDR1);
	writel_dma(0x8, DW_DMA_BURST); //dma burst size
	writel_dma((num_bytes/4), DW_DMA_BLK_LEN); //length
	writel_dma(SSD1962FB_DMA_BLK_CNT, DW_DMA_BLK_CNT); //number of blocks
}

int lcd_ctlr_detect(dw_lcd_controller *c)
{
	unsigned long id;

	if (strcmp(c->name, "Solomon 1962") == 0) {
		/* ID of Solomon is read differently than at other controllers */		
		unsigned char val[5];

#ifndef CONFIG_MUSB
		//Tear-effect signal
		gpio_direction_input(BGPIO(20));
		gpio_enable(BGPIO(20));
#endif

		readb_ssd(1, &val[0], 0xa1, 10*1000);	// Supplier high byte (0x01)
		readb_ssd(1, &val[1], 0xa8, 10*1000);	// Supplier low byte (0x57)
		readb_ssd(1, &val[2], 0xa8, 10*1000);	// Product ID (0x61)
		readb_ssd(1, &val[3], 0xa8, 10*1000);	// Revision code
		readb_ssd(1, &val[4], 0xa8, 10*1000);	// Exit code (0xFF)

		return ( (val[0] == 0x01) && (val[1] == 0x57) && (val[2] == 0x61) && (val[4] == 0xff) );
	}
	else {
		/* read ID number from controller */
		writel(TIMINGS, IO_ADDRESS(DW_LCD_BASE) + DW_FC_PULSETIME);

		writel(0x100241 | LCD_CS(c->cs), IO_ADDRESS(DW_LCD_BASE) + DW_FC_SEQUENCE); //READ

		writel(0x0,     IO_ADDRESS(DW_LCD_BASE) + DW_FC_ADDR_COL);
		writel(0x0,     IO_ADDRESS(DW_LCD_BASE) + DW_FC_CMD);
		writel(0x2,     IO_ADDRESS(DW_LCD_BASE) + DW_FC_DCOUNT);
		writel(0x0,     IO_ADDRESS(DW_LCD_BASE) + DW_FC_FBYP_CTL);

		mdelay(10);

		writel(MEM_TRANS, IO_ADDRESS(DW_LCD_BASE) + DW_FC_CTL);

		mdelay(10);

		while ((readl(IO_ADDRESS(DW_LCD_BASE) + DW_FC_FBYP_CTL)) & 0x1);

		mdelay(10);

		id = readl(IO_ADDRESS(DW_LCD_BASE) + DW_FC_FBYP_DATA);
		id >>= 16;
		id &= 0x00ff;

		return (id == c->id);
	}
}

void lcd_ctlr_init(dw_lcd_controller *c)
{
	if(strcmp(c->name, "Sharp LR38825") == 0) {
		lr38825_fb_do_hw_sequence(c);
	}
	else if (strcmp(c->name, "Epson S1D13742") == 0) {
		int tmp;

		s1d13742_fb_do_hw_sequence(c);

		mdelay(10);

		tmp = EPS_READ_REG(c, 0x4);
		tmp &= 0x80;
		tmp >>= 7;
		if(tmp)
			printf("Epson S1D13742 PLL locked\n");
		else
			printf("Epson S1D13742 PLL lock failed!\n");

		EPS_WRITE_REG(c, 0x06, 0x58); // TE is V non disp period
	}
	else if (strcmp(c->name, "Solomon 1962") == 0) {
		solomon1962_do_hw_sequence(c);
	}
}

static struct dw74lcdc_seq dw74lcdc_hw_init_seq[] = {
	{DW74_LCDC_REG_INTER,  0x1c},  /* enable underrun & fifo empty irqs */
	{DW74_LCDC_REG_GCER,   0},     /* disable gamma correction */
	{DW74_LCDC_REG_CLPER,  0},     /* disable clipping */
	{DW74_LCDC_REG_INTMR,  0},     /* 5:6:5 rgb input */

	{DW74_LCDC_REG_BACKCPR, 0},    /* black background */
	{DW74_LCDC_REG_RBCR,    0x80}, /* no color clipping */
	{DW74_LCDC_REG_GBCR,    0x80},
	{DW74_LCDC_REG_BBCR,    0x80},

	{DW74_LCDC_REG_DISPYSPOS2R, 0}, /* only used for CCIR656 */
	{DW74_LCDC_REG_DISPYEPOS2R, 0}, /* only used for CCIR656 */

	{DW74_LCDC_REG_DISPCR, 1}, /* enable display */
	{DW74_LCDC_REG_PARUP,  1}, /* update parameters on next frame */
	{DW74_LCDC_SEQ_END,    0},
};


static const struct dw74lcdc_panel dw74_chimei =
{
    .name = "Chimei WVGA",
	.xres = 800,
	.yres = 480,
	.bits_per_pixel = 16,
	.lcdc_gp_en = 0x1ff,
	.clock_rate = 34000000, /* 34mhz pixel clock = ~60fps @ 800x480 */
	.init_seq = {
		/* data driven on falling edge */
		{DW74_LCDC_REG_PANCSR,  0x0001},

		/* vertical timing */
		{DW74_LCDC_REG_VSTR,    4},
		{DW74_LCDC_REG_VFTR,    35},
		{DW74_LCDC_REG_VATR,    479},
		{DW74_LCDC_REG_VETR,    41},

		/* horizontal timing */
		{DW74_LCDC_REG_HSTR,    9},
		{DW74_LCDC_REG_HFTR,    65},
		{DW74_LCDC_REG_HADSTR,  799},
		{DW74_LCDC_REG_HAPWR,   799},
		{DW74_LCDC_REG_HETR,    73},

		//GP00(LD)
		{DW74_LCDC_REG_GP0B_H_HI,  0x037e},  //H_ST	894
		{DW74_LCDC_REG_GP0B_V_ST,  0x0028},  //V_ST	40
		{DW74_LCDC_REG_GP0B_H_LO,  0x0388},  //H_END	904
		{DW74_LCDC_REG_GP0B_V_END, 0x0207},  //V_END	519
		{DW74_LCDC_REG_GP0BCNTR,   0x8050},

		{DW74_LCDC_REG_GP0A_H_HI,  0x0000},
		{DW74_LCDC_REG_GP0A_V_ST,  0x0000},
		{DW74_LCDC_REG_GP0A_H_LO,  0x0000},
		{DW74_LCDC_REG_GP0A_V_END, 0x0000},
		{DW74_LCDC_REG_GP0ACNTR,   0x0000},

		//GP01(POL)
		{DW74_LCDC_REG_GP1B_H_HI,  0x031f},   //H_ST	799
		{DW74_LCDC_REG_GP1B_V_ST,  0x0028},   //V_ST	40
		{DW74_LCDC_REG_GP1B_H_LO,  0x037a},   //H_END	890
		{DW74_LCDC_REG_GP1B_V_END, 0x0208},   //V_END	520
		{DW74_LCDC_REG_GP1BCNTR,   0x8030},

		//GP02(CKV)
		{DW74_LCDC_REG_GP2_H_HI,  0x01da},   //H_ST 	474
		{DW74_LCDC_REG_GP2_V_ST,  0x0000},   //V_ST  0
		{DW74_LCDC_REG_GP2_H_LO,  0x0388},   //H_END	904
		{DW74_LCDC_REG_GP2_V_END, 0x0000},   //V_END	0
		{DW74_LCDC_REG_GP2CNTR,   0xe050},

		//GP03(OE)
		{DW74_LCDC_REG_GP3_H_HI,  0x0000},   //H_ST	0
		{DW74_LCDC_REG_GP3_V_ST,  0x0029},   //V_ST	41
		{DW74_LCDC_REG_GP3_H_LO,  0x0354},   //H_END	852
		{DW74_LCDC_REG_GP3_V_END, 0x0208},   //V_END	520
		{DW74_LCDC_REG_GP3CNTR,   0xa050},

		//GP04(STH)
		{DW74_LCDC_REG_GP4_H_HI,  0x004a},   //H_ST	74
		{DW74_LCDC_REG_GP4_V_ST,  0x0028},   //V_ST	40
		{DW74_LCDC_REG_GP4_H_LO,  0x004b},   //H_END	75
		{DW74_LCDC_REG_GP4_V_END, 0x0207},   //V_END	519
		{DW74_LCDC_REG_GP4CNTR,   0x8050},

		//GP05(STH)
		{DW74_LCDC_REG_GP5_H_HI,  0x004a},   //H_ST	74
		{DW74_LCDC_REG_GP5_V_ST,  0x0028},   //V_ST	40
		{DW74_LCDC_REG_GP5_H_LO,  0x004b},   //H_END	75
		{DW74_LCDC_REG_GP5_V_END, 0x0207},   //V_END	519
		{DW74_LCDC_REG_GP5CNTR,   0x8050},

		//GP06(STV)
		{DW74_LCDC_REG_GP6_H_HI,  0x034c},   //H_ST	844
		{DW74_LCDC_REG_GP6_V_ST,  0x0028},   //V_ST	40
		{DW74_LCDC_REG_GP6_H_LO,  0x034b},   //H_END	843
		{DW74_LCDC_REG_GP6_V_END, 0x0029},   //V_END	41
		{DW74_LCDC_REG_GP6CNTR,   0x4050},

		//GP07(STV)
		{DW74_LCDC_REG_GP7_H_HI,  0x034c},   //H_ST	844
		{DW74_LCDC_REG_GP7_V_ST,  0x0028},   //V_ST	40
		{DW74_LCDC_REG_GP7_H_LO,  0x034b},   //H_END	843
		{DW74_LCDC_REG_GP7_V_END, 0x002a},   //V_END	41
		{DW74_LCDC_REG_GP7CNTR,   0x40D0},

		{DW74_LCDC_REG_GP_HMAXR, 0x03b5},	//GP_HMAXR	949
		{DW74_LCDC_REG_GP_VMAXR, 0x0230},	//GP_VMAXR	560
		{DW74_LCDC_REG_GPSELR,   0x0001},	//GP_SELR

		{DW74_LCDC_SEQ_END,      0},
	 }

};


static const struct dw74lcdc_panel dw74_chimei_iframe =
{
	.name = "IFrame Chimei WVGA", 	
	.xres = 800, 					
	.yres = 480,					
	.bits_per_pixel = 16,						
	.lcdc_gp_en = 0x4,					
	.clock_rate = 34000000, /* 34mhz pixel clock = ~60fps @ 800x480 */
	.init_seq = {
			/* data driven on falling edge */
			{DW74_LCDC_REG_PANCSR,  0x0001},

			/* vertical timing */
			{DW74_LCDC_REG_VSTR,    4},
			{DW74_LCDC_REG_VFTR,    35},
			{DW74_LCDC_REG_VATR,    479},
			{DW74_LCDC_REG_VETR,    41},

			/* horizontal timing */
			{DW74_LCDC_REG_HSTR,    9},
			{DW74_LCDC_REG_HFTR,    165},                // 65 for chi-mei
			{DW74_LCDC_REG_HADSTR,  799},
			{DW74_LCDC_REG_HAPWR,   799},
			{DW74_LCDC_REG_HETR,    73},

			//GP00(LD)
			{DW74_LCDC_REG_GP0B_H_HI,  0x037e},  //H_ST  894
			{DW74_LCDC_REG_GP0B_V_ST,  0x0028},  //V_ST  40
			{DW74_LCDC_REG_GP0B_H_LO,  0x0388},  //H_END 904
			{DW74_LCDC_REG_GP0B_V_END, 0x0207},  //V_END 519
			{DW74_LCDC_REG_GP0BCNTR,   0x8050},

			{DW74_LCDC_REG_GP0A_H_HI,  0x0000},
			{DW74_LCDC_REG_GP0A_V_ST,  0x0000},
			{DW74_LCDC_REG_GP0A_H_LO,  0x0000},
			{DW74_LCDC_REG_GP0A_V_END, 0x0000},
			{DW74_LCDC_REG_GP0ACNTR,   0x0000},

			//GP01(POL)
			{DW74_LCDC_REG_GP1B_H_HI,  0x031f},   //H_ST 799
			{DW74_LCDC_REG_GP1B_V_ST,  0x0028},   //V_ST 40
			{DW74_LCDC_REG_GP1B_H_LO,  0x037a},   //H_END        890
			{DW74_LCDC_REG_GP1B_V_END, 0x0208},   //V_END        520
			{DW74_LCDC_REG_GP1BCNTR,   0x8030},

			//GP02(CKV)
			{DW74_LCDC_REG_GP2_H_HI,  0x01da},   //H_ST  474
			{DW74_LCDC_REG_GP2_V_ST,  0x0000},   //V_ST  0
			{DW74_LCDC_REG_GP2_H_LO,  0x0388},   //H_END 904
			{DW74_LCDC_REG_GP2_V_END, 0x0000},   //V_END 0
			{DW74_LCDC_REG_GP2CNTR,   0xe050},

			//GP03(OE)
			{DW74_LCDC_REG_GP3_H_HI,  0x0000},   //H_ST  0
			{DW74_LCDC_REG_GP3_V_ST,  0x0029},   //V_ST  41
			{DW74_LCDC_REG_GP3_H_LO,  0x0354},   //H_END 852
			{DW74_LCDC_REG_GP3_V_END, 0x0208},   //V_END 520
			{DW74_LCDC_REG_GP3CNTR,   0xa050},

			//GP04(STH)
			{DW74_LCDC_REG_GP4_H_HI,  0x004a},   //H_ST  74
			{DW74_LCDC_REG_GP4_V_ST,  0x0028},   //V_ST  40
			{DW74_LCDC_REG_GP4_H_LO,  0x004b},   //H_END 75
			{DW74_LCDC_REG_GP4_V_END, 0x0207},   //V_END 519
			{DW74_LCDC_REG_GP4CNTR,   0x8050},

			//GP05(STH)
			{DW74_LCDC_REG_GP5_H_HI,  0x004a},   //H_ST  74
			{DW74_LCDC_REG_GP5_V_ST,  0x0028},   //V_ST  40
			{DW74_LCDC_REG_GP5_H_LO,  0x004b},   //H_END 75
			{DW74_LCDC_REG_GP5_V_END, 0x0207},   //V_END 519
			{DW74_LCDC_REG_GP5CNTR,   0x8050},

			//GP06(STV)
			{DW74_LCDC_REG_GP6_H_HI,  0x034c},   //H_ST  844
			{DW74_LCDC_REG_GP6_V_ST,  0x0028},   //V_ST  40
			{DW74_LCDC_REG_GP6_H_LO,  0x034b},   //H_END 843
			{DW74_LCDC_REG_GP6_V_END, 0x0029},   //V_END 41
			{DW74_LCDC_REG_GP6CNTR,   0x4050},

			//GP07(STV)
			{DW74_LCDC_REG_GP7_H_HI,  0x034c},   //H_ST  844
			{DW74_LCDC_REG_GP7_V_ST,  0x0028},   //V_ST  40
			{DW74_LCDC_REG_GP7_H_LO,  0x034b},   //H_END 843
			{DW74_LCDC_REG_GP7_V_END, 0x002a},   //V_END 41
			{DW74_LCDC_REG_GP7CNTR,   0x40D0},

			{DW74_LCDC_REG_GP_HMAXR, 0x03b5},    //GP_HMAXR      949
			{DW74_LCDC_REG_GP_VMAXR, 0x0230},    //GP_VMAXR      560
			{DW74_LCDC_REG_GPSELR,   0x0000},    //GP_SELR               1 for chi-mei

			{DW74_LCDC_SEQ_END,      0}
	},



};

static struct dw74lcdc_panel dw74_tdt320 = {
	.name = "TCL TD-T320T2G707 HVGA",
	.xres = 320,
	.yres = 480,
	.bits_per_pixel = 16,
	.lcdc_gp_en = 0x1ff,
	.clock_rate = 10696000,
	.init_seq = {
		/* IO polarity and RGB/BGR order */
		{DW74_LCDC_REG_PANCSR,  0x000E},

		/* TFT 18 bits mode */
		{DW74_LCDC_REG_DISPIR, 0x0020},

		/* vertical timing */
		{DW74_LCDC_REG_VSTR,    1},
		{DW74_LCDC_REG_VFTR,    4},
		{DW74_LCDC_REG_VATR,    479},
		{DW74_LCDC_REG_VETR,    2},

		/* horizontal timing */
		{DW74_LCDC_REG_HSTR,    10},
		{DW74_LCDC_REG_HFTR,    40},
		{DW74_LCDC_REG_HADSTR,  319},
		{DW74_LCDC_REG_HAPWR,   319},
		{DW74_LCDC_REG_HETR,    20},

		{DW74_LCDC_SEQ_END,      0},
	},
};

static inline void dw74lcdc_writel(unsigned long addr, unsigned long val)
{
	writel(val, IO_ADDRESS(addr));
}


static void dw74lcdc_run_sequence(const struct dw74lcdc_seq *seq)
{
	struct dw74lcdc_seq *cmd = (struct dw74lcdc_seq *)seq;

	while (cmd->addr != DW74_LCDC_SEQ_END) {
		writel(cmd->value , IO_ADDRESS(cmd->addr));
		cmd++;
	}
}

static void dw74lcdc_hw_disable(void)
{
	dw74lcdc_writel(DW74_LCDC_REG_LCDCCR , 0);
}

static void dw74lcdc_hw_enable(void)
{
	
	dw74lcdc_writel(DW74_LCDC_REG_LCDCCR , 1);
}


static void dw74lcdc_hw_param_update(void)
{
	dw74lcdc_writel(DW74_LCDC_REG_PARUP, 1);
}

static void dw74lcdc_backlight_on(void)
{
	gpio_enable(AGPIO(6));
	gpio_direction_output(AGPIO(6), 1);

	/* or:
	gpio_enable(CGPIO(7));
	gpio_direction_output(CGPIO(7), 1);
	 */
}

static void dw74lcdc_hw_init_panel(const struct dw74lcdc_panel* panel)
{
	writel(panel->lcdc_gp_en, IO_ADDRESS(DW_SYSCFG_BASE) + 0x28);

	/* input data transfer sizes */
	dw74lcdc_writel(DW74_LCDC_REG_INDTR, (panel->xres >> 1) - 1);
	dw74lcdc_writel(DW74_LCDC_REG_INDXSR, panel->xres - 1);
	dw74lcdc_writel(DW74_LCDC_REG_INDYSR, panel->yres - 1);

	/* display position */
	dw74lcdc_writel(DW74_LCDC_REG_DISPXSPOSR,  0);
	dw74lcdc_writel(DW74_LCDC_REG_DISPXEPOSR,  panel->xres - 1);
	dw74lcdc_writel(DW74_LCDC_REG_DISPYSPOS1R, 0);
	dw74lcdc_writel(DW74_LCDC_REG_DISPYEPOS1R, panel->yres - 1);

	/* input buffer */
	dw74lcdc_writel(DW74_LCDC_REG_MSBAHBA0R, ((unsigned long)image_buffer) >> 16);
	dw74lcdc_writel(DW74_LCDC_REG_LSBAHBA0R, ((unsigned long)image_buffer) & 0xffff);
	dw74lcdc_writel(DW74_LCDC_REG_OFFAR0, panel->xres * sizeof(unsigned short));

	dw74lcdc_writel(DW74_LCDC_REG_MSBAHBA1R, 0);
	dw74lcdc_writel(DW74_LCDC_REG_LSBAHBA1R, 0);
	dw74lcdc_writel(DW74_LCDC_REG_OFFAR1,    0);

	dw74lcdc_run_sequence(panel->init_seq);
}

extern int dw_board;

static void dw74lcdc_gpio_config(void)
{
	gpio_disable(DGPIO(1));  /* LPCLK */
	gpio_disable(DGPIO(2));  /* CLD0 */
	gpio_disable(DGPIO(3));  /* CLD1 */
	gpio_disable(DGPIO(4));  /* CLD2 */
	gpio_disable(DGPIO(5));  /* CLD3 */
	gpio_disable(DGPIO(6));  /* CLD4 */
	gpio_disable(DGPIO(7));  /* CLD5 */
	gpio_disable(DGPIO(8));  /* CLD6 */
	gpio_disable(DGPIO(9));  /* CLD7 */
	gpio_disable(DGPIO(10)); /* CLD8 */
	gpio_disable(DGPIO(11)); /* CLD9 */
	gpio_disable(DGPIO(12)); /* CLD10 */
	gpio_disable(DGPIO(13)); /* CLD11 */
	gpio_disable(DGPIO(14)); /* CLD12 */
	gpio_disable(DGPIO(15)); /* CLD13 */
	gpio_disable(DGPIO(16)); /* CLD14 */
	gpio_disable(DGPIO(17)); /* CLD15 */
	gpio_disable(DGPIO(18)); /* CLD16 */
	gpio_disable(DGPIO(19)); /* CLD17 */
	gpio_disable(DGPIO(22)); /* LCLDGP0 */
	gpio_disable(DGPIO(20)); /* LCLDGP1 */
	gpio_disable(DGPIO(0));  /* LCLDGP2 */

#if 0
	gpio_disable(DGPIO(21)); /* LCLDGP3 */
	gpio_disable(DGPIO(24)); /* LCLDGP4 */
	gpio_disable(DGPIO(25)); /* LCLDGP5 */
	gpio_disable(DGPIO(26)); /* LCLDGP6 */
	gpio_disable(DGPIO(23)); /* LCLDGP7 */
	gpio_disable(AGPIO(1));  /* LCLDGP8 */
#endif

	gpio_enable(CGPIO(0)); /* Rotate LCD */
	gpio_direction_output(CGPIO(0), 1);

#if 0
	gpio_enable(BGPIO(29));
	gpio_direction_output(BGPIO(29), 0);
#endif

	gpio_enable(BGPIO(2));  /* LCD on/off */
	gpio_direction_output(BGPIO(2), 1);
}

static void dw74lcdc_clock_enable(void)
{
	clk_enable("lcdc");
}


static void dw74lcdc_syscfg(unsigned long clock_rate)
{
	unsigned long tmp;

	if (!(readl(IO_ADDRESS(DW_CMU_BASE) + DW_CMU_PLL4CR1) & 0x1)) {
		/* Enable PLL4 (source is OSC12) and set it to 492MHz:
		 * PLL4_R=0, PLL4_F=39, PLL4_OD=1 (VCO/2) */
		writel(0x27,  IO_ADDRESS(DW_CMU_BASE) + DW_CMU_PLL4CR0);
		writel(0x103, IO_ADDRESS(DW_CMU_BASE) + DW_CMU_PLL4CR1);
		tmp = (492000000 + (clock_rate-1)) / clock_rate;
	} else {
		tmp = (240000000 + (clock_rate-1)) / clock_rate;
	}
	if (tmp > 0)
		tmp--;
	tmp |= 0x100; /* use PLL4 @ 492MHz */
	writel(tmp, IO_ADDRESS(DW_CMU_BASE) + DW_CMU_CDR4);
}

static void dw74lcdc_color(int nr)
{
	unsigned long addr = (unsigned long)image_buffer;

	if (nr != 0)
		addr += 800*480*2;

	dw74lcdc_writel(DW74_LCDC_REG_MSBAHBA0R, addr >> 16);
	dw74lcdc_writel(DW74_LCDC_REG_LSBAHBA0R, addr & 0xffff);

	dw74lcdc_writel(DW74_LCDC_REG_PARUP, 1);
}

static int toggle_screen = 0;
static int lcd_type = DW74_LCDC_TYPE_UNKNOWN;
extern void set_backlight(int onoff);

void lcd_toggle(void)
{
	static unsigned long toggle = 0;

	if (! toggle_screen)
		return;

	toggle++;
	if ((lcd_type == DW74_LCDC_TYPE_CHIMEI_IFRAME) ||
	    (lcd_type == DW74_LCDC_TYPE_IMH) ||
	    (lcd_type == DW74_LCDC_TYPE_CHIMEI))
		dw74lcdc_color(toggle & 0x1);
}

void dw74lcdc_init_colors(void)
{
	int i;

	/* red: 0xf800f800 */
	for (i = 0; i < 800*480/2; i++)
		image_buffer[i] = 0x00000000; /* black */
	for (i = 0; i < 800*480/2; i++)
		image_buffer[i + 800*480/2] = 0x07e007e0; /* green */
}

static int dw74lcdc_get_type(void)
{
	char* toggle_screen_str;
	char* p_lcdc_type;

	toggle_screen_str = getenv("toggle_screen");
	if (toggle_screen_str != NULL)
		toggle_screen = simple_strtol(toggle_screen_str, NULL, 10);

	p_lcdc_type = getenv("lcdctype");
	if ( p_lcdc_type == NULL || *p_lcdc_type == 0 ){
		printf("lcdctype is empty - we don't use lcdc\n");
		return DW74_LCDC_TYPE_UNKNOWN;
	}

	if (!strcmp(p_lcdc_type , "0"))
		return DW74_LCDC_TYPE_CHIMEI_IFRAME;

	if (!strcmp(p_lcdc_type , "1"))
		return DW74_LCDC_TYPE_CHIMEI;

	if (!strcmp(p_lcdc_type , "2"))
		return DW74_LCDC_TYPE_IMH;

	printf("Unable to determine the lcdc type\n");
	return DW74_LCDC_TYPE_UNKNOWN;

}

int dw74_tdt320_init(void);

static int dw74lcdc_hw_init(void)
{
	lcd_type = dw74lcdc_get_type();

	if (lcd_type == DW74_LCDC_TYPE_UNKNOWN)
		return -1;

	if ((lcd_type == DW74_LCDC_TYPE_CHIMEI_IFRAME) ||
	    (lcd_type == DW74_LCDC_TYPE_CHIMEI) ||
	    (lcd_type == DW74_LCDC_TYPE_IMH)) {
		dw74lcdc_init_colors();

		if (lcd_type == DW74_LCDC_TYPE_CHIMEI_IFRAME) {
			dw74lcdc_syscfg(dw74_chimei_iframe.clock_rate);
		} else if (lcd_type == DW74_LCDC_TYPE_CHIMEI ) {
			dw74lcdc_syscfg(dw74_chimei.clock_rate);
		} else if (lcd_type == DW74_LCDC_TYPE_IMH) {
			dw74lcdc_syscfg(dw74_tdt320.clock_rate);
		}

		//Enable the clock of the LCDC
		dw74lcdc_clock_enable();

		//Configure the GPIO
		dw74lcdc_gpio_config();

		dw74lcdc_hw_disable();

		//LCDC general configuration
		dw74lcdc_run_sequence( dw74lcdc_hw_init_seq);

		//Specific panel configuration
		if (lcd_type == DW74_LCDC_TYPE_CHIMEI_IFRAME) {
			dw74lcdc_hw_init_panel(&dw74_chimei_iframe);
		} else if (lcd_type == DW74_LCDC_TYPE_CHIMEI ) {
			dw74lcdc_hw_init_panel(&dw74_chimei);
		} else if (lcd_type == DW74_LCDC_TYPE_IMH) {
			dw74lcdc_hw_init_panel(&dw74_tdt320);
		}

		dw74lcdc_hw_enable();

		if (lcd_type == DW74_LCDC_TYPE_IMH)
			dw74_tdt320_init();

		dw74lcdc_hw_param_update();

		dw74lcdc_backlight_on();
	}

	return 0;
}

void lcd__init(void)
{
	int ctlr;
	int ret;

	/* Read the chip type */
	unsigned id = readl(IO_ADDRESS(DW_SYSCFG_BASE) + DW_SYSCFG_CHIP_ID);

	if (id == 0x0740) {
		memset(image_buffer,0,sizeof(image_buffer));
		ret = dw74lcdc_hw_init();

		/* if we were able to configure lcdc - don't try to look for external LCD controllers */
		if (ret == 0) {
			return;
		}
	}

	/* enable clocks */
	clk_enable("fc");
	clk_enable("dma");
	clk_enable("dmafifo");

	mdelay(100);

	ctlr = 0;
	while (lcd_controllers[ctlr].name != NULL) {
		dw_lcd_controller *controller = &lcd_controllers[ctlr];

		if (!lcd_ctlr_detect(controller)) {
			ctlr++;
			continue;
		}


		mdelay(10);

		lcd_ctlr_init(controller);

		printf("Found LCD controller: %s (%dx%d)\n",
			controller->name,
			controller->h_size,
			controller->v_size);

		lcd_ctlr_fillscreen(controller);

		ctlr++;
	}
}

void lcd__deinit(void)
{
	if ((lcd_type == DW74_LCDC_TYPE_CHIMEI_IFRAME) ||
	    (lcd_type == DW74_LCDC_TYPE_CHIMEI))
		dw74lcdc_hw_disable();
}

static void lr38825_fb_do_hw_sequence(struct dw_lcd_controller_t *c)
{
	int i = 0;
	static const struct dw_lcd_sequence_t lr38825_seq[] = {
		{0xfd, 0xfd, 0},
		{0xfd, 0xfd, 100*mSEC_1},
		{0xee, 0x00, 0},
		{0xe0, 0x01, 0},
		{0x7f, 0x01, 5*uSEC_1},
		{0xe0, 0x00, 0},
		{0x7f, 0x01, 5*uSEC_1},
		{0x1b, 0x04, 0},
		{0xfe, 0xfe, 0},
		{0xfe, 0xfe, 5*uSEC_1},
		{0x10, 0x08, 0}, //0x08-RGB, 0x48-BGR
		{0x12, 0x00, 0},
		{0x13, 0x00, 0},
		{0x15, 0xaf, 0},
		{0x16, 0xdb, 0},
		{0x18, 0x00, 0},
		{0x03, 0x11, 0},
		{0x11, 0x00, 0},
		{0x1c, 0x00, 0},
		{0x45, 0x00, 0},
		{0x88, 0x02, 0},
		{0x7e, 0x05, 0},
		{0x7f, 0x01, 0},
		{0x80, 0x01, 0},
		{(int)NULL,}
	};


	writel(TIMINGS, IO_ADDRESS(DW_LCD_BASE) + DW_FC_PULSETIME);

	writel(0x100606 | LCD_CS(c->cs), IO_ADDRESS(DW_LCD_BASE) + DW_FC_SEQUENCE); //WRITE

	writel(0x0,     IO_ADDRESS(DW_LCD_BASE) + DW_FC_DCOUNT);
	writel(0x2,     IO_ADDRESS(DW_LCD_BASE) + DW_FC_FBYP_CTL);

	while (lr38825_seq[i].reg != 0) {
		writel((lr38825_seq[i].cmd << 8) | lr38825_seq[i].reg, IO_ADDRESS(DW_LCD_BASE) + DW_FC_ADDR_COL);

		// Start the transaction
		writel(MEM_TRANS, IO_ADDRESS(DW_LCD_BASE) + DW_FC_CTL);

		udelay(lr38825_seq[i].delay + 1);

		i++;
	}
}

#if 0
static void lr38825_fb_do_hw_sequence(struct dw_lcd_controller_t *c)
{
	int i = 0;

	writel(TIMINGS, IO_ADDRESS(DW_LCD_BASE) + DW_FC_PULSETIME);

	writel(0x100006 | LCD_CS(c->cs), IO_ADDRESS(DW_LCD_BASE) + DW_FC_SEQUENCE); //WRITE

	writel(0x0,     IO_ADDRESS(DW_LCD_BASE) + DW_FC_DCOUNT);
//	writel(0x2,     IO_ADDRESS(DW_LCD_BASE) + DW_FC_FBYP_CTL);

	while (c->seq[i].reg != 0) {
		writel((c->seq[i].cmd << 8) | c->seq[i].reg, IO_ADDRESS(DW_LCD_BASE) + DW_FC_ADDR_COL);

		// Clear all statuses (especially transaction_done...)
		writel(0xffffffff, IO_ADDRESS(DW_LCD_BASE) + DW_FC_STS_CLR );

		// Start the transaction
		writel(MEM_TRANS, IO_ADDRESS(DW_LCD_BASE) + DW_FC_CTL);

		// Wait for transaction to end
		while ( !(readl(IO_ADDRESS(DW_LCD_BASE) + DW_FC_STATUS) & 0x1)); //Wait while !transaction_done 

		delay(c->seq[i].delay + 1);

		i++;
	}
}
#endif

static const struct dw_lcd_sequence_t lr38825_seq[] = {
	{0x34, 0x80, uSEC_1}, // Display off
	{0x56, 0x02, uSEC_1},
	{0x04, 23  , uSEC_1}, //M-divider+1 (1:24) PLLCLK = 27M/24 = 1.125M (1M<PLLCLK<2M)
	{0x06, 0xF8, uSEC_1},
	{0x08, 0x80, uSEC_1},
	{0x0A, 0x28, uSEC_1},
	{0x0C, 0x00, uSEC_1},
	{0x0E, 45  , uSEC_1}, //LL PLLOut = LL x PLLCLK = 45 x 1.125 = 50.625M (SYSCLK)
	{0x12, 0x09, uSEC_1}, //PCLK divide, SYSCLK source = PLL PCLK = PLLOut / PCLKDIV = 50.625 / 2 = 25.3125M  (VGA@25.175M)
	{0x14, 0x00, uSEC_1}, //18bit panel, no swap
	{0x16, 86  , uSEC_1}, //HDISP: 0x50 = 80 => x8 = 640pixels  ML - Added more pixels to meet VGA requirement
	{0x18, 127 , uSEC_1}, //HNDP: 127 > HSW+HPS
	{0x1A, 0xE0, uSEC_1}, //VDP 480 lines (LSB)
	{0x1C, 0x01, uSEC_1}, //VDP 480 lines (MSB)
	{0x1E, 60  , uSEC_1}, //VNDP
	{0x20, 49  , uSEC_1}, //HSW
	{0x22, 22  , uSEC_1}, //HPS
	{0x24, 2   , uSEC_1}, //VSW
	{0x26, 10  , uSEC_1}, //VPS
	{0x28, 0x80, uSEC_1}, //PCLK pol
	{0x2A, 0x01, uSEC_1}, //16bpp, 5:6:5
	{0x56, 0x00, uSEC_1},
	{(int)NULL,}
};

static const struct dw_lcd_sequence_t lr38825_chimei_seq[] = {
	{0x56, 0x02, uSEC_1},
	{0x04, 23  , uSEC_1}, //M-divider+1 (1:24) PLLCLK = 27M/24 = 1.125M (1M<PLLCLK<2M)
	{0x06, 0xF8, uSEC_1},
	{0x08, 0x80, uSEC_1},
	{0x0A, 0x28, uSEC_1},
	{0x0C, 0x00, uSEC_1},
	{0x0E, 45  , uSEC_1}, //LL PLLOut = LL x PLLCLK = 45 x 1.125 = 50.625M (SYSCLK)
	{0x12, 0x09, uSEC_1}, //PCLK divide, SYSCLK source = PLL PCLK = PLLOut / PCLKDIV = 50.625 / 2 = 25.3125M  (VGA@25.175M)
	{0x14, 0x00, uSEC_1}, //18bit panel, no swap
	{0x16, 100 , uSEC_1}, //HDISP: 0x50 = 80 => x8 = 640pixels  ML - Added more pixels to meet VGA requirement
	{0x18, 16  , uSEC_1}, //HNDP: 127 > HSW+HPS
	{0x1A, 0xE0, uSEC_1}, //VDP 480 lines (LSB)
	{0x1C, 0x01, uSEC_1}, //VDP 480 lines (MSB)
	{0x1E, 3   , uSEC_1}, //VNDP
	{0x20, 3   , uSEC_1}, //HSW
	{0x22, 4   , uSEC_1}, //HPS
	{0x24, 1   , uSEC_1}, //VSW
	{0x26, 1   , uSEC_1}, //VPS
	{0x28, 0x80, uSEC_1}, //PCLK pol
	{0x2A, 0x01, uSEC_1}, //16bpp, 5:6:5
	{0x56, 0x00, uSEC_1},
	{(int)NULL,}
};


static void s1d13742_fb_do_hw_sequence(struct dw_lcd_controller_t *c)
{
	char *pType;
	struct dw_lcd_sequence_t *pLcdSeq;

	pType = getenv("epson_lcd_resolution");

	if(NULL != pType && 0 == strcmp(pType,"800x480")) {
		pLcdSeq = (struct dw_lcd_sequence_t *)&lr38825_chimei_seq[0];

		c->h_size = 800;
		c->v_size = 480;
	}
	else {
		pLcdSeq = (struct dw_lcd_sequence_t *) &lr38825_seq[0];
		c->h_size = 640;
		c->v_size = 480;
	}
    
	writel(EPSON_TIMINGS, IO_ADDRESS(DW_LCD_BASE) + DW_FC_PULSETIME);
	writel(WAITS, IO_ADDRESS(DW_LCD_BASE) + DW_FC_WAIT);
	writel(0x100043 | LCD_CS(c->cs), IO_ADDRESS(DW_LCD_BASE) + DW_FC_SEQUENCE); //WRITE

	writel(0x0, IO_ADDRESS(DW_LCD_BASE) + DW_FC_DCOUNT);
	writel(0x0, IO_ADDRESS(DW_LCD_BASE) + DW_FC_FBYP_CTL);

	while (pLcdSeq != NULL && pLcdSeq->reg != 0) {
		writel(pLcdSeq->cmd, IO_ADDRESS(DW_LCD_BASE) + DW_FC_ADDR_COL);
		writel(pLcdSeq->reg, IO_ADDRESS(DW_LCD_BASE) + DW_FC_CMD);

		// Clear all statuses (especially transaction_done...)
		writel(0xffffffff, IO_ADDRESS(DW_LCD_BASE) + DW_FC_STS_CLR );

		// Start the transaction
		writel(MEM_TRANS, IO_ADDRESS(DW_LCD_BASE) + DW_FC_CTL);

		// Wait for transaction to end
		while ( !(readl(IO_ADDRESS(DW_LCD_BASE) + DW_FC_STATUS) & 0x1));

		udelay(pLcdSeq->delay);

		pLcdSeq++;
	}
}

static void solomon1962_do_hw_sequence(dw_lcd_controller *c)
{
	unsigned char write_data[10];

	//set pll
	write_data[0] = 0x10; //mult=16; 27*(16+1)=459M
	write_data[1] = 0x3; //div=3; 459/(3+1)=114.75M
	write_data[2] = 0x54; //dummy
	writeb_ssd(3, &write_data[0], 0xe2, 10*1000);

	write_data[0] = 0x1; //enable pll
	writeb_ssd(1, &write_data[0], 0xe0, 10*1000);

	udelay(2000);

	write_data[0] = 0x3; //enable and lock pll
	writeb_ssd(1, &write_data[0], 0xe0, 10*1000);

	udelay(2000);

	//issue soft-reset
	writeb_ssd(0, NULL, 0x1, 0);

	udelay(2000);

	//lfreq
	write_data[0] = (0x4763d & 0xf0000) >> 16; //pclk=114.75M*x/1048576
	write_data[1] = (0x4763d & 0x0ff00) >> 8; //x=(1048576*32M)/114.75M
	write_data[2] = (0x4763d & 0x000ff); //x=0x4763d
	writeb_ssd(3, &write_data[0], 0xe6, 0);

	//lcd mode pad size
	write_data[0] = 0x0; //0xc;
	write_data[1] = 0x0;
	write_data[2] = (0x31f & 0xff00) >> 8;
	write_data[3] = (0x31f & 0xff);
	write_data[4] = (0x1df & 0xff00) >> 8;
	write_data[5] = (0x1df & 0xff);
	writeb_ssd(6, &write_data[0], 0xb0, 0);

	//set hori period
	write_data[0] = (0x420 & 0xff00) >> 8;
	write_data[1] = (0x420 & 0xff);
	write_data[2] = (0xd6 & 0xff00) >> 8;
	write_data[3] = (0xd6 & 0xff);
	write_data[4] = 0x7f; //0x80
	write_data[5] = 0x0;
	write_data[6] = 0x0;
	writeb_ssd(7, &write_data[0], 0xb4, 0);

	//set vert period
	write_data[0] = (0x20d & 0xff00) >> 8;
	write_data[1] = (0x20d & 0xff);
	write_data[2] = (0x23 & 0xff00) >> 8;
	write_data[3] = (0x23 & 0xff);
	write_data[4] = 0x2;
	write_data[5] = 0x0;
	write_data[6] = 0x0;
	writeb_ssd(7, &write_data[0], 0xb6, 0);

	//set display off (no data)
	writeb_ssd(0, NULL, 0x28, 0);

	//column address = 799 (800 pixels)
	write_data[0] = 0x0;
	write_data[1] = 0x0;
	write_data[2] = (0x31f & 0xff00) >> 8;
	write_data[3] = (0x31f & 0xff);
	writeb_ssd(4, &write_data[0], 0x2a, 0);

	//page address = 479 (480 pixels)
	write_data[0] = 0x0;
	write_data[1] = 0x0;
	write_data[2] = (0x1df & 0xff00) >> 8;
	write_data[3] = (0x1df & 0xff);
	writeb_ssd(4, &write_data[0], 0x2b, 0);

	//set address mode
	write_data[0] = 0x02;
	writeb_ssd(1, &write_data[0], 0x36, 0);

	//pixel data interface
	write_data[0] = 0x3;
	writeb_ssd(1, &write_data[0], 0xf0, 0);

	//tear-effect VSYNC only
	write_data[0] = 0x1;
	writeb_ssd(1, &write_data[0], 0x35, 0);
}


// Convert hex string to integer
// 1. str must not be NULL
// 2. the input should not contain 0x
// 3. the function will decode until finding character which is not hex digit
static int hex_str_to_int( const char *str )
{
	int ret = 0;
	int digit;
	char c;

	while ( 1 ) {
		// Get next character from string
		c = *str++;

		// Check if current character is hex digit
		if ( c >= '0' && c <= '9' )
			digit = c - '0';
		else if ( c >= 'A' && c <= 'F' )
			digit = 10 + c - 'A';
		else if ( c >= 'a' && c <= 'f' )
			digit = 10 + c - 'a';
		else
			break;

		// Add digit to number
		ret *= 16;
		ret += digit;
	}

	return ret;
}

// Find the location of the bmp file on the NAND
// Returns -1 in case of error
static int find_bmp_offset_on_nand( void )
{
	char *p_mtd_parts;
	char *p;
	int offset;

	p_mtd_parts = getenv("mtdparts");
	if ( p_mtd_parts == NULL || *p_mtd_parts == 0 )
    {
        printf("mtdparts is empty\n");
        return -1;
    }

	// The partition is defined as follows: size@offset(name) while offset is 0x12345...
	// We want to get the offset parameter
	p = strstr( p_mtd_parts, BMP_PARTITION_NAME );
	if ( !p )
    {
        printf("%s is not found in mtdparts\n",BMP_PARTITION_NAME);
        return -1;
    }
	
	// Search for x backwords	
	while ( p != p_mtd_parts ) {
		p--;
		if (*p == 'x')
			break;
	}

	// Return with error if we can't find the 'x' we were looking for
	if ( p == p_mtd_parts )
    {
        // Mtdparts doesn't contain partition address try to do it using jffs2 utility
        struct mtd_device *dev;
        struct part_info *part;
        u8 pnum;

        if ( (mtdparts_init() == 0) && 
            (find_dev_and_part(BMP_PARTITION_NAME, &dev, &pnum, &part) == 0) ) 
        {
            if ( dev->id->type != MTD_DEV_TYPE_NAND ) 
            {
                printf("not a NAND device\n");
                return -1;
            }

            offset = (int)part->offset;
            //*size = part->size;
        }
        else 
        {
            printf("Cannot find %s partition offset\n",BMP_PARTITION_NAME);
            return -1;
        }
    }
    else
    {
        // Convert to integer
        offset = hex_str_to_int( p+1 );
        
    }

    printf("Bmp offset %d\n",offset);

	return offset;
}

// Init process of BMP image loading from NAND
// Returns -1 in case of error. Otherwise - return 0.
static int init_read_bmp_from_nand( void )
{
	int partition_offset;
	int ret;
	nand_info_t *nand;

	partition_offset = find_bmp_offset_on_nand();

	if ( partition_offset == -1 ) {
		printf("Can't find BMP image on NAND. Please check for " BMP_PARTITION_NAME " in mtdparts\n");
		return -1;
	}

	bmp_cache_index = 0;
	bmp_file_index = partition_offset + sizeof(bmp_cache);

	// Fill cache with first chunk of BMP file
	nand = &nand_info[nand_curr_device];
	ret = dw_copy_nand_to_ram(nand, (ulong)bmp_cache, partition_offset, sizeof(bmp_cache) );

	if ( ret == -1 ) {
		printf("Error while trying to read BMP header from NAND\n");
		return -1;
	}

	return 0;
}


//
// Read next "size" bytes from BMP image on nand
// The function is using 4KB cache on RAM, and accesses the NAND only when needed
// The size to be read must be <= 2KB
//
static unsigned char* read_bmp_from_nand( int size )
{
	unsigned char *retval;
	int ret;
	nand_info_t *nand;

	if ( bmp_cache_index + size > sizeof(bmp_cache) ) {
		// We don't have the whole chunk in cache, so we will fill half of the cache
		// We will copy the remaining data in the cache to the end of the "first half", and then load the second half of the cache
		memcpy(	bmp_cache + bmp_cache_index - sizeof(bmp_cache)/2,
			bmp_cache + bmp_cache_index,
			sizeof(bmp_cache) - bmp_cache_index );

		// Update chache index
		bmp_cache_index -= sizeof(bmp_cache)/2;

		// Read from NAND
		nand = &nand_info[nand_curr_device];
		ret = dw_copy_nand_to_ram(	nand,
						(ulong)bmp_cache + sizeof(bmp_cache)/2,
						bmp_file_index,
						sizeof(bmp_cache)/2 );

		// Increment file index position
		bmp_file_index += sizeof(bmp_cache)/2;
	}

	// Calculate return address and increment cahce index
	retval = bmp_cache + bmp_cache_index;
	bmp_cache_index += size;

	return retval;
}


static void lcd_ctlr_fillscreen(dw_lcd_controller *c)
{
	int i;
    	int j;
//	unsigned char r, g, b;

#define DELAY 1

	if (strcmp(c->name, "Sharp LR38825") == 0)
	{
		//program machine for transaction
		writel(TIMINGS,     IO_ADDRESS(DW_LCD_BASE) + DW_FC_PULSETIME);

		writel(0x100606 | LCD_CS(c->cs), (IO_ADDRESS(DW_LCD_BASE) + DW_FC_SEQUENCE)); //WRITE transaction

		writel(0x0,         IO_ADDRESS(DW_LCD_BASE) + DW_FC_DCOUNT);
		writel(0x2,         IO_ADDRESS(DW_LCD_BASE) + DW_FC_FBYP_CTL);
	
		writel(0x0810,      IO_ADDRESS(DW_LCD_BASE) + DW_FC_ADDR_COL); //HIF1 (0x08 for PDF0=1)
		writel(MEM_TRANS,   IO_ADDRESS(DW_LCD_BASE) + DW_FC_CTL);
		udelay(DELAY);

		writel(0x1003,      IO_ADDRESS(DW_LCD_BASE) + DW_FC_ADDR_COL); //VMD
		writel(MEM_TRANS,   IO_ADDRESS(DW_LCD_BASE) + DW_FC_CTL);
		udelay(DELAY);
		
		writel(0x0011,      IO_ADDRESS(DW_LCD_BASE) + DW_FC_ADDR_COL); //HIF2
		writel(MEM_TRANS,   IO_ADDRESS(DW_LCD_BASE) + DW_FC_CTL);
		udelay(DELAY);
	
		writel(0xaf12,      IO_ADDRESS(DW_LCD_BASE) + DW_FC_ADDR_COL); //ASX
		writel(MEM_TRANS,   IO_ADDRESS(DW_LCD_BASE) + DW_FC_CTL);
		udelay(DELAY);

		writel(0xdb13,      IO_ADDRESS(DW_LCD_BASE) + DW_FC_ADDR_COL); //ASY
		writel(MEM_TRANS,   IO_ADDRESS(DW_LCD_BASE) + DW_FC_CTL);
		udelay(DELAY);

		writel(0x0015,      IO_ADDRESS(DW_LCD_BASE) + DW_FC_ADDR_COL); //AEX
		writel(MEM_TRANS,   IO_ADDRESS(DW_LCD_BASE) + DW_FC_CTL);
		udelay(DELAY);

		writel(0x0016,      IO_ADDRESS(DW_LCD_BASE) + DW_FC_ADDR_COL); //AEY
		writel(MEM_TRANS,   IO_ADDRESS(DW_LCD_BASE) + DW_FC_CTL);
		udelay(DELAY);

		writel(0x0318,      IO_ADDRESS(DW_LCD_BASE) + DW_FC_ADDR_COL); //AGM - 180 degrees rotation
		writel(MEM_TRANS,   IO_ADDRESS(DW_LCD_BASE) + DW_FC_CTL);
		udelay(DELAY);

	
		writel(TIMINGS,     IO_ADDRESS(DW_LCD_BASE) + DW_FC_PULSETIME);

		//Draw Tux
		for(j=0; j<220; j++) {
			// Start transaction of 176 words
			writel(0x100600 | LCD_CS(c->cs),    IO_ADDRESS(DW_LCD_BASE) + DW_FC_SEQUENCE); //WRITE
			writel(0x2,         IO_ADDRESS(DW_LCD_BASE) + DW_FC_FBYP_CTL);
			writel(176*2,       IO_ADDRESS(DW_LCD_BASE) + DW_FC_DCOUNT);
			writel(-1, IO_ADDRESS(DW_LCD_BASE) + DW_FC_STS_CLR);
			writel(0x1, IO_ADDRESS(DW_LCD_BASE) + DW_FC_CTL);

			for(i=0; i<176; i++) {
				while((readl((IO_ADDRESS(DW_LCD_BASE) + DW_FC_FBYP_CTL)) & 0x1) != 0); //Wait while BUSYSTS
#if 0
				r = (pic[j*176+i] & 0xf800) >> 11;
				g = (pic[j*176+i] & 0x07e0) >> 5;
				b = (pic[j*176+i] & 0x001f);
				writel((b<<11)|(g<<5)|r,  (IO_ADDRESS(DW_LCD_BASE) + DW_FC_FBYP_DATA));
#else
				writel(0,  (IO_ADDRESS(DW_LCD_BASE) + DW_FC_FBYP_DATA));
#endif
				writel(0x6,(IO_ADDRESS(DW_LCD_BASE) + DW_FC_FBYP_CTL));
				udelay(DELAY);
			}
			// Wait for transaction to be done
			while((readl(IO_ADDRESS(DW_LCD_BASE) + DW_FC_STATUS) & DW_FC_STATUS_TRANS_DONE) == 0);
		}
	}
	else if (strcmp(c->name, "Epson S1D13742") == 0) {
		unsigned short* image_buffer16 = (unsigned short*)image_buffer;
		unsigned int num_pixels;
		int ret;

		//Set full screen
		EPS_WRITE_REG(c, ((0x0 & 0x00ff)), 0x38); //XS
		EPS_WRITE_REG(c, ((0x0 & 0xff00) >> 8), 0x3A);
		EPS_WRITE_REG(c, ((0x0 & 0x00ff)), 0x3C); //YS
		EPS_WRITE_REG(c, ((0x0 & 0xff00) >> 8), 0x3E);
		EPS_WRITE_REG(c, ((799 & 0x00ff)), 0x40); //XE
		EPS_WRITE_REG(c, ((799 & 0xff00) >> 8), 0x42);
		EPS_WRITE_REG(c, ((479 & 0x00ff)), 0x44); //YE
		EPS_WRITE_REG(c, ((479 & 0xff00) >> 8), 0x46);

		// Display off
		EPS_WRITE_REG(c, 0x80,0x34);

		/* Load BMP file from NAND to RAM */
		ret = init_read_bmp_from_nand();
		if ( ret != -1 ) {
			/* Decode BMP RLE file into image_buffer */
			num_pixels = lcd_decode_bmp( image_buffer16, read_bmp_from_nand );

			/* Draw image_buffer to EPSON controller */
			EPS_START_WRITE_PIXELS(c);
			for ( i=0 ; i<num_pixels; i++ ) {
				EPS_WRITE_PIXEL(c, image_buffer16[i]);
			}

			EPS_STOP_WRITE_PIXELS(c);
		}

		// Display on
		EPS_WRITE_REG(c, 0x00,0x34);

#ifdef DW_EXPEDITOR2
		/* Turn on backlight of CHI_MEI LCD on Expeditor 2 (AGPIO 15) */
		gpio_direction_output(AGPIO(15), 1);
#endif
	}
	else if (strcmp(c->name, "Solomon 1962") == 0) {
		unsigned short* image_buffer16 = (unsigned short*)image_buffer;
		unsigned char* curr_image_pos = (unsigned char*)image_buffer;
		unsigned long remain_bytes;
		unsigned long bytes_to_transfer;
		unsigned int num_pixels;
		int ret;

		/* Load BMP file from NAND to RAM */
		ret = init_read_bmp_from_nand();
		if ( ret != -1 ) {
			/* Decode BMP RLE file into image_buffer */
			num_pixels = lcd_decode_bmp( image_buffer16, read_bmp_from_nand );

			/* Compute number of bytes in the picture*/
			remain_bytes = num_pixels * 2;

			/* Send WRITE_MEMORY_START command to Solomon 1962 */
			writeb_ssd(0, NULL, 0x2c, 1);

			/* Display the picture to LCD using DMA */
			while ( remain_bytes ) {

				/* Compute number of bytes to transfer in the next DAM operation */
				bytes_to_transfer = min(remain_bytes, SSD1962FB_DMA_TRANS_BYTES);

				/* Initialize FIFO and DMA HWs */
				ssd1962fb_setupfifo();
				ssd1962fb_setupdma(	(unsigned long)curr_image_pos,		// data_addr
							bytes_to_transfer );			// num_bytes

				/* Update remaining bytes and current position */
				remain_bytes -= bytes_to_transfer;
				curr_image_pos += bytes_to_transfer;

				/* Configure NFLC, DMA and FIFO HWs */
				writel_lcdif(0x0, DW_FC_CTL);
				writel_lcdif(0x3f, DW_FC_STS_CLR);
				writel_lcdif(0x0, DW_FC_INT_EN);
				writel_lcdif(0x0, DW_FC_SEQUENCE);

				writel_dma(0x3, DW_DMA_ICR);
				writel_dma(0x1, DW_DMA_IER);

				writel_fifo(0x1, DW_FIFO_ICR);
				writel_fifo(0x1, DW_FIFO_IER);

				writel_fifo(0x7, DW_FIFO_CTL);

				writel_dma(0x1, DW_DMA_CTL);
				writel_dma(0x1, DW_DMA_START);

				writel_lcdif(0x0, DW_FC_CTL);
				writel_lcdif(0x1, DW_FC_INT_EN);

				writel_lcdif(SSD1962FB_DMA_LCDIF_PULSETIME, DW_FC_PULSETIME);
				writel_lcdif(SSD1962FB_DMA_LCDIF_WAIT, DW_FC_WAIT);
				writel_lcdif(SSD1962FB_WRITE_WORD_SEQ | SSD1962FB_CS, DW_FC_SEQUENCE);
				writel_lcdif(bytes_to_transfer, DW_FC_DCOUNT);
				writel_lcdif(0x0, DW_FC_FBYP_CTL);

				/* Clear TRANS_DONE bit */
				writel_lcdif(-1, DW_FC_STS_CLR);

				/* Start the DMA transaction */
				writel_lcdif(SSD1962FB_LCDIF_MEM_TRANS, DW_FC_CTL);

				/* Wait for transaction to be ended */
				while( (readl_lcdif(DW_FC_STATUS) & DW_FC_STATUS_TRANS_DONE) == 0);
			}
		}

		/* Send SET_DISPLAY_ON command to the Solomon 1962 controller */
		writeb_ssd(0, NULL, 0x29, 0);	
	}
}

GraphicDevice gdev;

void *video_hw_init(void)
{
	if ((lcd_type == DW74_LCDC_TYPE_CHIMEI_IFRAME) ||
	    (lcd_type == DW74_LCDC_TYPE_CHIMEI) ||
	    (lcd_type == DW74_LCDC_TYPE_IMH)) {
		gdev.frameAdrs = (unsigned long)image_buffer;
		gdev.winSizeX = 800;
		if (lcd_type == DW74_LCDC_TYPE_IMH)
			gdev.winSizeX = 320;
		gdev.winSizeY = 480;
		gdev.gdfBytesPP = 2;
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
