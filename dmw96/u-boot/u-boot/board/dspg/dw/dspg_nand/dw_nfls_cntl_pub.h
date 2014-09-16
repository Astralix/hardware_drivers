/********************************************************************************
*              N A N D   F L A S H   C O N T R O L L E R
*********************************************************************************
*
*   Title:           Low-Level NAND-Flash controller Interface.
*
*   Filename:        $Workfile: $
*   SubSystem:       DM-56/ ARM 7
*   Authors:         Avi Miller
*   Latest update:   $Modtime: $
*   Created:         13 Mars, 2005.
*
*********************************************************************************
*  Description:      Interface to upper layer of Low-Level NAND-Flash controller.
*                    Running on ARM 7 in the DM-56 chip.
*                    Providing access to 32 - 512 MByte External flash chips,
*                    having 512 - 2K Bytes Page sizes; 8-bit or 16 access mode.
*********************************************************************************
*  $Header: $
*
*  Change history:
*  ---------------
*  $History: $
*
********************************************************************************/

#ifndef _NAND_FLASH_CONTROLLER_PUB_H_
#define _NAND_FLASH_CONTROLLER_PUB_H_

/*========================  Include Files  ===============================*/

#ifdef __LINUX_KERNEL__
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/rslib.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/errno.h>

#include <linux/mtd/doc2000.h>
#include <linux/mtd/compatmac.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/inftl.h>

#include <mach/platform.h>

#else

#include <common.h>
#include <malloc.h>
#include <asm/errno.h>
#include "../dw.h"

#define down(x)
#define up(x)

#define NFLC_CAPTURE(x)

#define cpu_relax(x) do { } while(0);

#define mdelay(x) udelay((x)*1000)

#endif

#include <asm/io.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>

#include "dw_nfls_type_defs.h"

#ifdef CONFIG_MTD_NAND_DW_DEBUG
#       ifdef __KERNEL__
#               define PDEBUG(fmt, args...) printk( KERN_ERR "DW_NAND: %s(%d) " fmt, __func__, __LINE__,  ## args)
#       else
#               define PDEBUG(fmt, args...) fprintf( stderr, "DW_NAND: %s(%d) " fmt, __func__, __LINE__,  ## args)
#       endif
#else
#               define PDEBUG(fmt, args...) do {} while (0)
#endif

#if defined (CONFIG_MTD_NAND_DW_DEBUG) && defined (CONFIG_MTD_NAND_DW_DEBUG_VERBOSE)
        #define DW_SPOT(args...)        \
                do {                    \
                        printk(KERN_ERR "********************** DW_NAND: %s(%d)", __func__, __LINE__);   \
                        printk( " "  args );            \
                        printk( "\n" );                 \
                        {\
                                volatile int i=0;\
                                for (i=0; i<100000; i++);\
                        }\
                } while (0)
#else
        #define DW_SPOT(args...)        \
                do {} while (0)
#endif

/*=======================  Comlilation Switches  =========================*/

//#define PC_SIMULATION    // for compilation only(!) in PC environment
#define DM56_SIM_ON_FPGA   // for DM-56 emulation in an FPGA (Altera)
#define SYS_CLK_24MHz      // for DM-56 emulation on evaluation board with 24MHz oscillator

/*========================  HW Definitions  ===============================*/

/*
 *  NAND FLASH/LCD Register Offsets.
 */
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
#define DW_FC_TIMEOUT		0x002C		/* FC timeout configuration register */
#define DW_FC_ECC_STS		0x0034		/* FC ECC status register */
#define DW_FC_ECC_S_12		0x0038		/* FC ECC S1 and S2 register */
#define DW_FC_ECC_S_34		0x003C		/* FC ECC S3 and S4 register */
#define DW_FC_ECC_S_57		0x0040		/* FC ECC S5 and S7 register */
#define DW_FC_HAMMING		0x0044		/* FC ECC Hamming code out register */
#define DW_FC_BCH_READ_L	0x0048		/* FC BCH syndrome read low */
#define DW_FC_BCH_READ_H	0x004C		/* FC BCH syndrome read high */
#define DW_FC_GF_MUL_OUT	0x0050		/* FC GF multiplier output register */
#define DW_FC_GF_MUL_IN		0x0054		/* FC GF multiplier input register */
#define DW_FC_FBYP_CTL		0x0058		/* FC GF FIFO bypass control register */
#define DW_FC_FBYP_DATA		0x005C		/* FC GF FIFO bypass data register */

#define DW_FC_CTL_ECC_OP_MODE(a)        (((a) & 0xF) << 0)
#define DW_FC_CTL_PAGE_SELECT(a)        (((a) & 0xF) << 4)
#define DW_FC_CTL_CHAIN_CNT_START(a)    (((a) & 0x1FFF) << 16)
#define DW_FC_CTL_CHAIN_CNT_START_LOC   (0x1FFF << 16)

#define DW_FC_MODULE_DISABLED       (0x0)
#define DW_FC_MEM_TRANS             (0x1)
#define DW_FC_S_CALC                (0x2)
#define DW_FC_CHAIN_SRCH            (0x3)
#define DW_FC_CHAIN_SRCH_CONT       (0x4)
#define DW_FC_READ_ERR_CODE         (0x8)
#define DW_FC_WRITE_ERR_CODE        (0x9)

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
//#define DW_FC_STATUS_ERR_LOC_MASK   (0x1FFFF << 16) // R.B.: Bug fix
#define DW_FC_STATUS_ERR_LOC_MASK   (0x1FFF << 16)

#define DW_FC_SEQ_CMD1_EN           (1 << 0)
#define DW_FC_SEQ_ADD1_EN           (1 << 1)
#define DW_FC_SEQ_ADD2_EN           (1 << 2)
#define DW_FC_SEQ_ADD3_EN           (1 << 3)
#define DW_FC_SEQ_ADD4_EN           (1 << 4)
#define DW_FC_SEQ_ADD5_EN           (1 << 5)
#define DW_FC_SEQ_WAIT1_EN          (1 << 6)
#define DW_FC_SEQ_CMD2_EN           (1 << 7)
#define DW_FC_SEQ_WAIT2_EN          (1 << 8)
#define DW_FC_SEQ_RW_EN             (1 << 9)
#define DW_FC_SEQ_DATA_READ_DIR     (0 << 10)
#define DW_FC_SEQ_DATA_WRITE_DIR    (1 << 10)
#define DW_FC_SEQ_DATA_ECC(a)       ((a & 0x1) << 11) // FIXME: check if a==0 (If a!={0,1} the code is wrong)
#define DW_FC_SEQ_CMD3_EN           (1 << 12)
#define DW_FC_SEQ_WAIT3_EN          (1 << 13)
#define DW_FC_SEQ_CMD4_EN           (1 << 14)
#define DW_FC_SEQ_WAIT4_EN          (1 << 15)
#define DW_FC_SEQ_READ_ONCE         (1 << 16)
#define DW_FC_SEQ_CHIP_SEL(a)       ((a & 0x3) << 17)
#define DW_FC_SEQ_KEEP_CS           (1 << 19)
#define DW_FC_SEQ_MODE8             (0 << 20)
#define DW_FC_SEQ_MODE16            (1 << 20)
#define DW_FC_SEQ_RDY_EN            (1 << 21)
#define DW_FC_SEQ_RDY_SEL(a)        (((a) & 0x1) << 22)

#define DW_FC_ADD1(a)               (((a) & 0xFF) << 0)
#define DW_FC_ADD2(a)               (((a) & 0xFF) << 8)

#define DW_FC_ADD3(a)               (((a) & 0xFF) << 0)
#define DW_FC_ADD4(a)               (((a) & 0xFF) << 8)
#define DW_FC_ADD5(a)               (((a) & 0xFF) << 16)

#define DW_FC_CMD1(a)              (((a) & 0xFF) << 0)
#define DW_FC_CMD2(a)              (((a) & 0xFF) << 8)
#define DW_FC_CMD3(a)              (((a) & 0xFF) << 16)
#define DW_FC_CMD4(a)              (((a) & 0xFF) << 24)

#define DW_FC_WAIT1A(a)             (((a) & 0xF) << 0)
#define DW_FC_WAIT1B(a)             (((a) & 0xF) << 4)
#define DW_FC_WAIT2A(a)             (((a) & 0xF) << 8)
#define DW_FC_WAIT2B(a)             (((a) & 0xF) << 12)
#define DW_FC_WAIT3A(a)             (((a) & 0xF) << 16)
#define DW_FC_WAIT3B(a)             (((a) & 0xF) << 20)
#define DW_FC_WAIT4A(a)             (((a) & 0xF) << 24)
#define DW_FC_WAIT4B(a)             (((a) & 0xF) << 28)

#define DW_FC_PT_READ_LOW(a)        (((a) & 0xF) << 0)
#define DW_FC_PT_READ_HIGH(a)       (((a) & 0xF) << 4)
#define DW_FC_PT_WRITE_LOW(a)       (((a) & 0xF) << 8)
#define DW_FC_PT_WRITE_HIGH(a)      (((a) & 0xF) << 12)
#define DW_FC_CLE_SETUP(a)          (((a) & 0xF) << 16)

#define DW_FC_DCOUNT_MAIN_CNT(a)    (((a) & 0x1FFF) << 0)
#define DW_FC_DCOUNT_SPARE_N2(a)    (((a) & 0xFF) << 16)
#define DW_FC_DCOUNT_SPARE_N1(a)    (((a) & 0xF) << 24)
#define DW_FC_DCOUNT_PAGE_CNT(a)    (((a) & 0xF) << 28)

#define DW_FC_TIMEOUT_RDY_CNT(a)    (((a) & 0x7F) << 0)
#define DW_FC_TIMEOUT_RDY_EN        (1 << 8)

#define DW_FC_ECC_STS_P0_BCH_ERR    (1 << 0)
#define DW_FC_ECC_STS_P1_BCH_ERR    (1 << 1)
#define DW_FC_ECC_STS_P2_BCH_ERR    (1 << 2)
#define DW_FC_ECC_STS_P3_BCH_ERR    (1 << 3)
#define DW_FC_ECC_STS_P4_BCH_ERR    (1 << 4)
#define DW_FC_ECC_STS_P5_BCH_ERR    (1 << 5)
#define DW_FC_ECC_STS_P6_BCH_ERR    (1 << 6)
#define DW_FC_ECC_STS_P7_BCH_ERR    (1 << 7)
#define DW_FC_ECC_STS_P0_HAM_ERR    (1 << 16)
#define DW_FC_ECC_STS_P1_HAM_ERR    (1 << 17)
#define DW_FC_ECC_STS_P2_HAM_ERR    (1 << 18)
#define DW_FC_ECC_STS_P3_HAM_ERR    (1 << 19)
#define DW_FC_ECC_STS_P4_HAM_ERR    (1 << 20)
#define DW_FC_ECC_STS_P5_HAM_ERR    (1 << 21)
#define DW_FC_ECC_STS_P6_HAM_ERR    (1 << 22)
#define DW_FC_ECC_STS_P7_HAM_ERR    (1 << 23)

#define DW_FC_ECC_LOW               (0x1FFF << 0)
#define DW_FC_ECC_HIGH              (0x1FFF << 16)

#define DW_FC_ECC_S1(a)             (((a) & 0x1FFF) << 0)
#define DW_FC_ECC_S2(a)             (((a) & 0x1FFF) << 16)

#define DW_FC_ECC_S3(a)             (((a) & 0x1FFF) << 0)
#define DW_FC_ECC_S4(a)             (((a) & 0x1FFF) << 16)

#define DW_FC_ECC_S5                (0x1FFF << 0)
#define DW_FC_ECC_S7                (0x1FFF << 16)

#define DW_FC_BCH_H                 (0x7FFFF << 0)
#define DW_FC_BCH_DEC_BUF           (0x7 << 20)

//#define DW_FC_GF_MUL_OUT_M_OUT          (0xFFF << 0) //R.B. BUG!!!
#define DW_FC_GF_MUL_OUT_M_OUT          (0x1FFF << 0)
#define DW_FC_GF_MUL_OUT_M_BUSY         (1 << 31)

#define DW_FC_GF_MUL_IN_A(a)            (((a) & 0x1FFF) << 0)
#define DW_FC_GF_MUL_IN_B(a)            (((a) & 0x1FFF) << 16)

#define DW_FC_HAMMING_CODE_OUT       	(0x1FFF << 0)

#define DW_FC_FBYP_CTL_BUSY             (1 << 0)
#define DW_FC_FBYP_CTL_BP_EN            (1 << 1)
#define DW_FC_FBYP_CTL_BP_WRITE         (1 << 2)
#define DW_FC_FBYP_CTL_BP_READ_REG      (1 << 3)
#define DW_FC_FBYP_CTL_BP_READ_FLASH    (1 << 4)

#define DW_FC_FBYP_DATA_FIFO_DATA       (0xFFFF << 0)
#define DW_FC_FBYP_DATA_FLASH_DATA      (0xFFFF << 16)

/*
 *  DMA Register Offsets.
 */
#define DW_DMA_CTL	  	  		0x0000 		/* DMA control register */
#define DW_DMA_ADDR1  	  		0x0004 		/* DMA address 1 */
#define DW_DMA_ADDR2            0x0008 		/* DMA address 2 */
#define DW_DMA_BURST            0x000C 		/* DMA burst length */
#define DW_DMA_BLK_LEN          0x0010 		/* DMA transfer block length (words) */
#define DW_DMA_BLK_CNT          0x0014 		/* DMA block transfer count */
#define DW_DMA_ISR              0x0018 		/* DMA interrupt status */
#define DW_DMA_IER              0x001C 		/* DMA interrupt enable */
#define DW_DMA_ICR              0x0020 		/* DMA interrupt clear */
#define DW_DMA_START            0x0024 		/* DMA start trigger */

#define DW_DMA_EN                   (1 << 0)
#define DW_DMA_DMA_MODE(a)          (((a) & 0x1) << 1)
#define DW_DMA_FLASH_MODE(a)        (((a) & 0x1) << 2)
#define DW_DMA_FLASH_ECC_WORDS(a)   (((a) & 0x3f) << 4)
#define DW_DMA_SWAP_MODE(a)         (((a) & 0x3) << 12)

#define DW_DMA_ISR_DMA_DONE     (1 << 0)
#define DW_DMA_ISR_DMA_ERROR    (1 << 1)

#define DW_DMA_IER_DMA_DONE     (1 << 0)
#define DW_DMA_IER_DMA_ERROR    (1 << 1)

#define DW_DMA_ICR_DMA_DONE     (1 << 0)
#define DW_DMA_ICR_DMA_ERROR    (1 << 1)

#define DW_DMA_START_TRIG       (1 << 0)

/*
 *  FIFO Register Offsets.
 */
#define DW_FIFO_CTL	  	  		0x0000 		/* FIFO control register */
#define DW_FIFO_AE_LEVEL  	  	0x0004 		/* FIFO almose empty level */
#define DW_FIFO_AF_LEVEL  	  	0x0008 		/* FIFO almost full level */
#define DW_FIFO_ISR             0x000C      /* FIFO interrupt status */
#define DW_FIFO_IER             0x0010      /* FIFO interrupot enable */
#define DW_FIFO_ICR             0x0014      /* FIFO interrupt clear*/
#define DW_FIFO_RST             0x0018      /* FIFO reset */
#define DW_FIFO_VAL             0x001C      /* FIFO value */
#define DW_FIFO_TST_EN		0x0020      /* FIFO RAM test enable */ // R.B 22/05/2008

#define DW_FIFO_CTL_FIFO_EN         (1 << 0)
#define DW_FIFO_CTL_FIFO_WIDTH(a)   (((a) & 0x1) << 1)
//#define DW_FIFO_CTL_FIFO_MODE(a)    ((a & 0x1) << 2)
#define DW_FIFO_CTL_FIFO_MODE(Direction) (((Direction == DMF_K_DmaDirReadAHB) ? C_FIFO_FifoMode_WriteFls : C_FIFO_FifoMode_ReadFls) << 2)
#define DW_FIFO_ISR_OVER        (1 << 0)
#define DW_FIFO_ISR_UNDER       (1 << 1)

#define DW_FIFO_IER_OVER        (1 << 0)
#define DW_FIFO_IER_UNDER       (1 << 1)

#define DW_FIFO_ICR_OVER        (1 << 0)
#define DW_FIFO_ICR_UNDER       (1 << 1)

#define CLR           (0)
#define SET           (1)
#define SET_32BIT     (0xFFFFFFFF)



#define DW_FC_SET(startaddr,reg,a,b,c,d,e,f,g,h,i,j) ({__u32 __v=0;\
__v=(a|b|c|d|e|f|g|h|i|j);\
writel(__v, (startaddr + reg));\
/*printk("%X %X\n", reg, __v);*/ })

#define DW_FC_ADD(startaddr,reg,a,b,c,d,e,f,g,h,i,j) ({__u32 __v=0;\
__v=readl(startaddr + reg);\
__v|=(a|b|c|d|e|f|g|h|i|j);\
writel(__v, (startaddr + reg));\
/*printk("%X %X\n", reg, __v);*/ })


/*========   Exported Const, types and variables definitions    ==========*/

// Base address of controller (e.g. in ARM memory space):
#ifdef DM56_SIM_ON_FPGA
   #define NFC_C_BaseAddrArmView         ((__u32)0x11000000)
   #define NFC_M_ArmToFpgaAddr(ArmAddr)  ((__u32)(ArmAddr) - NFC_C_BaseAddrArmView)
#else  // DM56_SIM_ON_FPGA
   #define NFC_C_BaseAddrArmView         (0)
   #define NFC_M_ArmToFpgaAddr(ArmAddr)  ((__u32)ArmAddr)
#endif // !DM56_SIM_ON_FPGA

#define C_NFlsCntlAddr          0x85404000

typedef struct
{
   char MnfctrCde;
   char DvcId;
   char Mnglss;
   char ExId;
} Y_FlsDevId;


/* Error code returned by Controller's Public Functions */
typedef enum
{
   NFC_K_Err_OK = 0,          // OK
   NFC_K_Err_InvalidChip,     // Invalid Flash chip
   NFC_K_Err_DfctBlk,         // Defected Block
   NFC_K_Err_InDfctBlk,       // Accessed Page is in Defected block
   NFC_K_Err_PageBoundary,    // Page Boundary exceeded
   NFC_K_Err_ProgFail,        // Page programming error
   NFC_K_Err_EraseFail,       // Block erasure error
   NFC_K_Err_TimeOut,         // Operation timed-out
   NFC_K_Err_InvalidParam,    // Invalid parameter
   NFC_K_Err_SwErr,           // NFC_K_Err_SwErr
   NFC_K_Err_EccFailed        // ECC correction failed on read
} NFC_E_Err;


// Supported upto 4 NAND Flash chips
typedef enum
{
   NFC_K_FlsDev0 = 0,
   NFC_K_FlsDev1,
#ifndef DM56_SIM_ON_FPGA
   NFC_K_FlsDev2,
   NFC_K_FlsDev3,
#endif
   NFC_K_FlsDevMAX
} NFC_E_FlsDev;

/* NAND Flash chip type - To be continued */
typedef enum
{
   NFC_K_Chip_None = 0,

   // 8 bit devices
   // -------------
   NFC_K_Chip_SamK9F5608,     // Samsung, 264 Mbit, 32 MByte.
                              // Page:  512+16 Bytes, 1/2 Page access.
                              // Block: 32-Pages, 2048 Blocks. Total:  65,536 Pages.

   NFC_K_Chip_SamK9F1G08,     // Samsung, 1 Gbit,  128 MByte.
                              // Page:  2048+64 Bytes.
                              // Block: 64-Pages, 1024 Blocks. Total:  65,536 Pages.

   NFC_K_Chip_SamK9F2G08,     // Samsung, 2 Gbit,  256 MByte.
                              // Page:  2048+64 Bytes.
                              // Block: 64-Pages, 2048 Blocks. Total: 131,072 Pages.

   NFC_K_Chip_TosTH58NVG3,    // Toshiba, 8 Gbit, 1024 MByte.
                              // Page:  2048+64 Bytes.
                              // Block: 128-Pages, 4096 Blocks. Total: 524,288 Pages.

   NFC_K_Chip_TosTH58DVG02,   // Toshiba, 1 Gbit, 128 MByte.
                              // Page:  512+16 Bytes.
                              // Block: 32-Pages, 8192 Blocks. Total:  262,144 Pages.

   NFC_K_Chip_TosTH58NVG1S3A, // Toshiba, 2 Gbit, 256 MByte.
                              // Page:  2048+64 Bytes.
                              // Block: 64-Pages, 2048 Blocks. Total: 131,072 Pages.

   NFC_K_Chip_Last8bit = NFC_K_Chip_TosTH58NVG1S3A,  // Marks number of last 8bit chip

   // 16 bit devices
   // --------------
   NFC_K_Chip_SamK9F5616,     // Samsung, 264 Mbit, 32 MByte.
                              // Page:  256+8 Words (512+16 Bytes).
                              // Block: 32-Pages, 2048 Blocks. Total:  65,536 Pages.

   NFC_K_Chip_SamK9F1G16,     // Samsung, 1 Gbit,  128 MByte.
                              // Page:  1024+32 Words (2048+64 Bytes).
                              // Block: 64-Pages, 1024 Blocks. Total:  65,536 Pages.

   NFC_K_Chip_SamK9F2G16,     // Samsung, 2 Gbit,  256 MByte.
                              // Page:  1024+32 Words (2048+64 Bytes).
                              // Block: 64-Pages, 2048 Blocks. Total: 131,072 Pages.

   NFC_K_Chip_Last16bit = NFC_K_Chip_SamK9F2G16,  // Marks number of last 16bit chip

   NFC_K_Chip_MAX             // Number of supported chips.

} NFC_E_Chip;


// Bad-blocks list
typedef struct
{
   __u16    NumBadBlks;
   __u16 *  BadBlkArrP;
} Y_BadBlkTbl;

typedef struct
{
   int         ChipValid;
   NFC_E_Chip   ChipType;
   Y_BadBlkTbl  BadBlkTbl;
} Y_FlsDevTbl;

/*
 * Data structures for DW NAND flash controller driver
 */

struct dw_dma_params{
	void __iomem *dma_base_reg;
	void __iomem *fifo_base_reg;
};


/* dw nand info */
struct dw_nand_info {
	/* mtd info */
	//struct nand_hw_control	controller;
	struct mtd_info		*mtd;
	struct nand_chip	*chip;
	struct dw_dma_params    dma_regs;
	void __iomem *nand;
//	NFC_E_Chip nand_chip_id;
	u8 FlsDev;
	u8 to_read;
	/* platform info */
	struct platform_device	*platform;
};


/**************************************************************************
*             P U B L I C    F U N C T I O N S
**************************************************************************/

/**************************************************************************
* Function Name:  NFC_F_Init
*
* Description:    Initializes the Flash controller module.
*                 Identifies the mounted NAND Flash chips and identifies
*                 their bad blocks.
*
* Input:          None.
* Output:         None.
* Return Value:   None.
*
* NOTE: This function must be called once to initialize the driver.
*       Any other function must be call After this initialization.
***************************************************************************/
//extern Y_FlsDevId  NFC_F_Init( void );


/**************************************************************************
* Function Name:  NFC_F_ChipType
*
* Description:    Returns the actual NAND Flash Chip "behind" the Flash Device.
*
* Input:          FlsDev:  Flash Device number.
*
* Output:         None.
*
* Return Value:   The actual chip, by NFC_E_Chip enumaration.
*                 Of failure, NFC_K_Chip_None is returned.
***************************************************************************/
extern NFC_E_Chip  NFC_F_ChipType( NFC_E_FlsDev  FlsDev );


/**************************************************************************
* Function Name:  NFC_F_PageNetSize
*
* Description:    Returns the Physical Page size (Not including the Spare size),
*                 in Bytes.
*
* Input:          FlsDev:  Flash Device number.
*
* Output:         None.
*
* Return Value:   The Physical Page size, in bytes; Not including the Spare size.
*                 In case of failure, 0 is returned.
***************************************************************************/
extern __u16  NFC_F_PageNetSize( NFC_E_FlsDev  FlsDev );


/**************************************************************************
* Function Name:  NFC_F_SpareSize
*
* Description:    Returns Physical Spare size, in Bytes.
*
* Input:          FlsDev:  Flash Device number.
*
* Output:         None.
*
* Return Value:   The Physical Page's Spare size, in bytes.
*                 In case of failure, 0 is returned.
***************************************************************************/
extern __u16  NFC_F_SpareSize( NFC_E_FlsDev  FlsDev );


/**************************************************************************
* Function Name:  NFC_F_BadBlkMarkPos
*
* Description:    Returns the Position of the Bad Block mark in the Spare
*                 Area.
*
*                 The Bad-Block Mark is a Mark that specifies that the Block
*                 is deteriorated and should Not(!) be further Programmed or Erased.
*                 Pages in such a Block should only be Read in order to retrieve there
*                 (probably faulty) data.
*
*                 This Mark(s) appear in the First or Second (or both) Pages of a Block.
*                 For a 8-bit chip, the Mark is one Byte (8bits) long. Any value Other Than
*                 0xFF determined that the Block is Faulty.
*                 For a 16-bit chip, the Mark is one Word (16bits) long. Any value Other Than
*                 0xFFFF determined that the Block is Faulty.
*
*                 The Position of the Mark depends on the chip. It is usually on the 1st or 6th
*                 Byte/Word of the Spare aera.
*                 For Example:
*                     If the Mark is on the 6th Byte on 8bit chip,
*                     then: if (Spare[5] != 0xFF) { Block is Bad }.
*
*                     If the Mark is on the 1st Word on 16bit chip,
*                     then: if (Spare[0] == 0xFFFF) { Block is OK }.
*
*
* Input:          FlsDev:  Flash Device number.
*
* Output:         None.
*
* Return Value:   Position of the Bad-Block Mark Byte/Word in the Spare area.
*                 In case of failure, 0 is returned.
*                 (0 is not a valid Position value; should be 1 or greater).
*
***************************************************************************/
extern __u16  NFC_F_BadBlkMarkPos( NFC_E_FlsDev  FlsDev );


/**************************************************************************
* Function Name:  NFC_F_N1Size
*
* Description:    Returns the (Virtual) Spare-N1 field size, in Bytes.
*
* Input:          FlsDev:  Flash Device number.
*
* Output:         None.
*
* Return Value:   The Virtual(!) Page's Spare size is always 16 bytes (regardless the Physical
*                 Spare size !).
*                 In case of failure, 0 is returned.
*
* NOTE:
*                 When ECC is Active:
*                    Spare-N1 size is a constatnt to be put in the SPARE_N1_COUNT field
*                    of FC_DCOUNT register.
*                    Spare-N2 size is a constatnt to be put in the SPARE_N2_COUNT field
*                    of FC_DCOUNT register.
*
*                    Following equations are in Bytes:
*                    Spare-N2 = 16 - Spare-N1
*                    Spare-N2 = 2[Hamming] + 7[BCH] + X[Non Protected Bytes]
*                    --> X = 16 - Spare-N1 - 9 --> X = 7 - Spare-N1.
*
***************************************************************************/
extern __u16  NFC_F_N1Size( NFC_E_FlsDev  FlsDev );


/**************************************************************************
* Function Name:  NFC_F_BlockSize
*
* Description:    Returns the number of Pages in a Block size.
*
* Input:          FlsDev:  Flash Device number.
*
* Output:         None.
*
* Return Value:   Number of Pages in a Block.
*                 In case of failure, 0 is returned.
***************************************************************************/
extern __u16  NFC_F_BlockSize( NFC_E_FlsDev  FlsDev );


/**************************************************************************
* Function Name:  NFC_F_NumBlocks
*
* Description:    Returns the number of Blocks in the chip.
*
* Input:          FlsDev:  Flash Device number.
*
* Output:         None.
*
* Return Value:   Number of Blocks in the chip..
*                 In case of failure, 0 is returned.
***************************************************************************/
extern __u32  NFC_F_NumBlocks( NFC_E_FlsDev  FlsDev );


/**************************************************************************
* Function Name:  NFC_F_NumTotalPages
*
* Description:    Returns the total(!) number of Pages in the chip
*                 (sum of all pages in all blocks).
*
* Input:          FlsDev:  Flash Device number.
*
* Output:         None.
*
* Return Value:   Total number of Pages in the chip..
*                 In case of failure, 0 is returned.
***************************************************************************/
extern __u32  NFC_F_NumTotalPages( NFC_E_FlsDev  FlsDev );


/**************************************************************************
* Function Name:  NFC_F_GetBadBlocks
*
* Description:    Returns a list of Bad Blocks in the Flash device.
*
* Input:          FlsDev:  Flash Device number.
*
* Output:         NumBadBlocksP: Returns the number of Bad Blocks.
*                 BadBlockListP: Returns a list of Bad Blocks.
*
* Return Value:   NFC_K_Err_OK:            OK - List of Bad blocks was built.
*                 NFC_K_Err_InvalidParam:  Invalid parameter.
*                 NFC_K_Err_InvalidChip:   Invalid chip.
*
* NOTE: User supplied BadBlockListP must point to a big enough allocated
*       memory area - The maximum number of allowed Bad Blocks per chip type
*       (at least 64 Bytes !)
*
***************************************************************************/
extern NFC_E_Err  NFC_F_GetBadBlocks( NFC_E_FlsDev  FlsDev,
                                      __u16 *      NumBadBlocksP,
                                      __u16 *      BadBlockListP );


/**************************************************************************
* Function Name:  NFC_F_GetFlsStatus
*
* Description:    Gets the current Flash chip status.
*
* Input:          FlsDev:  Flash Device number.
*
* Output:         None.
*
* Return Value:   true:  Device is Ready.
*                 false: Device is Busy or Error.
***************************************************************************/
extern int  NFC_F_GetFlsStatus( NFC_E_FlsDev  FlsDev );


/**************************************************************************
* Function Name:  NFC_F_EraseBlk
*
* Description:    Erases a NAND Flash Block.
*                 If the Block is Bad (contains faulty Pages), the actual
*                 erasure is skipped.
*                 If erasure fails, the Block is added to the chip's bad-blocks
*                 list.
*
* Input:          FlsDev:     Flash Device number.
*                 Block:      Block number.
*
* Output:         None.
*
* Return Value:   NFC_K_Err_OK:            Erase OK.
*                 NFC_K_Err_InvalidChip:   Invalid chip.
*                 NFC_K_Err_EraseFail:     Erase failed.
*                 NFC_K_Err_DfctBlk:       Defected Block.
*                 NFC_K_Err_InvalidParam:  Invalid Parameter.
*                 NFC_K_Err_SwErr:         Software error.
***************************************************************************/
extern NFC_E_Err  NFC_F_EraseBlk( NFC_E_FlsDev  FlsDev,
                                  __u16        Block );


/**************************************************************************
* Function Name:  NFC_F_ProgPage
*
* Description:    Programs a whole Physical(!) Page, including its Spare area.
*                 If the Page belongs to a Bad Block, the actual programming is skipped.
*                 If programming fails, the Block is added to the chip's bad-blocks
*                 list.
*
* Input:          FlsDev:     Flash Device number.
*                 Block:      Block number.
*                 PageInBlk:  Page offset within the block.
*                 N1Len:      Ecc N1 field length.
*                 IsEccF:     Whether to activate ECC machine.
*                 BypassDmaF: Whether to Bypass DMA&FIFO.
*                 DataP:      User's buffer holding the net-Page Data (w/o the spare data).
*                 SpareP:     User's buffer holding the Spare Data.
*
* Output:         None.
*
* Return Value:   NFC_K_Err_OK:            Erase OK.
*                 NFC_K_Err_InvalidChip:   Invalid chip.
*                 NFC_K_Err_ProgFail:      Programming failed.
*                 NFC_K_Err_DfctBlk:       Defected Block.
*                 NFC_K_Err_InvalidParam:  Invalid Parameter.
*                 NFC_K_Err_SwErr:         Software error.
*
* NOTE:
*  1. Under FPGA emulation of the DM56 And DMA is Used (BypassDmaF is false),
*     Page (DataP) and Spare (SpareP) buffers MUST be allocated within the FPGA RAM,
*     due to the DMA transfer limitation (not transferring over the AHB bus, just inside
*     the FPGA).
***************************************************************************/
extern NFC_E_Err  NFC_F_ProgPage( NFC_E_FlsDev  FlsDev,
                                  __u16        Block,
                                  __u8         PageInBlk,
                                  __u8         N1Len,
                                  int          IsEccF,
                                  int          BypassDmaF,
                                  void *        DataP,
                                  void *        SpareP );


/**************************************************************************
* Function Name:  NFC_F_ReadPage
*
* Description:    Reads a whole Physical(!) Page, including its Spare area.
*                 Reading is allowed even when the Page belongs to a bad Block.
*
* Input:          FlsDev:     Flash Device number.
*                 Block:      Block number.
*                 PageInBlk:  Page offset within the block.
*                 N1Len:      Ecc N1 field length.
*                 IsEccF:     Whether to activate ECC machine.
*                 BypassDmaF: Whether to Bypass DMA&FIFO.
*
* Output:         DataP:      User's buffer to hold the returned net-Page Data (w/o the spare data).
*                 SpareP:     User's buffer to hold the returned page's Spare Data.
*
* Return Value:   NFC_K_Err_OK:            Erase OK.
*                 NFC_K_Err_InvalidChip:   Invalid chip.
*                 NFC_K_Err_TimeOut:       Read Timeout.
*                 NFC_K_Err_InvalidParam:  Invalid Parameter.
*                 NFC_K_Err_SwErr:         Software error.
*
* NOTE:
*  1. Under FPGA emulation of the DM56 And DMA is Used (BypassDmaF is false),
*     Page (DataP) and Spare (SpareP) buffers MUST be allocated within the FPGA RAM,
*     due to the DMA transfer limitation (not transferring over the AHB bus, just inside
*     the FPGA).
***************************************************************************/
extern NFC_E_Err  NFC_F_ReadPage( NFC_E_FlsDev     FlsDev,
                                  __u16           Block,
                                  __u8            PageInBlk,
                                  __u8            N1Len,
                                  int             IsEccF,
                                  int             BypassDmaF,
                                  void *           DataP,
                                  void *           SpareP );



/* interface to the linux kernel */


NFC_E_Err  F_CmdReadOOBBypass( struct dw_nand_info *nand_info,
                                          __u16        Block,
                                          __u16        Column,
                                          __u8         PageInBlk,
                                          __u16        Len,
                                          void *       DataP );


NFC_E_Err  F_CmdWriteOOBBypass( struct dw_nand_info *nand_info,
                                          __u16        Block,
                                          __u8         PageInBlk,
                                          __u16        Len,
                                          void *       SpareP );
void 	F_ReadByPass(struct dw_nand_info *nand_info, uint8_t *buf, int len);

void F_Init1(struct dw_nand_info *nand_info);

NFC_E_Err dw_nand_repare_readpage(struct dw_nand_info *nand_info,
                                               __u16        Block,
                                               __u16        DataLen,
                                               __u16        SpareLen,
                                               __u8         PageInBlk,
                                               __u8         N1Len,
                                               int          IsEccF);

NFC_E_Err  dw_nand_CmdWritePageAndSpareBypass( struct dw_nand_info *nand_info,
                                                __u16        Block,
                                                __u16        DataLen,
                                                __u16        SpareLen,
                                                __u8         PageInBlk,
                                                __u8         N1Len,
                                                int          IsEccF);

#if 0
NFC_E_Err  dw_nand_ReadPageAndSpareBypass(     struct dw_nand_info *nand_info,
                                               __u16        Block,
                                               __u16        DataLen,
                                               __u16        SpareLen,
                                               __u16        Column,
                                               __u8         PageInBlk,
                                               __u8         N1Len,
                                               int          IsEccF,
                                               void *        DataP,
                                               void *        SpareP );

#endif 
NFC_E_Err  dw_nand_CompleteWritePageAndSpare( struct dw_nand_info *nand_info );


NFC_E_Err  dw_nand_prepareWritePageAndSpare( struct dw_nand_info *nand_info,
                                          __u16        Block,
                                          __u16        DataLen,
                                          __u16        SpareLen,
                                          __u8         PageInBlk,
                                          __u8         N1Len,
					  void *       DataP,
					  void *       SpareP,
                                          int          IsEccF);

void dw_nand_cntl_reset(struct dw_nand_info *nand_info);
void dw_nand_cntl_readid(struct dw_nand_info *nand_info);
void dw_nand_cntl_status(struct dw_nand_info *nand_info);

NFC_E_Err  F_CmdEraseBlkBypass( struct dw_nand_info *nand_info,
                                       __u16        Block );

NFC_E_Err  F_CmdReadPageAndSpare( struct dw_nand_info *nand_info,
                                         __u16        Block,
                                         __u16        DataLen,
                                         __u16        SpareLen,
                                         __u8         PageInBlk,
                                         __u8         N1Len,
                                         int          IsEccF,
                                         void *        DataP_DMA,
                                         void *        SpareP_DMA,
                                         void *        DataP_Virt,
                                         void *        SpareP_Virt );
#endif /* _NAND_FLASH_CONTROLLER_PUB_H_ */

/*============================   End of File   ==========================*/
