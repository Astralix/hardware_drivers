#include <common.h>
#include <command.h>
#include <nand.h>
#include <asm/io.h>
#include <malloc.h>
#include <div64.h>
#include <linux/err.h>

#include "../../../drivers/mtd/nand/dmw96_nfc.h"

extern int nand_curr_device;
extern nand_info_t nand_info[CONFIG_SYS_MAX_NAND_DEVICE];

/*
 * helpers
 */
static uint16_t
crc16(void *buf, unsigned long len)
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

struct rnd_state {
	u32 s1, s2, s3;
	u32 initialized;
};

static struct rnd_state rnd_state;

static u32 random32(void)
{
#define TAUSWORTHE(s,a,b,c,d) ((s&c)<<d) ^ (((s <<a) ^ s)>>b)

	rnd_state.s1 = TAUSWORTHE(rnd_state.s1, 13, 19, 4294967294UL, 12);
	rnd_state.s2 = TAUSWORTHE(rnd_state.s2, 2, 25, 4294967288UL, 4);
	rnd_state.s3 = TAUSWORTHE(rnd_state.s3, 3, 11, 4294967280UL, 17);

	return (rnd_state.s1 ^ rnd_state.s2 ^ rnd_state.s3);
}

static inline u32 __seed(u32 x, u32 m)
{
	return (x < m) ? x + m : x;
}

static void random32_init(u32 seed)
{
#define LCG(x) ((x) * 69069)
	rnd_state.s1 = __seed(LCG(seed), 1);
	rnd_state.s2 = __seed(LCG(rnd_state.s1), 7);
	rnd_state.s3 = __seed(LCG(rnd_state.s2), 15);
#undef LCG

	/* "warm it up" */
	random32();
	random32();
	random32();
	random32();
	random32();
	random32();
}

static void fillbuf_rand(void *buf, unsigned int size)
{
	u32 *p = buf;
	int i;

#if 0
	if (size & 3)
		printf("%s(): WARN, size is not multiple of 4\n", __func__);
#endif

	for (i = 0; i < size; i += 4)
		*p++ = random32();
}

int count_errors(void *buf1, void *buf2, int len)
{
	char *b1 = (char *)buf1;
	char *b2 = (char *)buf2;
	int errors = 0;
	int i;

	for (i = 0; i < len; i++) {
		char x = *b1++ ^ *b2++;

		while (x) {
			errors++;
			x &= (x - 1);
		}
	}

	return errors;
}

/*
 * command implementations
 */
static int tnand_id(int argc, char *const argv[])
{
	struct mtd_info *mtd = &nand_info[nand_curr_device];
	struct nand_chip *chip = mtd->priv;
	int maf_id, dev_id, ext1, ext2;
	int extid, writesize, oobsize, erasesize, bits;
	unsigned int oldopts;

	oldopts = chip->options;
	chip->options &= ~NAND_BUSWIDTH_16;

	chip->select_chip(mtd, 0);
	chip->cmdfunc(mtd, NAND_CMD_RESET, -1, -1);
	chip->cmdfunc(mtd, NAND_CMD_READID, 0x00, -1);
	maf_id = chip->read_byte(mtd);
	dev_id = chip->read_byte(mtd);
	ext1 = chip->read_byte(mtd);
	ext2 = chip->read_byte(mtd);

	chip->options = oldopts;

	extid = ext2;
	writesize = 1024 << (extid & 0x3);
	extid >>= 2;
	oobsize = (8 << (extid & 0x01)) * (writesize >> 9);
	extid >>= 2;
	erasesize = (64 * 1024) << (extid & 0x03);
	extid >>= 2;
	bits = (extid & 0x01) ? 16 : 8;

	printf("dev id: 0x%02X\n", dev_id);
	printf("maf id: 0x%02X\n", maf_id);
	printf("ext1:   0x%02X, %s\n", ext1, ext1 ? "mlc" : "slc");
	printf("ext2:   0x%02X, page = %d, oob = %d, erase = %d, %dbit\n",
	       ext2, writesize, oobsize, erasesize, bits);
	return 0;
}

static uint8_t tnand_onfi_buf[3][256];

static int tnand_onfi(int argc, char *const argv[])
{
	struct mtd_info *mtd = &nand_info[nand_curr_device];
	struct nand_chip *chip = mtd->priv;
	size_t page_size, oob_size, erase_size;
	uint64_t chip_size;
	uint8_t *buf;
	int i;
	int ret = 0;
	unsigned int oldopts;

	oldopts = chip->options;
	chip->options &= ~NAND_BUSWIDTH_16;

	chip->select_chip(mtd, 0);
	chip->cmdfunc(mtd, NAND_CMD_RESET, -1, -1);
	chip->cmdfunc(mtd, NAND_CMD_READID, 0x20, -1);
	if (chip->read_byte(mtd) != 'O' ||
	    chip->read_byte(mtd) != 'N' ||
	    chip->read_byte(mtd) != 'F' ||
	    chip->read_byte(mtd) != 'I') {
		printf("No ONFI Flash detected\n");
		ret = 1;
		goto out;
	}

	//writel(1, 0x101e4400); /* gpio0[0] output */
	//writel(1, 0x101e4004); /* high */

	chip->cmdfunc(mtd, 0xec, 0x00, -1);

	//writel(0, 0x101e4004); /* low */
	udelay(300);
	//writel(1, 0x101e4004); /* high */

	for (i = 0; i < 3; i++) {
		buf = tnand_onfi_buf[i];
		uint16_t crc;

		chip->read_buf(mtd, buf, 256);

		printf("First ONFI bytes @ 0x%p: 0x%02x 0x%02x 0x%02x 0x%02x\n",
		       buf, buf[0], buf[1], buf[2], buf[3]);

		crc = buf[255] << 8 | buf[254];
		if (crc16(buf, 254) == crc) {
			i = 0;
			break;
		}
	}

	if (i) {
		printf("ONFI chip detected, but could not read param table\n");
		ret = 1;
		goto out;
	}

	page_size =   buf[80]        | (buf[81] << 8) |
	             (buf[82] << 16) | (buf[83] << 24);
	oob_size =    buf[84]        | (buf[85] << 8);
	erase_size =  buf[92]        | (buf[93] << 8) |
	             (buf[94] << 16) | (buf[95] << 24);
	erase_size *= page_size;
	chip_size =   buf[96]        | (buf[97] << 8) |
	             (buf[98] << 16) | (buf[99] << 24);
	chip_size *= buf[100] * erase_size;

	printf("ONFI Data:\n\n"
	       " Bus width:  %s\n"
	       " Page size:  %d\n"
	       " OOB size:   %d\n"
	       " Erase size: %d\n"
	       " Chip size:  %llu\n"
	       " ECC bits:   %d\n",
	       buf[6] & 1 ? "16bit" : "8bit",
	       page_size,
	       oob_size,
	       erase_size,
	       chip_size,
	       buf[112]);

	printf("\nMTD Data:\n\n"
	       " Bus width:  %s\n"
	       " Page size:  %d\n"
	       " OOB size:   %d\n"
	       " Erase size: %d\n"
	       " Chip size:  %llu\n",
	       oldopts & NAND_BUSWIDTH_16 ? "16bit" : "8bit",
	       mtd->writesize,
	       mtd->oobsize,
	       mtd->erasesize,
	       chip->chipsize);

out:
	chip->options = oldopts;
	return ret;
}

static int tnand_biterr(int argc, char *const argv[])
{
	struct mtd_info *mtd = &nand_info[nand_curr_device];
	loff_t off;
	size_t size = mtd->writesize;
	int byte, bit;
	int old;
	u8 *buf;
	int ret = 0;

	if (argc != 3)
		return -1;

	buf = malloc(mtd->writesize);
	if (!buf) {
		printf("%s(): need more malloc space\n", __func__);
		return 1;
	}

	off = simple_strtoul(argv[2], NULL, 0);
	byte = off & (mtd->writesize - 1);
	off &= ~(mtd->writesize - 1);

	old = dmw96_nfc_use_ecc(mtd, 0);
	if (nand_read_skip_bad(mtd, off, &size, buf)) {
		printf("Read failed\n");
		ret = 1;
		goto out;
	}

	if (!buf[byte]) {
		printf("Byte is zero, no bits can be flipped\n");
		ret = 1;
		goto out;
	}

	//printf("byte = 0x%x\n", buf[byte]);

	/* find first bit set and flip it */
	bit = ffs(buf[byte]);
	buf[byte] ^= 1 << (bit - 1);

	//printf("byte = 0x%x\n", buf[byte]);

	if (nand_write_skip_bad(mtd, off, &size, buf, 0)) {
		printf("Write failed\n");
		ret = 1;
		goto out;
	}

out:
	dmw96_nfc_use_ecc(mtd, old);
	free(buf);
	return ret;
}

static int tnand_ecc(int argc, char *const argv[])
{
	struct mtd_info *mtd = &nand_info[nand_curr_device];
	int use_ecc;

	if (argc != 3)
		return -1;

	use_ecc = strcmp(argv[2], "off");
	dmw96_nfc_use_ecc(mtd, use_ecc);

	return 0;
}

static int tnand_ecc_mode(int argc, char *const argv[])
{
	struct mtd_info *mtd = &nand_info[nand_curr_device];
	int ecc_mode;

	if (argc > 3)
		return -1;

	if (argc == 2) {
		ecc_mode = dmw96_nfc_ecc_mode(mtd, 0);
		dmw96_nfc_ecc_mode(mtd, ecc_mode);
		printf("ecc_mode: %d\n", ecc_mode);
		return 0;
	}

	ecc_mode = simple_strtoul(argv[2], NULL, 0);
	if (dmw96_nfc_ecc_mode(mtd, ecc_mode) < 0)
		return 1;

	return 0;
}

static int tnand_get_oob_config(int argc, char *const argv[])
{
	struct mtd_info *mtd = &nand_info[nand_curr_device];
	struct nand_chip *chip = mtd->priv;
	struct dmw96_nfc_info *info = chip->priv;

	int ecc_bytes = chip->ecc.bytes;
	ecc_bytes /= mtd->writesize >> VIRTUAL_PAGESIZE_SHIFT;

	printf("OOB configuration:\n\n"
	       " OOB size: %d\n"
	       " OOB noecc size: %d\n\n",
	       mtd->oobsize, info->n2len - ecc_bytes);

	return 0;
}

struct nand_page {
	unsigned int pageno;
	unsigned int seed;
	unsigned char errors[16];
};

static int page_picked(struct nand_page *pages, unsigned int count,
                       unsigned int pageno)
{
	unsigned int i;

	for (i = 0; i < count; i++)
		if (pages[i].pageno == pageno)
			return 1;

	return 0;
}

#define TNAND_ECC_TEST_NUMPAGES 20000
struct nand_page pages[TNAND_ECC_TEST_NUMPAGES];
struct nand_page *last_page;
int last_pagenum;
uint8_t *pagebuf, *pagebuf2, *pagebuf3;
unsigned long seed;

static int tnand_ecc_test(int argc, char *const argv[])
{
	struct mtd_info *mtd = &nand_info[nand_curr_device];
	struct nand_chip *chip = mtd->priv;
	struct dmw96_nfc_info *info = chip->priv;
	nand_erase_options_t e_opts;
	loff_t ofs;
	int i, j;
	size_t retlen;
	int num_pages = lldiv(mtd->size, mtd->writesize);
	int num_vps = mtd->writesize / 512;
	int ecc_bits = info->ecc_mode ? info->ecc_mode << 2 : 1;
	int ret = 0;

	if (argc == 2)
		seed = get_timer_masked();
	else
		seed = simple_strtoul(argv[2], NULL, 0);

	pagebuf = malloc(mtd->writesize);
	if (!pagebuf) {
		printf("pagebuf: No memory, increase heap\n");
		return 1;
	}

	pagebuf2 = malloc(mtd->writesize);
	if (!pagebuf2) {
		free(pagebuf);
		printf("pagebuf2: No memory, increase heap\n");
		return 1;
	}

	pagebuf3 = malloc(mtd->writesize);
	if (!pagebuf3) {
		free(pagebuf);
		free(pagebuf2);
		printf("pagebuf3: No memory, increase heap\n");
		return 1;
	}

	memset(pages, 0, sizeof(pages));

	random32_init(seed);

	/* erase flash */
	printf("ecc_test: erasing flash\n");
	memset(&e_opts, 0, sizeof(e_opts));
	e_opts.length = mtd->size;
	e_opts.quiet = 1;
	nand_erase_opts(mtd, &e_opts);

	/* randomly pick TNAND_ECC_TEST_NUMPAGES pages */
	for (i = 0; i < TNAND_ECC_TEST_NUMPAGES; i++) {
		struct nand_page *page = &pages[i];

		do {
			page->pageno = random32() % num_pages;
		} while (mtd->block_isbad(mtd, page->pageno * mtd->writesize) ||
		         page_picked(pages, i, page->pageno));

		page->seed = random32();
	}

	num_pages = TNAND_ECC_TEST_NUMPAGES;

	/* write flash */
	printf("ecc_test: writing test pages\n");
	for (i = 0; i < num_pages; i++) {
		ofs = pages[i].pageno * mtd->writesize;

		random32_init(pages[i].seed);
		fillbuf_rand(pagebuf, mtd->writesize);
		mtd->write(mtd, ofs, mtd->writesize, &retlen, pagebuf);
	}

	/* randomly place errors in pages and read them back */
	printf("ecc_test: starting test, ignore dmw96_nfc_* messages\n");
	while (num_pages) {
		struct nand_page *page;
		int subpage;
		char cmd[128];

		/* select page randomly */
		i = random32() % num_pages;
		last_page = page = &pages[i];
		ofs = page->pageno * mtd->writesize;

		/* select subpage randomly */
		subpage = random32() % num_vps;

		/* recalculate original page values into pagebuf3 */
		random32_init(page->seed);
		fillbuf_rand(pagebuf3, mtd->writesize);

		/* read the page and compare to original values*/
		ret = mtd->read(mtd, ofs, mtd->writesize, &retlen, pagebuf);
		if (ret && ret != -EUCLEAN) {
			printf("ecc_test: failed to read page %d\n", page->pageno);
			ret = 1;
			goto out;
		}

		if (memcmp(pagebuf, pagebuf3, mtd->writesize)) {
			printf("ecc_test: page %d doesn't contain original values!\n", page->pageno);
			ret = 1;
			goto out;
		}

		/* create error at a random byte at the page */
		for (j=0; j < page->errors[subpage]; j++)
			random32();

		do {
			sprintf(cmd, "tnand biterr 0x%llx",
			        ofs + (subpage * 512) + (random32() % 512));
		} while (run_command(cmd, 0) < 0);
		page->errors[subpage]++;

		if (!i)
			printf("[%d] %2d %2d %2d %2d %2d %2d %2d %2d\n", subpage,
			       page->errors[0], page->errors[1],
			       page->errors[2], page->errors[3],
			       page->errors[4], page->errors[5],
			       page->errors[6], page->errors[7]);

		/* read the page again and make sure the ECC correction is working */
		ret = mtd->read(mtd, ofs, mtd->writesize, &retlen, pagebuf2);
		if (page->errors[subpage] <= ecc_bits && ret != -EUCLEAN) {
			printf("ecc_test: read failed at offset 0x%08x, -EUCLEAN expected\n",
			       page->pageno * mtd->writesize);
			ret = 1;
			goto out;
		}

		if (memcmp(pagebuf, pagebuf2, mtd->writesize) &&
		    page->errors[subpage] <= ecc_bits) {
			printf("ecc_test: correction failed for page %d\n",
			       page->pageno);
			ret = 1;
			goto out;
		}

		if (page->errors[subpage] > ecc_bits && ret >= 0) {
			printf("ecc_test: read should fail for page %d, but didnt\n",
			       page->pageno);
			ret = 1;
			goto out;
		}

		/*
		 * if we have a page with more than the supported number of
		 * bit errors "remove" this page from the pages array by overwriting
		 * it with the last page and reducing num_pages by one
		 */
		if (page->errors[subpage] > ecc_bits) {
			if (!i)
				printf("[%d] done\n", subpage);
			pages[i] = pages[num_pages - 1];
			num_pages--;
		}
	}

	printf("ecc_test: all passed\n");
	ret = 0;
out:
	last_pagenum = i;
	last_page = &pages[i];

	if (ret)
		while (1);

	free(pagebuf3);
	free(pagebuf2);
	free(pagebuf);
	return ret;
}

int subpage, j;
uint8_t oobbuf[224]  __attribute__((aligned(4)));
uint8_t oobbuf2[224] __attribute__((aligned(4)));
static struct mtd_oob_ops ops;

static int tnand_ecc_test2(int argc, char *const argv[])
{
	struct mtd_info *mtd = &nand_info[nand_curr_device];
	struct nand_chip *chip = mtd->priv;
	struct dmw96_nfc_info *info = chip->priv;
	nand_erase_options_t e_opts;
	loff_t ofs;
	int i;
	//int j;
	size_t retlen;
	int num_pages = lldiv(mtd->size, mtd->writesize);
	int num_vps = mtd->writesize / 512;
	int ecc_bits = info->ecc_mode ? info->ecc_mode << 2 : 1;
	int ret = 0;
	//int subpage;
	char cmd[128];

	if (argc == 2)
		seed = get_timer_masked();
	else
		seed = simple_strtoul(argv[2], NULL, 0);

	memset(pages, 0, sizeof(pages));

	pagebuf = malloc(mtd->writesize);
	if (!pagebuf) {
		printf("pagebuf: No memory, increase heap\n");
		return 1;
	}

	pagebuf2 = malloc(mtd->writesize);
	if (!pagebuf2) {
		free(pagebuf);
		printf("pagebuf2: No memory, increase heap\n");
		return 1;
	}

	pagebuf3 = malloc(mtd->writesize);
	if (!pagebuf3) {
		free(pagebuf);
		free(pagebuf2);
		printf("pagebuf3: No memory, increase heap\n");
		return 1;
	}

	memset(&ops, 0, sizeof(ops));
	ops.mode = MTD_OOB_PLACE;
	ops.len = mtd->writesize;
	ops.ooblen = mtd->oobsize;

	random32_init(seed);

	/* erase flash */
	printf("ecc_test2: erasing flash\n");
	memset(&e_opts, 0, sizeof(e_opts));
	e_opts.length = mtd->size;
	e_opts.quiet = 1;
	nand_erase_opts(mtd, &e_opts);

	for (i = 0; i < num_pages; i++) {
		struct nand_page *page = &pages[0];

		ofs = i * mtd->writesize;
		if (mtd->block_isbad(mtd, ofs))
			continue;

		memset(page, 0, sizeof(*page));
		page->pageno = i;

		/* write page */
		fillbuf_rand(pagebuf, mtd->writesize);
		mtd->write(mtd, ofs, mtd->writesize, &retlen, pagebuf);

		/* read the page and compare to original values*/
		ops.datbuf = pagebuf2;
		ops.oobbuf = oobbuf;

		dmw96_nfc_use_ecc(mtd, 0);
		ret = mtd->read_oob(mtd, ofs, &ops);
		dmw96_nfc_use_ecc(mtd, 1);

		if (ret && ret != -EUCLEAN) {
			printf("ecc_test2: failed to read page %d\n", page->pageno);
			ret = 1;
			goto out;
		}

		if (memcmp(pagebuf, pagebuf2, mtd->writesize)) {
			printf("ecc_test2: page at 0x%08x already is faulty!\n", page->pageno * mtd->writesize);
			ret = 1;
			goto out;
		}

		/* create errors until full */
		do {
			subpage = random32() % num_vps;

			/* read the page and compare to original values*/
			ret = mtd->read(mtd, ofs, mtd->writesize, &retlen, pagebuf2);
			if (ret && ret != -EUCLEAN) {
				printf("ecc_test2: failed to read page %d\n", page->pageno);
				ret = 1;
				goto out;
			}

			if (memcmp(pagebuf, pagebuf2, mtd->writesize)) {
				printf("ecc_test2: page %d doesn't contain original values!\n", page->pageno);
				ret = 1;
				goto out;
			}

			/* create error at a random byte at the page */
			do {
				j = random32() % 512;
				sprintf(cmd, "tnand biterr 0x%llx",
				        ofs + (subpage * 512) + j);
			} while (run_command(cmd, 0) < 0);
			page->errors[subpage]++;

#if 0
			printf("[%5d] [%d:%3d] %2d %2d %2d %2d %2d %2d %2d %2d\n", i, subpage, j,
			       page->errors[0], page->errors[1],
			       page->errors[2], page->errors[3],
			       page->errors[4], page->errors[5],
			       page->errors[6], page->errors[7]);
#endif

			/* read the page again and make sure the ECC correction is working */
			ops.datbuf = pagebuf3;
			ops.oobbuf = oobbuf2;
			ret = mtd->read_oob(mtd, ofs, &ops);

			if (page->errors[subpage] <= ecc_bits && ret != -EUCLEAN) {
				printf("ecc_test2: read failed at offset 0x%08llx, -EUCLEAN expected\n",
				       ofs);
				ret = 1;
				goto out;
			}

			if (memcmp(pagebuf, pagebuf3, mtd->writesize) &&
			    page->errors[subpage] <= ecc_bits) {
				printf("ecc_test2: correction failed at offset 0x%08llx\n", ofs);
				ret = 1;
				goto out;
			}

			if (memcmp(oobbuf, oobbuf2, mtd->oobsize)) {
				printf("ecc_test2: oob data has changed at offset 0x%08llx\n", ofs);
				ret = 1;
				goto out;
			}

			if (page->errors[subpage] > ecc_bits && ret >= 0) {
				printf("ecc_test2: read should fail at offset 0x%08llx, but didnt\n", ofs);
				ret = 1;
				goto out;
			}
		} while (page->errors[subpage] <= ecc_bits);

		//printf("[%5d] done\n", i);
	}

	printf("ecc_test2: all passed\n");
	ret = 0;
out:
	last_pagenum = i;
	last_page = &pages[i];

	if (ret)
		while (1);

	free(pagebuf3);
	free(pagebuf2);
	free(pagebuf);
	return ret;
}

static int tnand_write_read_test(int argc, char *const argv[])
{
	struct mtd_info *mtd = &nand_info[nand_curr_device];
	nand_erase_options_t e_opts;
	int ret = 0;
	loff_t ofs;
	size_t retlen;
	mtd_oob_ops_t oob_ops;

	if (argc == 2) {
		seed = get_timer_masked();
		printf("write_read: seed: 0x%08lx\n", seed);
	} else {
		seed = simple_strtoul(argv[2], NULL, 0);
	}

	random32_init(seed);

	pagebuf = malloc(mtd->writesize);
	if (!pagebuf) {
		printf("pagebuf: No memory, increase heap\n");
		return 1;
	}

	pagebuf2 = malloc(mtd->writesize);
	if (!pagebuf2) {
		printf("pagebuf2: No memory, increase heap\n");
		free(pagebuf);
		return 1;
	}

	memset(&oob_ops, 0, sizeof(oob_ops));
	oob_ops.datbuf = pagebuf;
	oob_ops.len = mtd->writesize;
	oob_ops.oobbuf = oobbuf;
	oob_ops.ooblen = mtd->oobavail;
	oob_ops.mode = MTD_OOB_AUTO;

	/* erase flash */
	printf("write_read: erasing flash\n");
	memset(&e_opts, 0, sizeof(e_opts));
	e_opts.length = mtd->size;
	e_opts.quiet = 1;
	nand_erase_opts(mtd, &e_opts);

#define TESTSIZE (mtd->size)
//#define TESTSIZE (5 << 20)
	/* write the whole flash (including oob) */
	for (ofs = 0; ofs < TESTSIZE; ofs += mtd->writesize) {
		static int mb_old = -1;
		int mb = (ofs + mtd->writesize) >> 20;

		if (mb != mb_old) {
			mb_old = mb;
			printf("\rwrite_read: writing test data: %4dmb / %4dmb",
			       mb, (int)(mtd->size >> 20));
		}

		if (mtd->block_isbad(mtd, ofs))
			continue;

		/* write data */
		fillbuf_rand(pagebuf, mtd->writesize);
		fillbuf_rand(oobbuf, mtd->oobavail);
		ret = mtd->write_oob(mtd, ofs, &oob_ops);
		if (ret) {
			printf("\nwrite_read: write failed at offset 0x%llx\n", ofs);
			ret = 1;
			goto out;
		}
	}

	printf("\n");

	/* re-init the pseudo random generator */
	random32_init(seed);

	oob_ops.datbuf = NULL;
	oob_ops.len = 0;
	oob_ops.oobbuf = pagebuf;

	/* read the whole flash */
	for (ofs = 0; ofs < TESTSIZE; ofs += mtd->writesize) {
		static int mb_old = -1;
		int mb = (ofs + mtd->writesize) >> 20;

		if (mb != mb_old) {
			mb_old = mb;
			printf("\rwrite_read: reading test data: %4dmb / %4dmb",
			       mb, (int)(mtd->size >> 20));
		}

		if (mtd->block_isbad(mtd, ofs))
			continue;

		/* read and compare data */
		fillbuf_rand(pagebuf, mtd->writesize);
		ret = mtd->read(mtd, ofs, mtd->writesize, &retlen, pagebuf2);

		if (ret == -EUCLEAN) {
			printf("\nwrite_read: fixed error @ 0x%llx\n", ofs);
		} else if (ret) {
			printf("\nwrite_read: read failed at offset 0x%llx\n", ofs);
			ret = 1;
			goto out;
		}

		if (memcmp(pagebuf, pagebuf2, mtd->writesize)) {
			printf("\nwrite_read: compare failed at offset 0x%llx\n", ofs);
			ret = 1;
			goto out;
		}

		/* read and compare oob */
		fillbuf_rand(pagebuf2, mtd->oobavail);
		ret = mtd->read_oob(mtd, ofs, &oob_ops);
		if (ret) {
			printf("\nwrite_read: read OOB failed at offset 0x%llx\n", ofs);
			ret = 1;
			goto out;
		}

		ret = count_errors(pagebuf, pagebuf2, mtd->oobavail);

		/* we tolerate up to four errors in ecc unprotected oob area */
		if (ret > 4) {
			printf("\nwrite_read: compare failed at OOB of offset 0x%llx\n", ofs);
			ret = 1;
			goto out;
		}
	}
#undef TESTSIZE

	printf("\n");

	printf("write_read: test passed\n");
	ret = 0;

out:
	if (ret) {
		printf("write_read: test failed\n");
		while (1);
	}

	free(pagebuf2);
	free(pagebuf);
	return ret;
}

static int tnand_check_erased(int argc, char *const argv[])
{
	struct mtd_info *mtd = &nand_info[nand_curr_device];
	uint8_t *pagebuf;
	uint8_t *pagebuf2;
	int ret = 0;
	loff_t start_ofs;
	loff_t ofs;
	loff_t end_ofs;
	mtd_oob_ops_t oob_ops;

	if (argc > 3)
		return -1;

	pagebuf = malloc(mtd->writesize + mtd->oobsize);
	if (!pagebuf) {
		printf("pagebuf: No memory, increase heap\n");
		return 1;
	}

	pagebuf2 = malloc(mtd->writesize + mtd->oobsize);
	if (!pagebuf) {
		printf("pagebuf2: No memory, increase heap\n");
		free(pagebuf);
		return 1;
	}

	memset(pagebuf2, 0xff, mtd->writesize + mtd->oobsize);

	memset(&oob_ops, 0, sizeof(oob_ops));
	oob_ops.datbuf = pagebuf;
	oob_ops.oobbuf = pagebuf + mtd->writesize; /* must exist, but oob data will be appended to oob_ops.datbuf */
	oob_ops.len = mtd->writesize;
	oob_ops.ooblen = mtd->oobsize;
	oob_ops.mode = MTD_OOB_RAW;

	if (argc == 3) {
		start_ofs = simple_strtoul(argv[2], NULL, 0);
		start_ofs &= ~(mtd->writesize - 1);
		end_ofs = start_ofs + mtd->writesize;
	}
	else {
		start_ofs = 0;
		end_ofs = mtd->size;
	}

	/* read the flash */
	for (ofs = start_ofs; ofs < end_ofs; ofs += mtd->writesize) {
		static int percent_old = -1;
		int percent = (int)lldiv((ofs - start_ofs + mtd->writesize) * 100, end_ofs - start_ofs);

		if (percent != percent_old) {
			percent_old = percent;
			printf("\rcheck_erased: reading: %3d%%", percent);
		}

		if (mtd->block_isbad(mtd, ofs))
			continue;

		/* read the data */
		ret = mtd->read_oob(mtd, ofs, &oob_ops);

		if (ret) {
			printf("\ncheck_erased: read failed at offset 0x%llx\n", ofs);
			ret = 1;
			break;
		}

		/* compare to empty data */
		if (memcmp(pagebuf, pagebuf2, mtd->writesize + mtd->oobsize)) {
			printf("\ncheck_erased: offset 0x%llx is not erased!\n", ofs);
			ret = 1;
			break;
		}
	}

	printf("\n");

	free(pagebuf2);
	free(pagebuf);
	return ret;
}

static int tnand_isbad(int argc, char *const argv[])
{
	struct mtd_info *mtd = &nand_info[nand_curr_device];
	unsigned long addr;

	if (argc != 3)
		return -1;

	addr = simple_strtoul(argv[2], NULL, 0);
	printf("0x%08lx is %s\n", addr, mtd->block_isbad(mtd, addr) ? "bad" : "good");

	return 0;
}

int forever_loops;

static int tnand_forever(void)
{
	forever_loops = 0;

	clear_ctrlc();

	while (1) {
		tnand_write_read_test(2, NULL);
		forever_loops++;
		printf("forever: finished loop %d\n", forever_loops);

		if (ctrlc()) {
			printf("Interrupted, exiting...\n");
			return 1;
		}
	}

	return 0;
}

int do_tnand(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int res = 1;

	if (argc < 2)
		goto usage;

	if (strcmp("id", argv[1]) == 0)
		res = tnand_id(argc, argv);
	else if (strcmp("onfi", argv[1]) == 0)
		res = tnand_onfi(argc, argv);
	else if (strcmp("biterr", argv[1]) == 0)
		res = tnand_biterr(argc, argv);
	else if (strcmp("ecc", argv[1]) == 0)
		res = tnand_ecc(argc, argv);
	else if (strcmp("ecc_mode", argv[1]) == 0)
		res = tnand_ecc_mode(argc, argv);
	else if (strcmp("get_oob_config", argv[1]) == 0)
		res = tnand_get_oob_config(argc, argv);
	else if (strcmp("ecc_test", argv[1]) == 0)
		res = tnand_ecc_test(argc, argv);
	else if (strcmp("ecc_test2", argv[1]) == 0)
		res = tnand_ecc_test2(argc, argv);
	else if (strcmp("write_read_test", argv[1]) == 0)
		res = tnand_write_read_test(argc, argv);
	else if (strcmp("check_erased", argv[1]) == 0)
		res = tnand_check_erased(argc, argv);
	else if (strcmp("isbad", argv[1]) == 0)
		res = tnand_isbad(argc, argv);
	else if (strcmp("forever", argv[1]) == 0)
		tnand_forever();

	if (res >= 0)
		return res;

usage:
	cmd_usage(cmdtp);
	return 1;
}

U_BOOT_CMD(tnand, 3, 0, do_tnand,
	"nand testing utilities",
	      "id                     - detect nand and print nand id\n"
	"tnand onfi                   - dump onfi table & detected mtd info\n"
	"tnand biterr <ofs>           - make bit error at offset <ofs>\n"
	"tnand ecc <on/off>           - turn ecc on or off\n"
	"tnand ecc_mode [mode]        - select the ecc mode to use\n"
	"tnand get_oob_config         - display oob configuration\n"
	"tnand ecc_test [seed]        - automated random ecc test\n"
	"tnand ecc_test2 [seed]       - automated random ecc test\n"
	"tnand write_read_test [seed] - automated random write/read test\n"
	"tnand check_erased [offset]  - test whether the flash is erased\n"
	"tnand isbad <ofs>            - check if block is bad at offset <ofs>\n"
);
