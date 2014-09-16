#include "common.h"
#include "bootrom.h"
#include "crc32.h"
#include "serial.h"
#include "load_nand.h"
#include "hardware.h"

#define NAND_MAX_PAGESIZE           8192
#define VIRTUAL_PAGESIZE_SHIFT         9
#define DMW96_NFC_MAX_ERRORS_PER_VP   12
#define DMW96_NFC_MAX_ERRORS_PER_PAGE \
        ((NAND_MAX_PAGESIZE >> VIRTUAL_PAGESIZE_SHIFT) * \
         DMW96_NFC_MAX_ERRORS_PER_VP)

#define __iomem
typedef uint32_t dma_addr_t;
typedef uint32_t phys_addr_t;

struct clk;
struct nand_chip;
struct mtd_erase_region_info;
struct erase_info;
struct mtd_oob_ops;
struct otp_info;

struct mtd_ecc_stats {
	uint32_t corrected;
	uint32_t failed;
	uint32_t badblocks;
	uint32_t bbtblocks;
};

struct mtd_info {
	u_char type;
	u_int32_t flags;
	uint64_t size;	 /* Total size of the MTD */

	/* "Major" erase size for the device. Naïve users may take this
	 * to be the only erase size available, or may use the more detailed
	 * information below if they desire
	 */
	u_int32_t erasesize;
	/* Minimal writable flash unit size. In case of NOR flash it is 1 (even
	 * though individual bits can be cleared), in case of NAND flash it is
	 * one NAND page (or half, or one-fourths of it), in case of ECC-ed NOR
	 * it is of ECC block size, etc. It is illegal to have writesize = 0.
	 * Any driver registering a struct mtd_info must ensure a writesize of
	 * 1 or larger.
	 */
	u_int32_t writesize;

	u_int32_t oobsize;   /* Amount of OOB data per block (e.g. 16) */
	u_int32_t oobavail;  /* Available OOB bytes per block */

	/* Kernel-only stuff starts here. */
	const char *name;
	int index;

	/* ecc layout structure pointer - read only ! */
	struct nand_ecclayout *ecclayout;

	/* Data for variable erase regions. If numeraseregions is zero,
	 * it means that the whole device has erasesize as given above.
	 */
	int numeraseregions;
	struct mtd_erase_region_info *eraseregions;

	/*
	 * Erase is an asynchronous operation.  Device drivers are supposed
	 * to call instr->callback() whenever the operation completes, even
	 * if it completes with a failure.
	 * Callers are supposed to pass a callback function and wait for it
	 * to be called before writing to the block.
	 */
	int (*erase) (struct mtd_info *mtd, struct erase_info *instr);

	/* This stuff for eXecute-In-Place */
	/* phys is optional and may be set to NULL */
	int (*point) (struct mtd_info *mtd, loff_t from, size_t len,
			size_t *retlen, void **virt, phys_addr_t *phys);

	/* We probably shouldn't allow XIP if the unpoint isn't a NULL */
	void (*unpoint) (struct mtd_info *mtd, loff_t from, size_t len);


	int (*read) (struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char *buf);
	int (*write) (struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen, const u_char *buf);

	/* In blackbox flight recorder like scenarios we want to make successful
	   writes in interrupt context. panic_write() is only intended to be
	   called when its known the kernel is about to panic and we need the
	   write to succeed. Since the kernel is not going to be running for much
	   longer, this function can break locks and delay to ensure the write
	   succeeds (but not sleep). */

	int (*panic_write) (struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen, const u_char *buf);

	int (*read_oob) (struct mtd_info *mtd, loff_t from,
			 struct mtd_oob_ops *ops);
	int (*write_oob) (struct mtd_info *mtd, loff_t to,
			 struct mtd_oob_ops *ops);

	/*
	 * Methods to access the protection register area, present in some
	 * flash devices. The user data is one time programmable but the
	 * factory data is read only.
	 */
	int (*get_fact_prot_info) (struct mtd_info *mtd, struct otp_info *buf, size_t len);
	int (*read_fact_prot_reg) (struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char *buf);
	int (*get_user_prot_info) (struct mtd_info *mtd, struct otp_info *buf, size_t len);
	int (*read_user_prot_reg) (struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char *buf);
	int (*write_user_prot_reg) (struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char *buf);
	int (*lock_user_prot_reg) (struct mtd_info *mtd, loff_t from, size_t len);

/* XXX U-BOOT XXX */
#if 0
	/* kvec-based read/write methods.
	   NB: The 'count' parameter is the number of _vectors_, each of
	   which contains an (ofs, len) tuple.
	*/
	int (*writev) (struct mtd_info *mtd, const struct kvec *vecs, unsigned long count, loff_t to, size_t *retlen);
#endif

	/* Sync */
	void (*sync) (struct mtd_info *mtd);

	/* Chip-supported device locking */
	int (*lock) (struct mtd_info *mtd, loff_t ofs, uint64_t len);
	int (*unlock) (struct mtd_info *mtd, loff_t ofs, uint64_t len);

	/* Power Management functions */
	int (*suspend) (struct mtd_info *mtd);
	void (*resume) (struct mtd_info *mtd);

	/* Bad block management functions */
	int (*block_isbad) (struct mtd_info *mtd, loff_t ofs);
	int (*block_markbad) (struct mtd_info *mtd, loff_t ofs);

/* XXX U-BOOT XXX */
#if 0
	struct notifier_block reboot_notifier;  /* default mode before reboot */
#endif

	/* ECC status information */
	struct mtd_ecc_stats ecc_stats;
	/* Subpage shift (NAND) */
	int subpage_sft;

	void *priv;

	struct module *owner;
	int usecount;

	/* If the driver is something smart, like UBI, it may need to maintain
	 * its own reference counting. The below functions are only for driver.
	 * The driver may register its callbacks. These callbacks are not
	 * supposed to be called by MTD users */
	int (*get_device) (struct mtd_info *mtd);
	void (*put_device) (struct mtd_info *mtd);
};

struct dmw96_nfc_info {
	struct clk *clk;
	struct mtd_info *mtd;
	struct nand_chip *chip;
	void __iomem *regs;
	int chipsel_nr;
	int ready_nr;

	int to_read;

	unsigned char n1len, n2len, n3len;
	int dirtypos;

	unsigned char *data_buff;
	dma_addr_t data_buff_phys;

#ifdef __LINUX_KERNEL__
	struct semaphore mutex;
#endif
	unsigned long offset;
	int bypass;
	int use_bypass;
	int ecc;
	int use_ecc;
	int ecc_mode;

	/* queue of ecc correction codes */
	uint32_t ecc_codes[DMW96_NFC_MAX_ERRORS_PER_PAGE];
	int ecc_numcodes;

	int wait_ready_0_fall;
	int wait_ready_0_rise;
	int wait_ready_1_fall;
	int wait_ready_1_rise;
	int wait_ready_2_fall;
	int wait_ready_2_rise;
	int wait_ready_3_fall;
	int wait_ready_3_rise;
	int wait_ready_4_fall;
	int wait_ready_4_rise;
	int wait_ready_5_fall;
	int wait_ready_5_rise;
	int wait_ready_6_fall;
	int wait_ready_6_rise;

	int pulse_ale_setup;
	int pulse_cle_setup;
	int pulse_wr_high;
	int pulse_wr_low;
	int pulse_rd_high;
	int pulse_rd_low;

	int ready_timeout;
};

/* define hooks into the legacy rom */
static struct dmw96_nfc_info *nfc_info =
	(void *)(DMW96_PSRAM_BASE + 0x3be0);

static int (*nand_read_skip_bad)(struct mtd_info *mtd, loff_t offset,
                                 size_t *length, u_char *buffer) =
	(void *)(DMW96_ROM_BASE + 0x1180);

static int
nand_to_ram(struct mtd_info *mtd, void *dest, uint32_t off, uint32_t size)
{
	int ret;
	size_t length = size;

	/* skip bad blocks while maintaining the offset into the block */
	while (mtd->block_isbad(mtd, off))
		off += mtd->erasesize;

	/* read the data skipping along the bad blocks within length */
	ret = nand_read_skip_bad(mtd, off, &length, dest);

	if (ret)
		return -1;

	return 0;
}

int legacy_nand_read(void *buf, unsigned long off, unsigned long size)
{
	struct mtd_info *mtd = nfc_info->mtd;

	return nand_to_ram(mtd, buf, off, size);
}

void legacy_nand_clkchg(int clk, unsigned long freq)
{
	/* setup slower timing because of higher clock */
	nfc_info->pulse_wr_high = 4;
	nfc_info->pulse_wr_low  = 4;
	nfc_info->pulse_rd_high = 4;
	nfc_info->pulse_rd_low  = 4;
}
