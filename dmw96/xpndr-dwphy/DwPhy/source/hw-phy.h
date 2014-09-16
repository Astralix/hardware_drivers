//
// DwPhy.h -- PHY Driver/Interface Functions for DW52
// Copyright 2007-2011 DSP Group, Inc. All rights reserved.
//
// This module is maintained by Barrett Brickner
// It is used by both the DwPhyMex test library and DW52 Kona WiFi driver
//

// Compile-time options (if defined)...
//
//    DWPHY_SUPPORT_BERMAI      -- support the Bermai Dakota 2/2g/4 + BER5000
//    DWPHY_SUPPORT_MOJAVE      -- support the DW52MB  baseband
//    DWPHY_SUPPORT_MOJAVE1B    -- support the DW52MB metal spin baseband
//    DWPHY_SUPPORT_NEVADA_FPGA -- support the DMW96 WLAN FPGA
//    DWPHY_SUPPORT_RF52        -- support the RF52 radio

#ifndef __DWPHY_H
#define __DWPHY_H

#if defined(__cplusplus) || defined(__cplusplus__)
    extern "C" {
#endif



// ================================================================================================
// CODE TO INCLUDE WHEN NOT INCLUDING THIS FILE IN THE DRIVER BUILD
// ================================================================================================
#ifdef DWPHY_NOT_DRIVER
    #include "mex.h"
    #ifdef WIN32
        #include "RvClient.h"      // Remote VWAL (for BERLab)
    #else
        #include "DwPhyUtil.h"     // Application for DW52/ARM (for DwPhyApp)
    #endif

    #include <malloc.h>            // memory allocation
    #include <memory.h>            // memcpy
    #define OS_MemAlloc(length, flags ) malloc(length)
    #define OS_MemFree(pBuffer, length) free(pBuffer)
    #define OS_MemSet(pBuffer, value, length) memset(pBuffer, value, length)
    #define copy_from_user(dest, src, count) ((memcpy(dest, src, count) == NULL) ? 1 : 0)

    #define DWPHY_SUPPORT_BERMAI      // Required to use a Bermai "Bob" card

    #define DWPHY_SUPPORT_MOJAVE      // DW52MBxx
    #define DWPHY_SUPPORT_MOJAVE1B    // DW52MB metal spin
    #define DWPHY_SUPPORT_NEVADA_FPGA // DMW96 WLAN FPGA
    #define DWPHY_SUPPORT_NEVADA      // DMW96 ASIC
    #define DWPHY_SUPPORT_RF52        // DW52RF52xx

    // ======================================================
    // SYMBOLS NORMALLY DEFINED ELSEWHERE IN THE DRIVER BUILD
    // ======================================================
    #define CMU_CSR0        0x05300004
    #define CMU_BSWRSTR     0x05300050

    #define DWPHY_DEBUG     1



#ifndef BIT
    #define BIT(b)	(1 << (b))
#endif

#else // code below is included in the driver

    #define DWPHY_SUPPORT_BERMAI // TODO (Chuck) relocate "support" definitions as appropriate
    #define DWPHY_SUPPORT_MOJAVE
    #define DWPHY_SUPPORT_MOJAVE1B
    #define DWPHY_SUPPORT_NEVADA_FPGA
    #define DWPHY_SUPPORT_NEVADA
    #define DWPHY_SUPPORT_RF52

    #include "common.h"
    #include "hw.h"
    #include "hw_i.h"
    #include "ahb.h"
    #include "utils.h"

#endif // DWPHY_NOT_DRIVER


// ================================================================================================
// DEFAULT REGISTER ADDR/DATA PAIRS
// ================================================================================================
typedef struct 
{
    uint16_t addr; // PHY address, 0 - 1023
    uint8_t  data; // data value,  0 - 255
} dwPhyRegPair_t;

// ================================================================================================
// FREQUENCY-BAND SELECTIVE REGISTER DATA
// ================================================================================================
typedef struct 
{
    uint16_t Addr;    // PHY address, 0 - 1023
    uint8_t  Mask;    // mask for field to be modified
    uint8_t  Data[9]; // data value,  0 - 255, by 9 individual bands
} dwPhyRegByBand_t;

// ================================================================================================
// CHANNEL-SPECIFIC REGISTER DATA
// ================================================================================================
typedef struct 
{
    uint16_t Addr;      // PHY address, 0 - 1023
    uint8_t  Mask;      // mask for field to be modified
    uint8_t  Data[256]; // data value,  0 - 255, by channel, index follows RF52 channel encoding
} dwPhyRegByChanl_t;

typedef struct 
{
    uint16_t Addr;     // PHY address, 0 - 1023
    uint8_t  Mask;     // mask for field to be modified
    uint8_t  Data[14]; // data value,  0 - 13 for 2.4 GHz band channels 1-14
} dwPhyRegByChanl24_t;

// ================================================================================================
// ENUMERATIONS
// ================================================================================================
typedef enum 
{
    DWPHY_RXMODE_DEFAULT  =-1, // default = maximum modulation compatibility allowed by baseband
    DWPHY_RXMODE_NONE     = 0, // this is allowed by the H/W but is not a valid setting
    DWPHY_RXMODE_802_11A  = 1, // OFDM only
    DWPHY_RXMODE_802_11B  = 2, // DSSS/CCK only
    DWPHY_RXMODE_802_11G  = 3, // OFDM + DSSS/CCK
    DWPHY_RXMODE_802_11N5 = 5, // HT-OFDM + L-OFDM (802.11n for 5 GHz)
    DWPHY_RXMODE_802_11N  = 7  // HT-OFDM, L-OFDM, DSSS/CCK (802.11n for 2.4 GHz)
} dwPhyRxMode_t;
        
typedef enum 
{
    DWPHY_DIVERSITY_DEFAULT = 0, // default
    DWPHY_DIVERSITY_PATH_A  = 1, // RXA only
    DWPHY_DIVERSITY_PATH_B  = 2, // RXB only
    DWPHY_DIVERSITY_MRC     = 3, // both paths
    DWPHY_DIVERSITY_ANTSEL  = 4  // antenna selection
} dwPhyDiversityMode_t;
        
typedef enum
{
    DWPHY_PLATFORM_DEFAULT        = 0, // Default configuration (DW52/74 ASIC)
    DWPHY_PLATFORM_VERSATILE_FPGA = 1, // Versatile ARM + FPGA WLAN
    DWPHY_PLATFORM_DMW96          = 2, // XpandR III
} dwPhyPlatform_t;

typedef enum // Baseband Part IDs
{
    DWPHY_BASEBAND_DAKOTA      = 1, // Original Bermai baseband (not really used anywhere)
    DWPHY_BASEBAND_DAKOTA2     = 2, // found on some engineering test boards
    DWPHY_BASEBAND_DAKOTA2G    = 3, // BER4001
    DWPHY_BASEBAND_DAKOTA4     = 4, // BER4002
    DWPHY_BASEBAND_MOJAVE      = 5, // DW52MB52
    DWPHY_BASEBAND_MOJAVE1B    = 6, // DW52MB52 metal spin
    DWPHY_BASEBAND_NEVADA_FPGA = 7, // DMW96 FPGA (WLAN Prototype)
    DWPHY_BASEBAND_NEVADA      = 8, // DMW96 ASIC
    DWPHY_BASEBAND_NEVADA1B    = 9, // DMW96B ASIC
} dwPhyBaseband_t;

typedef enum // Radio Part IDs
{
    // No Radio
    //
    DWPHY_RADIO_NONE = 0,
    //
    // Bermai Armstrong Radios
    //
    DWPHY_RADIO_ARMSTRONG_A1 =  1, DWPHY_RADIO_ARMSTRONG_A2, DWPHY_RADIO_ARMSTRONG_A3, DWPHY_RADIO_ARMSTRONG_A4, // skip 5
    DWPHY_RADIO_ARMSTRONG_A6 =  6,
    DWPHY_RADIO_ARMSTRONG_C1 =  7, DWPHY_RADIO_ARMSTRONG_C2, DWPHY_RADIO_ARMSTRONG_C3, DWPHY_RADIO_ARMSTRONG_C4, // skip 11
    DWPHY_RADIO_ARMSTRONG_C6 = 12,
    DWPHY_RADIO_ARMSTRONG_D1 = 13, DWPHY_RADIO_ARMSTRONG_D2, DWPHY_RADIO_ARMSTRONG_D3, DWPHY_RADIO_ARMSTRONG_D4, // skip 17
    DWPHY_RADIO_ARMSTRONG_D6 = 18,
    //
    // Bermai Basie (BER5000) Radios
    //
    DWPHY_RADIO_BASIE_A1 = 19, DWPHY_RADIO_BASIE_A2,
    DWPHY_RADIO_BASIE_B1 = 21, DWPHY_RADIO_BASIE_B2, DWPHY_RADIO_BASIE_B3,
    DWPHY_RADIO_BASIE_C1 = 24, DWPHY_RADIO_BASIE_C2, DWPHY_RADIO_BASIE_C3,
    DWPHY_RADIO_BASIE_D1 = 27, DWPHY_RADIO_BASIE_D2, DWPHY_RADIO_BASIE_D3,
    //
    // DSP Group RF52 Radios and Test Chips
    //
    DWPHY_RADIO_RF52SHC = 30,

    DWPHY_RADIO_RF52A01 = 31, DWPHY_RADIO_RF52A02, DWPHY_RADIO_RF52A03, DWPHY_RADIO_RF52A04,
    DWPHY_RADIO_RF52A110= 35, DWPHY_RADIO_RF52A111,DWPHY_RADIO_RF52A112,DWPHY_RADIO_RF52A113, 
    DWPHY_RADIO_RF52A120= 39, DWPHY_RADIO_RF52A130,DWPHY_RADIO_RF52A131,DWPHY_RADIO_RF52A140, DWPHY_RADIO_RF52A141,

    DWPHY_RADIO_RF52A210= 44, DWPHY_RADIO_RF52A211,DWPHY_RADIO_RF52A212,
    DWPHY_RADIO_RF52A220= 47, DWPHY_RADIO_RF52A230,DWPHY_RADIO_RF52A240,

    DWPHY_RADIO_RF52A320= 50, DWPHY_RADIO_RF52A321,
    DWPHY_RADIO_RF52A421= 52,
    DWPHY_RADIO_RF52A521= 53,

    DWPHY_RADIO_RF52B11 = 54, DWPHY_RADIO_RF52B12, DWPHY_RADIO_RF52B13,
    DWPHY_RADIO_RF52B21 = 57, DWPHY_RADIO_RF52B22, DWPHY_RADIO_RF52B23,
    DWPHY_RADIO_RF52B31 = 60, DWPHY_RADIO_RF52B32,
    //
    // DSP Group RF22 Radios
    //
    DWPHY_RADIO_RF22A01 = 65, DWPHY_RADIO_RF22A02 = 66,
    DWPHY_RADIO_RF22A11 = 67, DWPHY_RADIO_RF22A12 = 68,   
    DWPHY_RADIO_RF22B01 = 69, DWPHY_RADIO_RF22B02 = 70, DWPHY_RADIO_RF22B03 = 71, DWPHY_RADIO_RF22B04 = 72

} dwPhyRadio_t;

typedef enum // Parameters/Programmable Data Types
{
    DWPHY_PARAM_CHIPSET_ALIAS      = 1,
    DWPHY_PARAM_DEFAULTREGISTERS   = 2,
    DWPHY_PARAM_REGISTERSBYBAND    = 3,
    DWPHY_PARAM_REGISTERSBYCHANL   = 4,
    DWPHY_PARAM_REGISTERSBYCHANL24 = 5,

} dwPhyParam_t;


/*
typedef enum // DWPHY_PARAM_OPTION Types
{
    DWPHY_OPTION_WRITE_PARAM_LENGTHS = 1

} dwPhyOption_t;
*/

// ================================================================================================
// DEVICE INFORMATION STRUCTURE
// Define locally if not part of the driver
// ================================================================================================
#ifdef DWPHY_NOT_DRIVER

    typedef struct // Local version of device info structure. Elements contained here are a subset
    {              // of the full driver version. Element DwPhy is owned by this module.
        void *pDwPhy;
    } dwDevInfo_t;

#endif // DWPHY_NOT_DRIVER

// ================================================================================================
// RETURNED STATUS TYPE
// ================================================================================================
typedef int32_t dwPhyStatus_t;

// ================================================================================================
// FUNCTION POINTER FOR DW_PHYEnB ENABLE/DISABLE
// ================================================================================================
typedef uint32_t (*DwPhyEnableFn_t)(dwDevInfo_t*, uint32_t);

// ================================================================================================
// DWPHY PRIVATE INFORMATION STRUCTURE
// ================================================================================================
typedef struct
{
    dwPhyPlatform_t PlatformID;

    int32_t Baseband; // baseband ID (expanded to 32 bit)
    int32_t Radio;    // radio ID    (expanded to 32 bit)
    int32_t Chipset;  // (RadioID << 8) | (BasebandID) used internally, subject to change

    struct
    {
        int8_t SetPrdBm;     // signal detect/carrier sense limit Pr[dBm]
        int8_t GainOfs;      // gain offset from defaults (units: 1.5 dB)
        int8_t Pwr100dBmOfs; // offset to Pwr100dBm relative to default
        int8_t MsrPwrOfs;    // offset to measured power (RSSI)
        int8_t DataIsValid;  // is data in this structure valid?
    } RxSensitivity;

    struct // This structure contains default register values for the current channel.
    {      // These are stored in case InitAGain is changed to reduce sensitivity
        uint8_t InitAGain;
        uint8_t ThSwitchLNA;
        uint8_t ThSwitchLNA2;
        uint8_t ThCCA1;
        uint8_t ThCCA2;
        uint8_t Pwr100dBm;
        uint8_t ThCCA_SigDet;
        uint8_t ThCCA_RSSI1;
        uint8_t ThCCA_RSSI2;
        uint8_t ThCCA_GF1;
        uint8_t ThCCA_GF2;
        uint8_t SyncTh;
        int8_t DataIsValid;  // is data in this structure valid?
    } RecordedDefaultReg;

    dwPhyRxMode_t RxMode;

    int32_t ChipsetAlias; // forces value returned by Chipset
    
    dwPhyRegPair_t      *pDefaultReg;       // registers for initialization
    uint32_t              DefaultRegLength;

    dwPhyRegByBand_t    *pRegByBand;        // registers by band on channel select
    uint32_t              RegByBandLength;

    dwPhyRegByChanl_t   *pRegByChanl;       // registers by chanl on channel select
    uint32_t              RegByChanlLength;

    dwPhyRegByChanl24_t *pRegByChanl24;     // registers by 2.4 GHz chanl on channel select
    uint32_t              RegByChanl24Length;

    DwPhyEnableFn_t DwPhyEnableFn; // function to set the state of DW_PHYEnB

} DwPhy_t;

// ================================================================================================
// STATUS CODES
//  Successful functions return a zero (0) status code. Errors produce a negative number.
//  Warnings and other return status values are positive. The offset value is chosen so that the 
//  DWPHY status codes do not overlap status codes produced by VISA instrument drivers (0x3FFF0000) 
//  or the WiSE simulator (0x011A0000)
// ================================================================================================
#define DWPHY_SUCCESS  0

#define DWPHY_STATUS_OFFSET               0x011B0000L
#define DWPHY_ERROR                      (DWPHY_STATUS_OFFSET - 2147483647L - 1)

#define DWPHY_ERROR_PARAMETER1           (DWPHY_ERROR + 0x0001L)
#define DWPHY_ERROR_PARAMETER2           (DWPHY_ERROR + 0x0002L)
#define DWPHY_ERROR_PARAMETER3           (DWPHY_ERROR + 0x0003L)
#define DWPHY_ERROR_PARAMETER4           (DWPHY_ERROR + 0x0004L)
#define DWPHY_ERROR_PARAMETER5           (DWPHY_ERROR + 0x0005L)

#define DWPHY_ERROR_UNSUPPORTED_PLATFORM (DWPHY_ERROR + 0x0011L)
#define DWPHY_ERROR_UNSUPPORTED_CHIPSET  (DWPHY_ERROR + 0x0012L)
#define DWPHY_ERROR_UNSUPPORTED_BASEBAND (DWPHY_ERROR + 0x0013L)
#define DWPHY_ERROR_UNSUPPORTED_RADIO    (DWPHY_ERROR + 0x0014L)
#define DWPHY_ERROR_UNSUPPORTED_CHANNEL  (DWPHY_ERROR + 0x0015L)
#define DWPHY_ERROR_UNDEFINED_CASE       (DWPHY_ERROR + 0x0016L)
#define DWPHY_ERROR_REGISTER_ADDR_RANGE  (DWPHY_ERROR + 0x0017L)
#define DWPHY_ERROR_PLL_NOT_LOCKED       (DWPHY_ERROR + 0x0020L)
#define DWPHY_ERROR_MEMORY_ALLOCATION    (DWPHY_ERROR + 0x0030L)
#define DWPHY_ERROR_OSCOPYFROMUSER       (DWPHY_ERROR + 0x0031L)
#define DWPHY_ERROR_DWPHYENABLE          (DWPHY_ERROR + 0x0040L)
#define DWPHY_ERROR_TXDCOC_CAL_FAILED    (DWPHY_ERROR + 0x0050L)
#define DWPHY_ERROR_LOFT_CAL_FAILED      (DWPHY_ERROR + 0x0051L)
#define DWPHY_ERROR_IQ_CAL_FAILED        (DWPHY_ERROR + 0x0052L)        

#define DWPHY_ERROR_XTAL_CAL_FAILED      (DWPHY_ERROR + 0x0060L)     
#define DWPHY_ERROR_RXLPF_CAL_FAILED     (DWPHY_ERROR + 0x0061L)      

#define DWPHY_ERROR_UNKNOWN_FUNCTION     (DWPHY_ERROR + 0x00F1L)

// ================================================================================================
// MISCELLANENEOUS DEFINITIONS
// ================================================================================================
#define DWPHY_DEFAULT_RX_SENSITIVITY 0

// ================================================================================================
// DEFINE ALIASES
// These were originally included to support DwPhy on the Bermai driver. Now they are simply
// aliases for functions in the Kona driver.
// ================================================================================================
#define DwPhy_Delay(t_microsecond) OS_Delay(t_microsecond)

// ================================================================================================
// DWPHY REGISTER ACCESS FUNCTIONS
// ================================================================================================
uint8_t DwPhy_ReadReg (uint32_t Addr);
void    DwPhy_WriteReg(uint32_t Addr, uint8_t Data);

// ================================================================================================
// DWPHY SETUP FUNCTIONS -- CALL BEFORE INITIALIZATION
// ================================================================================================
void          DwPhy_SetDwPhyEnableFn (dwDevInfo_t *pDevInfo, DwPhyEnableFn_t pDwPhyEnableFn);
dwPhyStatus_t DwPhy_SetParameterData (dwDevInfo_t *pDevInfo, dwPhyParam_t ParamType, 
                                        uint32_t DataSize, uint8_t *pUserData);

// ================================================================================================
// DWPHY MODULE STARTUP/SHUTDOWN
// ================================================================================================
dwPhyStatus_t DwPhy_Startup (dwDevInfo_t *pDevInfo);
dwPhyStatus_t DwPhy_Shutdown(dwDevInfo_t *pDevInfo);

// ================================================================================================
// BASIC PHY INTERFACE FUNCTIONS
// ================================================================================================
dwPhyStatus_t   DwPhy_Reset            (dwDevInfo_t *pDevInfo);
dwPhyStatus_t   DwPhy_Initialize       (dwDevInfo_t *pDevInfo, dwPhyPlatform_t PlatformID);
dwPhyStatus_t   DwPhy_SetRxMode        (dwDevInfo_t *pDevInfo, dwPhyRxMode_t RxMode);
dwPhyStatus_t   DwPhy_SetChannelFreq   (dwDevInfo_t *pDevInfo, uint32_t FcMHz);
dwPhyStatus_t   DwPhy_RadarDetect      (dwDevInfo_t *pDevInfo, uint8_t Enable);
dwPhyStatus_t   DwPhy_SetStationAddress(dwDevInfo_t *pDevInfo, uint8_t Addr[6]);
dwPhyStatus_t   DwPhy_AddressFilter    (dwDevInfo_t *pDevInfo, uint8_t Enable);
dwPhyStatus_t   DwPhy_SetDiversityMode (dwDevInfo_t *pDevInfo, dwPhyDiversityMode_t DiversityMode);
int32_t         DwPhy_RadioPllIsLocked (dwDevInfo_t *pDevInfo);
dwPhyStatus_t   DwPhy_ConvertHeaderRSSI(dwDevInfo_t *pDevInfo, uint8_t *pRSSI0, uint8_t *pRSSI1); 

dwPhyStatus_t   DwPhy_Sleep            (dwDevInfo_t *pDevInfo);
dwPhyStatus_t   DwPhy_Wake             (dwDevInfo_t *pDevInfo);
uint16_t        DwPhy_WakeupTime       (dwDevInfo_t *pDevInfo);

uint32_t        DwPhy_ChipSet          (dwDevInfo_t *pDevInfo);
dwPhyBaseband_t DwPhy_BasebandID       (dwDevInfo_t *pDevInfo);
dwPhyRadio_t    DwPhy_RadioID          (dwDevInfo_t *pDevInfo);
        
//
// FUNCTIONS TO ADD
//
// - Need to define control and LUT for RF52 TX gain control
// - Address filter tuning (all/data packets, modulation, RSSI limit)
// - Handle band-specific tuning and per-part tuning using values from EEPROM/FLASH
//

// ================================================================================================
// EXTENDED FUNCTIONALITY FOR PERFORMANCE TUNING AND EXPERIMENTAL TESTING
// ================================================================================================
dwPhyStatus_t DwPhy_SetRxSensitivity (dwDevInfo_t *pDevInfo,  int8_t     dBm);
dwPhyStatus_t DwPhy_GetRxSensitivity (dwDevInfo_t *pDevInfo,  int8_t *pPrdBm);
dwPhyStatus_t DwPhy_GetHistogramRPI  (dwDevInfo_t *pDevInfo, uint16_t NumSamples, uint8_t Density[8]);
dwPhyStatus_t DwPhy_ClearCounters    (dwDevInfo_t *pDevInfo);
dwPhyStatus_t DwPhy_GetRxCounters    (dwDevInfo_t *pDevInfo, uint8_t *pRxFaultCount, 
                                      uint8_t *pRestartCount, uint8_t *pNuisanceCount);

// ================================================================================================
// PHY CALIBRATION FUNCTIONS
// ================================================================================================
dwPhyStatus_t DwPhy_CalibrateTXDCOC_RF22 (dwDevInfo_t *pDevInfo);
dwPhyStatus_t DwPhy_CalibrateTXDCOC_RF22A(dwDevInfo_t *pDevInfo);
dwPhyStatus_t DwPhy_CalibrateTXDCOC_RF22B(dwDevInfo_t *pDevInfo);

dwPhyStatus_t DwPhy_CalibrateLOFT_RF22 (dwDevInfo_t *pDevInfo); 
dwPhyStatus_t DwPhy_CalibrateLOFT_RF22A(dwDevInfo_t *pDevInfo);
dwPhyStatus_t DwPhy_CalibrateLOFT_RF22B(dwDevInfo_t *pDevInfo);

dwPhyStatus_t DwPhy_CalibrateIQ      (dwDevInfo_t *pDevInfo); // selects a routine from below
dwPhyStatus_t DwPhy_CalibrateIQ_DMW96(dwDevInfo_t *pDevInfo);
dwPhyStatus_t DwPhy_CalibrateIQ_RF22 (dwDevInfo_t *pDevInfo);
dwPhyStatus_t DwPhy_CalibrateXtal    (dwDevInfo_t *pDevInfo);
dwPhyStatus_t DwPhy_CalibrateRxLPF   (dwDevInfo_t *pDevInfo);

// ================================================================================================
// FUNCTIONS FOR DEVICE WORK-AROUNDS
// ================================================================================================
void DwPhy_PllClosedLoopCalibration(dwDevInfo_t *pDevInfo); // for RF52A321 (and A421)
void DwPhy_CalibrateDataConv(dwDevInfo_t *pDevInfo);        // not needed...fixed via register settings
dwPhyStatus_t DwPhy_RF52B21_ToggleSTARTO(dwDevInfo_t *pDevInfo);

// ================================================================================================
// BASIC INTERFACE FUNCTIONS FOR BER400X/BER5000 COMPATIBILITY
// ================================================================================================
void DwPhy_CalibrateVCO (dwDevInfo_t *pDevInfo);
void DwPhy_PLLAsyncReset(dwDevInfo_t *pDevInfo);
//-------------------------------------------------------------------------------------------------
#if defined(__cplusplus) || defined(__cplusplus__)
    }
#endif

#endif // __DWPHY_H

//-------------------------------------------------------------------------------------------------
//--- End of File ---------------------------------------------------------------------------------
