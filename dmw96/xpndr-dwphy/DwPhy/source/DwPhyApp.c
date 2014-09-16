//--< DwPhyApp.c >---------------------------------------------------------------------------------
//=================================================================================================
//
//  Simple console application for PHY bring-up
//  Copyright 2007-2008 DSP Group, Inc. All rights reserved.
//
//  Written by Barrett Brickner
//
//=================================================================================================
//-------------------------------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "vwal.h"
#include "hw-phy.h"
#include "DwPhyUtil.h"
#include "WifiApi.h"

static char          DevName[16] = {"wifi0"}; // target device
static dwDevInfo_t   DevInfo = {0};          // Device private information (used by DwPhy)
static dwDevInfo_t *pDevInfo =&DevInfo;       // pointer to DevInfo (needed for DwPhy_WriteReg/ReadReg)

#define GCR0           0x05200000
#define RATE_BMAP      0x065090A8

#define DEBUG printf(" ### FILE: %s, LINE %d\n",__FILE__,__LINE__);

#define XSTATUS(fn)                                            \
{                                                              \
    if((fn)<0)                                                 \
    {                                                          \
        printf(">>> Error at %s line %d\n",__FILE__,__LINE__); \
        return -1;                                             \
    }                                                          \
}                                                              \

#define RADIOISRF22(radioID)	( (((radioID) >= 65) && ((radioID) <= 68)) ? 1 : 0)
//--------------------------------------------------------------

// ================================================================================================
// FUNCTION  : DwPhyApp_SetTestMode()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
void DwPhyApp_SetTestMode(int argc, char *argv[])
{
    int n = strtol(argv[2], NULL, 0);
    
    if(n < 0 || n > 7)
        printf(">>> value out of range for testmode (0-7)\n\n");
    else
    {
        uint32_t x;
        uint8_t  d;

        x = OS_RegRead(GCR0);
        OS_RegWrite(GCR0, x|0x1000); // enable test port I/O
    
        d = (OS_RegReadBB(7) & 0x0F) | (n << 4); // set test port mux
        OS_RegWriteBB(7, d);
    }
}

// ================================================================================================
// FUNCTION  : DwPhyApp_ReadRegister()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
void DwPhyApp_ReadRegister(int argc, char *argv[])
{
    uint32_t addr = strtoul(argv[2], NULL, 0);
    
    if(addr < 1024 || (addr & 0x00008000))
    {
        uint8_t data;
        
        if (addr & 0x00008000)
        {
            OS_RegWriteBB(256+126, addr & 0x0000007F);
            data = OS_RegReadBB(256+127);     
        }
        else
            data = OS_RegReadBB(addr);
        
        printf("PHY Register 0x%04X = 0x%02X\n",addr,data);
    }
    else
    {
        uint32_t data = OS_RegRead(addr);
        printf("Register 0x%08X = 0x%04X\n",addr,data);
    }
}

// ================================================================================================
// FUNCTION  : DwPhyApp_WriteRegister()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
void DwPhyApp_WriteRegister(int argc, char *argv[])
{
    uint32_t addr = strtoul(argv[2], NULL, 0);
    
    if(addr < 1024 || (addr & 0x00008000))
    {
        uint8_t data = (uint8_t)strtoul(argv[3], NULL, 0);
        
        if (addr & 0x00008000)
        {
            OS_RegWriteBB(256+126, addr & 0x0000007F);
            OS_RegWriteBB(256+127, data);     
        }
        else
            OS_RegWriteBB(addr, data);
    }
    else
    {
        uint32_t data = (uint32_t)strtoul(argv[3], NULL, 0);
        OS_RegWrite(addr, data);
    }
}

// ================================================================================================
// FUNCTION  : DwPhyApp_SetChannel
// ------------------------------------------------------------------------------------------------
// ================================================================================================
void DwPhyApp_SetChannel(int argc, char *argv[])
{
    uint32_t FcMHz = strtoul(argv[2], NULL, 0);
    
    dwPhyStatus_t status = DwPhy_SetChannelFreq(pDevInfo, FcMHz);
    if(status != DWPHY_SUCCESS)
    {
        printf(">>> Unable to set channel to %d MHz\n",FcMHz);
        printf(">>> DwPhy_SetChannelFreq Status = 0x%08X\n",status);
    }
}

// ================================================================================================
// FUNCTION  : DwPhyApp_CalibrateRF22
// ------------------------------------------------------------------------------------------------
// ================================================================================================
void DwPhyApp_CalibrateRF22(int argc, char *argv[])
{
    uint32_t type = strtoul(argv[2], NULL, 0);
    dwPhyStatus_t status;

    switch (type)
    {
        case 1: status = DwPhy_CalibrateTXDCOC_RF22(pDevInfo); break;
        case 2: status = DwPhy_CalibrateLOFT_RF22(pDevInfo);   break;
        case 3: status = DwPhy_CalibrateIQ_RF22(pDevInfo);     break;            
        case 4: status = DwPhy_CalibrateXtal(pDevInfo);        break;
        case 5: status = DwPhy_CalibrateRxLPF(pDevInfo);       break;
            
    }
    
    if(status != DWPHY_SUCCESS)
    {
        printf(">>> DwPhy_CalibrateRF22 Status = 0x%08X\n",status);
    }
}

// ================================================================================================
// FUNCTION  : DwPhyApp_DisplayInfo()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
void DwPhyApp_DisplayInfo(int argc, char *argv[])
{
    char *RxModeStr[] = {"undefined","802.11a","802.11b","802.11g"};
    char *RadioMode[] = {"Sleep","Standby","Transmit","Receive"};
    uint8_t BasebandID   = OS_RegRead(      1);
    uint8_t PathSel      = OS_RegRead(      3);
    uint8_t RXMode       = OS_RegRead(      4);
    uint8_t TX_RATE      = OS_RegRead(      9) >> 4;
    uint16_t TX_LENGTH   =(OS_RegRead(      9) &0xF)*256 + OS_RegRead(10);
    uint8_t TX_PWRLVL    = OS_RegRead(     11);
    uint8_t TxFault      = OS_RegRead(     14) >> 7;
    uint8_t bRxMode      = OS_RegRead(     17) >> 7;
    uint8_t RxFault      = OS_RegRead(     17) &0x1;
    uint8_t RATE         = OS_RegRead(     18) >> 4;
    uint16_t LENGTH      =(OS_RegRead(     18) &0xF)*256 + OS_RegRead(19);
    uint8_t RSSI0        = OS_RegRead(     22);
    uint8_t RSSI1        = OS_RegRead(     23);
    uint8_t InitAGain    = OS_RegRead(     65);
    uint8_t SigDetTh1    = OS_RegRead(    112);
    uint8_t SigDetTh2H   = OS_RegRead(    113);
    uint8_t SigDetTh2L   = OS_RegRead(    114);
//    int8_t  CFO          = OS_RegRead(    193);
    uint8_t RxFaultCount = OS_RegRead(    194);
    uint8_t RestartCount = OS_RegRead(    195);
    uint8_t NuisanceCount= OS_RegRead(    196);
    uint8_t RadioID      = OS_RegRead(256+  1);
    uint8_t Channel      = OS_RegRead(256+ 12);
    uint8_t PllLocked    = OS_RegRead(256+ 41) >> 7; if (RADIOISRF22(RadioID)) PllLocked = OS_RegRead(256+19) >> 2; 
     int8_t CLCALADJ     = OS_RegRead(256+ 41)&0x1F; if (CLCALADJ>15) CLCALADJ -= 32; // signed
    uint8_t CAPSEL       = OS_RegRead(256+ 42)&0x1F;
    uint8_t PCOUNT       = OS_RegRead(256+ 77) >> 1;
    uint8_t MCOUT        =(OS_RegRead(256+  7) >> 4) & 0x3; if (RADIOISRF22(RadioID)) MCOUT = (OS_RegRead(256+8) >> 4) & 0x3;
    uint8_t AGAIN        =(OS_RegRead(256+  6) >> 1) & 0x3F;
    uint8_t LNAGAIN      =(OS_RegRead(256+  6) >> 7) & 0x1;

    unsigned FcMHz;

         if(Channel < 176) FcMHz = 5000 + 5*Channel;
    else if(Channel < 200) FcMHz = 4000 + 5*Channel;
    else if(Channel < 241) FcMHz = 0;
    else if(Channel < 254) FcMHz = 2412 + 5*(Channel-241);
    else                   FcMHz = 2484 + 5*(Channel-254);

    printf("\n");        
    printf("  DwPhy Console Application (%s, %s)\n",__DATE__,__TIME__);
    printf("  Copyright 2007-2010 DSP Group, Inc. All rights reserved.\n");
    printf("\n");
    printf("  BasebandID = %3u (%s)\n",BasebandID, DwPhyUtil_BasebandName(BasebandID));
    printf("  RadioID    = %3u (%s)\n",RadioID,    DwPhyUtil_RadioName(RadioID));
    printf("  Channel    = %3u (%u MHz) %s\n",Channel,FcMHz,PllLocked ? "PLL Locked" : "*** PLL NOT LOCKED ***\n");
    printf("  CAPSEL     = %3u, CLCALADJ = %d\n",CAPSEL, CLCALADJ);
    printf("  RSSI       = %3d, %d dBm\n",(int)RSSI0 - 100, (int)RSSI1 - 100);
    printf("  PCOUNT     = %3d\n",PCOUNT);
    printf("  \n");
    printf("  RxMode     = %3u (%s)\n",RXMode,RxModeStr[RXMode%4]);
    printf("  PathSel    = %3u\n",PathSel);
    printf("  InitAGain  = %3u\n",InitAGain);
    printf("  SigDetTh1  = %3u, SigDetTh2 = %3u\n",SigDetTh1,SigDetTh2H<<8 | SigDetTh2L);
    if(TxFault)
        printf("  Last TX    : FAULT (Baseband Controller is Probably Stuck)\n");
    else
        printf("  Last TX    : RATE = 0x%X, LENGTH = %u bytes, TX_PWRLVL = %u\n",TX_RATE,TX_LENGTH,TX_PWRLVL);
    if(RxFault)
        printf("  Last RX    : FAULT\n");
    else
        printf("  Last RX    : %s = 0x%X, LENGTH = %d bytes\n",bRxMode ? "bRATE":"RATE", RATE, LENGTH);
    printf("  RX Counters: RxFault = %u, Restart = %u, Nuisance = %u (max 255)\n",RxFaultCount,RestartCount,NuisanceCount);
//    printf("  CFO        = %3d (%d kHz)\n",CFO,(int)CFO*2441/1000); // wtf...this doesn't work
    printf("  RadioState = %s, LNAGAIN = %u, AGAIN = %u\n",RadioMode[MCOUT],LNAGAIN,AGAIN);
    printf("\n");
}

// ================================================================================================
// FUNCTION  : DwPhyApp_PllLockTest()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
int DwPhyApp_PllLockTest(int argc, char *argv[])
{
    uint32_t fcMHz[] = {2412, 2417, 2422, 2427, 2432, 2437, 2442, 2447, 2452, 2457, 2462, 2467, 2472, 2484,
                        4920, 4940, 4960, 4980, 5040, 5060, 5080, 5170, 5180, 5190, 5200, 5210, 5220, 5230, 
                        5240, 5260, 5280, 5300, 5320, 5500, 5520, 5540, 5560, 5580, 5600, 5620, 5640, 5680,
                        5700, 5745, 5765, 5785, 5805};

    uint32_t n, i;
    int32_t locked1, locked2, locked3, AllLocked = 1;
    uint32_t N = sizeof(fcMHz) / sizeof(uint32_t);
    int8_t CLCALADJ, CAPSEL;

    printf("\n");        
    printf("  DwPhy Console Application (%s, %s)\n",__DATE__,__TIME__);
    printf("  PLL Lock Test\n");

    for (n=0; n<N; n++)
    {
        DwPhy_Sleep(pDevInfo);
        DwPhy_SetChannelFreq(pDevInfo, fcMHz[n]);
        DwPhy_Wake(pDevInfo);
        DwPhy_Delay (500);

        locked1 = locked2 = locked3 = 1;
        locked1 = DwPhy_RadioPllIsLocked(pDevInfo);
        CAPSEL       = OS_RegRead(256+ 42)& 0x1F;
        CLCALADJ     = OS_RegRead(256+ 41)& 0x1F; 
        if (CLCALADJ>16) CLCALADJ -= 32; // two's complement

        for (i=0; i<1; i++)
        {
            DwPhy_Delay (1000); DwPhy_Sleep(pDevInfo);   // PHY->Sleep
            DwPhy_Delay (1000); DwPhy_Wake(pDevInfo);    // PHY->Wake
            DwPhy_Delay ( DwPhy_WakeupTime(pDevInfo) );  // wait for the PHY to wake
            locked2 &= DwPhy_RadioPllIsLocked(pDevInfo); // locked after Standby->RX transition
            DwPhy_PllClosedLoopCalibration(pDevInfo);    // RF52 closed-loop VCO calibration
            locked3 &= DwPhy_RadioPllIsLocked(pDevInfo); // locked after manual VCO calibration
        }
        AllLocked = AllLocked & (locked1 & locked2);

        printf("  %3d:%d, %u MHz, Locked = %d,%d,%d, CAPSEL=%u, CLCALADJ=%u, %c\n",n,N,fcMHz[n],
            locked1,locked2,locked3, CAPSEL,CLCALADJ,(locked1&locked2&locked3) ? ' ' : '<');
    }
    if (!AllLocked) 
        printf("  *** PLL FAILURE ON AT LEAST ONE CHANNEL ***\n");
    else
        printf("  All Channels Locked on Wake\n");

    printf("\n");
    return AllLocked;
}

// ================================================================================================
// FUNCTION  : DwPhyApp_PllLockDriver()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
int DwPhyApp_PllLockDriver(int argc, char *argv[])
{
    uint32_t CommandStop  = WIFI_MIB_API_COMMAND_STOP;
    uint32_t CommandStart = WIFI_MIB_API_COMMAND_START;
    uint32_t BssType      = DesiredBSSTypeIs80211SimpleTxMode;
    uint32_t fcMHz[] = {2412, 2417, 2422, 2427, 2432, 2437, 2442, 2447, 2452, 2457, 2462, 2467, 2472, 2484,
                        4920, 4940, 4960, 4980, 5040, 5060, 5080, 5170, 5180, 5190, 5200, 5210, 5220, 5230, 
                        5240, 5260, 5280, 5300, 5320, 5500, 5520, 5540, 5560, 5580, 5600, 5620, 5640, 5680,
                        5700, 5745, 5765, 5785, 5805};

    uint32_t n;
    int32_t Locked, AllLocked = 1;
    uint32_t N = sizeof(fcMHz) / sizeof(uint32_t);
    uint8_t CAPSEL, CH;
    int8_t CLCALADJ;

    printf("\n");        
    printf("  DwPhy Console Application (%s, %s)\n",__DATE__,__TIME__);
    printf("  PLL Lock Test Using Driver Internal Functions\n");

    for (n=0; n<N; n++)
    {
        XSTATUS( WriteMib( MIBOID_DSPGCOMMAND,             &CommandStop ) );
        XSTATUS( WriteMib( MIBOID_BSSTYPE,                 &BssType     ) );
        XSTATUS( WriteMib( MIBOID_CURRENTCHANNELFREQUENCY, fcMHz+n      ) );
        XSTATUS( WriteMib( MIBOID_DSPGCOMMAND,             &CommandStart) );
        DwPhy_Delay (500);

        CH           = OS_RegRead(256+ 12) & 0xFF;
        CAPSEL       = OS_RegRead(256+ 42) & 0x1F;
        CLCALADJ     = OS_RegRead(256+ 41) & 0x1F;
        if (CLCALADJ>16) CLCALADJ -= 32; // two's complement
        Locked = DwPhy_RadioPllIsLocked(pDevInfo);
        AllLocked &= Locked;

        printf("  %3d:%d, %u MHz, CH=%3u, CAPSEL=%2u, CLCALADJ=%2d, Locked = %d %c\n",
            n,N,fcMHz[n],CH,CAPSEL,CLCALADJ,Locked, Locked ? ' ' : '<');
    }
    if (!AllLocked) 
        printf("  *** PLL FAILURE ON AT LEAST ONE CHANNEL ***\n");
    else
        printf("  All Channels Locked on Driver Start\n");

    printf("\n");
    return AllLocked;
}

// ================================================================================================
// FUNCTION  : DwPhyApp_DumpRegisters
// ------------------------------------------------------------------------------------------------
// ================================================================================================
void DwPhyApp_DumpRegisters(int argc, char *argv[])
{
    uint32_t base, offset, addr;
    uint8_t  RadioID;
    
    printf("                              DW52 PHY Register Dump\n");
    printf("\n");
    printf("  addr    |<--------------------------- offset ----------------------->|\n");
    printf("  base     0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F\n");
    printf("------    --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --\n");

    RadioID = OS_RegRead(256+1);
 
    for(base=0x000; base<=0x17F; base+=0x010 )
    {
        printf("0x%04X  ",base);
        for (offset=0; offset<0x10; offset++)
        {
            addr = base + offset;
            printf("  %02X",DwPhy_ReadReg(addr));
        }
        if((base>>4) % 4 == 3) printf("\n\n"); else printf("\n");
    }

    if (RADIOISRF22(RadioID))    
    {
        for(base=0x8000; base<=0x807D; base+=0x010 )
        {
            printf("0x%04X  ",base);
            for (offset=0; offset<0x10; offset++)
            {
                addr = base + offset;
                printf("  %02X",DwPhy_ReadReg(addr));
            }
            if((base>>4) % 4 == 3) printf("\n\n"); else printf("\n");
        }
    }

    for(base=0x300; base<=0x330; base+=0x010 )
    {
        printf("0x%04X  ",base);
        for (offset=0; offset<0x10; offset++)
        {
            addr = base + offset;
            printf("  %02X",DwPhy_ReadReg(addr));
        }
        printf("\n");
    }
    printf("\n");
}

// ================================================================================================
// FUNCTION  : DwPhyApp_SimpleTxMode()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
int32_t DwPhyApp_SimpleTxMode(int argc, char *argv[])
{
    uint32_t CommandStop  = WIFI_MIB_API_COMMAND_STOP;
    uint32_t CommandStart = WIFI_MIB_API_COMMAND_START;
    uint32_t BssType      = DesiredBSSTypeIs80211SimpleTxMode;
    uint32_t fcMHz        = 2412;

    XSTATUS( WriteMib( MIBOID_DSPGCOMMAND,             &CommandStop ) );
    XSTATUS( WriteMib( MIBOID_BSSTYPE,                 &BssType     ) );
    XSTATUS( WriteMib( MIBOID_CURRENTCHANNELFREQUENCY, &fcMHz       ) );
    XSTATUS( WriteMib( MIBOID_DSPGCOMMAND,             &CommandStart) );

    return 0;
}

// ================================================================================================
// FUNCTION   : DwPhyApp_SimpleBroadcast
// ------------------------------------------------------------------------------------------------
// Purpose    : Generate a simple broadcast data packet
// Parameters : pTxPacketBuffer -- Buffer into which the packet data is placed
//              Length          -- PSDU length minus 4 byte FCS field
// ================================================================================================
void DwPhyApp_SimpleBroadcast(uint8_t* pTxPacketBuffer, uint16_t Length)
{
    const uint8_t MacHdr[] = {0x08, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x0E,
                              0x4C, 0x00, 0x00, 0x01, 0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x70, 0x18};
    int i;

    if(Length < 24)
    {
        printf(">>> Length too short\n");
        return ;
    }
    for(i=0; i<24; i++)
        pTxPacketBuffer[i] = MacHdr[i];

    for(i=0; i<Length-24; i++)
        pTxPacketBuffer[24+i] = (uint8_t)(i%256);

} 
// end of DwPhyApp_SimpleBroadcast()

// ================================================================================================
// FUNCTION  : DwPhyApp_TxSingle()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
void DwPhyApp_TxSingle(int argc, char *argv[])
{
    TX_WIFI_PACKET_DESC TxWifiPktDesc = {0};

    uint8_t  TxPacketBuffer[1600] = {0};
    uint32_t Length = strtoul(argv[3], NULL, 0);
    uint32_t Status;

    if(Length<28 || Length >1600)
    {
        printf(">>> Length parameter out of range (28 - 1600)\n");
        return;
    }
    Length -= 4; // reduce for FCS
    DwPhyApp_SimpleBroadcast(TxPacketBuffer, Length);

    TxWifiPktDesc.nextDesc      = NULL;
    TxWifiPktDesc.length        = Length;
    TxWifiPktDesc.packet        = TxPacketBuffer;
    TxWifiPktDesc.flags         = WIFI_PACKET_API_80211_TXFLAGS_USE_PWRLVL_DIRECT;
    TxWifiPktDesc.phyDataRate   = strtoul(argv[2], NULL, 0);
    TxWifiPktDesc.powerLevel    = strtoul(argv[4], NULL, 0);
    TxWifiPktDesc.tsfTimestampOffset = 0;
    TxWifiPktDesc.retries            = 0;
    TxWifiPktDesc.wifiQosAPIHandle   = 0;
    TxWifiPktDesc.txStatusFlags      = 0;

    Status = Wifi80211APISendPkt(DevName, &TxWifiPktDesc);
    if( Status != WIFI_SUCCESS )
        printf(">>> WifiPacketAPISend80211Packet Status %d in %s, line %d\n",Status,__FILE__,__LINE__);
}

// ================================================================================================
// FUNCTION  : DwPhyApp_TxBatch()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
void DwPhyApp_TxBatch(int argc, char *argv[])
{
    dwMibTxBatch_t TxBatchDesc = {0};

    if( argc == 6)
    {
        uint32_t Length = strtoul(argv[3], NULL, 0);

        if(Length<28 || Length > (MIBOID_TXBATCHMODE_MAXPKTSIZE+4))
        {
            printf(">>> Length parameter out of range (28 - %u)\n",MIBOID_TXBATCHMODE_MAXPKTSIZE+4);
            return;
        }

        Length -= 4; // reduce for FCS
        DwPhyApp_SimpleBroadcast(TxBatchDesc.pkt, Length);

        TxBatchDesc.numTransmits = strtoul(argv[5], NULL, 0);
        TxBatchDesc.phyRate      = strtoul(argv[2], NULL, 0);
        TxBatchDesc.pktLen       = Length;
        TxBatchDesc.powerLevel   = strtoul(argv[4], NULL, 0);
        // printf("%d %d %d %d\n", TxBatchDesc.phyRate, Length, TxBatchDesc.powerLevel, TxBatchDesc.numTransmits);
    }
    else
        TxBatchDesc.numTransmits = 0; // stop TxBatch

    if( WriteMib( MIBOID_DBGBATCHTX, &TxBatchDesc ) != 0)
        printf(">>> WriteMib( %d, * ) failed in %s, line %d\n",MIBOID_DBGBATCHTX,__FILE__,__LINE__);
}

// ================================================================================================
// FUNCTION  : DwPhyApp_TxBatchStatus()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
uint32_t DwPhyApp_TxBatchStatus(int argc, char *argv[])
{
    dwMibTxBatch_t TxBatchDesc = {0};

    if( ReadMib( MIBOID_DBGBATCHTX, &TxBatchDesc ) != 0)
        printf(">>> ReadMib( %d, * ) failed in %s, line %d\n",MIBOID_DBGBATCHTX,__FILE__,__LINE__);

    if (argc != 0)
        printf("Tx Batach Status = %d\n", TxBatchDesc.numTransmits);
    
    return TxBatchDesc.numTransmits;
}

// ================================================================================================
// FUNCTION  : DwPhyApp_CheckPendingTxBatch()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
uint32_t DwPhyApp_CheckPendingTxBatch(int argc, char *argv[])
{
    uint32_t j = 0;
    uint32_t status;
    
    status = DwPhyApp_TxBatchStatus(0, argv);
    while (j<40 && status)
    {
        DwPhyApp_TxBatch(0, argv);
        j++;
        OS_Delay(1000);
        status = DwPhyApp_TxBatchStatus(0, argv);
     }
 
    if (j == 40)
        return 1; // fail to stop on-going Tx Batch
    else
        return 0;
}

// ================================================================================================
// FUNCTION  : DwPhyApp_PrintMAC()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
void DwPhyApp_PrintMAC(int argc, char *argv[])
{
    char *addr[15] = { "0x06509004", "0x06509008", "0x0650900C", "0x06509010",
                       "0x06509014", "0x06509018", "0x06509084", "0x065090B4",
                       "0x065090B8", "0x065090D0", "0x06508090", "0x09700204",
                       "0x09700208", "0x0970020C", "0x09700210"}; 

    char *name[15] = {          "MAC_CRTL",    "TXMAC_CRTL",    "TXMAC_STATUS",       "RXMAC_CRTL",
                           "TXMAC_ERRSTAT", "RXMAC_ERRSTAT", "TXFIFO_0_FCOUNT", "REG_PROC_MSC_CTL",
                       "REG_PROC_TRPC_CTL",         "RXCTL",       "TSF_TMRLO",       "MSC_STATUS",
                                "MSC_COMM",   "TRPC_STATUS",       "TRPC_COMM"};
    int i;
    uint32_t address;

    printf("\n");
    for (i=0; i<15; i++)
        printf("%s %-17s = 0x%08X\n", addr[i], name[i], OS_RegRead(strtoul(addr[i], NULL, 0)));

    
    OS_RegWrite(strtoul(addr[13], NULL, 0), strtoul("0x00010000", NULL, 0));
    printf("\n==> 0x00010000 written to 0x0970020C.\n");
    for (i=0; i<10; i++)
        printf("%s %-17s = 0x%08X\n", addr[13], name[13], OS_RegRead(strtoul(addr[13], NULL, 0)));

    
    OS_RegWrite(strtoul(addr[13], NULL, 0), strtoul("0x05380000", NULL, 0));
    printf("\n==> 0x05380000 written to 0x0970020C.\n");
    for (i=0; i<10; i++)
        printf("%s %-17s = 0x%08X\n", addr[13], name[13], OS_RegRead(strtoul(addr[13], NULL, 0)));

    
    OS_RegWrite(strtoul(addr[13], NULL, 0), strtoul("0xFFFF0000", NULL, 0));
    printf("\n==> 0xFFFF0000 written to 0x0970020C.\n");
    for (i=0; i<20; i++)
        printf("%s %-17s = 0x%08X\n", addr[13], name[13], OS_RegRead(strtoul(addr[13], NULL, 0)));

    
    printf("\n");
    address = 0x06502000;
    for (i=0; i<20; i++)
        printf("0x%08X = 0x%08X\n", address, OS_RegRead(address));   

    
    printf("\n");
    address = 0x06504000;
    for (i=0; i<20; i++)
        printf("0x%08X = 0x%08X\n", address, OS_RegRead(address));   

    
    printf("\n");
    address = 0x06508000;
    for (i=0; i<36; i++)
    {
        printf("0x%08X = 0x%08X\n", address, OS_RegRead(address));
        address += 4;
    }
}

// ================================================================================================
// FUNCTION  : DwPhyApp_TxDFS()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
void DwPhyApp_TxDFS(int argc, char *argv[])
{
/*
    TX_WIFI_PACKET_DESC TxWifiPktDesc = {0};

    uint8_t  TxPacketBuffer[1600] = {0};
    uint32_t Length = 1500;
    uint8_t  DataRate = 6;
    uint32_t Status;

    clock_t c1, c2;
    int i;

    Length -= 4; // reduce for FCS
    DwPhyApp_SimpleBroadcast(TxPacketBuffer, Length);

    TxWifiPktDesc.nextDesc      = NULL;
    TxWifiPktDesc.length        = Length;
    TxWifiPktDesc.packet        = TxPacketBuffer;
    TxWifiPktDesc.flags         = WIFI_PACKET_API_80211_TXFLAGS_USE_PWRLVL_DIRECT;
    TxWifiPktDesc.phyDataRate   = DataRate;
    TxWifiPktDesc.powerLevel    = 32;
    TxWifiPktDesc.tsfTimestampOffset = 0;
    TxWifiPktDesc.retries            = 0;
    TxWifiPktDesc.wifiQosAPIHandle   = 0;
    TxWifiPktDesc.txStatusFlags      = 0;

    c2 = clock() + CLOCKS_PER_SEC * 1;  // end time for loop

    while (clock() < c2)
    {
        c1 = clock() + CLOCKS_PER_SEC / 10; // end time after 100 ms
        printf("TxBurst for DFS\n");


        for (i=0; i<15; i++)
        {
            Status = Wifi80211APISendPkt(DevName, &TxWifiPktDesc);
            if( Status != WIFI_SUCCESS )
                printf(">>> WifiPacketAPISend80211Packet Status %d in %s, line %d\n",Status,__FILE__,__LINE__);
        }

        while (clock() < c1)
            ; // wait
    }
*/
system("/opt/TxForDFS &");
}

// ================================================================================================
// FUNCTION  : DwPhyApp_RxSingle()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
void DwPhyApp_RxSingle(int argc, char *argv[])
{
    RX_WIFI_PACKET_DESC rxDesc;
    uint32_t Status, i,j,k;
    uint8_t numRxDesc = 1;

    Status = Wifi80211APIReceivePkt( DevName, &rxDesc, &numRxDesc );
    if( Status != WIFI_SUCCESS )
    {
        printf(">>> Wifi80211APIReceivePkt Status %d in %s, line %d\n",Status,__FILE__,__LINE__);
    }
    else
    {
        if(numRxDesc)
        {
            printf("  - Rate  = %u Mbps, PSDU Length = %u bytes\n",rxDesc.phyDataRate, rxDesc.length+4);
            printf("  - RSSI0 = %d dBm, RSSI1 = %d dBm\n",(int)rxDesc.rssiChannelA-100,(int)rxDesc.rssiChannelB-100);
            printf("  - tsfTimestamp = %u, flags = 0x%04X\n",rxDesc.tsfTimestamp,rxDesc.flags);
            printf("  - PSDU (less FCS)...\n\n");

            for(i=0; i<rxDesc.length; i+=16)
            {
                printf("    ");
                for(j=0, k=i; j<16; j++, k++)
                {
                    if(k<rxDesc.length) 
                        printf("%02X ",rxDesc.packet[k]); 
                    else 
                        printf("   ");
                }
                printf("   ");
                for(j=0, k=i; j<16; j++, k++)
                {
                    if(k<rxDesc.length) 
                        printf("%c", (rxDesc.packet[k]>=32 && rxDesc.packet[k]<127) ? rxDesc.packet[k] : '.'); 
                    else 
                        printf(" ");
                }
                printf("\n");
            }
            printf("\n\n");
        }
        else
            printf("RX Packet: no packets pending\n");
    }
}

// ================================================================================================
// FUNCTION  : DwPhyApp_TxTone()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
void DwPhyApp_TxTone(int argc, char *argv[])
{
    uint8_t Channel = DwPhy_ReadReg(256+12); // radio channel number
    uint8_t Power   = strtoul(argv[2], NULL, 0);

    uint8_t AGain        =(Power & 0x1F);
    uint8_t FixedLNAGain =(Power & 0x20) << 1;
    uint8_t FixedGain    = 0x80;
    uint8_t Reg80        = FixedGain | FixedLNAGain; // combine fields

    DwPhy_WriteReg(  7, 0x0A ); // enable +10 MHz tone generator (also turns off any test mode) 
    DwPhy_WriteReg( 65, AGain); // set InitAGain = Power
    DwPhy_WriteReg( 80, Reg80); // fixed gain (keep Power on LNAGAIN/AGAIN[4:0])
    DwPhy_WriteReg(224, 0xAA ); // radio is always in TX mode (regardless of baseband mode)
    DwPhy_WriteReg(225, 0x3F ); // baesband DACs are always on
    DwPhy_WriteReg(228, 0x0F ); // SW1 in TX position (always)
    DwPhy_WriteReg(230, Channel<241 ? 1 : 4); // turn on PA (5 GHz for channel<241, 2.4 GHz otherwise)
}

// ================================================================================================
// FUNCTION  : DwPhyApp_StopTxTone()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
int DwPhyApp_StopTxTone(int argc, char *argv[])
{
    uint32_t CommandStart = WIFI_MIB_API_COMMAND_START;
    uint32_t CommandStop  = WIFI_MIB_API_COMMAND_STOP;

    uint8_t Buffer[3] = {1,0,0};
    dwMibPhyParameter_t PhyParameter;

    PhyParameter.paramType = WIFI_PHYPARAM_DEFAULTREGISTERS;
    PhyParameter.data = Buffer;
    PhyParameter.dataSize = sizeof(Buffer);

    XSTATUS( WriteMib( MIBOID_DSPGCOMMAND,  &CommandStop ) );
    XSTATUS( WriteMib( MIBOID_PHYPARAMETER, &PhyParameter ) );
    XSTATUS( WriteMib( MIBOID_DSPGCOMMAND,  &CommandStart) );

    return 0;
}

// ================================================================================================
// FUNCTION  : DwPhyApp_SetRxSensitivity
// ------------------------------------------------------------------------------------------------
// ================================================================================================
void DwPhyApp_SetRxSensitivity(int argc, char *argv[])
{
    int8_t dBm = strtol(argv[2], NULL, 0); // sensitivity threshold [dBm]
    if(dBm < 0)
        DwPhy_SetRxSensitivity(pDevInfo, dBm);
    else
        printf(">>> Sensitivity should be a negative number (units are dBm)\n");
}

// ================================================================================================
// FUNCTION  : DwPhyApp_DwPhyEnable()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
void DwPhyApp_DwPhyEnable(int argc, char *argv[])
{
    uint32_t Enable = strtoul(argv[2], NULL, 0); // enable state
    DwPhyEnableFn( pDevInfo, Enable );
}

// ================================================================================================
// FUNCTION  : DwPhyApp_CapSelBypass()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
void DwPhyApp_CapSelBypass(int argc, char *argv[])
{
    uint8_t CAPSEL = DwPhy_ReadReg(256+42) & 0x1F; // capacitor bank setting for current channel
    uint8_t X      = DwPhy_ReadReg(256+35);        // register to be modified

    X = (X & 0xC0) | 0x20 | CAPSEL; // set CAPSELBYPEN=1 and CAPSELBYP=CAPSEL
    DwPhy_WriteReg(256+35, X); 
}

// ================================================================================================
// FUNCTION  : DwPhyApp_Debug071012
// ------------------------------------------------------------------------------------------------
// Test function to reproduce the OFDM ACK to an 11 Mbps CCK packet
// ================================================================================================
int32_t DwPhyApp_Debug071012(int argc, char *argv[])
{
    uint32_t data;

    // Set the MAC Address (same as BERLab)
    {
        uint32_t CommandStop  = WIFI_MIB_API_COMMAND_STOP;
        uint32_t CommandStart = WIFI_MIB_API_COMMAND_START;
        uint8_t  MacAddr[6]   = {0x00, 0x03, 0x90, 0x01, 0x23, 0x45};

        XSTATUS( WriteMib( MIBOID_DSPGCOMMAND, &CommandStop ) );
        XSTATUS( WriteMib( MIBOID_MACADDRESS,   MacAddr) );
        XSTATUS( WriteMib( MIBOID_DSPGCOMMAND, &CommandStart) );
    }
    // Set SimpleTxMode
    {
        uint32_t CommandStop  = WIFI_MIB_API_COMMAND_STOP;
        uint32_t CommandStart = WIFI_MIB_API_COMMAND_START;
        uint32_t BssType      = DesiredBSSTypeIs80211SimpleTxMode;

        XSTATUS( WriteMib( MIBOID_DSPGCOMMAND, &CommandStop ) );
        XSTATUS( WriteMib( MIBOID_BSSTYPE,     &BssType     ) );
        XSTATUS( WriteMib( MIBOID_DSPGCOMMAND, &CommandStart) );
    }
    // Initialize the PHY
    {
        int32_t status = DwPhy_Initialize(pDevInfo, 0);
        if(status != 0) printf(" Status = 0x%08X at %s line %d\n",status,__FILE__,__LINE__);
    }
    // Wake the PHY (if not already)
    //
    OS_RegWrite(0xFFFFFE3D, 1); // assert DW_PHYEnB;
    OS_Delay(300); // make sure we are awake

    // Set the channel
    {
        uint32_t FcMHz = 2412;
        int32_t status = DwPhy_SetChannelFreq(pDevInfo, FcMHz);
        if(status != 0) printf(" Status = 0x%08X at %s line %d\n",status,__FILE__,__LINE__);
    }

    // Set modulation compatibility
    //
    DwPhy_SetRxMode(pDevInfo, DWPHY_RXMODE_802_11G);

    // Reduce the signal detect sensitivity
    // ...reduces impact from interference
    //
    DwPhy_SetRxSensitivity(pDevInfo, -62); // set to Pr >= -62 dBm

    // ...Now, the good stuff
    //
    data = OS_RegRead(0x06509008);
    OS_RegWrite(0x06509008, data & 0xFE); // disable TX MAC

    data = OS_RegRead(0x06509004);
    OS_RegWrite(0x06509004, data & 0xCFFF); // should be in 802.11b mode

    OS_RegWrite(0x065090A8, 0xEF); // allow all 11b data rates for ACK

    data = OS_RegRead(0x06509008);
    OS_RegWrite(0x06509008, data | 0x01); // enable TX MAC
    
    return 0;
}

// ================================================================================================
// DwPhyApp_Debug071026a
// Test function to create a large transmit burst that slows down the processor
// ================================================================================================
int32_t DwPhyApp_Debug071026a(int argc, char *argv[])
{
    uint32_t data;

    // Set the MAC Address (same as BERLab)
    {
        uint32_t CommandStop  = WIFI_MIB_API_COMMAND_STOP;
        uint32_t CommandStart = WIFI_MIB_API_COMMAND_START;
        uint8_t  MacAddr[6]   = {0x00, 0x03, 0x90, 0x01, 0x23, 0x45};

        XSTATUS( WriteMib( MIBOID_DSPGCOMMAND, &CommandStop ) );
        XSTATUS( WriteMib( MIBOID_MACADDRESS,   MacAddr) );
        XSTATUS( WriteMib( MIBOID_DSPGCOMMAND, &CommandStart) );
    }
    // Set SimpleTxMode
    {
        uint32_t CommandStop  = WIFI_MIB_API_COMMAND_STOP;
        uint32_t CommandStart = WIFI_MIB_API_COMMAND_START;
        uint32_t BssType      = DesiredBSSTypeIs80211SimpleTxMode;

        XSTATUS( WriteMib( MIBOID_DSPGCOMMAND, &CommandStop ) );
        XSTATUS( WriteMib( MIBOID_BSSTYPE,     &BssType     ) );
        XSTATUS( WriteMib( MIBOID_DSPGCOMMAND, &CommandStart) );
    }
    // Initialize the PHY
    {
        int32_t status = DwPhy_Initialize(pDevInfo, 0);
        if(status != 0) printf(" Status = 0x%08X at %s line %d\n",status,__FILE__,__LINE__);
    }
    // Wake the PHY (if not already)
    //
    OS_RegWrite(0xFFFFFE3D, 1); // assert DW_PHYEnB;
    OS_Delay(300); // make sure we are awake

    // Set the channel
    {
        uint32_t FcMHz = 2412;
        int32_t status = DwPhy_SetChannelFreq(pDevInfo, FcMHz);
        if(status != 0) printf(" Status = 0x%08X at %s line %d\n",status,__FILE__,__LINE__);
    }

    // Set modulation compatibility
    //
    DwPhy_SetRxMode(pDevInfo, DWPHY_RXMODE_802_11G);

    // Reduce the signal detect sensitivity
    // ...reduces impact from interference
    //
    DwPhy_SetRxSensitivity(pDevInfo, -62); // set to Pr >= -62 dBm

    // ...Now, the good stuff
    //
    data = OS_RegRead(0x06509008);
    OS_RegWrite(0x06509008, data & 0xFE); // disable TX MAC

    data = OS_RegRead(0x06509004);
    OS_RegWrite(0x06509004, data & 0xCFFF); // should be in 802.11b mode

    OS_RegWrite(0x065090A8, 0xEF); // allow all 11b data rates for ACK

    data = OS_RegRead(0x06509008);
    OS_RegWrite(0x06509008, data | 0x01); // enable TX MAC
    
    return 0;
}

// ================================================================================================
// DwPhyApp_Debug080508a
// Test function to disable RF52 closed-loop calibration at the end of DwPhy_SetChannel_RF52a()
// ================================================================================================
int32_t DwPhyApp_Debug080508a(int argc, char *argv[])
{
    uint8_t Buffer[12];
    dwMibPhyParameter_t PhyParameter;
    uint8_t i;

    Buffer[0] = (uint8_t)35;
    Buffer[1] = (uint8_t) 1;
    Buffer[2] = (uint8_t)0x80;

    for (i=3; i<12; i++) Buffer[i] = 0;

    PhyParameter.paramType = WIFI_PHYPARAM_REGISTERSBYBAND;
    PhyParameter.data = Buffer;
    PhyParameter.dataSize = sizeof(Buffer);

    XSTATUS( WriteMib( MIBOID_PHYPARAMETER, &PhyParameter ) );
    
    return 0;
}

// ================================================================================================
// DwPhyApp_Debug080508b
// Test function to poll the CAPSEL value 
// ================================================================================================
int32_t DwPhyApp_Debug080508b(int argc, char *argv[])
{
    uint8_t data0 = 0, data; int i;

    for (i=0; i<6000; i++)
    {
        data = OS_RegReadBB(256+42) & 0x1F;
        if (i && (data != data0))
            printf("\n%d --> %d\n",data0,data);
        else
            printf(".");
        data0 = data;
        DwPhy_Delay (10000);
    }
    printf("\n");
    return 0;
}

// ================================================================================================
// DwPhyApp_Debug080508c
// Increase VCO calibration window by 1 count
// ================================================================================================
int32_t DwPhyApp_Debug080508c(int argc, char *argv[])
{
    uint8_t Buffer[24];
    dwMibPhyParameter_t PhyParameter;
    uint8_t i;

    Buffer[0] = (uint8_t)25;
    Buffer[1] = (uint8_t) 1;
    Buffer[2] = (uint8_t)0x1F;

    for (i=3; i<12; i++) Buffer[i] = 0x17;

    Buffer[12] = (uint8_t)26;
    Buffer[13] = (uint8_t) 1;
    Buffer[14] = (uint8_t)0x1F;

    for (i=15; i<24; i++) Buffer[i] = 0x05;

    PhyParameter.paramType = WIFI_PHYPARAM_REGISTERSBYBAND;
    PhyParameter.data = Buffer;
    PhyParameter.dataSize = sizeof(Buffer);

    XSTATUS( WriteMib( MIBOID_PHYPARAMETER, &PhyParameter ) );
    
    return 0;
}

// ================================================================================================
// DwPhyApp_Debug080508d
// Increase VCO calibration window by 2 counts
// ================================================================================================
int32_t DwPhyApp_Debug080508d(int argc, char *argv[])
{
    uint8_t Buffer[24];
    dwMibPhyParameter_t PhyParameter;
    uint8_t i;

    Buffer[0] = (uint8_t)25;
    Buffer[1] = (uint8_t) 1;
    Buffer[2] = (uint8_t)0x1F;

    for (i=3; i<12; i++) Buffer[i] = 0x18;

    Buffer[12] = (uint8_t)26;
    Buffer[13] = (uint8_t) 1;
    Buffer[14] = (uint8_t)0x1F;

    for (i=15; i<24; i++) Buffer[i] = 0x04;

    PhyParameter.paramType = WIFI_PHYPARAM_REGISTERSBYBAND;
    PhyParameter.data = Buffer;
    PhyParameter.dataSize = sizeof(Buffer);

    XSTATUS( WriteMib( MIBOID_PHYPARAMETER, &PhyParameter ) );
    
    return 0;
}

// ================================================================================================
// DwPhyApp_Debug080508e
// Decrease VCO calibration window by 2 counts
// ================================================================================================
int32_t DwPhyApp_Debug080508e(int argc, char *argv[])
{
    uint8_t Buffer[24];
    dwMibPhyParameter_t PhyParameter;
    uint8_t i;

    Buffer[0] = (uint8_t)25;
    Buffer[1] = (uint8_t) 1;
    Buffer[2] = (uint8_t)0x1F;

    for (i=3; i<12; i++) Buffer[i] = 0x14;

    Buffer[12] = (uint8_t)26;
    Buffer[13] = (uint8_t) 1;
    Buffer[14] = (uint8_t)0x1F;

    for (i=15; i<24; i++) Buffer[i] = 0x08;

    PhyParameter.paramType = WIFI_PHYPARAM_REGISTERSBYBAND;
    PhyParameter.data = Buffer;
    PhyParameter.dataSize = sizeof(Buffer);

    XSTATUS( WriteMib( MIBOID_PHYPARAMETER, &PhyParameter ) );
    
    return 0;
}

// ================================================================================================
// DwPhyApp_Debug110107
// Debug non-stopping Tx Batch problem on DW74
// ------------------------------------------------------------------------------------------------
// ================================================================================================
int32_t DwPhyApp_Debug110107(int argc, char *argv[])
{
    uint32_t status;
    uint32_t i, k;
    uint32_t fcMHz[] = {2412, 2417, 2422, 2427, 2432, 2437, 2442, 2447, 2452, 2457, 2462, 2467, 2472, 2484};
    uint8_t  RadioID;
    char     *burst1[6] = {"", "", "6",  "150", "63", "1000000000"};
    char     *burst2[6] = {"", "", "6", "1500", "63", "1500"};

    // Set SimpleTxMode
    {
        uint32_t CommandStop  = WIFI_MIB_API_COMMAND_STOP;
        uint32_t CommandStart = WIFI_MIB_API_COMMAND_START;
        uint32_t BssType      = DesiredBSSTypeIs80211SimpleTxMode;
        uint32_t fcMHz        = 2412;
        
        XSTATUS( WriteMib( MIBOID_DSPGCOMMAND,             &CommandStop ) );
        XSTATUS( WriteMib( MIBOID_BSSTYPE,                 &BssType     ) );
        XSTATUS( WriteMib( MIBOID_CURRENTCHANNELFREQUENCY, &fcMHz       ) );
        XSTATUS( WriteMib( MIBOID_DSPGCOMMAND,             &CommandStart) );
    }

    // Initialize the PHY
    {
        int32_t status = DwPhy_Initialize(pDevInfo, 0);
        if(status != 0) printf(" Status = 0x%08X at %s line %d\n",status,__FILE__,__LINE__);
    }

    // Wake the PHY (if not already)
    //
    OS_RegWrite(0xFFFFFE3D, 1); // assert DW_PHYEnB;
    OS_Delay(300); // make sure we are awake

    RadioID = OS_RegRead(256+1);
    DwPhy_SetRxSensitivity(pDevInfo, -50);

    // Make sure no pending Tx Batch exists
    //
    DwPhyApp_TxBatch(0, argv);
    status = DwPhyApp_CheckPendingTxBatch(argc, argv);
    if (status)
    {
        printf("Cannot start debugging.\n");
        return 0;
    }

    // Start test
    //
    for (k=0; k<14; k++) // channel loop
    {
        DwPhyApp_TxBatch(0, argv); // stop any on-going Tx Batch
        OS_Delay(2100);

        status = DwPhyApp_CheckPendingTxBatch(argc, argv);
        if (status)
        {
            printf("TxBatch pending timeout 1.\n");
            return 0;
        }

        OS_Delay(2000);        
        printf("channel = %d (%d MHz)\n", k+1, fcMHz[k]);
        DwPhy_SetChannelFreq(pDevInfo, fcMHz[k]);

        if (RADIOISRF22(RadioID))
        {               
            status = DwPhy_CalibrateTXDCOC_RF22(pDevInfo);
            if (status != 0) printf(" Status = 0x%08X at %s line %d\n",status,__FILE__,__LINE__);
            
            status = DwPhy_CalibrateLOFT_RF22(pDevInfo);
            if (status != 0) printf(" Status = 0x%08X at %s line %d\n",status,__FILE__,__LINE__);
            
            status = DwPhy_CalibrateIQ_RF22(pDevInfo);
            if (status != 0) printf(" Status = 0x%08X at %s line %d\n",status,__FILE__,__LINE__);
        }
        
        DwPhy_Wake(pDevInfo);
        
        DwPhyApp_TxBatch(6, burst1); // start burst 1 for TX ALC
        OS_Delay(1000000);
  
        for (i=0; i<4; i++) // burst 2 loop
        {
            DwPhyApp_TxBatch(0, argv); // stop any on-going Tx Batch
            if (i==0)
                OS_Delay(350000);
            else
                OS_Delay(500000);

            status = DwPhyApp_CheckPendingTxBatch(argc, argv);
            if (status)
            {
                printf("TxBatch pending timeout 2 (i = %d)\n", i);
                return 0;
            }

            DwPhyApp_TxBatch(6, burst2); // start burst 2
            OS_Delay(350000);
        }
    }

    return 0;
}

// ================================================================================================
// DwPhyApp_Debug110201
// Debug non-stopping Tx Batch problem on DW74 in a simplified scenario
// ------------------------------------------------------------------------------------------------
// ================================================================================================
int32_t DwPhyApp_Debug110201(int argc, char *argv[])
{
    uint32_t status;
    uint32_t i;
    char     *burst[6] = {"", "", "6", "1500", "63", "1500"};

    // Set SimpleTxMode
    {
        uint32_t CommandStop  = WIFI_MIB_API_COMMAND_STOP;
        uint32_t CommandStart = WIFI_MIB_API_COMMAND_START;
        uint32_t BssType      = DesiredBSSTypeIs80211SimpleTxMode;
        uint32_t fcMHz        = 2412;
        
        XSTATUS( WriteMib( MIBOID_DSPGCOMMAND,             &CommandStop ) );
        XSTATUS( WriteMib( MIBOID_BSSTYPE,                 &BssType     ) );
        XSTATUS( WriteMib( MIBOID_CURRENTCHANNELFREQUENCY, &fcMHz       ) );
        XSTATUS( WriteMib( MIBOID_DSPGCOMMAND,             &CommandStart) );
    }

    // Initialize the PHY
    {
        int32_t status = DwPhy_Initialize(pDevInfo, 0);
        if(status != 0) printf(" Status = 0x%08X at %s line %d\n",status,__FILE__,__LINE__);
    }

    // Wake the PHY (if not already)
    //
    OS_RegWrite(0xFFFFFE3D, 1); // assert DW_PHYEnB;
    OS_Delay(300);              // make sure we are awake

    DwPhy_SetRxSensitivity(pDevInfo, -50);

    // Make sure no pending Tx Batch exists
    //
    DwPhyApp_TxBatch(0, argv);
    status = DwPhyApp_CheckPendingTxBatch(argc, argv);
    if (status)

    {
        printf("Cannot start debugging.\n");
        return 0;
    }

    // the following 3 writes simplify the scenario
    DwPhy_WriteReg   (  4, 1);    // 11a mode
    DwPhy_WriteReg   (223, 1);    // PWR_CCA only
    DwPhy_WriteRegBit(103, 2, 1); // IdleSDEnB
    
    // either case will help the TxBatch problem
    if (0)
        DwPhy_WriteRegBit(233, 1, 0); // ADCFCAL1 = 0, i.e. disable ADC Cal from Sleep->Standby->Rx
    else
    {   // longer gap between ADC Cal till DFE enabling (i.e. power CCA active)
        DwPhy_WriteReg(214, 5); // DelayModem
        DwPhy_WriteReg(215, 5); // DelayDFE
    }
    
    // Start test
    //
    for (i=0; i<10; i++) 
    {
        printf("----- Loop = %d -----\n", i);
        DwPhy_Sleep(pDevInfo);
        OS_Delay(10000);
        
        DwPhy_Wake(pDevInfo);
        OS_Delay(10000);
        
        DwPhyApp_TxBatch(6, burst); // start Tx burst 
        OS_Delay(30000);
  
        DwPhyApp_TxBatch(0, argv); // stop any on-going Tx Batch

        status = DwPhyApp_CheckPendingTxBatch(argc, argv);
        if (status)
        {
            printf("TxBatch pending timeout (Loop = %d)\n", i);
            DwPhyApp_PrintMAC(argc, argv);       
            return 0;
        }
    }

    return 0;
}

// ================================================================================================
// FUNCTION  : CommandMatch()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
int CommandMatch(int argc, char *argv[], char *Cmd, int NumParam)
{
    if(argc >= 2)
    {
        if( strcmp(argv[1], Cmd) == 0 )
        {
            if( argc == (NumParam + 2) )
                return 1;
            else
                printf("\n>>> Wrong number of arguments for option \"%s\"\n\n",Cmd);
        }
    }
    return 0;
}

// ================================================================================================
// FUNCTION   : EndianTest()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
void EndianTest(void)
{
    uint32_t X4 = 0x12345678;
    uint16_t X2 = 0x1234;
    uint8_t *p;


    p = (uint8_t *)(&X4);
    printf("X4 by byte = %02X %02X %02X %02X\n",p[0],p[1],p[2],p[3]);

    p = (uint8_t *)(&X2);
    printf("X2 by byte = %02X %02X\n",p[0],p[1]);
    printf("\n");
}

// ================================================================================================
// FUNCTION   : main()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
int main(int argc, char *argv[])
{
    int result = 0;

    // Initialize VWAL
    //
    if( !VWAL_Init() )
    {
        printf(" VWAL Initialization Failed.\n\n");
        return -1;
    }
    // Startup and define the DW_PHYEnB control function
    //
    DwPhy_Startup(pDevInfo);
    DwPhy_SetDwPhyEnableFn(pDevInfo, (DwPhyEnableFn_t)DwPhyEnableFn);

    // Parse Command-Line Parameters --------------------------------------------------------------
    // 
    if( 0 ) ; // dummy "if" so the stuff below looks pretty

    else if(CommandMatch(argc, argv, "-adcfcal",0))      DwPhy_CalibrateDataConv(pDevInfo);
    else if(CommandMatch(argc, argv, "-reset",0))        DwPhy_Reset            (pDevInfo);
    else if(CommandMatch(argc, argv, "-initialize",0))   DwPhy_Initialize       (pDevInfo, 0);
    else if(CommandMatch(argc, argv, "-clc",0))          DwPhy_PllClosedLoopCalibration(pDevInfo);

    else if(CommandMatch(argc, argv, "-capselbyp",0))    DwPhyApp_CapSelBypass    (argc, argv);
    else if(CommandMatch(argc, argv, "-dwphyenable",1))  DwPhyApp_DwPhyEnable     (argc, argv);
    else if(CommandMatch(argc, argv, "-dumpreg",0))      DwPhyApp_DumpRegisters   (argc, argv);
    else if(CommandMatch(argc, argv, "-info", 0))        DwPhyApp_DisplayInfo     (argc, argv);
    else if(CommandMatch(argc, argv, "-plllocktest", 0))  result=DwPhyApp_PllLockTest  (argc, argv);
    else if(CommandMatch(argc, argv, "-plllockdriver",0)) result=DwPhyApp_PllLockDriver(argc, argv);
    else if(CommandMatch(argc, argv, "-r", 1))           DwPhyApp_ReadRegister    (argc, argv);
    else if(CommandMatch(argc, argv, "-rx",0))           DwPhyApp_RxSingle        (argc, argv);
    else if(CommandMatch(argc, argv, "-rxsensitivity",1))DwPhyApp_SetRxSensitivity(argc, argv);
    else if(CommandMatch(argc, argv, "-setchannel",1))   DwPhyApp_SetChannel      (argc, argv);
    else if(CommandMatch(argc, argv, "-simpletxmode",0)) DwPhyApp_SimpleTxMode    (argc, argv);
    else if(CommandMatch(argc, argv, "-testmode", 1))    DwPhyApp_SetTestMode     (argc, argv);
    else if(CommandMatch(argc, argv, "-tx",3))           DwPhyApp_TxSingle        (argc, argv);
    else if(CommandMatch(argc, argv, "-txbatch",4))      DwPhyApp_TxBatch         (argc, argv);
    else if(CommandMatch(argc, argv, "-txbatchstop",0))  DwPhyApp_TxBatch         (argc, argv);
    else if(CommandMatch(argc, argv, "-txbatchstatus",0))DwPhyApp_TxBatchStatus   (argc, argv);    
    else if(CommandMatch(argc, argv, "-txdfs",0))        DwPhyApp_TxDFS           (argc, argv);
    else if(CommandMatch(argc, argv, "-txtone",1))       DwPhyApp_TxTone          (argc, argv);
    else if(CommandMatch(argc, argv, "-stoptxtone",0))   DwPhyApp_StopTxTone      (argc, argv);
    else if(CommandMatch(argc, argv, "-w", 2))           DwPhyApp_WriteRegister   (argc, argv);
    else if(CommandMatch(argc, argv, "-rf22cal",1))      DwPhyApp_CalibrateRF22   (argc, argv);

    else if(CommandMatch(argc, argv, "-debug071012",0))  DwPhyApp_Debug071012     (argc, argv);
    else if(CommandMatch(argc, argv, "-debug071026a",0)) DwPhyApp_Debug071026a    (argc, argv);
    else if(CommandMatch(argc, argv, "-debug080508a",0)) DwPhyApp_Debug080508a    (argc, argv);
    else if(CommandMatch(argc, argv, "-debug080508b",0)) DwPhyApp_Debug080508b    (argc, argv);
    else if(CommandMatch(argc, argv, "-debug080508c",0)) DwPhyApp_Debug080508c    (argc, argv);
    else if(CommandMatch(argc, argv, "-debug080508d",0)) DwPhyApp_Debug080508d    (argc, argv);
    else if(CommandMatch(argc, argv, "-debug080508e",0)) DwPhyApp_Debug080508e    (argc, argv);
    else if(CommandMatch(argc, argv, "-debug110107", 0)) DwPhyApp_Debug110107     (argc, argv);    
    else if(CommandMatch(argc, argv, "-debug110201", 0)) DwPhyApp_Debug110201     (argc, argv);
    
    else 
    {
        // Display Help ---------------------------------------------------------------------------
        // 
        printf("\n");        
        printf("DwPhy Console Application (%s, %s)\n",__DATE__,__TIME__);
        printf("Copyright 2007-2010 DSP Group, Inc. All rights reserved.\n");
        printf("\n");
        printf("Usage  : %s [command]\n",argv[0]);
        printf("\n");
        printf("Commands List:\n");
        printf("  -adcfcal               calibrate data converters\n");
        printf("  -capselbyp             bypass the PLL calibration with current settings\n");
        printf("  -clc                   pll closed loop calibration (for RF52A321, A421)\n");
        printf("  -dumpreg               display baseband+radio registers\n");
        printf("  -dwphyenable <enable>  enable or disable the PHY\n");
        printf("  -info                  display device information\n");
        printf("  -initialize            initialize the PHY\n");
        printf("  -plllocktest           check for pll lock on all channels\n");
        printf("  -plllockdriver         check for pll lock on all channels using driver functions\n");
        printf("  -r <addr>              reads the register at <addr>\n");
        printf("  -reset                 reset the baseband\n");
        printf("  -rxsensitivity <dBm>   set receiver sensitivity\n");
        printf("  -setchannel <FcMHz>    set channel to frequency <FcMHz>\n");
        printf("  -testmode <n>          enables the baseband test port with TestMode = <n>\n");
        printf("  -txtone <pwr>          enables continuous TX with +10 MHz tone\n");
        printf("  -stoptxtone            disables continuous TX tone\n");        
        printf("  -w <addr> <data>       writes <data> to the register at <addr>\n");
        printf("  -rf22cal <type>        enable RF22 calibration <1>:TXDCOC, <2>:LOFT, <3>:IQ\n");
        printf("\n");
        printf("Commands for 80211SimpleTxMode\n");
        printf("  -rx                               receive and display a buffered packet\n");
        printf("  -simpletxmode                     set BSS type as 80211SimpleTxMode\n");
        printf("  -tx      <mbps> <len> <pwr>       transmit a simple data packet\n");
        printf("  -txbatch <mbps> <len> <pwr> <num> continuous transmit of a simple data packet\n");
        printf("  -txbatchstop                      stops a txbatch operation\n");
        printf("  -txbatchstatus                    displays the status of the last txbatch operation (0:done, 1:on-going)\n");        
        printf("Debug Tests\n");
        printf("  -debug080508a   disable PLL closed-loop calibration after channel selection\n");
        printf("  -debug080508b   poll CAPSEL for 60 seconds\n");
        printf("  -debug080508c   increase VCO calibration window by 2x1\n");
        printf("  -debug080508d   increase VCO calibration window by 2x2\n");
        printf("  -debug080508e   decrease VCO calibration window by 2x2\n");
        printf("  -debug110107    debug non-stopping Tx Batch problem on DW74\n");
        printf("  -debug110201    debug non-stopping Tx Batch problem on DW74 in a simplified scenario\n");        
        printf("\n");
    }
    // 
    // Shutdown VWAL and Exit
    // 
    DwPhy_Shutdown(pDevInfo);
    VWAL_Shutdown();

    return result;
}
// end of main()

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------

// 2008-03-25 Added write to MIBOID_CURRENTCHANNELFREQUENC in DwPhyApp_SimpleTxMode
// 2008-06-18 Added DwPhyApp_PllLockDriver
// 2010-12-26 Added DwPhyApp_CalibrateRF22 [SM]
// 2011-01-07 Added DwPhyApp_Debug110107 [SM]

