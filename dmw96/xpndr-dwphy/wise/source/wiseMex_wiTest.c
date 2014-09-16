//--< wiseMex_wiTest.c >-----------------------------------------------------------------------------
//=================================================================================================
//
//  wiseMex Command Dispatch for wiTest Functions
//  Copyright 2008-2009 DSP Group, Inc. All rights reserved.
//
//=================================================================================================

#include <string.h>
#include "wiseMex.h"
#include "wiTest.h"
#include "wiPHY.h"
#include "wiMAC.h"
#include "wiParse.h"

#define COMMAND(TargetStr) (wiMexUtil_CommandMatch(cmd, TargetStr))
#define MATCH(s1,s2)       ((strcmp(s1,s2) == 0) ? 1:0)

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Module Local Parameters and Macro Definitions
//
static char TxRxMatlabFunction[128]      = {""};
static char OnePacketMatlabFunction[128] = {""};
static char ScriptMatlabFunction[128]    = {""};

static mxArray *pX = NULL;

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Module Local Function Prototypes
//
wiStatus wiseMex_wiTest_TxRxCallback(void);
wiStatus wiseMex_wiTest_OnePacketCallback(void);
wiStatus wiseMex_wiTest_ScriptCallback(void);

// ================================================================================================
// FUNCTION  : wiseMex_wiTest()
// ------------------------------------------------------------------------------------------------
// Purpose   : Command lookup and execute from wiseMex
// Parameters: nlhs -- Number of passed left-hand-side parameters
//             plhs -- Pointer to the left-hand-side parameter list
//             nrhs -- Number of passed right-hand-side arguments
//             prhs -- Pointer to the right-hand-side argument list
//             cmd  -- Command string
// ================================================================================================
wiStatus wiseMex_wiTest(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[], char *cmd)
{
    if (strstr(cmd, "wiTest_"))
    {
	    if COMMAND( "wiTest_GetConfig" )
	    {
	        wiMexUtil_CheckParameters(nlhs,nrhs,1,1);
	        MCHK(wiTest_WriteConfig(wiMexUtil_MsgFunc));
	        wiMexUtil_PutString(wiMexUtil_GetMsgFuncBuffer(), plhs);
	        wiMexUtil_ResetMsgFunc();
	        return WI_SUCCESS;
	    }
		//-------------------------------------------------------------------------------
        if COMMAND( "wiTest_WriteConfig" )
        {
            wiMexUtil_CheckParameters(nlhs,nrhs,0,1);
            MCHK(wiTest_WriteConfig(wiMexUtil_Output));
            return WI_SUCCESS;
        }
        //-------------------------------------------------------------------------------
        if COMMAND( "wiTest_ClearCounters" )
        {
            wiMexUtil_CheckParameters(nlhs,nrhs,0,1);
            MCHK(wiTest_ClearCounters());
            return WI_SUCCESS;
        }
        //-------------------------------------------------------------------------------
        if COMMAND( "wiTest_SetTxData" )
        {
            wiUWORD *TxData;
            wiInt NumBytes;

            wiMexUtil_CheckParameters(nlhs,nrhs,0,2);

            TxData = wiMexUtil_GetUWORDArray(prhs[1], &NumBytes);
            MCHK (wiTest_SetTxData(NumBytes, TxData) );

            mxFree(TxData);
            return WI_SUCCESS;
        }
        //-------------------------------------------------------------------------------
        if COMMAND( "wiTest_TxPacket" )
        {
            wiTest_State_t *pTest = wiTest_State();

			if (nlhs == 0 && nrhs == 5) // original implementation (TX without channel model)
			{
                wiComplex **S;
                wiReal Prms, Fs, bps;
                wiInt  N, nBytes, FrameBodyType, Broadcast;

                bps           = wiMexUtil_GetRealValue(prhs[1]);
				nBytes        = wiMexUtil_GetIntValue (prhs[2]);
				FrameBodyType = wiMexUtil_GetIntValue (prhs[3]);
				Broadcast     = wiMexUtil_GetIntValue (prhs[4]);
		        MCHK( wiMAC_DataFrame(nBytes, pTest->TxData[0], FrameBodyType, Broadcast) );
		        MCHK( wiPHY_TX(bps, nBytes, pTest->TxData[0], &N, &S, &Fs, &Prms) );
			}
			else if (nlhs <= 2 && nrhs <= 3) // new version: closer to wiTest_TxPacket 'C' usage
			{
                wiComplex **R; 
                wiReal SNR, Fs;
                wiInt NumRx, Nr, Mode = 0;

                if (nrhs == 2) Mode = wiMexUtil_GetIntValue(prhs[1]);

				if (nrhs == 3) SNR = wiMexUtil_GetRealValue(prhs[2]);
				else           SNR = Mode ? pTest->MinPrdBm : pTest->MinSNR;

				XSTATUS( wiTest_TxPacket( Mode, SNR, pTest->DataRate, pTest->Length, 
					                      pTest->DataType, &Nr, &R, &Fs ) );

				if (nlhs > 0) // return the channel output vector(s)
				{
					for (NumRx=0; NumRx<WICHANL_RXMAX && R[NumRx]; NumRx++) ; // count # of output vectors
					wiMexUtil_PutComplexMatrix(NumRx, Nr, R, &plhs[0]);
				}
				if (nlhs > 1) // return the sample rate
					wiMexUtil_PutRealValue(Fs, &plhs[1]);
			}
			else // error trap
			{
		        mexErrMsgTxt("This function should be called with either 0 output and 5 inputs or 0-1 outputs and 0-3 inputs.\n");
		    }
            return WI_SUCCESS;
        }
        //-------------------------------------------------------------------------------
        if COMMAND( "wiTest_TxRxPacket" )
        {
            wiReal Pr_dBm, bps;
            wiInt  nBytes;

            wiMexUtil_CheckParameters(nlhs,nrhs,0,4);
            Pr_dBm = wiMexUtil_GetRealValue(prhs[1]);
            bps    = wiMexUtil_GetRealValue(prhs[2]);
            nBytes = wiMexUtil_GetIntValue(prhs[3]);
            MCHK(wiTest_TxRxPacket(1, Pr_dBm, bps, nBytes));
            return WI_SUCCESS;
        }
        //-------------------------------------------------------------------------------
        if COMMAND( "wiTest_GetState" )
        {
            const char *FieldNames[] = {
                "RndSeed","RndStep0","RndStep1","DataType","Length","DataRate","MinSNR","MaxSNR","StepSNR",
                "MinPrdBm","MaxPrdBm","StepPrdBm","LastPrdBm","MinFail","MinPER","MaxPackets","UpdateRate",
				"TXPWRLVL","Comment","tx_data","BitCount","BitErrorCount","PacketCount",
				"PacketErrorCount","MinPacketCount","TotalPacketCount","TotalPacketErrorCount",
                "BER","PER","MultipleTxPackets","CountByPoint","ClearTotalsOnStartTest",
                "TestIsDone","TestCount"
            };
            const char *CountByPointFieldNames[] = {
                "ParamValue","BitCount","BitErrorCount","PacketCount","PacketErrorCount","BER","PER"
            };

            int n;
            const mwSize dims[2] = {1,1};
            int NumFields  = sizeof(FieldNames)/sizeof(*FieldNames);
            int NumFields2 = sizeof(CountByPointFieldNames)/sizeof(*CountByPointFieldNames);
            mxArray *CountByPoint;
            mxArray *pArray;

            wiTest_State_t *pState = wiTest_State();
            wiMexUtil_CheckParameters(nlhs,nrhs,1,1);

            plhs[0]      = mxCreateStructArray(2, dims, NumFields,  FieldNames);
    		CountByPoint = mxCreateStructArray(2, dims, NumFields2, CountByPointFieldNames);
            if (!plhs[0] || !CountByPoint) mexErrMsgTxt("Memory Allocation Error");
            
            mxSetField(plhs[0],0,"RndSeed",    mxCreateDoubleScalar(pState->RndSeed));
            mxSetField(plhs[0],0,"RndStep0",   mxCreateDoubleScalar(pState->RndStep[0]));
            mxSetField(plhs[0],0,"RndStep1",   mxCreateDoubleScalar(pState->RndStep[1]));
            mxSetField(plhs[0],0,"DataType",   mxCreateDoubleScalar(pState->DataType));
            mxSetField(plhs[0],0,"Length",     mxCreateDoubleScalar(pState->Length));
            mxSetField(plhs[0],0,"DataRate",   mxCreateDoubleScalar(pState->DataRate));
            mxSetField(plhs[0],0,"MinSNR",     mxCreateDoubleScalar(pState->MinSNR));
            mxSetField(plhs[0],0,"MaxSNR",     mxCreateDoubleScalar(pState->MaxSNR));
            mxSetField(plhs[0],0,"StepSNR",    mxCreateDoubleScalar(pState->StepSNR));
            mxSetField(plhs[0],0,"minPr_dBm",  mxCreateDoubleScalar(pState->MinPrdBm));
            mxSetField(plhs[0],0,"maxPr_dBm",  mxCreateDoubleScalar(pState->MaxPrdBm));
            mxSetField(plhs[0],0,"stepPr_dBm", mxCreateDoubleScalar(pState->StepPrdBm));
            mxSetField(plhs[0],0,"LastPrdBm",  mxCreateDoubleScalar(pState->LastPrdBm));
            mxSetField(plhs[0],0,"MinFail",    mxCreateDoubleScalar(pState->MinFail));
            mxSetField(plhs[0],0,"MinPER",     mxCreateDoubleScalar(pState->MinPER));
            mxSetField(plhs[0],0,"MaxPackets", mxCreateDoubleScalar(pState->MaxPackets));
            mxSetField(plhs[0],0,"UpdateRate", mxCreateDoubleScalar(pState->UpdateRate));
            mxSetField(plhs[0],0,"BitCount",              mxCreateDoubleScalar(pState->BitCount));
            mxSetField(plhs[0],0,"BitErrorCount",         mxCreateDoubleScalar(pState->BitErrorCount));
            mxSetField(plhs[0],0,"PacketCount",           mxCreateDoubleScalar(pState->PacketCount));
            mxSetField(plhs[0],0,"PacketErrorCount",      mxCreateDoubleScalar(pState->PacketErrorCount));
            mxSetField(plhs[0],0,"TotalPacketCount",      mxCreateDoubleScalar(pState->TotalPacketCount));
            mxSetField(plhs[0],0,"TotalPacketErrorCount", mxCreateDoubleScalar(pState->TotalPacketErrorCount));
            mxSetField(plhs[0],0,"MinPacketCount",        mxCreateDoubleScalar(pState->MinPacketCount));
            mxSetField(plhs[0],0,"BER",                   mxCreateDoubleScalar(pState->BER));
            mxSetField(plhs[0],0,"PER",                   mxCreateDoubleScalar(pState->PER));
            mxSetField(plhs[0],0,"MultipleTxPackets",     mxCreateDoubleScalar(pState->MultipleTxPackets));
            mxSetField(plhs[0],0,"ClearTotalsOnStartTest",mxCreateDoubleScalar(pState->ClearTotalsOnStartTest));
            mxSetField(plhs[0],0,"TxData",wiMexUtil_CreateUWORDArrayAsDouble(WITEST_MAX_LENGTH, pState->TxData[0], NULL));
            mxSetField(plhs[0],0,"TestIsDone",            mxCreateDoubleScalar(pState->TestIsDone));
            mxSetField(plhs[0],0,"TestCount",             mxCreateDoubleScalar(pState->TestCount));

            n = pState->CountByPoint.NumberOfPoints;
            mxSetField(plhs[0],0,"CountByPoint", CountByPoint);

            wiMexUtil_PutRealArray(n, pState->CountByPoint.ParamValue,       &pArray);  mxSetField(CountByPoint,0, "ParamValue",       pArray);
            wiMexUtil_PutUIntArray(n, pState->CountByPoint.BitCount,         &pArray);  mxSetField(CountByPoint,0, "BitCount",         pArray);
            wiMexUtil_PutUIntArray(n, pState->CountByPoint.BitErrorCount,    &pArray);  mxSetField(CountByPoint,0, "BitErrorCount",    pArray);
            wiMexUtil_PutUIntArray(n, pState->CountByPoint.PacketCount,      &pArray);  mxSetField(CountByPoint,0, "PacketCount",      pArray);
            wiMexUtil_PutUIntArray(n, pState->CountByPoint.PacketErrorCount, &pArray);  mxSetField(CountByPoint,0, "PacketErrorCount", pArray);
            wiMexUtil_PutRealArray(n, pState->CountByPoint.BER,              &pArray);  mxSetField(CountByPoint,0, "BER",              pArray);
            wiMexUtil_PutRealArray(n, pState->CountByPoint.PER,              &pArray);  mxSetField(CountByPoint,0, "PER",              pArray);

            return WI_SUCCESS;
        }
        //-------------------------------------------------------------------------------
        if COMMAND( "wiTest_GetParameter" )
        {
            char *ParamName;
            wiTest_State_t *pState = wiTest_State();
            wiMexUtil_CheckParameters(nlhs,nrhs,1,2);
            ParamName = wiMexUtil_GetStringValue(prhs[1],0);

                 if MATCH(ParamName,"DataType"    ) wiMexUtil_PutIntValue ( pState->DataType,  &plhs[0] );
            else if MATCH(ParamName,"RndSeed"     ) wiMexUtil_PutIntValue ( pState->RndSeed,   &plhs[0] );
            else if MATCH(ParamName,"RndInitState") wiMexUtil_PutUIntArray( 3, pState->RndInitState, &plhs[0]);
            else if MATCH(ParamName,"DataType"    ) wiMexUtil_PutIntValue ( pState->DataType,  &plhs[0] );
            else if MATCH(ParamName,"DataRate"    ) wiMexUtil_PutRealValue( pState->DataRate,  &plhs[0] );
            else if MATCH(ParamName,"Length"      ) wiMexUtil_PutIntValue ( pState->Length,    &plhs[0] );
            else if MATCH(ParamName,"MinFail"     ) wiMexUtil_PutIntValue ( pState->MinFail,   &plhs[0] );
            else if MATCH(ParamName,"MaxPackets"  ) wiMexUtil_PutIntValue ( pState->MaxPackets,&plhs[0] );
            else if MATCH(ParamName,"MinPER"      ) wiMexUtil_PutRealValue( pState->MinPER,    &plhs[0] );
            else if MATCH(ParamName,"MinSNR"      ) wiMexUtil_PutRealValue( pState->MinSNR,    &plhs[0] );
            else if MATCH(ParamName,"MaxSNR"      ) wiMexUtil_PutRealValue( pState->MaxSNR,    &plhs[0] );
            else if MATCH(ParamName,"StepSNR"     ) wiMexUtil_PutRealValue( pState->StepSNR,   &plhs[0] );
            else if MATCH(ParamName,"MinPrdBm"    ) wiMexUtil_PutRealValue( pState->MinPrdBm,  &plhs[0] );
            else if MATCH(ParamName,"MaxPrdBm"    ) wiMexUtil_PutRealValue( pState->MaxPrdBm,  &plhs[0] );
            else if MATCH(ParamName,"StepPrdBm"   ) wiMexUtil_PutRealValue( pState->StepPrdBm, &plhs[0] );
            else if MATCH(ParamName,"LastPrdBm"   ) wiMexUtil_PutRealValue( pState->LastPrdBm, &plhs[0] );
            else if MATCH(ParamName,"TestCount"   ) wiMexUtil_PutUIntValue( pState->TestCount, &plhs[0] );
            else if MATCH(ParamName,"TestIsDone"  ) wiMexUtil_PutUIntValue( pState->TestIsDone,&plhs[0] );
            else if MATCH(ParamName,"BER"         ) wiMexUtil_PutRealValue( pState->BER,       &plhs[0] );
            else if MATCH(ParamName,"PER"         ) wiMexUtil_PutRealValue( pState->PER,       &plhs[0] );
            else if MATCH(ParamName,"BitCount"        )      wiMexUtil_PutUIntValue( pState->BitCount,              &plhs[0] );
            else if MATCH(ParamName,"BitErrorCount"   )      wiMexUtil_PutUIntValue( pState->BitErrorCount,         &plhs[0] );
            else if MATCH(ParamName,"PacketCount"     )      wiMexUtil_PutUIntValue( pState->PacketCount,           &plhs[0] );
            else if MATCH(ParamName,"PacketErrorCount")      wiMexUtil_PutUIntValue( pState->PacketErrorCount,      &plhs[0] );
            else if MATCH(ParamName,"TotalPacketCount")      wiMexUtil_PutUIntValue( pState->TotalPacketCount,      &plhs[0] );
            else if MATCH(ParamName,"TotalPacketErrorCount") wiMexUtil_PutUIntValue( pState->TotalPacketErrorCount, &plhs[0] );
            else if MATCH(ParamName,"TxData") wiMexUtil_PutUIntArray( pState->Length, (unsigned int *)pState->TxData[0], &plhs[0] );
            else mexErrMsgTxt("wiTest_GetParameter: parameter not available");

            mxFree(ParamName);
            return WI_SUCCESS;
        }
        //-------------------------------------------------------------------------------
        if COMMAND( "wiTest_GetStatistics" )
        {
            const char *field_names[] = {
                "PacketCount","PacketErrorCount","TotalPacketCount","TotalPacketErrorCount",
                "BitCount","BitErrorCount","BER","PER" };
            const mwSize dims[2] = {1,1};
            int nfields = (sizeof(field_names)/sizeof(*field_names));
            
            wiTest_State_t *pState = wiTest_State();

            wiMexUtil_CheckParameters(nlhs,nrhs,1,1);

            plhs[0] = mxCreateStructArray(2, dims, nfields, field_names);
            if (!plhs[0]) mexErrMsgTxt("Memory Allocation Error");

            mxSetField(plhs[0],0,"BitCount",              mxCreateDoubleScalar(pState->BitCount));
            mxSetField(plhs[0],0,"BitErrorCount",         mxCreateDoubleScalar(pState->BitErrorCount));
            mxSetField(plhs[0],0,"PacketCount",           mxCreateDoubleScalar(pState->PacketCount));
            mxSetField(plhs[0],0,"PacketErrorCount",      mxCreateDoubleScalar(pState->PacketErrorCount));
            mxSetField(plhs[0],0,"TotalPacketCount",      mxCreateDoubleScalar(pState->TotalPacketCount));
            mxSetField(plhs[0],0,"TotalPacketErrorCount", mxCreateDoubleScalar(pState->TotalPacketErrorCount));
            mxSetField(plhs[0],0,"BER",                   mxCreateDoubleScalar(pState->BER));
            mxSetField(plhs[0],0,"PER",                   mxCreateDoubleScalar(pState->PER));
            return WI_SUCCESS;
        }

        // *** NOTE ABOUT MATLAB CALLBACKS
        // *** Do not retrieve structured data types from a Matlab callback as calling back into
        // *** the DLL prevents MATLAB from cleaning up the memory such that a leak occurs.

        //-------------------------------------------------------------------------------
        if COMMAND( "wiTest_SetTxRxCallback" )
        {
            char *Name, *pBuf;

            wiMexUtil_CheckParameters(nlhs,nrhs,0,2);
            Name = wiMexUtil_GetStringValue(prhs[1], NULL);
            pBuf = Name;

            mexPrintf("Name = %s\n",Name);
            if (strlen(Name) == 0)
            {
                *TxRxMatlabFunction = '\0';
                wiTest_SetTxRxCallback(NULL);
            }
            else
            {
                if (wiseMex_AllowSubstitution())
                {
                    MCHK( wiParse_SubstituteStrings(&Name) );
                }
            }
            if (strlen(Name) < 128)
            {
                strcpy(TxRxMatlabFunction, Name);
                wiTest_SetTxRxCallback(wiseMex_wiTest_TxRxCallback);
            }
            else 
            {
                wiFree(pBuf);
                return WI_ERROR;
            }
            wiFree(pBuf);
            return WI_SUCCESS;
        }
        //-------------------------------------------------------------------------------
        if COMMAND( "wiTest_SetOnePacketCallback" )
        {
            char *Name;

            wiMexUtil_CheckParameters(nlhs,nrhs,0,2);
            Name = wiMexUtil_GetStringValue(prhs[1], NULL);

            if (strlen(Name) == 0)
            {
                *OnePacketMatlabFunction = '\0';
                wiTest_SetOnePacketCallback(NULL);
            }
            if (strlen(Name) < 128)
            {
                strcpy(OnePacketMatlabFunction, Name);
                wiTest_SetOnePacketCallback(wiseMex_wiTest_OnePacketCallback);
            }
            else 
            {
                wiFree(Name);
                return WI_ERROR;
            }
            wiFree(Name);
            return WI_SUCCESS;
        }
        //-------------------------------------------------------------------------------
        if COMMAND( "wiTest_SetScriptCallback" )
        {
            char *Name;

            wiMexUtil_CheckParameters(nlhs,nrhs,0,2);
            Name = wiMexUtil_GetStringValue(prhs[1], NULL);

            if (strlen(Name) == 0)
            {
                *ScriptMatlabFunction = '\0';
                wiTest_SetScriptCallback(NULL);
            }
            if (strlen(Name) < 128)
            {
                strcpy(ScriptMatlabFunction, Name);
                wiTest_SetScriptCallback(wiseMex_wiTest_ScriptCallback);
            }
            else 
            {
                wiFree(Name);
                return WI_ERROR;
            }
            wiFree(Name);
            return WI_SUCCESS;
        }
        //-------------------------------------------------------------------------------
        if COMMAND( "wiTest_ClearTxRxCallback" )
        {
            *TxRxMatlabFunction = '\0';
            wiTest_SetTxRxCallback(NULL);
            return WI_SUCCESS;
        }
        if COMMAND( "wiTest_ClearTestIsDone" )
        {
            wiMexUtil_CheckParameters(nlhs,nrhs,0,1);
            wiTest_ClearTestIsDone();
            return WI_SUCCESS;
        }
        if COMMAND( "wiTest_TestIsDone" )
        {
            wiMexUtil_CheckParameters(nlhs,nrhs,1,1);
            wiMexUtil_PutIntValue( wiTest_TestIsDone(), &plhs[0] );
            return WI_SUCCESS;
        }
    }

	return WI_WARN_PARSE_FAILED;   
}
// end of wiseMex_wiTest()

// ================================================================================================
// FUNCTION : wiseMex_wiTest_ParseLine()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiseMex_wiTest_ParseLine(wiString text)
{
    char buf[256], *cptr = buf; wiStatus status;

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Assign MATLAB Callback Functions
    //
    //  Allow the script to specify a MATLAB function to be assigned via the callbacks
    //  supported in wiseMex_wiTest().
    //
    if ((status = WICHK(wiParse_Function(text,"wiTest_SetTxRxCallback(%255s)",buf))) == WI_SUCCESS)
    {
        if (strlen(buf) == 0)
        {
            *TxRxMatlabFunction = '\0';
            wiTest_SetTxRxCallback(NULL);
        }
        else
        {
            cptr = buf;
            if (wiseMex_AllowSubstitution())
            {
                MCHK( wiParse_SubstituteStrings(&cptr) );
            }
        }
        if (strlen(cptr) < 128)
        {
            strcpy(TxRxMatlabFunction, cptr);
            wiTest_SetTxRxCallback(wiseMex_wiTest_TxRxCallback);
        }
        else 
        {
            return WICHK(WI_ERROR);
        }
        return WI_SUCCESS;
    }
    if ((status = WICHK(wiParse_Function(text,"wiTest_SetOnePacketCallback(%255s)",buf))) == WI_SUCCESS)
    {
        if (strlen(buf) == 0)
        {
            *OnePacketMatlabFunction = '\0';
            wiTest_SetOnePacketCallback(NULL);
        }
        if (strlen(buf) < 128)
        {
            strcpy(OnePacketMatlabFunction, buf);
            wiTest_SetOnePacketCallback(wiseMex_wiTest_OnePacketCallback);
        }
        else 
        {
            return WICHK(WI_ERROR);
        }
        return WI_SUCCESS;
    }
    if ((status = WICHK(wiParse_Function(text,"wiTest_SetOnePacketCallback(%255s)",buf))) == WI_SUCCESS)
    {
        if (strlen(buf) == 0)
        {
            *ScriptMatlabFunction = '\0';
            wiTest_SetScriptCallback(NULL);
        }
        if (strlen(buf) < 128)
        {
            strcpy(ScriptMatlabFunction, buf);
            wiTest_SetScriptCallback(wiseMex_wiTest_ScriptCallback);
        }
        else 
        {
            return WICHK(WI_ERROR);
        }
        return WI_SUCCESS;
    }

    return WI_WARN_PARSE_FAILED;
}
// end of wiseMex_wiTest_ParseLine()

// ================================================================================================
// FUNCTION  : wiseMex_wiTest_TxRxCallback()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiseMex_wiTest_TxRxCallback(void)
{
    if (TxRxMatlabFunction[0] != '\0')
    {
        mxArray *X;
        int ReturnStatus;

        mexCallMATLAB(1, &X, 0, NULL, TxRxMatlabFunction);
       
        ReturnStatus = wiMexUtil_GetIntValue(X);
        mxDestroyArray(X);

        if (ReturnStatus != 0)
            wiUtil_WriteLogFile("mexCallMATLAB(\"%s\") returned %d\n",TxRxMatlabFunction,ReturnStatus);
        if (ReturnStatus <  0)
            return WI_ERROR_RETURNED;
    }
    return WI_SUCCESS;
}
// end of wiseMex_wiTest_TxRxCallback()

// ================================================================================================
// FUNCTION  : wiseMex_wiTest_OnePacketCallback()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiseMex_wiTest_OnePacketCallback(void)
{
    if (OnePacketMatlabFunction[0] != '\0')
    {
        mxArray *X;
        int ReturnStatus;

        mexCallMATLAB(1, &X, 0, NULL, OnePacketMatlabFunction);
       
        ReturnStatus = wiMexUtil_GetIntValue(X);
        mxDestroyArray(X);

        if (ReturnStatus != 0)
            wiUtil_WriteLogFile("mexCallMATLAB(\"%s\") returned %d\n",OnePacketMatlabFunction,ReturnStatus);
        if (ReturnStatus <  0)
            return WI_ERROR_RETURNED;
    }
    return WI_SUCCESS;
}
// end of wiseMex_wiTest_OnePacketCallback()

// ================================================================================================
// FUNCTION  : wiseMex_wiTest_ScriptCallback()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiseMex_wiTest_ScriptCallback(void)
{
    if (ScriptMatlabFunction[0] != '\0')
    {
        mxArray *X;
        int ReturnStatus;

        mexCallMATLAB(1, &X, 0, NULL, ScriptMatlabFunction);
       
        ReturnStatus = wiMexUtil_GetIntValue(X);
        mxDestroyArray(X);

        if (ReturnStatus != 0)
            wiUtil_WriteLogFile("mexCallMATLAB(\"%s\") returned %d\n",ScriptMatlabFunction,ReturnStatus);
        if (ReturnStatus <  0)
            return WI_ERROR_RETURNED;
    }
    return WI_SUCCESS;
}
// end of wiseMex_wiTest_ScriptCallback()

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------

