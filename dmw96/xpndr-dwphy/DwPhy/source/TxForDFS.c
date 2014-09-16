//--< TxForDFS.c >---------------------------------------------------------------------------------
//=================================================================================================
//
//  Simple console application to generate a 30% TX channel load for ETSI DFS tests
//  Copyright 2007 DSP Group, Inc. All rights reserved.
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

static char          DevName[16] = {"wifi0"}; // target device

#define DEBUG printf(" ### FILE: %s, LINE %d\n",__FILE__,__LINE__);

#define XSTATUS(fn)                                            \
{                                                              \
    if((fn)<0)                                                 \
    {                                                          \
        printf(">>> Error at %s line %d\n",__FILE__,__LINE__); \
        return -1;                                             \
    }                                                          \
}                                                              \
//--------------------------------------------------------------

// ================================================================================================
// FUNCTION   : SimpleBroadcast
// ------------------------------------------------------------------------------------------------
// Purpose    : Generate a simple broadcast data packet
// Parameters : pTxPacketBuffer -- Buffer into which the packet data is placed
//              Length          -- PSDU length minus 4 byte FCS field
// ================================================================================================
void SimpleBroadcast(uint8_t* pTxPacketBuffer, uint16_t Length)
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

// ================================================================================================
// FUNCTION  : SimpleBurst
// ------------------------------------------------------------------------------------------------
// ================================================================================================
void SimpleBurst(int argc, char *argv[])
{
    TX_WIFI_PACKET_DESC TxWifiPktDesc = {0};

    uint8_t  TxPacketBuffer[1600] = {0};
    uint32_t Length = 1500;
    uint8_t  DataRate = 6;
    uint32_t Status;

    clock_t c1, c2;
    int i;

    unsigned long T = strtoul(argv[2],0,0);

    Length -= 4; // reduce for FCS
    SimpleBroadcast(TxPacketBuffer, Length);

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

    c2 = clock() + CLOCKS_PER_SEC * T;  // end time for loop

    while (clock() < c2)
    {
        c1 = clock() + CLOCKS_PER_SEC / 10; // end time after 100 ms

        for (i=0; i<15; i++)
        {
            Status = Wifi80211APISendPkt(DevName, &TxWifiPktDesc);
            if( Status != WIFI_SUCCESS ) 
            {
                printf(">>> WifiPacketAPISend80211Packet Status %d in %s, line %d\n",Status,__FILE__,__LINE__);
                return ;
            }
        }

        while (clock() < c1)
            ; // wait
    }
}

// ================================================================================================
// FUNCTION  : SimpleSpreadBurst
// ------------------------------------------------------------------------------------------------
// ================================================================================================
void SimpleSpreadBurst(int argc, char *argv[])
{
    TX_WIFI_PACKET_DESC TxWifiPktDesc = {0};

    uint8_t  TxPacketBuffer[1600] = {0};
    uint32_t Length = 1500;
    uint8_t  DataRate = 6;
    uint32_t Status;

    clock_t c1, c2;
    int i;

    unsigned long T = strtoul(argv[2],0,0);
    unsigned long dt = strtoul(argv[3],0,0);

    Length -= 4; // reduce for FCS
    SimpleBroadcast(TxPacketBuffer, Length);

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

    c2 = clock() + CLOCKS_PER_SEC * T;  // end time for loop

    while (clock() < c2)
    {
        c1 = clock() + CLOCKS_PER_SEC / 10; // end time after 100 ms

        for (i=0; i<15; i++)
        {
            Status = Wifi80211APISendPkt(DevName, &TxWifiPktDesc);
            if( Status != WIFI_SUCCESS ) 
            {
                printf(">>> WifiPacketAPISend80211Packet Status %d in %s, line %d\n",Status,__FILE__,__LINE__);
                return ;
            }
            {
                int j; clock_t c;
                for(j=0; j<800; j++)
                    c = clock();
            }
        }

        while (clock() < c1)
            ; // wait
    }
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
// FUNCTION   : main()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
int main(int argc, char *argv[])
{
    // Initialize VWAL
    //
    if( !VWAL_Init() )
    {
        printf(" VWAL Initialization Failed.\n\n");
        return -1;
    }
    // Parse Command-Line Parameters --------------------------------------------------------------
    // 
    if( 0 ) ; // dummy "if" so the stuff below looks pretty

    else if(CommandMatch(argc, argv, "-burst30" ,1))  SimpleBurst        (argc, argv);
    else if(CommandMatch(argc, argv, "-spread30",2))  SimpleSpreadBurst  (argc, argv);

    else 
    {
        // Display Help ---------------------------------------------------------------------------
        // 
        printf("\n");        
        printf("TxForDFS Console Application (%s, %s)\n",__DATE__,__TIME__);
        printf("Copyright 2007 DSP Group, Inc. All rights reserved.\n");
        printf("\n");
        printf("Usage  : %s [command]\n",argv[0]);
        printf("\n");
        printf("Commands List:\n");
        printf("  -burst30  <t>       30%%/100 ms TX for a total time of t(seconds)\n");
        printf("  -spread30 <t> <dt>  30%% for t(seconds) with dt(usec) between packets\n");
        printf("\n");
    }
    // 
    // Shutdown VWAL and Exit
    // 
    VWAL_Shutdown();

    return 0;
}
// end of main()

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------
