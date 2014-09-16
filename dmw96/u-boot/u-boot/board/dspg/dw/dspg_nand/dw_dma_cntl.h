/********************************************************************************
*                      D M A  &  F I F O   C O N T R O L L E R
*********************************************************************************
*
*   Title:           DMA and FIFO controller Internal declerations.
*
*   Filename:        $Workfile: $
*   SubSystem:       DM-56 / ARM 7
*   Authors:         Avi Miller
*   Latest update:   $Modtime: $
*   Created:         17 Mars, 2005.
*
*********************************************************************************
*  Description:   DM-56 / ARM7 domain DMA and FIFO controller.
*                 Used to transfer data between ARM bus (AHB2), through the FIFO,
*                 TO/FROM the NAND Flash or IDE (hard disk).
*                 Also used to transfer data to/from the MMC card, bypassing(!)
*                 the FIFO.
*
*********************************************************************************
*  $Header: $
*
*  Change history:
*  ---------------
*  $History: $
*
********************************************************************************/

#ifndef _DMA_FIFO_CONTROL_INTERNAL_H_
#define _DMA_FIFO_CONTROL_INTERNAL_H_


/*=======================  Comlilation Switches  =========================*/

/*========================  Include Files  ===============================*/
#include "dw_nfls_cntl_pub.h"
#include "dw_nfls_type_defs.h"

/*========   Exported Const, types and variables definitions    ==========*/

// Read/Write from/to AHB bus
typedef enum
{
   DMF_K_DmaDirReadAHB = 0,
   DMF_K_DmaDirWriteAHB
} DMF_E_DmaDir;

// AHB bus NVM Peripheral
typedef enum
{
   DMF_K_PeriphFlsCntl = 0,
   DMF_K_PeriphIde,
   DMF_K_PeriphMmc
} DMF_E_Periph;

// FIFO bus Width
typedef enum
{
   DMF_K_Width8bit = 0,
   DMF_K_Width16bit
} DMF_E_Width;


#define DW_FIFO_SET(startaddr,reg,a,b,c,d,e,f,g,h,i,j) ({__u32 __v=0;\
__v=(a|b|c|d|e|f|g|h|i|j);\
writel(__v, (startaddr + reg));\
/*printk("%X %X\n", reg, __v);*/ })

#define DW_FIFO_ADD(startaddr,reg,a,b,c,d,e,f,g,h,i,j) ({__u32 __v=0;\
__v=readl(startaddr + reg);\
__v|=(a|b|c|d|e|f|g|h|i|j);\
writel(__v, (startaddr + reg));\

#define DW_DMA_SET(startaddr,reg,a,b,c,d,e,f,g,h,i,j) ({__u32 __v=0;\
__v=(a|b|c|d|e|f|g|h|i|j);\
writel(__v, (startaddr + reg));\
/*printk("%X %X\n", reg, __v);*/ })

#define DW_DMA_ADD(startaddr,reg,a,b,c,d,e,f,g,h,i,j) ({__u32 __v=0;\
__v=readl(startaddr + reg);\
__v|=(a|b|c|d|e|f|g|h|i|j);\
writel(__v, (startaddr + reg));\



// Added by Roman 04/06/2008, for the DMA ISR support
typedef enum
{
	DMF_K_ModeLCD = 0,
	DMF_K_ModeFlash
} DMF_E_Mode;

typedef struct 
{
	DMF_E_DmaDir   Direction;
        DMF_E_Periph   AhbPeriph;
        DMF_E_Width    FifoWidth;
        __u16         DataSize;
        __u16         SpareSize;
        __u32 *       DataP;
        __u32 *       SpareP;
} Y_DMATransConfig;

typedef struct
{
//	spinlock_t lock;
	DMF_E_Mode Mode;
	bool HasOngoingTransaction;
	Y_DMATransConfig TransConfig;
} Y_DMAConfig;
	


#define SET_DMACONFIG(var, dir, ahb, width, dsize, ssize, datap, sparep) \
	 do { \
		var.Direction = dir; \
		var.AhbPeriph = ahb; \
		var.FifoWidth = width; \
		var.DataSize = dsize; \
		var.SpareSize = ssize; \
		var.DataP = datap; \
		var.SpareP = sparep; \
	} while (0) \



extern Y_DMAConfig dw_DMA_config;



/**************************************************************************
*             P U B L I C    F U N C T I O N S
**************************************************************************/

/**************************************************************************
* Function Name:  DMF_F_ConfDmaFifo
*
* Description:    Configures the generic DMA controller, FIFO and GCR2
*
* Input:          Direction: Transfer direction, to/from AHB bus
*                 AhbPeriph: Peripheral: Flash-Controller or IDE or MMC card.
*                 FifoWidth: FIFO external bus width: 8 or 16 bits.
*                 DataSize:  Data size in bytes: Flash - Net Page; IDE - Block
*                 SpareSize: Flash - Spare size in bytes: IDE/MMC - Must(!) be 0.
*
* Output:         DataP:  Pointer to output Data buffer, in ARM addr space.
*                 SpareP: Pointer to output Spare buffer, in ARM addr space (NULL for IDE/MMC).
*
* Return Value:   None.
***************************************************************************/
extern void  DMF_F_ConfDmaFifo( struct dw_dma_params *dw_dma_param,
				DMF_E_DmaDir   Direction,
                                DMF_E_Periph   AhbPeriph,
                                DMF_E_Width    FifoWidth,
                                __u16         DataSize,
                                __u16         SpareSize,
                                __u32 *       DataP,
                                __u32 *       SpareP );


/**************************************************************************
* Function Name:  DMF_F_TrigDma
*
* Description:    Triggers a DMA transaction, usint the FIFO
*
* Input:          None.
* Output:         None.
* Return Value:   None.
***************************************************************************/
extern void  DMF_F_TrigDma(  struct dw_dma_params *dw_dma_param );


/**************************************************************************
* Function Name:  DMF_F_ConfDmaBypass
*
* Description:    Disables the DMA and FIFO modules, for Bypass read/write
*                 the Falsh device. Still configures GCR2 register, for
*                 setting the MUX'es selection.
*
* Input:          AhbPeriph: Flash, IDE or MMC
*
* Output:         None.
*
* Return Value:   None.
***************************************************************************/
extern void  DMF_F_ConfDmaBypass(  struct dw_dma_params *dw_dma_param, DMF_E_Periph  AhbPeriph );


/**************************************************************************
* Function Name:  DMF_F_IsXferDone
*
* Description:    Checks whether a DMA transfer has been completed.
*
* Input:          None.
* Output:         None.
*
* Return Value:   true:  Transaction is Done;
*                 false: Transaction has not ended yet.
***************************************************************************/
extern bool  DMF_F_IsXferDone(  struct dw_dma_params *dw_dma_param );

/*=======   Macro definitions   ==========================================*/

/*=======   Exported variables ===========================================*/


/*=======   Local const, types and variables =============================*/
// DMA
#define C_DMA_DmaMode_ReadAhb   0x0  // Read AHB, Write Fifo
#define C_DMA_DmaMode_WriteAhb  0x1  // Read Fifo, Write AHB

#define C_DMA_FlsMode_Disable   0x0  // Read flash Page without Spare
#define C_DMA_FlsMode_Enable    0x1  // Read flash Page with Spare

#define C_DMA_SwapMode_Disable  0x0

// FIFO
#define C_FIFO_FifoMode_ReadFls  0x0 // Read Flash
#define C_FIFO_FifoMode_WriteFls 0x1 // Write Flash

#define C_FIFO_FifoWidth8bit    0x0 // 8bit external bus
#define C_FIFO_FifoWidth16bit   0x1 // 16bit external bus

#define C_FIFO_LowWaterMark     0x8
#define C_FIFO_HighWaterMark    0x8

// GCR2
#define C_GCR2_PeriphMask            ((__u16)0x0300)
#define C_GCR2_DmaWithFifoIde        ((__u16)0x0000)
#define C_GCR2_DmaWithFifoFlsCntl    ((__u16)0x0100)
#define C_GCR2_DmaNoFifoMmc          ((__u16)0x0200)


/* Generic DMA Registers: */
/* ---------------------- */

// Control register
typedef struct
{
   unsigned int  DmaEn:     1; // bit   0;      R/W : '1' - DMA block enable
   unsigned int  DmaMode:   1; // bit   1;      R/W : '0' - Read AHB; '1'- Write AHB
   unsigned int  FlsMode:   1; // bit   2;      R/W : '0' - Flash-Mode (Spare) Disable; '1' - Enable
   unsigned int  Rsrv1:     1; // bit   3;      R   : Reserved
   unsigned int  EccLen:    6; // bits  4 -  9  R/W : Flash-Mode Spare data length; in 32-bit Words
   unsigned int  Rsrv2:     2; // bits 10 - 11  R   : Reserved
   unsigned int  SwapMode:  2; // bits 12 - 13  R/W : Swap Mode: '00' - no swap
   unsigned int  Rsrv3:     2; // bits 14 - 15  R   : Reserved
   // ------------------------
   unsigned int  Rsrv4:    16; // bits 16 - 31  R   : Reserved
} Y_DMA_Cntl;


// Address 1 register. Output for Page net read
typedef struct
{
   unsigned int  PageDataTrgt;  // R/W. Read Flash Page into this RAM address. 2 LSbits always read '0'.
} Y_DMA_Addr1;

// Address 2 register. Output for Pages's Spare read
typedef struct
{
   unsigned int  SpareDataTrgt; // R/W. Read Flash Spare into this RAM address. 2 LSbits always read '0'.
} Y_DMA_Addr2;

// Burst length register
typedef struct
{
   unsigned int  BurstLen:   7; // bits  0 -  6 R/W. Bits Burst size (in 32bit words) to Read/Write FIFO
   unsigned int  Rsrv1:      1; // bit   7      R    Reserved
   unsigned int  Rsrv2:      8; // bits  8 - 15 R    Reserved
   // -------------------------
   unsigned int  Rsrv3:     16; // bits 16 - 31 R    Reserved
} Y_DMA_Burst;

// Block length register
typedef struct
{
   unsigned int  BlkLen:    11; // bits  0 - 10 R/W. Trnasfer block Length (in 32bit words).
   unsigned int  Rsrv1:      5; // bits 11 - 15 R
   // -------------------------
   unsigned int  Rsrv2:     16; // bits 16 - 31 R
} Y_DMA_BlkLen;

// Block-count per transfer register
typedef struct
{
   unsigned int  NumBlks:    7; // bits  0 -  6 R/W.  Number of block in the DMA transfer; Max 128
   unsigned int  Rsrv1:      1; // bit   7      R     Reserved
   unsigned int  Rsrv2:      8; // bits  8 - 15 R     Reserved
   // -------------------------
   unsigned int  Rsrv3:     16; // bits 16 - 31 R     Reserved
} Y_DMA_BlkCnt;

// (Interrupt) Status register
typedef struct
{
   unsigned int  XferDoneStat:   1; // bit   0      R.  Transfer Finished status.
   unsigned int  AhbErrStat:     1; // bit   1      R.  Split/Retry AHB response status. Fatal error !
   unsigned int  Rsrv1:          6; // bits  2 -  7 R   Reserved
   unsigned int  Rsrv2:          8; // bits  2 -  7 R   Reserved
   // -----------------------------
   unsigned int  Rsrv3:         16; // bits 16 - 31 R   Reserved
} Y_DMA_Isr;

// Interrupt Enable mask register
typedef struct
{
   unsigned int  XferDoneIntrEn: 1; // bit   0      R/W.  Transfer Finished Interrupt Enable.
   unsigned int  AhbErrIntrEn:   1; // bit   1      R/W.  Split/Retry AHB response Interrupt Enable.
   unsigned int  Rsrv1:          6; // bits  2 -  7 R   Reserved
   unsigned int  Rsrv2:          8; // bits  2 -  7 R   Reserved
   // --------------  ---------------
   unsigned int  Rsrv3:         16; // bits 16 - 31 R   Reserved
} Y_DMA_Ier;

// Interrupt Clear mask register
typedef struct
{
   unsigned int  XferDoneIntrClr: 1; // bit   0      W.  Transfer Finished Interrupt Clear.
   unsigned int  AhbErrIntrClr:   1; // bit   1      W.  Split/Retry AHB response Interrupt Clear.
   unsigned int  Rsrv1:           6; // bits  2 -  7 R   Reserved
   unsigned int  Rsrv2:           8; // bits  2 -  7 R   Reserved
   // ------------------------------
   unsigned int  Rsrv3:          16; // bits 16 - 31 R   Reserved
} Y_DMA_Icr;

// Start (trigger) DMA-transfer register
typedef struct
{
   unsigned int  Trig:     1;  // bit   0       W. DMA transfer Trigger
   unsigned int  Rsrv1:   15;  // bits  1 - 15  R  Resreved
   // -----------------------
   unsigned int  Rsrv2:   16;  // bits 16 - 31  R  Reserved
} Y_DMA_Start;


// Register base:
typedef struct
{
   Y_DMA_Cntl     DMA_Cntl;     // Data Control
   Y_DMA_Addr1    DMA_Addr1;    // Address 1
   Y_DMA_Addr2    DMA_Addr2;    // Address 2
   Y_DMA_Burst    DMA_Burst;    // Burst Length
   Y_DMA_BlkLen   DMA_BlkLen;   // Tansfer block Length; in 32-bit Words
   Y_DMA_BlkCnt   DMA_BlkCnt;   // Transfer block count; max: 128 blocks.
   Y_DMA_Isr      DMA_Isr;      // Status
   Y_DMA_Ier      DMA_Ier;      // Interrupt Enable
   Y_DMA_Icr      DMA_Icr;      // Interrupt Clear
   Y_DMA_Start    DMA_Start;    // Start Trigger
} Y_DMA_Regs;

typedef enum
{
   K_EccModeDisable = 0,  // IDE or Flash-without-ECC
   K_EccModeEnable        // Flash with ECC (Page + Spare)
} E_EccMode;



/* Generic FIFO Registers: */
/* ----------------------- */

typedef struct
{
   unsigned int  FifoEn:     1;  // bit   0      R/W. FIFO enable
   unsigned int  FifoWidth:  1;  // bit   1      R/W. FIFO Ext. bus mode: '0'-8bit; '1'-16bit
   unsigned int  FifoMode:   1;  // bit   2      R/W. Fifo Dir.: '0' - flash->bus; '1' - bus->flash
   unsigned int  Rsrv1:      5;  // bits  3 -  7 R    Reserved
   unsigned int  Rsrv2:      8;  // bits  8 - 15 R    Reserved
   // -------------------------
   unsigned int  Rsrv3:     16;  // bits 16 - 31 R    Reserved
} Y_FIFO_cntl;

typedef struct
{
   unsigned int  LowWaterMark:   4;  // bits  0 -  3  R/W. Almost Empty level
   unsigned int  Rsrv1:          4;  // bits  3 -  7  R    Reserved
   unsigned int  Rsrv2:          8;  // bits  8 - 15 R     Reserved
   // -----------------------------
   unsigned int  Rsrv3:         16;  // bits 16 - 31 R    Reserved
} Y_FIFO_AeLvl;

typedef struct
{
   unsigned int  HighWaterMark:  4;  // bits  0 -  3  R/W. Almost Full level
   unsigned int  Rsrv1:          4;  // bits  3 -  7  R    Reserved
   unsigned int  Rsrv2:          8;  // bits  8 - 15 R     Reserved
   // -----------------------------
   unsigned int  Rsrv3:         16;  // bits 16 - 31 R    Reserved
} Y_FIFO_AfLvl;

typedef struct
{
   unsigned int  OverrunStat:    1;  // bit   0       R. Overrun status
   unsigned int  UnderrunStat:   1;  // bit   1       R. Underrun status
   unsigned int  Rsrv1:          6;  // bits  2 -  7  R  Reserved
   unsigned int  Rsrv2:          8;  // bits  8 - 15  R  Reserved
   // -----------------------------
   unsigned int  Rsrv3:         16;  // bits 16 - 31  R  Reserved
} Y_FIFO_Isr;

typedef struct
{
   unsigned int  OverrunIntrEn:  1;  // bit   0       R. Overrun interrupt Enable
   unsigned int  UnderrunIntrEn: 1;  // bit   1       R. Underrun interrupt Enable
   unsigned int  Rsrv1:          6;  // bits  2 -  7  R  Reserved
   unsigned int  Rsrv2:          8;  // bits  8 - 15  R  Reserved
   // -----------------------------
   unsigned int  Rsrv3:         16;  // bits 16 - 31  R  Reserved
} Y_FIFO_Ier;

typedef struct
{
   unsigned int  OverrunIntrClr:  1;  // bit   0       R. OverRun interrupt Clear
   unsigned int  UnderrunIntrClr: 1;  // bit   1       R. UnderRun interrupt Clear
   unsigned int  Rsrv1:           6;  // bits  2 -  7  R  Reserved
   unsigned int  Rsrv2:           8;  // bits  8 - 15  R  Reserved
   // ------------------------------
   unsigned int  Rsrv3:          16;  // bits 16 - 31  R  Reserved
} Y_FIFO_Icr;

typedef struct
{
   unsigned int  FifoReset:   1;  // bit   0       W. Fifo Reset clear all pointers & flags
   unsigned int  Rsrv1:       7;  // bits  1 -  7  R  Reserved
   unsigned int  Rsrv2:       8;  // bits  8 - 15  R  Reserved
   // --------------------------
   unsigned int  Rsrv3:      16;  // bits 16 - 31  R  Reserved
} Y_FIFO_Rst;

typedef struct
{
   unsigned int  FifoVal:     4;  // bits  0 -  3  R. Num words currently in Fifo, about to TX to AHB.
   unsigned int  Rsrv1:       4;  // bits  4 -  7  R  Reserved
   unsigned int  Rsrv2:       8;  // bits  8 - 15  R  Reserved
   // --------------------------
   unsigned int  Rsrv3:      16;  // bits 16 - 31  R  Reserved
} Y_FIFO_Val;

typedef struct
{
   unsigned int  FifoTstEn:   1;  // bit   0       W. Fifo RAM test Enable mode
   unsigned int  Rsrv1:       7;  // bits  1 -  7  R  Reserved
   unsigned int  Rsrv2:       8;  // bits  8 - 15  R  Reserved
   // --------------------------
   unsigned int  Rsrv3:      16;  // bits 16 - 31  R  Reserved
} Y_FIFO_TstEn;


typedef struct
{
   Y_FIFO_cntl    FIFO_cntl;          // Control
   Y_FIFO_AeLvl   FIFO_AeLvl;         // Almost-Empty level
   Y_FIFO_AfLvl   FIFO_AfLvl;         // Almost-Full level
   Y_FIFO_Isr     FIFO_Isr;           // (Interrupt) status
   Y_FIFO_Ier     FIFO_Ier;           // Interrupt Enable
   Y_FIFO_Icr     FIFO_Icr;           // Interrupt Clear
   Y_FIFO_Rst     FIFO_Rst;           // FIFO Reset
   Y_FIFO_Val     FIFO_Val;           // FIFO Value
   Y_FIFO_TstEn   FIFO_TstEn;         // Test Enable
   __u32         Dummy1;             // Address space
   __u32         Dummy2;             // Address space
   __u32         Dummy3;             // Address space
   __u32         Dummy4;             // Address space
   __u32         FIFO_TstAddr0;      // FIFO RAM test ADDR0
   __u32         FIFO_TstAddr1;      // FIFO RAM test ADDR1
   __u32         FIFO_TstAddr2;      // FIFO RAM test ADDR2
   __u32         FIFO_TstAddr3;      // FIFO RAM test ADDR3
   __u32         FIFO_TstAddr4;      // FIFO RAM test ADDR4
   __u32         FIFO_TstAddr5;      // FIFO RAM test ADDR5
   __u32         FIFO_TstAddr6;      // FIFO RAM test ADDR6
   __u32         FIFO_TstAddr7;      // FIFO RAM test ADDR7
   __u32         FIFO_TstAddr8;      // FIFO RAM test ADDR8
   __u32         FIFO_TstAddr9;      // FIFO RAM test ADDR9
   __u32         FIFO_TstAddr10;     // FIFO RAM test ADDR10
   __u32         FIFO_TstAddr11;     // FIFO RAM test ADDR11
   __u32         FIFO_TstAddr12;     // FIFO RAM test ADDR12
   __u32         FIFO_TstAddr13;     // FIFO RAM test ADDR13
   __u32         FIFO_TstAddr14;     // FIFO RAM test ADDR14
   __u32         FIFO_TstAddr15;     // FIFO RAM test ADDR15
} Y_FIFO_Regs;


#ifdef PC_SIMULATION

//static Y_FIFO_Regs    V_FIFO_Regs;
//static Y_FIFO_Regs *  V_FIFO_RegsP = &V_FIFO_Regs;

//static Y_DMA_Regs     V_DMA_Regs;
//static Y_DMA_Regs *   V_DMA_RegsP = &V_DMA_Regs;

//static __u16         V_GCR2Reg;
//static __u16 *       V_GCR2RegP = &V_GCR2Reg;

#else // PC_SIMULATION

// Flash-Controller registers-array Pointer
//static volatile Y_FIFO_Regs *  V_FIFO_RegsP = (Y_FIFO_Regs *)C_GenFifoAddr;
//static volatile Y_DMA_Regs *   V_DMA_RegsP = (Y_DMA_Regs *)C_GenDmaAddr;
//static volatile __u16 *       V_GCR2RegP = (__u16 *)C_GenCtrlGCR2Addr; // R.B. 18/05/2008

#endif // ! PC_SIMULATION




#endif /* _DMA_FIFO_CONTROL_INTERNAL_H_ */

/*============================   End of File   ==========================*/
