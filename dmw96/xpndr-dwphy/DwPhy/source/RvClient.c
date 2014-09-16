//--< RvClient.c >---------------------------------------------------------------------------------
//=================================================================================================
//
//  Remote VWAL Client
//  Copyright 2007-2010 DSP Group, Inc. All rights reserved.
//
//  Written by Barrett Brickner
//
//=================================================================================================
//-------------------------------------------------------------------------------------------------

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "RvTypes.h"
#include "RvClient.h"
#include "RvSocket.h"
#include "RvUtil.h"
#include "WifiApi.h"

// ================================================================================================
// MODULE-SPECIFIC PARAMETERS
// ================================================================================================
static char     ServerAddr[32] = {""};     // Remote VWAL Server IP Address
static uint16_t ServerPort     = 30000;    // Sockets Port
static int      AutoConnect = 1;           // Automatically connect if not connected?

// ================================================================================================
// DIAGNOSTIC DEFINITIONS
// ================================================================================================
#define  STATUS(arg) RVCHECK(arg)
#define XSTATUS(arg) { if(STATUS(arg) < RVWAL_SUCCESS) return RVWAL_ERROR; }
#define VPRINTF if(RvUtil_Verbose()) RvPrintf

// ================================================================================================
// INTERNAL FUNCTION PROTOTYPES
// ================================================================================================
RvStatus_t RvClientCommand(RvFrame_t* pFrame, size_t FrameSize);
RvStatus_t RvClientRegCommand   (RvRegFrame_t*      pFrame);
RvStatus_t RvClientMibCommand   (RvMibFrame_t*      pFrame);
RvStatus_t RvClientPacketCommand(RvTxPacketFrame_t* pFrame);
RvStatus_t RvClientMultiplePacketCommand(RvTxMultiplePacketFrame_t* pFrame);
RvStatus_t RvClientBufferCommand(RvBufferFrame_t*   pFrame);
RvStatus_t RvClientCheckConnection(void);

#ifdef WIN32 // include DllMain only for Windows clients
#pragma warning( disable: 4100 ) // do not complain about unused parameter for DllMain

// ================================================================================================
// FUNCTION   : DllMain()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD Reason, LPVOID lpvReserved)
{
    switch (Reason)
    {
        case DLL_PROCESS_ATTACH:
            RvPrintf(" RvClient Build %s %s\n",__DATE__,__TIME__);
            break;

        case DLL_PROCESS_DETACH:
            RvSocketShutdown();
            break;

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            // do nothing
            break;
    }
    return TRUE;
}
// end of DllMain()

#pragma warning( default: 4100 )
#endif // WIN32

// ================================================================================================
// FUNCTION   : RvClientConnect()
// ------------------------------------------------------------------------------------------------
// Purpose    : Connect to the RVWAL Server
// Parameters : Addr -- String containing the server IP address, e.g., "10.82.0.2"
//              Port -- Port number used for the socket
// ================================================================================================
RVWAL_API RvStatus_t RvClientConnect(char *Addr, uint16_t Port)
{
    ServerPort = Port;
    sprintf(ServerAddr, Addr); // need to check length [TBD]

    // ------------------------------------------
    // Connect to the RVWAL Server if not already
    // ------------------------------------------
    if( !RvSocketIsConnected() )
    {
        XSTATUS( RvSocketCreateClient(ServerAddr, ServerPort) );
        XSTATUS( RvClientCheckConnection() );
    }
    return RVWAL_SUCCESS;
} 
// end of RvClientConnect()

// ================================================================================================
// FUNCTION   : RvClientDisconnect()
// ------------------------------------------------------------------------------------------------
// Purpose    : Disconnect from the RVWAL Server
// Parameters : none
// ================================================================================================
RVWAL_API RvStatus_t RvClientDisconnect(void)
{
    XSTATUS( RvSocketCloseConnection() );
    XSTATUS( RvSocketShutdown() );

    return RVWAL_SUCCESS;
} 
// end of RvClientDisconnect()

// ================================================================================================
// FUNCTION   : RvClientSeedRandomGenerator()
// ------------------------------------------------------------------------------------------------
// Purpose    : Enable PHY Test Mode (used by BERLab)
// Parameters : none
// ================================================================================================
RVWAL_API RvStatus_t RvClientSeedRandomGenerator(uint32_t Seed)
{
    RvRegFrame_t Frame;
    Frame.Command = RVWAL_CMD_SEEDRANDOM;
    Frame.Addr    = 0;
    Frame.Data    = Seed;

    STATUS( RvClientRegCommand( &Frame ) );
    STATUS( Frame.Status );

    return RVWAL_SUCCESS;
} 
// end of RvClientSeedRandomGenerator()

// ================================================================================================
// FUNCTION   : RvClientPhyTestMode()
// ------------------------------------------------------------------------------------------------
// Purpose    : Enable PHY Test Mode (used by BERLab)
// Parameters : none
// ================================================================================================
RVWAL_API RvStatus_t RvClientPhyTestMode(void)
{
    RvCmdFrame_t Frame;
    Frame.Command = RVWAL_CMD_PHYTESTMODE;

    XSTATUS( RvClientCommand( (RvFrame_t *)(&Frame), sizeof(RvCmdFrame_t) ) );
    XSTATUS( Frame.Status );

    return RVWAL_SUCCESS;
} 
// end of RvClientPhyTestMode()

// ================================================================================================
// FUNCTION   : RvClientShutdown()
// ------------------------------------------------------------------------------------------------
// Purpose    : Shutdown the R-VWAL server and disconnect
// Parameters : none
// ================================================================================================
RVWAL_API RvStatus_t RvClientShutdown(void)
{
    RvCmdFrame_t Frame;
    Frame.Command = RVWAL_CMD_SHUTDOWN;

    XSTATUS( RvClientCommand( (RvFrame_t *)(&Frame), sizeof(RvCmdFrame_t) ) );
    XSTATUS( Frame.Status );

    XSTATUS( RvClientDisconnect() );

    return RVWAL_SUCCESS;
} 
// end of RvClientShutdown()

// ================================================================================================
// FUNCTION   : RvClientDisplay()
// ------------------------------------------------------------------------------------------------
// Purpose    : Configure the display functions
// Parameters : pPrintfFn -- "printf" style function to use (can be simply printf)
//              Verbose   -- Verbose state (0=minimum, 1=normal, 2=max detail)
// ================================================================================================
RVWAL_API RvStatus_t RvClientDisplay(RvPrintfFn_t pPrintfFn, int Verbose)
{
    RvUtil_SetPrintf ( pPrintfFn);
    RvUtil_SetVerbose( Verbose  );

    return RVWAL_SUCCESS;
} 
// end of RvClientDisplay()

// ================================================================================================
// FUNCTION   : RvClientCheckConnection()
// ------------------------------------------------------------------------------------------------
// Purpose    : Check RVWAL connection looking for byte order or 32-bit word alignment problems
// Parameters : none
// ================================================================================================
RvStatus_t RvClientCheckConnection(void)
{
    RvFrame_t Frame;
    uint32_t i, Word1, Word2, Word3, Word4;

    // ----------------------------------------------------------------------------------
    // Server is looking for 0xFFFFFFFF in the Command field. Load 64 bytes with all ones
    // to insure it sees this regardless of byte order or structure element alignment
    // ----------------------------------------------------------------------------------
    for( i=0; i<64; i++ )
        Frame.Buf[i] = 0xFF;

    XSTATUS( RvClientCommand( &Frame, 64 ) ); // send/receive 64 bytes

    // ----------------------------------------------------------------
    // Pull out data by structure elements and check for expected value
    // ----------------------------------------------------------------
    Word1 = Frame.Reg.Command;
    Word2 = Frame.Reg.Status;
    Word3 = Frame.Reg.Addr;
    Word4 = Frame.Reg.Data;

    if((Word1 != RVWAL_CHKWORD1) || (Word2 != RVWAL_CHKWORD2) || (Word3 != RVWAL_CHKWORD3) || (Word4 != RVWAL_CHKWORD4))
    {
        RvPrintf("\n");
        RvPrintf(" Remote VWAL Communications Error: Structure Packing or Byte Order\n");
        RvPrintf("\n");
        RvPrintf(" Expected: 0x%08X,%08X,%08X,%08X\n",RVWAL_CHKWORD1,RVWAL_CHKWORD2,RVWAL_CHKWORD3,RVWAL_CHKWORD4);
        RvPrintf(" Received: 0x%08X,%08X,%08X,%08X\n",Word1,Word2,Word3,Word4);
        RvPrintf("\n");
        return RVWAL_ERROR;
    }
    return RVWAL_SUCCESS;
} 
// end of RvClientCheckConnection()

// ================================================================================================
// FUNCTION   : RvClientLoadTxBuffer()
// ------------------------------------------------------------------------------------------------
// Purpose    : Load the transmit packet buffer on the server
// Parameters : Length -- Number of bytes to load
//              pData  -- Data bytes (input)
// ================================================================================================
RVWAL_API RvStatus_t RvClientLoadTxBuffer(uint32_t Length, uint8_t* pData)
{
    RvBufferFrame_t Frame = {0};

    Frame.Command     = RVWAL_BUF_LOAD_TX;
    Frame.FrameSize   = sizeof(RvBufferFrame_t);
    Frame.Length      = Length;
    memcpy( Frame.Buffer, pData, Length );

    XSTATUS( RvClientBufferCommand(&Frame) );
    XSTATUS( Frame.Status );
    
    return RVWAL_SUCCESS;
} 
// end of RvClientLoadTxBuffer()

// ================================================================================================
// FUNCTION   : RvClientTxBufferPacket()
// ------------------------------------------------------------------------------------------------
// Purpose    : Transmit the contents of the packet buffer
// Parameters : PhyRate    -- PHY data rate (Mbps as truncated integer)
//              PowerLevel -- TX power level
// ================================================================================================
RVWAL_API RvStatus_t RvClientTxBufferPacket(uint8_t PhyRate, uint8_t PowerLevel)
{
    RvTxPacketFrame_t Frame;

    Frame.Command     = RVWAL_PKT_TX_BUFFER;
    Frame.PacketType  = 0;
    Frame.Length      = 0;
    Frame.PhyRate     = PhyRate;
    Frame.PowerLevel  = PowerLevel;

    XSTATUS( RvClientPacketCommand(&Frame) );
    XSTATUS( Frame.Status );
    
    return RVWAL_SUCCESS;
} 
// end of RvClientTxBufferPacket()

// ================================================================================================
// FUNCTION   : RvClientStartTxBatch()
// ------------------------------------------------------------------------------------------------
// Purpose    : Transmit a packet continuously
// Parameters : NumTransmits -- Number of packets to transmit
//              Length       -- PSDU length (not including 4 byte FCS)
//              PhyRate      -- PHY data rate (Mbps as truncated integer)
//              PowerLevel   -- TX power level
//              PacketType   -- Enumerated in header file
//              LongPreamble -- Use Long preamble for 2, 5.5, 11 Mbps
// ================================================================================================
RVWAL_API RvStatus_t RvClientStartTxBatch(uint32_t NumTransmits, uint16_t Length, uint8_t PhyRate, 
                                          uint8_t PowerLevel, uint32_t PacketType, uint32_t LongPreamble)
{
    RvTxPacketFrame_t Frame;

    Frame.Command      = RVWAL_PKT_TX_BATCH;
    Frame.PacketType   = PacketType;
    Frame.Length       = Length;
    Frame.PhyRate      = PhyRate;
    Frame.PowerLevel   = PowerLevel;
    Frame.NumTransmits = NumTransmits;
    Frame.LongPreamble = LongPreamble;

    XSTATUS( RvClientPacketCommand(&Frame) );
    XSTATUS( Frame.Status );
    
    return RVWAL_SUCCESS;
} 
// end of RvClientStartTxBatch()

// ================================================================================================
// FUNCTION   : RvClientStartTxBatchBuffer()
// ------------------------------------------------------------------------------------------------
// Purpose    : Transmit a packet continuously from the packet buffer
// Parameters : NumTransmits -- Number of packets to transmit
//              PhyRate      -- PHY data rate (Mbps as truncated integer)
//              PowerLevel   -- TX power level
//              LongPreamble -- Use Long preamble for 2, 5.5, 11 Mbps
// ================================================================================================
RVWAL_API RvStatus_t RvClientStartTxBatchBuffer(uint32_t NumTransmits, uint8_t PhyRate, 
                                                uint8_t PowerLevel, uint32_t LongPreamble)
{
    RvTxPacketFrame_t Frame;

    Frame.Command      = RVWAL_PKT_TX_BATCH_BUFFER;
    Frame.PacketType   = 0;
    Frame.Length       = 0;
    Frame.PhyRate      = PhyRate;
    Frame.PowerLevel   = PowerLevel;
    Frame.NumTransmits = NumTransmits;
    Frame.LongPreamble = LongPreamble;

    XSTATUS( RvClientPacketCommand(&Frame) );
    XSTATUS( Frame.Status );
    
    return RVWAL_SUCCESS;
} 
// end of RvClientStartTxBatchBufferBuffer()

// ================================================================================================
// FUNCTION   : RvClientStopTxBatch()
// ------------------------------------------------------------------------------------------------
// Purpose    : Stop continuous transmission
// Parameters : none
// ================================================================================================
RVWAL_API RvStatus_t RvClientStopTxBatch(void)
{
    RvTxPacketFrame_t Frame = {0};

    Frame.Command     = RVWAL_PKT_TX_BATCH_STOP;

    XSTATUS( RvClientPacketCommand(&Frame) );
    XSTATUS( Frame.Status );
    
    return RVWAL_SUCCESS;
} 
// end of RvClientStopTxBatch

// ================================================================================================
// FUNCTION   : RvClientQueryTxBatch()
// ------------------------------------------------------------------------------------------------
// Purpose    : 
// Parameters : none
// ================================================================================================
RVWAL_API RvStatus_t RvClientQueryTxBatch(uint8_t *pData)
{
    RvTxPacketFrame_t Frame = {0};

    Frame.Command     = RVWAL_PKT_TX_BATCH_STATUS;

    XSTATUS( RvClientPacketCommand(&Frame) );
    XSTATUS( Frame.Status );

    *pData = (uint8_t)Frame.NumTransmits;
    
    return RVWAL_SUCCESS;
} 
// end of RvClientQueryTxBatch

// ================================================================================================
// FUNCTION   : RvClientTxSinglePacket()
// ------------------------------------------------------------------------------------------------
// Purpose    : Transmit a single packet
// Parameters : Length       -- PSDU length (not including 4 byte FCS)
//              PhyRate      -- PHY data rate (Mbps as truncated integer)
//              PowerLevel   -- TX power level
//              PacketType   -- Enumerated in header file
//              LongPreamble -- Use Long preamble for 2, 5.5, 11 Mbps
// ================================================================================================
RVWAL_API RvStatus_t RvClientTxSinglePacket(uint16_t Length, uint8_t PhyRate, uint8_t PowerLevel, 
                                            uint32_t PacketType, uint32_t LongPreamble)
{
    RvTxPacketFrame_t Frame;

    Frame.Command      = RVWAL_PKT_TX_SINGLE;
    Frame.PacketType   = PacketType;
    Frame.Length       = Length;
    Frame.PhyRate      = PhyRate;
    Frame.PowerLevel   = PowerLevel;
    Frame.LongPreamble = LongPreamble;

    XSTATUS( RvClientPacketCommand(&Frame) );
    XSTATUS( Frame.Status );
    
    return RVWAL_SUCCESS;
} 
// end of RvClientTxSinglePacket()

// ================================================================================================
// FUNCTION   : RvClientTxMultiplePacket()
// ------------------------------------------------------------------------------------------------
// Purpose    : Transmit a multiple packets
// Parameters : NumPkts      -- Number of packets
//              Length       -- PSDU length (not including 4 byte FCS)
//              PhyRate      -- PHY data rate (Mbps as truncated integer)
//              PowerLevel   -- TX power level
//              PacketType   -- Enumerated in header file
//              LongPreamble -- Use Long preamble for 2, 5.5, 11 Mbps
// ================================================================================================
RVWAL_API RvStatus_t RvClientTxMultiplePackets(uint8_t NumPkts, uint16_t Length[], uint8_t PhyRate[], 
                                               uint8_t PowerLevel[], uint32_t PacketType[], uint32_t LongPreamble[])
{
    RvTxMultiplePacketFrame_t Frame;
    int i;

    if (NumPkts > RVWAL_NUMPKT) return RVWAL_ERROR_PARAMETER_RANGE;

    Frame.Command = RVWAL_PKT_TX_MULTIPLE;
    Frame.PacketCount = NumPkts;

    for (i=0; i<NumPkts; i++)
    {    
        Frame.Packet[i].PacketType   = PacketType[i];
        Frame.Packet[i].Length       = Length[i];
        Frame.Packet[i].PhyRate      = PhyRate[i];
        Frame.Packet[i].PowerLevel   = PowerLevel[i];
        Frame.Packet[i].LongPreamble = LongPreamble[i];
    }
    XSTATUS( RvClientMultiplePacketCommand(&Frame) );
    XSTATUS( Frame.Status );
    
    return RVWAL_SUCCESS;
} 
// end of RvClientTxMultiplePackets()

// ================================================================================================
// FUNCTION   : RvClientRxSinglePacket()
// ------------------------------------------------------------------------------------------------
// Purpose    : Receive a single RX packet (if available)
// Parameters : pFrame -- Pointer to the data structure containing the relevant information
// ================================================================================================
RVWAL_API RvStatus_t RvClientRxSinglePacket( RvRxPacketFrame_t* pFrame )
{
    memset( pFrame, 0,  sizeof(RvRxPacketFrame_t) );
    pFrame->FrameSize = sizeof(RvRxPacketFrame_t);
    pFrame->Command = RVWAL_PKT_RX_SINGLE;

    XSTATUS( RvClientCommand((RvFrame_t *)pFrame, sizeof(RvRxPacketFrame_t) ) );
    XSTATUS( pFrame->Status );
    
    return RVWAL_SUCCESS;
} 
// end of RvClientRxSinglePacket()

// ================================================================================================
// FUNCTION   : RvClientSystem()
// ------------------------------------------------------------------------------------------------
// Purpose    : Execute a command via the 'C' function system() on the target
// Parameters : CommandString -- Argument to the system function on the target
// ================================================================================================
RVWAL_API RvStatus_t RvClientSystem( char *CommandString )
{
    RvBufferFrame_t Frame = {0};

    if (strlen(CommandString) >= RVWAL_MAXPKT) 
        return RVWAL_ERROR_PARAMETER_RANGE;

    Frame.Command = RVWAL_SYSTEM_COMMAND;
    Frame.Length = strlen(CommandString);

    strcpy(Frame.Buffer, CommandString);

    XSTATUS( RvClientBufferCommand(&Frame) );
    XSTATUS( Frame.Status );
    
    return RVWAL_SUCCESS;
} 
// end of RvClientRxSinglePacket()

// ================================================================================================
// FUNCTION   : RvClientRxRSSI()
// ------------------------------------------------------------------------------------------------
// Purpose    : Receive packets and retrieve the RSSI values
// Parameters : pFrame -- Pointer to the data structure containing the relevant information
// ================================================================================================
RVWAL_API RvStatus_t RvClientRxRSSI( RvRxPacketFrame_t* pFrame )
{
    memset( pFrame, 0,  sizeof(RvRxPacketFrame_t) );
    pFrame->FrameSize = sizeof(RvRxPacketFrame_t);
    pFrame->Command = RVWAL_PKT_RX_RSSI;

    XSTATUS( RvClientCommand((RvFrame_t *)pFrame, sizeof(RvRxPacketFrame_t) ) );
    XSTATUS( pFrame->Status );
    
    return RVWAL_SUCCESS;
} 
// end of RvClientRxRSSI()

// ================================================================================================
// FUNCTION   : RvClientRxRSSIValues()
// ------------------------------------------------------------------------------------------------
// Purpose    : Receive packets and retrieve the RSSI values
// Parameters : pFrame -- Pointer to the data structure containing the relevant information
// ================================================================================================
RVWAL_API RvStatus_t RvClientRxRSSIValues( RvBufferFrame_t* pFrame )
{
    memset( pFrame, 0,  sizeof(RvBufferFrame_t) );
    pFrame->FrameSize = sizeof(RvBufferFrame_t);
    pFrame->Command = RVWAL_RX_RSSI_VALUES;

    XSTATUS( RvClientCommand((RvFrame_t *)pFrame, sizeof(RvBufferFrame_t) ) );
    XSTATUS( pFrame->Status );
    
    return RVWAL_SUCCESS;
} 
// end of RvClientRxRSSIValues()

// ================================================================================================
// FUNCTION   : RvClientRegReadMultiple()
// ------------------------------------------------------------------------------------------------
// Purpose    : Retrieve multiple consecutive reads of a baseband register address
// Parameters : pFrame -- Pointer to the data structure containing the relevant information
//              Addr   -- PHY address
// ================================================================================================
RVWAL_API RvStatus_t RvClientRegReadMultiple( RvBufferFrame_t* pFrame, uint32_t Addr )
{
    memset( pFrame, 0,  sizeof(RvBufferFrame_t) );
    pFrame->FrameSize = sizeof(RvBufferFrame_t);
    pFrame->Command = RVWAL_REG_READ_MULTIPLE;

    *((uint32_t *)pFrame->Buffer) = Addr;

    XSTATUS( RvClientCommand((RvFrame_t *)pFrame, sizeof(RvBufferFrame_t) ) );
    XSTATUS( pFrame->Status );
    
    return RVWAL_SUCCESS;
} 
// end of RvClientRegReadMultiple()

// ================================================================================================
// FUNCTION   : RvClientReadRAM256()
// ------------------------------------------------------------------------------------------------
// Purpose    : Read 256 words through address indirection (usually from a RAM)
// Parameters : pFrame -- Pointer to the data structure containing the relevant information
// ================================================================================================
RVWAL_API RvStatus_t RvClientReadRAM256( RvRamFrame_t* pFrame, uint32_t IndexAddr, uint32_t DataAddr[4] )
{
    pFrame->FrameSize = sizeof(RvRamFrame_t);
    pFrame->Command = RVWAL_REG_READ_RAM256;

    pFrame->IndexAddr = IndexAddr;
    pFrame->DataAddr[0] = DataAddr[0];
    pFrame->DataAddr[1] = DataAddr[1];
    pFrame->DataAddr[2] = DataAddr[2];
    pFrame->DataAddr[3] = DataAddr[3];
    memset( pFrame->Data, 0,  256*sizeof(uint32_t) );

    XSTATUS( RvClientCommand((RvFrame_t *)pFrame, sizeof(RvRamFrame_t)) );
    XSTATUS( pFrame->Status );
    
    return RVWAL_SUCCESS;
} 
// end of RvClientReadRAM256()

// ================================================================================================
// FUNCTION   : RvClientGetPerCounters()
// ------------------------------------------------------------------------------------------------
// Purpose    : Retrieve data used by BERLab PER measurements via MIB/registers
// Parameters : pFrame -- Pointer to the data structure containing the relevant information
// ================================================================================================
RVWAL_API RvStatus_t RvClientGetPerCounters( RvPerFrame_t* pFrame )
{
    memset( pFrame, 0, sizeof(RvPerFrame_t) );
    pFrame->Command = RVWAL_CMD_PERCOUNTERS;

    XSTATUS( RvClientCommand((RvFrame_t *)pFrame, sizeof(RvPerFrame_t) ) );
    XSTATUS( pFrame->Status );
    
    return RVWAL_SUCCESS;
} 
// end of RvClientGetPerCounters()

// ================================================================================================
// FUNCTION   : RvClientReadMib()
// ------------------------------------------------------------------------------------------------
// Purpose    : Read the specified MIB Object
// Parameters : MibObjectID   -- MIB object identifier
//              pMibObject    -- MIB object data (size and type depend on identifier) (output)
//              MibObjectSize -- Number of bytes in the supplied pMibObject buffer
// ================================================================================================
RVWAL_API RvStatus_t RvClientReadMib(uint16_t MibObjectID, void *pMibObject, size_t MibObjectSize)
{
    RvMibFrame_t Frame;

    if(!pMibObject) return STATUS(RVWAL_ERROR_NULL_POINTER);
    if(MibObjectSize > RVWAL_MAXMIB) return STATUS(RVWAL_ERROR_PARAMETER_RANGE);

    Frame.Command     = RVWAL_MIB_READ;
    Frame.MibObjectID = MibObjectID;
    Frame.MibStatus   = 0;

    XSTATUS( RvClientMibCommand( &Frame ) );
    XSTATUS( Frame.Status );
    
    memcpy( pMibObject, Frame.MibObject, MibObjectSize );
    return RVWAL_SUCCESS;
} 
// end of RvClientReadMib()

// ================================================================================================
// FUNCTION   : RvClientWriteMib()
// ------------------------------------------------------------------------------------------------
// Purpose    : Write the specified MIB Object
// Parameters : MibObjectID   -- MIB object identifier
//              pMibObject    -- MIB object data (size and type depend on identifier) (input)
//              MibObjectSize -- Number of bytes in the pMibObject buffer
// ================================================================================================
RVWAL_API RvStatus_t RvClientWriteMib(uint16_t MibObjectID, void *pMibObject, size_t MibObjectSize)
{
    RvMibFrame_t Frame;

    if( !pMibObject                 ) return STATUS( RVWAL_ERROR_NULL_POINTER    );
    if( MibObjectSize > RVWAL_MAXMIB) return STATUS( RVWAL_ERROR_PARAMETER_RANGE );

    Frame.Command     = RVWAL_MIB_WRITE;
    Frame.MibObjectID = MibObjectID;
    Frame.MibStatus   = 0;
    memcpy( Frame.MibObject, pMibObject, MibObjectSize );

    XSTATUS( RvClientMibCommand( &Frame ) );
    XSTATUS( Frame.Status );
    
    return RVWAL_SUCCESS;
} 
// end of RvClientWriteMib()

// ================================================================================================
// FUNCTION   : RvClientWriteMacAddress()
// ------------------------------------------------------------------------------------------------
// Purpose    : Write the station MAC address
// Parameters : pAddr -- pointer to 6-byte address array
// ================================================================================================
RVWAL_API RvStatus_t RvClientWriteMacAddress(uint8_t* pAddr)
{
    RvMibFrame_t Frame;

    if(!pAddr) return STATUS(RVWAL_ERROR_NULL_POINTER);

    Frame.Command     = RVWAL_MACADDRESS_WRITE;
    memcpy(Frame.MibObject, pAddr, 6);
    Frame.MibStatus   = 0;

    XSTATUS( RvClientMibCommand( &Frame ) );
    XSTATUS( Frame.Status );
    
    return RVWAL_SUCCESS;
} 
// end of RvClientWriteMacAddress()

// ================================================================================================
// FUNCTION   : RvClientReadMacAddress()
// ------------------------------------------------------------------------------------------------
// Purpose    : Read the station MAC address
// Parameters : pAddr -- pointer to 6-byte address array
// ================================================================================================
RVWAL_API RvStatus_t RvClientReadMacAddress(uint8_t* pAddr)
{
    RvMibFrame_t Frame;

    if(!pAddr) return STATUS(RVWAL_ERROR_NULL_POINTER);

    Frame.Command     = RVWAL_MACADDRESS_READ;
    Frame.MibObjectID = 0;
    Frame.MibStatus   = 0;

    XSTATUS( RvClientMibCommand( &Frame ) );
    XSTATUS( Frame.Status );
    
    memcpy( pAddr, Frame.MibObject, 6 );
    return RVWAL_SUCCESS;
} 
// end of RvClientReadMacAddress()

// ================================================================================================
// FUNCTION   : OS_RegRead()
// ------------------------------------------------------------------------------------------------
// Purpose    : Read the specified MAC register
// Parameters : Addr -- Address (only 16 bits used)
// ================================================================================================
RVWAL_API uint32_t OS_RegRead(uint32_t Addr)
{
    RvRegFrame_t Frame;

    Frame.Command = RVWAL_REG_READ;
    Frame.Addr    = Addr;

    STATUS( RvClientRegCommand( &Frame ) );
    STATUS( Frame.Status );

    return Frame.Data;
} 
// end of OS_RegRead()

// ================================================================================================
// FUNCTION   : OS_RegWrite()
// ------------------------------------------------------------------------------------------------
// Purpose    : Write the specified MAC register
// Parameters : Addr  -- Address (only 16 bits used)
//              Value -- Value to be written (32 bits)
// ================================================================================================
RVWAL_API void OS_RegWrite(uint32_t Addr, uint32_t Value)
{
    RvRegFrame_t Frame;

    Frame.Command = RVWAL_REG_WRITE;
    Frame.Addr    = Addr;
    Frame.Data    = Value;

    STATUS( RvClientRegCommand( &Frame ) );
    STATUS( Frame.Status );
}
// ed of OS_RegWrite()

// ================================================================================================
// FUNCTION   : OS_RegReadBB()
// ------------------------------------------------------------------------------------------------
// Purpose    : Read the specified baseband register
// Parameters : Addr  -- Address (only 10 bits valid)
// ================================================================================================
RVWAL_API uint8_t OS_RegReadBB(uint32_t Addr)
{
    RvRegFrame_t Frame;

    if(Addr > 1023) return 0; // nothing to do...address out of range

    Frame.Command = RVWAL_REG_READ;
    Frame.Addr    = Addr;

    STATUS( RvClientRegCommand( &Frame ) );
    STATUS( Frame.Status );

    return (uint8_t)Frame.Data;
} 
// end of OS_RegReadBB()

// ================================================================================================
// FUNCTION   : OS_RegWriteBB()
// ------------------------------------------------------------------------------------------------
// Purpose    : Write the specified baseband register
// Parameters : Addr  -- Address (only 10 bits valid)
//              Value -- Value to be written (8 bits)
// ================================================================================================
RVWAL_API void OS_RegWriteBB(uint32_t Addr, uint8_t Value)
{
    RvRegFrame_t Frame;

    if(Addr > 1023) return ; // nothing to do...address out of range

    Frame.Command = RVWAL_REG_WRITE;
    Frame.Addr    = Addr;
    Frame.Data    = Value;

    STATUS( RvClientRegCommand(&Frame) );
    STATUS( Frame.Status );
}
// end of OS_RegWriteBB()

// ================================================================================================
// FUNCTION   : OS_Delay()
// ------------------------------------------------------------------------------------------------
// Purpose    : Implement a delay
// Parameters : microsecond -- Request delay period (microseconds)
// ================================================================================================
RVWAL_API void OS_Delay(uint32_t microsecond)
{
    double tEnd;

#ifdef WIN32

    LARGE_INTEGER Count0, Count, Frequency;

    // -----------------------------------------------------
    // Windows High Resolution Timer. 
    // Not avaialable if QueryPerformanceCounter() returns 0
    // -----------------------------------------------------
    if( QueryPerformanceCounter( &Count0 ) )
    {
        double t;
        double x32 = (double)(0xFFFFFFFFU) + 1; // 2^32

        QueryPerformanceFrequency( &Frequency );

        // ------------------------------------------------------------------------
        // Compute end time: note that we only use the LowPart from Frequency which
        // implies the counter clock rate is < 4 GHz (pretty safe assumption)
        // ------------------------------------------------------------------------
        tEnd = (Count0.HighPart * x32) + Count0.LowPart
           + (double)Frequency.LowPart * microsecond / 1.0E+6;

        do // Wait until the specified time expires
        {
            QueryPerformanceCounter( &Count );
            t = ( Count.HighPart * x32 ) + Count.LowPart;
        } while(t < tEnd);
    }
    else
#endif
    // --------------------------------------------------
    // Contingency: use 'C' process time (low resolution)
    // --------------------------------------------------
    {
        tEnd = (double)clock() + (double)microsecond/1e6 * CLOCKS_PER_SEC;
        while( (double)clock() < tEnd )
            ;
    }
}
// end of OS_Delay()

// ================================================================================================
// FUNCTION   : RvClientCommand()
// ------------------------------------------------------------------------------------------------
// Purpose    : Send a command frame and receive a response from the RVWAL Server
// Parameters : pFrame    -- Pointer to the frame (used for sending, also buffers response)
//              FrameSize -- Size of the frame (bytes)
// ================================================================================================
RvStatus_t RvClientCommand(RvFrame_t* pFrame, size_t FrameSize)
{
    size_t SendLen, RecvLen;

    // ------------------------------------------
    // Connect to the RVWAL Server if not already
    // ------------------------------------------
    if ( !RvSocketIsConnected() )
    {
        if (AutoConnect)
        {
            // Generate default server address if not present
            //
            if ( strlen(ServerAddr) < 8 )
            {
                char *ComputerName = getenv("COMPUTERNAME");

                if (ComputerName != NULL)
                {
                    #if defined(BUILD_DWPHYMEX)

                        if      (!strcmp(ComputerName,"MN-PCXP-07083")) sprintf(ServerAddr,"10.82.0.11"); // MNTestStation1
                        else if (!strcmp(ComputerName,"MN-PCXP-07048")) sprintf(ServerAddr,"10.82.0.21"); // MNTestStation2
                        else if (!strcmp(ComputerName,"MN-PCXP-07076")) sprintf(ServerAddr,"10.82.0.31"); // MNTestStation3

                    #elif defined(BUILD_DWPHYMEX2)

                        if      (!strcmp(ComputerName,"MN-PCXP-07083")) sprintf(ServerAddr,"10.82.0.12"); // MNTestStation1
                        else if (!strcmp(ComputerName,"MN-PCXP-07048")) sprintf(ServerAddr,"10.82.0.22"); // MNTestStation2
                        else if (!strcmp(ComputerName,"MN-PCXP-07076")) sprintf(ServerAddr,"10.82.0.32"); // MNTestStation3

                    #endif
                }

                if (strlen(ServerAddr) < 8)
                    sprintf(ServerAddr, "10.82.0.99"); // default...not intended to work
            }

            RvPrintf("RvClient Autoconnect to %s\n",ServerAddr);

            XSTATUS( RvSocketCreateClient( ServerAddr, ServerPort ) );
            XSTATUS( RvClientCheckConnection() );
        }
        else return RVWAL_ERROR_NOT_CONNECTED;
    }
    // ------------------------------------
    // Frame Exchange with the RVWAL Server
    // ------------------------------------
    XSTATUS( RvSocketSend   ( (char *)pFrame, FrameSize, &SendLen ) );
    XSTATUS( RvSocketReceive( (char *)pFrame, FrameSize, &RecvLen ) );

    if ( (RecvLen > 64) && (RecvLen < FrameSize) )
    {
        XSTATUS( RvSocketReceiveRemainder( (char *)pFrame, FrameSize, &RecvLen ) );
    }

    return RVWAL_SUCCESS;
} 
// end of RvClientCommand()

// ================================================================================================
// FUNCTION   : RvClientRegCommand()
// ------------------------------------------------------------------------------------------------
// Purpose    : Implement a register read/write command
// Parameters : pFrame -- Pointer to the frame (used for sending, also buffers response)
// ================================================================================================
RvStatus_t RvClientRegCommand(RvRegFrame_t* pFrame)
{
    pFrame->FrameSize = sizeof(RvRegFrame_t);

    XSTATUS( RvClientCommand( (RvFrame_t *)pFrame, sizeof(RvRegFrame_t) ) );
    XSTATUS( pFrame->Status );

    return RVWAL_SUCCESS;
} 
// end of RvClientRegCommand()

// ================================================================================================
// FUNCTION   : RvClientMibCommand()
// ------------------------------------------------------------------------------------------------
// Purpose    : Implement a MIB read/write command
// Parameters : pFrame -- Pointer to the frame (used for sending, also buffers response)
// ================================================================================================
RvStatus_t RvClientMibCommand(RvMibFrame_t* pFrame)
{
    pFrame->FrameSize = sizeof(RvMibFrame_t);

    XSTATUS( RvClientCommand( (RvFrame_t *)pFrame, sizeof(RvMibFrame_t) ) );
    XSTATUS( pFrame->Status );

    return RVWAL_SUCCESS;
} 
// end of RvClientMibCommand()

// ================================================================================================
// FUNCTION   : RvClientPacketCommand()
// ------------------------------------------------------------------------------------------------
// Purpose    : Implement a WiFi packet operation
// Parameters : pFrame -- Pointer to the frame (used for sending, also buffers response)
// ================================================================================================
RvStatus_t RvClientPacketCommand(RvTxPacketFrame_t* pFrame)
{
    pFrame->FrameSize = sizeof(RvTxPacketFrame_t);

    XSTATUS( RvClientCommand( (RvFrame_t *)pFrame, sizeof(RvTxPacketFrame_t) ) );
    XSTATUS( pFrame->Status );

    return RVWAL_SUCCESS;
} 
// end of RvClientPacketCommand()

// ================================================================================================
// FUNCTION   : RvClientMultiplePacketCommand()
// ------------------------------------------------------------------------------------------------
// Purpose    : Implement a WiFi (multiple) packet operation
// Parameters : pFrame -- Pointer to the frame (used for sending, also buffers response)
// ================================================================================================
RvStatus_t RvClientMultiplePacketCommand(RvTxMultiplePacketFrame_t* pFrame)
{
    pFrame->FrameSize = sizeof(RvTxMultiplePacketFrame_t);

    XSTATUS( RvClientCommand( (RvFrame_t *)pFrame, sizeof(RvTxMultiplePacketFrame_t) ) );
    XSTATUS( pFrame->Status );

    return RVWAL_SUCCESS;
} 
// end of RvClientPacketCommand()

// ================================================================================================
// FUNCTION   : RvClientBufferCommand()
// ------------------------------------------------------------------------------------------------
// Purpose    : Implement a buffer transfer operation
// Parameters : pFrame -- Pointer to the frame (used for sending, also buffers response)
// ================================================================================================
RvStatus_t RvClientBufferCommand(RvBufferFrame_t* pFrame)
{
    pFrame->FrameSize = sizeof(RvBufferFrame_t);

    XSTATUS( RvClientCommand( (RvFrame_t *)pFrame, sizeof(RvBufferFrame_t) ) );
    XSTATUS( pFrame->Status );

    return RVWAL_SUCCESS;
} 
// end of RvClientBufferCommand()

//-------------------------------------------------------------------------------------------------
//--- End of File ---------------------------------------------------------------------------------

// Revised 2008-11-03
// - Changed names for default supported hosts to match IT renaming of PCs
//
// Revised 2010-12-18 [SM]
// - Added RvClientQueryTxBatch() to retrieve status of Tx Batch.

