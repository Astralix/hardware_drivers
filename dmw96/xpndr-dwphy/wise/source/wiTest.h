//
//  wiTest.h -- WISE Test Routines
//  Copyright 2001 Bermai, 2005-2011 DSP Group, Inc. All rights reserved.
//

#ifndef __WITEST_H
#define __WITEST_H

#include <stdio.h>    // needed for definition of FILE
#include <time.h>     // needed for clock_t definition
#include "wiChanl.h"  // needed for WICHANL_MAXPACKETS definition
#include "wiUtil.h"   // needed for WISE_MAX_THREADS
#include "wiType.h"

#if defined(__cplusplus) || defined(__cplusplus__)
    extern "C" {
#endif

// Define the maximum number of background packets to be sent prior to the PER test packet
// 
#define WITEST_MAX_BACKGROUNDPACKETS  WICHANL_MAXPACKETS
#define WITEST_MAX_DATATYPE            16
#define WITEST_MAX_POINTS             128

// Define length of the TxData array. For 802.11n this should be 65535 bytes.
// Define length for the Comment field (actual array has one extra byte for termination)
//
#define WITEST_MAX_LENGTH             65535
#define WITEST_MAX_COMMENT              127
#define WITEST_MAX_FILENAME_EXTENSION    63


enum wiTestDataTypeE
{
    WITEST_DATATYPE_RANDOM                        =  0,
    WITEST_DATATYPE_ANNEX_G                       =  1,
    WITEST_DATATYPE_FIXED_BYTE                    =  2,
    WITEST_DATATYPE_BUFFER                        =  3,
    
    WITEST_DATATYPE_802_11_DATA_MOD256            =  4,
    WITEST_DATATYPE_802_11_DATA_RANDOM            =  5,
    WITEST_DATATYPE_802_11_DATA_MOD256_BROADCAST  =  6,
    WITEST_DATATYPE_802_11_DATA_PRBS              =  7,
    WITEST_DATATYPE_802_11_DATA_BUFFER            =  8,

    WITEST_DATATYPE_802_11_AMPDU_MOD256           = 14,
    WITEST_DATATYPE_802_11_AMPDU_RANDOM           = 15,
    WITEST_DATATYPE_802_11_AMPDU_MOD256_BROADCAST = 16,

};

enum wiTestRndSeedTypeE
{
    WITEST_RNDSEED_RANDOM       = -1, // seed is generated based on system clock
    WITEST_RNDSEED_RATELENGTH   = -2, // seed is a function of packet rate and length
    WITEST_RNDSEED_PACKETNUMBER = -3, // seed is a function of the packet sequence number
    WITEST_RNDSEED_ZERO         = -4, // seed is 0 for all threads/packets
};


typedef struct
{
    wiInt  DataType;// type of packet
    wiReal DataRate; // data rate (bps)
    wiUInt Length;   // packet length (bytes)
    wiReal PrdBm;    // receive power level (dBm)
    wiReal FcMHz;    // baseband center frequency (MHz)
    wiReal Delay;    // packet starting time in composite waveform (seconds)

} wiTestBackgroundPacket_t;

// Callback function type
typedef wiStatus (* wiTestCallbackFn_t)(void);

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Test Parameter/State Structure
//
typedef struct
{
    wiInt      DataType;

    wiInt      RndSeed;        // Seed for random number generator (-1, -2, -3 are special)
    wiUInt     RndStep[2];     // Step into random seed
    wiUInt     RndInitState[3];// State of the wiRnd at the start of the last test
    wiUInt     UpdateRate;     // rate (number of packets) to write the wiseStatus.txt file

    wiReal     DataRate;      // Data rate in bps
    wiUInt     Length;        // Message length (bytes)
    wiUInt     FixedByte;     // pattern for DataType = 2

    wiReal     MinSNR;
    wiReal     MaxSNR;
    wiReal     StepSNR;
    wiReal     MinPrdBm;
    wiReal     MaxPrdBm;
    wiReal     StepPrdBm;
    wiReal     PrdBmForSNR;   // wiChanl PrdBm value when testing using SNR instead of PrdBm
	wiReal     LastPrdBm;     // Last PrdBm value from wiTest_TxPacket()
    wiReal     MinPER;        // Minimum packet error rate
    wiUInt     MinFail;       // Minimum number of packet failures
    wiUInt     MaxPackets;    // Maximum number of test packets
    wiUInt     MinPacketCount;// Minimum number of packets to test
    wiBoolean  UpdateMinPacketCount;

    wiUWORD   *TxData[WISE_MAX_THREADS]; // array allocated in wiTest
    wiUWORD   *RxData[WISE_MAX_THREADS]; // pointer to array from PHY 
    char       Comment[WITEST_MAX_COMMENT+1];
    char       FilenameExtension[WITEST_MAX_FILENAME_EXTENSION+1];

    wiUInt     BitCount;              // number of bits in the packet
    wiUInt     BitErrorCount;         // number of bit errors in the packet
    wiUInt     PacketCount;           // number of packets at the test point
    wiUInt     PacketErrorCount;      // number of packet errors at the test point
    wiUInt     TotalPacketCount;      // number of packets for the entire test
    wiUInt     TotalPacketErrorCount; // number of packet errors for the entire test
    wiUInt     NumberOfByteErrors;    // number of byte errors for current packet
    wiReal     BER;                   // computed bit error rate
    wiReal     PER;                   // computed packet error rate
    wiBoolean  DisableOutputFile;     // disable generation of test output file
    wiBoolean  ClearTotalsOnStartTest;// clear Total*Count values on wiTest_StartTest()
    FILE      *OutputFile;            // handle for test output
    wiReal     ProcessTime0;          // process time recorded in wiTest_StartTest()
    wiBoolean  PauseAfterTest;        // pause script parsing at the end of the current test
    wiBoolean  TestIsDone;            // flag to indicate a call to wiTest_EndTest()
    wiBoolean  TraceKeyOp;
    wiUInt     TestCount;             // counter for the number of calls to wiTest_EndTest()
    clock_t    TestStartClock;        // process time at start of a test
    time_t     TestStartTime;         // time at the start of a test
    wiBoolean  PrintProcessTime;      // display the process time

    wiInt      NumberOfThreads;       // number of parallel TxRxPacket threads
    wiBoolean  UseSingleTxRxThread;   // call TxRxPacket threads serially with a single thread (for debug)
    wiBoolean  OnePacketUsesSNR;      // wiTest_OnePacket() uses SNR[dB] instead of Pr[dBm]

    wiTestCallbackFn_t TxRxCallback;      // function to call after TxRxPacket
    wiTestCallbackFn_t OnePacketCallback; // function to call after wiTest_OnePacket()
    wiTestCallbackFn_t ScriptCallback;    // function to call on script command wiTest_Callback()

    // store definition for "background packets" to be transmitted before the primary packet.
    // Used by wiTest_OnePacketTxMultiple() and wiTest_OnePacketTx2()
    //
    wiBoolean  MultipleTxPackets;         // transmit background packets
    wiUInt     NumberOfBackgroundPackets; // number of packets in BackgroundPacket

    wiTestBackgroundPacket_t BackgroundPacket[WITEST_MAX_BACKGROUNDPACKETS+1];

    struct
    {
        wiUInt NumberOfPoints;
        wiReal ParamValue      [WITEST_MAX_POINTS];
        wiUInt BitCount        [WITEST_MAX_POINTS];
        wiUInt BitErrorCount   [WITEST_MAX_POINTS];
        wiUInt PacketCount     [WITEST_MAX_POINTS];
        wiUInt PacketErrorCount[WITEST_MAX_POINTS];
        wiReal BER             [WITEST_MAX_POINTS];
        wiReal PER             [WITEST_MAX_POINTS];

    } CountByPoint; // statistics by parameter value in a PER vs. <Param> test

}  wiTest_State_t;

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION PROTOTYPES
//
wiStatus wiTest_Init(void);
wiStatus wiTest_Close(void);
wiStatus wiTestWr(char *format, ...);

wiTest_State_t * wiTest_State(void);
wiStatus wiTest_SetTxRxCallback     (wiTestCallbackFn_t Fn);
wiStatus wiTest_SetOnePacketCallback(wiTestCallbackFn_t Fn);
wiStatus wiTest_SetScriptCallback   (wiTestCallbackFn_t Fn);

wiStatus wiTest_SingleByteMessage(int n, wiUWORD buf[], unsigned pat);
wiStatus wiTest_RandomMessage    (int n, wiUWORD buf[]);
wiStatus wiTest_Annex_G_Message  (int n, wiUWORD buf[]);

wiBoolean wiTest_TestIsDone(void);
wiStatus  wiTest_ClearTestIsDone(void);

wiStatus wiTest_ClearCounters(void);
wiStatus wiTest_StartTest(wiString TestFnName);
wiStatus wiTest_EndTest(void);
wiStatus wiTest_WriteConfig(wiMessageFn MsgFunc);

wiStatus wiTest_SetTxData(wiInt NumBytes, wiUWORD *pDataBuffer);
wiStatus wiTest_GetTxData(wiInt DataType, wiInt NumBytes, wiUWORD *pDataBuffer);
wiStatus wiTest_TxPacket( wiInt Mode, wiReal Pr_dBm, wiReal bps, wiInt nBytes, 
                          wiInt DataType, int *pN, wiComplex ***pR, wiReal *pFs );
wiStatus wiTest_TxRxPacket(wiInt Mode, wiReal Pr_dBm, wiReal bps, wiInt nBytes);

//-------------------------------------------------------------------------------------------------
#if defined(__cplusplus) || defined(__cplusplus__)
    }
#endif

#endif
//-------------------------------------------------------------------------------------------------
//--- End of File ---------------------------------------------------------------------------------
