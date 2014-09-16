/*
 * drivers/mtd/dspg_nand/dw_nand.c
 *
 * Copyright (C) 2009 DSP Group Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "dw_nfls_cntl_pub.h"
#include "dw74fb-mpmc.h"

extern void lcd_toggle(void);
static int lcd_counter = 0;

#define DRV_NAME	"dw_flash_nand"

/*
 * Define partitions for flash device
 */
#define DEFAULT_NUM_PARTITIONS 1

int IsEcc = 1;

unsigned long dw_nand_offset;
u8            dw_nand_access_mode;

#ifdef __LINUX_KERNEL__
static struct mtd_partition dw_nand_default_partition_info[] =
{
	{
	.name = "jffs2",
	.offset = 0,
	.size = 128 * 1024 * 1024,
	}
};
#endif

#ifdef CONFIG_MTD_PARTITIONS
const char *part_probes[] = { "cmdlinepart", NULL };
#endif

static struct nand_ecclayout dw_nand_oob_16 = {
	.eccbytes = 15,
	.eccpos = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14},
	.oobfree = { {15, 1} }
};

static struct nand_ecclayout dw_nand_oob_32 = {
	.eccbytes = 30,
	.eccpos = {
	    0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
	   16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30},
	.oobfree = { {15, 1}, {31, 1} }
};

static struct nand_ecclayout dw_nand_oob_64 = {
	.eccbytes = 36,
	.eccpos = { 0 },  /* Not in use  - FIXME: not enough space to hold 512/9 ECC scheme. */
	.oobfree = { {15, 1}, {31, 1}, {47, 1}, {63, 1} }
};


void	dw_nand_cmdfunc(struct mtd_info *mtd, unsigned command,
					      int column, int page_addr);
void	dw_nand_read_buf(struct mtd_info *mtd, uint8_t *buf, int len);
void	dw_nand_write_buf(struct mtd_info *mtd, const uint8_t *buf, int len);
uint8_t dw_nand_read_byte(struct mtd_info *mtd);
void	dw_nand_select_chip(struct mtd_info *mtd, int chipnr);
struct	nand_flash_dev *nand_get_flash_type(struct mtd_info *mtd, struct nand_chip *chip,
						  int busw, int *maf_id);
void    dw_nand_flush(struct dw_nand_info *info);

static int dw_nand_kernel_errs(NFC_E_Err retCode)
{
	int res;

	switch (retCode) 
	{
		case NFC_K_Err_OK:
			res = 0;
			break;
		case NFC_K_Err_InvalidChip:
			res = -EINVAL;
			break;
		case NFC_K_Err_DfctBlk:
			BUG(); /* Not in use! */
			/* If it was used, this is the correct answer */
			res = -EIO;
			break;
		case NFC_K_Err_PageBoundary:
			BUG(); /* Not in use! */
			/* If it was used, this is the correct answer */
			res = -EINVAL;
			break;
		case NFC_K_Err_ProgFail:
			res = -EIO;
			break;
		case NFC_K_Err_EraseFail:
			res = -EIO;
			break;
		case NFC_K_Err_TimeOut:
			/* Subtle: this case is not properly handled in current kernel */
			res = -EIO;
			break;
		case NFC_K_Err_InvalidParam:
			res = -EINVAL;
			break;
		case NFC_K_Err_SwErr:
			BUG(); /* Not in use! */
			/* If it was used, this is the correct answer */
			res = -EINVAL;
			break;
		case NFC_K_Err_EccFailed:
			res = -EBADMSG;
			break;
		default:
			BUG();
			res = -EINVAL;
			break;
		/* We lack a verify-after-write function result, which may
		 * return -EFAULT. */
	}
	return res;
}

void *internal_ram;
#define INTERNAL_SRAM_VIRT_ADDRESS internal_ram 

#ifdef MACH_VERSATILE_BROADTILE
 #define DW_SSRAM_BASE 0x60000000
#endif

#ifdef CONFIG_MTD_NAND_DW_DMA_THROUGH_SSRAM
unsigned char * global_data_buf;
#endif

//#ifdef CONFIG_MTD_NAND_DW_DMA_THROUGH_SSRAM
//#ifdef __LINUX_KERNEL__
void *virt2phys(void * virt)
{
	unsigned int mask_16kiB = 0x3fff;

	if (((unsigned int)virt & ~mask_16kiB) != (unsigned int )INTERNAL_SRAM_VIRT_ADDRESS) {
		BUG();
	}

	return (void *)(DW_SSRAM_BASE + ((unsigned int)virt & mask_16kiB));
}
//#endif
/*#else
void *virt2phys(void *virt)
{
	return virt;
}
#endif
*/
void dw_nand_cmdfunc(struct mtd_info *mtd, unsigned command,
			      int column, int page_addr)
{
	NFC_E_Err err;
	struct dw_nand_info *nand_info = NULL;
	struct nand_chip *chip = NULL;

	if (mtd) {
		chip      = mtd->priv;
		nand_info = (struct dw_nand_info*) chip->priv; 
	}

	if (command == NAND_CMD_RESET) {
		dw_nand_access_mode = 1;
		dw_nand_flush(nand_info);
		dw_nand_cntl_reset(nand_info);
	}

	if (command == NAND_CMD_READID) {
		dw_nand_access_mode = 1;
		dw_nand_flush(nand_info);
		dw_nand_cntl_readid(nand_info);
	}

	if (command == NAND_CMD_STATUS) {
		dw_nand_access_mode = 1;
		dw_nand_flush(nand_info);
		dw_nand_cntl_status(nand_info);
	}

	if (command == NAND_CMD_PAGEPROG) {
#ifndef CONFIG_MTD_NAND_DW_DMA_THROUGH_SSRAM
#else
		err = dw_nand_CompleteWritePageAndSpare(nand_info);
#endif
	}

	if ((command == NAND_CMD_SEQIN) || (command == NAND_CMD_ERASE1) ||
	    (command == NAND_CMD_READ0 )) {
		__u16 block = (__u16) (page_addr >> (chip->phys_erase_shift - chip->page_shift));
		
		if(command == NAND_CMD_ERASE1) {
			dw_nand_flush(nand_info);
			err = F_CmdEraseBlkBypass(nand_info, block);
			//printk("NAND-erase returns 0x%x @block 0x%x\n", err, block);
		} else {
			__u8 PageInBlk;
			void *DataP, *SpareP;
			__u8 N1Len     = 6;
			//int IsEcc     = 1;
			dw_nand_offset = 0;

#ifdef CONFIG_MTD_NAND_DW_DMA_THROUGH_SSRAM
			DataP         = global_data_buf;
			SpareP        = global_data_buf + mtd->writesize;
			memset(global_data_buf, 0, mtd->writesize + mtd->oobsize);
			
			/* if(chip->ecc.mode != NAND_ECC_HW)
				IsEcc = 0; */
#endif
			PageInBlk = (__u8) (page_addr & ((1 << (chip->phys_erase_shift - chip->page_shift)) - 1));

			if(command == NAND_CMD_SEQIN)
			{
				dw_nand_flush(nand_info);
				err = dw_nand_prepareWritePageAndSpare(nand_info, block, mtd->writesize, mtd->oobsize,
                                                PageInBlk, N1Len, virt2phys(DataP), virt2phys(SpareP), IsEcc );
			}

			else if(command == NAND_CMD_READ0) {
				dw_nand_access_mode = 0;
				
#ifndef CONFIG_MTD_NAND_DW_DMA_THROUGH_SSRAM
				dw_nand_prepare_readpage(nand_info, block, mtd->writesize, mtd->oobsize, PageInBlk, N1Len, IsEcc);
#else
				dw_nand_flush(nand_info);
				F_CmdReadPageAndSpare(nand_info, block, mtd->writesize, mtd->oobsize, PageInBlk, N1Len, IsEcc, virt2phys(DataP), virt2phys(SpareP), DataP, SpareP);
#endif
			}
		}
	}

	switch (command) {
	case NAND_CMD_CACHEDPROG:
	case NAND_CMD_PAGEPROG:
	case NAND_CMD_ERASE1:
	case NAND_CMD_ERASE2:
	case NAND_CMD_SEQIN:
	case NAND_CMD_RNDIN:
	case NAND_CMD_STATUS:
	case NAND_CMD_DEPLETE1:
	case NAND_CMD_RNDOUT:
	case NAND_CMD_STATUS_ERROR:
	case NAND_CMD_STATUS_ERROR0:
	case NAND_CMD_STATUS_ERROR1:
	case NAND_CMD_STATUS_ERROR2:
	case NAND_CMD_STATUS_ERROR3:
	default:
		/* printk("command 0x%x\n", command); */
		break;
	}
}

/*
 * BYPASS mode for buffer writing and reading
 */
void dw_nand_read_buf(struct mtd_info *mtd, uint8_t *buf, int len)
{
#ifdef CONFIG_MTD_NAND_DW_DMA_THROUGH_SSRAM
	if(unlikely(dw_nand_access_mode)) {
#endif
		struct dw_nand_info *nand_info = (struct dw_nand_info*) (((struct nand_chip*) mtd->priv)->priv);
		
		F_ReadByPass(nand_info,buf, len);
		return;

#ifdef CONFIG_MTD_NAND_DW_DMA_THROUGH_SSRAM
	}
	if(dw_nand_offset < len + mtd->writesize + mtd->oobsize) {
		memcpy(buf, global_data_buf + dw_nand_offset, len);
		dw_nand_offset += len;
	}
#endif
}

void dw_nand_write_buf(struct mtd_info *mtd, const uint8_t *buf, int len)
{
#ifdef CONFIG_MTD_NAND_DW_DMA_THROUGH_SSRAM
	if(dw_nand_offset < len + mtd->writesize + mtd->oobsize) {
		memcpy(global_data_buf + dw_nand_offset, buf, len);
		dw_nand_offset += len;
	}
#endif
}

uint8_t dw_nand_read_byte(struct mtd_info *mtd)
{
	uint8_t val;
	
	dw_nand_read_buf(mtd, &val, 1);
	return val;
}

int dw_nand_read_page_hwecc(struct mtd_info *mtd, struct nand_chip *chip,
				  uint8_t *buf)
{
	chip->read_buf(mtd, buf, mtd->writesize);
	chip->read_buf(mtd, chip->oob_poi, mtd->oobsize);

	return 0;
}

void dw_nand_write_page_hwecc(struct mtd_info *mtd, struct nand_chip *chip,
				  const uint8_t *buf)
{
	lcd_counter++;
	chip->write_buf(mtd, buf, mtd->writesize);
	chip->write_buf(mtd, chip->oob_poi, mtd->oobsize);
	if ((lcd_counter % 100) == 0)
		lcd_toggle();
}

/* R.B.: According to nand layer, this function gives raw write access to the
 * oob part of the media. The oob data is stored WITHOUT ECC.
 * to might be between (page boundary) and (page boundary + oobsize-1).
 * len is restricted by SINGLE oob area.
*/
static int dw_nand_write_oob(struct mtd_info *mtd, struct nand_chip *chip,
			int page)
{
	struct nand_chip *nand = mtd->priv;
	struct dw_nand_info *nand_info = (struct dw_nand_info*) nand->priv;
	int ret = 0;
	char FfTempbuf[128];
	int column;

	__u16 Block, DataLen;
	__u8 PageInBlk;

	dw_nand_flush(nand_info);

	memset(FfTempbuf, 0xff, mtd->oobsize);

	Block = (__u16) (page >> (nand->phys_erase_shift - nand->page_shift));
	PageInBlk = (__u8) (page & ((1 << (nand->phys_erase_shift - nand->page_shift)) - 1));
	column = page & (mtd->oobsize -1);

	/*
	 * Note on write method. In most FLASH chips it IS possible to write
	 * in OOB area from any position. However, some old chips disallow
	 * that and require padding the written data with 0xFF. In order to
	 * keep the code simple (remembering that the minimal transaction
	 * length is 2 bytes in NFLC), we always pad the data and write the
	 *whole OOB area.
	*/
	memcpy(&FfTempbuf[column], nand->oob_poi, mtd->oobsize); 

	DataLen = mtd->oobsize;
	ret = (int)F_CmdWriteOOBBypass(nand_info, Block, PageInBlk,
	                               DataLen, FfTempbuf);
	if (ret) {
		printk("dspg_nand_write_oob: write failed = %d\n", ret);
		goto out;
	}

out:
	return dw_nand_kernel_errs(ret);
}

/* R.B.: According to nand layer, this function gives raw read access to the
 * oob part of the media. The oob data is read WITHOUT ECC.
 * to might be between (page boundary) and (page boundary + oobsize-1). 
 * len is NOT restricted by SINGLE oob area.
 * FIXME: to be checked and updated.
*/
static int dw_nand_read_oob(struct mtd_info *mtd, struct nand_chip *chip,
			int page, int sndcmd)
{
	struct nand_chip *nand = mtd->priv;
	struct dw_nand_info *nand_info = (struct dw_nand_info*) nand->priv;
	int ret = 0;

	__u16 Block, DataLen, Column;
	__u8 PageInBlk;
	__u8 *DataP = chip->oob_poi;

	dw_nand_flush(nand_info);

	Block = (__u16) (page >> (nand->phys_erase_shift - nand->page_shift));
	PageInBlk = (__u8) (page & ((1 << (nand->phys_erase_shift - nand->page_shift)) - 1));

	/* Get raw starting column */
	Column = mtd->writesize & (mtd->oobsize - 1);

	DataLen = mtd->oobsize;
//	printk("reading OOB 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, \n",ChipType, FlsDev, Block, Column, PageInBlk,
//                                      DataLen, DataP);
	ret = (int)F_CmdReadOOBBypass(nand_info, Block, Column, PageInBlk,
                                      DataLen, DataP);
//	F_CmdReadPageAndSpare(nand_info, block, mtd->writesize, mtd->oobsize, PageInBlk, N1Len, IsEcc, virt2phys(DataP), virt2phys(SpareP), DataP, SpareP);

//	ret = (int) F_CmdReadPageAndSpare(nand_info, Block, 0, mtd->oobsize, PageInBlk,6,0, DW_SSRAM_BASE, DW_SSRAM_BASE, global_data_buf, global_data_buf );
	if (ret) {
		printk("dspg_nand_read_oob: read failed = %d\n", ret);
		goto out;
	}

out:
	return dw_nand_kernel_errs(ret);
}

static int dw_nand_wait(struct mtd_info *mtd, struct nand_chip *chip)
{
	return 0;
}

int dw_nand_init(struct mtd_info *mtd);

int dw_nand_scan(struct mtd_info *mtd, int maxchips)
{
	struct nand_chip *nand = mtd->priv;
	int  err;

	dw_nand_init(mtd);

	err = nand_scan_ident(mtd, 1);

	if(err)
		return err;

	/* we can only calculate the ECC per page, not per sub-page */
	nand->options |= NAND_NO_SUBPAGE_WRITE;

	/* ReadId -> set ChipType as Index in V_NFlsParms[] */
	switch (mtd->oobsize) {
	case 16:
		nand->ecc.layout = &dw_nand_oob_16;
		break;
	case 32:
		nand->ecc.layout = &dw_nand_oob_32;
		break;
	case 64:
		nand->ecc.layout = &dw_nand_oob_64;
		break;
	default:
		printk(KERN_WARNING "No oob scheme defined for oobsize %d\n",
	               mtd->oobsize);
		BUG();
	}

	nand->ecc.size = 512;
	nand->ecc.bytes = 16;
	nand->ecc.read_oob = dw_nand_read_oob;
	nand->ecc.write_oob = dw_nand_write_oob;
	nand->ecc.read_page = dw_nand_read_page_hwecc;
	nand->ecc.write_page = dw_nand_write_page_hwecc;
	err = nand_scan_tail(mtd);

	return err;
}

#ifdef __LINUX_KERNEL__
/*
 * Conversion functions
 */
static struct dw_nand_info *to_nand_info(struct platform_device *pdev)
{
	return platform_get_drvdata(pdev);
}

static struct platform_device *to_nand_plat(struct platform_device *pdev)
{
	return pdev->dev.platform_data;
}

/* PM Support */
#ifdef CONFIG_PM

static int dw_nand_suspend(struct platform_device *dev, pm_message_t pm)
{
	struct dw_nand_info *info = platform_get_drvdata(dev);

	return 0;
}

static int dw_nand_resume(struct platform_device *dev)
{
	struct dw_nand_info *info = platform_get_drvdata(dev);

	return 0;
}

#else
#define dw_nand_suspend NULL
#define dw_nand_resume NULL
#endif

static int __devexit dw_nand_remove(struct platform_device *pdev)
{
	struct dw_nand_info *info = to_nand_info(pdev);
	struct mtd_info *mtd = NULL;

	platform_set_drvdata(pdev, NULL);

	/* first thing we need to do is release all our mtds
	 * and their partitions, then go through freeing the
	 * resources used
	 */
	mtd = &info->mtd;
	if (mtd)
		nand_release(mtd);

	/* free the common resources */
	kfree(info);

	return 0;
}

static int dw_nand_probe(struct platform_device *pdev)
{
	struct platform_device *plat = to_nand_plat(pdev);
	struct dw_nand_info *info = NULL;
	struct mtd_info *mtd = NULL;
	struct nand_chip *nand = NULL;
	struct resource *res;
	struct mtd_partition* dw_partition_info;
	int ret = 0;
	int err;

	internal_ram = dw74fb_mpmc_init(&pdev->dev);
	global_data_buf = (unsigned char*) internal_ram;
	if (!plat) {
		dev_err(&pdev->dev, "no platform specific information\n");
		return -EINVAL;
	}

	/* Allocate memory for MTD device structure and private data */
	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info) {
		printk (KERN_ERR "Unable to allocate DW NAND MTD device structure.");
		err = -ENOMEM;
		goto out;
	}

	platform_set_drvdata(pdev, info);

	info->platform = plat;

	res = pdev->resource;

	if (!request_mem_region(res->start, res->end - res->start,
                       "dw_nand_regs")) {
		dev_err(&pdev->dev, "request_mem_region failed\n");
		return -EBUSY;
	}

	info->nand = ioremap_nocache(res->start, res->end - res->start);

	if (!info->nand) {
		dev_err(&pdev->dev, "unable to map registers\n");
		ret = -ENOMEM;
		goto out;
	}

	res++;

	if (!request_mem_region(res->start, res->end - res->start,
                       "dw_nand_regs")) {
		dev_err(&pdev->dev, "request_mem_region failed\n");
		return -EBUSY;
	}

	info->dma_regs.dma_base_reg = ioremap_nocache(res->start, res->end - res->start);

	if (!info->dma_regs.dma_base_reg) {
		dev_err(&pdev->dev, "unable to map registers\n");
		ret = -ENOMEM;
		goto out;
	}

	res++;

	if (!request_mem_region(res->start, res->end - res->start,
                       "dw_nand_regs")) {
		dev_err(&pdev->dev, "request_mem_region failed\n");
		return -EBUSY;
	}

	info->dma_regs.fifo_base_reg = ioremap_nocache(res->start, res->end - res->start);

	if (!info->dma_regs.fifo_base_reg) {
		dev_err(&pdev->dev, "unable to map registers\n");
		ret = -ENOMEM;
		goto out;
	}




	/* initialise chip data struct */
	nand = &info->chip;

	nand->options = 0;
	nand->cmdfunc = dw_nand_cmdfunc;
	nand->read_byte = dw_nand_read_byte;
	nand->read_buf = dw_nand_read_buf;
	nand->write_buf = dw_nand_write_buf;
	nand->select_chip = dw_nand_select_chip;
	nand->waitfunc = dw_nand_wait;
	nand->ecc.mode = NAND_ECC_HW;

	/* initialise mtd info data struct */
	mtd = &info->mtd;
	mtd->priv = nand;
	mtd->owner = THIS_MODULE;

	/* initialize the hardware */
	info->FlsDev = NFC_K_FlsDev0;
	nand->priv = info; 
	F_Init1(info);

	if (dw_nand_scan(mtd, 1)) {
		err = -ENXIO;
		goto outscan;
	}

	//Register the partitions
	mtd->name = "dwflash";
	nr_partitions = parse_mtd_partitions(mtd, part_probes,
						&dw_partition_info, 0);

	if (nr_partitions <= 0) {
		nr_partitions = DEFAULT_NUM_PARTITIONS;
		dw_partition_info = dw_nand_default_partition_info;
	}

	add_mtd_partitions(mtd, dw_partition_info, nr_partitions);

	return 0;

outscan:
	platform_set_drvdata(pdev, NULL);
	kfree(mtd);
out:
	return ret;
}

/* driver device registration */
static struct platform_driver dw_nand_driver = {
	.probe		= dw_nand_probe,
	.remove		= __devexit_p(dw_nand_remove),
	.suspend	= dw_nand_suspend,
	.resume		= dw_nand_resume,
	.driver		= {
		.name	= DRV_NAME,
		.owner	= THIS_MODULE,
	},
};

static int __init init_dw_nand(void)
{
	return platform_driver_register(&dw_nand_driver);
}

static void __exit cleanup_dw_nand(void)
{
	platform_driver_unregister(&dw_nand_driver);
}

module_init(init_dw_nand);
module_exit(cleanup_dw_nand);

MODULE_AUTHOR("DSP Group Ltd");
MODULE_DESCRIPTION("DW Nand Flash driver");
MODULE_LICENSE("GPL");
#else
int dw_nand_init(struct mtd_info *mtd)
{
	struct dw_nand_info *info = NULL;
	struct nand_chip *nand = NULL;
	int ret = 0;
	int err;

#ifdef MACH_VERSATILE_BROADTILE
	internal_ram = dw74fb_mpmc_init(NULL); // &pdev->dev); TODO
	global_data_buf = (unsigned char*) internal_ram;
#else
	internal_ram = (void *)DW_SSRAM_BASE;
	global_data_buf = (unsigned char *)internal_ram;
#endif

	/* Allocate memory for MTD device structure and private data */
	info = (struct dw_nand_info *) kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info) {
		printk (KERN_ERR "Unable to allocate DW NAND MTD device structure.");
		err = -ENOMEM;
		goto out;
	}

	info->nand = (void __iomem *)DW_NAND_LCD_BASE;
	info->dma_regs.dma_base_reg = (void __iomem *)DW_DMA_BASE;
	info->dma_regs.fifo_base_reg = (void __iomem *)DW_FIFO_BASE;

	/* initialise chip data struct */
	//nand = &info->chip;
	nand = (struct nand_chip *)mtd->priv; //&info->chip;

	nand->options = 0;
	nand->cmdfunc = dw_nand_cmdfunc;
	nand->read_byte = dw_nand_read_byte;
	nand->read_buf = dw_nand_read_buf;
	nand->write_buf = dw_nand_write_buf;
	nand->select_chip = dw_nand_select_chip;
	nand->waitfunc = dw_nand_wait;
	nand->ecc.mode = NAND_ECC_HW;

	/* initialise mtd info data struct */
	//mtd->priv = nand;
	info->mtd = mtd;
	info->chip = nand;

	/* initialize the hardware */
	info->FlsDev = NFC_K_FlsDev0;
	nand->priv = info; 
	F_Init1(info);

	/*if (dw_nand_scan(mtd, 1)) {
		err = -ENXIO;
		goto outscan;
	}*/

	return 0;

out:
	return ret;
}
#endif
