/*
 * (C) Copyright 2006-2007
 * Moshe Lazarov, DSP Group, moshel@dsp.co.il
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

#include <linux/stddef.h>

#define uSEC_1			1
#define mSEC_1			1000
#define ARRAY_SIZE(x)		(sizeof(x) / sizeof((x)[0]))
#define MEM_TRANS		(0x401)
#define POWER_ON		1
#define TIMINGS			(0xFFFFF)
#define WAITS			(0xCCCCCCCC)
#define EPSON_TIMINGS		(0xF1111)
#define mdelay(n)		udelay((n)*1000)
#define DW_SYSCFG_BASE                  0x05200000 
#define DW_CSR2							0x05300064

#define DW74_LCDC_TYPE_IMH			2
#define DW74_LCDC_TYPE_CHIMEI			1
#define DW74_LCDC_TYPE_CHIMEI_IFRAME		0
#define DW74_LCDC_TYPE_UNKNOWN			0xffff

/*
 *  NAND FLASH/LCD Register Offsets.
 */
#define DW_LCD_BASE		0x05400000	/* NAND/LCD */

#define DW_FC_CTL		0x0000		/* FC control register */
#define DW_FC_STATUS		0x0004		/* FC status register */
#define DW_FC_STS_CLR		0x0008		/* FC status clear register */
#define DW_FC_INT_EN		0x000C		/* FC interrupt enable mask register */
#define DW_FC_SEQUENCE		0x0010		/* FC sequence register */
#define DW_FC_ADDR_COL		0x0014		/* FC address-column register */
#define DW_FC_ADDR_ROW		0x0018		/* FC address-row register */
#define DW_FC_CMD		0x001C		/* FC command code configuration register */
#define DW_FC_WAIT		0x0020		/* FC wait time configuration register */
#define DW_FC_PULSETIME		0x0024		/* FC pulse time configuration register */
#define DW_FC_DCOUNT		0x0028		/* FC data count register */
#define DW_FC_FBYP_CTL		0x0058		/* FC GF FIFO bypass control register */
#define DW_FC_FBYP_DATA		0x005C		/* FC GF FIFO bypass data register */

#define DW_FC_STATUS_TRANS_DONE (1 << 0)
#define DW_FC_STATUS_TRANS_BUSY (1 << 6)

#define DW_FC_FBYP_CTL_BP_EN    (1 << 1)
#define DW_FC_FBYP_CTL_BP_WRITE (1 << 2)


/*
 *  DMA Register Offsets.
 */
#define DW_DMA_CTL	  	0x0000 		/* DMA control register */
#define DW_DMA_ADDR1  	  	0x0004 		/* DMA address 1 */
#define DW_DMA_ADDR2            0x0008 		/* DMA address 2 */
#define DW_DMA_BURST            0x000C 		/* DMA burst length */
#define DW_DMA_BLK_LEN          0x0010 		/* DMA transfer block length (words) */
#define DW_DMA_BLK_CNT          0x0014 		/* DMA block transfer count */
#define DW_DMA_ISR              0x0018 		/* DMA interrupt status */
#define DW_DMA_IER              0x001C 		/* DMA interrupt enable */
#define DW_DMA_ICR              0x0020 		/* DMA interrupt clear */
#define DW_DMA_START            0x0024 		/* DMA start trigger */

#define DW_DMA_DW_EN            (1 << 0)
#define DW_DMA_DMA_MODE         (1 << 1)
#define DW_DMA_FLASH_MODE       (1 << 2)
#define DW_DMA_FLASH_ECC_WORDS  (1 << 4)
#define DW_DMA_SWAP_MODE        (1 << 12)

#define DW_DMA_ISR_DMA_DONE     (1 << 0)
#define DW_DMA_ISR_DMA_ERROR    (1 << 1)

#define DW_DMA_IER_DMA_DONE     (1 << 0)
#define DW_DMA_IER_DMA_ERROR    (1 << 1)

#define DW_DMA_ICR_DMA_DONE     (1 << 0)
#define DW_DMA_ICR_DMA_ERROR    (1 << 1)

/*
 *  FIFO Register Offsets.
 */
#define DW_FIFO_CTL	  	0x0000 		/* FIFO control register */
#define DW_FIFO_AE_LEVEL  	0x0004 		/* FIFO almose empty level */
#define DW_FIFO_AF_LEVEL  	0x0008 		/* FIFO almost full level */
#define DW_FIFO_ISR             0x000C      /* FIFO interrupt status */
#define DW_FIFO_IER             0x0010      /* FIFO interrupot enable */
#define DW_FIFO_ICR             0x0014      /* FIFO interrupt clear*/
#define DW_FIFO_RST             0x0018      /* FIFO reset */
#define DW_FIFO_VAL             0x001C      /* FIFO value */

#define DW_FIFO_CTL_FIFO_EN     (1 << 0)
#define DW_FIFO_CTL_FIFO_WIDTH  (1 << 1)
#define DW_FIFO_CTL_FIFO_MODE   (1 << 2)

#define DW_FIFO_ISR_OVER        (1 << 0)
#define DW_FIFO_ISR_UNDER       (1 << 1)

#define DW_FIFO_IER_OVER        (1 << 0)
#define DW_FIFO_IER_UNDER       (1 << 1)

#define DW_FIFO_ICR_OVER        (1 << 0)
#define DW_FIFO_ICR_UNDER       (1 << 1)

/* SOLOMON SSD1962 defines */
#define SSD1962FB_CS			0x60000
#define SSD1962FB_LCDIF_WAIT		0xcccccccc
#define SSD1962FB_DMA_LCDIF_WAIT	0x22222222
#define SSD1962FB_LCDIF_PULSETIME	0xfffff
#define SSD1962FB_DMA_LCDIF_PULSETIME	0x22222
#define SSD1962FB_LCDIF_MEM_TRANS	0x401
#define SSD1962FB_WRITE_OFFSET_SEQ	0x000041
#define SSD1962FB_WRITE_BYTE_SEQ	0x000002
#define SSD1962FB_READ_BYTE_SEQ		0x010000
#define SSD1962FB_WRITE_WORD_SEQ	0x100600
#define SSD1962FB_DMA_TRANS_BYTES	7936
#define SSD1962FB_DMA_BLK_CNT		1

#define DW74_LCDC_BASE            0x06B00000
#define DW74_LCDC_REG_INTR        (DW74_LCDC_BASE + 0x0000)
#define DW74_LCDC_REG_INTSR       (DW74_LCDC_BASE + 0x0004)
#define DW74_LCDC_REG_INTER       (DW74_LCDC_BASE + 0x0008)
#define DW74_LCDC_REG_LCDCCR      (DW74_LCDC_BASE + 0x0010)
#define DW74_LCDC_REG_INTMR       (DW74_LCDC_BASE + 0x0020)
#define DW74_LCDC_REG_DISPCR      (DW74_LCDC_BASE + 0x0024)
#define DW74_LCDC_REG_OSDMCR      (DW74_LCDC_BASE + 0x0028)
#define DW74_LCDC_REG_INDTR       (DW74_LCDC_BASE + 0x0040)
#define DW74_LCDC_REG_INDXSR      (DW74_LCDC_BASE + 0x0044)
#define DW74_LCDC_REG_INDYSR      (DW74_LCDC_BASE + 0x0048)
#define DW74_LCDC_REG_DISPXSPOSR  (DW74_LCDC_BASE + 0x0050)
#define DW74_LCDC_REG_DISPXEPOSR  (DW74_LCDC_BASE + 0x0054)
#define DW74_LCDC_REG_DISPYSPOS1R (DW74_LCDC_BASE + 0x0060)
#define DW74_LCDC_REG_DISPYEPOS1R (DW74_LCDC_BASE + 0x0064)
#define DW74_LCDC_REG_DISPYSPOS2R (DW74_LCDC_BASE + 0x0068)
#define DW74_LCDC_REG_DISPYEPOS2R (DW74_LCDC_BASE + 0x006c)
#define DW74_LCDC_REG_DISPIDXR    (DW74_LCDC_BASE + 0x0090)
#define DW74_LCDC_REG_MSBAHBA0R   (DW74_LCDC_BASE + 0x00a0)
#define DW74_LCDC_REG_LSBAHBA0R   (DW74_LCDC_BASE + 0x00a4)
#define DW74_LCDC_REG_OFFAR0      (DW74_LCDC_BASE + 0x00a8)
#define DW74_LCDC_REG_MSBAHBA1R   (DW74_LCDC_BASE + 0x00B0)
#define DW74_LCDC_REG_LSBAHBA1R   (DW74_LCDC_BASE + 0x00B4)
#define DW74_LCDC_REG_OFFAR1      (DW74_LCDC_BASE + 0x00B8)
#define DW74_LCDC_REG_PALR0       (DW74_LCDC_BASE + 0x0100)
#define DW74_LCDC_REG_PALR1       (DW74_LCDC_BASE + 0x0104)
#define DW74_LCDC_REG_PALR2       (DW74_LCDC_BASE + 0x0108)
#define DW74_LCDC_REG_PALR3       (DW74_LCDC_BASE + 0x010c)
#define DW74_LCDC_REG_BACKCPR     (DW74_LCDC_BASE + 0x0140)
#define DW74_LCDC_REG_RBCR        (DW74_LCDC_BASE + 0x0150)
#define DW74_LCDC_REG_GBCR        (DW74_LCDC_BASE + 0x0154)
#define DW74_LCDC_REG_BBCR        (DW74_LCDC_BASE + 0x0158)
#define DW74_LCDC_REG_GC0R        (DW74_LCDC_BASE + 0x0160)
#define DW74_LCDC_REG_GC1R        (DW74_LCDC_BASE + 0x0164)
#define DW74_LCDC_REG_GC2R        (DW74_LCDC_BASE + 0x0168)
#define DW74_LCDC_REG_GC3R        (DW74_LCDC_BASE + 0x016c)
#define DW74_LCDC_REG_GC4R        (DW74_LCDC_BASE + 0x0170)
#define DW74_LCDC_REG_GC5R        (DW74_LCDC_BASE + 0x0174)
#define DW74_LCDC_REG_GC6R        (DW74_LCDC_BASE + 0x0178)
#define DW74_LCDC_REG_GC7R        (DW74_LCDC_BASE + 0x017c)
#define DW74_LCDC_REG_GC8R        (DW74_LCDC_BASE + 0x0180)
#define DW74_LCDC_REG_GC9R        (DW74_LCDC_BASE + 0x0184)
#define DW74_LCDC_REG_GC10R       (DW74_LCDC_BASE + 0x0188)
#define DW74_LCDC_REG_GC11R       (DW74_LCDC_BASE + 0x018c)
#define DW74_LCDC_REG_GC12R       (DW74_LCDC_BASE + 0x0190)
#define DW74_LCDC_REG_GC13R       (DW74_LCDC_BASE + 0x0194)
#define DW74_LCDC_REG_GC14R       (DW74_LCDC_BASE + 0x0198)
#define DW74_LCDC_REG_GC15R       (DW74_LCDC_BASE + 0x019c)
#define DW74_LCDC_REG_GC16R       (DW74_LCDC_BASE + 0x01a0)
#define DW74_LCDC_REG_GCER        (DW74_LCDC_BASE + 0x01a4)
#define DW74_LCDC_REG_YCLPCR      (DW74_LCDC_BASE + 0x0200)
#define DW74_LCDC_REG_CCLPCR      (DW74_LCDC_BASE + 0x0204)
#define DW74_LCDC_REG_CLPER       (DW74_LCDC_BASE + 0x0208)
#define DW74_LCDC_REG_DISPIR      (DW74_LCDC_BASE + 0x0240)
#define DW74_LCDC_REG_PANCSR      (DW74_LCDC_BASE + 0x0250)
#define DW74_LCDC_REG_VSTR        (DW74_LCDC_BASE + 0x0270)
#define DW74_LCDC_REG_VFTR        (DW74_LCDC_BASE + 0x0274)
#define DW74_LCDC_REG_VATR        (DW74_LCDC_BASE + 0x0278)
#define DW74_LCDC_REG_VETR        (DW74_LCDC_BASE + 0x027C)
#define DW74_LCDC_REG_HSTR        (DW74_LCDC_BASE + 0x0280)
#define DW74_LCDC_REG_HFTR        (DW74_LCDC_BASE + 0x0284)
#define DW74_LCDC_REG_HADSTR      (DW74_LCDC_BASE + 0x0288)
#define DW74_LCDC_REG_HAPWR       (DW74_LCDC_BASE + 0x028c)
#define DW74_LCDC_REG_HETR        (DW74_LCDC_BASE + 0x0290)
#define DW74_LCDC_REG_CTLTR0      (DW74_LCDC_BASE + 0x02a8)
#define DW74_LCDC_REG_CMDFSR      (DW74_LCDC_BASE + 0x02c0)
#define DW74_LCDC_REG_CMDAR       (DW74_LCDC_BASE + 0x02c4)
#define DW74_LCDC_REG_TXDR        (DW74_LCDC_BASE + 0x02c8)
#define DW74_LCDC_REG_RXDR        (DW74_LCDC_BASE + 0x02cc)
#define DW74_LCDC_REG_GP0A_H_HI   (DW74_LCDC_BASE + 0x0310)
#define DW74_LCDC_REG_GP0A_V_ST   (DW74_LCDC_BASE + 0x0314)
#define DW74_LCDC_REG_GP0A_H_LO   (DW74_LCDC_BASE + 0x0318)
#define DW74_LCDC_REG_GP0A_V_END  (DW74_LCDC_BASE + 0x031c)
#define DW74_LCDC_REG_GP0ACNTR    (DW74_LCDC_BASE + 0x0320)
#define DW74_LCDC_REG_GP0B_H_HI   (DW74_LCDC_BASE + 0x0330)
#define DW74_LCDC_REG_GP0B_V_ST   (DW74_LCDC_BASE + 0x0334)
#define DW74_LCDC_REG_GP0B_H_LO   (DW74_LCDC_BASE + 0x0338)
#define DW74_LCDC_REG_GP0B_V_END  (DW74_LCDC_BASE + 0x033c)
#define DW74_LCDC_REG_GP0BCNTR    (DW74_LCDC_BASE + 0x0340)
#define DW74_LCDC_REG_GP1A_H_HI   (DW74_LCDC_BASE + 0x0350)
#define DW74_LCDC_REG_GP1A_V_ST   (DW74_LCDC_BASE + 0x0354)
#define DW74_LCDC_REG_GP1A_H_LO   (DW74_LCDC_BASE + 0x0358)
#define DW74_LCDC_REG_GP1A_V_END  (DW74_LCDC_BASE + 0x035c)
#define DW74_LCDC_REG_GP1ACNTR    (DW74_LCDC_BASE + 0x0360)
#define DW74_LCDC_REG_GP1B_H_HI   (DW74_LCDC_BASE + 0x0370)
#define DW74_LCDC_REG_GP1B_V_ST   (DW74_LCDC_BASE + 0x0374)
#define DW74_LCDC_REG_GP1B_H_LO   (DW74_LCDC_BASE + 0x0378)
#define DW74_LCDC_REG_GP1B_V_END  (DW74_LCDC_BASE + 0x037c)
#define DW74_LCDC_REG_GP1BCNTR    (DW74_LCDC_BASE + 0x0380)
#define DW74_LCDC_REG_GP2_H_HI    (DW74_LCDC_BASE + 0x0390)
#define DW74_LCDC_REG_GP2_V_ST    (DW74_LCDC_BASE + 0x0394)
#define DW74_LCDC_REG_GP2_H_LO    (DW74_LCDC_BASE + 0x0398)
#define DW74_LCDC_REG_GP2_V_END   (DW74_LCDC_BASE + 0x039c)
#define DW74_LCDC_REG_GP2_H_EX    (DW74_LCDC_BASE + 0x03a0)
#define DW74_LCDC_REG_GP2_V_EX    (DW74_LCDC_BASE + 0x03a4)
#define DW74_LCDC_REG_GP2CNTR     (DW74_LCDC_BASE + 0x03a8)
#define DW74_LCDC_REG_GP3_H_HI    (DW74_LCDC_BASE + 0x03b0)
#define DW74_LCDC_REG_GP3_V_ST    (DW74_LCDC_BASE + 0x03b4)
#define DW74_LCDC_REG_GP3_H_LO    (DW74_LCDC_BASE + 0x03b8)
#define DW74_LCDC_REG_GP3_V_END   (DW74_LCDC_BASE + 0x03bc)
#define DW74_LCDC_REG_GP3_H_EX    (DW74_LCDC_BASE + 0x03c0)
#define DW74_LCDC_REG_GP3_V_EX    (DW74_LCDC_BASE + 0x03c4)
#define DW74_LCDC_REG_GP3CNTR     (DW74_LCDC_BASE + 0x03c8)
#define DW74_LCDC_REG_GP4_H_HI    (DW74_LCDC_BASE + 0x03d0)
#define DW74_LCDC_REG_GP4_V_ST    (DW74_LCDC_BASE + 0x03d4)
#define DW74_LCDC_REG_GP4_H_LO    (DW74_LCDC_BASE + 0x03d8)
#define DW74_LCDC_REG_GP4_V_END   (DW74_LCDC_BASE + 0x03dc)
#define DW74_LCDC_REG_GP4CNTR     (DW74_LCDC_BASE + 0x03e0) /* TODO: typo in spec */
#define DW74_LCDC_REG_GP5_H_HI    (DW74_LCDC_BASE + 0x03f0)
#define DW74_LCDC_REG_GP5_V_ST    (DW74_LCDC_BASE + 0x03f4)
#define DW74_LCDC_REG_GP5_H_LO    (DW74_LCDC_BASE + 0x03f8)
#define DW74_LCDC_REG_GP5_V_END   (DW74_LCDC_BASE + 0x03fc)
#define DW74_LCDC_REG_GP5CNTR     (DW74_LCDC_BASE + 0x0400)
#define DW74_LCDC_REG_GP6_H_HI    (DW74_LCDC_BASE + 0x0410)
#define DW74_LCDC_REG_GP6_V_ST    (DW74_LCDC_BASE + 0x0414)
#define DW74_LCDC_REG_GP6_H_LO    (DW74_LCDC_BASE + 0x0418)
#define DW74_LCDC_REG_GP6_V_END   (DW74_LCDC_BASE + 0x041c)
#define DW74_LCDC_REG_GP6CNTR     (DW74_LCDC_BASE + 0x0420)
#define DW74_LCDC_REG_GP7_H_HI    (DW74_LCDC_BASE + 0x0430)
#define DW74_LCDC_REG_GP7_V_ST    (DW74_LCDC_BASE + 0x0434)
#define DW74_LCDC_REG_GP7_H_LO    (DW74_LCDC_BASE + 0x0438)
#define DW74_LCDC_REG_GP7_V_END   (DW74_LCDC_BASE + 0x043c)
#define DW74_LCDC_REG_GP7CNTR     (DW74_LCDC_BASE + 0x0440)
#define DW74_LCDC_REG_GP8A_H_HI   (DW74_LCDC_BASE + 0x0450)
#define DW74_LCDC_REG_GP8A_V_ST   (DW74_LCDC_BASE + 0x0454)
#define DW74_LCDC_REG_GP8A_H_LO   (DW74_LCDC_BASE + 0x0458)
#define DW74_LCDC_REG_GP8A_V_END  (DW74_LCDC_BASE + 0x045c)
#define DW74_LCDC_REG_GP8ACNTR    (DW74_LCDC_BASE + 0x0460)
#define DW74_LCDC_REG_GP8B_H_HI   (DW74_LCDC_BASE + 0x0470)
#define DW74_LCDC_REG_GP8B_V_ST   (DW74_LCDC_BASE + 0x0474)
#define DW74_LCDC_REG_GP8B_H_LO   (DW74_LCDC_BASE + 0x0478)
#define DW74_LCDC_REG_GP8B_V_END  (DW74_LCDC_BASE + 0x047c)
#define DW74_LCDC_REG_GP8BCNTR    (DW74_LCDC_BASE + 0x0480)
#define DW74_LCDC_REG_GP_HMAXR    (DW74_LCDC_BASE + 0x0490)
#define DW74_LCDC_REG_GP_VMAXR    (DW74_LCDC_BASE + 0x0494)
#define DW74_LCDC_REG_GPSELR      (DW74_LCDC_BASE + 0x04a0)
#define DW74_LCDC_REG_GP_FMR      (DW74_LCDC_BASE + 0x04a4)
#define DW74_LCDC_REG_PARUP       (DW74_LCDC_BASE + 0x04d0)
#define DW74_LCDC_REG_CDISPUPR    (DW74_LCDC_BASE + 0x04d4)
#define DW74_LCDC_REG_ISTAR0      (DW74_LCDC_BASE + 0x04f0)
#define DW74_LCDC_REG_ISTAR1      (DW74_LCDC_BASE + 0x04f4)
#define DW74_LCDC_REG_ISTAR2      (DW74_LCDC_BASE + 0x04f8)
#define DW74_LCDC_REG_ISTAR3      (DW74_LCDC_BASE + 0x04fc)
#define DW74_LCDC_REG_ISTAR4      (DW74_LCDC_BASE + 0x0500)
#define DW74_LCDC_REG_CMDFIFOR0   (DW74_LCDC_BASE + 0x0528)
#define DW74_LCDC_REG_CMDFIFOR1   (DW74_LCDC_BASE + 0x052c)
#define DW74_LCDC_REG_CMDFIFOR2   (DW74_LCDC_BASE + 0x0530)
#define DW74_LCDC_REG_CMDFIFOR3   (DW74_LCDC_BASE + 0x0534)
#define DW74_LCDC_REG_CMDFIFOR4   (DW74_LCDC_BASE + 0x0538)
#define DW74_LCDC_REG_CMDFIFOR5   (DW74_LCDC_BASE + 0x053c)
#define DW74_LCDC_REG_CMDFIFOR6   (DW74_LCDC_BASE + 0x0540)
#define DW74_LCDC_REG_CMDFIFOR7   (DW74_LCDC_BASE + 0x0544)
#define DW74_LCDC_REG_CMDFIFOR8   (DW74_LCDC_BASE + 0x0548)
#define DW74_LCDC_REG_CMDFIFOR9   (DW74_LCDC_BASE + 0x054c)
#define DW74_LCDC_REG_CMDFIFOR10  (DW74_LCDC_BASE + 0x0550)
#define DW74_LCDC_REG_CMDFIFOR11  (DW74_LCDC_BASE + 0x0554)
#define DW74_LCDC_REG_RSCMDR      (DW74_LCDC_BASE + 0x0558)
#define DW74_LCDC_REG_RWCMDR      (DW74_LCDC_BASE + 0x055c)
#define DW74_LCDC_REG_LCMDR       (DW74_LCDC_BASE + 0x0560)

#define DW74_LCDC_SEQ_END         0xffffffff



typedef struct dw_lcd_sequence_t {
	unsigned short reg;
	unsigned short cmd;
	int delay;
} dw_lcd_sequence;


typedef struct dw_lcd_controller_t {
	char *name;
	unsigned char cs;
	unsigned short id;
	unsigned short h_size;
	unsigned short v_size;
} dw_lcd_controller;

dw_lcd_controller lcd_controllers[] = {
	{
		.name = "Sharp LR38825",
		.cs = 1,
		.id = 0x25,
		.h_size = 176,
		.v_size = 220
	},
	{
		.name =	"Epson S1D13742",
		.cs = 3,
		.id = 0x81
	},
	{
		.name = "Solomon 1962",
		.cs = 3,
		.id = 0x00,		// No ID register at Solomon 1962... we will use #define...
		.h_size = 800,
		.v_size = 480
	},
	{
		NULL,
	},
};

struct dw74lcdc_seq {
	unsigned long addr;
	unsigned short value;
};

struct dw74lcdc_panel {
	const char *name;
	int xres;
	int yres;
	int bits_per_pixel;
	unsigned long lcdc_gp_en;
	unsigned long clock_rate;
	struct dw74lcdc_seq init_seq[];
};

/*
 * Exported function
 */
void lcd__init (void);


