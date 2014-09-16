/*
 * (C) Copyright 2011, DSP Group
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

#define LCDC_BASE            0x06B00000
#define LCDC_REG_INTR        (LCDC_BASE + 0x0000)
#define LCDC_REG_INTSR       (LCDC_BASE + 0x0004)
#define LCDC_REG_INTER       (LCDC_BASE + 0x0008)
#define LCDC_REG_LCDCCR      (LCDC_BASE + 0x0010)
#define LCDC_REG_INTMR       (LCDC_BASE + 0x0020)
#define LCDC_REG_DISPCR      (LCDC_BASE + 0x0024)
#define LCDC_REG_OSDMCR      (LCDC_BASE + 0x0028)
#define LCDC_REG_INDTR       (LCDC_BASE + 0x0040)
#define LCDC_REG_INDXSR      (LCDC_BASE + 0x0044)
#define LCDC_REG_INDYSR      (LCDC_BASE + 0x0048)
#define LCDC_REG_DISPXSPOSR  (LCDC_BASE + 0x0050)
#define LCDC_REG_DISPXEPOSR  (LCDC_BASE + 0x0054)
#define LCDC_REG_DISPYSPOS1R (LCDC_BASE + 0x0060)
#define LCDC_REG_DISPYEPOS1R (LCDC_BASE + 0x0064)
#define LCDC_REG_DISPYSPOS2R (LCDC_BASE + 0x0068)
#define LCDC_REG_DISPYEPOS2R (LCDC_BASE + 0x006c)
#define LCDC_REG_DISPIDXR    (LCDC_BASE + 0x0090)
#define LCDC_REG_MSBAHBA0R   (LCDC_BASE + 0x00a0)
#define LCDC_REG_LSBAHBA0R   (LCDC_BASE + 0x00a4)
#define LCDC_REG_OFFAR0      (LCDC_BASE + 0x00a8)
#define LCDC_REG_MSBAHBA1R   (LCDC_BASE + 0x00B0)
#define LCDC_REG_LSBAHBA1R   (LCDC_BASE + 0x00B4)
#define LCDC_REG_OFFAR1      (LCDC_BASE + 0x00B8)
#define LCDC_REG_PALR0       (LCDC_BASE + 0x0100)
#define LCDC_REG_PALR1       (LCDC_BASE + 0x0104)
#define LCDC_REG_PALR2       (LCDC_BASE + 0x0108)
#define LCDC_REG_PALR3       (LCDC_BASE + 0x010c)
#define LCDC_REG_BACKCPR     (LCDC_BASE + 0x0140)
#define LCDC_REG_RBCR        (LCDC_BASE + 0x0150)
#define LCDC_REG_GBCR        (LCDC_BASE + 0x0154)
#define LCDC_REG_BBCR        (LCDC_BASE + 0x0158)
#define LCDC_REG_GC0R        (LCDC_BASE + 0x0160)
#define LCDC_REG_GC1R        (LCDC_BASE + 0x0164)
#define LCDC_REG_GC2R        (LCDC_BASE + 0x0168)
#define LCDC_REG_GC3R        (LCDC_BASE + 0x016c)
#define LCDC_REG_GC4R        (LCDC_BASE + 0x0170)
#define LCDC_REG_GC5R        (LCDC_BASE + 0x0174)
#define LCDC_REG_GC6R        (LCDC_BASE + 0x0178)
#define LCDC_REG_GC7R        (LCDC_BASE + 0x017c)
#define LCDC_REG_GC8R        (LCDC_BASE + 0x0180)
#define LCDC_REG_GC9R        (LCDC_BASE + 0x0184)
#define LCDC_REG_GC10R       (LCDC_BASE + 0x0188)
#define LCDC_REG_GC11R       (LCDC_BASE + 0x018c)
#define LCDC_REG_GC12R       (LCDC_BASE + 0x0190)
#define LCDC_REG_GC13R       (LCDC_BASE + 0x0194)
#define LCDC_REG_GC14R       (LCDC_BASE + 0x0198)
#define LCDC_REG_GC15R       (LCDC_BASE + 0x019c)
#define LCDC_REG_GC16R       (LCDC_BASE + 0x01a0)
#define LCDC_REG_GCER        (LCDC_BASE + 0x01a4)
#define LCDC_REG_YCLPCR      (LCDC_BASE + 0x0200)
#define LCDC_REG_CCLPCR      (LCDC_BASE + 0x0204)
#define LCDC_REG_CLPER       (LCDC_BASE + 0x0208)
#define LCDC_REG_DISPIR      (LCDC_BASE + 0x0240)
#define LCDC_REG_PANCSR      (LCDC_BASE + 0x0250)
#define LCDC_REG_VSTR        (LCDC_BASE + 0x0270)
#define LCDC_REG_VFTR        (LCDC_BASE + 0x0274)
#define LCDC_REG_VATR        (LCDC_BASE + 0x0278)
#define LCDC_REG_VETR        (LCDC_BASE + 0x027C)
#define LCDC_REG_HSTR        (LCDC_BASE + 0x0280)
#define LCDC_REG_HFTR        (LCDC_BASE + 0x0284)
#define LCDC_REG_HADSTR      (LCDC_BASE + 0x0288)
#define LCDC_REG_HAPWR       (LCDC_BASE + 0x028c)
#define LCDC_REG_HETR        (LCDC_BASE + 0x0290)
#define LCDC_REG_CTLTR0      (LCDC_BASE + 0x02a8)
#define LCDC_REG_CMDFSR      (LCDC_BASE + 0x02c0)
#define LCDC_REG_CMDAR       (LCDC_BASE + 0x02c4)
#define LCDC_REG_TXDR        (LCDC_BASE + 0x02c8)
#define LCDC_REG_RXDR        (LCDC_BASE + 0x02cc)
#define LCDC_REG_GP0A_H_HI   (LCDC_BASE + 0x0310)
#define LCDC_REG_GP0A_V_ST   (LCDC_BASE + 0x0314)
#define LCDC_REG_GP0A_H_LO   (LCDC_BASE + 0x0318)
#define LCDC_REG_GP0A_V_END  (LCDC_BASE + 0x031c)
#define LCDC_REG_GP0ACNTR    (LCDC_BASE + 0x0320)
#define LCDC_REG_GP0B_H_HI   (LCDC_BASE + 0x0330)
#define LCDC_REG_GP0B_V_ST   (LCDC_BASE + 0x0334)
#define LCDC_REG_GP0B_H_LO   (LCDC_BASE + 0x0338)
#define LCDC_REG_GP0B_V_END  (LCDC_BASE + 0x033c)
#define LCDC_REG_GP0BCNTR    (LCDC_BASE + 0x0340)
#define LCDC_REG_GP1A_H_HI   (LCDC_BASE + 0x0350)
#define LCDC_REG_GP1A_V_ST   (LCDC_BASE + 0x0354)
#define LCDC_REG_GP1A_H_LO   (LCDC_BASE + 0x0358)
#define LCDC_REG_GP1A_V_END  (LCDC_BASE + 0x035c)
#define LCDC_REG_GP1ACNTR    (LCDC_BASE + 0x0360)
#define LCDC_REG_GP1B_H_HI   (LCDC_BASE + 0x0370)
#define LCDC_REG_GP1B_V_ST   (LCDC_BASE + 0x0374)
#define LCDC_REG_GP1B_H_LO   (LCDC_BASE + 0x0378)
#define LCDC_REG_GP1B_V_END  (LCDC_BASE + 0x037c)
#define LCDC_REG_GP1BCNTR    (LCDC_BASE + 0x0380)
#define LCDC_REG_GP2_H_HI    (LCDC_BASE + 0x0390)
#define LCDC_REG_GP2_V_ST    (LCDC_BASE + 0x0394)
#define LCDC_REG_GP2_H_LO    (LCDC_BASE + 0x0398)
#define LCDC_REG_GP2_V_END   (LCDC_BASE + 0x039c)
#define LCDC_REG_GP2_H_EX    (LCDC_BASE + 0x03a0)
#define LCDC_REG_GP2_V_EX    (LCDC_BASE + 0x03a4)
#define LCDC_REG_GP2CNTR     (LCDC_BASE + 0x03a8)
#define LCDC_REG_GP3_H_HI    (LCDC_BASE + 0x03b0)
#define LCDC_REG_GP3_V_ST    (LCDC_BASE + 0x03b4)
#define LCDC_REG_GP3_H_LO    (LCDC_BASE + 0x03b8)
#define LCDC_REG_GP3_V_END   (LCDC_BASE + 0x03bc)
#define LCDC_REG_GP3_H_EX    (LCDC_BASE + 0x03c0)
#define LCDC_REG_GP3_V_EX    (LCDC_BASE + 0x03c4)
#define LCDC_REG_GP3CNTR     (LCDC_BASE + 0x03c8)
#define LCDC_REG_GP4_H_HI    (LCDC_BASE + 0x03d0)
#define LCDC_REG_GP4_V_ST    (LCDC_BASE + 0x03d4)
#define LCDC_REG_GP4_H_LO    (LCDC_BASE + 0x03d8)
#define LCDC_REG_GP4_V_END   (LCDC_BASE + 0x03dc)
#define LCDC_REG_GP4CNTR     (LCDC_BASE + 0x03e0) /* TODO: typo in spec */
#define LCDC_REG_GP5_H_HI    (LCDC_BASE + 0x03f0)
#define LCDC_REG_GP5_V_ST    (LCDC_BASE + 0x03f4)
#define LCDC_REG_GP5_H_LO    (LCDC_BASE + 0x03f8)
#define LCDC_REG_GP5_V_END   (LCDC_BASE + 0x03fc)
#define LCDC_REG_GP5CNTR     (LCDC_BASE + 0x0400)
#define LCDC_REG_GP6_H_HI    (LCDC_BASE + 0x0410)
#define LCDC_REG_GP6_V_ST    (LCDC_BASE + 0x0414)
#define LCDC_REG_GP6_H_LO    (LCDC_BASE + 0x0418)
#define LCDC_REG_GP6_V_END   (LCDC_BASE + 0x041c)
#define LCDC_REG_GP6CNTR     (LCDC_BASE + 0x0420)
#define LCDC_REG_GP7_H_HI    (LCDC_BASE + 0x0430)
#define LCDC_REG_GP7_V_ST    (LCDC_BASE + 0x0434)
#define LCDC_REG_GP7_H_LO    (LCDC_BASE + 0x0438)
#define LCDC_REG_GP7_V_END   (LCDC_BASE + 0x043c)
#define LCDC_REG_GP7CNTR     (LCDC_BASE + 0x0440)
#define LCDC_REG_GP8A_H_HI   (LCDC_BASE + 0x0450)
#define LCDC_REG_GP8A_V_ST   (LCDC_BASE + 0x0454)
#define LCDC_REG_GP8A_H_LO   (LCDC_BASE + 0x0458)
#define LCDC_REG_GP8A_V_END  (LCDC_BASE + 0x045c)
#define LCDC_REG_GP8ACNTR    (LCDC_BASE + 0x0460)
#define LCDC_REG_GP8B_H_HI   (LCDC_BASE + 0x0470)
#define LCDC_REG_GP8B_V_ST   (LCDC_BASE + 0x0474)
#define LCDC_REG_GP8B_H_LO   (LCDC_BASE + 0x0478)
#define LCDC_REG_GP8B_V_END  (LCDC_BASE + 0x047c)
#define LCDC_REG_GP8BCNTR    (LCDC_BASE + 0x0480)
#define LCDC_REG_GP_HMAXR    (LCDC_BASE + 0x0490)
#define LCDC_REG_GP_VMAXR    (LCDC_BASE + 0x0494)
#define LCDC_REG_GPSELR      (LCDC_BASE + 0x04a0)
#define LCDC_REG_GP_FMR      (LCDC_BASE + 0x04a4)
#define LCDC_REG_PARUP       (LCDC_BASE + 0x04d0)
#define LCDC_REG_CDISPUPR    (LCDC_BASE + 0x04d4)
#define LCDC_REG_ISTAR0      (LCDC_BASE + 0x04f0)
#define LCDC_REG_ISTAR1      (LCDC_BASE + 0x04f4)
#define LCDC_REG_ISTAR2      (LCDC_BASE + 0x04f8)
#define LCDC_REG_ISTAR3      (LCDC_BASE + 0x04fc)
#define LCDC_REG_ISTAR4      (LCDC_BASE + 0x0500)
#define LCDC_REG_CMDFIFOR0   (LCDC_BASE + 0x0528)
#define LCDC_REG_CMDFIFOR1   (LCDC_BASE + 0x052c)
#define LCDC_REG_CMDFIFOR2   (LCDC_BASE + 0x0530)
#define LCDC_REG_CMDFIFOR3   (LCDC_BASE + 0x0534)
#define LCDC_REG_CMDFIFOR4   (LCDC_BASE + 0x0538)
#define LCDC_REG_CMDFIFOR5   (LCDC_BASE + 0x053c)
#define LCDC_REG_CMDFIFOR6   (LCDC_BASE + 0x0540)
#define LCDC_REG_CMDFIFOR7   (LCDC_BASE + 0x0544)
#define LCDC_REG_CMDFIFOR8   (LCDC_BASE + 0x0548)
#define LCDC_REG_CMDFIFOR9   (LCDC_BASE + 0x054c)
#define LCDC_REG_CMDFIFOR10  (LCDC_BASE + 0x0550)
#define LCDC_REG_CMDFIFOR11  (LCDC_BASE + 0x0554)
#define LCDC_REG_RSCMDR      (LCDC_BASE + 0x0558)
#define LCDC_REG_RWCMDR      (LCDC_BASE + 0x055c)
#define LCDC_REG_LCMDR       (LCDC_BASE + 0x0560)

#define LCDC_SEQ_END         0xffffffff

struct dmw_lcdc_seq {
	unsigned long addr;
	unsigned short value;
};

struct dmw_lcdc_panel {
	const char *name;
	int xres;
	int yres;
	int bits_per_pixel;
	unsigned long lcdc_gp_en;
	unsigned long clock_rate;
	void (*init_fn)(void);
	void (*standby_fn)(void);
	void (*resume_fn)(void);
	struct dmw_lcdc_seq init_seq[];
};

void dmw_lcdc_init(void);
void lcdc_suspend_mode (unsigned int on);
