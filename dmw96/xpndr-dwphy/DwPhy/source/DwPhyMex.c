//--< DwPhyMex.c >---------------------------------------------------------------------------------
//=================================================================================================
//
//  MATLAB Interface to the DwPhy module and WLAN driver
//  Copyright 2007-2011 DSP Group, Inc. All rights reserved.
//
//  Written by Barrett Brickner
//  This module is part of the DwPhyTest PHY evaluation software
//
//=================================================================================================

#include <stdio.h>
#include <string.h>
#include <windows.h>

#include "mex.h"
#include "hw-phy.h"
#include "wiMexUtil.h"
#include "RvClient.h"
#include "WifiApi.h"

// ================================================================================================
// --  LOCAL FILE PARAMETERS
// ================================================================================================
static wiBoolean     Initialized = 0;  // Is the DwPhyMex module initialized?
static wiBoolean     Verbose = 0;      // Print diagnostic messages to the console
static dwDevInfo_t   DevInfo ={0};     // Device private information (used by DwPhy)
static dwDevInfo_t *pDevInfo =&DevInfo;// pointer to DevInfo (needed for DwPhy_WriteReg/ReadReg)

// ================================================================================================
// --  INTERNAL ERROR HANDLING
// ================================================================================================
#define DEBUG     mexPrintf("### FILE: %s, LINE: %d\n",__FILE__,__LINE__);
#define MCHK(fn)  if((fn) != 0) mexErrMsgTxt("Internal Function Error");

//==========================================================================
//  RANGE CHECKING
//  This macro inplements basic parameter range checking.
//==========================================================================
#ifndef InvalidRange
#define InvalidRange(x,minx,maxx) ((((x)<(minx)) || ((x)>(maxx))) ? 1 : 0)
#endif

// ================================================================================================
// --  INTERNAL FUNCTION PROTOTYPES
// ================================================================================================
static void __cdecl DwPhyMex_Init(void);
static void __cdecl DwPhyMex_Close(void);
uint32_t DwPhyEnableFn( dwDevInfo_t *pDevInfo, uint32_t Enable );

// ================================================================================================
// --  HIDDEN PROTOTYPES FROM DwPhy.c
// ================================================================================================
dwPhyStatus_t DwPhy_DefaultRegisters(uint32_t PhyConfiguration, int32_t Chipset, uint32_t* NumReg, dwPhyRegPair_t **pDefaultReg);

// ================================================================================================
// FUNCTION  : mexFunction
// ------------------------------------------------------------------------------------------------
// Purpose   : Entry point for calls from MATLAB
// Parameters: nlhs -- Number of passed left-hand-side parameters
//             plhs -- Pointer to the left-hand-side parameter list
//             nrhs -- Number of passed right-hand-side arguments
//             prhs -- Pointer to the right-hand-side argument list
// ================================================================================================
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    char *tcmd, cmd[256];  // passed command parameter

    if(Verbose)
    {
        if(nrhs) 
        {
            tcmd = wiMexUtil_GetStringValue(prhs[0],0);
            mexPrintf(" mexFunction(%d,*,%d,\"%s\")\n",nlhs,nrhs,tcmd);
            mxFree(tcmd);

        }
        else mexPrintf(" mexFunction(%d,*,%d,*)\n",nlhs,nrhs);
    }

    // --------------
    // Check for Init
    // --------------
    if(!Initialized) DwPhyMex_Init();

    // ------------------
    // Dummy library call
    // ------------------
    if(!nrhs && !nlhs) 
    {
        mexPrintf("\n");
        mexPrintf("   MATLAB Interface to the Kona WLAN Driver API (%s %s)\n",__DATE__,__TIME__);
        mexPrintf("   Copyright 2007-2011 DSP Group, Inc. All rights reserved.\n");
        mexPrintf("\n");
        return;
    }
    // ----------------------
    // Get the command string
    // ----------------------
    if(nrhs<1) mexErrMsgTxt("A string argument must be given as the first parameter.");
    tcmd = wiMexUtil_GetStringValue(prhs[0],0);          // first parameter should be a string
    strncpy(cmd, tcmd, 256); mxFree(tcmd); cmd[255] = 0; // copied and free tcmd as it is not free'd below

    // ********************************************
    // *****                                  *****
    // *****    COMMAND LOOKUP-AND-EXECUTE    *****
    // *****                                  *****
    // ********************************************

    //----- About --------------------------------
    //--------------------------------------------
    if(!strcmp(cmd,"About"))
    {
        char buf[2][256] = {"",""};
        wiMexUtil_CheckParameters(nlhs,nrhs,1,1);
        sprintf(buf[0],"DwPhyMex BUILD: %s %s",__DATE__,__TIME__);
        wiMexUtil_PutString(buf[0], &plhs[0]);
        return;
    }
    //----- RvClientConnect ----------------------
    //--------------------------------------------
    if(strstr(cmd, "RvClientConnect"))
    {
        char *Addr; uint16_t Port;
        RvStatus_t Status;

        wiMexUtil_CheckParameters(nlhs,nrhs,1,3);
        Addr = wiMexUtil_GetStringValue(prhs[1], 0);
        Port = (uint16_t)wiMexUtil_GetUIntValue  (prhs[2]);

        Status = RvClientConnect(Addr, Port);
        wiMexUtil_PutIntValue(Status, plhs);
        return;
    }
    //----- RvClientSeedRandomGenerator ----------
    //--------------------------------------------
    if(strstr(cmd, "RvClientSeedRandomGenerator"))
    {
        uint32_t Seed;
        RvStatus_t Status;

        wiMexUtil_CheckParameters(nlhs,nrhs,1,2);
        Seed = wiMexUtil_GetUIntValue(prhs[1]);
        Status = RvClientSeedRandomGenerator(Seed);
        wiMexUtil_PutIntValue(Status, plhs);
        return;
    }
    //----- RvClientPhyTestMode ------------------
    //--------------------------------------------
    if(strstr(cmd, "RvClientPhyTestMode"))
    {
        RvStatus_t Status;

        wiMexUtil_CheckParameters(nlhs,nrhs,1,1);
        Status = RvClientPhyTestMode();
        wiMexUtil_PutIntValue(Status, plhs);
        return;
    }
    //----- OS_RegWrite --------------------------
    //--------------------------------------------
    if(!strcmp(cmd, "OS_RegWrite"))
    {
        uint32_t addr, data;
        
        wiMexUtil_CheckParameters(nlhs,nrhs,0,3);
        addr = wiMexUtil_GetUIntValue(prhs[1]);
        data = wiMexUtil_GetUIntValue(prhs[2]);

        if (addr <= 0x80FF) DwPhy_WriteReg(addr, (uint8_t)data);
        else                OS_RegWrite(addr, data);
        
        return;
    }
    //----- OS_RegRead ---------------------------
    //--------------------------------------------
    if(!strcmp(cmd, "OS_RegRead"))
    {
        uint32_t addr, data = 0;
        
        wiMexUtil_CheckParameters(nlhs,nrhs,1,2);
        addr = wiMexUtil_GetUIntValue(prhs[1]);

        if (addr <= 0x80FF) data = DwPhy_ReadReg(addr);
        else                data = OS_RegRead(addr);
        
        wiMexUtil_PutUIntValue(data, plhs);
        return;
    }
    //----- OS_Delay -----------------------------
    //--------------------------------------------
    if(!strcmp(cmd, "OS_Delay"))
    {
        wiMexUtil_CheckParameters(nlhs,nrhs,0,2);
        OS_Delay( wiMexUtil_GetUIntValue(prhs[1]) );
        return;
    }
    //----- RvClientReadMib ----------------------
    //--------------------------------------------
    if(!strcmp(cmd, "RvClientReadMib"))
    {
        int     MibObjectID;
        int     MibObjectSize;
        uint8_t MibObject[RVWAL_MAXMIB] = {0};

        wiMexUtil_CheckParameters(nlhs,nrhs,1,3);

        MibObjectID   = wiMexUtil_GetIntValue(prhs[1]);
        MibObjectSize = wiMexUtil_GetIntValue(prhs[2]);

        if(InvalidRange(MibObjectID, 0, 65535)) mexErrMsgTxt("MibObjectID out of range");
        if(InvalidRange(MibObjectSize, 0, RVWAL_MAXMIB)) mexErrMsgTxt("MibObjectSize out of range");

        MCHK( RvClientReadMib((uint16_t)MibObjectID, MibObject, (size_t)MibObjectSize) );

        wiMexUtil_PutBinaryArray(MibObjectSize, MibObject, &plhs[0]);
        return;
    }
    //----- RvClientWriteMib ---------------------
    //--------------------------------------------
    if(!strcmp(cmd, "RvClientWriteMib"))
    {
        int     MibObjectID;
        int     MibObjectSize;
        uint8_t *pMibObject;

        wiMexUtil_CheckParameters(nlhs,nrhs,0,3);

        MibObjectID = wiMexUtil_GetIntValue(prhs[1]);
        pMibObject  = wiMexUtil_GetBinaryArray(prhs[2], &MibObjectSize);

        if(InvalidRange(MibObjectID, 0, 65535)) mexErrMsgTxt("MibObjectID out of range");
        if(InvalidRange(MibObjectSize, 0, RVWAL_MAXMIB)) mexErrMsgTxt("MibObjectSize out of range");

        MCHK( RvClientWriteMib((uint16_t)MibObjectID, pMibObject, (size_t)MibObjectSize) );
        mxFree(pMibObject);
        return;
    }
    //----- RvClientLoadTxBuffer -----------------
    //--------------------------------------------
    if(!strcmp(cmd, "RvClientLoadTxBuffer"))
    {
        uint8_t *pBuffer;
        int      Length;

        wiMexUtil_CheckParameters(nlhs,nrhs,0,2);

        pBuffer = (uint8_t *)wiMexUtil_GetByteArray(prhs[1], &Length);
        if(Length<14 || Length>4095) mexErrMsgTxt("TX Buffer size must be 14 to 4095 bytes");

        MCHK( RvClientLoadTxBuffer( Length, pBuffer ) );
        mxFree(pBuffer);
        return;
    }
    //----- RvClientTxBufferPacket ---------------
    //--------------------------------------------
    if(!strcmp(cmd, "RvClientTxBufferPacket"))
    {
        uint8_t PhyRate, PowerLevel;

        wiMexUtil_CheckParameters(nlhs,nrhs,0,3);

        PhyRate    = (uint8_t)wiMexUtil_GetUIntValue( prhs[1] );
        PowerLevel = (uint8_t)wiMexUtil_GetUIntValue( prhs[2] );

        MCHK( RvClientTxBufferPacket( PhyRate, PowerLevel ) );
        return;
    }
    //----- RvClientTxSinglePacket ---------------
    //--------------------------------------------
    if(!strcmp(cmd, "RvClientTxSinglePacket"))
    {
        uint32_t PacketType, LongPreamble = 0;
        uint16_t Length;
        uint8_t  PhyRate, PowerLevel;
        char msg[80];

        switch (nrhs)
        {
            case 5: LongPreamble = 0; break;
            case 6: LongPreamble = (uint32_t)wiMexUtil_GetUIntValue( prhs[5] ); break;
            default:
                sprintf(msg,"5 or 6 input parameters were expected, %d were received.\n",nrhs);
                mexErrMsgTxt(msg);
        }
        if(nlhs)
        {
            sprintf(msg,"No output terms were expected, %d were received.\n",nlhs);
            mexErrMsgTxt(msg);
        }

        Length     = (uint16_t)wiMexUtil_GetUIntValue( prhs[1] );
        PhyRate    = (uint8_t )wiMexUtil_GetUIntValue( prhs[2] );
        PowerLevel = (uint8_t )wiMexUtil_GetUIntValue( prhs[3] );
        PacketType = (uint32_t)wiMexUtil_GetUIntValue( prhs[4] );

        MCHK( RvClientTxSinglePacket( Length, PhyRate, PowerLevel, PacketType, LongPreamble ) );
        return;
    }
    //----- RvClientTxMulitplePackets ------------
    //--------------------------------------------
    if(!strcmp(cmd, "RvClientTxMultiplePackets"))
    {
        uint8_t NumPkts;
        uint32_t PacketType[RVWAL_NUMPKT] = {0};
        uint16_t Length[RVWAL_NUMPKT] = {0};
        uint8_t  PhyRate[RVWAL_NUMPKT] = {0};
        uint8_t  PowerLevel[RVWAL_NUMPKT] = {0};
        uint32_t LongPreamble[RVWAL_NUMPKT] = {0};
        int *x[5], n[5], i;

        wiMexUtil_CheckParameters(nlhs,nrhs,0,6);

        for (i=0; i<5; i++) {
            x[i] = (int *)wiMexUtil_GetIntArray( prhs[i+1], n+i );
        }
        if ((n[0]==n[1]) && (n[0]==n[2]) && (n[0]==n[3]) && (n[0]==n[4]))
        {
            NumPkts = (uint8_t)n[0];
            for (i=0; i<NumPkts; i++)
            {
                Length[i]       = (uint16_t)(x[0][i]);
                PhyRate[i]      = (uint8_t )(x[1][i]);
                PowerLevel[i]   = (uint8_t )(x[2][i]);
                PacketType[i]   = (uint32_t)(x[3][i]);
                LongPreamble[i] = (uint32_t)(x[4][i]);
            }
            MCHK( RvClientTxMultiplePackets( NumPkts, Length, PhyRate, PowerLevel, PacketType, LongPreamble ) );
            for (i=0; i<5; i++) mxFree(x[i]);
        }
        else mexErrMsgTxt("Array length mismatch");
        return;
    }
    //----- RvClientStartTxBatchBuffer ------
    //---------------------------------------
    if(!strcmp(cmd, "RvClientStartTxBatchBuffer"))
    {
        uint32_t NumTransmits, LongPreamble = 0;
        uint8_t  PhyRate, PowerLevel;
        char msg[80];

        switch (nrhs)
        {
            case 4: LongPreamble = 0; break;
            case 5: LongPreamble = (uint32_t)wiMexUtil_GetUIntValue( prhs[6] ); break;
            default:
                sprintf(msg,"6 or 7 input parameters were expected, %d were received.\n",nrhs);
                mexErrMsgTxt(msg);
        }
        if(nlhs)
        {
            sprintf(msg,"No output terms were expected, %d were received.\n",nlhs);
            mexErrMsgTxt(msg);
        }

        NumTransmits = (uint32_t)wiMexUtil_GetUIntValue( prhs[1] );
        PhyRate      = (uint8_t )wiMexUtil_GetUIntValue( prhs[2] );
        PowerLevel   = (uint8_t )wiMexUtil_GetUIntValue( prhs[3] );

        MCHK( RvClientStartTxBatchBuffer( NumTransmits, PhyRate, PowerLevel, LongPreamble ) );
        return;
    }
    //----- RvClientStartTxBatch ------------
    //---------------------------------------
    if(!strcmp(cmd, "RvClientStartTxBatch"))
    {
        uint32_t PacketType, NumTransmits, LongPreamble = 0;
        uint16_t Length;
        uint8_t  PhyRate, PowerLevel;
        char msg[80];

        switch (nrhs)
        {
            case 6: LongPreamble = 0; break;
            case 7: LongPreamble = (uint32_t)wiMexUtil_GetUIntValue( prhs[6] ); break;
            default:
                sprintf(msg,"6 or 7 input parameters were expected, %d were received.\n",nrhs);
                mexErrMsgTxt(msg);
        }
        if(nlhs)
        {
            sprintf(msg,"No output terms were expected, %d were received.\n",nlhs);
            mexErrMsgTxt(msg);
        }

        NumTransmits = (uint32_t)wiMexUtil_GetUIntValue( prhs[1] );
        Length       = (uint16_t)wiMexUtil_GetUIntValue( prhs[2] );
        PhyRate      = (uint8_t )wiMexUtil_GetUIntValue( prhs[3] );
        PowerLevel   = (uint8_t )wiMexUtil_GetUIntValue( prhs[4] );
        PacketType   = (uint32_t)wiMexUtil_GetUIntValue( prhs[5] );

        MCHK( RvClientStartTxBatch( NumTransmits, Length, PhyRate, PowerLevel, PacketType, LongPreamble ) );
        return;
    }
    //----- RvClientStopTxBatch ------------
    //--------------------------------------
    if(!strcmp(cmd, "RvClientStopTxBatch"))
    {
        wiMexUtil_CheckParameters(nlhs,nrhs,0,1);
        MCHK( RvClientStopTxBatch() );
        return;
    }
    //----- RvClientQueryTxBatch ------------
    //--------------------------------------
    if(!strcmp(cmd, "RvClientQueryTxBatch"))
    {
        uint8_t TxBatchStatus;
        
        wiMexUtil_CheckParameters(nlhs,nrhs,1,1);
        MCHK( RvClientQueryTxBatch(&TxBatchStatus) );
        wiMexUtil_PutUIntValue(TxBatchStatus, plhs);
        return;
    }
    //----- RvClientRxSinglePacket ---------------
    //--------------------------------------------
    if(!strcmp(cmd, "RvClientRxSinglePacket"))
    {
        const unsigned Dimensions[2] = {1,1};
        const char *FieldNames[] = {
            "Length","RxBitRate","RSSI0","RSSI1","Packet","Flags","tsfTimestamp"
        };
        int NumFields = sizeof(FieldNames)/sizeof(*FieldNames);
        
        RvRxPacketFrame_t Frame;
    
        wiMexUtil_CheckParameters(nlhs,nrhs,1,1);
        MCHK( RvClientRxSinglePacket( &Frame ) );

        plhs[0] = mxCreateStructArray(2, Dimensions, NumFields, FieldNames);
        if(!plhs[0]) mexErrMsgTxt("Memory Allocation Error");

        mxSetField(plhs[0], 0, "Length",    mxCreateDoubleScalar(Frame.Length               ));
        mxSetField(plhs[0], 0, "RxBitRate", mxCreateDoubleScalar(Frame.PhyRate              ));
        mxSetField(plhs[0], 0, "RSSI0",     mxCreateDoubleScalar(Frame.RSSI0                ));
        mxSetField(plhs[0], 0, "RSSI1",     mxCreateDoubleScalar(Frame.RSSI1                ));
        mxSetField(plhs[0], 0, "Flags",     mxCreateDoubleScalar(Frame.flags ));
        mxSetField(plhs[0], 0, "tsfTimestamp",mxCreateDoubleScalar(Frame.tsfTimestamp ));

        if(Frame.Length)
        {
            mxArray *pm;
            void *pData;
            mwSize length = Frame.Length;
            
            pm = mxCreateNumericArray(1, &length, mxUINT8_CLASS, mxREAL);
            pData = mxGetData(pm);
            memcpy(pData, Frame.Packet, Frame.Length);
            mxSetField(plhs[0], 0, "Packet", pm);
        }
        return;
    }
    //----- RvClientSystem -----------------------
    //--------------------------------------------
    if(strstr(cmd, "RvClientSystem"))
    {
        char *CommandString;

        wiMexUtil_CheckParameters(nlhs,nrhs,0,2);
        CommandString = wiMexUtil_GetStringValue(prhs[1], NULL);
        MCHK( RvClientSystem(CommandString) );
        mxFree(CommandString);
        return;
    }
    //----- RvClientGetPerCounters ---------------
    //--------------------------------------------
    if(strstr(cmd, "RvClientGetPerCounters"))
    {
        const unsigned Dimensions[2] = {1,1};
        const char *FieldNames[] = {
            "ReceivedFragmentCount","FCSErrorCount","Length","RxBitRate","RSSI0","RSSI1",
            "BSSType","ConnectionState","DFS"
        };
        int NumFields = sizeof(FieldNames)/sizeof(*FieldNames);
        
        RvPerFrame_t Frame;
    
        wiMexUtil_CheckParameters(nlhs,nrhs,1,1);
        MCHK( RvClientGetPerCounters( &Frame ) );

        plhs[0] = mxCreateStructArray(2, Dimensions, NumFields, FieldNames);
        if(!plhs[0]) mexErrMsgTxt("Memory Allocation Error");

        mxSetField(plhs[0], 0, "ReceivedFragmentCount",mxCreateDoubleScalar(Frame.ReceivedFragmentCount));
        mxSetField(plhs[0], 0, "FCSErrorCount",        mxCreateDoubleScalar(Frame.FCSErrorCount        ));
        mxSetField(plhs[0], 0, "Length",               mxCreateDoubleScalar(Frame.Length               ));
        mxSetField(plhs[0], 0, "RxBitRate",            mxCreateDoubleScalar(Frame.RxBitRate            ));
        mxSetField(plhs[0], 0, "RSSI0",                mxCreateDoubleScalar(Frame.RSSI0                ));
        mxSetField(plhs[0], 0, "RSSI1",                mxCreateDoubleScalar(Frame.RSSI1                ));
        mxSetField(plhs[0], 0, "BSSType",              mxCreateDoubleScalar(Frame.BSSType              ));
        mxSetField(plhs[0], 0, "ConnectionState",      mxCreateDoubleScalar(Frame.ConnectionState      ));
        mxSetField(plhs[0], 0, "DFS",                  mxCreateDoubleScalar(Frame.DFS                  ));
        return;
    }
    //----- RvClientRxRSSI -----------------------
    //--------------------------------------------
    if(!strcmp(cmd, "RvClientRxRSSI"))
    {
        RvRxPacketFrame_t Frame;
    
        wiMexUtil_CheckParameters(nlhs,nrhs,1,1);
        MCHK( RvClientRxRSSI( &Frame ) );

        wiMexUtil_PutByteArray(Frame.Length, Frame.Packet, plhs);
        return;
    }
    //----- RvClientRxRSSIValues -----------------
    //--------------------------------------------
    if(!strcmp(cmd, "RvClientRxRSSIValues"))
    {
        RvBufferFrame_t Frame;
    
        wiMexUtil_CheckParameters(nlhs,nrhs,1,1);
        MCHK( RvClientRxRSSIValues( &Frame ) );

        wiMexUtil_PutByteArray(Frame.Length, Frame.Buffer, plhs);
        return;
    }
    //----- RvClientRegReadMultiple --------------
    //--------------------------------------------
    if(!strcmp(cmd, "RvClientRegReadMultiple"))
    {
        RvBufferFrame_t Frame;
        uint32_t Addr;
    
        wiMexUtil_CheckParameters(nlhs,nrhs,1,2);
        Addr = wiMexUtil_GetUIntValue(prhs[1]);

        MCHK( RvClientRegReadMultiple( &Frame, Addr ) );

        wiMexUtil_PutByteArray(Frame.Length, Frame.Buffer, plhs);
        return;
    }
    //----- RvClientReadTPC ----------------------
    //--------------------------------------------
    if(!strcmp(cmd, "RvClientReadTPC"))
    {
        RvRamFrame_t Frame;
        uint32_t DataAddr[4] = {1003, 1002, 1001, 0};
    
        wiMexUtil_CheckParameters(nlhs,nrhs,1,1);
    
        MCHK( RvClientReadRAM256( &Frame, 1000, DataAddr ) );
        wiMexUtil_PutUWORDArray(256, (wiUWORD *)Frame.Data, plhs);
        return;
    }
    //----- RvClientReadMacAddress ---------------
    //--------------------------------------------
    if(!strcmp(cmd, "RvClientReadMacAddress"))
    {
        uint8_t buf[6];
        wiMexUtil_CheckParameters(nlhs,nrhs,1,1);
        MCHK( RvClientReadMacAddress(buf) );
        wiMexUtil_PutBinaryArray(6, buf, plhs);
        return;
    }
    //----- RvClientWriteMacAddress --------------
    //--------------------------------------------
    if(!strcmp(cmd, "RvClientWriteMacAddress"))
    {
        uint8_t *buf;
        wiMexUtil_CheckParameters(nlhs,nrhs,0,2);
        buf = wiMexUtil_GetBinaryArray(prhs[1], 0);
        MCHK( RvClientWriteMacAddress(buf) );
        return;
    }
    //----- RvClientShutdown ---------------------
    //--------------------------------------------
    if(!strcmp(cmd, "RvClientShutdown"))
    {
        wiMexUtil_CheckParameters(nlhs,nrhs,0,1);
        MCHK( RvClientShutdown() );
        return;
    }
    //----- SetVerbose ---------------------------
    //--------------------------------------------
    if(strstr(cmd, "SetVerbose"))
    {
        wiMexUtil_CheckParameters(nlhs,nrhs,0,2);
        Verbose = wiMexUtil_GetIntValue(prhs[1]);
        RvClientDisplay(mexPrintf, Verbose);
        return;
    }
    //-----------------------------------------------------------------------
    //-----
    //----- DwPhy Commands 
    //-----
    //-----------------------------------------------------------------------
    if(strstr(cmd, "DwPhy_"))
    {
        //----- DwPhy_ChipSet ------------------------
        //--------------------------------------------
        if(strstr(cmd, "DwPhy_ChipSet"))
        {
            int chipset;

            wiMexUtil_CheckParameters(nlhs,nrhs,1,1);
            chipset = DwPhy_ChipSet(pDevInfo);
            wiMexUtil_PutIntValue(chipset, plhs);
            return;
        }
        //----- DwPhy_DefaultRegisters ---------------
        //--------------------------------------------
        if(strstr(cmd, "DwPhy_DefaultRegisters"))
        {
            unsigned int i, n; int chipset;
            dwPhyRegPair_t *DefaultReg;

            wiMexUtil_CheckParameters(nlhs,nrhs,1,2);
            chipset = wiMexUtil_GetIntValue(prhs[1]);
            if(DwPhy_DefaultRegisters(0, chipset, &n, &DefaultReg) == DWPHY_SUCCESS)
            {
                wiUWORD x[2048];
                for(i=0; i<n; i++)
                {
                    x[2*i+0].word = DefaultReg[i].addr;
                    x[2*i+1].word = DefaultReg[i].data;
                }
                wiMexUtil_PutUWORDArray(2*n, x, plhs);
            }
            else mexErrMsgTxt("Unabled to retrieve register values\n");

            return;
        }
        //----- DwPhy_Reset --------------------------
        //--------------------------------------------
        if(strstr(cmd, "DwPhy_Reset"))
        {
            dwPhyStatus_t status;
            wiMexUtil_CheckParameters(nlhs,nrhs,1,1);
            status = DwPhy_Reset(pDevInfo);
            wiMexUtil_PutIntValue(status, plhs);
            return;
        }
        //----- DwPhy_CalibrateDataConv --------------
        //--------------------------------------------
        if(strstr(cmd, "DwPhy_CalibrateDataConv"))
        {
            wiMexUtil_CheckParameters(nlhs,nrhs,0,1);
            DwPhy_CalibrateDataConv(pDevInfo);
            return;
        }
        //----- DwPhy_Initialize ---------------------
        //--------------------------------------------
        if(strstr(cmd, "DwPhy_Initialize"))
        {
            dwPhyStatus_t status;
            dwPhyPlatform_t PlatformID;
            char msg[80];

            if (nrhs > 2) {
                sprintf(msg,"1 or 2 input parameters were expected, %d were received.\n",nrhs);
                mexErrMsgTxt(msg);
            }
            if (nlhs != 1) {
                sprintf(msg,"1 output terms was expected, %d were received.\n",nlhs);
                mexErrMsgTxt(msg);
            }
            if (nrhs >= 2)
                PlatformID = wiMexUtil_GetIntValue(prhs[1]);
            else
                PlatformID = DWPHY_PLATFORM_DEFAULT;
            
            status = DwPhy_Initialize(pDevInfo, PlatformID);
            wiMexUtil_PutIntValue(status, plhs);
            return;
        }
        //----- DwPhy_Sleep --------------------------
        //--------------------------------------------
        if(strstr(cmd, "DwPhy_Sleep"))
        {
            dwPhyStatus_t status;
            wiMexUtil_CheckParameters(nlhs,nrhs,1,1);
            status = DwPhy_Sleep(pDevInfo);
            wiMexUtil_PutIntValue(status, plhs);
            return;
        }
        //----- DwPhy_Wake ---------------------------
        //--------------------------------------------
        if(strstr(cmd, "DwPhy_Wake"))
        {
            dwPhyStatus_t status;
            wiMexUtil_CheckParameters(nlhs,nrhs,1,1);
            status = DwPhy_Wake(pDevInfo);
            wiMexUtil_PutIntValue(status, plhs);
            return;
        }
        //----- DwPhy_SetChannelFreq -----------------
        //--------------------------------------------
        if(strstr(cmd, "DwPhy_SetChannelFreq"))
        {
            unsigned int FcMHz;
            dwPhyStatus_t status;

            wiMexUtil_CheckParameters(nlhs,nrhs,1,2);
            FcMHz = wiMexUtil_GetUIntValue(prhs[1]);
            status = DwPhy_SetChannelFreq(pDevInfo, FcMHz);
            wiMexUtil_PutIntValue(status, plhs);
            return;
        }
        //----- DwPhy_PllClosedLoopCalibration -------
        //--------------------------------------------
        if(strstr(cmd, "DwPhy_PllClosedLoopCalibration"))
        {
            wiMexUtil_CheckParameters(nlhs,nrhs,0,1);
            DwPhy_PllClosedLoopCalibration(pDevInfo);
            return;
        }
        //----- DwPhy_WriteReg -----------------------
        //--------------------------------------------
        if(strstr(cmd, "DwPhy_WriteReg"))
        {
            unsigned int addr, data;
            
            wiMexUtil_CheckParameters(nlhs,nrhs,0,3);
            addr = wiMexUtil_GetUIntValue(prhs[1]);
            data = wiMexUtil_GetUIntValue(prhs[2]);
            DwPhy_WriteReg(addr, (uint8_t)data);
            return;
        }
        //----- DwPhy_ReadReg ------------------------
        //--------------------------------------------
        if(strstr(cmd, "DwPhy_ReadReg"))
        {
            unsigned int addr, data;
        
            wiMexUtil_CheckParameters(nlhs,nrhs,1,2);
            addr = wiMexUtil_GetUIntValue(prhs[1]);
            data = DwPhy_ReadReg(addr);
            wiMexUtil_PutUIntValue(data, plhs);
            return;
        }
        //----- DwPhy_SetDiversityMode ---------------
        //--------------------------------------------
        if(strstr(cmd, "DwPhy_SetDiversityMode"))
        {
            dwPhyRxMode_t DiversityMode;
            dwPhyStatus_t status;

            wiMexUtil_CheckParameters(nlhs,nrhs,1,2);
            DiversityMode = wiMexUtil_GetIntValue(prhs[1]);
            status = DwPhy_SetDiversityMode(pDevInfo, DiversityMode);
            wiMexUtil_PutIntValue(status, plhs);
            return;
        }
        //----- DwPhy_ConvertHeaderRSSI --------------
        //--------------------------------------------
        if(strstr(cmd, "DwPhy_ConvertHeaderRSSI"))
        {
            uint8_t RSSI0, RSSI1;

            wiMexUtil_CheckParameters(nlhs,nrhs,2,3);
            RSSI0 = (uint8_t)wiMexUtil_GetIntValue(prhs[1]);
            RSSI1 = (uint8_t)wiMexUtil_GetIntValue(prhs[2]);
            DwPhy_ConvertHeaderRSSI(pDevInfo, &RSSI0, &RSSI1);
            wiMexUtil_PutIntValue(RSSI0, plhs+0);
            wiMexUtil_PutIntValue(RSSI1, plhs+1);
            return;
        }
        //----- DwPhy_SetRxMode ----------------------
        //--------------------------------------------
        if(strstr(cmd, "DwPhy_SetRxMode"))
        {
            dwPhyRxMode_t RxMode;
            dwPhyStatus_t status;

            wiMexUtil_CheckParameters(nlhs,nrhs,1,2);
            RxMode = (int8_t)(wiMexUtil_GetIntValue(prhs[1]));
            status = DwPhy_SetRxMode(pDevInfo, RxMode);
            wiMexUtil_PutIntValue(status, plhs);
            return;
        }
        //----- DwPhy_SetRxSensitivity ---------------
        //--------------------------------------------
        if(strstr(cmd, "DwPhy_SetRxSensitivity"))
        {
            int8_t dBm;
            dwPhyStatus_t status;

            wiMexUtil_CheckParameters(nlhs,nrhs,1,2);
            dBm = (int8_t)(wiMexUtil_GetIntValue(prhs[1]));
            status = DwPhy_SetRxSensitivity(pDevInfo, dBm);
            wiMexUtil_PutIntValue(status, plhs);
            return;
        }
        //----- DwPhy_GetRxSensitivity ---------------
        //--------------------------------------------
        if(strstr(cmd, "DwPhy_GetRxSensitivity"))
        {
            int8_t dBm = -1;

            wiMexUtil_CheckParameters(nlhs,nrhs,1,1);
            DwPhy_GetRxSensitivity(pDevInfo, &dBm);
            wiMexUtil_PutIntValue(dBm, plhs);
            return;
        }
        //----- DwPhy_AddressFilter ------------------
        //--------------------------------------------
        if(strstr(cmd, "DwPhy_AddressFilter"))
        {
            uint8_t Enable;
            dwPhyStatus_t status;

            wiMexUtil_CheckParameters(nlhs,nrhs,1,2);
            Enable = (uint8_t)wiMexUtil_GetUIntValue(prhs[1]);
            status = DwPhy_AddressFilter(pDevInfo, Enable);
            wiMexUtil_PutIntValue(status, plhs);
            return;
        }
        //----- DwPhy_SetStationAddress --------------
        //--------------------------------------------
        if(strstr(cmd, "DwPhy_SetStationAddress"))
        {
            uint8_t *pAddr;
            dwPhyStatus_t status;

            wiMexUtil_CheckParameters(nlhs,nrhs,1,2);
            pAddr = wiMexUtil_GetBinaryArray(prhs[1], 0);
            status = DwPhy_SetStationAddress(pDevInfo, pAddr);
            wiMexUtil_PutIntValue(status, plhs);
            mxFree(pAddr);
            return;
        }
        //----- DwPhy_SetParameter -------------------
        //--------------------------------------------
        if(strstr(cmd, "DwPhy_SetParameterData"))
        {
            uint8_t *pData;
            dwPhyParam_t Parameter;
			int DataSize;
            dwPhyStatus_t status;

            wiMexUtil_CheckParameters(nlhs,nrhs,1,3);
            pData     = wiMexUtil_GetBinaryArray(prhs[2], &DataSize);
            Parameter = (dwPhyParam_t)wiMexUtil_GetUIntValue  (prhs[1]);
            status = DwPhy_SetParameterData(pDevInfo, Parameter, (uint32_t)DataSize, pData);
            wiMexUtil_PutIntValue(status, plhs);
            if (pData) mxFree(pData);
            return;
        }
        //----- DwPhy_Startup ------------------------
        //--------------------------------------------
        if(strstr(cmd, "DwPhy_Startup"))
        {
            dwPhyStatus_t status;
            wiMexUtil_CheckParameters(nlhs,nrhs,1,1);
            status = DwPhy_Startup(pDevInfo);
            wiMexUtil_PutIntValue(status, plhs);
            return;
        }
        //----- DwPhy_Shutdown -----------------------
        //--------------------------------------------
        if(strstr(cmd, "DwPhy_Shutdown"))
        {
            dwPhyStatus_t status;
            wiMexUtil_CheckParameters(nlhs,nrhs,1,1);
            status = DwPhy_Shutdown(pDevInfo);
            wiMexUtil_PutIntValue(status, plhs);
            return;
        }
        //----- DwPhy_SetDwPhyEnableFn ---------------
        //--------------------------------------------
        if(strstr(cmd, "DwPhy_SetDwPhyEnableFn"))
        {
            wiMexUtil_CheckParameters(nlhs,nrhs,0,1);
            DwPhy_SetDwPhyEnableFn(pDevInfo, DwPhyEnableFn);
            return;
        }    
        //----- DwPhy_RF52B21_ToggleSTARTO -----------
        //--------------------------------------------
        if(strstr(cmd, "DwPhy_RF52B21_ToggleSTARTO"))
        {
            wiMexUtil_CheckParameters(nlhs,nrhs,0,1);
            DwPhy_RF52B21_ToggleSTARTO(pDevInfo);
            return;
        }
        //----- DwPhy_GetHistogramRPI ----------------
        //--------------------------------------------
        if (strstr(cmd, "DwPhy_GetHistogramRPI"))
        {
            uint16_t NumSamples;
            uint8_t Density[8];

            wiMexUtil_CheckParameters(nlhs,nrhs,1,2);
            NumSamples = (uint16_t)wiMexUtil_GetUIntValue  (prhs[1]);
            DwPhy_GetHistogramRPI(pDevInfo, NumSamples, Density);
            wiMexUtil_PutByteArray(8, Density, &plhs[0]);
            return;
        }
        //----- DwPhy_CalibrateTXDCOC_RF22 -----------
        //--------------------------------------------
        if(strstr(cmd, "DwPhy_CalibrateTXDCOC_RF22"))
        {
            dwPhyStatus_t status;
            wiMexUtil_CheckParameters(nlhs,nrhs,1,1);
            status = DwPhy_CalibrateTXDCOC_RF22(pDevInfo);
            wiMexUtil_PutIntValue(status, plhs);            
            return;
        }
        //----- DwPhy_CalibrateLOFT_RF22 -------------
        //--------------------------------------------
        if(strstr(cmd, "DwPhy_CalibrateLOFT_RF22"))
        {
            dwPhyStatus_t status;
            wiMexUtil_CheckParameters(nlhs,nrhs,1,1);
            status = DwPhy_CalibrateLOFT_RF22(pDevInfo);
            wiMexUtil_PutIntValue(status, plhs);            
            return;
        }
        //----- DwPhy_CalibrateIQ_RF22 ---------------
        //--------------------------------------------
        if(strstr(cmd, "DwPhy_CalibrateIQ_RF22"))
        {
            dwPhyStatus_t status;
            wiMexUtil_CheckParameters(nlhs,nrhs,1,1);
            status = DwPhy_CalibrateIQ_RF22(pDevInfo);
            wiMexUtil_PutIntValue(status, plhs);            
            return;
        }
        //----- DwPhy_CalibrateIQ_DMW96 -------------
        //--------------------------------------------
        if(strstr(cmd, "DwPhy_CalibrateIQ_DMW96"))
        {
            dwPhyStatus_t status;
            wiMexUtil_CheckParameters(nlhs,nrhs,1,1);
            status = DwPhy_CalibrateIQ_DMW96(pDevInfo);
            wiMexUtil_PutIntValue(status, plhs);            
            return;
        }
        //----- DwPhy_CalibrateIQ --------------------
        //--------------------------------------------
        if(strstr(cmd, "DwPhy_CalibrateIQ"))
        {
            dwPhyStatus_t status;
            wiMexUtil_CheckParameters(nlhs,nrhs,1,1);
            status = DwPhy_CalibrateIQ(pDevInfo);
            wiMexUtil_PutIntValue(status, plhs);            
            return;
        }
        //----- DwPhy_CalibrateXtal------------------
        //--------------------------------------------
        if(strstr(cmd, "DwPhy_CalibrateXtal"))
        {
            dwPhyStatus_t status;
            wiMexUtil_CheckParameters(nlhs,nrhs,1,1);
            status = DwPhy_CalibrateXtal(pDevInfo);
            wiMexUtil_PutIntValue(status, plhs);            
            return;
        }
        //----- DwPhy_CalibrateRxLPF------------------
        //--------------------------------------------
        if(strstr(cmd, "DwPhy_CalibrateRxLPF"))
        {
            dwPhyStatus_t status;
            wiMexUtil_CheckParameters(nlhs,nrhs,1,1);
            status = DwPhy_CalibrateRxLPF(pDevInfo);
            wiMexUtil_PutIntValue(status, plhs);            
            return;
        }

        

    }
    // --------------------------------------------
    // No Command Found...Generate an Error Message
    // --------------------------------------------
    mexErrMsgTxt("DwPhyMex: Unrecognized command in the first argument.");
}
// end of mexFunction()

// ================================================================================================
// FUNCTION   : DwPhyEnableFn()
// ------------------------------------------------------------------------------------------------
// Purpose    : Function to set the state of DW_PHYEnB
// Parameters : pDevInfo -- Device information
//              Enable   -- PHY state (0=sleep, 1=wake)
// ================================================================================================
uint32_t DwPhyEnableFn( dwDevInfo_t *pDevInfo, uint32_t Enable )
{
    OS_RegWrite( 0xFFFFFE3D, Enable );
    return 0;
}
// end of DwPhyEnableFn()

// ================================================================================================
// FUNCTION  : DwPhyMex_Init()
// ------------------------------------------------------------------------------------------------
// Purpose   : Initialize this DLL when loaded into MATLAB
// Parameters: none
// ================================================================================================
void __cdecl DwPhyMex_Init(void)
{
    if(Verbose) mexPrintf(" DwPhyMex_Init()\n");

    RvClientDisplay(mexPrintf, Verbose); // assign "printf" handler function

/**
    // just a quick endian check
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
**
    // check process to get user variable
    {
        char *Computer = getenv("COMPUTERNAME");
        mexPrintf("Computer = '%s'\n",Computer);
    }
/**/

    // -----------------------------------------------
    // Register the close/cleanup function with MATLAB
    // -----------------------------------------------
    mexAtExit(DwPhyMex_Close);

    // ------------------------------------------------------------------
    // Startup the DwPhy module and assign the DW_PHYEnB control function
    // ------------------------------------------------------------------
    DwPhy_Startup(pDevInfo);
    DwPhy_SetDwPhyEnableFn(pDevInfo, DwPhyEnableFn);

    Initialized = 1;
}
// end of DwPhyMex_Init()

// ================================================================================================
// FUNCTION  : DwPhyMex_Close()
// ------------------------------------------------------------------------------------------------
// Purpose   : Shutdown/close this module (when unloaded from MATLAB)
// Parameters: none
// ================================================================================================
void __cdecl DwPhyMex_Close(void)
{
    if(Verbose) mexPrintf(" DwPhyMex_Close()\n");
    DwPhy_Shutdown(pDevInfo);
    Initialized = 0;
}
// end of DwPhyMex_Close()

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------

// Revised 2010-11-03
// - Add TXDCOC/LOFT/IQ calibration interface for RF22
//
// Revised 2010-12-18
// - Add 'RvClientQueryTxBatch' command to retrieve status of Tx Batch.
