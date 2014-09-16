/*
 * drivers/mtd/nand/dmw96_nfc.c
 *
 * Driver for the NAND flash controller on DMW96 chips.
 * Supports DMA and PIO transfers and different hardware ECC modes.
 *
 * Copyright (C) 2010, 2011 DSP Group Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include <common.h>
#include <malloc.h>
#include <asm/errno.h>
#include <asm/io.h>

#include <linux/ccu.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>

#include "dmw96_nfc.h"

static int use_ecc = 1;

#ifdef CONFIG_NAND_DMW96_REV1
static uint32_t dmw96_nfc_chien_codes[] = {
	0x0B19,
	0x14E2,
	0x0D8D,
	0x01E8,
	0x0D7D,
	0x1B24,
	0x1351,
	0x14AE,
	0x1EA6,
	0x0E12,
	0x1512,
	0x09E9,
	0x1365,
	0x0AE3,
	0x1E4A,
	0x11B8,
	0x14A1,
	0x11ED,
	0x1C13,
	0x0CE5,
	0x0D18,
	0x02C4,
	0x11B9,
	0x0FBC,
	0x0D62,
	0x1A03,
	0x0772,
	0x1346,
	0x1980,
	0x1DF5,
	0x1140,
	0x1D45,
};
#else
static uint32_t dmw96_nfc_chien_codes[] = {
	0x0B19,
	0x14E2,
	0x0D8D,
	0x01E8,
	0x0D7D,
	0x1B24,
	0x1351,
	0x14AE,
	0x1EA6,
	0x0E12,
	0x1512,
	0x09E9,
	0x14A1,
	0x11ED,
	0x1C13,
	0x0CE5,
	0x0D18,
	0x02C4,
	0x11B9,
	0x0FBC,
	0x0D62,
	0x1A03,
	0x0772,
	0x1346,
};
#endif

#ifdef CONFIG_MTD_CMDLINE_PARTS
const char *part_probes[] = { "cmdlinepart", NULL };
#endif

#define NAND_DATABUF_SIZE      (NAND_MAX_PAGESIZE + NAND_MAX_OOBSIZE)

/* Restrictions:
 *
 * - FC_DCOUNT.VIRTUAL_PAGE_COUNT >= FC_FTSR.TRANSFER_SIZE
 * - FC_DCOUNT.SPARE_N3_COUNT < FC_DCOUNT.SPARE_N1_COUNT
 * - FC_DCOUNT.SPARE_N1_COUNT even only numbers in range of 0..12
 * - VP_SIZE * VIRTUAL_PAGE_COUNT <= 65535
 * - if NFC HW stalled because ECC_ERR_FF (Fifo full), SW should read the
 *   errors to allow HW to continue
 */

static struct nand_ecclayout dmw96_nfc_ecclayout;

static unsigned dmw96_nfc_eccmode_bits[] = {
	1,  /* hamming */
	4,  /* bch4 */
	8,  /* bch8 */
#ifdef CONFIG_NAND_DMW96_REV1
	16, /* bch16 */
#else
	12, /* bch12 */
#endif
};

static unsigned
dmw96_nfc_eccbits(unsigned ecc_mode)
{
	BUG_ON(ecc_mode >= ARRAY_SIZE(dmw96_nfc_eccmode_bits));
	return dmw96_nfc_eccmode_bits[ecc_mode];
}

static unsigned
dmw96_nfc_eccsize(unsigned ecc_mode)
{
	BUG_ON(ecc_mode >= ARRAY_SIZE(dmw96_nfc_eccmode_bits));
	return ((dmw96_nfc_eccbits(ecc_mode) * 13) + 7) / 8;
}

static int
dmw96_nfc_eccfind(unsigned bits)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(dmw96_nfc_eccmode_bits); i++) {
		if (dmw96_nfc_eccmode_bits[i] == bits)
			return i;
	}

	return -EINVAL;
}

int
dmw96_nfc_use_ecc(struct mtd_info *mtd, int enable)
{
	struct nand_chip *chip = mtd->priv;
	struct dmw96_nfc_info *info = chip->priv;
	int old;

	old = info->ecc;
	info->ecc = !!enable;

	//printk("%s(): ecc turned %s\n", __func__, enable ? "on" : "off");
	return old;
}

int dmw96_nfc_ecc_mode(struct mtd_info *mtd, int ecc_mode)
{
	struct nand_chip *chip = mtd->priv;
	struct dmw96_nfc_info *info = chip->priv;
	int old;

	if (ecc_mode < 0 || ecc_mode > 3)
		return -1;

	old = info->ecc_mode;
	info->ecc_mode = ecc_mode;

	//printk("%s(): ecc mode %d activated\n", __func__, info->ecc_mode);
	return old;
}

static unsigned long
dmw96_nfc_readl(struct dmw96_nfc_info *info, int reg)
{
	return readl(info->regs + reg);
}

static void
dmw96_nfc_writel(int value, struct dmw96_nfc_info *info, int reg)
{
	writel(value, info->regs + reg);
}

static void
dmw96_nfc_clear_status(struct dmw96_nfc_info *info)
{
	dmw96_nfc_writel(0xffffffff, info, DW_FC_STS_CLR);
}

static void
dmw96_nfc_start(struct dmw96_nfc_info *info)
{
	dmw96_nfc_clear_status(info);
	dmw96_nfc_writel(0x1, info, DW_FC_START);
}

static void
dmw96_nfc_configure_ready_timeout(struct dmw96_nfc_info *info)
{
#if 0
	dmw96_nfc_writel(DW_FC_TIMEOUT_RDY_EN |
	                 DW_FC_TIMEOUT_RDY_CNT(info->ready_timeout),
	                 info, DW_FC_TIMEOUT);
#endif
}

/*
 * in the below layout generation, we have to take care of the following:
 *   - ecc in varying sizes
 *   - bad block marker (two bytes to account for 16bit flash)
 *   - dirty page marker (two bytes to account for 16bit flash)
 *
 * the dirty page marker is needed since the controller tries to calculate
 * ecc values also for blank pages which results in errors. in order to detect
 * and not handle those cases, we use the dirty page marker which gets set
 * once the page is written.
 *
 * the dirty page marker is stored at ((badblockpos & ~1) + 2), and the ecc is
 * stored at (dirty_marker + 2). the dirty marker always is ecc protected
 * (n3 = n1 - 2).
 */
static int
dmw96_nfc_gen_ecclayout(struct dmw96_nfc_info *info,
                        struct nand_ecclayout *layout)
{
	struct mtd_info *mtd = info->mtd;
	int vp_count = mtd->writesize >> VIRTUAL_PAGESIZE_SHIFT;
	int vp_oobsize = mtd->oobsize / vp_count;
	int vp_bytesremain;
	int vp_eccbytes;
	int vp, eccbyte;

	vp_eccbytes = dmw96_nfc_eccsize(info->ecc_mode);

	vp_bytesremain = vp_oobsize - info->n1len - vp_eccbytes;
	if (vp_bytesremain < 0)
		return -1;

	memset(layout, 0, sizeof(*layout));
	layout->eccbytes = vp_count * vp_eccbytes;

	for (vp = 0; vp < vp_count; vp++) {
		int ofs = vp * vp_oobsize;

		for (eccbyte = 0; eccbyte < vp_eccbytes; eccbyte++) {
			int eccpos_idx = vp * vp_eccbytes + eccbyte;

			/* protect against out-of-bounds access */
			if (eccpos_idx >= ARRAY_SIZE(layout->eccpos))
				break;

			layout->eccpos[eccpos_idx] =
				ofs + info->n1len + eccbyte;
		}

		layout->oobfree[vp].offset = ofs + info->n1len + vp_eccbytes;
		layout->oobfree[vp].length = vp_bytesremain;
	}

	return 0;
}

static void dmw96_nfc_ecc_fetch(struct dmw96_nfc_info *info);

static void
dmw96_nfc_wait_trans_done(struct dmw96_nfc_info *info)
{
	uint32_t done = DW_FC_STATUS_TRANS_DONE;
	uint32_t reg;

	reg = dmw96_nfc_readl(info, DW_FC_SEQUENCE);

	/* if we have a data stage, also wait for the AHB transfer to finish */
	if (reg & DW_FC_SEQ_RW_EN)
		done |= DW_FC_STATUS_AHB_DONE;

	/* incase of ecc also wait for the ecc block to finish */
	if (reg & DW_FC_SEQ_DATA_ECC(1))
		done |= DW_FC_STATUS_ECC_DONE;

	do {
		reg = dmw96_nfc_readl(info, DW_FC_STATUS);

		/* handle ecc */
		if (reg & (DW_FC_STATUS_ECC_NC_ERR | DW_FC_STATUS_ERR_FOUND))
			dmw96_nfc_ecc_fetch(info);

	} while ((reg & done) != done);
}

static int
dmw96_nfc_check_timeout(struct dmw96_nfc_info *info)
{
#if 0
	/* check if there was a timeout (chip blocked) */
	if (readl(info->regs + DW_FC_STATUS) & DW_FC_STATUS_RDY_TIMEOUT)
		return 1;
#endif

	return 0;
}

static void
dmw96_nfc_set_fixed_regs(struct dmw96_nfc_info *info)
{
	int i;

	dmw96_nfc_writel(DW_FC_WAIT1A(info->wait_ready_1_fall) |
	                 DW_FC_WAIT1B(info->wait_ready_1_rise) |
	                 DW_FC_WAIT2A(info->wait_ready_2_fall) |
	                 DW_FC_WAIT2B(info->wait_ready_2_rise),
	                 info, DW_FC_WAIT1);

	dmw96_nfc_writel(DW_FC_WAIT3A(info->wait_ready_3_fall) |
	                 DW_FC_WAIT3B(info->wait_ready_3_rise) |
	                 DW_FC_WAIT4A(info->wait_ready_4_fall) |
	                 DW_FC_WAIT4B(info->wait_ready_4_rise),
	                 info, DW_FC_WAIT2);

	dmw96_nfc_writel(DW_FC_WAIT5A(info->wait_ready_5_fall) |
	                 DW_FC_WAIT5B(info->wait_ready_5_rise) |
	                 DW_FC_WAIT6A(info->wait_ready_6_fall) |
	                 DW_FC_WAIT6B(info->wait_ready_6_rise),
	                 info, DW_FC_WAIT3);

	dmw96_nfc_writel(DW_FC_WAIT0A(info->wait_ready_0_fall) |
	                 DW_FC_WAIT0B(info->wait_ready_0_rise),
	                 info, DW_FC_WAIT4);

	dmw96_nfc_writel(DW_FC_PT_READ_LOW(info->pulse_rd_low)    |
	                 DW_FC_PT_READ_HIGH(info->pulse_rd_high)  |
	                 DW_FC_PT_WRITE_LOW(info->pulse_wr_low)   |
	                 DW_FC_PT_WRITE_HIGH(info->pulse_wr_high) |
	                 DW_FC_CLE_SETUP(info->pulse_cle_setup)   |
	                 DW_FC_ALE_SETUP(info->pulse_ale_setup),
	                 info, DW_FC_PULSETIME);

	for (i = 0; i < ARRAY_SIZE(dmw96_nfc_chien_codes); i++) {
		dmw96_nfc_writel(dmw96_nfc_chien_codes[i], info,
		                 DW_FC_CHIEN_SEARCH_1 + i*4);
	}
}

static void
dmw96_nfc_sequence(struct dmw96_nfc_info *info, int column, int page_addr,
                   int cmd)
{
	struct mtd_info *mtd = info->mtd;
	struct nand_chip *chip = mtd->priv;
	int sequence = 0;

	int cycle1 = 0;
	int cycle2 = 0;
	int cycle3 = 0;
	int cycle4 = 0;
	int cycle5 = 0;

	int cmd1 = 0;
	int cmd2 = 0;
	int cmd3 = 0;
	int cmd4 = 0;
	int cmd5 = 0;
	int cmd6 = 0;

	//printk("%s(): a = 0x%08x\n", __func__, column, page_addr);

	if (column != -1) {
		/* emulate NAND_CMD_READOOB for large page devices */
		if ((mtd->writesize > 512) && (cmd == NAND_CMD_READOOB))
			column += mtd->writesize;

		/* on 16bit bus we do not have A0 -> shift */
		if (chip->options & NAND_BUSWIDTH_16)
			column >>= 1;

		/*
		 * on small page flashes only cycle1 is used (A0 - A7). the
		 * remaining address line is controlled by either issueing
		 * NAND_CMD_READ0 or NAND_CMD_READ1 for reading the first or
		 * the second half of the page respectively
		 */
		cycle1 = column & 0xff;
		cycle2 = (column >> 8) & 0xff;
	}

	sequence = DW_FC_SEQ_CMD1_EN | DW_FC_SEQ_WAIT0_EN;
	sequence |= DW_FC_SEQ_CHIP_SEL(info->chipsel_nr);
	sequence |= DW_FC_SEQ_RDY_EN | DW_FC_SEQ_RDY_SEL(info->ready_nr);

	if ((cmd == NAND_CMD_READ0) || (cmd == NAND_CMD_READOOB) ||
	    (cmd == NAND_CMD_SEQIN)) {
		sequence |= DW_FC_SEQ_ADD1_EN | DW_FC_SEQ_RW_EN;

		/* also enable add2 for large page devices */
		if (likely(mtd->writesize > 512))
			sequence |= DW_FC_SEQ_ADD2_EN;
	}

	if (chip->options & NAND_BUSWIDTH_16)
		sequence |= DW_FC_SEQ_MODE16;

	if ((cmd == NAND_CMD_READ0) || (cmd == NAND_CMD_READOOB)) {
		if (mtd->writesize > 512) {
			cmd1 = NAND_CMD_READ0;
			cmd2 = NAND_CMD_READSTART;
			cmd5 = NAND_CMD_RNDOUT;
			cmd6 = NAND_CMD_RNDOUTSTART;

			sequence &= ~DW_FC_SEQ_WAIT0_EN;

			sequence |= DW_FC_SEQ_CMD2_EN | DW_FC_SEQ_WAIT2_EN;
			sequence |= DW_FC_SEQ_CMD5_EN | DW_FC_SEQ_WAIT5_EN;
			sequence |= DW_FC_SEQ_CMD6_EN | DW_FC_SEQ_WAIT6_EN;
		} else {
			cmd1 = cmd; /* NAND_CMD_READ0 or NAND_CMD_READOOB */
		}

		sequence |= DW_FC_SEQ_DATA_READ_DIR | DW_FC_SEQ_WAIT1_EN;
	} else if (cmd == NAND_CMD_SEQIN) {
		if (mtd->writesize > 512) {
			cmd1 = NAND_CMD_SEQIN;
			cmd3 = NAND_CMD_PAGEPROG; /* after data write */
			cmd4 = NAND_CMD_STATUS;   /* after data write */
			cmd5 = NAND_CMD_RNDIN;

			sequence |= DW_FC_SEQ_CMD3_EN | DW_FC_SEQ_WAIT3_EN;
			sequence |= DW_FC_SEQ_CMD4_EN | DW_FC_SEQ_WAIT4_EN;
			sequence |= DW_FC_SEQ_CMD5_EN | DW_FC_SEQ_WAIT5_EN;

			sequence |= DW_FC_SEQ_DATA_WRITE_DIR;

			sequence &= ~DW_FC_SEQ_WAIT0_EN;
			sequence |= DW_FC_SEQ_WAIT6_EN;
			sequence |= DW_FC_SEQ_READ_ONCE;
		} else {
			cmd1 = NAND_CMD_SEQIN;
			cmd3 = NAND_CMD_PAGEPROG; /* after data write */

			sequence |= DW_FC_SEQ_DATA_WRITE_DIR |
			            DW_FC_SEQ_CMD3_EN | DW_FC_SEQ_WAIT3_EN;
		}
	} else if (cmd == NAND_CMD_ERASE1) {
		cmd1 = NAND_CMD_ERASE1;
		cmd2 = NAND_CMD_ERASE2;
		cmd4 = NAND_CMD_STATUS;

		sequence |= DW_FC_SEQ_CMD2_EN | DW_FC_SEQ_WAIT2_EN |
		            DW_FC_SEQ_CMD4_EN | DW_FC_SEQ_WAIT4_EN;
		sequence |= DW_FC_SEQ_DATA_READ_DIR;
		sequence |= DW_FC_SEQ_READ_ONCE;
	}

	cycle3 = page_addr & 0xff;
	cycle4 = (page_addr >> 8) & 0xff;
	sequence |= DW_FC_SEQ_ADD3_EN | DW_FC_SEQ_ADD4_EN;
	if ((chip->chipsize >> chip->page_shift) > (256*256))  {
		cycle5 = (page_addr >> 16) & 0xff;
		sequence |= DW_FC_SEQ_ADD5_EN;
	}

	sequence |= DW_FC_SEQ_DATA_ECC(info->use_ecc);

	if (info->use_ecc) {
		dmw96_nfc_writel(DW_FC_CTL_ECC_OP_MODE(info->ecc_mode) |
		                 DW_FC_CTL_CHIEN_CNT_START(0x0e6c), /* predef */
		                 info, DW_FC_CTL);
	}

	dmw96_nfc_writel(sequence, info, DW_FC_SEQUENCE);

	/* configure address registers */
	dmw96_nfc_writel(DW_FC_ADD1(cycle1) | DW_FC_ADD2(cycle2),
	                 info, DW_FC_ADDR_COL);

	dmw96_nfc_writel(DW_FC_ADD3(cycle3) | DW_FC_ADD4(cycle4) |
	                 DW_FC_ADD5(cycle5),
	                 info, DW_FC_ADDR_ROW);

	/* configure command register */
	dmw96_nfc_writel(DW_FC_CMD_1(cmd1) | DW_FC_CMD_2(cmd2) |
	                 DW_FC_CMD_3(cmd3) | DW_FC_CMD_4(cmd4),
	                 info, DW_FC_CMD1);

	dmw96_nfc_writel(DW_FC_CMD_5(cmd5) | DW_FC_CMD_6(cmd6),
	                 info, DW_FC_CMD2);

	dmw96_nfc_writel(info->data_buff_phys, info, DW_FC_DPR);
	dmw96_nfc_writel(info->data_buff_phys + mtd->writesize,
	                 info, DW_FC_RDPR);

	dmw96_nfc_configure_ready_timeout(info);
}

static void
dmw96_nfc_set_datacount(struct dmw96_nfc_info *info, int page_size,
                        int spare_size)
{
	int vp_size, n1, n2, n3, page_count;
	int transfer_size;
	int spare_words = ((info->n1len + info->n2len + 3) & ~3L) >> 2;
	uint32_t reg;

	n1 = n2 = n3 = page_count = 0;

	if (likely(info->use_ecc)) {
		vp_size = VIRTUAL_PAGESIZE;
		n1 = info->n1len;
		n2 = info->n2len;
		n3 = info->n3len;
		transfer_size = (page_size >> VIRTUAL_PAGESIZE_SHIFT);
		page_count = transfer_size - 1;
	} else {
		if (likely(info->mtd->writesize > 512)) {
			int spare_offset = page_size;

			if (info->chip->options & NAND_BUSWIDTH_16)
				spare_offset >>= 1;

			vp_size = spare_offset;
			n2 = spare_size;
			transfer_size = page_size;
		} else {
			vp_size = page_size;
			transfer_size = page_size + spare_size;
		}
	}

	dmw96_nfc_writel(DW_FC_DCOUNT_VP_SIZE(vp_size) |
	                 DW_FC_DCOUNT_SPARE_N3(n3) |
	                 DW_FC_DCOUNT_SPARE_N2(n2) |
	                 DW_FC_DCOUNT_SPARE_N1(n1) |
	                 DW_FC_DCOUNT_PAGE_CNT(page_count),
	                 info, DW_FC_DCOUNT);

	dmw96_nfc_writel(DW_FC_FTSR_TRANSFER_SIZE(transfer_size),
	                 info, DW_FC_FTSR);

	reg = dmw96_nfc_readl(info, DW_FC_PULSETIME);
	reg &= ~DW_FC_PT_SPARE_WORDS(0x3f);
	reg |= DW_FC_PT_SPARE_WORDS(spare_words);
	dmw96_nfc_writel(reg, info, DW_FC_PULSETIME);
}

static void
dmw96_nfc_select_chip(struct mtd_info *mtd, int chipnr)
{
	/*
	info = (struct dmw96_nfc_info *)
	                (((struct nand_chip *)mtd->priv)->priv);

	if (chipnr != -1)
		info->chipsel_nr = chipnr;
	*/
}

static void
dmw96_nfc_read_buf(struct mtd_info *mtd, uint8_t *buf, int len)
{
	struct nand_chip *chip = mtd->priv;
	struct dmw96_nfc_info *info = chip->priv;

	if (info->offset + len <= mtd->writesize + mtd->oobsize) {
		memcpy(buf, info->data_buff + info->offset, len);
		info->offset += len;
	}
}

static void
dmw96_nfc_write_buf(struct mtd_info *mtd, const uint8_t *buf, int len)
{
	struct nand_chip *chip = mtd->priv;
	struct dmw96_nfc_info *info = chip->priv;

	if (info->offset + len <= mtd->writesize + mtd->oobsize) {
		memcpy(info->data_buff + info->offset, buf, len);
		info->offset += len;
	}
}

static uint8_t
dmw96_nfc_read_byte(struct mtd_info *mtd)
{
	uint16_t val;

	dmw96_nfc_read_buf(mtd, (uint8_t *)&val, 1);
	return (uint8_t)val;
}

static void
dmw96_nfc_simple_command(struct dmw96_nfc_info *info, int command, int column,
                         int len)
{
	int sequence;

	sequence = DW_FC_SEQ_CMD1_EN | DW_FC_SEQ_WAIT1_EN |
	           DW_FC_SEQ_CHIP_SEL(info->chipsel_nr) |
	           DW_FC_SEQ_RDY_EN | DW_FC_SEQ_RDY_SEL(info->ready_nr);

	if (info->chip->options & NAND_BUSWIDTH_16)
		sequence |= DW_FC_SEQ_MODE16;

	if (likely(len))
		sequence |= DW_FC_SEQ_RW_EN | DW_FC_SEQ_DATA_READ_DIR;

	if (column != -1)
		sequence |= DW_FC_SEQ_ADD1_EN | DW_FC_SEQ_WAIT1_EN;

	dmw96_nfc_writel(sequence, info, DW_FC_SEQUENCE);

	if (column == -1)
		dmw96_nfc_writel(0, info, DW_FC_ADDR_COL);
	else
		dmw96_nfc_writel(column, info, DW_FC_ADDR_COL);

	dmw96_nfc_writel(0, info, DW_FC_DCOUNT);
	dmw96_nfc_writel(0, info, DW_FC_TIMEOUT);
	dmw96_nfc_writel(len, info, DW_FC_FTSR);
	dmw96_nfc_writel(command, info, DW_FC_CMD1);

	dmw96_nfc_writel(info->data_buff_phys, info, DW_FC_DPR);

	dmw96_nfc_start(info);
	dmw96_nfc_wait_trans_done(info);
}

static void
dmw96_nfc_reset(struct dmw96_nfc_info *info)
{
	dmw96_nfc_simple_command(info, NAND_CMD_RESET, -1, 0);
}

/* This function prepares the program transaction for small-page chips (<= 512 bytes) */
static int
dmw96_nfc_prepare_small_write(struct dmw96_nfc_info *info, int column)
{
	unsigned int res = column;
	unsigned int page_size = info->mtd->writesize;
	unsigned int read_cmd;
	unsigned int sequence;

	printk("%s()\n", __func__);

	if (page_size > 512)
		BUG();

	if (column >= page_size) {
		/* OOB area */
		res -= page_size;
		read_cmd = NAND_CMD_READOOB;
	} else if (column >= 256) {
		res -= 256;
		read_cmd = NAND_CMD_READ1;
	} else {
		read_cmd = NAND_CMD_READ0;
	}

	sequence = DW_FC_SEQ_CMD4_EN | DW_FC_SEQ_WAIT4_EN | DW_FC_SEQ_KEEP_CS |
	           DW_FC_SEQ_CHIP_SEL(info->chipsel_nr) |
	           DW_FC_SEQ_RDY_SEL(info->ready_nr);

	if (info->chip->options & NAND_BUSWIDTH_16)
		sequence |= DW_FC_SEQ_MODE16;

	dmw96_nfc_writel(sequence, info, DW_FC_SEQUENCE);
	dmw96_nfc_writel(DW_FC_CMD_1(0) | DW_FC_CMD_2(0) |
	                 DW_FC_CMD_3(0) | DW_FC_CMD_4(read_cmd),
	                 info, DW_FC_CMD1);

	dmw96_nfc_configure_ready_timeout(info);

	info->use_ecc = 0;
	dmw96_nfc_set_datacount(info, 0, 0);

	dmw96_nfc_start(info);
	dmw96_nfc_wait_trans_done(info);

	return res; /* return column; */
}

static void
dmw96_nfc_erase(struct dmw96_nfc_info *info, int page_addr)
{
	info->use_ecc = 0;

	dmw96_nfc_sequence(info, -1, page_addr, NAND_CMD_ERASE1);
	dmw96_nfc_writel(0, info, DW_FC_DCOUNT);
	dmw96_nfc_writel(0, info, DW_FC_FTSR);

	dmw96_nfc_start(info);
}

static void
dmw96_nfc_ecc_flush(struct dmw96_nfc_info *info)
{
	int ecc_reg = info->ecc_mode ? DW_FC_BCH : DW_FC_HAMMING;

	while (dmw96_nfc_readl(info, DW_FC_STATUS) & DW_FC_STATUS_ERR_FOUND)
		dmw96_nfc_readl(info, ecc_reg);
}

static void
dmw96_nfc_ecc_fix_bit(struct dmw96_nfc_info *info, int sequence, int location)
{
	u8 *p = info->data_buff;
	int byte, bit;

	bit   = location & 0x7;
	byte  = location >> 3;

	if (byte >= VIRTUAL_PAGESIZE) {
		/* we are fixing an ecc byte */
		byte += info->mtd->writesize - VIRTUAL_PAGESIZE;
		byte += sequence * (info->n1len + info->n2len);
	} else
		byte += sequence * VIRTUAL_PAGESIZE;

	if (byte >= info->mtd->writesize + info->mtd->oobsize) {
		printk("%s(): byte = %d\n", __func__, byte);
		//BUG();
		return;
	}

	p[byte] ^= (1 << bit);
}

static int
dmw96_nfc_ecc_fix_hamming(struct dmw96_nfc_info *info)
{
	int stat = 0;
	uint32_t hamming;
	int sequence;
	int location;

	while (info->ecc_numcodes) {
		hamming = info->ecc_codes[info->ecc_numcodes - 1];
		sequence = (hamming >> 14) & 0xf;
		location = hamming & 0x3fff;

		if ((hamming & 1) == 0) {
			info->ecc_numcodes = 0;
			return -1;
		}

		location = (location - 1) >> 1;

		dmw96_nfc_ecc_fix_bit(info, sequence, location);
		stat++;

		info->ecc_numcodes--;
	}

	return stat;
}

/* we use one of the reserved bits of FC_BCH for non-correctable indication */
#define DMW96_NFC_BCH_NCMASK (1 << 31)

static int
dmw96_nfc_ecc_fix_bch(struct dmw96_nfc_info *info)
{
	int stat = 0;
	uint32_t bch;
	int sequence, location;
	int vector_length;

	/* see suggested formula in spec */
	vector_length  = (4096 + (info->n1len * 8));
	vector_length += dmw96_nfc_eccbits(info->ecc_mode) * 13 - 1;

	while (info->ecc_numcodes) {
		bch = info->ecc_codes[info->ecc_numcodes - 1];

		sequence = (bch >> 13) & 0xf;
		location = bch & 0x1fff;

		location = vector_length - location;

		if ((bch & DMW96_NFC_BCH_NCMASK) || location < 0) {
			info->ecc_numcodes = 0;
			return -1;
		}

		dmw96_nfc_ecc_fix_bit(info, sequence, location);
		stat++;

		info->ecc_numcodes--;
	}

	return stat;
}

/*
 * this fetches the ecc codes during the transfer so that we can correct
 * erroneous bits with dmw96_nfc_ecc_fix_errors() after the data transfer to
 * the memory has finished
 */
static void
dmw96_nfc_ecc_fetch(struct dmw96_nfc_info *info)
{
	int code_reg = info->ecc_mode ? DW_FC_BCH : DW_FC_HAMMING;
	uint32_t status;
	int sequence;

	status = dmw96_nfc_readl(info, DW_FC_STATUS);

	/* special case for bch */
	if (unlikely(status & DW_FC_STATUS_ECC_NC_ERR)) {

		/* we manually craft and push a bch code here */
		sequence = (status >> 12) & 0xf;
		info->ecc_codes[0] = DMW96_NFC_BCH_NCMASK | sequence << 13;
		info->ecc_numcodes = 1;

		dmw96_nfc_writel(DW_FC_STATUS_ECC_NC_ERR, info, DW_FC_STS_CLR);
		dmw96_nfc_ecc_flush(info);
		return;
	}

	while (dmw96_nfc_readl(info, DW_FC_STATUS) & DW_FC_STATUS_ERR_FOUND) {
		info->ecc_codes[info->ecc_numcodes++] =
			dmw96_nfc_readl(info, code_reg);
	}
}

static void
dmw96_nfc_ecc_fix_errors(struct dmw96_nfc_info *info)
{
	struct mtd_info *mtd = info->mtd;
	int stat;

	/* fetch remaining ecc codes */
	dmw96_nfc_ecc_fetch(info);

	/* check if this is a blank page; if so, bail out */
	if (info->data_buff[mtd->writesize + info->dirtypos] == 0xff) {
		info->ecc_numcodes = 0;
		return;
	}

	if (info->ecc_mode == 0)
		stat = dmw96_nfc_ecc_fix_hamming(info);
	else
		stat = dmw96_nfc_ecc_fix_bch(info);

	if (stat < 0)
		mtd->ecc_stats.failed++;
	else
		mtd->ecc_stats.corrected += stat;
}

static void
dmw96_nfc_readpage(struct dmw96_nfc_info *info, int column, int page_addr)
{
	int datalen, sparelen;

	if (column >= info->mtd->writesize) {
		info->use_ecc = 0;
		datalen = info->mtd->oobsize;
		sparelen = 0;
		dmw96_nfc_sequence(info, 0, page_addr, NAND_CMD_READOOB);
		dmw96_nfc_set_datacount(info, datalen, sparelen);
	} else {
		info->use_ecc = info->ecc;
		datalen = info->mtd->writesize;
		sparelen = info->mtd->oobsize;
		dmw96_nfc_sequence(info, 0, page_addr, NAND_CMD_READ0);
		dmw96_nfc_set_datacount(info, datalen, sparelen);
	}

	dmw96_nfc_start(info);
	dmw96_nfc_wait_trans_done(info);
	dmw96_nfc_writel(1, info, DW_FC_STS_CLR);

	/*
	 * ATTENTION: The following call will guarantee that all memory
	 * transactions done by the device have actually hit the memory.
	 */
	ccu_barrier();

	if (dmw96_nfc_check_timeout(info)) {
		printk("%s(): nand timeout, unrecoverable!\n", __func__);
		while(1);
	}

	if (info->use_ecc)
		dmw96_nfc_ecc_fix_errors(info);
}

static void
dmw96_nfc_writepage(struct dmw96_nfc_info *info, int column, int page_addr)
{
	int datalen, sparelen;

	if (info->mtd->writesize <= 512)
		column = dmw96_nfc_prepare_small_write(info, column);

	if (column >= info->mtd->writesize) {
		info->use_ecc = 0;
		datalen = info->mtd->oobsize;
		sparelen = 0;
		dmw96_nfc_set_datacount(info, datalen, sparelen);
	} else {
		info->use_ecc = info->ecc;
		datalen = info->mtd->writesize;
		sparelen = info->mtd->oobsize;
		dmw96_nfc_set_datacount(info, datalen, sparelen);
	}

	dmw96_nfc_sequence(info, column, page_addr, NAND_CMD_SEQIN);
}

static void
dmw96_nfc_complete_writepage(struct dmw96_nfc_info *info)
{
	dmw96_nfc_start(info);
}

static void
dmw96_nfc_cmdfunc(struct mtd_info *mtd, unsigned command,
                  int column, int page_addr)
{
	struct nand_chip *chip;
	struct dmw96_nfc_info *info;

	if (!mtd)
		return;

	chip = mtd->priv;
	info = chip->priv;

	if (command != NAND_CMD_PAGEPROG)
		info->offset = 0;

	switch(command) {
	case NAND_CMD_RESET:
		dmw96_nfc_reset(info);
		break;
	case NAND_CMD_READID:
		dmw96_nfc_simple_command(info, command, column, 8);
		break;
	case NAND_CMD_PARAM:
		dmw96_nfc_simple_command(info, command, 0, 256*3);
		break;
	case NAND_CMD_STATUS:
		dmw96_nfc_simple_command(info, command, -1, 1);
		break;
	case NAND_CMD_READ0:
		dmw96_nfc_readpage(info, column, page_addr);
		break;
	case NAND_CMD_READOOB:
		dmw96_nfc_readpage(info, mtd->writesize, page_addr);
		break;
	case NAND_CMD_SEQIN:
		dmw96_nfc_writepage(info, column, page_addr);
		break;
	case NAND_CMD_PAGEPROG:
		dmw96_nfc_complete_writepage(info);
		break;
	case NAND_CMD_ERASE1:
		dmw96_nfc_erase(info, page_addr);
		break;
	case NAND_CMD_ERASE2:
		break;
	default:
		printk(KERN_WARNING "unsupported command 0x%x\n", command);
		break;
	}
}

static int
dmw96_nfc_read_page_hwecc(struct mtd_info *mtd, struct nand_chip *chip,
                          uint8_t *buf, int page)
{
	dmw96_nfc_read_buf(mtd, buf, mtd->writesize);
	dmw96_nfc_read_buf(mtd, chip->oob_poi, mtd->oobsize);

	return 0;
}

static void
dmw96_nfc_write_page_hwecc(struct mtd_info *mtd, struct nand_chip *chip,
                           const uint8_t *buf)
{
	struct dmw96_nfc_info *info = chip->priv;

	/* set dirty marker */
	chip->oob_poi[info->dirtypos] = 0;

	dmw96_nfc_write_buf(mtd, buf, mtd->writesize);
	dmw96_nfc_write_buf(mtd, chip->oob_poi, mtd->oobsize);
}

static int
dmw96_nfc_nand_wait(struct mtd_info *mtd, struct nand_chip *chip)
{
	struct dmw96_nfc_info *info = (struct dmw96_nfc_info *)chip->priv;
	int status;

	dmw96_nfc_wait_trans_done(info);

	if (dmw96_nfc_check_timeout(info)) {
		printk("%s(): nand timeout, unrecoverable!\n", __func__);
		while(1);
	}

	status = dmw96_nfc_readl(info, DW_FC_FBYP_DATA);
	if (!(status & NAND_STATUS_READY))
		printk("%s(): status not ready - OMFG\n", __func__);

	return status;
}

static uint16_t
dmw96_nfc_crc16(void *buf, unsigned long len)
{
	const uint32_t crcinit = 0x4f4e; /* initial crc value */
	const uint32_t polynom = 0x8005;
	uint32_t i, j, c;
	uint8_t *l_buffer = buf;
	uint32_t bit, crc = crcinit;
	uint32_t crcmask = 0xffff, crchighbit = (uint32_t)(1 << 15);

	for (i = 0; i < len; i++) {
		c = l_buffer[i];
		for (j = 0x80; j; j >>= 1) {
			bit = crc & crchighbit;
			crc <<= 1;
			if (c & j)
				bit ^= crchighbit;
			if (bit)
				crc ^= polynom;
		}
		crc &= crcmask;
	}
	return crc;
}

static void
dmw96_nfc_erase_cmd(struct mtd_info *mtd, int page)
{
	dmw96_nfc_cmdfunc(mtd, NAND_CMD_ERASE1, -1, page);
}

extern void nand_set_defaults(struct nand_chip *chip, int busw);

static int
dmw96_nfc_identify_onfi(struct dmw96_nfc_info *info)
{
	struct mtd_info *mtd = info->mtd;
	struct nand_chip *chip = info->chip;
	unsigned char *buf;
	int res = -ENODEV;
	int i;

	nand_set_defaults(chip, chip->options & NAND_BUSWIDTH_16);

	dmw96_nfc_cmdfunc(info->mtd, NAND_CMD_READID, 0x20, -1);

	if ((info->data_buff[0] != 'O') ||
	    (info->data_buff[1] != 'N') ||
	    (info->data_buff[2] != 'F') ||
	    (info->data_buff[3] != 'I'))
		goto out;

	dmw96_nfc_cmdfunc(info->mtd, NAND_CMD_PARAM, 0x00, -1);

	/*
	 * step through each copy of the params and find the first one
	 * with matching crc
	 */
	for (i = 0; i < 3; i++) {
		uint16_t crc;
		buf = info->data_buff + 256 * i;

		crc = buf[255] << 8 | buf[254];
		if (dmw96_nfc_crc16(buf, 254) == crc) {
			i = 0;
			break;
		}
	}

	if (i) {
		printk("ONFI chip detected, but could not read info\n");
		res = -EIO;
		goto out;
	}

	if (buf[6] & 0x1)
		chip->options |= NAND_BUSWIDTH_16;

	mtd->writesize =  buf[80] | (buf[81] << 8) |
	                 (buf[82] << 16) | (buf[83] << 24);
	mtd->oobsize   =  buf[84] | (buf[85] << 8);
	mtd->erasesize = (buf[92] | (buf[93] << 8) |
	                 (buf[94] << 16) | (buf[95] << 24))
	                 * info->mtd->writesize;
	chip->chipsize = (buf[96] | (buf[97] << 8) |
	                 (buf[98] << 16) | (buf[99] << 24));
	chip->chipsize *= buf[100] * info->mtd->erasesize;

	res = dmw96_nfc_eccfind(buf[112]);
	if (res < 0) {
		printk(KERN_ERR
		       "Unsupported number of correctable bits: %d\n",
		       buf[112]);
		goto out;
	}
	info->ecc_mode = res;

	chip->page_shift = ffs(mtd->writesize) - 1;
	chip->pagemask = (chip->chipsize >> chip->page_shift) - 1;

	chip->bbt_erase_shift = chip->phys_erase_shift =
		ffs(mtd->erasesize) - 1;
	if (chip->chipsize & 0xffffffff)
		chip->chip_shift = ffs((unsigned)chip->chipsize) - 1;
	else
		chip->chip_shift = ffs((unsigned)(chip->chipsize >> 32)) + 31;

	chip->badblockpos = mtd->writesize > 512 ?
		NAND_LARGE_BADBLOCK_POS : NAND_SMALL_BADBLOCK_POS;

	/* erase all options except the buswidth */
	chip->options &= ~(NAND_CHIPOPTIONS_MSK & ~NAND_BUSWIDTH_16);

	/* these options are from nand_ids.c (LP_OPTIONS) */
	chip->options |= NAND_NO_READRDY | NAND_NO_AUTOINCR;

	/* set flags for samsung largepage devices */
	if (buf[64] == NAND_MFR_SAMSUNG && mtd->writesize > 512)
		chip->options |= NAND_SAMSUNG_LP_OPTIONS;

	/*
	 * normally this gets set to single_erase_cmd in nand_base.c.
	 * however it is declared as static so we reimplement it in a
	 * driver specific version which also saves one call to cmdfunc
	 * and one indirect branch
	 */
	chip->erase_cmd = dmw96_nfc_erase_cmd;

	/* TODO: support chip array as nand_scan_ident() does */
	chip->numchips = 1;
	mtd->size = 1 * chip->chipsize;

	res = 0;
out:
	return res;
}

void dmw96_nfc_init(struct mtd_info *mtd);

int dmw96_nfc_scan(struct mtd_info *mtd, int maxchips)
{
	struct nand_chip *chip = mtd->priv;
	struct dmw96_nfc_info *info;
	int err;

#ifdef CONFIG_ARCH_VERSATILE_BROADTILE
	/* wait for fpga */
	udelay(100000);
#endif

	dmw96_nfc_init(mtd);
	info = chip->priv;

	dmw96_nfc_reset(info);

	/* scan for ONFI chip */
	chip->options = 0;
	err = dmw96_nfc_identify_onfi(info);
	if (!err)
		goto detected;
	if (err != -ENODEV)
		return err;

	/* scan for flash using the device id */
	chip->options = 0;
	err = nand_scan_ident(mtd, 1, NULL);
	if (!err)
		goto known_id;

	return -ENODEV;

known_id:
	if (!(chip->cellinfo & NAND_CI_CELLTYPE_MSK))
		info->ecc_mode = 0;
	else {
		info->ecc_mode = 1;
		/* ONFI 2.2, "spare size recommendations" (p. 202): */
		if (((mtd->writesize == 2048) && (mtd->oobsize > 64)) ||
		    ((mtd->writesize == 4096) && (mtd->oobsize > 128)) ||
		    ((mtd->writesize == 8192) && (mtd->oobsize > 256)))
			info->ecc_mode = 3;
		/* FIXME: when should we use ecc_mode 2 (8 correctable bits) */
	}

detected:
	/* TODO: remove */
	//info->ecc_mode = 1;

	/* we can only calculate the ECC per page, not per sub-page */
	chip->options |= NAND_NO_SUBPAGE_WRITE;

	if (mtd->writesize <= 512) {
		printk(KERN_WARNING "%s(): warning: ecc not possible with "
		       "small page devices\n", __func__);
		info->ecc = info->use_ecc = 0;
	}

	info->dirtypos = (chip->badblockpos & ~1) + 2;
	info->n1len = info->dirtypos + 2;
	if (info->n1len > 12) {
		printk("invalid N1 length setting\n");
		BUG();
	}

	info->n3len = info->n1len - 2; /* protect dirty marker */
	info->n2len = mtd->oobsize / (mtd->writesize >> VIRTUAL_PAGESIZE_SHIFT);
	info->n2len -= info->n1len;

	err = dmw96_nfc_gen_ecclayout(info, &dmw96_nfc_ecclayout);
	if (err) {
		printk(KERN_WARNING "Could not generate oob scheme "
		                    "(oobsize: %d, %dbit ecc)\n", mtd->oobsize,
		       info->ecc_mode ? info->ecc_mode << 2 : 1);
		return err;
	}

	chip->ecc.layout = &dmw96_nfc_ecclayout;
	chip->ecc.bytes = chip->ecc.layout->eccbytes;
	chip->ecc.size = VIRTUAL_PAGESIZE;
	chip->ecc.read_page = dmw96_nfc_read_page_hwecc;
	chip->ecc.write_page = dmw96_nfc_write_page_hwecc;

	err = nand_scan_tail(mtd);
	return err;
}

static char dmw96_nfc_databuf[NAND_DATABUF_SIZE] __attribute__((aligned(4)));
static struct dmw96_nfc_info nfc_info;

void
dmw96_nfc_init(struct mtd_info *mtd)
{
	struct dmw96_nfc_info *info = &nfc_info;
	struct nand_chip *chip = mtd->priv;

	/*   Initialize write size and oob size*/
	/*   dmw96_nfc_read_buf fails without this fix*/
	/*   For non ONFI flashes, read ID scenario */

	mtd->writesize = NAND_MAX_PAGESIZE;
	mtd->oobsize = NAND_MAX_OOBSIZE;

	info->mtd = mtd;
	info->chip = chip;

#ifdef CONFIG_ARCH_VERSATILE_BROADTILE
	info->data_buff = (void *)0x96000000;

	/* the FPGA bus has an offset of 0x80000000 */
	info->data_buff_phys = (dma_addr_t)info->data_buff - 0x80000000;
#else
	info->data_buff = (void *)dmw96_nfc_databuf;
	info->data_buff_phys = (dma_addr_t)info->data_buff;
#endif

	info->wait_ready_0_fall = WAIT_READY_0_FALL;
	info->wait_ready_0_rise = WAIT_READY_0_RISE;
	info->wait_ready_1_fall = WAIT_READY_1_FALL;
	info->wait_ready_1_rise = WAIT_READY_1_RISE;
	info->wait_ready_2_fall = WAIT_READY_2_FALL;
	info->wait_ready_2_rise = WAIT_READY_2_RISE;
	info->wait_ready_3_fall = WAIT_READY_3_FALL;
	info->wait_ready_3_rise = WAIT_READY_3_RISE;
	info->wait_ready_4_fall = WAIT_READY_4_FALL;
	info->wait_ready_4_rise = WAIT_READY_4_RISE;
	info->wait_ready_5_fall = WAIT_READY_5_FALL;
	info->wait_ready_5_rise = WAIT_READY_5_RISE;
	info->wait_ready_6_fall = WAIT_READY_6_FALL;
	info->wait_ready_6_rise = WAIT_READY_6_RISE;

	info->pulse_ale_setup   = PULSE_ALE_SETUP;
	info->pulse_cle_setup   = PULSE_CLE_SETUP;
	info->pulse_wr_high     = PULSE_WR_HIGH;
	info->pulse_wr_low      = PULSE_WR_LOW;
	info->pulse_rd_high     = PULSE_RD_HIGH;
	info->pulse_rd_low      = PULSE_RD_LOW;

	info->ready_timeout     = READY_TIMEOUT;
	info->regs              = (void *)CONFIG_SYS_NAND_BASE;

	/* initialise chip data struct */
	chip->cmdfunc     = dmw96_nfc_cmdfunc;
	chip->read_byte   = dmw96_nfc_read_byte;
	chip->read_buf    = dmw96_nfc_read_buf;
	chip->write_buf   = dmw96_nfc_write_buf;
	chip->select_chip = dmw96_nfc_select_chip;
	chip->waitfunc    = dmw96_nfc_nand_wait;
	chip->ecc.mode    = NAND_ECC_HW;

	if (use_ecc)
		info->ecc = 1;
	else
		info->ecc = 0;
	info->use_ecc = info->ecc;

	/* initialize the hardware */
	info->chipsel_nr = 0;
	info->ready_nr = 1;
	chip->priv = info;


	dmw96_nfc_clear_status(info);
	dmw96_nfc_set_fixed_regs(info);
}
