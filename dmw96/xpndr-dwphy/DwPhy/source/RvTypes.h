//
// RvTypes.h -- Common types and definitions for Remote VWAL client/server
// Copyright 2007-2008 DSP Group, Inc. All rights reserved.
//

#ifndef __RVTYPES_H
#define __RVTYPES_H

#define RVWAL_MAXBUF 4352 // maximum buffer length for RVWAL frames
#define RVWAL_MAXMIB  256 // maximum buffer for MibObject data
#define RVWAL_MAXPKT 4096 // maxumum buffer for packet data (PSDU length - 4 FCS bytes ?)
#define RVWAL_NUMPKT   64 // maximum number of packet in multiple packet frame

// ================================================================================================
// GENERIC INTEGER DATA TYPES
// ...these may be defined elsewhere in other projects, so beware
// ================================================================================================
#ifndef __int8_t_defined
typedef   signed char   int8_t;
typedef   signed short  int16_t;
typedef   signed int    int32_t;
#endif

#ifndef __uint32_t_defined
typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;
#endif

// ================================================================================================
// FUNCTION STATUS CODES
// ================================================================================================
typedef int RvStatus_t;

// ================================================================================================
// PRINTF FUNCTION TYPE
// Define a pointer to a function having the same prototype as "printf"...used in RvUtil
// ================================================================================================
typedef int (*RvPrintfFn_t)(const char *__format, ...);

// ================================================================================================
// SIMPLE COMMAND FRAME
// ================================================================================================
typedef struct RVWAL_CMD_FRAME
{ 
    uint32_t Command;    // common command code
    uint32_t Status;     // returned status code (RvStatus_t)
    uint32_t FrameSize;   // size of this frame

} RvCmdFrame_t;

// ================================================================================================
// REGISTER DATA READ/WRITE FRAME
// ================================================================================================
typedef struct RVWAL_REG_FRAME
{ 
    uint32_t Command;    // common command code
    uint32_t Status;     // returned status code (RvStatus_t)
    uint32_t FrameSize;   // size of this frame

    uint32_t Addr;       // register address
    uint32_t Data;       // register data

} RvRegFrame_t;

// ================================================================================================
// MIB READ/WRITE FRAME
// ================================================================================================
typedef struct RVWAL_MIB_FRAME
{ 
    uint32_t Command;     // common command code
    uint32_t Status;      // returned status code (RvStatus_t)
    uint32_t FrameSize;   // size of this frame

    uint32_t MibObjectID; // ID for the MIB object (only 16 bits valid)
    uint32_t MibStatus;   // Status for the MIB API operation (only 8 bits valid)
    uint8_t  MibObject[RVWAL_MAXMIB]; // MIB data (type/format determined by MIB element ID)

} RvMibFrame_t;

// ================================================================================================
// BUFFER READ/WRITE
// ================================================================================================
typedef struct RVWAL_BUFFER_FRAME
{ 
    uint32_t Command;     // common command code
    uint32_t Status;      // returned status code (RvStatus_t)
    uint32_t FrameSize;   // size of this frame

    uint32_t Length;      // number of valid bytes in Buffer
    uint8_t  Buffer[RVWAL_MAXPKT]; // data buffer

} RvBufferFrame_t;

// ================================================================================================
// PACKET TRANSMIT
// ================================================================================================
typedef struct RVWAL_TX_PACKET_FRAME
{ 
    uint32_t Command;     // common command code
    uint32_t Status;      // returned status code (RvStatus_t)
    uint32_t FrameSize;   // size of this frame

    uint32_t PacketType;  // Packet type (enumerated below)
    uint16_t Length;      // packet length (does not include FCS)
    uint8_t  PhyRate;     // PHY data rate (Mbps except 5.5 Mbps maps as 55)
    uint8_t  PowerLevel;  // transmit power level
    uint32_t NumTransmits;// [only used for burst transmission]
    uint32_t LongPreamble;// use Long preamble to relavent PhyRate = (2, 55, 11)

} RvTxPacketFrame_t;

// ================================================================================================
// MULTIPLE PACKET TRANSMIT
// ================================================================================================
typedef struct RVWAL_TX_MULTIPLE_PACKET_FRAME
{ 
    uint32_t Command;     // common command code
    uint32_t Status;      // returned status code (RvStatus_t)
    uint32_t FrameSize;   // size of this frame
    uint32_t PacketCount; // number of packets described below

    struct
    {
        uint32_t PacketType;  // Packet type (enumerated below)
        uint16_t Length;      // packet length (does not include FCS)
        uint8_t  PhyRate;     // PHY data rate (Mbps except 5.5 Mbps maps as 55)
        uint8_t  PowerLevel;  // transmit power level
        uint32_t LongPreamble;// use Long preamble to relavent PhyRate = (2, 55, 11)

    } Packet[RVWAL_NUMPKT];

} RvTxMultiplePacketFrame_t;

// ================================================================================================
// PACKET RECEIVE
// ================================================================================================
typedef struct RVWAL_RX_PACKET_FRAME
{ 
    uint32_t Command;     // common command code
    uint32_t Status;      // returned status code (RvStatus_t)
    uint32_t FrameSize;   // size of this frame

    uint16_t Length;      // packet length (does not include FCS)
    uint8_t  PhyRate;     // PHY data rate (Mbps except 5.5 Mbps maps as 55)
    uint8_t  RSSI0, RSSI1;// RSSI for antennas "A" and "B"
    uint32_t tsfTimestamp;// TSF timer (per wifiapi.h)
    uint32_t flags;       // (per wifiapi.h)
    uint8_t  Packet[RVWAL_MAXPKT]; // PSDU (without FCS)

} RvRxPacketFrame_t;

// ================================================================================================
// PER COUNTERS FRAME (for BERLAB/DwPhyMex)
// ================================================================================================
typedef struct RVWAL_PER_FRAME
{ 
    uint32_t Command;     // common command code
    uint32_t Status;      // returned status code (RvStatus_t)
    uint32_t FrameSize;   // size of this frame

    uint32_t ReceivedFragmentCount; // number of FCS clean packets
    uint32_t FCSErrorCount;         // number of FCS error packets
    uint32_t Length;                // last packet length
    uint32_t RxBitRate;             // current RX bit rate
    uint32_t BSSType;               // BSS type
    uint32_t ConnectionState;       // connextion state
    uint32_t DFS;                   // DFS radar detect (clear on read)
    uint32_t RSSI0, RSSI1;          // RSSI for antennas A and B

} RvPerFrame_t;

// ================================================================================================
// INDIRECT REGISTER ACCESS TO RAM (for DwPhyMex)
// ================================================================================================
typedef struct RVWAL_RAM_FRAME
{ 
    uint32_t Command;     // common command code
    uint32_t Status;      // returned status code (RvStatus_t)
    uint32_t FrameSize;   // size of this frame

    uint32_t IndexAddr;   // address for the data index
    uint32_t DataAddr[4]; // address for the data words
    uint32_t Data[256];   // contents for index 0-255

} RvRamFrame_t;

// ================================================================================================
// GENERIC RVWAL PACKET
//
// This type defines the set of all Remote VWAL frames. In all cases, the first two words are
// Command and Status. The actual frame format is inferred from the Command value.
// 
// ================================================================================================
typedef union RVWAL_FRAME
{
    uint32_t          Command;   // command code (always first word)
    RvCmdFrame_t      Cmd;       // overlay simple command frame
    RvRegFrame_t      Reg;       // overlay Register access frame
    RvMibFrame_t      Mib;       // overlay Mib generic access frame
    RvPerFrame_t      Per;       // overlay PER counters frame
    RvRamFrame_t      Ram;       // overlay RAM access frame
    RvTxPacketFrame_t TxPacket;  // overlay Packet send frame
    RvRxPacketFrame_t RxPacket;  // overlay Packet recive frame

    uint8_t Buf[RVWAL_MAXBUF];   // byte-oriented access to data

} RvFrame_t;
        
// ================================================================================================
// ENUMERATE: Command Codes for RvFrame_t
// ================================================================================================
enum 
{
    RVWAL_REG_WRITE       = 0x11, // (RvRegFrame) write register
    RVWAL_REG_READ        = 0x12, // (RvRegFrame) read register
    RVWAL_REG_WRITE_BB    = 0x13, // (RvRegFrame) write baseband register
    RVWAL_REG_READ_BB     = 0x14, // (RvRegFrame) read baseband register
    RVWAL_REG_READ_MULTIPLE=0x15, // (RvRxFrame) retrieve a series of register read values
    RVWAL_REG_READ_RAM256 =0x16,  // (RvRamFrame) read 256 entries of indirect memory

    RVWAL_MIB_WRITE       = 0x21, // (RvMibFrame) write MIB
    RVWAL_MIB_READ        = 0x22, // (RvMibFrame) read MIB
    
    RVWAL_PKT_TX_BUFFER   = 0x30, // (RvPktFrame) transmit the current transmit buffer contents
    RVWAL_PKT_TX_SINGLE   = 0x31, // (RvPktFrame) transmit simple broadcast packet, modulo-256 data
    RVWAL_PKT_TX_MULTIPLE = 0x32, // (RvPktFrame) transmit multiple consecutive packets

    RVWAL_PKT_TX_BATCH_STATUS=0x3C, // (RvPktFrame)     
    RVWAL_PKT_TX_BATCH_BUFFER=0x3D, // (RvPktFrame) start continuous transmit with buffer packet
    RVWAL_PKT_TX_BATCH     =0x3E, // (RvPktFrame) start continuous transmit
    RVWAL_PKT_TX_BATCH_STOP=0x3F, // (RvPktFrame) stop  continuous transmit

    RVWAL_CMD_PHYTESTMODE = 0x40, // (RvCmdFrame) set PHY test mode (for BERLab)
    RVWAL_CMD_PERCOUNTERS = 0x41, // (RvPerFrame) get PER counters  (for BERLab)
    RVWAL_CMD_SEEDRANDOM  = 0x42, // (RvRegFrame) set the random number generator seed for RvPacket
    RVWAL_CMD_SHUTDOWN    = 0x43, // (RvCmdFrame) shutdown remote VWAL server

    RVWAL_BUF_LOAD_TX     = 0x50, // (RvBufFrame) load packet transmit buffer on the RVWAL server

    RVWAL_PKT_RX_SINGLE   = 0x61, // (RvRxFrame) receive a packet
    RVWAL_PKT_RX_RSSI     = 0x62, // (RvRxFrame) receive RSSI values
    RVWAL_RX_RSSI_VALUES  = 0x63, // (RvBufFrame)retrieve a series of RSSI values

    RVWAL_MACADDRESS_WRITE= 0x71, // (RvMibFrame) write STA MAC Address
    RVWAL_MACADDRESS_READ = 0x72, // (RvMibFrame) read STA MAC Address

    RVWAL_SYSTEM_COMMAND  = 0x80, // (RvBufFrame) execute a command on the target system

    RVWAL_RESERVED1       = 0xFF,       // Reserve all combinations that would set the first, last,
    RVWAL_RESERVED4       = 0xFFFFFFFF, // or all bytes to all ones (not completely specified here)

} ERvCommand;
        
// ================================================================================================
// ENUMERATE: Packet Types for TX
// ================================================================================================
enum 
{
    RVWAL_PACKET_BROADCAST_MOD256 = 0,
    RVWAL_PACKET_BROADCAST_RANDOM = 1,
    RVWAL_PACKET_BROADCAST_ONES   = 2,

} ERvTxPacketTypes;

// ================================================================================================
// ENUMERATE: Status Codes for RvFrame_t and Function Return Status
// ================================================================================================
enum 
{
    RVWAL_SUCCESS                   =   0,
    RVWAL_ERROR                     =  -1,
    RVWAL_ERROR_UNDEFINED_COMMAND   =  -2,
    RVWAL_ERROR_NOT_CONNECTED       =  -3,
    RVWAL_ERROR_PARAMETER_RANGE     =  -4,
    RVWAL_ERROR_NULL_POINTER        =  -5,
    RVWAL_ERROR_MEMORY_ALLOCATION   =  -6,
    RVWAL_ERROR_CONNECTION_LOCKED   =  -7,
    RVWAL_ERROR_UNDEFINED_CASE      =  -8,
    RVWAL_ERROR_SOCKET_NO_RECV_DATA =  -9,

} ERvStatus;

// ================================================================================================
// DEFINE: Byte Order and Structure Packing Check Words
// ================================================================================================
#define RVWAL_CHKWORD1 0x01234567
#define RVWAL_CHKWORD2 0x89ABCDEF
#define RVWAL_CHKWORD3 0x0000AAAA
#define RVWAL_CHKWORD4 0x00550C0C

#endif // __REMOTEVWAL_H

//-------------------------------------------------------------------------------------------------
//--- End of File ---------------------------------------------------------------------------------
