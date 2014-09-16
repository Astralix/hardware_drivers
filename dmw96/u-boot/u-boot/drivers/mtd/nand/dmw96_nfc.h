#ifndef DMW96_NFC_H
#define DMW96_NFC_H

#include <linux/mtd/mtd.h>

/*
 *  NAND FLASH/LCD Register Offsets.
 */
#define DW_FC_CTL		0x0000		/* FC control register */
#define DW_FC_STATUS		0x0004		/* FC status register */
#define DW_FC_STS_CLR		0x0008		/* FC status clear register */
#define DW_FC_INT_EN		0x000C		/* FC interrupt enable mask register */
#define DW_FC_INT_CAUSE		0x0010		/* FC interrupt enable mask register */
#define DW_FC_SEQUENCE		0x0014		/* FC sequence register */
#define DW_FC_ADDR_COL		0x0018		/* FC address-column register */
#define DW_FC_ADDR_ROW		0x001C		/* FC address-row register */
#define DW_FC_CMD1		0x0020		/* FC command code configuration register */
#define DW_FC_CMD2		0x0024		/* FC command code configuration register */
#define DW_FC_WAIT1		0x0028		/* FC wait time configuration register */
#define DW_FC_WAIT2		0x002C		/* FC wait time configuration register */
#define DW_FC_WAIT3		0x0030		/* FC wait time configuration register */
#define DW_FC_WAIT4		0x0034		/* FC wait time configuration register */
#define DW_FC_PULSETIME		0x0038		/* FC pulse time configuration register */
#define DW_FC_DCOUNT		0x003C		/* FC data count register */
#define DW_FC_FTSR		0x0040		/* FC transfer size register */
#define DW_FC_DPR		0x0044		/* FC data pointer register */
#define DW_FC_RDPR		0x0048		/* FC redundancy data pointer register */
#define DW_FC_TIMEOUT		0x004C		/* FC timeout configuration register */
#define DW_FC_BCH		0x0050		/* FC ECC BCH code out register */
#define DW_FC_HAMMING		0x0054		/* FC ECC Hamming code out register */
#define DW_FC_START		0x0058		/* FC BCH Errors locatoins out register */
#define DW_FC_FBYP_CTL		0x005C		/* FC GF FIFO bypass control register */
#define DW_FC_FBYP_DATA		0x0060		/* FC GF FIFO bypass data register */
#define DW_FC_LCD_HADD		0x0064		/* FC LCDC high address */

#define DW_FC_CHIEN_SEARCH_1	0x0068		/* FC Chien search coefficient 1 */
#define DW_FC_CHIEN_SEARCH_2	0x006C		/* FC Chien search coefficient 2 */
#define DW_FC_CHIEN_SEARCH_3	0x0070		/* FC Chien search coefficient 3 */
#define DW_FC_CHIEN_SEARCH_4	0x0074		/* FC Chien search coefficient 4 */
#define DW_FC_CHIEN_SEARCH_5	0x0078		/* FC Chien search coefficient 5 */
#define DW_FC_CHIEN_SEARCH_6	0x007C		/* FC Chien search coefficient 6 */
#define DW_FC_CHIEN_SEARCH_7	0x0080		/* FC Chien search coefficient 7 */
#define DW_FC_CHIEN_SEARCH_8	0x0084		/* FC Chien search coefficient 8 */
#define DW_FC_CHIEN_SEARCH_9	0x0088		/* FC Chien search coefficient 9 */
#define DW_FC_CHIEN_SEARCH_10	0x008C		/* FC Chien search coefficient 10 */
#define DW_FC_CHIEN_SEARCH_11	0x0090		/* FC Chien search coefficient 11 */
#define DW_FC_CHIEN_SEARCH_12	0x0094		/* FC Chien search coefficient 12 */
#define DW_FC_CHIEN_SEARCH_13	0x0098		/* FC Chien search coefficient 13 */
#define DW_FC_CHIEN_SEARCH_14	0x009C		/* FC Chien search coefficient 14 */
#define DW_FC_CHIEN_SEARCH_15	0x00A0		/* FC Chien search coefficient 15 */
#define DW_FC_CHIEN_SEARCH_16	0x00A4		/* FC Chien search coefficient 16 */
#define DW_FC_CHIEN_SEARCH_17	0x00A8		/* FC Chien search coefficient 17 */
#define DW_FC_CHIEN_SEARCH_18	0x00AC		/* FC Chien search coefficient 18 */
#define DW_FC_CHIEN_SEARCH_19	0x00B0		/* FC Chien search coefficient 19 */
#define DW_FC_CHIEN_SEARCH_20	0x00B4		/* FC Chien search coefficient 20 */
#define DW_FC_CHIEN_SEARCH_21	0x00B8		/* FC Chien search coefficient 21 */
#define DW_FC_CHIEN_SEARCH_22	0x00BC		/* FC Chien search coefficient 22 */
#define DW_FC_CHIEN_SEARCH_23	0x00C0		/* FC Chien search coefficient 23 */
#define DW_FC_CHIEN_SEARCH_24	0x00C4		/* FC Chien search coefficient 24 */

#define DW_FC_AHB_CTL		0x00C8		/* FC AHB DMA control */
#define DW_FC_AHB_DBG_1		0x00CC		/* FC AHB Debug register 1 */
#define DW_FC_AHB_DBG_2		0x00D0		/* FC AHB Debug register 2 */
#define DW_FC_DBG_1		0x00D4		/* FC NFC Debug register 1 */
#define DW_FC_DBG_2		0x00D8		/* FC NFC Debug register 2 */
#define DW_FC_ECC_DBG_1		0x00DC		/* FC NFC ECC Debug register 1 */
#define DW_FC_ECC_DBG_2		0x00E0		/* FC NFC ECC Debug register 2 */
#define DW_FC_ECC_DBG_3		0x00E4		/* FC NFC ECC Debug register 3 */
#define DW_FC_ECC_DBG_4		0x00E8		/* FC NFC ECC Debug register 4 */
#define DW_FC_ECC_DBG_5		0x00EC		/* FC NFC ECC Debug register 5 */

#define DW_FC_CTL_ECC_OP_MODE(a)        (((a) & 0xF) << 0)
#define DW_FC_CTL_LCD_IM(a)             (((a) & 0x1) << 9)
#define DW_FC_CTL_LCD_CLE_INV(a)        (((a) & 0x1) << 10)
#define DW_FC_CTL_CHIEN_CNT_START(a)    (((a) & 0x1FFF) << 16)

#define DW_FC_MODULE_DISABLED       (0x0)
#define DW_FC_1_BIT_ECC             (0x1)
#define DW_FC_4_BIT_ECC             (0x2)
#define DW_FC_8_BIT_ECC             (0x3)
#define DW_FC_12_BIT_ECC            (0x4)

#define DW_FC_STATUS_TRANS_DONE     (1 << 0)
#define DW_FC_STATUS_ECC_DONE       (1 << 1)
#define DW_FC_STATUS_ERR_FOUND      (1 << 2)
#define DW_FC_STATUS_RDY_TIMEOUT    (1 << 3)
#define DW_FC_STATUS_RDY1_RISE      (1 << 4)
#define DW_FC_STATUS_RDY2_RISE      (1 << 5)
#define DW_FC_STATUS_TRANS_BUSY     (1 << 6)
#define DW_FC_STATUS_ECC_BUSY       (1 << 7)
#define DW_FC_STATUS_RDY1_STAT      (1 << 8)
#define DW_FC_STATUS_RDY2_STAT      (1 << 9)
#define DW_FC_STATUS_ECC_ERR_FF     (1 << 10)
#define DW_FC_STATUS_ECC_NC_ERR     (1 << 11)
#define DW_FC_STATUS_ECC_NC_SEQ     (1 << 12) /* 4 bits */
#define DW_FC_STATUS_AHB_ERR        (1 << 16)
#define DW_FC_STATUS_AHB_DONE       (1 << 17)

#define DW_FC_SEQ_CMD1_EN           (1 << 0)
#define DW_FC_SEQ_WAIT0_EN          (1 << 1)
#define DW_FC_SEQ_ADD1_EN           (1 << 2)
#define DW_FC_SEQ_ADD2_EN           (1 << 3)
#define DW_FC_SEQ_ADD3_EN           (1 << 4)
#define DW_FC_SEQ_ADD4_EN           (1 << 5)
#define DW_FC_SEQ_ADD5_EN           (1 << 6)
#define DW_FC_SEQ_WAIT1_EN          (1 << 7)
#define DW_FC_SEQ_CMD2_EN           (1 << 8)
#define DW_FC_SEQ_WAIT2_EN          (1 << 9)
#define DW_FC_SEQ_RW_EN             (1 << 10)
#define DW_FC_SEQ_RW_DIR            (1 << 11)
#define DW_FC_SEQ_DATA_READ_DIR     (0 << 11)
#define DW_FC_SEQ_DATA_WRITE_DIR    (1 << 11)
#define DW_FC_SEQ_DATA_ECC(a)       ((a & 0x1) << 12) // FIXME: check if a==0 (If a!={0,1} the code is wrong)
#define DW_FC_SEQ_CMD3_EN           (1 << 13)
#define DW_FC_SEQ_WAIT3_EN          (1 << 14)
#define DW_FC_SEQ_CMD4_EN           (1 << 15)
#define DW_FC_SEQ_WAIT4_EN          (1 << 16)
#define DW_FC_SEQ_READ_ONCE         (1 << 17)
#define DW_FC_SEQ_CHIP_SEL(a)       ((a & 0x3) << 18)
#define DW_FC_SEQ_KEEP_CS           (1 << 20)
#define DW_FC_SEQ_MODE8             (0 << 21)
#define DW_FC_SEQ_MODE16            (1 << 21)
#define DW_FC_SEQ_RDY_EN            (1 << 22)
#define DW_FC_SEQ_RDY_SEL(a)        (((a) & 0x1) << 23)
#define DW_FC_SEQ_CMD5_EN           (1 << 24)
#define DW_FC_SEQ_WAIT5_EN          (1 << 25)
#define DW_FC_SEQ_CMD6_EN           (1 << 26)
#define DW_FC_SEQ_WAIT6_EN          (1 << 27)

#define DW_FC_ADD1(a)               (((a) & 0xFF) << 0)
#define DW_FC_ADD2(a)               (((a) & 0xFF) << 8)

#define DW_FC_ADD3(a)               (((a) & 0xFF) << 0)
#define DW_FC_ADD4(a)               (((a) & 0xFF) << 8)
#define DW_FC_ADD5(a)               (((a) & 0xFF) << 16)

#define DW_FC_CMD_1(a)              (((a) & 0xFF) << 0)
#define DW_FC_CMD_2(a)              (((a) & 0xFF) << 8)
#define DW_FC_CMD_3(a)              (((a) & 0xFF) << 16)
#define DW_FC_CMD_4(a)              (((a) & 0xFF) << 24)

#define DW_FC_CMD_5(a)              (((a) & 0xFF) << 0)
#define DW_FC_CMD_6(a)              (((a) & 0xFF) << 8)

#define DW_FC_WAIT0A(a)             (((a) & 0x3F) << 0)
#define DW_FC_WAIT0B(a)             (((a) & 0x3F) << 8)
#define DW_FC_WAIT1A(a)             (((a) & 0x3F) << 0)
#define DW_FC_WAIT1B(a)             (((a) & 0x3F) << 8)
#define DW_FC_WAIT2A(a)             (((a) & 0x3F) << 16)
#define DW_FC_WAIT2B(a)             (((a) & 0x3F) << 24)
#define DW_FC_WAIT3A(a)             (((a) & 0x3F) << 0)
#define DW_FC_WAIT3B(a)             (((a) & 0x3F) << 8)
#define DW_FC_WAIT4A(a)             (((a) & 0x3F) << 16)
#define DW_FC_WAIT4B(a)             (((a) & 0x3F) << 24)
#define DW_FC_WAIT5A(a)             (((a) & 0x3F) << 0)
#define DW_FC_WAIT5B(a)             (((a) & 0x3F) << 8)
#define DW_FC_WAIT6A(a)             (((a) & 0x3F) << 16)
#define DW_FC_WAIT6B(a)             (((a) & 0x3F) << 24)

#define DW_FC_PT_READ_LOW(a)        (((a) & 0xF) << 0)
#define DW_FC_PT_READ_HIGH(a)       (((a) & 0xF) << 4)
#define DW_FC_PT_WRITE_LOW(a)       (((a) & 0xF) << 8)
#define DW_FC_PT_WRITE_HIGH(a)      (((a) & 0xF) << 12)
#define DW_FC_CLE_SETUP(a)          (((a) & 0xF) << 16)
#define DW_FC_ALE_SETUP(a)          (((a) & 0xF) << 20)
#define DW_FC_PT_SPARE_WORDS(a)     (((a) & 0x3F) << 24)

#define DW_FC_DCOUNT_VP_SIZE(a)     (((a) & 0x1FFF) << 0)
#define DW_FC_DCOUNT_SPARE_N3(a)    (((a) & 0x7) << 13)
#define DW_FC_DCOUNT_SPARE_N2(a)    (((a) & 0xFF) << 16)
#define DW_FC_DCOUNT_SPARE_N1(a)    (((a) & 0xF) << 24)
#define DW_FC_DCOUNT_PAGE_CNT(a)    (((a) & 0xF) << 28)

#define DW_FC_FTSR_TRANSFER_SIZE(a) (((a) & 0x1FFF) << 0)
#define DW_FC_FTSR_SEQ_NUM(a)       (((a) & 0xF) << 16)
#define DW_FC_FTSR_VP_NUM(a)        (((a) & 0xF) << 20)

#define DW_FC_TIMEOUT_RDY_CNT(a)    (((a) & 0x7F) << 0)
#define DW_FC_TIMEOUT_RDY_EN        (1 << 8)

#define DW_FC_BCH_H                 (0x7FFFF << 0)
#define DW_FC_BCH_DEC_BUF           (0x7 << 20)

#define DW_FC_HAMMING_CODE_OUT       	(0x1FFF << 0)

#define DW_FC_FBYP_CTL_BUSY             (1 << 0)
#define DW_FC_FBYP_CTL_BP_EN            (1 << 1)
#define DW_FC_FBYP_CTL_BP_WRITE         (1 << 2)
#define DW_FC_FBYP_CTL_BP_READ_REG      (1 << 3)
#define DW_FC_FBYP_CTL_BP_READ_FLASH    (1 << 4)

#define DW_FC_FBYP_DATA_BYPASS_DATA       (0xFFFF << 0)

#define VIRTUAL_PAGESIZE            512
#define VIRTUAL_PAGESIZE_SHIFT        9
#define DMW96_NFC_MAX_ERRORS_PER_VP  16
#define DMW96_NFC_MAX_ERRORS_PER_PAGE \
	((NAND_MAX_PAGESIZE >> VIRTUAL_PAGESIZE_SHIFT) * \
	 DMW96_NFC_MAX_ERRORS_PER_VP)

// Time values in CLK Cycles
#ifdef CONFIG_MTD_NAND_DMW96_CONFIGURE_TIMING
	#define WAIT_READY_0_FALL CONFIG_MTD_NAND_DMW96_WAIT_READY_0_FALL
	#define WAIT_READY_0_RISE CONFIG_MTD_NAND_DMW96_WAIT_READY_0_RISE
	#define WAIT_READY_1_FALL CONFIG_MTD_NAND_DMW96_WAIT_READY_1_FALL
	#define WAIT_READY_1_RISE CONFIG_MTD_NAND_DMW96_WAIT_READY_1_RISE
	#define WAIT_READY_2_FALL CONFIG_MTD_NAND_DMW96_WAIT_READY_2_FALL
	#define WAIT_READY_2_RISE CONFIG_MTD_NAND_DMW96_WAIT_READY_2_RISE
	#define WAIT_READY_3_FALL CONFIG_MTD_NAND_DMW96_WAIT_READY_3_FALL
	#define WAIT_READY_3_RISE CONFIG_MTD_NAND_DMW96_WAIT_READY_3_RISE
	#define WAIT_READY_4_FALL CONFIG_MTD_NAND_DMW96_WAIT_READY_4_FALL
	#define WAIT_READY_4_RISE CONFIG_MTD_NAND_DMW96_WAIT_READY_4_RISE
	#define WAIT_READY_5_FALL CONFIG_MTD_NAND_DMW96_WAIT_READY_5_FALL
	#define WAIT_READY_5_RISE CONFIG_MTD_NAND_DMW96_WAIT_READY_5_RISE
	#define WAIT_READY_6_FALL CONFIG_MTD_NAND_DMW96_WAIT_READY_6_FALL
	#define WAIT_READY_6_RISE CONFIG_MTD_NAND_DMW96_WAIT_READY_6_RISE

	#define PULSE_ALE_SETUP   CONFIG_MTD_NAND_DMW96_PULSE_ALE_SETUP
	#define PULSE_CLE_SETUP   CONFIG_MTD_NAND_DMW96_PULSE_CLE_SETUP
	#define PULSE_WR_HIGH     CONFIG_MTD_NAND_DMW96_PULSE_WR_HIGH
	#define PULSE_WR_LOW      CONFIG_MTD_NAND_DMW96_PULSE_WR_LOW
	#define PULSE_RD_HIGH     CONFIG_MTD_NAND_DMW96_PULSE_RD_HIGH
	#define PULSE_RD_LOW      CONFIG_MTD_NAND_DMW96_PULSE_RD_LOW

	#define READY_TIMEOUT     CONFIG_MTD_NAND_DMW96_READY_TIMEOUT
#else
	// WaitRdy values must be 2-15 if READYi line is used, 0 otherwise.
	#define WAIT_READY_0_FALL 4
	#define WAIT_READY_0_RISE 4
	#define WAIT_READY_1_FALL 4
	#define WAIT_READY_1_RISE 4
	#define WAIT_READY_2_FALL 4
	#define WAIT_READY_2_RISE 4
	#define WAIT_READY_3_FALL 4
	#define WAIT_READY_3_RISE 4
	#define WAIT_READY_4_FALL 4
	#define WAIT_READY_4_RISE 4
	#define WAIT_READY_5_FALL 4
	#define WAIT_READY_5_RISE 4
	#define WAIT_READY_6_FALL 4
	#define WAIT_READY_6_RISE 4

	// Write & Read pulse shape: value = n --> (n+1) wait cycles generated
	#define PULSE_ALE_SETUP   4
	#define PULSE_CLE_SETUP   4
	#define PULSE_WR_HIGH     4
	#define PULSE_WR_LOW      4
	#define PULSE_RD_HIGH     4
	#define PULSE_RD_LOW      4

	// Wait for Ready back high - overall transaction timeout
	#define READY_TIMEOUT     32 // timeout > 10 mSec
#endif

struct dmw96_nfc_info {
	struct clk *clk;
	struct mtd_info *mtd;
	struct nand_chip *chip;
	void __iomem *regs;
	int chipsel_nr;
	int ready_nr;

	unsigned char n1len, n2len, n3len;
	int dirtypos;

	unsigned char *data_buff;
	dma_addr_t data_buff_phys;

#ifdef __LINUX_KERNEL__
	struct semaphore mutex;
#endif
	unsigned long offset;
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

int dmw96_nfc_use_ecc(struct mtd_info *mtd, int enable);
int dmw96_nfc_ecc_mode(struct mtd_info *mtd, int ecc_mode);
int dmw96_nfc_use_bypass(struct mtd_info *mtd, int enable);

#endif /* DMW96_NFC_H */
