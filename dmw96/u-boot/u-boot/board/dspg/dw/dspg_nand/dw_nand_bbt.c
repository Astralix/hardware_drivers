/*
 * drivers/mtd/dw_nand_block_markbad
 *
 * Author: moshe Lazarov <moshel@dsp.co.il>
 *
 * DSPG's DW NAND Flash driver.
 *
 *
 * $Id: diskonchip.c,v 1.1 2006/09/21 07:28:59 odedg Exp $
 */

#include "dw_nfls_cntl_pub.h"

#if 0
static uint8_t ff_pattern[] = { 0xff, 0xff };

static struct nand_bbt_descr bbt_main_descr = {
	.options = NAND_BBT_CREATE | NAND_BBT_WRITE | NAND_BBT_2BIT | NAND_BBT_VERSION ,
//    .options = 0, // R.B. 17/09/2008
    .offs =	0,
    .len = 2,
//    .pattern = ff_pattern // R.B. 17/09/2008
    .pattern = ff_pattern
};
#endif

static uint8_t scan_ff_pattern[] = { 0xff, 0xff };

static struct nand_bbt_descr smallpage_memorybased = {
        .options = NAND_BBT_SCAN2NDPAGE,
        .offs = 5,
        .len = 1,
        .pattern = scan_ff_pattern
};

static struct nand_bbt_descr largepage_memorybased = {
        .options = 0,
        .offs = 0,
        .len = 2,
        .pattern = scan_ff_pattern
};


static int dw_create_bbt(struct mtd_info *mtd, uint8_t *buf, struct nand_bbt_descr *bd, int chip);
static inline int dw_nand_memory_bbt(struct mtd_info *mtd, struct nand_bbt_descr *bd);
int dw_nand_scan_bbt(struct mtd_info *mtd);
int dw_nand_isbad_bbt(struct mtd_info *mtd, loff_t offs, int allowbbt);

extern Y_FlsDevTbl V_FlsDevTbl[NFC_K_FlsDevMAX];



static int dw_check_short_pattern(uint8_t *buf, struct nand_bbt_descr *td)
{
	int i;
	uint8_t *p = buf;

	//Compare the pattern
	for (i = 0; i < td->len; i++)
    {
		if (p[td->offs + i] != td->pattern[i])
			return -1;
	}
	return 0;
}


static int dw_create_bbt(struct mtd_info *mtd, uint8_t *buf, struct nand_bbt_descr *bd, int chip)
{
    struct nand_chip *nand = mtd->priv;
    int i, j, numblocks, len, ret = 0;
    int startblock;
    loff_t from;
	int total_bads = 0;
    struct mtd_oob_ops ops;

    //Scan 1st and 2nd pages of each block ?
//	len = 2; // R.B.: FIX
	
	// len here is the number of pages we should scan in each block.
    if (bd->options & NAND_BBT_SCAN2NDPAGE)
            len = 2;
    else
            len = 1;

	
	//Note that numblocks is 2 * (real numblocks) here;
	//see i += 2 below as it makses shifting and masking less painful
	numblocks = mtd->size >> (nand->bbt_erase_shift - 1);
	startblock = 0;
	from = 0;

	// Note the real number of the block is i/2.
	for (i = startblock; i < numblocks; )
    {
		for (j = 0; j < len; j++)
        {
		ops.ooblen = mtd->oobsize;
		ops.oobbuf = buf;
		ops.ooboffs = 0;
		ops.datbuf = NULL;
		ops.mode = MTD_OOB_RAW;
		//ops.len = len;

			//No need to read pages fully, just read required OOB bytes
            ret = mtd->read_oob(mtd, from + j * mtd->writesize, &ops);

			if (ret)
				return ret;

			if (dw_check_short_pattern(buf, bd))
            {
				nand->bbt[i >> 3] |= 0x03 << (i & 0x6);
				printk(KERN_WARNING "dw_create_bbt: Bad eraseblock %d at 0x%08x\n",
					i >> 1, (unsigned int) from);
				break;
			}
		}
		i += 2;
		from += (1 << nand->bbt_erase_shift);
	}
	printk(KERN_WARNING "dw_create_bbt: Total bad eraseblocks: %d\n",
		total_bads);


	return 0;
}


static inline int dw_nand_memory_bbt(struct mtd_info *mtd, struct nand_bbt_descr *bd)
{
	struct nand_chip *nand = mtd->priv;

    bd->options &= ~NAND_BBT_SCANEMPTY;

	return dw_create_bbt(mtd, nand->buffers->databuf, bd, -1);
}


int dw_nand_isbad_bbt(struct mtd_info *mtd, loff_t offs, int allowbbt)
{
    struct nand_chip *nand = mtd->priv;
    int block;
    uint8_t	res;

    //Get block number * 2
    block = (int)(offs >> (nand->bbt_erase_shift - 1));
    res = (nand->bbt[block >> 3] >> (block & 0x06)) & 0x03;

    /*DEBUG (MTD_DEBUG_LEVEL2, "\ndw_nand_isbad_bbt(): bbt info for offs 0x%08x: (block %d) 0x%02x\n",
        (unsigned int)offs, block >> 1, res);*/

    switch ((int)res)
    {
        case 0x00:	return 0;
        case 0x01:	return 1;
        case 0x02:	return allowbbt ? 0 : 1;
    }
    return 1;
}


int dw_nand_scan_bbt(struct mtd_info *mtd)
{
    struct nand_chip *nand = mtd->priv;
    int len, ret = 0;

    NFC_E_Chip ChipType;
    NFC_E_FlsDev FlsDev = NFC_K_FlsDev0;

    //printk("\ndw_nand_scan_bbt\n");

    ChipType = V_FlsDevTbl[FlsDev].ChipType;

    len = mtd->size >> (nand->bbt_erase_shift + 2);

    //Allocate memory (2bit per block)
    nand->bbt = kmalloc(len, GFP_KERNEL);
    if(!nand->bbt)
    {
        printk(KERN_ERR "\ndw_nand_scan_bbt: Out of memory\n");
        return -ENOMEM;
    }

    //Clear the memory bad block table
    memset(nand->bbt, 0x00, len);

    if (!nand->badblock_pattern) {
            nand->badblock_pattern = (mtd->writesize > 512) ?
                    &largepage_memorybased : &smallpage_memorybased;
	}

//    if((ret = dw_nand_memory_bbt(mtd, &bbt_main_descr))) // R.B.
#ifndef CONFIG_MTD_NAND_DW_SKIP_BBT
    if((ret = dw_nand_memory_bbt(mtd, nand->badblock_pattern)))
    {
        printk(KERN_ERR "dw_nand_scan_bbt: Can't scan flash and build the RAM-based BBT\n");
        kfree(nand->bbt);
        nand->bbt = NULL;
    }
#endif
    return ret;
}
