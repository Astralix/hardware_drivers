/********************************************************************************
*              N A N D   F L A S H   C O N T R O L L E R
*********************************************************************************
*
*   Title:           Low-Level NAND-Flash controller implementation.
*
*   Filename:        $Workfile: $
*   SubSystem:
*   Authors:         Avi Miller
*   Latest update:   $Modtime: $
*   Created:         16 Mars, 2005
*
*********************************************************************************
*  Description:      Low-Level NAND-Flash controller.
*                    Running on ARM 7 in the DM-56 chip->
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


/*=======   Include Files  ===============================================*/

#include "dw_nfls_cntl_pub.h"

#include "dw_polynom.h"
#include "dw_dma_cntl.h"
#include "dw_nfls_cntl.h"

extern struct semaphore unified_dw_nflc_mutex;

Y_FlsDevTbl V_FlsDevTbl[NFC_K_FlsDevMAX];

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

		#if defined (CONFIG_MTD_NAND_DW_DEBUG_VERBOSE_FUNCTIONS)
			#define DW_SPOT_FUN DW_SPOT
		#else
			#define DW_SPOT_FUN(args...) do {} while (0)
		#endif

#else
        #define DW_SPOT(args...)        \
                do {} while (0)
		#define DW_SPOT_FUN DW_SPOT
#endif



//#define DW_PROFILE_NFLC
#ifdef  DW_PROFILE_NFLC

#define DW_PROFILE_NFLC_MAXPIDS 20
#define DW_PROFILE_NFLC_BINS 50
static int dw_nflc_pids[DW_PROFILE_NFLC_MAXPIDS] = {0};
static char *dw_nflc_pidnames[DW_PROFILE_NFLC_MAXPIDS] = {(char *)0};
static int dw_nflc_times[DW_PROFILE_NFLC_MAXPIDS][DW_PROFILE_NFLC_BINS] = {{0}};
struct timespec from1;
struct timespec from2;
struct timespec to1;
struct timespec to2;
void g1(void) {
	getnstimeofday(&from1);
}
void g2(void) {
	getnstimeofday(&to1);
}
void g3(void) {
	getnstimeofday(&from2);
}
void g4(void) {
	getnstimeofday(&to2);
}

void print_g(void) {
//	printk("F_CmdReadPageAndSpare before down(mutex) " "time in ns: %15lld\n", timespec_to_ns(&from1));
//	printk("F_CmdReadPageAndSpare  after down(mutex) " "time in ns: %15lld\n", timespec_to_ns(&to1));
//	printk("F_CmdReadPageAndSpare before   up(mutex) " "time in ns: %15lld\n", timespec_to_ns(&from2));
//	printk("F_CmdReadPageAndSpare  after   up(mutex) " "time in ns: %15lld\n", timespec_to_ns(&to2));
	printk("\n");
}

#else

#define g1() do{} while (0)
#define g2() do{} while (0)
#define g3() do{} while (0)
#define g4() do{} while (0)
#define print_g() do{} while (0)

#endif

// 11 Nov, 2008: "ReadOnce" command, as part of a memory transaction sequence, is still not operable in the current version of DW52.
// The macro USE_ReadOnce should be enabled once hw bug is fixed.
//#define USE_ReadOnce

#ifndef USE_ReadOnce

	#define M_SetUpReadStatus(CmdEn, Cmd, WaitEn, ReadOnce) \
		do { 					\
			CmdEn = 0;			\
			Cmd = 0; 			\
			WaitEn = 0; 		\
			ReadOnce = 0; 		\
		} while (0)

	#define M_ReadStatus(FlsDev) \
		F_CmdReadDeviceStatDirectBypass(FlsDev)

#else

	#define M_ReadStatus(FlsDev) \
		((__u16)((readl(DW_NAND_LCD_BASE  + DW_FC_FBYP_DATA) & DW_FC_FBYP_DATA_FLASH_DATA) >> 16))

	#define M_SetUpReadStatus(CmdEn, Cmd, WaitEn, ReadOnce) \
		do { 										\
			CmdEn = DW_FC_SEQ_CMD4_EN;				\
			Cmd = C_FlsCmdReadStat; 				\
			WaitEn = DW_FC_SEQ_WAIT4_EN; 			\
			ReadOnce = DW_FC_SEQ_READ_ONCE; 		\
		} while (0)

#endif


// Flash commands
static __u8  V_FlsCmnd1;
static __u8  V_FlsCmnd2;
static __u8  V_FlsCmnd3;
static __u8  V_FlsCmnd4;

// Flash command cycles
static __u8  V_FlsCycle1;
static __u8  V_FlsCycle2;
static __u8  V_FlsCycle3;
static __u8  V_FlsCycle4;
static __u8  V_FlsCycle5;

// Error-Correction Polynoms
static __u16  V_S1;
static __u16  V_S2;
static __u16  V_S3;
static __u16  V_S4;
static __u16  V_S5;
static __u16  V_S7;

static __u16  V_T1;
static __u16  V_T2;
static __u16  V_T3;
static __u16  V_T4;
static __u16  V_T5;
static __u16  V_T6;
static __u16  V_T7;

static __u16  V_L1;
static __u16  V_L2;
static __u16  V_L3;
static __u16  V_L4;
static __u16  V_D;

static const Y_BchChainParms  V_BchChainParms[ C_MaxBchErrors ] =
{
   // /* Spare_N1_Count */  Chain_Count_Start,   K1,     K2,     K3,     K4

      /* 0 */               {   4164,         0x19C4, 0x1EBF, 0x033D, 0x05C2  },
      /* 2 */               {   4180,         0x0B4C, 0x05F3, 0x12CE, 0x186B  },
      /* 4 */               {   4196,         0x1937, 0x0B8C, 0x1298, 0x15C5  },
      /* 6 */               {   4212,         0x13D4, 0x0AC4, 0x01AB, 0x055D  }
};


enum dwnandcmd{
	DW_NAND_CMD_READ,
	DW_NAND_CMD_WRITE,
	DW_NAND_CMD_ERASE,
	DW_NAND_CMD_READ_OOB,
	DW_NAND_CMD_MAX,

};
/*========   Local function prototypes   =================================*/

// General utilities:
// ------------------

/*static*/ void  inline F_SetFixedCntrlRegs( void *nand_base_reg );

/*static*/ NFC_E_Chip  F_MatchChipByDevId( __u8  MnfctrCode, __u8  DeviceId );

/*static*/ void  F_SetDataCount( void * nand_base_reg, __u16  PageSize, __u16  SpareSize, int  IsEccF, __u8  N1Len );

// Flash access routines, in ByPass mode:
// --------------------------------------

/*static*/ void  F_CmdReadDeviceIdBypass( struct dw_nand_info *nand_info, Y_FlsDevId *FlsId);

/*static*/ unsigned  F_CmdReadDeviceStatDirectBypass( struct dw_nand_info *nand_info );
// Error Correction utilities:
// ---------------------------

/*static*/ 
void  F_FixEccErrors( struct dw_nand_info *nand_info,
                             void *      DataP,
                             void *      SpareP );

/*static*/ 
void  F_HammCorrect( void * nand_base_reg, __u8   VirtPage,
                            __u8   N1Size,
                            void *  DataP,
                            void *  SpareP );

/*static*/ 
int  F_IsSingleBitBchErr( void* nand_base_reg, __u8  VirtPage );

/*static*/ 
__u8  F_BchDetect( void * nand_base_reg );

/*static*/ 
void  F_BchCorrect( struct dw_nand_info *nand_info,
			   __u8   NumBchErrsExpctd,
                           __u8   VirtPage,
                           __u8   N1Size,
                           void *  DataP,
                           void *  SpareP );

/*static*/ 
void  F_CorrectSingleByte( __u8 *  BadByteP, __u16  BadBitLoc );


/*static*/ 
__u16  F_GfMultPolynom(void * nand_base_reg, __u16  P1, __u16  P2);

/*static*/ 
__u16  F_ReciprocalPolynom( __u16  Poly );

// Debugging functions:
// --------------------
/*static*/ 
void  F_DebugDelay( __u32  NumCycles );

void set_addr_cycles(struct dw_nand_info *nand_info, unsigned int block, unsigned int page_in_blk, unsigned int column, enum dwnandcmd cmd)

{


	int page_in_blk_bits = 0;
	int block_bits = 0;
	int offset_for_page_in_blk = 1;
	int page_in_blk_mask;
	int block_bits_offset = 0;

	V_FlsCycle1 = column & 0xff;
	V_FlsCycle2 = (column >> 8) & 0xff;
	V_FlsCycle3 = 0;
	V_FlsCycle4 = 0;
	V_FlsCycle5 = 0;

	V_FlsCmnd1 = 0;
	V_FlsCmnd2 = 0;
	V_FlsCmnd3 = 0;
	V_FlsCmnd4 = 0;

	if (nand_info->mtd->writesize > 512) 
		offset_for_page_in_blk = 2;

	DW_FC_SET(nand_info->nand, DW_FC_SEQUENCE,
	          DW_FC_SEQ_CMD1_EN, DW_FC_SEQ_ADD3_EN,
	          DW_FC_SEQ_CHIP_SEL(nand_info->FlsDev), DW_FC_SEQ_RDY_EN,
	          DW_FC_SEQ_RDY_SEL(nand_info->FlsDev >> 1),
	          0,0,0,0,0);

	if ((cmd == DW_NAND_CMD_READ) || (cmd == DW_NAND_CMD_READ_OOB) ||
	    (cmd == DW_NAND_CMD_WRITE))
		DW_FC_ADD(nand_info->nand, DW_FC_SEQUENCE,
             DW_FC_SEQ_ADD1_EN, DW_FC_SEQ_ADD2_EN, 
             0,0,0,0,0,0,0,0);


	if (nand_info->chip->options & NAND_BUSWIDTH_16) {
		DW_FC_ADD(nand_info->nand, DW_FC_SEQUENCE,
                          DW_FC_SEQ_MODE16,
		          0,0,0,0,0,0,0,0,0);
	}

	if ((cmd == DW_NAND_CMD_READ) || (cmd == DW_NAND_CMD_READ_OOB)) {
		if (offset_for_page_in_blk > 1) {
			V_FlsCmnd1 = C_SamK9F2G_FlsCmdRead1Cyc;
			V_FlsCmnd2 = C_SamK9F2G_FlsCmdRead2Cyc;
			DW_FC_ADD(nand_info->nand, DW_FC_SEQUENCE,
			          DW_FC_SEQ_CMD2_EN, DW_FC_SEQ_WAIT2_EN,
			          0,0,0,0,0,0,0,0);
		} else {
			if(cmd == DW_NAND_CMD_READ_OOB)
				V_FlsCmnd1 = C_SamK9F56_FlsCmdReadSpare;
			else
				V_FlsCmnd1 = C_SamK9F56_FlsCmdRead;
		}
		DW_FC_ADD(nand_info->nand, DW_FC_SEQUENCE,
		          DW_FC_SEQ_RW_EN, DW_FC_SEQ_DATA_READ_DIR,
		          DW_FC_SEQ_WAIT1_EN,
		          0,0,0,0,0,0,0);
	} else if(cmd == DW_NAND_CMD_WRITE) {
		V_FlsCmnd1 = C_FlsCmdPageProg1Cyc; // Program Setup
		V_FlsCmnd3 = C_FlsCmdPageProg2Cyc; // Program
		DW_FC_ADD(nand_info->nand, DW_FC_SEQUENCE,
		          DW_FC_SEQ_RW_EN, DW_FC_SEQ_DATA_WRITE_DIR,
		          0,0,0,0,0,0,0,0);
		DW_FC_ADD(nand_info->nand, DW_FC_SEQUENCE,
		          DW_FC_SEQ_CMD3_EN, DW_FC_SEQ_WAIT3_EN, DW_FC_SEQ_ADD4_EN,
		          0,0,0,0,0,0,0);
	} else if(cmd == DW_NAND_CMD_ERASE){
		V_FlsCmnd1 = C_FlsCmdBlkErase1Cyc;
		V_FlsCmnd2 = C_FlsCmdBlkErase2Cyc;
		DW_FC_ADD(nand_info->nand, DW_FC_SEQUENCE,
		          DW_FC_SEQ_CMD2_EN, DW_FC_SEQ_DATA_READ_DIR,
		          DW_FC_SEQ_WAIT2_EN,0,0,0,0,0,0,0);


	}
	page_in_blk_bits = nand_info->chip->phys_erase_shift - nand_info->chip->page_shift;
	page_in_blk_mask = (1 << page_in_blk_bits) - 1;
	block_bits = nand_info->chip->chip_shift - nand_info->chip->phys_erase_shift;

	if (offset_for_page_in_blk == 1) {
		V_FlsCycle2 = page_in_blk & page_in_blk_mask;
		V_FlsCycle2 |= (block & ((1 << (8 - page_in_blk_bits))-1)) << page_in_blk_bits;
		block_bits_offset = 8 - page_in_blk_bits;

		DW_FC_ADD(nand_info->nand, DW_FC_SEQUENCE,
		          DW_FC_SEQ_ADD2_EN,
		          0,0,0,0,0,0,0,0,0);

		if (block_bits_offset >= block_bits)
			return;
		
		V_FlsCycle3 = (block >> block_bits_offset) & 0xff;
		block_bits_offset += 8;

		DW_FC_ADD(nand_info->nand, DW_FC_SEQUENCE,
		          DW_FC_SEQ_ADD3_EN,
		          0,0,0,0,0,0,0,0,0);

		if (block_bits_offset >= block_bits)
			return;

		V_FlsCycle4 = (block >> block_bits_offset) & 0xff;

		DW_FC_ADD(nand_info->nand, DW_FC_SEQUENCE,
		          DW_FC_SEQ_ADD4_EN,
		          0,0,0,0,0,0,0,0,0);
            } else {
			V_FlsCycle3 = page_in_blk & page_in_blk_mask;
			V_FlsCycle3 |= (block & ((1 << (8 - page_in_blk_bits))-1)) << page_in_blk_bits;
			block_bits_offset = 8 - page_in_blk_bits;

			if (block_bits_offset >= block_bits)
				return;

			V_FlsCycle4 = (block >> block_bits_offset) & 0xff;
			block_bits_offset += 8;
		DW_FC_ADD(nand_info->nand, DW_FC_SEQUENCE,
		          DW_FC_SEQ_ADD4_EN,
		          0,0,0,0,0,0,0,0,0);

		if (block_bits_offset >= block_bits)
			return;

		V_FlsCycle5 = (block >> block_bits_offset) & 0xff;
		DW_FC_ADD(nand_info->nand, DW_FC_SEQUENCE,
		          DW_FC_SEQ_ADD5_EN,
		          0,0,0,0,0,0,0,0,0);
            }
}



/**************************************************************************
*              P U B L I C    F U N C T I O N S
**************************************************************************/

/**************************************************************************
* Function Name:  NFC_F_Init1
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

void F_Init1(struct dw_nand_info *nand_info)
{
	down(&unified_dw_nflc_mutex);
	/*! CLEAR controller Status bits */
	writel(SET_32BIT, (nand_info->nand + DW_FC_STS_CLR));

	/*! CONFIGURE one-time-set controller registers: */
	F_SetFixedCntrlRegs(nand_info->nand) ;
	up(&unified_dw_nflc_mutex);

} /* end F_Init1() */

/**************************************************************************
* Function Name:  F_SetFixedCntrlRegs
*
* Description:    Presets some Flash-Controller registers, that have
*                 to be set once only (good for all transactions on
*                 all Flash chip types.
*
* Input:          None.
* Output:         None.
* Return Value:   None.
***************************************************************************/
/*static*/ void inline F_SetFixedCntrlRegs( void *nand_base_reg )
{
   // WaitTime register
   DW_FC_SET(nand_base_reg, DW_FC_WAIT,
             DW_FC_WAIT1A(C_FlsWaitRdy1Fall),DW_FC_WAIT1B(C_FlsWaitRdy1Rise),
             DW_FC_WAIT2A(C_FlsWaitRdy2Fall),DW_FC_WAIT2B(C_FlsWaitRdy2Rise),
             DW_FC_WAIT3A(C_FlsWaitRdy3Fall),DW_FC_WAIT3B(C_FlsWaitRdy3Rise),
             DW_FC_WAIT4A(C_FlsWaitRdy4Fall),DW_FC_WAIT4B(C_FlsWaitRdy4Rise),0,0);

   // PulseTime register
   DW_FC_SET(nand_base_reg, DW_FC_PULSETIME,
             DW_FC_PT_READ_LOW(C_FlsPulseRdLow),DW_FC_PT_READ_HIGH(C_FlsPulseRdHigh),
             DW_FC_PT_WRITE_LOW(C_FlsPulseWrLow),DW_FC_PT_WRITE_HIGH(C_FlsPulseWrHigh),
             DW_FC_CLE_SETUP(C_FlsPulseCleStup),0,0,0,0,0);

   // Disable all FC interrupts
   writel(CLR, (nand_base_reg + DW_FC_INT_EN));

} /* end F_SetFixedCntrlRegs() */


/* 
    The following function is used to "translate" virtual addresses into physical ones.
    It is currently only capable to translate 
		INTERNAL_SRAM_VIRT_ADDRESS ==> DW_SSRAM_BASE
    and all the adresses within 16kiB.
*/

void dw_nand_select_chip(struct mtd_info *mtd, int chipnr)
{
}

void dw_nand_cntl_reset(struct dw_nand_info *nand_info)
{
	/*! DISABLE ByPass mode */
	writel(!DW_FC_FBYP_CTL_BP_EN, (nand_info->nand + DW_FC_FBYP_CTL));
	/*! DISABLE DMA & FIFO */
	DMF_F_ConfDmaBypass(&nand_info->dma_regs, DMF_K_PeriphFlsCntl);

	/*! PREPARE Manufacture & Device Id read, using 8 width */
	// Read DeviceId command
	V_FlsCmnd1 = C_FlsCmdReset;
	V_FlsCycle1 = 0x00;

	// Clear relevant bits in Status register
	writel(SET_32BIT, (nand_info->nand + DW_FC_STS_CLR));
	// Disable all FC interrupts - To be changed
	/* NOTE: This register is configured by F_Init() */
	// ID read sequence
	// Don't Monitor Ready line (not activated by the Flash chip in this command),
	// don't keep CS for next transaction, Disable ECC, enable R/W in Read direction
	DW_FC_SET(nand_info->nand, DW_FC_SEQUENCE,
	          DW_FC_SEQ_CMD1_EN/*for Reset command*/,
	          0 /*no addr*/,
	          0, 0/*no data*/,
	          DW_FC_SEQ_CHIP_SEL(0), //tmpFlsDev),
	          DW_FC_SEQ_MODE8/*8 bits interface*/,
	          0,0,0,0);

	// Configure Address registers
	//V_FC_RegsP->FC_AddrCol.Addr1 = V_FlsCycle1;
	writel(V_FlsCycle1, (nand_info->nand + DW_FC_ADDR_COL));
	// Configure Command Code register
	//V_FC_RegsP->FC_Cmd.Cmd1 = V_FlsCmnd1;
	writel(V_FlsCmnd1, (nand_info->nand + DW_FC_CMD));
	// Configure DataCount register
	nand_info->to_read=0;
	writel(0x0, (nand_info->nand + DW_FC_DCOUNT));
	// Configure Ready-Timeout register
	writel(CLR, (nand_info->nand + DW_FC_TIMEOUT));
	/*! SET Bypass enabled */
	writel(DW_FC_FBYP_CTL_BP_EN, (nand_info->nand + DW_FC_FBYP_CTL));

	writel(SET_32BIT, (nand_info->nand + DW_FC_STS_CLR));

	/*! TRIGGER Flash controller */
	writel(DW_FC_CTL_ECC_OP_MODE(DW_FC_MEM_TRANS), (nand_info->nand + DW_FC_CTL));

	/* wait for ready bit rise */
	while ((readl(nand_info->nand + DW_FC_STATUS) & 0x10) == 0) {
	}
}

void dw_nand_flush(struct dw_nand_info *nand_info)
{
	if (! (readl(nand_info->nand + DW_FC_FBYP_CTL) & DW_FC_FBYP_CTL_BP_EN))
		return;

	if(nand_info->to_read >0){
		unsigned char buf[nand_info->to_read];
		F_ReadByPass(nand_info,buf,nand_info->to_read);
	//	printk("NAND flushed 0x%x\n", nand_info->to_read);
	
	}
	/*! DISABLE ByPass mode */
	writel(!DW_FC_FBYP_CTL_BP_EN, (nand_info->nand + DW_FC_FBYP_CTL));

}

void dw_nand_cntl_readid(struct dw_nand_info *nand_info)
{
     	/*! DISABLE ByPass mode */
	writel(!DW_FC_FBYP_CTL_BP_EN, (nand_info->nand + DW_FC_FBYP_CTL));
	/*! DISABLE DMA & FIFO */
	DMF_F_ConfDmaBypass(&nand_info->dma_regs, DMF_K_PeriphFlsCntl);
	/*! PREPARE Manufacture & Device Id read, using 8 width */
	// Read DeviceId command
	V_FlsCmnd1 = C_FlsCmdReadId;
	V_FlsCycle1 = 0x00;

	// Clear relevant bits in Status register
	writel(SET_32BIT, (nand_info->nand + DW_FC_STS_CLR));
	// Disable all FC interrupts - To be changed
	/* NOTE: This register is configured by F_Init() */
	// ID read sequence
	// Don't Monitor Ready line (not activated by the Flash chip in this command),
	// don't keep CS for next transaction, Disable ECC, enable R/W in Read direction
	DW_FC_SET(nand_info->nand, DW_FC_SEQUENCE,
	          DW_FC_SEQ_CMD1_EN/*for Read Device Id command*/,
	          DW_FC_SEQ_ADD1_EN/*for 0x00 addr*/,
	          DW_FC_SEQ_RW_EN, DW_FC_SEQ_DATA_READ_DIR/*read*/,
	          DW_FC_SEQ_CHIP_SEL(0), //tmpFlsDev),
	          DW_FC_SEQ_MODE8/*8 bits interface*/,
	          0,0,0,0);

	// Configure Address registers
	//V_FC_RegsP->FC_AddrCol.Addr1 = V_FlsCycle1;
	writel(V_FlsCycle1, (nand_info->nand + DW_FC_ADDR_COL));
	// Configure Command Code register
	//V_FC_RegsP->FC_Cmd.Cmd1 = V_FlsCmnd1;
	writel(V_FlsCmnd1, (nand_info->nand + DW_FC_CMD));
	// Configure DataCount register
//	printk("read id to_read=4\n");
	nand_info->to_read=4;
	writel(0x4, (nand_info->nand + DW_FC_DCOUNT));
	// Configure Ready-Timeout register
	writel(CLR, (nand_info->nand + DW_FC_TIMEOUT));
	/*! SET Bypass enabled */
	writel(DW_FC_FBYP_CTL_BP_EN, (nand_info->nand + DW_FC_FBYP_CTL));

	writel(SET_32BIT, (nand_info->nand + DW_FC_STS_CLR));

	/*! TRIGGER Flash controller */
	writel(DW_FC_CTL_ECC_OP_MODE(DW_FC_MEM_TRANS), (nand_info->nand + DW_FC_CTL));

}

void dw_nand_cntl_status(struct dw_nand_info *nand_info)
{
	/*! DISABLE ByPass mode */
	writel(!DW_FC_FBYP_CTL_BP_EN, (nand_info->nand + DW_FC_FBYP_CTL));

	F_SetFixedCntrlRegs(nand_info->nand) ;

	/*! DISABLE DMA & FIFO */
	DMF_F_ConfDmaBypass(&nand_info->dma_regs, DMF_K_PeriphFlsCntl);

	/*! PREPARE Device-Status Read, using 16bit width */
	// Read Device Status command
	V_FlsCmnd1 = C_FlsCmdReadStat;

	// Clear relevant bits in Status register
	writel(SET_32BIT, (nand_info->nand + DW_FC_STS_CLR));

	// STATUS read sequence
	// Don't Monitor Ready line (not activated by the Flash chip in this command),
	// don't keep CS for next transaction, Disable ECC, enable R/W in Read direction
	//
	// We have to make 1 transaction on the bus. The only way to do it is read 2 bytes in __16_bits__ mode.
	// However we are urged to use 8 bit mode due to resrictions on the pinout.
	DW_FC_SET(nand_info->nand, DW_FC_SEQUENCE,
	          DW_FC_SEQ_CMD1_EN/*for Read Status command*/,
	          DW_FC_SEQ_WAIT1_EN,
	          DW_FC_SEQ_RW_EN, DW_FC_SEQ_DATA_READ_DIR/*read*/,
	          DW_FC_SEQ_CHIP_SEL(0), DW_FC_SEQ_RDY_EN, // tmpFlsDev
	          DW_FC_SEQ_MODE8/*8 bits interface*/, 
	          0,0,0);

	// Configure Command Code register
	writel(V_FlsCmnd1, (nand_info->nand + DW_FC_CMD));
	// Configure DataCount register
	writel(0x2, (nand_info->nand + DW_FC_DCOUNT));
//	printk("readstatus to_read=2\n");
	nand_info->to_read = 2;
	// Configure Ready-Timeout register
	writel(CLR, (nand_info->nand + DW_FC_TIMEOUT));

	/*! SET Bypass enabled */
	writel(DW_FC_FBYP_CTL_BP_EN, (nand_info->nand + DW_FC_FBYP_CTL));

	/*! TRIGGER Flash controller */
	writel(SET_32BIT, (nand_info->nand + DW_FC_STS_CLR));
	//NFLC_CAPTURE();
	/*! TRIGGER Flash controller */
	writel(DW_FC_CTL_ECC_OP_MODE(DW_FC_MEM_TRANS), (nand_info->nand + DW_FC_CTL));


}


/**************************************************************************
* Function Name:  F_CmdReadDeviceIdBypass
*
* Description:    Reads the Flash Manufacture Code and Device ID,
*                 bypassing the DMA & Fifo.
*
* Input:          FlsDev: Flash Device number.
*
* Output:         MnfcrCodeP: Pointer to returned Manufacture Code
*                 DeviceIdP:  Pointer to returned Device ID
*
* Return Value:   None.
***************************************************************************/
/*static*/ void F_CmdReadDeviceIdBypass(struct dw_nand_info *nand_info, Y_FlsDevId *FlsId)
{
   u8 FlsDev = nand_info->FlsDev;
   /*! DISABLE DMA & FIFO */
   DMF_F_ConfDmaBypass(&nand_info->dma_regs, DMF_K_PeriphFlsCntl);

   /*! PREPARE Manufacture & Device Id read, using 8 width */
   // Read DeviceId command
   V_FlsCmnd1 = C_FlsCmdReadId;
   V_FlsCycle1 = 0x00;

   // Clear relevant bits in Status register
   writel(SET_32BIT, (nand_info->nand + DW_FC_STS_CLR));

   // Disable all FC interrupts - To be changed
   /* NOTE: This register is configured by F_Init() */

   // ID read sequence
   // Don't Monitor Ready line (not activated by the Flash chip in this command),
   // don't keep CS for next transaction, Disable ECC, enable R/W in Read direction
   DW_FC_SET(nand_info->nand, DW_FC_SEQUENCE,
             DW_FC_SEQ_CMD1_EN/*for Read Device Id command*/,
             DW_FC_SEQ_ADD1_EN/*for 0x00 addr*/,
             DW_FC_SEQ_RW_EN, DW_FC_SEQ_DATA_READ_DIR/*read*/,
             DW_FC_SEQ_CHIP_SEL(FlsDev),
             DW_FC_SEQ_MODE8/*8 bits interface*/,
             0,0,0,0);

   // Configure Address registers
   //V_FC_RegsP->FC_AddrCol.Addr1 = V_FlsCycle1;
   writel(V_FlsCycle1, (nand_info->nand + DW_FC_ADDR_COL));

   // Configure Command Code register
   //V_FC_RegsP->FC_Cmd.Cmd1 = V_FlsCmnd1;
   writel(V_FlsCmnd1, (nand_info->nand + DW_FC_CMD));

   // Configure DataCount register
   writel(0x4, (nand_info->nand + DW_FC_DCOUNT));

   // Configure Ready-Timeout register
   writel(CLR, (nand_info->nand + DW_FC_TIMEOUT));

   /*! SET Bypass enabled */
   writel(DW_FC_FBYP_CTL_BP_EN, (nand_info->nand + DW_FC_FBYP_CTL));

   writel(SET_32BIT, (nand_info->nand + DW_FC_STS_CLR));
NFLC_CAPTURE();
  /*! TRIGGER Flash controller */
   writel(DW_FC_CTL_ECC_OP_MODE(DW_FC_MEM_TRANS), (nand_info->nand + DW_FC_CTL));

   /*! READ Flash Data via Bypass */
   writel(0x12, (nand_info->nand + DW_FC_FBYP_CTL));
   while((readl(nand_info->nand + DW_FC_FBYP_CTL) & DW_FC_FBYP_CTL_BUSY) != 0);
   FlsId->MnfctrCde= (__u8)((readl(nand_info->nand  + DW_FC_FBYP_DATA) & DW_FC_FBYP_DATA_FLASH_DATA) >> 16);

   writel(0x12, (nand_info->nand + DW_FC_FBYP_CTL));
   while((readl(nand_info->nand + DW_FC_FBYP_CTL) & DW_FC_FBYP_CTL_BUSY) != 0);
   FlsId->DvcId = (__u8)((readl(nand_info->nand  + DW_FC_FBYP_DATA) & DW_FC_FBYP_DATA_FLASH_DATA) >> 16);

   writel(0x12, (nand_info->nand + DW_FC_FBYP_CTL));
   while((readl(nand_info->nand + DW_FC_FBYP_CTL) & DW_FC_FBYP_CTL_BUSY) != 0);
   FlsId->Mnglss = (__u8)((readl(nand_info->nand  + DW_FC_FBYP_DATA) & DW_FC_FBYP_DATA_FLASH_DATA) >> 16);

   writel(0x12, (nand_info->nand + DW_FC_FBYP_CTL));
   while((readl(nand_info->nand + DW_FC_FBYP_CTL) & DW_FC_FBYP_CTL_BUSY) != 0);
   FlsId->ExId = (__u8)((readl(nand_info->nand  + DW_FC_FBYP_DATA) & DW_FC_FBYP_DATA_FLASH_DATA) >> 16);

DW_SPOT("\n\t\tFlsId->MnfctrCde = %0#4x, FlsId->DvcId = %0#4x, FlsId->Mnglss = %0#4x, FlsId->ExId = %0#4x.", 
		FlsId->MnfctrCde, FlsId->DvcId, FlsId->Mnglss, FlsId->ExId);
   /*! DISABLE ByPass mode */
   writel(!DW_FC_FBYP_CTL_BP_EN, (nand_info->nand + DW_FC_FBYP_CTL));

} /* end F_CmdReadDeviceIdBypass() */


/**************************************************************************
* Function Name:  F_CmdReadDeviceStatDirectBypass
*
* Description:    Reads the Flash status, bypassing the DMA & Fifo.
*
* Input:          FlsDev: Flash Device number.
*
* Output:         None.
*
* Return Value:   Status from the chip->
***************************************************************************/
/*static*/ unsigned  F_CmdReadDeviceStatDirectBypass( struct dw_nand_info *nand_info )
{
//   Y_FC_Ctrl     FC_CtrlImg;
//   Y_FC_CmdSeq   FC_CmdSeqImg;
   __u16        FlashData;
   __u8         FlsStat;

//	udelay(100);
//NFLC_CAPTURE();
//   writel(0, (DW_NAND_LCD_BASE + DW_FC_CTL));
   F_SetFixedCntrlRegs(nand_info->nand) ;

//   *((__u32 *)&FC_CtrlImg) = 0;
//   *((__u32 *)&FC_CmdSeqImg) = 0;

   /*! DISABLE DMA & FIFO */
   DMF_F_ConfDmaBypass(&nand_info->dma_regs, DMF_K_PeriphFlsCntl);

   /*! PREPARE Device-Status Read, using 16bit width */
   // Read Device Status command
   V_FlsCmnd1 = C_FlsCmdReadStat;

   /*! CONFIGURE Flash-Controller registers: */
   /* NOTE: Image of FC_Ctrl register is needed, because a write to
   **       this register triggers a Flash-Controller transaction. */
//   FC_CtrlImg.ChainCntStart = 0;
//   FC_CtrlImg.PageSelect = 0;
//   FC_CtrlImg.EccOpMode = C_FC_Ctrl_EccOpMode_MemTrans; // Memory transaction

   // Clear relevant bits in Status register
   writel(SET_32BIT, (nand_info->nand + DW_FC_STS_CLR));

   // Disable all FC interrupts - To be changed
   /* NOTE: This register is configured by F_Init() */

   // Configutre Sequence register

//   FC_CmdSeqImg.RdyInEn = C_BitDisable;     // Don't Monitor Ready line - not activated by the
                                                     //   Flash chip in this command !!!
//   FC_CmdSeqImg.KeepChpSlct = C_BitDisable; // Don't keep CS for next transaction
//   FC_CmdSeqImg.DataEccEn = C_BitDisable;   // Disable ECC. To be changed
//   FC_CmdSeqImg.DataRWDir = 0;              // Read from Flash
//   FC_CmdSeqImg.DataRWEn = C_BitEnable;     // Enable Read/Write transfer

//   FC_CmdSeqImg.Mode16 = K_FlsAccess_16bit;
//   FC_CmdSeqImg.ReadOnce = C_BitDisable;    // Disable ReadOnce
//   FC_CmdSeqImg.Wait4En = C_BitDisable;
//   FC_CmdSeqImg.Cmd4En = C_BitDisable;
//   FC_CmdSeqImg.Wait3En = C_BitDisable;
//   FC_CmdSeqImg.Cmd3En = C_BitDisable;
//   FC_CmdSeqImg.Wait2En = C_BitDisable;
//   FC_CmdSeqImg.Cmd2En = C_BitDisable;
//   FC_CmdSeqImg.Wait1En = C_BitEnable;
//   FC_CmdSeqImg.Addr5En = C_BitDisable;
//   FC_CmdSeqImg.Addr4En = C_BitDisable;
//   FC_CmdSeqImg.Addr3En = C_BitDisable;
//   FC_CmdSeqImg.Addr2En = C_BitDisable;
//   FC_CmdSeqImg.Addr1En = C_BitDisable;
//   FC_CmdSeqImg.Cmd1En = C_BitEnable;   // for Read Device Id command

//   V_FC_RegsP->FC_CmdSeq = FC_CmdSeqImg;

   // STATUS read sequence
   // Don't Monitor Ready line (not activated by the Flash chip in this command),
   // don't keep CS for next transaction, Disable ECC, enable R/W in Read direction
   //
   // We have to make 1 transaction on the bus. The only way to do it is read 2 bytes in __16_bits__ mode.
   // However we are urged to use 8 bit mode due to resrictions on the pinout.
   DW_FC_SET(nand_info->nand, DW_FC_SEQUENCE,
             DW_FC_SEQ_CMD1_EN/*for Read Status command*/,
			 DW_FC_SEQ_WAIT1_EN,
             DW_FC_SEQ_RW_EN, DW_FC_SEQ_DATA_READ_DIR/*read*/,
             DW_FC_SEQ_CHIP_SEL(nand_info->FlsDev), DW_FC_SEQ_RDY_EN,
             DW_FC_SEQ_MODE8/*8 bits interface*/, 
             0,0,0);

   // Configure Command Code register
//   V_FC_RegsP->FC_Cmd.Cmd1 = V_FlsCmnd1;
   writel(V_FlsCmnd1, (nand_info->nand + DW_FC_CMD));

   // Configure DataCount register
//   V_FC_RegsP->FC_Dcount.VirtPageCnt = 0; // ECC not activated
//   V_FC_RegsP->FC_Dcount.SpareN1Cnt = 0;
//   V_FC_RegsP->FC_Dcount.SpareN2Cnt = 0;
//   V_FC_RegsP->FC_Dcount.VirtMainCnt = 2; // 2 bytes on 16bit access --> Output 1 Read signals: FlsStat.
   // Configure DataCount register
   writel(0x2, (nand_info->nand + DW_FC_DCOUNT));


   // Configure Ready-Timeout register
//   V_FC_RegsP->FC_TimeOut.RdyTimeOutEn = C_BitDisable;
//   V_FC_RegsP->FC_TimeOut.RdyTimeOutCnt = 0;
   // Configure Ready-Timeout register
   writel(CLR, (nand_info->nand  + DW_FC_TIMEOUT));

   /*! SET Bypass enabled */
   //V_FC_RegsP->FC_FBypCtrl.FBypEn = C_BitEnable;
//   writel(DW_FC_FBYP_CTL_BP_EN, (nand_info->nand  + DW_FC_FBYP_CTL));
   /*! SET Bypass enabled */
   writel(DW_FC_FBYP_CTL_BP_EN, (nand_info->nand  + DW_FC_FBYP_CTL));

   /*! TRIGGER Flash controller */
//   V_FC_RegsP->FC_Ctrl = FC_CtrlImg;
   writel(SET_32BIT, (nand_info->nand  + DW_FC_STS_CLR));
NFLC_CAPTURE();
   /*! TRIGGER Flash controller */
   writel(DW_FC_CTL_ECC_OP_MODE(DW_FC_MEM_TRANS), (nand_info->nand  + DW_FC_CTL));

   /*! READ Flash Data via Bypass */
   writel(0x12, (nand_info->nand  + DW_FC_FBYP_CTL));
   while((readl(nand_info->nand  + DW_FC_FBYP_CTL) & DW_FC_FBYP_CTL_BUSY) != 0)
		;
   FlashData= (__u16)((readl(nand_info->nand   + DW_FC_FBYP_DATA) & DW_FC_FBYP_DATA_FLASH_DATA) >> 16);
   FlsStat = (__u8)FlashData;

   // Dummy read - see note above on 8-bit mode.
   /*! READ Flash Data via Bypass */
   writel(0x12, (nand_info->nand  + DW_FC_FBYP_CTL));
   while((readl(nand_info->nand  + DW_FC_FBYP_CTL) & DW_FC_FBYP_CTL_BUSY) != 0);
   FlashData= (__u16)((readl(nand_info->nand   + DW_FC_FBYP_DATA) & DW_FC_FBYP_DATA_FLASH_DATA) >> 16);

//DW_SPOT("Status read = %0#4x.",	FlsStat);
   /*! DISABLE ByPass mode */
   writel(!DW_FC_FBYP_CTL_BP_EN, (nand_info->nand  + DW_FC_FBYP_CTL));

   return FlsStat;
} /* end F_CmdReadDeviceStatBypass() */

/**************************************************************************
* Function Name:  F_CmdReadOOBBypass
*
* Description:    Reads the Bytes/Words in the Spare area, where the manufacturer
*                 has written the Bad Block marking, bypassing the DAM and FIFO.
*
* Input:          Chip:       Flash chip identity.
*                 FlsDev:     Flash Device number.
*                 Block:      Block number.
*                 Column:     Offset in the OOB area.
*                 PageInBlk:  Number of Page inside the Block.
*                 Len:        Number of bytes to read. The read should not cross
*                             the physical page boundary (Column + Len < OOBSize)
*
* Output:         DataP:  The value of the Bad Block Marking area.
*
* Return Value:   None.
*
***************************************************************************/
/*static*/ NFC_E_Err F_CmdReadOOBBypass( struct dw_nand_info *nand_info,
                                          __u16        Block,
                                          __u16        Column,
                                          __u8         PageInBlk,
                                          __u16        Len,
                                          void *       DataP )
{
   int ret_value = NFC_K_Err_OK;

   __u16     FlashData;
   __u16     i;

   down(&unified_dw_nflc_mutex);
	 /*! DISABLE DMA & FIFO */
//	writel(!DW_FC_FBYP_CTL_BP_EN, (nand_info->nand + DW_FC_FBYP_CTL));


   F_SetFixedCntrlRegs(nand_info->nand) ;

DW_SPOT_FUN("Entering: Chip=%d, FlsDev=%d, Block=%d, Column=%d, PageInBlk=%d, Len=%d", Chip, FlsDev, Block, Column, PageInBlk, Len);
     DMF_F_ConfDmaBypass(&nand_info->dma_regs, DMF_K_PeriphFlsCntl);

   // Clear relevant bits in Status register
   writel(SET_32BIT, (nand_info->nand + DW_FC_STS_CLR));
   
   V_FlsCycle1 = Column; //R.B.

   set_addr_cycles (nand_info, Block, PageInBlk, nand_info->mtd->writesize, DW_NAND_CMD_READ_OOB );
    // Configure Address registers
   /*V_FC_RegsP->FC_AddrCol.Addr1 = V_FlsCycle1;
   V_FC_RegsP->FC_AddrCol.Addr2 = V_FlsCycle2;
   V_FC_RegsP->FC_AddrRow.Addr3 = V_FlsCycle3;
   V_FC_RegsP->FC_AddrRow.Addr4 = V_FlsCycle4;
   V_FC_RegsP->FC_AddrRow.Addr5 = V_FlsCycle5;*/
   DW_FC_SET(nand_info->nand, DW_FC_ADDR_COL,
             DW_FC_ADD1(V_FlsCycle1), DW_FC_ADD2(V_FlsCycle2),
             0,0,0,0,0,0,0,0);
   DW_FC_SET(nand_info->nand, DW_FC_ADDR_ROW,
             DW_FC_ADD3(V_FlsCycle3), DW_FC_ADD4(V_FlsCycle4), DW_FC_ADD5(V_FlsCycle5),
             0,0,0,0,0,0,0);

   // Configure Command Code registers
   /*V_FC_RegsP->FC_Cmd.Cmd1 = V_FlsCmnd1;
   V_FC_RegsP->FC_Cmd.Cmd2 = V_FlsCmnd2;*/
   DW_FC_SET(nand_info->nand, DW_FC_CMD,
             DW_FC_CMD1(V_FlsCmnd1), DW_FC_CMD2(V_FlsCmnd2),
             0,0,0,0,0,0,0,0);

//   writel(DW_FC_DCOUNT_MAIN_CNT((Len+Column)), (DW_NAND_LCD_BASE + DW_FC_DCOUNT));
   writel(DW_FC_DCOUNT_MAIN_CNT(Len), (nand_info->nand + DW_FC_DCOUNT)); // R.B.: BUG fixed.

   // Configure Ready-Timeout register
   /*V_FC_RegsP->FC_TimeOut.RdyTimeOutEn = C_BitEnable;
   V_FC_RegsP->FC_TimeOut.RdyTimeOutCnt = C_FlsRdyTimeout;*/
   DW_FC_SET(nand_info->nand, DW_FC_TIMEOUT,
             DW_FC_TIMEOUT_RDY_EN, DW_FC_TIMEOUT_RDY_CNT(C_FlsRdyTimeout),
             0,0,0,0,0,0,0,0);

   /*! SET Bypass enabled */
   //V_FC_RegsP->FC_FBypCtrl.FBypEn = C_BitEnable;
   writel(DW_FC_FBYP_CTL_BP_EN, (nand_info->nand + DW_FC_FBYP_CTL));


   writel(SET_32BIT, (nand_info->nand + DW_FC_STS_CLR));
   /*! TRIGGER Flash controller */
NFLC_CAPTURE();
   //V_FC_RegsP->FC_Ctrl = FC_CtrlImg;
   writel(DW_FC_CTL_ECC_OP_MODE(DW_FC_MEM_TRANS), (nand_info->nand + DW_FC_CTL));

//   for(i=0; i<Len+Column; i++)
	for (i = 0; i < Len; i++) // R.B.: BUG fix
	{
		 //V_FC_RegsP->FC_FBypCtrl.FBypRdFls = C_BitEnable;
		writel(DW_FC_FBYP_CTL_BP_READ_FLASH | DW_FC_FBYP_CTL_BP_EN, (nand_info->nand + DW_FC_FBYP_CTL));
		 //while (V_FC_RegsP->FC_FBypCtrl.FBypBusy != 0);
		while ((readl(nand_info->nand + DW_FC_FBYP_CTL) & DW_FC_FBYP_CTL_BUSY) != 0)
			 ;
		//FlashData = (__u16)(V_FC_RegsP->FC_FBypData.FlsData);
		FlashData = (__u16)((readl(nand_info->nand  + DW_FC_FBYP_DATA) & DW_FC_FBYP_DATA_FLASH_DATA) >> 16);
		((__u8 *)DataP)[i] = (__u8)FlashData;

	}

   /*! POLL for transaction end */
   /* NOTE: This is a test to verify that you have performed all the declared Reads! */
   //while (V_FC_RegsP->FC_Stat.TransDone == 0);
//   while((readl(nand_info->nand + DW_FC_STATUS) & DW_FC_STATUS_TRANS_DONE) == 0)
//			;

       /*! DISABLE ByPass mode */
   //V_FC_RegsP->FC_FBypCtrl.FBypEn = C_BitDisable;
   writel(!DW_FC_FBYP_CTL_BP_EN, (nand_info->nand + DW_FC_FBYP_CTL)); //R.B.: FIXME: remove superfluous 

   /*! CHECK if Timeout was marked (chip blocked) */
   //FC_StatImg = V_FC_RegsP->FC_Stat;
   //if (FC_StatImg.RdyTimeOut != 0)
    if(readl(nand_info->nand + DW_FC_STATUS) & DW_FC_STATUS_RDY_TIMEOUT)
	{
		ret_value = NFC_K_Err_TimeOut;
	}
	up(&unified_dw_nflc_mutex);

   return ret_value;


} /* end F_CmdReadOOBBypass() */


// This function prepares the program transaction for small-page chips.
// Return value: the the actual column to be sent to the chip (offset within the program zone)
// NOTE: this function makes access to the controller, so it should be called with the acquired mutex
unsigned int F_PrepareSmallPageWriteTransaction( struct dw_nand_info *nand_info,
												 unsigned int  Column // offset from the start of physical page
												)
{
	unsigned int res = Column;
	unsigned int PageSize = nand_info->mtd->writesize;
	unsigned int read_cmd;

	if (PageSize > 512) {
		BUG();
	}

	if (Column >= PageSize) {
		// OOB area
		res -= PageSize;
		read_cmd = C_SamK9F56_FlsCmdReadSpare;
	} else if (Column >= 256) {
		res -= 256;
		read_cmd = C_SamK9F56_FlsCmdReadStrtHighHalf;
	} else {
		read_cmd = C_SamK9F56_FlsCmdReadStrtLowHalf;
	}


	V_FlsCmnd1 = 0;
	V_FlsCmnd2 = 0;
	V_FlsCmnd3 = 0;
	V_FlsCmnd4 = read_cmd;

	DW_FC_SET(nand_info->nand, DW_FC_SEQUENCE,
		     DW_FC_SEQ_CMD4_EN,
		     DW_FC_SEQ_WAIT4_EN, DW_FC_SEQ_KEEP_CS,
		     DW_FC_SEQ_CHIP_SEL(nand_info->FlsDev), DW_FC_SEQ_RDY_SEL(nand_info->FlsDev >>1),
		     DW_FC_SEQ_MODE8,
		     0,0,0,0);

	DW_FC_SET(nand_info->nand, DW_FC_CMD,
		     DW_FC_CMD1(V_FlsCmnd1), DW_FC_CMD2(V_FlsCmnd2),
		     DW_FC_CMD3(V_FlsCmnd3), DW_FC_CMD4(V_FlsCmnd4),
		     0,0,0,0,0,0);

	F_SetDataCount(nand_info->nand,0, 0, 0, 0);

	// Configure Ready-Timeout register
	DW_FC_SET(nand_info->nand, DW_FC_TIMEOUT,
		     DW_FC_TIMEOUT_RDY_EN, DW_FC_TIMEOUT_RDY_CNT(C_FlsRdyTimeout),
		     0,0,0,0,0,0,0,0);

	/*! SET Bypass disabled */
	writel(0, (nand_info->nand + DW_FC_FBYP_CTL));

   writel(SET_32BIT, (nand_info->nand + DW_FC_STS_CLR));

NFLC_CAPTURE();
	/*! TRIGGER Flash controller */
	writel(DW_FC_CTL_ECC_OP_MODE(DW_FC_MEM_TRANS), (nand_info->nand + DW_FC_CTL));

	/*! POLL for transaction end */
	while ((readl(nand_info->nand + DW_FC_STATUS) & DW_FC_STATUS_TRANS_DONE) == 0)
		;
	return res;
}

/**************************************************************************
* Function Name:  F_CmdWriteOOBBypass
*
* Description:    Writes the Bytes/Words in the Spare area, where the manufacturer
*                 has written the Bad Block marking, bypassing the DAM and FIFO.
*
* Input:          Chip:       Flash chip identity.
*                 FlsDev:     Flash Device number.
*                 Block:      Block number.
*                 PageInBlk:  Number of Page inside the Block.
                  Len:        NUmber of bytes to read.
*
* Output:         DataP:  The value of the Bad Block Marking area.
*
* Return Value:   None.
*
***************************************************************************/
/*static*/ NFC_E_Err  F_CmdWriteOOBBypass( struct dw_nand_info *nand_info,
                                          __u16        Block,
                                          __u8         PageInBlk,
                                          __u16        Len,
                                          void *       SpareP )
{


	__u16     FlashData;
	__u16               i;
	int ret_value = NFC_K_Err_OK;
	unsigned int column = nand_info->mtd->writesize;

DW_SPOT_FUN("Chip=%d, FlsDev=%d, Block=%u, Len=%u, PageInBlk=%u, SpareP=%0#10x", 
	 (int)Chip, (int)FlsDev, (unsigned)Block, (unsigned)Len, (unsigned)PageInBlk, (unsigned)SpareP);

	down(&unified_dw_nflc_mutex);
	F_SetFixedCntrlRegs(nand_info->nand) ;

	if (column <= 512)
		column = F_PrepareSmallPageWriteTransaction(nand_info, column);

	/*! DISABLE DMA & FIFO */
	DMF_F_ConfDmaBypass(&nand_info->dma_regs, DMF_K_PeriphFlsCntl);

	// Clear relevant bits in Status register
	writel(SET_32BIT, (nand_info->nand + DW_FC_STS_CLR));

	set_addr_cycles (nand_info, Block, PageInBlk, column, DW_NAND_CMD_WRITE );

	V_FlsCmnd4 = 0;
	// Configure Address registers
	/*V_FC_RegsP->FC_AddrCol.Addr1 = V_FlsCycle1;
	V_FC_RegsP->FC_AddrCol.Addr2 = V_FlsCycle2;
	V_FC_RegsP->FC_AddrRow.Addr3 = V_FlsCycle3;
	V_FC_RegsP->FC_AddrRow.Addr4 = V_FlsCycle4;
	V_FC_RegsP->FC_AddrRow.Addr5 = V_FlsCycle5;*/
	DW_FC_SET(nand_info->nand, DW_FC_ADDR_COL,
		     DW_FC_ADD1(V_FlsCycle1), DW_FC_ADD2(V_FlsCycle2),
		     0,0,0,0,0,0,0,0);
	DW_FC_SET(nand_info->nand, DW_FC_ADDR_ROW,
		     DW_FC_ADD3(V_FlsCycle3), DW_FC_ADD4(V_FlsCycle4), DW_FC_ADD5(V_FlsCycle5),
		     0,0,0,0,0,0,0);

	// Configure Command Code registers
	DW_FC_SET(nand_info->nand, DW_FC_CMD,
		     DW_FC_CMD1(V_FlsCmnd1), DW_FC_CMD2(V_FlsCmnd2),
		     DW_FC_CMD3(V_FlsCmnd3), DW_FC_CMD4(V_FlsCmnd4),
		     0,0,0,0,0,0);
	// Configure DataCount register
	/*V_FC_RegsP->FC_Dcount.VirtPageCnt = 0; // ECC not activated
	V_FC_RegsP->FC_Dcount.SpareN1Cnt = 0;
	V_FC_RegsP->FC_Dcount.SpareN2Cnt = 0;
	#ifndef USE_ReadOnce
	V_FC_RegsP->FC_Dcount.VirtMainCnt = 2; // 2 bytes on 16bit access --> Output 1 Read signals: MarkArea.
	#else
	V_FC_RegsP->FC_Dcount.VirtMainCnt = 0;
	#endif*/
	F_SetDataCount(nand_info->nand,Len, 0, 0, 0);

	// Configure Ready-Timeout register
	/*V_FC_RegsP->FC_TimeOut.RdyTimeOutEn = C_BitEnable;
	V_FC_RegsP->FC_TimeOut.RdyTimeOutCnt = C_FlsRdyTimeout;*/
	DW_FC_SET(nand_info->nand, DW_FC_TIMEOUT,
		     DW_FC_TIMEOUT_RDY_EN, DW_FC_TIMEOUT_RDY_CNT(C_FlsRdyTimeout),
		     0,0,0,0,0,0,0,0);

	/*! SET Bypass enabled */
	//V_FC_RegsP->FC_FBypCtrl.FBypEn = C_BitEnable;
	writel(DW_FC_FBYP_CTL_BP_EN, (nand_info->nand + DW_FC_FBYP_CTL));


   writel(SET_32BIT, (nand_info->nand + DW_FC_STS_CLR));
	/*! TRIGGER Flash controller */
NFLC_CAPTURE();
	//V_FC_RegsP->FC_Ctrl = FC_CtrlImg;
	writel(DW_FC_CTL_ECC_OP_MODE(DW_FC_MEM_TRANS), (nand_info->nand + DW_FC_CTL));

	for (i = 0; i < Len; i++)
	{
	   //while (V_FC_RegsP->FC_FBypCtrl.FBypBusy != 0);
	   while ((readl(nand_info->nand + DW_FC_FBYP_CTL) & DW_FC_FBYP_CTL_BUSY) != 0)
			;
	   //V_FC_RegsP->FC_FBypData.FifoData = *TmpU8P++;
	   writel(((__u8 *)SpareP)[i], (nand_info->nand + DW_FC_FBYP_DATA));
	   //V_FC_RegsP->FC_FBypCtrl.FBypWr = C_BitEnable;
	   writel(DW_FC_FBYP_CTL_BP_EN|DW_FC_FBYP_CTL_BP_WRITE,
		      (nand_info->nand + DW_FC_FBYP_CTL));
	}

	/*! POLL for transaction end */
	/* NOTE: This is a test to verify that you have performed all the declared Writes! */
	//while (V_FC_RegsP->FC_Stat.TransDone == 0);
	while((readl(nand_info->nand + DW_FC_STATUS) & DW_FC_STATUS_TRANS_DONE) == 0);

	   /*! DISABLE ByPass mode */
	//V_FC_RegsP->FC_FBypCtrl.FBypEn = C_BitDisable;
	writel(!DW_FC_FBYP_CTL_BP_EN, (nand_info->nand + DW_FC_FBYP_CTL));
	/*! CHECK if Timeout was marked (chip blocked) */
	//FC_StatImg = V_FC_RegsP->FC_Stat;
	//if (FC_StatImg.RdyTimeOut != 0)
	if(readl(nand_info->nand + DW_FC_STATUS) & DW_FC_STATUS_RDY_TIMEOUT)
	{
	//      return (NFC_K_Err_TimeOut);
		ret_value = NFC_K_Err_TimeOut;
		goto out;
	}

	FlashData = M_ReadStatus(nand_info);

	if ((FlashData & 0x001) != 0)
	{
	//       return (NFC_K_Err_ProgFail);
		ret_value = NFC_K_Err_ProgFail;
		goto out;
	}

out:
    up(&unified_dw_nflc_mutex);
	return ret_value;
} /* end F_CmdWriteOOBBypass() */



/**************************************************************************
* Function Name:  F_CmdEraseBlkBypass
*
* Description:    Erases a whole Flash Block, Bypassing the DMA and FIFO.
*
* Input:          Chip:       Flash chip identity.
*                 FlsDev:     Flash Device number.
*                 Block:      Block number.
*
* Output:         None.
*
* Return Value:   NFC_K_Err_OK:              Block Erase OK.
*                 NFC_K_Err_EraseFail:       Block Erase failed.
*
***************************************************************************/
/*static*/ NFC_E_Err  F_CmdEraseBlkBypass( struct dw_nand_info *nand_info,
                                       __u16        Block )
{
   __u16        FlashData;
   __u8         stat=0;
   int ret_value = NFC_K_Err_OK;

DW_SPOT_FUN("Chip=%d, FlsDev=%d, Block=%u", (int)Chip, (int)FlsDev, (unsigned)Block);

   down(&unified_dw_nflc_mutex);
   
   F_SetFixedCntrlRegs(nand_info->nand) ;
   set_addr_cycles(nand_info, Block, 0 , 0 , DW_NAND_CMD_ERASE);
   /*! DISABLE DMA & FIFO */
   DMF_F_ConfDmaBypass(&nand_info->dma_regs, DMF_K_PeriphFlsCntl);

   // Clear relevant bits in Status register
   writel(SET_32BIT, (nand_info->nand + DW_FC_STS_CLR));

   // Configure Address registers
   /*V_FC_RegsP->FC_AddrCol.Addr1 = V_FlsCycle1;
   V_FC_RegsP->FC_AddrCol.Addr2 = V_FlsCycle2;
   V_FC_RegsP->FC_AddrRow.Addr3 = V_FlsCycle3;
   V_FC_RegsP->FC_AddrRow.Addr4 = V_FlsCycle4;
   V_FC_RegsP->FC_AddrRow.Addr5 = V_FlsCycle5;*/
   DW_FC_SET(nand_info->nand, DW_FC_ADDR_COL,
             DW_FC_ADD1(V_FlsCycle1), DW_FC_ADD2(V_FlsCycle2),
             0,0,0,0,0,0,0,0);
   DW_FC_SET(nand_info->nand, DW_FC_ADDR_ROW,
             DW_FC_ADD3(V_FlsCycle3), DW_FC_ADD4(V_FlsCycle4), DW_FC_ADD5(V_FlsCycle5),
             0,0,0,0,0,0,0);

   // Configure Command Code register
   /*V_FC_RegsP->FC_Cmd.Cmd1 = V_FlsCmnd1;
   V_FC_RegsP->FC_Cmd.Cmd2 = V_FlsCmnd2;
   V_FC_RegsP->FC_Cmd.Cmd3 = V_FlsCmnd3;
   V_FC_RegsP->FC_Cmd.Cmd4 = V_FlsCmnd4;*/
   DW_FC_SET(nand_info->nand, DW_FC_CMD,
             DW_FC_CMD1(V_FlsCmnd1), DW_FC_CMD2(V_FlsCmnd2), DW_FC_CMD4(V_FlsCmnd4),
             0,0,0,0,0,0,0);

   // Configure DataCount register
   /*V_FC_RegsP->FC_Dcount.VirtPageCnt = 0; // ECC not activated
   V_FC_RegsP->FC_Dcount.SpareN1Cnt = 0;
   V_FC_RegsP->FC_Dcount.SpareN2Cnt = 0;
   V_FC_RegsP->FC_Dcount.VirtMainCnt = 0;*/
   writel(CLR, (nand_info->nand + DW_FC_DCOUNT));

   // Configure Ready-Timeout register
   /*V_FC_RegsP->FC_TimeOut.RdyTimeOutEn = C_BitEnable;
   V_FC_RegsP->FC_TimeOut.RdyTimeOutCnt = 50; // 3 mSec in 33MHz clk*/
   DW_FC_SET(nand_info->nand, DW_FC_TIMEOUT,
             DW_FC_TIMEOUT_RDY_EN, DW_FC_TIMEOUT_RDY_CNT(C_FlsRdyTimeout),
             0,0,0,0,0,0,0,0);

   /*! SET Bypass Disabled */
   //V_FC_RegsP->FC_FBypCtrl.FBypEn = C_BitDisable;
   writel(!DW_FC_FBYP_CTL_BP_EN, (nand_info->nand + DW_FC_FBYP_CTL));

   writel(SET_32BIT, (nand_info->nand + DW_FC_STS_CLR));
   /*! TRIGGER Flash controller */
NFLC_CAPTURE();
   //V_FC_RegsP->FC_Ctrl = FC_CtrlImg;
   writel(DW_FC_CTL_ECC_OP_MODE(DW_FC_MEM_TRANS), (nand_info->nand + DW_FC_CTL));

   /*! POLL for transaction End */
   //while (V_FC_RegsP->FC_Stat.TransDone == 0);
   while((readl(nand_info->nand + DW_FC_STATUS) 
          & DW_FC_STATUS_TRANS_DONE) == 0);

   /*! READ Flash data (Successful Erase: I/O 0 = '0'; Error in Erase: I/O 0 = '1') */
//   FlashData = M_ReadStatus(FlsDev); // R.B.: need to wait for erasing to finish
   while (((FlashData = M_ReadStatus(nand_info)) & 0x40) == 0 || // FIXME: need to add timeout
			FlashData == 0xff)                                // FIXME: this fixes weird feature with small-paged Toshiba
		; 

   stat = (__u8)FlashData;
   if ((stat & 0x01) == 0x00)
   {
      //return (NFC_K_Err_OK);
      ret_value = NFC_K_Err_OK;
   }
   else
   {
      //return (NFC_K_Err_EraseFail);
      ret_value = NFC_K_Err_EraseFail;
   }
   up(&unified_dw_nflc_mutex);
   return ret_value;
} /* end F_CmdEraseBlkBypass() */

NFC_E_Err dw_nand_prepare_readpage(struct dw_nand_info *nand_info,
                                               __u16        Block,
                                               __u16        DataLen,
                                               __u16        SpareLen,
                                               __u8         PageInBlk,
                                               __u8         N1Len,
                                               int          IsEccF)

{

   Y_FC_Ctrl            FC_CtrlImg;
   Y_FC_CmdSeq          FC_CmdSeqImg;


   __u16 Column = 0;
 //  printk("!!! CHECK 0x%x, 0x%x, 0x%x, 0x%x , 0x%x, 0x%x, 0x%x, 0x%x\n", Chip, FlsDev, Block, DataLen, SpareLen, PageInBlk, N1Len, IsEccF);
   //IsEccF=0; 
   down(&unified_dw_nflc_mutex);

   *((__u32 *)&FC_CtrlImg) = 0;
   *((__u32 *)&FC_CmdSeqImg) = 0;

   /*! DISABLE DMA & FIFO */
   DMF_F_ConfDmaBypass(&nand_info->dma_regs, DMF_K_PeriphFlsCntl);

   /*! SET pointer to Flash chip parameters */

   set_addr_cycles (nand_info, Block, PageInBlk, 0, DW_NAND_CMD_READ );


   // Clear relevant bits in Status register
   writel(SET_32BIT, (nand_info->nand + DW_FC_STS_CLR));

   DW_FC_SET(nand_info->nand, DW_FC_ADDR_COL,
             DW_FC_ADD1(V_FlsCycle1), DW_FC_ADD2(V_FlsCycle2),
             0,0,0,0,0,0,0,0);
   DW_FC_SET(nand_info->nand, DW_FC_ADDR_ROW,
             DW_FC_ADD3(V_FlsCycle3), DW_FC_ADD4(V_FlsCycle4), DW_FC_ADD5(V_FlsCycle5),
             0,0,0,0,0,0,0);


   // Configure Command Code register
   DW_FC_SET(nand_info->nand, DW_FC_CMD,
             DW_FC_CMD1(V_FlsCmnd1), DW_FC_CMD2(V_FlsCmnd2),
             0,0,0,0,0,0,0,0);

   /*****************************************************/

   // Configure DataCount register
   F_SetDataCount(nand_info->nand, DataLen +Column, SpareLen, IsEccF, N1Len);

   // Configure Ready-Timeout register
   /*V_FC_RegsP->FC_TimeOut.RdyTimeOutEn = C_BitEnable;
   V_FC_RegsP->FC_TimeOut.RdyTimeOutCnt = C_FlsRdyTimeout;*/
   DW_FC_SET(nand_info->nand, DW_FC_TIMEOUT,
             DW_FC_TIMEOUT_RDY_EN, DW_FC_TIMEOUT_RDY_CNT(C_FlsRdyTimeout),
             0,0,0,0,0,0,0,0);

   /*! SET Bypass enabled */
   //V_FC_RegsP->FC_FBypCtrl.FBypEn = C_BitEnable;
   writel(DW_FC_FBYP_CTL_BP_EN, (nand_info->nand + DW_FC_FBYP_CTL));

   /*! TRIGGER Flash controller */
   //V_FC_RegsP->FC_Ctrl = FC_CtrlImg;
   writel(DW_FC_CTL_ECC_OP_MODE(DW_FC_MEM_TRANS), (nand_info->nand + DW_FC_CTL));

   up(&unified_dw_nflc_mutex);

   return 0;
   
}



/*static*/ NFC_E_Err  dw_nand_CmdWritePageAndSpareBypass( struct dw_nand_info *nand_info,
                                                __u16        Block,
                                                __u16        DataLen,
                                                __u16        SpareLen,
                                                __u8         PageInBlk,
                                                __u8         N1Len,
                                                int          IsEccF)
{
   Y_FC_Ctrl            FC_CtrlImg;
   Y_FC_CmdSeq          FC_CmdSeqImg;

   int ret_value = NFC_K_Err_OK;
 //  printk("!!! CHECK 0x%x, 0x%x, 0x%x, 0x%x , 0x%x, 0x%x, 0x%x, 0x%x\n", Chip, FlsDev, Block, DataLen, SpareLen, PageInBlk, N1Len, IsEccF);
   //IsEccF=0;
   down(&unified_dw_nflc_mutex);

   *((__u32 *)&FC_CtrlImg) = 0;
   *((__u32 *)&FC_CmdSeqImg) = 0;

/*! DISABLE DMA & FIFO */
   DMF_F_ConfDmaBypass(&nand_info->dma_regs, DMF_K_PeriphFlsCntl);

   // Clear relevant bits in Status register
   writel(SET_32BIT, (nand_info->nand + DW_FC_STS_CLR));

   set_addr_cycles (nand_info, Block, PageInBlk, 0, DW_NAND_CMD_WRITE );

   // Configure Address registers
   /*V_FC_RegsP->FC_AddrCol.Addr1 = V_FlsCycle1;
   V_FC_RegsP->FC_AddrCol.Addr2 = V_FlsCycle2;
   V_FC_RegsP->FC_AddrRow.Addr3 = V_FlsCycle3;
   V_FC_RegsP->FC_AddrRow.Addr4 = V_FlsCycle4;
   V_FC_RegsP->FC_AddrRow.Addr5 = V_FlsCycle5;*/
   DW_FC_SET(nand_info->nand, DW_FC_ADDR_COL,
             DW_FC_ADD1(V_FlsCycle1), DW_FC_ADD2(V_FlsCycle2),
             0,0,0,0,0,0,0,0);
   DW_FC_SET(nand_info->nand, DW_FC_ADDR_ROW,
             DW_FC_ADD3(V_FlsCycle3), DW_FC_ADD4(V_FlsCycle4), DW_FC_ADD5(V_FlsCycle5),
             0,0,0,0,0,0,0);

   // Configure Command Code register
   /*V_FC_RegsP->FC_Cmd.Cmd1 = V_FlsCmnd1;
   V_FC_RegsP->FC_Cmd.Cmd2 = V_FlsCmnd2; // Not used in this operation.
   V_FC_RegsP->FC_Cmd.Cmd3 = V_FlsCmnd3;
   V_FC_RegsP->FC_Cmd.Cmd4 = V_FlsCmnd4;*/
   DW_FC_SET(nand_info->nand, DW_FC_CMD,
             DW_FC_CMD1(V_FlsCmnd1), DW_FC_CMD2(V_FlsCmnd2),
             DW_FC_CMD3(V_FlsCmnd3), DW_FC_CMD4(V_FlsCmnd4),
             0,0,0,0,0,0);

   // Configure DataCount register
   F_SetDataCount(nand_info->nand, DataLen , SpareLen, IsEccF, N1Len);

  // Configure Ready-Timeout register
   /*V_FC_RegsP->FC_TimeOut.RdyTimeOutEn = C_BitEnable;
   V_FC_RegsP->FC_TimeOut.RdyTimeOutCnt = C_FlsRdyTimeout;*/
   DW_FC_SET(nand_info->nand, DW_FC_TIMEOUT,
             DW_FC_TIMEOUT_RDY_EN, DW_FC_TIMEOUT_RDY_CNT(C_FlsRdyTimeout),
             0,0,0,0,0,0,0,0);

   /*! SET Bypass enabled */
   //V_FC_ResP->FC_FBypCtrl.FBypEn = C_BitEnable;
   writel(DW_FC_FBYP_CTL_BP_EN, (nand_info->nand + DW_FC_FBYP_CTL));

   /*! TRIGGER Flash controller */
   //V_FC_RegsP->FC_Ctrl = FC_CtrlImg;
   writel(DW_FC_CTL_ECC_OP_MODE(DW_FC_MEM_TRANS), (nand_info->nand + DW_FC_CTL));

   return ret_value;
}
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
                                               void *        SpareP )
{

	int ret_value = NFC_K_Err_OK;

   	return ret_value;

} /* end F_CmdReadPageAndSpareBypass() */

#endif
void F_ReadByPass(struct dw_nand_info *nand_info, uint8_t *buf, int len)
{
	int i;
	nand_info->to_read -= len;
	for (i = 0; i < len; i++)
       	{
		writel(DW_FC_FBYP_CTL_BP_EN|DW_FC_FBYP_CTL_BP_READ_FLASH,
	       		(nand_info->nand + DW_FC_FBYP_CTL));
		while ((readl(nand_info->nand + DW_FC_FBYP_CTL)
               		& DW_FC_FBYP_CTL_BUSY) != 0)
			cpu_relax();

		buf[i] = (__u8)((readl(nand_info->nand +
                 	DW_FC_FBYP_DATA) & DW_FC_FBYP_DATA_FLASH_DATA) >> 16);
	}
//	if(nand_info->to_read>0)
//		printk("NAND still to read 0x%x\n",nand_info->to_read);

};


/**************************************************************************
* Function Name:  F_CmdReadPageAndSpare
*
* Description:    Prepares a Command Sequence for Reading a whole
*                 physical Page + its Spare area.
*                 Configures DMA & FIFO, then activates the transaction.
*                 Possible combination:
*                   1. Net Page size:  512 Bytes; Spare area size:  16 Bytes.
*                   2. Net Page size: 2048 Bytes; Spare area size:  64 Bytes.
*                   3. Net Page size: 4096 Bytes; Spare area size: 128 Bytes.
*
*                 In FPGA emulation of the DM56, Page (DataP) and Spare (SpareP) buffers
*                 !!! MUST !!! be allocated within the FPGA RAM, due to the DMA transfer !
*
* Input:          Chip:       Flash chip identity.
*                 FlsDev:     Flash Device number.
*                 Block:      Block number.
*                 PageInBlk:  Number of Page inside the Block.
*                 N1Len:      Ecc N1 field length.
*                 IsEccF:     Whether to activate ECC machine.
*
* Output:         DataP:  User supplied Page Data output buffer.
*                 SpareP: User supplied Page's Spare-Area output buffer.
*
* Return Value:   NFC_K_Err_OK:       Read OK.
*                 NFC_K_Err_TimeOut:  Read Timeout.
*
* NOTE: P/O the spare-area data contains the ECC (Error correction Code).
*
***************************************************************************/
/*static*/ NFC_E_Err  F_CmdReadPageAndSpare( struct dw_nand_info *nand_info,
                                         __u16        Block,
                                         __u16        DataLen,
                                         __u16        SpareLen,
                                         __u8         PageInBlk,
                                         __u8         N1Len,
                                         int          IsEccF,
                                         void *        DataP_DMA,
                                         void *        SpareP_DMA,
                                         void *        DataP_Virt,
                                         void *        SpareP_Virt )
{
   DMF_E_Width          FifoBusWidth;
   Y_FC_Ctrl            FC_CtrlImg;
   int ret_value = NFC_K_Err_OK;
g1();
   down(&unified_dw_nflc_mutex);
g2();
   //IsEccF=0;
   F_SetFixedCntrlRegs(nand_info->nand) ;

/*
  printk("READ: Chip %u, FlsDev %u, Block %4u, DataLen %u, SpareLen %u, PageInBlk %2u, N1Len %u, IsEccF %u, DataP_DMA %0#10x, SpareP_DMA %0#10x\n",
	Chip, FlsDev, Block, DataLen, SpareLen, PageInBlk, N1Len, IsEccF, (unsigned)DataP_DMA, (unsigned)SpareP_DMA);
*/
/*	DW_SPOT_FUN("READ: Chip %u, FlsDev %u, Block %4u, DataLen %u, SpareLen %u, PageInBlk %2u, N1Len %u, IsEccF %u, DataP_DMA %0#10x, SpareP_DMA %0#10x",
		Chip, FlsDev, Block, DataLen, SpareLen, PageInBlk, N1Len, IsEccF, (unsigned)DataP_DMA, (unsigned)SpareP_DMA);
*/
DW_SPOT_FUN("READ: Chip %u, Dev %u, Blk %4u, Pg %2u, DLen %u, SLen %u, IsEccF %u, DataP %0#10x, SpareP %0#10x",
	               Chip, FlsDev, Block, PageInBlk, DataLen, SpareLen, IsEccF, (unsigned)DataP_DMA, (unsigned)SpareP_DMA);


   *((__u32 *)&FC_CtrlImg) = 0;
   /*! SET pointer to Flash chip parameters */
   FifoBusWidth = (!(NAND_BUSWIDTH_16 &(nand_info->chip->options))) ? DMF_K_Width8bit : DMF_K_Width16bit;
   set_addr_cycles (nand_info, Block, PageInBlk, 0, DW_NAND_CMD_READ );

   /*! CONFIGURE DMA & FIFO & GCR2 registers */
   DMF_F_ConfDmaFifo( &nand_info->dma_regs,
		      DMF_K_DmaDirWriteAHB,
                      DMF_K_PeriphFlsCntl,
                      FifoBusWidth,
                      DataLen,
                      SpareLen,
                      (__u32 *)DataP_DMA,
                      (__u32 *)SpareP_DMA );

   /*! CONFIGURE Flash-Controller registers: */
  
   // Clear relevant bits in Status register
   writel(SET_32BIT, (nand_info->nand + DW_FC_STS_CLR));

   // Disable all FC interrupts - To be changed
   /* NOTE: This register is configured by F_Init() */

   // Configutre Sequence register

/*
   FC_CmdSeqImg.RdyInEn = C_BitEnable;      // Monitor Ready line
   FC_CmdSeqImg.KeepChpSlct = C_BitDisable; // Don't keep CS for next transaction
   FC_CmdSeqImg.DataEccEn = IsEccF ? C_BitEnable: C_BitDisable; // Enable / Disable ECC
   FC_CmdSeqImg.DataRWDir = 0;              // Read from Flash
   FC_CmdSeqImg.DataRWEn = C_BitEnable;     // Enable Read/Write transfer

   FC_CmdSeqImg.ReadOnce = C_BitDisable;
   FC_CmdSeqImg.Wait4En = C_BitDisable;
   FC_CmdSeqImg.Cmd4En = C_BitDisable;
   FC_CmdSeqImg.Wait3En = C_BitDisable;
   FC_CmdSeqImg.Cmd3En = C_BitDisable;
   FC_CmdSeqImg.Cmd1En = C_BitEnable;  // for Read command
   FC_CmdSeqImg.Addr3En = C_BitEnable;
   FC_CmdSeqImg.Addr2En = C_BitEnable;
   FC_CmdSeqImg.Addr1En = C_BitEnable;
*/
   DW_FC_ADD(nand_info->nand, DW_FC_SEQUENCE, DW_FC_SEQ_DATA_ECC(IsEccF),
		   0,0,0,0,0,0,0,0,0);
   // Configure Address registers
   /*V_FC_RegsP->FC_AddrCol.Addr1 = V_FlsCycle1;
   V_FC_RegsP->FC_AddrCol.Addr2 = V_FlsCycle2;
   V_FC_RegsP->FC_AddrRow.Addr3 = V_FlsCycle3;
   V_FC_RegsP->FC_AddrRow.Addr4 = V_FlsCycle4;
   V_FC_RegsP->FC_AddrRow.Addr5 = V_FlsCycle5;*/
   DW_FC_SET(nand_info->nand, DW_FC_ADDR_COL,
             DW_FC_ADD1(V_FlsCycle1), DW_FC_ADD2(V_FlsCycle2),
             0,0,0,0,0,0,0,0);
   DW_FC_SET(nand_info->nand, DW_FC_ADDR_ROW,
             DW_FC_ADD3(V_FlsCycle3), DW_FC_ADD4(V_FlsCycle4), DW_FC_ADD5(V_FlsCycle5),
             0,0,0,0,0,0,0);
   // Configure Command Code register
   /*V_FC_RegsP->FC_Cmd.Cmd1 = V_FlsCmnd1;
   V_FC_RegsP->FC_Cmd.Cmd2 = V_FlsCmnd2;*/
   DW_FC_SET(nand_info->nand, DW_FC_CMD,
             DW_FC_CMD1(V_FlsCmnd1), DW_FC_CMD2(V_FlsCmnd2),
             0,0,0,0,0,0,0,0);
   // Configure WaitTime register
   /* NOTE: This register is configured by F_Init() */ // Done above in the function F_SetFixedCntrlRegs

   // Configure PulseTime register
   /* NOTE: This register is configured by F_Init() */ // Done above in the function F_SetFixedCntrlRegs

   // Configure DataCount register
   F_SetDataCount(nand_info->nand, DataLen , SpareLen, IsEccF, N1Len);
 
  // Configure Ready-Timeout register
   /*V_FC_RegsP->FC_TimeOut.RdyTimeOutEn = C_BitEnable;
   V_FC_RegsP->FC_TimeOut.RdyTimeOutCnt = C_FlsRdyTimeout;*/
   DW_FC_SET(nand_info->nand, DW_FC_TIMEOUT,
             DW_FC_TIMEOUT_RDY_EN, DW_FC_TIMEOUT_RDY_CNT(C_FlsRdyTimeout),
             0,0,0,0,0,0,0,0);

   // Configure Flash-Controller to Disable DMA/FIFO Bypass
//   writel(!DW_FC_FBYP_CTL_BP_EN, (DW_NAND_LCD_BASE + DW_FC_FBYP_CTL)); // ERROR!
   writel(0x0, (nand_info->nand + DW_FC_FBYP_CTL));

   /*! TRIGGER DMA controller */
   DMF_F_TrigDma(&nand_info->dma_regs);
   writel(SET_32BIT, (nand_info->nand + DW_FC_STS_CLR));

NFLC_CAPTURE();
   /*! TRIGGER Flash controller */
   writel(DW_FC_CTL_ECC_OP_MODE(DW_FC_MEM_TRANS), (nand_info->nand + DW_FC_CTL));

   /* NOTE: The Following section will be replaced by interrupt handling
   **       ISR, which will send an event to a waiting task.
   **       This task will fix ECC errors, then call a call-back function
   **       of the registered user - This is for ASync I/O */

   while(((readl(nand_info->nand + DW_FC_STATUS) & DW_FC_STATUS_TRANS_DONE) == 0)
       || ((readl(nand_info->dma_regs.dma_base_reg + DW_DMA_ISR) & DW_DMA_ISR_DMA_DONE) == 0));

   /*! CHECK if Timeout was marked (chip blocked) */
   //FC_StatImg = V_FC_RegsP->FC_Stat;
   //if (FC_StatImg.RdyTimeOut != 0)
   if(readl(nand_info->nand + DW_FC_STATUS) & DW_FC_STATUS_RDY_TIMEOUT)
   {
	ret_value = NFC_K_Err_TimeOut;
	goto out;
   }

	// Due to HW bug the DW_FC_STATUS_TRANS_DONE bit reports that the transaction is done while
	//   the transaction is still on going. Using debug register to overcome the obstacle.
   if (IsEccF != 0)
   {
//DW_SPOT("We are inside    if (IsEccF != 0)");
      F_FixEccErrors(nand_info, DataP_Virt, SpareP_Virt);
   }
out:
g3();
   up(&unified_dw_nflc_mutex);
g4();
print_g();

   return ret_value;

} /* end F_CmdReadPageAndSpare() */


/*static*/ NFC_E_Err  dw_nand_prepareWritePageAndSpare( struct dw_nand_info *nand_info,
                                          __u16        Block,
                                          __u16        DataLen,
                                          __u16        SpareLen,
                                          __u8         PageInBlk,
                                          __u8         N1Len,
					  void *       DataP,
					  void *       SpareP,
                                          int          IsEccF)
{
	DMF_E_Width                    FifoBusWidth;
	int ret_value                = NFC_K_Err_OK;
	unsigned int PageSize        = nand_info->mtd->writesize;
	unsigned int Column          = 0; // We start the write operation from the beginning of the physical page.


//        printk("dw_nand_prepareWrite:  0%x, 0%x, 0%x, 0%x, 0%x, 0%x\n", Block, DataLen,SpareLen,PageInBlk,N1Len,IsEccF);
	down(&unified_dw_nflc_mutex);
	
	F_SetFixedCntrlRegs(nand_info->nand) ;

	if (PageSize <= 512) {
		Column = F_PrepareSmallPageWriteTransaction(nand_info, Column);
	}
 
	/*! SET pointer to Flash chip parameters */
	//IsEccF=0;
	FifoBusWidth = (!(NAND_BUSWIDTH_16 &(nand_info->chip->options))) ? DMF_K_Width8bit : DMF_K_Width16bit;

	set_addr_cycles (nand_info, Block, PageInBlk, 0, DW_NAND_CMD_WRITE );

	SET_DMACONFIG(dw_DMA_config.TransConfig, 
		      DMF_K_DmaDirReadAHB, 
                      DMF_K_PeriphFlsCntl,
                      FifoBusWidth,
                      DataLen,
                      SpareLen,
                      (__u32 *)DataP,
                      (__u32 *)SpareP);
	DMF_F_ConfDmaFifo(&nand_info->dma_regs,
		      DMF_K_DmaDirReadAHB,
                      DMF_K_PeriphFlsCntl,
                      FifoBusWidth,
                      DataLen,
                      SpareLen,
                      (__u32 *)DataP,
                      (__u32 *)SpareP );

	/*! CONFIGURE Flash-Controller registers: */

	// Clear relevant bits in Status register
	writel(SET_32BIT, (nand_info->nand + DW_FC_STS_CLR));
	DW_FC_ADD(nand_info->nand,DW_FC_SEQUENCE, DW_FC_SEQ_DATA_ECC(IsEccF),0,0,0,0,0,0,0,0,0);
   	// Configure Address registers
   	/*V_FC_RegsP->FC_AddrCol.Addr1 = V_FlsCycle1;
   	V_FC_RegsP->FC_AddrCol.Addr2 = V_FlsCycle2;
  	V_FC_RegsP->FC_AddrRow.Addr3 = V_FlsCycle3;
   	V_FC_RegsP->FC_AddrRow.Addr4 = V_FlsCycle4;
   	V_FC_RegsP->FC_AddrRow.Addr5 = V_FlsCycle5;*/
   	DW_FC_SET(nand_info->nand, DW_FC_ADDR_COL,
             DW_FC_ADD1(V_FlsCycle1), DW_FC_ADD2(V_FlsCycle2),
             0,0,0,0,0,0,0,0);
   	DW_FC_SET(nand_info->nand, DW_FC_ADDR_ROW,
             DW_FC_ADD3(V_FlsCycle3), DW_FC_ADD4(V_FlsCycle4), DW_FC_ADD5(V_FlsCycle5),
             0,0,0,0,0,0,0);

  	// Configure Command Code register
   	/*V_FC_RegsP->FC_Cmd.Cmd1 = V_FlsCmnd1;
   	V_FC_RegsP->FC_Cmd.Cmd2 = V_FlsCmnd2; // Not used in this operation.
   	V_FC_RegsP->FC_Cmd.Cmd3 = V_FlsCmnd3;
   	V_FC_RegsP->FC_Cmd.Cmd4 = V_FlsCmnd4;*/

   	DW_FC_SET(nand_info->nand, DW_FC_CMD,
             DW_FC_CMD1(V_FlsCmnd1), DW_FC_CMD2(V_FlsCmnd2),
             DW_FC_CMD3(V_FlsCmnd3), DW_FC_CMD4(V_FlsCmnd4),
             0,0,0,0,0,0);

   	// Configure WaitTime register
   	/* NOTE: This register is configured by F_Init() */

   	// Configure PulseTime register
   	/* NOTE: This register is configured by F_Init() */

  	// Configure DataCount register
   	F_SetDataCount(nand_info->nand, DataLen , SpareLen, IsEccF, N1Len);

   	// Configure Ready-Timeout register
   	/*V_FC_RegsP->FC_TimeOut.RdyTimeOutEn = C_BitEnable;
  	V_FC_RegsP->FC_TimeOut.RdyTimeOutCnt = C_FlsRdyTimeout;*/
   	DW_FC_SET(nand_info->nand, DW_FC_TIMEOUT,
             DW_FC_TIMEOUT_RDY_EN, DW_FC_TIMEOUT_RDY_CNT(C_FlsRdyTimeout),
             0,0,0,0,0,0,0,0);

   	// Configure Flash-Controller to Disable DMA/FIFO Bypass
   	//F_DisableBypassMode();
  	writel(!DW_FC_FBYP_CTL_BP_EN, (nand_info->nand + DW_FC_FBYP_CTL));

	return ret_value;
}
NFC_E_Err  dw_nand_CompleteWritePageAndSpare(struct dw_nand_info *nand_info )
{
	NFC_E_Err ret_value = NFC_K_Err_OK;
  	__u16                 FlashData;
	//udelay(2000);
	/*! TRIGGER DMA controller */
	DMF_F_TrigDma(&nand_info->dma_regs);

	writel(SET_32BIT, (nand_info->nand + DW_FC_STS_CLR));
	NFLC_CAPTURE();
   	/*! TRIGGER Flash controller */
   	writel(DW_FC_CTL_ECC_OP_MODE(DW_FC_MEM_TRANS), (nand_info->nand + DW_FC_CTL));

   	/* NOTE: The Following section will be replaced by interrupt handling
   	**       ISR, which will send an event to a waiting task.
   	**       This task will fix ECC errors, then call a call-back function
   	**       of the registered user - This is for ASync I/O
   	*/

   	/*! POLL for transaction end */
   	//while ((V_FC_RegsP->FC_Stat.TransDone == 0) || (DMF_F_IsXferDone() == 0));
   	while(((readl(nand_info->nand + DW_FC_STATUS) & DW_FC_STATUS_TRANS_DONE) == 0)
 //      || (readl(__io_address(DW_DMA_BASE) + DW_DMA_ISR_DMA_DONE))); // R.B. 19/05/2008
		 || (DMF_F_IsXferDone(&nand_info->dma_regs) == 0))
		;
   	/*! CHECK if Timeout was marked (chip blocked) */
   	if(readl(nand_info->nand + DW_FC_STATUS) & DW_FC_STATUS_RDY_TIMEOUT)
   	{
//      	return (NFC_K_Err_TimeOut);
		ret_value = NFC_K_Err_TimeOut;
		goto out;
   	}


	if ((readl(nand_info->nand + DW_FC_STATUS) & DW_FC_STATUS_RDY1_STAT) == 0) {
		DW_SPOT("We have a problem");
	}

	// Due to HW bug the DW_FC_STATUS_TRANS_DONE bit reports that the transaction is done while
	//   the transaction is still on going. Using debug register to overcome the obstacle.
  	FlashData = M_ReadStatus(nand_info);

   	if ((FlashData & 0x001) != 0)
  	{
		ret_value = NFC_K_Err_ProgFail;
		goto out;
   	}

	out:
   	up(&unified_dw_nflc_mutex);
	return ret_value;

} /* end F_CmdWritePageAndSpare() */



/**************************************************************************
* Function Name:  F_SetDataCount
*
* Description:    Returns value for setting the Data Count register of the
*                 Flash Controller, according to the Flash chip type and the
*                 ECC Activation / Deactivation
*
* Input:          PageSize:   Physical Data page size (not including Spare bytes).
*                 Spare Size: Physical Spare area Size.
*                 IsEccF:     Whether to activate ECC machine.
*                 N1Len:      Ecc N1 filed length.
*
* Output:         None.
*
* Return Value:   The value to set the Data Count register.
***************************************************************************/
/*static*/ void  F_SetDataCount( void * nand_base_reg,__u16  PageSize, __u16  SpareSize,
                             int    IsEccF,   __u8   N1Len )
{
   if (IsEccF == 0)
   {
      // ECC DeActivated
      /*FC_DcountImg.VirtPageCnt = 0;
      FC_DcountImg.SpareN1Cnt = 0;
      FC_DcountImg.SpareN2Cnt = SpareSize;
      FC_DcountImg.VirtMainCnt = PageSize;*/
      DW_FC_SET(nand_base_reg, DW_FC_DCOUNT,
                DW_FC_DCOUNT_MAIN_CNT(PageSize),
                DW_FC_DCOUNT_SPARE_N2(SpareSize),
                DW_FC_DCOUNT_SPARE_N1(0), DW_FC_DCOUNT_PAGE_CNT(0),
                0,0,0,0,0,0);
   }
   else
   {
      // ECC Active
      /*FC_DcountImg.VirtPageCnt = (PageSize >> 9) - 1 ; // (Net-Page Physical Size :512) - 1 !!!
      FC_DcountImg.SpareN1Cnt = N1Len;
      FC_DcountImg.SpareN2Cnt = C_VirtSpareSize - N1Len;
      FC_DcountImg.VirtMainCnt = C_VirtPageSize;*/
      DW_FC_SET(nand_base_reg, DW_FC_DCOUNT,
                DW_FC_DCOUNT_MAIN_CNT(C_VirtPageSize),
                DW_FC_DCOUNT_SPARE_N2(C_VirtSpareSize - N1Len),
                DW_FC_DCOUNT_SPARE_N1(N1Len), DW_FC_DCOUNT_PAGE_CNT((PageSize >> 9) - 1),
                0,0,0,0,0,0);
   }

} /* end F_SetDataCount() */

/**************************************************************************
* Function Name:  F_FixEccErrors
*
* Description:    Detects and corrects errors in Physical Page and Spare areas,
*                 using Hamming and BCH algorithms.
*                 The Physical Page/Spare areas are divided to Virtual Page/Spare
*                 sections (512/16 bytes). On each section, errors up to 4 bits
*                 can be detected and corrected.
*
* Input:          Chip:       The NAND-Flash chip type.
*                 DataP:      Pointer to net-Page Data (w/o the spare data), already read.
*                 SpareP:     Pointer to page's Spare Data, already read.
*
* Output:         None.
* Return Value:   None.
***************************************************************************/
/*static*/ void  F_FixEccErrors( struct dw_nand_info *nand_info,
                             void *      DataP,
                             void *      SpareP )
{
   __u32               EccStatImg;
   __u8                NumVirtPages;
   __u8                CurrVirtPage;
   __u8                HamErrStat;
   __u8                BchErrStat;
   __u8                NumBchErrsExpected;

   /*! TEST if ECC-HW-machine has found errors */
   //EccStatImg = *( (__u32 *)(&V_FC_RegsP->FC_EccStat) );  // get Ecc status register
   EccStatImg = (__u32)readl(nand_info->nand + DW_FC_ECC_STS);

   EccStatImg &= (__u32)0x00FF00FF;  // b23-b16: Hamming; b7-b0: BCH
   if (EccStatImg != 0x00000000)
   {
      /* ECC error(s) found ! */
//DW_SPOT("ERROR found. EccStat: %0#10x", (unsigned)EccStatImg);
      BchErrStat = (__u8)EccStatImg;
      HamErrStat = (__u8)(EccStatImg >> 16);

      NumVirtPages = (__u8)(nand_info->mtd->writesize >> 9); // divide by 512 - each Virtual Page has 512 Bytes. //FIXME: get this from upper layer

      /*! TREAT each Virtual Page independently */
      for (CurrVirtPage = 0; CurrVirtPage < NumVirtPages; CurrVirtPage++)
      {
         /*! TEST for BCH errors in current Virtual Page */
         if ( (BchErrStat & (0x01 << CurrVirtPage)) != 0x00 )
         {
            /* BCH error(s) found in 512b-VirtualPage and/or in Spare-N1
            ** and/or in 2b-Ecc.Hamming and/or in 7b-Ecc.Bch fields */

            /*! TEST if a single-bit error */
            if (F_IsSingleBitBchErr( nand_info->nand, CurrVirtPage ))
            {
               NumBchErrsExpected = F_BchDetect(nand_info->nand); // should be 1 error
               F_BchCorrect( nand_info, NumBchErrsExpected, CurrVirtPage, 6, DataP, SpareP );
#if 0
               /* Single-bit error found --> Use Hamming Code to correct */
//DW_SPOT("Single-bit error found --> Use Hamming Code to correct");
               /*! TEST for Hamming errors */
               if ((HamErrStat & (0x01 << CurrVirtPage)) != 0x00)
               {
                  /* Hamming error found in 512b-VirtualPage or and/or in Spare-N1 or in 2b-Ecc.Hamming */
                  /*! FIX single-bit error */
                  F_HammCorrect( nand_info->nand, CurrVirtPage, 6 , DataP, SpareP );
                  nand_info->mtd->ecc_stats.corrected++;
		  // UDI to check 6 previously was FlsParmsP->Spare_N1_count
               }
               else
               {
//DW_SPOT("Hamming error Not found: Error must be in 7b-Ecc.Bch field --> Not covered by Hamming");
                  /* Hamming error Not found: Error must be in 7b-Ecc.Bch field --> Not covered by Hamming */
                  /*! DO Nothing */
               }
#endif
            }
            else
            {
               /* Multiple bit errors found (Hamming Code is useless) in
               ** 512b-VirtualPage and/or in Spare-N1 and/or in 2b-Ecc.Hamming and/or 7b-Ecc.Bch */
               /*! FIX multiple-bit errors */
//DW_SPOT("Multiple bit errors found (Hamming Code is useless)");
               NumBchErrsExpected = F_BchDetect(nand_info->nand); // should be 2, 3 or 4 errors
               F_BchCorrect( nand_info, NumBchErrsExpected, CurrVirtPage, 6, DataP, SpareP );
	       // UDI to check 6 previously was FlsParmsP->Spare_N1_count


               /* TBD */
            }
         } /* end_if BCH error(s) found in current Virtual Page */

      } /* end_for CurrVirtPage */

   } /* end_if Ecc error(s) found */

} /* end F_FixEccErrors() */


/**************************************************************************
* Function Name:  F_HammCorrect
*
* Description:    Correct single-bit error using Hamming Code.
*                 To be invoked right after a Page Read, in case
*                 there is a Single(!) error (as defined by BCH algorithm).
*
* Input:          VirtPage:     Number of Virtual Page (within Physical Page): 0 to 7.
*                 N1Size:       N1 (HW Protected) filed size: 0, 2, 4 or 6 (bytes).
*
*  Output:        DataP:        Pointer to already read Physical Page (net) data - To be error-corrected.
*                 SpareP:       Pointer to already read Physical Spare data - To be error-corrected.
*
* Return Value:   None.
***************************************************************************/
/*static*/ void  F_HammCorrect( void * nand_base_reg, __u8   VirtPage,
                            __u8   N1Size,
                            void *  DataP,
                            void *  SpareP )
{
   __u16      HammVec;
   __u16      TmpVec;
   __u16      MatrixSize;
   __u16      BitLoc;
   __u16      i;
   __u16      LocByteOffset;
   __u16      LocBitOffset;


   /*! GET Hamming Code vector */
   //HammVec = V_FC_RegsP->FC_Hamming.HammCodeOut & 0x1FFF; // b12 - b0 : 13bit Hamming Code
   HammVec = readl(nand_base_reg + DW_FC_HAMMING) & DW_FC_HAMMING_CODE_OUT;
   switch (N1Size)
   {
      case 0: MatrixSize = C_MatSizeN1Is0; break;
      case 2: MatrixSize = C_MatSizeN1Is2; break;
      case 4: MatrixSize = C_MatSizeN1Is4; break;
      case 6: MatrixSize = C_MatSizeN1Is6; break;
      default:
         /*! LOG Error */
         printk(KERN_CRIT "Error: please consult DSP Group - romanb@dsp.co.il");
         return;
   } /* end_switch */

   /*! GET the 0-bit Position */
   if (HammVec & 0x0001)
   {
      /* MSBit = '1' */
      BitLoc = (HammVec == 0x0001) ? (MatrixSize - 1) : ((HammVec >> 1) - 8);
   }
   else
   {
      /* MSBit = '0' */
      TmpVec = HammVec >> 1;
      for (i = 0; i < (C_HammCodeLen - 1); i++)
      {
         if ( TmpVec == (0x0001 << ((C_HammCodeLen - 2) - i)) )
         {
            BitLoc = MatrixSize - (__u16)C_HammCodeLen + i;
            /*! Location found - SKIP rest */
            goto LABEL_LocFound;
         }
      } /* end_for */

      BitLoc = ((__u16)4088 + (0x0001 << (C_HammCodeLen - 1)) - 1) - TmpVec;
   } /* end_if MSBit = '0' */

LABEL_LocFound:

   /*! FIX single-bit error: Toggle bit */
   LocByteOffset = BitLoc >> 3;
   LocBitOffset = BitLoc & 0x0007;

//DW_SPOT("Single Error found: (Bitloc = %0#6x), byte %x, bit %x", (unsigned)BitLoc, (unsigned)LocByteOffset, (unsigned)LocBitOffset);
   if (LocByteOffset < C_VirtPageSize)
   {
      /* Error is in net-Page Data */
      // *( (__u8 *)DataP + (VirtPage * C_VirtPageSize) + LocByteOffset ) ^= (0x01 << LocBitOffset);
      F_CorrectSingleByte( (__u8 *)DataP + (VirtPage * C_VirtPageSize) + LocByteOffset,
                           LocBitOffset );
   }
   else
   {
      /* Error is in N1 field or in Hamming-Code field of the Spare area */
      // *( (__u8 *)SpareP + (VirtPage * C_VirtSpareSize) + (LocByteOffset - C_VirtPageSize) ) ^= (0x01 << LocBitOffset);
      F_CorrectSingleByte( (__u8 *)SpareP + (VirtPage * C_VirtSpareSize) + (LocByteOffset - C_VirtPageSize),
                           LocBitOffset );
   }

} /* end F_HammCorrect() */


/**************************************************************************
* Function Name:  F_IsSingleBitBchErr
*
* Description:    Retrieves BCH S1 - S5, S7 polynoms.
*                 Calculates T1 & T2 values from BCH Syndroms S1 - S5, S7.
*                 Determines if there is a Single(!) bit error or Multiple
*                 bit errors.
*
* Input:          VirtPage: The Virtual Page number (a 512bytes unit on the Phisical net Page)
*
* Output:         None.
*
* Return Value:   1: Single bit error; 0: Multiple bit errors.
*
* NOTE: The '*' and '+' operations are math operations above 2^13 Galois field.
*       '+' is 13bit XOR, '*' is using the GF2^13 HW multiplier.
***************************************************************************/
/*static*/ int  F_IsSingleBitBchErr( void * nand_base_reg, __u8  VirtPage )
{
   Y_FC_Ctrl   FC_CtrlImg;

   *((__u32 *)&FC_CtrlImg) = 0;

   /*! Clear relevant bits in Status register */
   writel(SET_32BIT, (nand_base_reg + DW_FC_STS_CLR));

   /*! SET machine to get the stored Hamming & BCH codes of the Virtual Page */
   /*FC_CtrlImg.ChainCntStart = 0;
   FC_CtrlImg.PageSelect = VirtPage;
   FC_CtrlImg.EccOpMode = C_FC_Ctrl_EccOpMode_RdErrCodes;*/  // Read_Err_Codes transaction.
   /*! TRIGGER Flash controller */
   //V_FC_RegsP->FC_Ctrl = FC_CtrlImg;
   writel(SET_32BIT, (nand_base_reg + DW_FC_STS_CLR));
NFLC_CAPTURE();
   DW_FC_SET(nand_base_reg, DW_FC_CTL,
             DW_FC_CTL_ECC_OP_MODE(DW_FC_READ_ERR_CODE),
             DW_FC_CTL_PAGE_SELECT(VirtPage),
             DW_FC_CTL_CHAIN_CNT_START(0),
             0,0,0,0,0,0,0);

   /*! POLL for ECC-Busy End */
//   while (V_FC_RegsP->FC_Stat.EccBysy != 0);
   while((readl(nand_base_reg + DW_FC_STATUS) & DW_FC_STATUS_ECC_BUSY) != 0) // R.B. fix
		;

   /*! Clear relevant bits in Status register */
   writel(SET_32BIT, (nand_base_reg + DW_FC_STS_CLR));

   /*! SET machine to clculate BCH 'S' vectotrs of the Virtual Page */
   /*FC_CtrlImg.ChainCntStart = 0;
   FC_CtrlImg.PageSelect = VirtPage;
   FC_CtrlImg.EccOpMode = C_FC_Ctrl_EccOpMode_SCalc;*/  // S_Calculation transaction.
   /*! TRIGGER Flash controller */
   //V_FC_RegsP->FC_Ctrl = FC_CtrlImg;
   writel(SET_32BIT, (nand_base_reg + DW_FC_STS_CLR));
NFLC_CAPTURE();
   DW_FC_SET(nand_base_reg, DW_FC_CTL,
             DW_FC_CTL_ECC_OP_MODE(DW_FC_S_CALC),
             DW_FC_CTL_PAGE_SELECT(VirtPage),
             DW_FC_CTL_CHAIN_CNT_START(0),
             0,0,0,0,0,0,0);

   /*! POLL for ECC-Busy End */
//   while (V_FC_RegsP->FC_Stat.EccBysy != 0);  // 13 clocks
   while((readl(nand_base_reg + DW_FC_STATUS) & DW_FC_STATUS_ECC_BUSY) != 0) // R.B. fix
		;

   /*! GET S vectors */
   //V_S1 = V_FC_RegsP->FC_Ecc_s_12.S1Data & 0x1FFF;
   V_S1 = readl(nand_base_reg + DW_FC_ECC_S_12) & DW_FC_ECC_LOW;
   //V_S2 = V_FC_RegsP->FC_Ecc_s_12.S2Data & 0x1FFF;
   V_S2 = (readl(nand_base_reg + DW_FC_ECC_S_12) & DW_FC_ECC_HIGH) >> 16;
   //V_S3 = V_FC_RegsP->FC_Ecc_s_34.S3Data & 0x1FFF;
   V_S3 = readl(nand_base_reg + DW_FC_ECC_S_34) & DW_FC_ECC_LOW;
   //V_S4 = V_FC_RegsP->FC_Ecc_s_34.S4Data & 0x1FFF;
//   V_S4 = (readl(DW_NAND_LCD_BASE + DW_FC_ECC_S_12) & DW_FC_ECC_HIGH) >> 16; // R.B. BUG FIX
   V_S4 = (readl(nand_base_reg + DW_FC_ECC_S_34) & DW_FC_ECC_HIGH) >> 16;
   //V_S5 = V_FC_RegsP->FC_Ecc_s_57.S5Data & 0x1FFF;
   V_S5 = readl(nand_base_reg + DW_FC_ECC_S_57) & DW_FC_ECC_LOW;
   //V_S7 = V_FC_RegsP->FC_Ecc_s_57.S7Data & 0x1FFF;
   V_S7 = (readl(nand_base_reg + DW_FC_ECC_S_57) & DW_FC_ECC_HIGH) >> 16;

   // T1 = S1 * S2 + S3
   V_T1 = F_GfMultPolynom(nand_base_reg, V_S1, V_S2);
   V_T1 ^= V_S3;
   V_T1 &= 0x1FFF;

   // T2 = S1 * S4 + S5
   V_T2 = F_GfMultPolynom(nand_base_reg, V_S1, V_S4);
   V_T2 ^= V_S5;
   V_T2 &= 0x1FFF;

   if ((V_T1 == 0x0000) && (V_T2 == 0x0000))
   {
      /* Single bit error */
      return (1);
   }

   /* Multiple bit error */
   return (0);

} /* end F_IsSingleBitBchErr() */


/**************************************************************************
* Function Name:  F_BchDetect
*
* Description:    Determines how many bit errors detected on a Virtual Page,
*                 by calculating BCH polynoms. Calculates L1 - L4 vectors
*                 for calculating the error location(s).
*
* Input:          None.
* Output:         None.
*
* Return Value:   Number of bit errors: 2, 3 or 4.
*
* NOTE: This function shoulb be invoked only when there are more than a
*        single-bit error.
*
***************************************************************************/
/*static*/ __u8  F_BchDetect( void * nand_base_reg )
{
   __u8   NumErrors;
   __u16  tmpPoly1;
   __u16  tmpPoly2;
   __u16  tmpPoly3;
   __u16  tmpPoly4;
   __u16  tmpPoly5;

//DW_SPOT("V_S3 %hu, V_T1 %hu, V_S1 %hu, V_T2 %hu", V_S3, V_T1, V_S1, V_T2);
   // D = S3 * T1 + S1 * T2
   tmpPoly1 = F_GfMultPolynom(nand_base_reg, V_S3, V_T1);
   tmpPoly2 = F_GfMultPolynom(nand_base_reg, V_S1, V_T2);
   V_D = (tmpPoly1 ^ tmpPoly2) & 0x1FFF;

   if ((V_D == 0) && (V_S1 != 0) && (V_S2 != 0))
   {
      /*! SET L1 to L4 */
      // L1 = S1
      V_L1 = V_S1;
      // L2 = T1 * 1/S1
      tmpPoly1 = F_ReciprocalPolynom(V_S1);
      V_L2 = F_GfMultPolynom(nand_base_reg, V_T1, tmpPoly1);
      // L3 = 0
      V_L3 = 0;
      // L4 = 0
      V_L4 = 0;

      NumErrors = (V_L2 == 0) ? 1 : 2;
   }
   else
   {
      /* Expect 3 OR 4 errors */

      // T3 = S1 * S2 * S4 + S7
      tmpPoly1 = F_GfMultPolynom(nand_base_reg, V_S1, V_S2);
      tmpPoly2 = F_GfMultPolynom(nand_base_reg, tmpPoly1, V_S4);
      V_T3 = (tmpPoly2 ^ V_S7) & 0x1FFF;
      // T4 = S1 * T3 + S3 * T2
      tmpPoly1 = F_GfMultPolynom(nand_base_reg, V_S1, V_T3);
      tmpPoly2 = F_GfMultPolynom(nand_base_reg, V_S3, V_T2);
      V_T4 = (tmpPoly1 ^ tmpPoly2) & 0x1FFF;
      // T5 = T4 * 1/D
      tmpPoly1 = F_ReciprocalPolynom(V_D);
      V_T5 = F_GfMultPolynom(nand_base_reg, V_T4, tmpPoly1);
      // T6 = T1 + S1 * T5
      tmpPoly1 = F_GfMultPolynom(nand_base_reg, V_S1, V_T5);
      V_T6 = (V_T1 ^ tmpPoly1) & 0x1FFF;
      // T7 = (S5 + S2 * S3 + T1 * T5) * 1/S1
      tmpPoly1 = F_GfMultPolynom(nand_base_reg, V_S2, V_S3);
      tmpPoly2 = F_GfMultPolynom(nand_base_reg, V_T1, V_T5);
      tmpPoly3 = (V_S5 ^ tmpPoly1) & 0x1FFF;
      tmpPoly4 = (tmpPoly3 ^ tmpPoly2) & 0x1FFF;
      tmpPoly5 = F_ReciprocalPolynom(V_S1);
      V_T7 = F_GfMultPolynom(nand_base_reg, tmpPoly4, tmpPoly5);

      /*! SET L1 to L4 */
      // L1 = S1
      V_L1 = V_S1;
      // L2 = T5
      V_L2 = V_T5;
      // L3 = T6
      V_L3 = V_T6;
      // L4 = T7
      V_L4 = V_T7;

      NumErrors = (V_L4 == 0) ? 3 : 4;
   }

//DW_SPOT("NumErrors = %u", (unsigned int)NumErrors);
   return (NumErrors);

} /* end F_BchDetect() */


/**************************************************************************
* Function Name:  F_BchCorrect
*
* Description:    Performs chain-search to locate the BCH errors bit positions.
*                 Corrects up to 4 errors.
*
* Input:          NumBchErrsExpctd: Number of errors (previously detected by BCH).
*                 VirtPage:         The Virtual Page number (a 512bytes unit on the Phisical net Page).
*                 N1Size:           Size of N1 field, in bytes (0,2,4 or 6).
*
* Output:         DataP:   Pointer to net-Page Data (w/o the spare data), already read.
*                 SpareP:  Pointer to page's Spare Data, already read.
*
* Return Value:   None.
***************************************************************************/
/*static*/ void  F_BchCorrect( struct dw_nand_info *nand_info, __u8   NumBchErrsExpctd,
                           __u8   VirtPage,
                           __u8   N1Size,
                           void *  DataP,
                           void *  SpareP )
{
   __u16      S1_Load;
   __u16      S2_Load;
   __u16      S3_Load;
   __u16      S4_Load;
   __u16      BitLoc;
   __u16      LocByteOffset;
   __u16      LocBitOffset;
   int       IsChainStart;
   __u8       BchParmsIx;
//   Y_FC_Ctrl   FC_CtrlImg;
   const Y_BchChainParms *  BchChainParmsP;


//   *((__u32 *)&FC_CtrlImg) = 0;
//DW_SPOT_FUN("NumBchErrsExpctd %2d, VirtPage %2d, N1Size %2d", (int)NumBchErrsExpctd, (int)VirtPage, (int)N1Size);
   /* BCH should have found 2, 3 or 4 errors (One error has been treated previously) */
   if (NumBchErrsExpctd > 4)
   {
	  BUG();
      /* LOG HW/SW error */
      return;
   }
	// TODO: This would only work with the only OOB placement scheme: 6(BadBlockMark) + 9(ECC) + 1(OOB)
   if (  ((unsigned int *)SpareP)[0] == 0xffffffff &&
      	 ((unsigned int *)SpareP)[1] == 0xffffffff &&
      	 ((unsigned int *)SpareP)[2] == 0xffffffff &&
      	(((unsigned int *)SpareP)[3] &  0x00ffffff) == 0x00ffffff) {
      		return ; // Do nothing - this is fresh page!
   }

   BchParmsIx = N1Size >> 1; // 0,2,4,6 --> 0,1,2,3
   BchChainParmsP = &V_BchChainParms[ BchParmsIx ];

   /*! CALCULATE 'S' registers Load value */
   S1_Load = F_GfMultPolynom( nand_info->nand, BchChainParmsP->K1PolyVal, V_L1 ) & 0x1FFF;
   S2_Load = F_GfMultPolynom( nand_info->nand, BchChainParmsP->K2PolyVal, V_L2 ) & 0x1FFF;
   S3_Load = F_GfMultPolynom( nand_info->nand, BchChainParmsP->K3PolyVal, V_L3 ) & 0x1FFF;
   S4_Load = F_GfMultPolynom( nand_info->nand, BchChainParmsP->K4PolyVal, V_L4 ) & 0x1FFF;

   /*! PREPARE Chain-Search transaction on Virtual Page */
   //V_FC_RegsP->FC_Ecc_s_12.S1Data = S1_Load;
   //V_FC_RegsP->FC_Ecc_s_12.S2Data = S2_Load;
   DW_FC_SET(nand_info->nand, DW_FC_ECC_S_12,
             DW_FC_ECC_S1(S1_Load), DW_FC_ECC_S2(S2_Load),
             0,0,0,0,0,0,0,0);
  
   //V_FC_RegsP->FC_Ecc_s_34.S3Data = S3_Load;
   //V_FC_RegsP->FC_Ecc_s_34.S4Data = S4_Load;
   DW_FC_SET(nand_info->nand, DW_FC_ECC_S_34,
             DW_FC_ECC_S3(S3_Load), DW_FC_ECC_S4(S4_Load),
             0,0,0,0,0,0,0,0);

   // Clear relevant bits in Status register
   writel(SET_32BIT, (nand_info->nand + DW_FC_STS_CLR));

   IsChainStart = 1;
   while (NumBchErrsExpctd-- > 0)
   {
//      FC_CtrlImg.PageSelect = VirtPage;
     // Clear relevant bits in Status register
      writel(SET_32BIT, (nand_info->nand + DW_FC_STS_CLR)); // Moved from below
      if (IsChainStart != 0)
      {
         /*! SET for chain-search Start */
         IsChainStart = 0;
         //FC_CtrlImg.ChainCntStart = BchChainParmsP->ChainCountStartVal;
         //FC_CtrlImg.EccOpMode = C_FC_Ctrl_EccOpMode_ChainSrchStrt;  // Chain-Search Start
   writel(SET_32BIT, (nand_info->nand + DW_FC_STS_CLR));
NFLC_CAPTURE();
         DW_FC_SET(nand_info->nand, DW_FC_CTL,
                   DW_FC_CTL_ECC_OP_MODE(DW_FC_CHAIN_SRCH),
                   DW_FC_CTL_PAGE_SELECT(VirtPage),
                   DW_FC_CTL_CHAIN_CNT_START(BchChainParmsP->ChainCountStartVal),
                   0,0,0,0,0,0,0);
      }
      else
      {
         /*! SET for chain-search Continue */
         //FC_CtrlImg.EccOpMode = C_FC_Ctrl_EccOpMode_ChainSrchCont;  // Chain-Search Continue
/* R.B. Bug fix
         DW_FC_SET(DW_NAND_LCD_BASE, DW_FC_CTL,
                   DW_FC_CTL_ECC_OP_MODE(DW_FC_CHAIN_SRCH_CONT),
                   DW_FC_CTL_PAGE_SELECT(VirtPage),
                   DW_FC_CTL_CHAIN_CNT_START(readl(DW_NAND_LCD_BASE + DW_FC_CTL_CHAIN_CNT_START_LOC)),
                   0,0,0,0,0,0,0);
*/
   writel(SET_32BIT, (nand_info->nand + DW_FC_STS_CLR));
NFLC_CAPTURE();
         DW_FC_SET(nand_info->nand, DW_FC_CTL,
                   DW_FC_CTL_ECC_OP_MODE(DW_FC_CHAIN_SRCH_CONT),
                   DW_FC_CTL_PAGE_SELECT(VirtPage),
                   DW_FC_CTL_CHAIN_CNT_START(0),
                   0,0,0,0,0,0,0);

      }
     // Clear relevant bits in Status register
//      writel(SET_32BIT, (DW_NAND_LCD_BASE + DW_FC_STS_CLR)); // Moved up

      /*! TRIGGER Flash controller */
      //V_FC_RegsP->FC_Ctrl = FC_CtrlImg;


      /*! POLL for ECC-Busy OR Chain-Search ERR-Found */
      //while ( (V_FC_RegsP->FC_Stat.EccBysy != 0) && (V_FC_RegsP->FC_Stat.ChainSrcErrFnd == 0) );
      while ((readl(nand_info->nand + DW_FC_STATUS) & DW_FC_STATUS_ECC_BUSY)
            && ((readl(nand_info->nand + DW_FC_STATUS) & DW_FC_STATUS_ERR_FOUND) == 0));

      //if (V_FC_RegsP->FC_Stat.ChainSrcErrFnd != 0)
      if (readl(nand_info->nand + DW_FC_STATUS) & DW_FC_STATUS_ERR_FOUND)
      {
		 unsigned int Ix = (readl(nand_info->nand + DW_FC_STATUS) & DW_FC_STATUS_ERR_LOC_MASK) >> 16;
         /*! FIX Error */
//         BitLoc = BchChainParmsP->ChainCountStartVal - V_FC_RegsP->FC_Stat.ChainSrchErrIx; // R.B. Fix
         BitLoc = BchChainParmsP->ChainCountStartVal - Ix;

         /* NOTE: Due to HW implemetation, BitLoc here is off-by-1. This is UnDocumented !!! */
         if (BitLoc > 0) --BitLoc;

         LocByteOffset = BitLoc >> 3;
         LocBitOffset = BitLoc & 0x0007;

//DW_SPOT("Error found: (Bitloc = %0#6x), byte %x, bit %x", (unsigned)BitLoc, (unsigned)LocByteOffset, (unsigned)LocBitOffset);

         if (LocByteOffset < C_VirtPageSize)
         {
/*
DW_SPOT("DataP %0#10x, LocByteOffset %4hu, (VirtPage * C_VirtPageSize) %4hu, ", 
	(unsigned int)DataP, LocByteOffset, (unsigned short)(VirtPage * C_VirtPageSize));
*/
            /* Error is in net-Page Data */
            // *( (__u8 *)DataP + (VirtPage * C_VirtPageSize) + LocByteOffset ) ^= (0x01 << LocBitOffset);
            F_CorrectSingleByte( (__u8 *)DataP + (VirtPage * C_VirtPageSize) + LocByteOffset,
                                 LocBitOffset );
         }
         else
         {
//DW_SPOT("LocByteOffset %hu", LocByteOffset);
            /* Error is in N1 field or in Hamming-Code field or in Bch field of the Spare area */
            // *( (__u8 *)SpareP + (VirtPage * C_VirtSpareSize) + (LocByteOffset - C_VirtPageSize) ) ^= (0x01 << LocBitOffset);
            F_CorrectSingleByte( (__u8 *)SpareP + (VirtPage * C_VirtSpareSize) + (LocByteOffset - C_VirtPageSize),
                                 LocBitOffset );
         }
         nand_info->mtd->ecc_stats.corrected++;
      } /* end_if chain-search found an error */
      else
      {
         nand_info->mtd->ecc_stats.failed++;
         break;
      }

   } /* end_while NumBchErrsExpctd */

   /*! CLEAR machine from Chain-search mode */
   writel(SET_32BIT, (nand_info->nand + DW_FC_STS_CLR));
NFLC_CAPTURE();
   DW_FC_SET(nand_info->nand, DW_FC_CTL,
             DW_FC_CTL_ECC_OP_MODE(DW_FC_CHAIN_SRCH),
             DW_FC_CTL_PAGE_SELECT(VirtPage),
             DW_FC_CTL_CHAIN_CNT_START(0),
             0,0,0,0,0,0,0);
/*#endif*/
   //V_FC_RegsP->FC_Ctrl = FC_CtrlImg;
   //while (V_FC_RegsP->FC_Stat.EccBysy != 0);
   while (readl(nand_info->nand + DW_FC_STATUS) & DW_FC_STATUS_ECC_BUSY);

} /* end F_BchCorrect() */


/**************************************************************************
* Function Name:  F_CorrectSingleByte
*
* Description:    Swaps one bad bit within a Byte, where the access
*                 mode to the RAM is by __u32 only.
*                 This function works only for Little Endian !!!
*
* Input:          BadByteP:  Address of the defected Byte.
*                 BadBitLoc: Bad-Bit location within the defected Byte (0 - 7).
*
* Output:         None.
* Return Value:   None.
***************************************************************************/
/*static*/ void  F_CorrectSingleByte( __u8 *  BadByteP, __u16  BadBitLoc )
{
   __u8  *  TmpByteP;
   __u32 *  BadDWordP;
   __u32    BadDWordVal;
   __u32    BadBitMask;


   if (BadByteP == NULL)
   {
      /*! EXIT Badly */
      return;
   }
DW_SPOT("Fixing one bit");
   TmpByteP = BadByteP;
   BadBitMask = 0x00000001;

   switch ( (__u32)BadByteP & 0x00000003 )
   {
      case 0x03:
         TmpByteP--;
         BadBitMask <<= 8;
         /*! FALL through - no break ! */

      case 0x02:
         TmpByteP--;
         BadBitMask <<= 8;
         /*! FALL through - no break ! */

      case 0x01:
         TmpByteP--;
         BadBitMask <<= 8;
         /*! FALL through - no break ! */

      case 0x00:
         BadDWordP = (__u32 *)TmpByteP;
         BadDWordVal = *BadDWordP;
         BadDWordVal ^= (BadBitMask << BadBitLoc);
         *BadDWordP = BadDWordVal;
         break;
   } /* end_switch */

} /* end F_CorrectSingleByte() */


/**************************************************************************
* Function Name:  F_GfMultPolynom
*
* Description:    Muliplies 13bit Polynoms over Glois Field, using a HW multiplier.
*
* Input:          P1:  First polynom.
*                 P2:  Second polynom.
*
* Output:         None.
*
* Return Value:   The Product Polynom.
*
* Note: The product is over modulo 2^13 GLOIS ("GALUAH") mathematical Field.
***************************************************************************/
/*static*/ __u16  F_GfMultPolynom( void * nand_base_reg, __u16  P1, __u16  P2)
{
   __u16        prod;
   //Y_FC_GfMulIn  GfMulInImg;

   /*! SET GF 2^13 Multiplier */
   /*GfMulInImg.A_in = P1;
   GfMulInImg.B_in = P2;*/

   /*! TRIGGER Multiplier */
   //V_FC_RegsP->FC_GfMulIn = GfMulInImg;
   writel(DW_FC_GF_MUL_IN_A(P1) | DW_FC_GF_MUL_IN_B(P2),
          (nand_base_reg + DW_FC_GF_MUL_IN));
   /*! POLL for multiplier Ready */
   //while (V_FC_RegsP->FC_GfMulOut.MultBusy != 0);  // 13 clocks
//   while(readl(DW_NAND_LCD_BASE + DW_FC_GF_MUL_OUT_M_BUSY) >> 31); // R.B. Bug fixed
   while((readl(nand_base_reg + DW_FC_GF_MUL_OUT) & DW_FC_GF_MUL_OUT_M_BUSY) != 0);
   
   /*! GET product result */
   //prod = V_FC_RegsP->FC_GfMulOut.GfMultOut & 0x1FFF;
   prod = readl(nand_base_reg + DW_FC_GF_MUL_OUT) & DW_FC_GF_MUL_OUT_M_OUT;
//DW_SPOT("Multiplying %04x and %04x yields %04x", P1, P2, prod);
   return (prod);

} /* end F_GfMultPolynom() */


/**************************************************************************
* Function Name:  F_ReciprocalPolynom
*
* Description:    For a given "binary" Polynom P(x), finds the 1/P(x)
*                 Polynom.
*
* NOTE: Contrary to all other functions in this module,
*       These polynoms are over Binary fields, Not Galois field !!!
*
* Input:          None.
* Output:         None.
* Return Value:   None.
***************************************************************************/
/*static*/ __u16  F_ReciprocalPolynom( __u16  Poly )
{
   POL_Y_Polynom   PolRin;
   POL_Y_Polynom   PolMin;
   POL_Y_Polynom   PolInvrs;

   //POL_F_InitFromStr(&PolMin, "10 0000 0001 1011");
   POL_F_InitFromVal(&PolMin, 0x201B);

   POL_F_InitFromVal(&PolRin, Poly);
   POL_F_InverseWithMod(&PolRin, &PolMin, &PolInvrs);

   return (__u16)(PolInvrs.PolData);

} /* end F_ReciprocalPolynom() */


/**************************************************************************
* Function Name:  F_DebugDelay
*
* Description:    Wastes time, for Debugging only!
*
* Input:          NumCycles: Number of cycles to loop
* Output:         None.
* Return Value:   None.
***************************************************************************/
/*static*/ void  F_DebugDelay( __u32  NumCycles )
{
   __u32  i, j;

   for (i = 0; i < NumCycles; i++)
   {
      j += i;
   }
} /* end F_DebugDelay() */


/**************************************************************************
* Function Name:
*
* Description:
*
* Input:          None.
* Output:         None.
* Return Value:   None.
***************************************************************************/


/*==========================   End of File   ============================*/

