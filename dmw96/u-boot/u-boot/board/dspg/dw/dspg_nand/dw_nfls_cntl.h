/********************************************************************************
*              N A N D   F L A S H   C O N T R O L L E R
*********************************************************************************
*
*   Title:           Low-Level NAND-Flash controller Internal declerations.
*
*   Filename:        $Workfile: $
*   SubSystem:       DM-56/ ARM 7
*   Authors:         Avi Miller
*   Latest update:   $Modtime: $
*   Created:         13 Mars, 2005.
*
*********************************************************************************
*  Description:      Low-Level NAND-Flash controller, in the DM-56 chip,
*                    Running on ARM 7. Provides internal functionality of the
*                    build-in flash Controller.
*********************************************************************************
*  $Header: $
*
*  Change history:
*  ---------------
*  $History: $
*
********************************************************************************/

#ifndef _NAND_FLASH_CONTROL_H_
#define _NAND_FLASH_CONTROL_H_


/*=======   Compilation switces  =========================================*/


/*========================  Include Files  ===============================*/
#include "dw_nfls_cntl_pub.h"

/*========   Exported Const, types and variables definitions    ==========*/

#ifdef DM56_SIM_ON_FPGA
// NOTE: RAM on FPGA is 32KBytes, starting at 0x14000000:
#define C_IntrnlSRamAddr ((__u32)(NFC_C_BaseAddrArmView + 0x14000000))
#endif

/* NAND Flash chip Access Mode: 8bit / 16bit width */
typedef enum
{
   K_FlsAccess_8bit = 0,
   K_FlsAccess_16bit = 1
} E_FlsAccess;

/* NAND Flash chip Parameters */
typedef struct
{
   NFC_E_Chip    Chip;              // Chip type index.
   __u8          MakerCode;         // Manufacture Code.
   __u8          DeviceId;          // Device ID.
   __u16         MaxBadBlks;        // Max allowed number of Bad Blocks.
   E_FlsAccess   AccMode;           // Access mode: 8bit or 16bit.
   __u8          BadBlkMarkPos;     // Position of Bad-Block Mark, in Spare area.
   __u8          Spare_N1_count;    // Size in Bytes of N1 field in the Spare area.
   __u16         PageSize;          // Page size in Bytes/Words, Not(!) including the Spare Area.
   __u16         SpareSize;         // Spare size in Bytes/Words.
   __u16         NumPagesInBlk;     // Number of Pages per Block.
   __u16         NumBlocks;         // Number of Blocks (Block - the smalleset unit to erase).
   __u32         NumTotPages;       // Total number of Pages, throughout all Blocks.
} Y_NFlsParms;

/* NAND Flash chips Database */
//extern const Y_NFlsParms  V_NFlsParms[ NFC_K_Chip_MAX ];

/*=======   Macro definitions   ==========================================*/


/*=======   Exported variables ===========================================*/


/*=======   Local const, types and variables =============================*/

#define C_BitEnable   0x1
#define C_BitDisable  0x0
#define C_BitClear    0x1

// Common Flash chip Commands:
#define C_FlsCmdReadStat            ((__u8)0x70)
#define C_FlsCmdReadId              ((__u8)0x90)
#define C_FlsCmdReset               ((__u8)0xFF)
#define C_FlsCmdPageProg1Cyc        ((__u8)0x80)
#define C_FlsCmdPageProg2Cyc        ((__u8)0x10)
#define C_FlsCmdBlkErase1Cyc        ((__u8)0x60)
#define C_FlsCmdBlkErase2Cyc        ((__u8)0xD0)

// SAMSUNG K9F5608, K9F5616 (32 MByte), TOSHIBA TC58DVG02A1FT00:
// ------------------------------------
#define C_SamK9F56_FlsCmdRead                 ((__u8)0x00)
#define C_SamK9F56_FlsCmdReadStrtLowHalf      ((__u8)0x00)
#define C_SamK9F56_FlsCmdReadStrtHighHalf     ((__u8)0x01)
#define C_SamK9F56_FlsCmdReadSpare            ((__u8)0x50)

// SAMSUNG K9F2G08, K9F2G16 (256 MByte), K9K4G08 (512 MByte):
// SAMSUNG K9F1G08, K9F1G16 (128 Mbyte):
// TOSHIBA TH58NVG3D4BTGI0
// ---------------------------------------------------------
#define C_SamK9F2G_FlsCmdRead1Cyc             ((__u8)0x00)
#define C_SamK9F2G_FlsCmdRead2Cyc             ((__u8)0x30)


// Error correction constants
#define  C_VirtSpareSize    16
#define  C_VirtPageSize    512

#define  C_MatSizeN1Is0    ((__u16)4109)
#define  C_MatSizeN1Is2    ((__u16)4125)
#define  C_MatSizeN1Is4    ((__u16)4141)
#define  C_MatSizeN1Is6    ((__u16)4147)
#define  C_HammCodeLen      13
#define  C_MaxBchErrors      4

typedef struct
{
   __u16  ChainCountStartVal;
   __u16  K1PolyVal;
   __u16  K2PolyVal;
   __u16  K3PolyVal;
   __u16  K4PolyVal;
} Y_BchChainParms;

/* Flash-Controller Registers: */
/* --------------------------- */

// Control register:
typedef struct
{
   unsigned int  EccOpMode:      4;    // bits  3 -  0;  R/W
   unsigned int  PageSelect:     4;    // bits  4 -  7;  R/W
   unsigned int  Rsrv1     :     8;    // bits  8 - 15;
   // -----------------------------
   unsigned int  ChainCntStart: 13;    // bits 16 - 28;  W
   unsigned int  Rsrv2:          3;    // bits 29 - 31;
} Y_FC_Ctrl;

#define C_FC_Ctrl_EccOpMode_Disabled        0x0   // Flash Module Disabled
#define C_FC_Ctrl_EccOpMode_MemTrans        0x1   // Memory Transaction
#define C_FC_Ctrl_EccOpMode_SCalc           0x2   // 'S' Calculation
#define C_FC_Ctrl_EccOpMode_ChainSrchStrt   0x3   // Chain-Search Start
#define C_FC_Ctrl_EccOpMode_ChainSrchCont   0x4   // Chain-Search Continue
#define C_FC_Ctrl_EccOpMode_RdErrCodes      0x8   // Read Error Codes
#define C_FC_Ctrl_EccOpMode_WrErrCodes      0x9   // Write Error Codes

// Time values in CLK Cycles
#ifdef CONFIG_MTD_NAND_DW_CONFIGURE_TIMING
	   #define C_FlsWaitRdy1Fall  CONFIG_MTD_NAND_DW_C_FlsWaitRdy1Fall
	   #define C_FlsWaitRdy1Rise  CONFIG_MTD_NAND_DW_C_FlsWaitRdy1Rise
	   #define C_FlsWaitRdy2Fall  CONFIG_MTD_NAND_DW_C_FlsWaitRdy2Fall
	   #define C_FlsWaitRdy2Rise  CONFIG_MTD_NAND_DW_C_FlsWaitRdy2Rise
	   #define C_FlsWaitRdy3Fall  CONFIG_MTD_NAND_DW_C_FlsWaitRdy3Fall
	   #define C_FlsWaitRdy3Rise  CONFIG_MTD_NAND_DW_C_FlsWaitRdy3Rise
	   #define C_FlsWaitRdy4Fall  CONFIG_MTD_NAND_DW_C_FlsWaitRdy4Fall
	   #define C_FlsWaitRdy4Rise  CONFIG_MTD_NAND_DW_C_FlsWaitRdy4Rise

	   #define C_FlsPulseCleStup  CONFIG_MTD_NAND_DW_C_FlsPulseCleStup
	   #define C_FlsPulseWrHigh   CONFIG_MTD_NAND_DW_C_FlsPulseWrHigh
	   #define C_FlsPulseWrLow    CONFIG_MTD_NAND_DW_C_FlsPulseWrLow
	   #define C_FlsPulseRdHigh   CONFIG_MTD_NAND_DW_C_FlsPulseRdHigh
	   #define C_FlsPulseRdLow    CONFIG_MTD_NAND_DW_C_FlsPulseRdLow

	   #define C_FlsRdyTimeout    CONFIG_MTD_NAND_DW_C_FlsRdyTimeout
#else //CONFIGURE_MTD_NAND_DW_CONFIGURE_TIMING
	#ifdef SYS_CLK_24MHz
	   // Clock is 41.67 nSec

	   // WaitRdy values must be 2-15 if READYi line is used, 0 otherwise.
	   #define C_FlsWaitRdy1Fall  4 // tWB
	   #define C_FlsWaitRdy1Rise  4 // max(tAR,tRR, tWHR1)
	   #define C_FlsWaitRdy2Fall  4 // TBD; Was 0
	   #define C_FlsWaitRdy2Rise  4 // TBD; Was 0
	   #define C_FlsWaitRdy3Fall  4 // TBD; Was 0
	   #define C_FlsWaitRdy3Rise  4 // TBD; Was 0
	   #define C_FlsWaitRdy4Fall  4 // TBD; Was 0
	   #define C_FlsWaitRdy4Rise  4 // TBD; Was 0

	   // Write & Read pulse shape: value = n --> (n+1) wait cycles generated
	   #define C_FlsPulseCleStup  4 // tCLS; 0,2,3 values tested OK on 13.8MHz clk.
	   #define C_FlsPulseWrHigh   4 // tWH;  0,2,3 values tested OK on 13.8MHz clk.
	   #define C_FlsPulseWrLow    4 // tWP;  0,2,3 values tested OK on 13.8MHz clk.
	   #define C_FlsPulseRdHigh   4 // tREH; 0,2,3 values tested OK on 13.8MHz clk.
	   #define C_FlsPulseRdLow    4 // tRP;  0,2,3 values tested OK on 13.8MHz clk.

	   // Wait for Ready back high - overall transaction timeout
	   #define C_FlsRdyTimeout    32 // timeout > 10 mSec

	#endif // SYS_CLK_24MHz
#endif //CONFIGURE_MTD_NAND_DW_CONFIGURE_TIMING

// Status register:
typedef struct
{
   unsigned int  TransDone:       1;    // bit 0;  R
   unsigned int  EccDone:         1;    // bit 1;  R
   unsigned int  ChainSrcErrFnd:  1;    // bit 2;  R
   unsigned int  RdyTimeOut:      1;    // bit 3;  R
   unsigned int  Rdy1RiseStat:    1;    // bit 4;  R
   unsigned int  Rdy2RiseStat:    1;    // bit 5;  R
   unsigned int  TransBusy:       1;    // bit 6;  R
   unsigned int  EccBysy:         1;    // bit 7;  R
   unsigned int  FlsReady1:       1;    // bit 8;  R
   unsigned int  FlsReady2:       1;    // bit 9;  R
   unsigned int  Rsrv1:           6;    // bits 10 - 15;
   // ------------------------------
   unsigned int  ChainSrchErrIx: 13;    // bits 16 - 28;  R
   unsigned int  Rsrv2:           3;    // bits 28 - 31;
} Y_FC_Stat;

// Status-Clear register:
typedef struct
{
   unsigned int  ClrTransDone:       1;    // bit 0;  W
   unsigned int  ClrEccDone:         1;    // bit 1;  W
   unsigned int  ClrChainSrcErrFnd:  1;    // bit 2;  W
   unsigned int  ClrRdyTimeOut:      1;    // bit 3;  W
   unsigned int  ClrRdy1RiseStat:    1;    // bit 4;  W
   unsigned int  ClrRdy2RiseStat:    1;    // bit 5;  W
   unsigned int  Rsrv1:              2;    // bits  6 -  7;
   unsigned int  Rsrv2:              8;    // bits  8 - 15;
   // ---------------------------------
   unsigned int  Rsrv3:             16;    // bits 16 - 31;
} Y_FC_StatClr;

// Interrupt-Enable Mask register:
typedef struct
{
   unsigned int  TransCmpltEn:  1;  // bit 0;  R/W
   unsigned int  EccCmpltEn:    1;  // bit 1;  R/W
   unsigned int  ChainErrFndEn: 1;  // bit 2;  R/W
   unsigned int  RdyTimeOutEn:  1;  // bit 3;  R/W
   unsigned int  Rdy1RiseEn:    1;  // bit 4;  R/W
   unsigned int  Rdy2RiseEn:    1;  // bit 5;  R/W
   unsigned int  Rsrv1:         2;  // bits  6 -  7;
   unsigned int  Rsrv2:         8;  // bits  8 - 15;
   // ----------------------------
   unsigned int  Rsrv3:        16;  // bits 16 - 31;
} Y_FC_IntrEn;

// Sequence register
typedef struct
{
   unsigned int  Cmd1En:      1;  // bit  0;  R/W
   unsigned int  Addr1En:     1;  // bit  1;  R/W
   unsigned int  Addr2En:     1;  // bit  2;  R/W
   unsigned int  Addr3En:     1;  // bit  3;  R/W
   unsigned int  Addr4En:     1;  // bit  4;  R/W
   unsigned int  Addr5En:     1;  // bit  5;  R/W
   unsigned int  Wait1En:     1;  // bit  6;  R/W
   unsigned int  Cmd2En:      1;  // bit  7;  R/W
   unsigned int  Wait2En:     1;  // bit  8;  R/W
   unsigned int  DataRWEn:    1;  // bit  9;  R/W
   unsigned int  DataRWDir:   1;  // bit 10:  R/W
   unsigned int  DataEccEn:   1;  // bit 11:  R/W
   unsigned int  Cmd3En:      1;  // bit 12;  R/W
   unsigned int  Wait3En:     1;  // bit 13;  R/W
   unsigned int  Cmd4En:      1;  // bit 14;  R/W
   unsigned int  Wait4En:     1;  // bit 15;  R/W
   // -------------------------
   unsigned int  ReadOnce:    1;  // bit  16;       R/W
   unsigned int  ChipSlct:    2;  // bits 17 - 18;  R/W
   unsigned int  KeepChpSlct: 1;  // bit  19;       R/W
   unsigned int  Mode16:      1;  // bit  20;       R/W
   unsigned int  RdyInEn:     1;  // bit  21;       R/W
   unsigned int  RdySlct:     1;  // bit  22;       R/W
   unsigned int  Rsrv1:       1;  // bit  23;       R
   unsigned int  Rsrv2:       8;  // bits 24 - 31;  R
} Y_FC_CmdSeq;

#define C_FC_CmdSeq_ChipSlct_Dev0  0x0
#define C_FC_CmdSeq_ChipSlct_Dev1  0x1
#define C_FC_CmdSeq_ChipSlct_Dev2  0x2
#define C_FC_CmdSeq_ChipSlct_Dev3  0x3

#define C_FC_CmdSeq_RdySlct_Rdy1   0x0
#define C_FC_CmdSeq_RdySlct_Rdy2   0x1

// Address Column register
typedef struct
{
   unsigned int  Addr1:   8;  // bits  0 -  7  R/W  - First  cycle's address
   unsigned int  Addr2:   8;  // bits  8 - 15  R/W  - Second cycle's address
   // ----------------------
   unsigned int  Rsrv1:  16;  // bits 16 - 31
} Y_FC_AddrCol;

// Address Row register
typedef struct
{
   unsigned int  Addr3:   8;  // bits  0 -  7  R/W  - Third  cycle's address
   unsigned int  Addr4:   8;  // bits  8 - 15  R/W  - Fourth cycle's address
   // ----------------------
   unsigned int  Addr5:   8;  // bits 16 - 23  R/W  - Fifth cycle's address
   unsigned int  Rsrv1:   8;  // bits 24 - 31
} Y_FC_AddrRow;

// Command code config register
typedef struct
{
   unsigned int  Cmd1:   8;   // bits  0 -  7  R/W  - First command
   unsigned int  Cmd2:   8;   // bits  8 - 15  R/W  - Second command
   // ---------------------
   unsigned int  Cmd3:   8;   // bits 16 - 23  R/W  - Third command
   unsigned int  Cmd4:   8;   // bits 24 - 31  R/W  - Fourth command
} Y_FC_Cmd;

// Wait-Time config register
typedef struct
{
   unsigned int  Wait1A:   4;   // bits  0 -  3  R/W
   unsigned int  Wait1B:   4;   // bits  4 -  7  R/W
   unsigned int  Wait2A:   4;   // bits  8 - 11  R/W
   unsigned int  Wait2B:   4;   // bits 12 - 15  R/W
   // -----------------------
   unsigned int  Wait3A:   4;   // bits 16 - 19  R/W
   unsigned int  Wait3B:   4;   // bits 20 - 23  R/W
   unsigned int  Wait4A:   4;   // bits 24 - 27  R/W
   unsigned int  Wait4B:   4;   // bits 28 - 31  R/W
} Y_FC_Wait;

// Pulse-Time config register
typedef struct
{
   unsigned int  ReadLow:    4;   // bits  0 -  3  R/W
   unsigned int  ReadHigh:   4;   // bits  4 -  7  R/W
   unsigned int  WriteLow:   4;   // bits  8 - 11  R/W
   unsigned int  WriteHigh:  4;   // bits 12 - 15  R/W
   // -------------------------
   unsigned int  CleSetup:   4;   // bits 16 - 19  R/W
   unsigned int  Rsrv1:      12;  // bits 20 - 31  R
} Y_FC_PulseTime;

// Data Count register
typedef struct
{
   unsigned int  VirtMainCnt: 13;  // bits  0 - 12  R/W
   unsigned int  Rsrv1:        3;  // bits 13 - 15
   // ---------------------------
   unsigned int  SpareN2Cnt:   8;  // bits 16 - 23  R/W
   unsigned int  SpareN1Cnt:   4;  // bits 24 - 27  R/W
   unsigned int  VirtPageCnt:  4;  // bits 28 - 31  R/W
} Y_FC_Dcount;

// Timeout config register
typedef struct
{
   unsigned int  RdyTimeOutCnt:  8;  // bits  0 -  7  R/W
   unsigned int  RdyTimeOutEn:   1;  // bit   8       R/W
   unsigned int  Rsrv1:          7;  // bits  9 - 15  R
   // -----------------------------
   unsigned int  Rsrv2:         16;  // bits 16 - 31  R
} Y_FC_TimeOut;

// ECC Status
typedef struct
{
   unsigned int  P0BchErr:    1;  // bit   0       R
   unsigned int  P1BchErr:    1;  // bit   1       R
   unsigned int  P2BchErr:    1;  // bit   2       R
   unsigned int  P3BchErr:    1;  // bit   3       R
   unsigned int  P4BchErr:    1;  // bit   4       R
   unsigned int  P5BchErr:    1;  // bit   5       R
   unsigned int  P6BchErr:    1;  // bit   6       R
   unsigned int  P7BchErr:    1;  // bit   7       R
   unsigned int  Rsrv1:       8;  // bits  8 - 15  R
   // --------------------------
   unsigned int  P0HamErr:    1;  // bit  16       R
   unsigned int  P1HamErr:    1;  // bit  17       R
   unsigned int  P2HamErr:    1;  // bit  18       R
   unsigned int  P3HamErr:    1;  // bit  19       R
   unsigned int  P4HamErr:    1;  // bit  20       R
   unsigned int  P5HamErr:    1;  // bit  21       R
   unsigned int  P6HamErr:    1;  // bit  22       R
   unsigned int  P7HamErr:    1;  // bit  23       R
   unsigned int  Rsrv2:       8;  // bits 24 - 31  R
} Y_FC_EccStat;

// ECC 'S1' & 'S2' register
typedef struct
{
   unsigned int  S1Data:     13;  // bits  0 - 12  R/W
   unsigned int  Rsrv1:       3;  // bits 13 - 15  R
   // --------------------------
   unsigned int  S2Data:     13;  // bits 16 - 28  R/W
   unsigned int  Rsrv2:       3;  // bits 29 - 31  R
} Y_FC_Ecc_s_12;

// ECC S3 & S4 register
typedef struct
{
   unsigned int  S3Data:     13;  // bits  0 - 12  R/W
   unsigned int  Rsrv1:       3;  // bits 13 - 15  R
   // --------------------------
   unsigned int  S4Data:     13;  // bits 16 - 28  R/W
   unsigned int  Rsrv2:       3;  // bits 29 - 31  R
} Y_FC_Ecc_s_34;

// ECC S5 & S7 register
typedef struct
{
   unsigned int  S5Data:     13;  // bits  0 - 12  R/W
   unsigned int  Rsrv1:       3;  // bits 13 - 15  R
   // --------------------------
   unsigned int  S7Data:     13;  // bits 16 - 28  R/W
   unsigned int  Rsrv2:       3;  // bits 29 - 31  R
} Y_FC_Ecc_s_57;

// ECC Hamming-Code Out register
typedef struct
{
   unsigned int  HammCodeOut:  13;  // bits  0 - 12  R/W. b12-LSBit, b0-MSBit !!!
   unsigned int  Rsrv1:         3;  // bits 13 - 15  R
   // ----------------------------
   unsigned int  Rsrv2:        16;  // bits 16 - 31  R
} Y_FC_Hamming;

// BCH Syndrom Read Low register
typedef struct
{
   unsigned int  BchLow:      32;  // bits 0 - 31  R/W
} Y_FC_BchReadLow;

// BCH Syndrom Read High register
typedef struct
{
   unsigned int  BchHigh:     20;  // bits  0 - 19  R/W
   unsigned int  BchDecBuf:    4;  // bits 20 - 23  R/W
   unsigned int  Rsrv1:        8;  // bits 24 - 31
} Y_FC_BchReadHigh;

// GF Multiplier Output register
typedef struct
{
   unsigned int  GfMultOut:   13;  // bits  0 - 12  R
   unsigned int  Rsrv1:        3;  // bits 13 - 15  R
   // ---------------------------
   unsigned int  Rsrv2:       15;  // bits 16 - 30  R
   unsigned int  MultBusy:     1;  // bit  31       R
} Y_FC_GfMulOut;

// GF Multiplier Input register
typedef struct
{
   unsigned int  A_in:   13;  // bits  0 - 12  W
   unsigned int  Rsrv1:   3;  // bits 13 - 15  R
   // ----------------------
   unsigned int  B_in:   13;  // bits 16 - 28  W
   unsigned int  Rsrv2:   3;  // bits 29 - 31  R
} Y_FC_GfMulIn;

// GF FIFO Bypass Control register
typedef struct
{
   unsigned int  FBypBusy:   1;  // bit   0       R
   unsigned int  FBypEn:     1;  // bit   1       R/W
   unsigned int  FBypWr:     1;  // bit   2       R/W
   unsigned int  FBypRdReg:  1;  // bit   3       R/W
   unsigned int  FBypRdFls:  1;  // bit   4       R/W
   unsigned int  Rsrv1:      3;  // bits  5 -  7  R
   unsigned int  Rsrv2:      8;  // bits  8 - 15  R
   // -------------------------
   unsigned int  Rsrv3:     16;  // bits 16 - 31  R
} Y_FC_FBypCtrl;

// GF FIFO Bypass Data register
typedef struct
{
   unsigned int  FifoData:  16;  // bits  0 - 15  R/W
   // -------------------------
   unsigned int  FlsData:   16;  // bits 16 - 31  R/W
} Y_FC_FBypData;


// Register base:
typedef struct
{
   Y_FC_Ctrl          FC_Ctrl;          // Control
   Y_FC_Stat          FC_Stat;          // Status
   Y_FC_StatClr       FC_StatClr;       // Status Clear
   Y_FC_IntrEn        FC_IntrEn;        // Interrupt-Enable Mask
   Y_FC_CmdSeq        FC_CmdSeq;        // Command-Sequence
   Y_FC_AddrCol       FC_AddrCol;       // Address Column
   Y_FC_AddrRow       FC_AddrRow;       // Address Row
   Y_FC_Cmd           FC_Cmd;           // Command code config
   Y_FC_Wait          FC_Wait;          // Wait-Time config
   Y_FC_PulseTime     FC_PulseTime;     // Pulse-Time config
   Y_FC_Dcount        FC_Dcount;        // Data Count
   Y_FC_TimeOut       FC_TimeOut;       // Timeout config
   __u32             dummy1;           // No register - Address gap to next registers
   Y_FC_EccStat       FC_EccStat;       // ECC Status
   Y_FC_Ecc_s_12      FC_Ecc_s_12;      // ECC S1 & S2
   Y_FC_Ecc_s_34      FC_Ecc_s_34;      // ECC S3 & S4
   Y_FC_Ecc_s_57      FC_Ecc_s_57;      // ECC S5 & S7
   Y_FC_Hamming       FC_Hamming;       // ECC Hamming-Code Out
   Y_FC_BchReadLow    FC_BchReadLow;    // BCH Syndrom Read Low
   Y_FC_BchReadHigh   FC_BchReadHigh;   // BCH Syndrom Read High
   Y_FC_GfMulOut      FC_GfMulOut;      // GF Multiplier Out
   Y_FC_GfMulIn       FC_GfMulIn;       // GF Multiplier In
   Y_FC_FBypCtrl      FC_FBypCtrl;      // GF FIFO Bypass Control
   Y_FC_FBypData      FC_FBypData;      // GF FIFO Bypass Data
} Y_FC_Regs;

#endif /* _NAND_FLASH_CONTROL_H_ */

/*============================   End of File   ==========================*/
