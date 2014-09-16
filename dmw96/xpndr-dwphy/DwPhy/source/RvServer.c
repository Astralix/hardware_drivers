//--< RvServer.c >---------------------------------------------------------------------------------
//=================================================================================================
//
//  Remote VWAL Server
//  Copyright 2007-2010 DSP Group, Inc. All rights reserved.
//
//  Written by Barrett Brickner
//
//  Notes: Currently RVWAL does not accomodate differences in byte order or structure packing
//         between the client and server. Byte order on the ARM seems to match Intel. Building
//         RvClient with MSVC 6.0 and RvServer with gcc for ARM yields the same structure packing.
//         It would probably be a good idea to enforce the latter with compiler options.
//
//=================================================================================================
//-------------------------------------------------------------------------------------------------

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include "RvPacket.h"
#include "RvSocket.h"
#include "RvUtil.h"
#include "vwal.h"

// ================================================================================================
// GLOBAL PARAMETERS
// ================================================================================================
char     DevName[16] = {"wifi0"};   // target device
int      InPhyTestMode = 0;         // indicates wheter PHY Test mode has been set
uint16_t ServerPort = 30000;        // sockets port
uint8_t  TxPacketBuffer[RVWAL_MAXPKT] = {0}; // buffer for TX packet
uint16_t TxPacketBufferLength = 0;  // length of packet in the buffer
RX_WIFI_PACKET_DESC rxDesc;         // receive packet descriptor

// ================================================================================================
// MACROS FOR DEBUGGING AND ERROR CHECKING
// ================================================================================================
#define VPRINTF if(RvUtil_Verbose()) RvPrintf
#define DEBUG   RvPrintf("# FILE: %s, LINE: %d\n",__FILE__,__LINE__);

#define  STATUS(arg) RVCHECK(arg)
#define XSTATUS(arg) { if(STATUS(arg) < RVWAL_SUCCESS) return RVWAL_ERROR; }

#define CHECKSEND(fn) if( (fn) != RVWAL_SUCCESS)                                                \
                      {                                                                         \
                          RvPrintf(" Error Sending Status back to Client. Aborting...\n");      \
                          RunServer = 0;                                                        \
                      }

#ifndef min
#define min(a,b) ((b)<(a) ? (b):(a))
#endif

// ================================================================================================
// INTERNAL FUNCTION PROTOTYPES
// ================================================================================================
RvStatus_t RvServer( void );
void       RvServerAtExit(void);
RvStatus_t RvServerPhyTestMode(void);
RvStatus_t TxSinglePacket(TX_WIFI_PACKET_DESC* pWifiPacket);
RvStatus_t GetPerCounters(RvPerFrame_t* pFrame);
RvStatus_t ReadRegister  (RvRegFrame_t* pFrame);
RvStatus_t WriteRegister (RvRegFrame_t* pFrame);
RvStatus_t ReadMib       (uint16_t mibObjectID, void *pMibValue);
RvStatus_t WriteMib      (uint16_t mibObjectID, void *pMibValue);

// ================================================================================================
// FUNCTION   : main()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
int main( int argc, char **argv )
{
    int i;
    int DisplayUsage = 0, BadOptions = 0;
    char *ServerName = argv[0];
    
    // --------------------------
    // Title and copyright notice
    // --------------------------
    RvPrintf("\n");
    RvPrintf(" Remote VWAL Server (rvwald)\n");
    RvPrintf(" Copyright 2007 DSP Group, Inc. All Rights Reserved.\n");
    RvPrintf(" Build %s, %s\n",__DATE__,__TIME__);
    RvPrintf("\n");

    // -----------------------------
    // Retrieve command line options
    // -----------------------------
    for(i=1; i<argc; i++)
    {
        if (!strcmp("-p", argv[i]) && i<argc-1) 
        {
            ServerPort = atoi (argv[++i]);
        }
        else if (!strcmp("-dev",argv[i])  && i<argc-1) 
        {
            strncpy(DevName, argv[++i], 16);
            DevName[15] = '\0'; // force termination of the string
        }
        else if (!strcmp("-v",argv[i])) 
        {
            RvUtil_SetVerbose(1);
        }
        else
        {
            DisplayUsage = 1;
            BadOptions = 1;
        }
    }
    if(DisplayUsage)
    {
        while(strchr(ServerName,'/')) ServerName++; // skip the pathname

        RvPrintf(" Usage: %s [options]\n",ServerName);
        RvPrintf(" Options:\n");
        RvPrintf("   -dev <device name>\n");
        RvPrintf("   -p <port number>\n");
        RvPrintf("   -v (set to verbose)\n");
        RvPrintf("\n");
        RvPrintf(" Example : %s -dev wifi0 -p 30000\n",ServerName);
        RvPrintf("\n");
    }
    if(!BadOptions)
    {
        atexit( RvServerAtExit ); // server shutdown on normal exit
        RvServer();               // run actual server
    }
    return 0;
}
// end of main()

// ================================================================================================
// FUNCTION   : RvServer()
// ------------------------------------------------------------------------------------------------
// Purpose    : Server top-level: receive command via sockets and dispatch instructions
// Parameters : none
// ================================================================================================
RvStatus_t RvServer(void)
{
    int RunServer = 1;
    char *FrameBuffer;
    int ReceivedLength;
    RvFrame_t *pFrame; // pointer cast as a generic frame

    // --------------------------------------------
    // Allocate memory for the sockets frame buffer
    // --------------------------------------------
    FrameBuffer = (char *)malloc(RVWAL_MAXBUF);
    if(!FrameBuffer)
    {
        RvPrintf("\n ERROR in %s at line %d\n Unable to allocate FrameBuffer memory\n\n",__FILE__,__LINE__);
        return RVWAL_ERROR;
    }
    pFrame = (RvFrame_t *)FrameBuffer;

    // ---------------
    // Initialize VWAL
    // ---------------
    if( !VWAL_Init() )
    {
        RvPrintf(" VWAL Initialization Failed.\n\n");
        exit(-1);
    }
    // ------------------------------------------
    // Create the Server Socket and Bind the Port
    // ------------------------------------------
    RvSocketCreateServer(ServerPort);

    // ----------------------------------
    // Loop Forever (or until !RunServer)
    // ----------------------------------
    while(RunServer)
    {
        if(RvSocketAcceptConnection() != RVWAL_SUCCESS)
        {
            RvPrintf("\n RVWAL ERROR: Unable to accept socket connections.\n\n");
            return RVWAL_ERROR;
        }
        // ---------------------------------------------------------------------------
        // Loop Until a Connection Error Occurs (normally when the client disconnects)
        // ---------------------------------------------------------------------------
        while (RvSocketReceive(FrameBuffer, RVWAL_MAXBUF, &ReceivedLength) == RVWAL_SUCCESS)
        {
            VPRINTF(" > Received Command: 0x%08X (%d bytes)\n",pFrame->Command,ReceivedLength);

            // --------------------------------------
            // Check for More Data (Fragmented Frame)
            // --------------------------------------
            if (ReceivedLength > 64)
            {
                if (ReceivedLength < pFrame->Cmd.FrameSize)
                {
                    VPRINTF("   - Attempting to received the packet remainder\n");
                    XSTATUS( RvSocketReceiveRemainder(FrameBuffer, pFrame->Cmd.FrameSize, &ReceivedLength) );
                }
            }
            // ---------------------------------------------------
            // Execute the command specified in the received frame
            // ---------------------------------------------------
            switch (pFrame->Command)
            {
                case 0x000000FF:
                case 0x0000FFFF:
                case 0xFFFF0000:
                case 0xFFFFFFFF:
                {
                    pFrame->Reg.Command = RVWAL_CHKWORD1;
                    pFrame->Reg.Status  = RVWAL_CHKWORD2;
                    pFrame->Reg.Addr    = RVWAL_CHKWORD3;
                    pFrame->Reg.Data    = RVWAL_CHKWORD4;
                    RvSocketSend( FrameBuffer, sizeof(RvRegFrame_t), NULL );
                    break;
                }

                case RVWAL_REG_READ:
                {
                    RvRegFrame_t *pFrame = (RvRegFrame_t *)FrameBuffer;
                    pFrame->Status = ReadRegister( pFrame ); 
                    CHECKSEND( RvSocketSend( FrameBuffer, sizeof(RvRegFrame_t), NULL ) );
                    break;
                }

                case RVWAL_REG_WRITE:
                {
                    RvRegFrame_t *pFrame = (RvRegFrame_t *)FrameBuffer;
                    pFrame->Status = WriteRegister( pFrame ); 
                    CHECKSEND( RvSocketSend( FrameBuffer, sizeof(RvRegFrame_t), NULL ) );
                    break;
                }

                case RVWAL_MIB_READ:
                {
                    RvMibFrame_t *pFrame = (RvMibFrame_t *)FrameBuffer;
                    pFrame->Status = ReadMib( pFrame->MibObjectID, pFrame->MibObject );
                    CHECKSEND( RvSocketSend( FrameBuffer, sizeof(RvMibFrame_t), NULL ) );
                    break;
                }

                case RVWAL_MIB_WRITE:
                {
                    RvMibFrame_t *pFrame = (RvMibFrame_t *)FrameBuffer;
                    pFrame->Status = WriteMib( pFrame->MibObjectID, pFrame->MibObject );
                    CHECKSEND( RvSocketSend( FrameBuffer, sizeof(RvMibFrame_t), NULL ) );
                    break;
                }

                case RVWAL_CMD_PHYTESTMODE:
                {
                    RvCmdFrame_t *pFrame = (RvCmdFrame_t *)FrameBuffer;
                    pFrame->Status = RvServerPhyTestMode();
                    CHECKSEND( RvSocketSend( FrameBuffer, sizeof(RvCmdFrame_t), NULL ) );
                    break;
                }

                case RVWAL_CMD_PERCOUNTERS:
                {
                    RvPerFrame_t *pFrame = (RvPerFrame_t *)FrameBuffer;
                    pFrame->Status = GetPerCounters( pFrame );
                    CHECKSEND( RvSocketSend( FrameBuffer, sizeof(RvPerFrame_t), NULL ) );
                    break;
                }

                case RVWAL_CMD_SEEDRANDOM:
                {
                    RvRegFrame_t *pFrame = (RvRegFrame_t *)FrameBuffer;
                    pFrame->Status = RvPacket_SetRandomSeed( pFrame->Data );
                    CHECKSEND( RvSocketSend( FrameBuffer, sizeof(RvRegFrame_t), NULL ) );
                    break;
                }

                case RVWAL_CMD_SHUTDOWN:
                {
                    RvCmdFrame_t *pFrame = (RvCmdFrame_t *)FrameBuffer;
                    RunServer = 0;
                    pFrame->Status = RVWAL_SUCCESS;
                    CHECKSEND( RvSocketSend( FrameBuffer, sizeof(RvCmdFrame_t), NULL ) );

                    break;
                }

                case RVWAL_PKT_TX_SINGLE:
                {
                    RvTxPacketFrame_t *pFrame = (RvTxPacketFrame_t *)FrameBuffer;

                    if( RvPacket_SimpleBroadcast(TxPacketBuffer, pFrame->Length, pFrame->PacketType) == RVWAL_SUCCESS )
                    {
                        TX_WIFI_PACKET_DESC TxWifiPktDesc = {0};

                        TxWifiPktDesc.nextDesc      = NULL;
                        TxWifiPktDesc.length        = pFrame->Length;
                        TxWifiPktDesc.packet        = TxPacketBuffer;
                        TxWifiPktDesc.flags         = WIFI_PACKET_API_80211_TXFLAGS_USE_PWRLVL_DIRECT
                                                    | (pFrame->LongPreamble ? WIFI_PACKET_API_80211_TXFLAGS_USE_LONG_PREAMBLE : 0)
                                                    | (pFrame->PhyRate>=128 ? WIFI_PACKET_API_80211_TXFLAGS_USE_MCS_INDEX     : 0);
                        TxWifiPktDesc.phyDataRate   = (pFrame->PhyRate & 0x7F);
                        TxWifiPktDesc.powerLevel    = pFrame->PowerLevel;
                        TxWifiPktDesc.tsfTimestampOffset = 0;
                        TxWifiPktDesc.retries            = 0;
                        TxWifiPktDesc.wifiQosAPIHandle   = 0;
                        TxWifiPktDesc.txStatusFlags      = 0;

                        pFrame->Status = TxSinglePacket( &TxWifiPktDesc );
                        TxPacketBufferLength = pFrame->Length;

                    }
                    else pFrame->Status = RVWAL_ERROR;

                    CHECKSEND( RvSocketSend( FrameBuffer, sizeof(RvTxPacketFrame_t), NULL ) );
                    break;                  
                }

                case RVWAL_PKT_TX_MULTIPLE:
                {
                    RvTxMultiplePacketFrame_t *pFrame = (RvTxMultiplePacketFrame_t *)FrameBuffer;
                    TX_WIFI_PACKET_DESC TxWifiPktDesc[RVWAL_NUMPKT] = {{0}};
                    uint8_t *PktBuffer[RVWAL_NUMPKT];
                    int i;

                    pFrame->Status = RVWAL_SUCCESS;

                    if (pFrame->PacketCount <=RVWAL_NUMPKT)
                    {
                        for (i=0; i<pFrame->PacketCount; i++)
                        {
                            PktBuffer[i] = (uint8_t *)malloc(4+pFrame->Packet[i].Length);
                            if (!PktBuffer[i])
                            {
                                RvPrintf("\n ERROR in %s at line %d\n Unable to allocate PktBuffer[%d] memory\n\n",__FILE__,__LINE__,i);
                                return RVWAL_ERROR;
                            }
                        }
                        for (i=0; i<pFrame->PacketCount; i++)
                        {
                            if( RvPacket_SimpleBroadcast(PktBuffer[i], pFrame->Packet[i].Length, pFrame->Packet[i].PacketType) == RVWAL_SUCCESS )
                            {
                                TxWifiPktDesc[i].nextDesc      = (i < (pFrame->PacketCount-1)) ? (TxWifiPktDesc+i+1) : NULL;
                                TxWifiPktDesc[i].length        = pFrame->Packet[i].Length;
                                TxWifiPktDesc[i].packet        = PktBuffer[i];
                                TxWifiPktDesc[i].flags         = WIFI_PACKET_API_80211_TXFLAGS_USE_PWRLVL_DIRECT
                                                               | (pFrame->Packet[i].LongPreamble ? WIFI_PACKET_API_80211_TXFLAGS_USE_LONG_PREAMBLE : 0)
                                                               | (pFrame->Packet[i].PhyRate>=128 ? WIFI_PACKET_API_80211_TXFLAGS_USE_MCS_INDEX     : 0);
                                TxWifiPktDesc[i].phyDataRate   = (pFrame->Packet[i].PhyRate & 0x7F);
                                TxWifiPktDesc[i].powerLevel    = pFrame->Packet[i].PowerLevel;
                                TxWifiPktDesc[i].tsfTimestampOffset = 0;
                                TxWifiPktDesc[i].retries            = 0;
                                TxWifiPktDesc[i].wifiQosAPIHandle   = 0;
                                TxWifiPktDesc[i].txStatusFlags      = 0;
                            }
                            else 
                                pFrame->Status = RVWAL_ERROR;
                        }
                        if (pFrame->Status != RVWAL_ERROR)
                        {
                            pFrame->Status = TxSinglePacket( TxWifiPktDesc );
                        }
                        for (i=0; i<pFrame->PacketCount; i++)
                        {
                            free(PktBuffer[i]);
                        }
                    }
                    else
                    {
                        RvPrintf("\n ERROR: PacketCount = %u\n",pFrame->PacketCount);
                        pFrame->Status = RVWAL_ERROR;
                    }

                    CHECKSEND( RvSocketSend( FrameBuffer, sizeof(RvTxPacketFrame_t), NULL ) );
                    break;                  
                }                        

                case RVWAL_PKT_TX_BUFFER:
                {
                    TX_WIFI_PACKET_DESC TxWifiPktDesc = {0};
    
                    RvTxPacketFrame_t *pFrame = (RvTxPacketFrame_t *)FrameBuffer;

                    if(TxPacketBufferLength)
                    {
                        TxWifiPktDesc.nextDesc      = NULL;
                        TxWifiPktDesc.length        = TxPacketBufferLength;
                        TxWifiPktDesc.packet        = TxPacketBuffer;
                        TxWifiPktDesc.flags         = WIFI_PACKET_API_80211_TXFLAGS_USE_PWRLVL_DIRECT
                                                    | (pFrame->LongPreamble ? WIFI_PACKET_API_80211_TXFLAGS_USE_LONG_PREAMBLE : 0)
                                                    | (pFrame->PhyRate>=128 ? WIFI_PACKET_API_80211_TXFLAGS_USE_MCS_INDEX     : 0);
                        TxWifiPktDesc.phyDataRate   = (pFrame->PhyRate & 0x7F);
                        TxWifiPktDesc.powerLevel    = pFrame->PowerLevel;
                        TxWifiPktDesc.tsfTimestampOffset = 0;
                        TxWifiPktDesc.retries            = 0;
                        TxWifiPktDesc.wifiQosAPIHandle   = 0;
                        TxWifiPktDesc.txStatusFlags      = 0;

                        pFrame->Status = TxSinglePacket( &TxWifiPktDesc );
                    }
                    else pFrame->Status = RVWAL_ERROR; // no packet in buffer

                    CHECKSEND( RvSocketSend( FrameBuffer, sizeof(RvTxPacketFrame_t), NULL ) );
                    break;                  
                }

                case RVWAL_PKT_TX_BATCH:
                {
                    dwMibTxBatch_t   TxBatchDesc = {0};
                    RvTxPacketFrame_t* pFrame = (RvTxPacketFrame_t *)FrameBuffer;

                    if( RvPacket_SimpleBroadcast(TxBatchDesc.pkt, pFrame->Length, pFrame->PacketType) == RVWAL_SUCCESS )
                    {
                        TxBatchDesc.numTransmits = pFrame->NumTransmits;
                        TxBatchDesc.phyRate      = (pFrame->PhyRate & 0x7F)
                                                 | (pFrame->LongPreamble ? WIFI_BATCHTX_RATE_USE_LONG_PREAMBLE : 0)
                                                 | (pFrame->PhyRate>=128 ? WIFI_BATCHTX_RATE_MCS_IDX : 0);
                        TxBatchDesc.pktLen       = pFrame->Length;
                        TxBatchDesc.powerLevel   = pFrame->PowerLevel;

                        pFrame->Status = WriteMib( MIBOID_DBGBATCHTX, &TxBatchDesc );
                    }
                    else pFrame->Status = RVWAL_ERROR;

                    CHECKSEND( RvSocketSend( FrameBuffer, sizeof(RvTxPacketFrame_t), NULL ) );
                    break;                  
                }

                case RVWAL_PKT_TX_BATCH_BUFFER:
                {
                    dwMibTxBatch_t   TxBatchDesc = {0};
                    RvTxPacketFrame_t* pFrame = (RvTxPacketFrame_t *)FrameBuffer;

                    TxBatchDesc.numTransmits = pFrame->NumTransmits;
                    TxBatchDesc.phyRate      = (pFrame->PhyRate & 0x7F)
                                             | (pFrame->LongPreamble ? WIFI_BATCHTX_RATE_USE_LONG_PREAMBLE : 0)
                                             | (pFrame->PhyRate>=128 ? WIFI_BATCHTX_RATE_MCS_IDX : 0);
                    TxBatchDesc.pktLen       = TxPacketBufferLength;
                    TxBatchDesc.powerLevel   = pFrame->PowerLevel;

                    memcpy( TxBatchDesc.pkt, TxPacketBuffer, TxPacketBufferLength );

                    pFrame->Status = WriteMib( MIBOID_DBGBATCHTX, &TxBatchDesc );

                    CHECKSEND( RvSocketSend( FrameBuffer, sizeof(RvTxPacketFrame_t), NULL ) );
                    break;                  
                }

                case RVWAL_PKT_TX_BATCH_STOP:
                {
                    dwMibTxBatch_t TxBatchDesc = {0};
    
                    RvTxPacketFrame_t *pFrame = (RvTxPacketFrame_t *)FrameBuffer;
                    pFrame->Status = WriteMib( MIBOID_DBGBATCHTX, &TxBatchDesc );
                    CHECKSEND( RvSocketSend( FrameBuffer, sizeof(RvTxPacketFrame_t), NULL ) );
                    break;                  
                }

                case RVWAL_PKT_TX_BATCH_STATUS:
                {
                    dwMibTxBatch_t TxBatchDesc = {0};    
                    RvTxPacketFrame_t *pFrame = (RvTxPacketFrame_t *)FrameBuffer;
                    
                    pFrame->Status = ReadMib( MIBOID_DBGBATCHTX, &TxBatchDesc );
                    pFrame->NumTransmits = TxBatchDesc.numTransmits;
                    CHECKSEND( RvSocketSend( FrameBuffer, sizeof(RvTxPacketFrame_t), NULL ) );
                    break;                  
                }
                
                case RVWAL_BUF_LOAD_TX:
                {
                    RvBufferFrame_t *pFrame = (RvBufferFrame_t *)FrameBuffer;

                    if( pFrame->Length>=10 && pFrame->Length<4092 ) 
                    {
                        memset( TxPacketBuffer, 0, RVWAL_MAXPKT );                // clear the buffer
                        memcpy( TxPacketBuffer, pFrame->Buffer, pFrame->Length ); // copy in the new data
                        pFrame->Status = RVWAL_SUCCESS;
                        TxPacketBufferLength = pFrame->Length;
                    }
                    else pFrame->Status = RVWAL_ERROR_PARAMETER_RANGE;
                    
                    CHECKSEND( RvSocketSend( FrameBuffer, sizeof(RvBufferFrame_t), NULL ) );
                    break;                  
                }

                case RVWAL_PKT_RX_SINGLE:
                {
                    uint32_t Status;
                    uint8_t numRxDesc = 1;

                    RvRxPacketFrame_t *pFrame = (RvRxPacketFrame_t *)FrameBuffer;
                    pFrame->Status = RVWAL_SUCCESS;

                    if(pFrame->Status == RVWAL_SUCCESS)
                    {
                        Status = Wifi80211APIReceivePkt( DevName, &rxDesc, &numRxDesc );
                        if( Status != WIFI_SUCCESS )
                        {
                            RvPrintf(" Wifi80211APIReceivePkt Status %d in %s, line %d\n",Status,__FILE__,__LINE__);
                            pFrame->Status = RVWAL_ERROR;
                        }
                        else
                        {
                            VPRINTF("RVWAL_PK_RX_SINGLE: numRxDesc = %d, tsfTimestamp = %u\n",numRxDesc,rxDesc.tsfTimestamp);
                            pFrame->Status  = RVWAL_SUCCESS;
    
                            if(numRxDesc > 0)
                            {
                                pFrame->Length  = rxDesc.length;
                                pFrame->PhyRate = rxDesc.phyDataRate;
                                pFrame->RSSI0   = rxDesc.rssiChannelA;
                                pFrame->RSSI1   = rxDesc.rssiChannelB;
                                pFrame->flags   = rxDesc.flags;
                                pFrame->tsfTimestamp = rxDesc.tsfTimestamp;
                                memcpy(pFrame->Packet, rxDesc.packet, min( rxDesc.length, RVWAL_MAXPKT ));
                            }
                            else
                            {
                                pFrame->Length  = 0;
                                pFrame->PhyRate = 0;
                                pFrame->RSSI0   = 0;
                                pFrame->RSSI1   = 0;
                                pFrame->flags   = 0;
                                pFrame->tsfTimestamp = 0;
                            }
                        }
                    }
                    CHECKSEND( RvSocketSend( FrameBuffer, sizeof(RvRxPacketFrame_t), NULL ) );
                    break;                  
                }

                case RVWAL_SYSTEM_COMMAND:
                {
                    RvBufferFrame_t *pFrame = (RvBufferFrame_t *)FrameBuffer;

                    int err = system ((char *)(pFrame->Buffer));

                    if (err != 0)
                    {
                        RvPrintf(" system(\"%s\")\n",pFrame->Buffer);
                        RvPrintf(" errno = %d\n",errno);
                        pFrame->Status = RVWAL_ERROR;
                    }
                    else pFrame->Status = RVWAL_SUCCESS;
                    
                    CHECKSEND( RvSocketSend( FrameBuffer, sizeof(RvBufferFrame_t), NULL ) ); 
                    break;
                }

                case RVWAL_PKT_RX_RSSI:
                {
                    RvRxPacketFrame_t *pFrame = (RvRxPacketFrame_t *)FrameBuffer;
                    uint8_t numRxDesc = 1;

                    pFrame->Status = RVWAL_SUCCESS;
                    pFrame->Length = 0;
                    
                    while( numRxDesc && pFrame->Length < (RVWAL_MAXPKT))
                    {
                        uint32_t Status = Wifi80211APIReceivePkt( DevName, &rxDesc, &numRxDesc );
                        if( Status != WIFI_SUCCESS )
                        {
                            RvPrintf(" Wifi80211APIReceivePkt Status %d in %s, line %d\n",Status,__FILE__,__LINE__);
                            return RVWAL_ERROR;
                        }
                        if( numRxDesc > 0 )
                        {
                            pFrame->Packet[ pFrame->Length++ ] = rxDesc.rssiChannelA;
                            pFrame->Packet[ pFrame->Length++ ] = rxDesc.rssiChannelB;                          
                        }
                    }   
                    CHECKSEND( RvSocketSend( FrameBuffer, sizeof(RvRxPacketFrame_t), NULL ) );
                    break;                  
                }

                case RVWAL_RX_RSSI_VALUES:
                {
                    RvBufferFrame_t *pFrame = (RvBufferFrame_t *)FrameBuffer;
                    RvRegFrame_t RegFrame[2];

                    pFrame->Length = 0;

                    RegFrame[0].Addr =  22; // RSSI0
                    RegFrame[1].Addr =  23; // RSSI1

                    while ((pFrame->Length+1) < RVWAL_MAXPKT)
                    {
                        pFrame->Status = ReadRegister(RegFrame+0);
                        if (pFrame->Status != RVWAL_SUCCESS) break;

                        pFrame->Status = ReadRegister(RegFrame+1);
                        if (pFrame->Status != RVWAL_SUCCESS) break;

                        pFrame->Buffer[ pFrame->Length++ ] = RegFrame[0].Data;
                        pFrame->Buffer[ pFrame->Length++ ] = RegFrame[1].Data;
                    }
                    CHECKSEND( RvSocketSend( FrameBuffer, sizeof(RvBufferFrame_t), NULL ) );
                    break;
                }

                case RVWAL_REG_READ_MULTIPLE:
                {
                    RvBufferFrame_t *pFrame = (RvBufferFrame_t *)FrameBuffer;
                    RvRegFrame_t RegFrame;

                    pFrame->Length = 0;

                    RegFrame.Addr =  *((uint32_t *)pFrame->Buffer);

                    while (pFrame->Length < RVWAL_MAXPKT)
                    {
                        pFrame->Status = ReadRegister(&RegFrame);
                        pFrame->Buffer[ pFrame->Length++ ] = RegFrame.Data & 0xFF;
                        if (pFrame->Status != RVWAL_SUCCESS) break;
                    }
                    CHECKSEND( RvSocketSend( FrameBuffer, sizeof(RvBufferFrame_t), NULL ) );
                    break;
                }

                case RVWAL_MACADDRESS_READ:
                {
                    RvMibFrame_t *pFrame = (RvMibFrame_t *)FrameBuffer;
                    pFrame->Status = ReadMib( MIBOID_MACADDRESS, pFrame->MibObject );
                    CHECKSEND( RvSocketSend( FrameBuffer, sizeof(RvMibFrame_t), NULL ) );
                    break;
                }

                case RVWAL_MACADDRESS_WRITE:
                {
                    uint32_t CommandStop  = WIFI_MIB_API_COMMAND_STOP;
                    uint32_t CommandStart = WIFI_MIB_API_COMMAND_START;

                    RvMibFrame_t *pFrame = (RvMibFrame_t *)FrameBuffer;
                    XSTATUS( WriteMib( MIBOID_DSPGCOMMAND, &CommandStop ) );
                    XSTATUS( WriteMib( MIBOID_MACADDRESS, pFrame->MibObject) );
                    XSTATUS( WriteMib( MIBOID_DSPGCOMMAND, &CommandStart) );
                    pFrame->Status = RVWAL_SUCCESS;
                    CHECKSEND( RvSocketSend( FrameBuffer, sizeof(RvMibFrame_t), NULL ) );
                    break;
                }

                case RVWAL_REG_READ_RAM256:
                {
                    uint32_t i,m, M;
                    RvRegFrame_t DataRegFrame[4], AddrRegFrame;

                    RvRamFrame_t *pFrame = (RvRamFrame_t *)FrameBuffer;

                    for (m=0; m<4; m++)
                    {
                        if (pFrame->DataAddr[m] != 0) {
                            DataRegFrame[m].Addr = pFrame->DataAddr[m];
                            M = m + 1;
                        }
                        else DataRegFrame[m].Data = 0;
                    }
                    AddrRegFrame.Addr = pFrame->IndexAddr;
                    
                    for (i=0; i<256; i++)
                    {
                        AddrRegFrame.Data = i;
                        pFrame->Status = WriteRegister(&AddrRegFrame);
                        if (pFrame->Status != RVWAL_SUCCESS) break;
                        
                        for (m=0; m<M; m++)
                        {
                            pFrame->Status = ReadRegister(DataRegFrame+m);
                            if (pFrame->Status != RVWAL_SUCCESS) break;
                        }
                        pFrame->Data[i] = (DataRegFrame[0].Data <<  0)
                                        | (DataRegFrame[1].Data <<  8)
                                        | (DataRegFrame[2].Data << 16)
                                        | (DataRegFrame[3].Data << 24);
                    }
                    CHECKSEND( RvSocketSend( FrameBuffer, sizeof(RvRamFrame_t), NULL ) );
                    break;
                }

                default:
                {
                    RvPrintf(" Undefined Command 0x%08X in %s, line %d\n",pFrame->Command,__FILE__,__LINE__);
                    RvPrintf(" >>> ReceivedLength = %d\n",ReceivedLength);
                    ((RvCmdFrame_t *)pFrame)->Status = RVWAL_ERROR_UNDEFINED_COMMAND;
                    CHECKSEND( RvSocketSend( FrameBuffer, sizeof(RvCmdFrame_t), NULL ) );
                }
            }
        } // while(RvSocketReceive...

        RvSocketCloseConnection();

    } // while(RunServer)

    return 0;
}
// end of RvServer()

// ================================================================================================
// FUNCTION   : RvServerAtExit()
// ------------------------------------------------------------------------------------------------
// Purpose    : Handle normal exit
// ================================================================================================
void RvServerAtExit( void )
{
    VWAL_Shutdown();
    RvPrintf(" rvwald: exit.\n\n");
}
// end of RvServerAtExit()

// ================================================================================================
// FUNCTION   : RvServerCheckConnectionState
// ------------------------------------------------------------------------------------------------
// Purpose    : Wait for the desired connection state
// Parameters : TargetState -- see WifiApi.h
// ================================================================================================
RvStatus_t RvServerCheckConnectionState(uint32_t TargetState)
{
    uint32_t ConnectionState = ~TargetState;
    double tEnd = (double)clock() + 2.0 * (double)CLOCKS_PER_SEC;

    while ( (ConnectionState != TargetState) && ((double)clock() < tEnd) )
        XSTATUS( ReadMib( MIBOID_CONNECTIONSTATE, &ConnectionState) );

    if (ConnectionState != TargetState)
    {
        RvPrintf(" Timeout waiting for connection state %d\n",TargetState);
        return RVWAL_ERROR;
    }
    return RVWAL_SUCCESS;
}
// end of RvServerCheckConnectionState

// ================================================================================================
// FUNCTION   : RvServerPhyTestMode()
// ------------------------------------------------------------------------------------------------
// Purpose    : Enable PHY Test Mode (used by BERLab)
// Parameters : none
// ================================================================================================
RvStatus_t RvServerPhyTestMode(void)
{
    VPRINTF(" RvServerPhyTestMode()\n");
    if( !InPhyTestMode )
    {
        uint32_t CommandStop  = WIFI_MIB_API_COMMAND_STOP;
        uint32_t CommandStart = WIFI_MIB_API_COMMAND_START;
        uint32_t BssType      = DesiredBSSTypeIs80211SimpleTxMode;
        uint32_t fcMHz        = 2412;

        XSTATUS( WriteMib( MIBOID_DSPGCOMMAND,             &CommandStop ) );
        XSTATUS( RvServerCheckConnectionState(WIFI_MIB_API_CONNECTION_STATE_STOPPED) );

        XSTATUS( WriteMib( MIBOID_BSSTYPE,                 &BssType     ) );
        XSTATUS( WriteMib( MIBOID_CURRENTCHANNELFREQUENCY, &fcMHz       ) );

        XSTATUS( WriteMib( MIBOID_DSPGCOMMAND,             &CommandStart) );
        XSTATUS( RvServerCheckConnectionState(WIFI_MIB_API_CONNECTION_STATE_CONNECTED) );

        InPhyTestMode = 1;
    }
    return RVWAL_SUCCESS;
} 
// end of RvClientPhyTestMode()

// ================================================================================================
// FUNCTION   : GetPerCounters()
// ------------------------------------------------------------------------------------------------
// Purpose    : Retrieve data for BERLab PER testing
// Parameters : pFrame -- pointer to frame (to be populated with MIB objects)
// ================================================================================================
RvStatus_t GetPerCounters( RvPerFrame_t* pFrame )
{
    uint8_t numRxDesc = 1;
    uint32_t Status, n = 0;

    XSTATUS( ReadMib( MIBOID_BSSTYPE,           &(pFrame->BSSType        ) ) );
    XSTATUS( ReadMib( MIBOID_CONNECTIONSTATE,   &(pFrame->ConnectionState) ) );
    XSTATUS( ReadMib( MIBOID_DFS,               &(pFrame->DFS            ) ) );

    while( numRxDesc )
    {
        Status = Wifi80211APIReceivePkt( DevName, &rxDesc, &numRxDesc );
        if( Status != WIFI_SUCCESS )
        {
            RvPrintf(" Wifi80211APIReceivePkt Status %d in %s, line %d\n",Status,__FILE__,__LINE__);
            return RVWAL_ERROR;
        }
        n += numRxDesc;
    }   
    if(n) // if any packets were received....
    {
        uint8_t IsMCS     = rxDesc.flags & WIFI_PACKET_API_80211_RXFLAGS_MCS_INDEX ? 1:0;
        pFrame->Length    = rxDesc.length;
        pFrame->RxBitRate = rxDesc.phyDataRate | (IsMCS ? 0x80 : 0);
        pFrame->RSSI0     = rxDesc.rssiChannelA;
        pFrame->RSSI1     = rxDesc.rssiChannelB;
    }
    else
    {
        pFrame->Length    = 0;
        pFrame->RxBitRate = 0;
        pFrame->RSSI0     = 0;
        pFrame->RSSI1     = 0;
    }
    XSTATUS( ReadMib( MIBOID_RECEIVEDFRAGCOUNT, &(pFrame->ReceivedFragmentCount) ) );
    XSTATUS( ReadMib( MIBOID_FCSERRORCOUNT,     &(pFrame->FCSErrorCount  ) ) );         

    return RVWAL_SUCCESS;
} 
// end of GetPerCounters()

// ================================================================================================
// FUNCTION   : TxSinglePacket()
// ------------------------------------------------------------------------------------------------
// Purpose    : Transmit a single packet using the Wifi Packet API
// Parameters : 
// ================================================================================================
RvStatus_t TxSinglePacket( TX_WIFI_PACKET_DESC* pTxWifiPktDesc )
{
    uint32_t Status;
    VPRINTF("   - TxSinglePacket: %d bytes at rate %d\n",pTxWifiPktDesc->length,pTxWifiPktDesc->phyDataRate);

    Status = Wifi80211APISendPkt(DevName, pTxWifiPktDesc);
    if( Status != WIFI_SUCCESS )
    {
        RvPrintf(" WifiPacketAPISend80211Packet Status %d in %s, line %d\n",Status,__FILE__,__LINE__);
        return RVWAL_ERROR;
    }
    return RVWAL_SUCCESS;
}
// end of TxSinglePacket()

// ================================================================================================
// FUNCTION   : ReadMib()
// ------------------------------------------------------------------------------------------------
// Purpose    : Reads the MIB using VWAL
// Parameters : MibObjectID -- defined in WifiApi.h
//              pMibValue   -- pointer to buffer for returned value (type depends on mibObjectID)
// ================================================================================================
RvStatus_t ReadMib( uint16_t MibObjectID, void *pMibValue )
{
    mibObjectContainer_t MibObjectContainer[] = {
        {MibObjectID, WIFI_MIB_API_OPERATION_READ, 0, pMibValue},
    };
    int MibApiStatus = WifiMibAPI(DevName, MibObjectContainer, 1);

    if( MibApiStatus != WIFI_SUCCESS )
    {
        RvPrintf(" ReadMib( %d, * )\n",MibObjectID);
        RvPrintf(" > WifiMibAPI Status %d in %s, line %d\n",MibApiStatus,__FILE__,__LINE__);
        return RVWAL_ERROR;
    }
    return RVWAL_SUCCESS;
}
// end of ReadMib()

// ================================================================================================
// FUNCTION   : WriteMib()
// ------------------------------------------------------------------------------------------------
// Purpose    : Writes the MIB using VWAL
// Parameters : MibObjectID -- defined in WifiApi.h
//              pMibValue   -- pointer to buffer for the write data (type depends on mibObjectID)
// ================================================================================================
RvStatus_t WriteMib( uint16_t MibObjectID, void *pMibValue )
{
    mibObjectContainer_t MibObjectContainer[] = {
        {MibObjectID, WIFI_MIB_API_OPERATION_WRITE, 0, pMibValue},
    };
    int MibApiStatus = WifiMibAPI(DevName, MibObjectContainer, 1);
        
    if( MibApiStatus != WIFI_SUCCESS )
    {
        RvPrintf(" WriteMib( %d, * ), MibValue = 0x%08X...\n",MibObjectID, *((uint32_t *)pMibValue));
        RvPrintf(" > WifiMibAPI Status %d in %s, line %d\n",MibApiStatus,__FILE__,__LINE__);
        return RVWAL_ERROR;
    }
    return RVWAL_SUCCESS;
}
// end of WriteMib()

// ================================================================================================
// FUNCTION   : ReadRegister()
// ------------------------------------------------------------------------------------------------
// Purpose    : Read the register specified by pFrame->Address and return its value in pFrame->Data
// Parameters : pFrame -- Pointer to RegFrame
// ================================================================================================
RvStatus_t ReadRegister(RvRegFrame_t* pFrame)
{
    if(   WriteMib(MIBOID_MAC_REG_ACCESS_ADDR,  &(pFrame->Addr)) == RVWAL_SUCCESS)
        if(ReadMib (MIBOID_MAC_REG_ACCESS_DWORD, &(pFrame->Data)) == RVWAL_SUCCESS)
            return RVWAL_SUCCESS;

    RvPrintf("Error in ReadRegister\n");
    return RVWAL_ERROR;
} 
// end of ReadRegister()

// ================================================================================================
// FUNCTION   : WriteRegister()
// ------------------------------------------------------------------------------------------------
// Purpose    : Write the register specified by pFrame->Address with the value pFrame->Data
// Parameters : pFrame -- Pointer to RegFrame
// ================================================================================================
RvStatus_t WriteRegister(RvRegFrame_t* pFrame)
{
    if(    WriteMib(MIBOID_MAC_REG_ACCESS_ADDR,  &(pFrame->Addr)) == WIFI_SUCCESS)
        if(WriteMib(MIBOID_MAC_REG_ACCESS_DWORD, &(pFrame->Data)) == WIFI_SUCCESS)
            return RVWAL_SUCCESS;

    RvPrintf(" > Error in WriteRegister (Addr = 0x%04X, Data = 0x%02X\n",pFrame->Addr,pFrame->Data);
    return RVWAL_ERROR;
}
// end of WriteRegister()

//-------------------------------------------------------------------------------------------------
//--- End of Source -------------------------------------------------------------------------------

// 2007-07-20 B.Brickner: Added ReadMib(MIBOID_RECEIVEDFRAGCOUNT,...) to GetPerCounters
// 2008-01-09 B.Brickner: Moved "get counters" after RSSI in GetPerCounters
// 2008-02-29 B.Brickner: Added write to MIBOID_CURRENTCHANNELFREQUENCY in RvServerPhyTestMode()
//                        to support drivers that require this to be set before starting
// 2008-03-04 B.Brickner: Added checking for connection state in RvServerPhyTestMode()
// 2008-08-08 B.Brickner: Added multiple individual packet transmit
// 2010-05-19 B.Brickner: Added support for TX MCS=0-7; added command for repeated register read
// 2010-05-24 B.Brickner: Added command to read 256 entries from the TestPort Capture RAM
// 2010-08-12 B.Brickner: Record packet length on RVWAL_PKT_TX_SINGLE to TxPacketBufferLength
// 2010-12-18 S.Mou     : Added RVWAL_PKT_TX_BATCH_STATUS command to retrieve status of Tx Batch 
