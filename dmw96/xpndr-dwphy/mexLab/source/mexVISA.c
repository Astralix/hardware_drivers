//--< mexVISA.c >----------------------------------------------------------------------------------
//=================================================================================================
//
//  MATLAB Interface to the VXI PnP Virtual Instrument Software Architecture (VISA)
//  Copyright 2003 Bermai Inc, 2007 DSP Group, Inc.
//
//  Developed by Barrett Brickner
//
//=================================================================================================

#include "ViMexUtil.h"
#include "mexVISA.h"
#include "visa.h"

// ================================================================================================
// --  LOCAL FILE TYPES
// ================================================================================================
typedef struct MEXVISA_HANDLE_S
{
    ViSession Session;
    ViChar    Resource[20];
    ViChar    Source[512];

    struct MEXVISA_HANDLE_S *Next;
    struct MEXVISA_HANDLE_S *Prev;

} MexVisaSessionList_t;

// ================================================================================================
// --  LOCAL FILE PARAMETERS
// ================================================================================================
static ViBoolean mexVISA_IsInitialized = VI_FALSE; // Has everything been initialized?
static ViBoolean ResourceChecking = VI_TRUE; // enable checking of viOpen/viClose
static ViBoolean AllowSharedSessions = VI_FALSE; // return session handle if resource is already opened
static ViSession _rmSession = 0;
static ViStatus  ViChkStatus;  // used with VICHK() macro
static MexVisaSessionList_t *pSessionList = NULL;

// ================================================================================================
// --  INTERNAL ERROR HANDLING
// ================================================================================================
#define VICHK(fn) { if( (ViChkStatus = (fn) ) < 0 )                                               \
                        mexPrintf("VICHK(%s #%d): Status=0x%08X\n",__FILE__,__LINE__,ViChkStatus);\
                  }
#define MXERR(fn) if((fn)<0) mexPrintf("MXERR(%s,#%d)\n",__FILE__,__LINE__);

#define MCHK(function)  mexVISA_ErrorHandler(WICHK(function))
#define DEBUG           mexPrintf("### FILE: %s, LINE: %d\n",__FILE__,__LINE__);

// ================================================================================================
// --  INTERNAL FUNCTION PROTOTYPES
// ================================================================================================
static void     __cdecl    mexVISA_Init(void);
static void     __cdecl    mexVISA_Close(void);
static ViStatus __fastcall mexVISA_ErrorHandler(ViStatus Status);
static int  mexVISA_VISA_ErrorHandler(int Status);
static int  mexVISA_Get_VI_EVENT(char *String, ViEventType *pValue);
static int  mexVISA_Get_VI_ATTR (char *String, ViUInt32 *pValue, int *pType);
void mexVISA_InsertSession(ViSession Session, const ViString Resource, const ViString Source);
void mexVISA_RemoveSession(ViSession Session);
void mexVISA_DisplaySessionList(void);
ViBoolean mexVISA_ResourceIsOpen(ViString Resource, ViSession *pSession);

// ================================================================================================
// FUNCTION  : LeCroyInspectReal
// ------------------------------------------------------------------------------------------------
// Purpose   : Query a parameter value
// Parameters: Session -- VISA instrument session handle
//             trace   -- {"TA".."TD","M1".."M4","C1".."C4"}
//             param   -- parameter name to be insepected
//             pval    -- return value (by reference)
// ================================================================================================
static int LeCroyInspectReal(ViSession Session, char *trace, char *param, double *pval)
{
    unsigned char buf[256], *cptr;
    
    VICHK( viPrintf(Session, "%s:INSP? '%s'\n",trace, param) );
    VICHK( viRead  (Session, buf, 256, NULL) );
    
    if((cptr=strstr(buf, param)) == NULL) return -1; cptr += strlen(param);
    if((cptr=strchr(cptr, ':' )) == NULL) return -1; cptr++;
    while(*cptr==' ') cptr++;
    *pval = strtod(cptr, NULL);

    return 0;
}
// end of LeCroyInspectReal()

// ================================================================================================
// FUNCTION  : LeCroyInspectInt
// ------------------------------------------------------------------------------------------------
// Purpose   : Query a parameter value
// Parameters: Session -- VISA instrument session handle
//             trace   -- {"TA".."TD","M1".."M4","C1".."C4"}
//             param   -- parameter name to be insepected
//             pval    -- return value (by reference)
// ================================================================================================
static int LeCroyInspectInt(ViSession Session, char *trace, char *param, int *pval)
{
    char buf[256], *cptr;
    
    VICHK( viPrintf(Session, "%s:INSP? '%s'\n",trace, param) );
    VICHK( viRead  (Session, buf, 256, NULL) );
    
    if((cptr=strstr(buf, param)) == NULL) return -1; cptr += strlen(param);
    if((cptr=strchr(cptr, ':' )) == NULL) return -1; cptr++;
    while(*cptr==' ') cptr++; // skip whitespace
    *pval = strtol(cptr, NULL, 0);

    return 0;
}
// end of LeCroyInspectInt()

// ================================================================================================
// FUNCTION  : mexVISA_GetViRM
// ------------------------------------------------------------------------------------------------
// ================================================================================================
ViStatus mexVISA_GetViRM(ViSession *rmSession)
{
    if(_rmSession) 
    {
        *rmSession = _rmSession;
        return VI_SUCCESS;
    }
    else 
    {
        ViStatus status = viOpenDefaultRM(rmSession);
        _rmSession = *rmSession;
        return status;
    }
}
// end of mexVISA_GetViRM()

// ================================================================================================
// FUNCTION  : mexVISA
// ------------------------------------------------------------------------------------------------
// Purpose   : Implement basic VISA functions for a MATLAB toolbox
// Parameters: passed from MATLAB mexFunction
// Returns   : {0=success, -1=failure}
// ================================================================================================
int mexVISA(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    ViStatus Status;      // returned status from called VISA function
    char *tcmd, cmd[256]; // passed command parameter

    // ----------------------
    // Get the command string
    // ----------------------
    if(nrhs<1) mexErrMsgTxt("A string argument must be given as the first parameter.");
    tcmd = ViMexUtil_GetStringValue(prhs[0],0);
    strncpy(cmd, tcmd, 256); mxFree(tcmd); cmd[255] = 0; // copied and free tcmd as it is not free'd below

    // ******************************************
    // *****                                *****
    // *****   COMMAND LOOKUP-AND-EXECUTE   *****
    // *****                                *****
    // ******************************************

    // --------------
    // viOpen
    // --------------
    if(!strcmp(cmd, "viOpen()"))
    {
        ViSession RMSession, Session = 0;
        ViString  Resource; ViUInt32 Timeout;

        if( (nrhs!=3) && (nrhs!=4) )
            mexErrMsgTxt("Incorrect number of input parameters (needs to be 3 or 4)");
        if( nlhs != 2)
            mexErrMsgTxt("2 output terms were expected\n");

        if(!mexVISA_GetViRM(&RMSession))
        {
            Resource = ViMexUtil_GetStringValue(prhs[1], 0);
            Timeout  = (ViUInt32)ViMexUtil_GetUIntValue(prhs[2]);
            
            if(ResourceChecking)
            {
                if(mexVISA_ResourceIsOpen(Resource, &Session))
                {
                    if(AllowSharedSessions) 
                        Status = VI_SUCCESS;
                    else
                        mexErrMsgTxt(" Resource is already open");
                }
            }
            if(!Session)
            {
                Status = viOpen(RMSession, Resource, VI_NULL, Timeout, &Session);
                if(ResourceChecking && (Status == VI_SUCCESS))
                {
                    if(nrhs == 3)
                        mexVISA_InsertSession(Session, Resource, "no stack trace available\n");
                    else
                    {

                        ViString StackTrace = ViMexUtil_GetStringValue(prhs[3], 0);
                        mexVISA_InsertSession(Session, Resource, StackTrace);
                        mxFree(StackTrace);
                    }
                }
            }
            ViMexUtil_PutIntValue(Status, &plhs[0]);
            ViMexUtil_PutUIntValue((unsigned)Session, &plhs[1]);
            mxFree(Resource);
            return 0;
        }
        else mexErrMsgTxt("mexVISA: Call to viOpenDefaultRM prior to viOpen failed.");
    }
    // ---------------
    // viClose
    // ---------------
    if(!strcmp(cmd, "viClose()"))
    {
        ViSession Session;
        ViMexUtil_CheckParameters(nlhs,nrhs,1,2);
        Session = (ViSession)ViMexUtil_GetUIntValue(prhs[1]);
        Status = viClose(Session);
        if(ResourceChecking && (Status == VI_SUCCESS))
            mexVISA_RemoveSession(Session);
        ViMexUtil_PutIntValue((int)Status, &plhs[0]);
        return 0;
    }
    // ---------------
    // viWrite
    // ---------------
    if(!strcmp(cmd, "viWrite()"))
    {
        ViSession Session;
        ViUInt32 Count, Return_Count;
        ViBuf Buffer; char *Message;

        ViMexUtil_CheckParameters(nlhs,nrhs,2,3);
        Session = (ViSession)ViMexUtil_GetUIntValue(prhs[1]);
        Buffer  = (ViBuf)ViMexUtil_GetStringValue(prhs[2], ((ViUInt32 *)&Count));
        Message = (char *)mxCalloc(Count+2, sizeof(char));
        if(!Message) mexErrMsgTxt("Memory allocation error.");
        sprintf(Message,"%s",Buffer);
        Status = viWrite(Session, Message, Count-1, &Return_Count);
        ViMexUtil_PutIntValue((int)Status, &plhs[0]);
        ViMexUtil_PutUIntValue(Return_Count, &plhs[1]);
        mxFree(Message); mxFree((char *)Buffer);
        return 0;
    }
    // ---------------
    // viWriteBinary
    // ---------------
    if(!strcmp(cmd, "viWriteBinary()"))
    {
        ViSession Session;
        ViUInt32 Count, Return_Count;
        ViBuf Buffer;

        ViMexUtil_CheckParameters(nlhs,nrhs,2,3);
        Session = (ViSession)ViMexUtil_GetUIntValue(prhs[1]);
        Buffer  = (ViBuf)ViMexUtil_GetBinaryArray(prhs[2], ((ViUInt32 *)&Count));
        Status = viWrite(Session, Buffer, Count, &Return_Count);
        ViMexUtil_PutIntValue((int)Status, &plhs[0]);
        ViMexUtil_PutUIntValue(Return_Count, &plhs[1]);
        mxFree((char *)Buffer);
        return 0;
    }
    // ---------------
    // viPrintf
    // ---------------
    if(!strcmp(cmd, "viPrintf()"))
    {
        ViSession Session;
        ViUInt32 Count;
        ViBuf Buffer;

        ViMexUtil_CheckParameters(nlhs,nrhs,1,3);
        Session = (ViSession)ViMexUtil_GetUIntValue(prhs[1]);
        Buffer  = (ViBuf)ViMexUtil_GetStringValue(prhs[2], ((ViUInt32 *)&Count));
        Status = viPrintf(Session,"%s",Buffer);
        ViMexUtil_PutIntValue((int)Status, &plhs[0]);
        mxFree((char *)Buffer);
        return 0;
    }
    // --------------
    // viRead
    // --------------
    if(!strcmp(cmd, "viRead()"))
    {
        ViSession Session;
        ViUInt32 Count, Return_Count;
        unsigned char *Buffer;

        ViMexUtil_CheckParameters(nlhs,nrhs,2,3);
        Session = (ViSession)ViMexUtil_GetUIntValue(prhs[1]);
        Count   = (ViUInt32)ViMexUtil_GetUIntValue(prhs[2]);
        Buffer  = (unsigned char *)mxCalloc(Count+1,sizeof(char));
        if(!Buffer) mexErrMsgTxt("Memory Allocation Failed.\n");
        Status = viRead(Session, Buffer, Count, &Return_Count);
        if(Return_Count < Count) Buffer[Return_Count-1] = '\0';
        ViMexUtil_PutIntValue((int)Status, &plhs[0]);
        ViMexUtil_PutString(Buffer, &plhs[1]);
        mxFree(Buffer); // removed 6/4/04 for debug
        return 0;
    }
    // --------------
    // viReadBinary
    // --------------
    if(!strcmp(cmd, "viReadBinary()"))
    {
        ViSession Session;
        ViUInt32 Count, Return_Count;
        unsigned char *Buffer;

        ViMexUtil_CheckParameters(nlhs,nrhs,2,3);
        Session = (ViSession)ViMexUtil_GetUIntValue(prhs[1]);
        Count   = (ViUInt32)ViMexUtil_GetUIntValue(prhs[2]);
        Buffer  = (unsigned char *)mxCalloc(Count+1,sizeof(char));
        if(!Buffer) mexErrMsgTxt("Memory Allocation Failed.\n");
        Status = viRead(Session, Buffer, Count, &Return_Count);
        ViMexUtil_PutIntValue((int)Status, &plhs[0]);
        ViMexUtil_PutBinaryArray(Return_Count, Buffer, &plhs[1]);
        mxFree(Buffer);
        return 0;
    }
    // -------
    // viClear
    // -------
    if(!strcmp(cmd, "viClear()"))
    {
        ViSession Session;

        ViMexUtil_CheckParameters(nlhs,nrhs,1,2);
        Session = (ViSession)ViMexUtil_GetUIntValue(prhs[1]);
        Status = viClear(Session);
        ViMexUtil_PutIntValue((int)Status, &plhs[0]);
        return 0;
    }
    // ---------
    // viReadSTB
    // ---------
    if(!strcmp(cmd, "viReadSTB()"))
    {
        ViSession Session;
        ViUInt16 Status_Byte;

        ViMexUtil_CheckParameters(nlhs,nrhs,2,2);
        Session = (ViSession)ViMexUtil_GetUIntValue(prhs[1]);
        Status = viReadSTB(Session, &Status_Byte);
        ViMexUtil_PutIntValue((int)Status, &plhs[0]);
        ViMexUtil_PutUIntValue((unsigned)Status_Byte, &plhs[1]);
        return 0;
    }
    // ---------------
    // viAssertTrigger
    // ---------------
    if(!strcmp(cmd, "viAssertTrigger()"))
    {
        ViSession Session;
        ViUInt16 Protocol;
        char *ProtocolString;

        ViMexUtil_CheckParameters(nlhs,nrhs,1,3);
        Session = (ViSession)ViMexUtil_GetUIntValue(prhs[1]);
        ProtocolString = ViMexUtil_GetStringValue(prhs[2], 0);

             if(!strcmp(ProtocolString, "VI_TRIG_PROT_DEFAULT")) Protocol = VI_TRIG_PROT_DEFAULT;
        else if(!strcmp(ProtocolString, "VI_TRIG_PROT_ON"))      Protocol = VI_TRIG_PROT_ON;
        else if(!strcmp(ProtocolString, "VI_TRIG_PROT_OFF"))     Protocol = VI_TRIG_PROT_OFF;
        else if(!strcmp(ProtocolString, "VI_TRIG_PROT_SYNC"))    Protocol = VI_TRIG_PROT_SYNC;  
        else 
        {
            mxFree(ProtocolString);
            mexErrMsgTxt("Unrecognized protocol definition");
        }
        Status = viAssertTrigger(Session, Protocol);
        ViMexUtil_PutIntValue((int)Status, &plhs[0]);
        mxFree(ProtocolString);
        return 0;
    }
    // ------------
    // viStatusDesc
    // ------------
    if(!strcmp(cmd, "viStatusDesc()"))
    {
        ViSession Session;
        char *Description;

        ViMexUtil_CheckParameters(nlhs,nrhs,2,3);
        Session = (ViSession)ViMexUtil_GetUIntValue(prhs[1]);
        Status  = (ViStatus) ViMexUtil_GetIntValue(prhs[2]);
        Description = (char *)mxCalloc(256,sizeof(char));
        Status = viStatusDesc(Session, Status, Description);
        ViMexUtil_PutIntValue((int)Status, &plhs[0]);
        ViMexUtil_PutString(Description, &plhs[1]);
        //mxFree(Description);
        return 0;
    }
    // --------------
    // viGetAttribute
    // --------------
    if(!strcmp(cmd, "viGetAttribute()"))
    {
        ViSession Session;
        ViAttr Attribute; char *AttrString;
        int Type; double dVal; char buffer[256];

        ViMexUtil_CheckParameters(nlhs,nrhs,2,3);
        Session = (ViSession)ViMexUtil_GetUIntValue(prhs[1]);
        AttrString = ViMexUtil_GetStringValue(prhs[2], 0);

        if(mexVISA_Get_VI_ATTR(AttrString, &Attribute, &Type)) {
            mxFree(AttrString);
            mexErrMsgTxt("Unrecognized/unsupported VISA Attribute type.");
        }
        Status = viGetAttribute(Session, Attribute, ((void *)buffer));
        ViMexUtil_PutIntValue((int)Status, &plhs[0]);

        if(Status >= 0)
        {
            switch(Type)
            {
                case TYPE_ViString    : break; // handle below
                case TYPE_ViStatus    : dVal = (double)(*((ViStatus    *)buffer)); break;
                case TYPE_ViBoolean   : dVal = (double)(*((ViBoolean   *)buffer)); break;
                case TYPE_ViInt8      : dVal = (double)(*((ViInt8      *)buffer)); break;
                case TYPE_ViInt16     : dVal = (double)(*((ViInt16     *)buffer)); break;
                case TYPE_ViInt32     : dVal = (double)(*((ViInt32     *)buffer)); break;
                case TYPE_ViUInt8     : dVal = (double)(*((ViUInt8     *)buffer)); break;
                case TYPE_ViUInt16    : dVal = (double)(*((ViUInt16    *)buffer)); break;
                case TYPE_ViUInt32    : dVal = (double)(*((ViUInt32    *)buffer)); break;
                case TYPE_ViSession   : dVal = (double)(*((ViSession   *)buffer)); break;
                case TYPE_ViBusAddress: dVal = (double)(*((ViBusAddress*)buffer)); break;
                case TYPE_ViBusSize   : dVal = (double)(*((ViBusSize   *)buffer)); break;
                case TYPE_ViVersion   : dVal = (double)(*((ViVersion   *)buffer)); break;
                case TYPE_ViAccessMode: dVal = (double)(*((ViAccessMode*)buffer)); break;
                case TYPE_ViEventType : dVal = (double)(*((ViEventType *)buffer)); break;
                case TYPE_ViJobId     : dVal = (double)(*((ViJobId     *)buffer)); break;
                default: 
                {
                    mxFree(AttrString);
                    mexErrMsgTxt("Unhandled case in mexVISA.c under viGetAttribute.");
                }
            }
            if(Type == TYPE_ViString) {
                ViMexUtil_PutString(buffer, &plhs[1]);
            }
            else
                ViMexUtil_PutRealValue(dVal, &plhs[1]);
        }
        else ViMexUtil_PutRealValue(-1.0, &plhs[1]);
        mxFree(AttrString);
        return 0;
    }
    // --------------
    // viSetAttribute
    // --------------
    if(!strcmp(cmd, "viSetAttribute()"))
    {
        ViSession Session;
        ViAttr Attribute; char *AttrString; 
        double dVal; int Type;

        ViMexUtil_CheckParameters(nlhs,nrhs,1,4);
        Session = (ViSession)ViMexUtil_GetUIntValue(prhs[1]);
        AttrString = ViMexUtil_GetStringValue(prhs[2], 0);

        if(mexVISA_Get_VI_ATTR(AttrString, &Attribute, &Type)) {
            mxFree(AttrString);
            mexErrMsgTxt("Unrecognized/unsupported VISA Attribute type.");
        }
        if(Type != TYPE_ViString) {
            dVal = ViMexUtil_GetRealValue(prhs[3]);
        }
        switch(Type)
        {
            case TYPE_ViInt8:
            case TYPE_ViInt16:
            case TYPE_ViInt32:
            {
                ViAttrState attrState = (ViAttrState)((ViInt32)dVal);
                Status = viSetAttribute(Session, Attribute, attrState);
            }  break;
 
            case TYPE_ViStatus:
            case TYPE_ViBoolean:
            case TYPE_ViUInt8:
            case TYPE_ViUInt16:
            case TYPE_ViUInt32:
            case TYPE_ViSession:
            case TYPE_ViBusAddress:
            case TYPE_ViBusSize:
            case TYPE_ViVersion:
            case TYPE_ViAccessMode:
            case TYPE_ViEventType:
            case TYPE_ViJobId:
            {
                ViAttrState attrState = (ViAttrState)((ViUInt32)dVal);
                Status = viSetAttribute(Session, Attribute, attrState);
            }  break;

            case TYPE_ViString:
            {
                mxFree(AttrString);
                mexErrMsgTxt("Setting string attributes is not allowed.");
            }  break;

            default: 
            {
                mxFree(AttrString);
                mexErrMsgTxt("Unhandled case in mexVISA.c under viGetAttribute.");
            }
        }
        ViMexUtil_PutIntValue((int)Status, &plhs[0]);
        mxFree(AttrString);
        return 0;
    }
    // -------------
    // viEnableEvent
    // -------------
    if(!strcmp(cmd, "viEnableEvent()"))
    {
        ViSession Session;
        ViEventType Event; char *EventString;
        ViUInt16 Mechanism;

        ViMexUtil_CheckParameters(nlhs,nrhs,1,4);
        Session     = (ViSession)ViMexUtil_GetUIntValue(prhs[1]);
        EventString = ViMexUtil_GetStringValue(prhs[2], 0);
        Mechanism   = (ViUInt16)ViMexUtil_GetIntValue(prhs[3]);

        if(Mechanism != 1) {
            mxFree(EventString);
            mexErrMsgTxt("EventMechanism must be VI_QUEUE = 1");
        }
        if(mexVISA_Get_VI_EVENT(EventString, &Event)) {
            mxFree(EventString);
            mexErrMsgTxt("Unrecognized/unsupported VISA Event type.");
        }
        Status = viEnableEvent(Session, Event, Mechanism, VI_NULL);
        ViMexUtil_PutIntValue((int)Status, &plhs[0]);
        mxFree(EventString);
        return 0;
    }
    // ---------------------
    // viDisableEvent
    // ---------------------
    if(!strcmp(cmd, "viDisableEvent()"))
    {
        ViSession Session;
        ViEventType Event; char *EventString;
        ViUInt16 Mechanism;

        ViMexUtil_CheckParameters(nlhs,nrhs,1,4);
        Session     = (ViSession)ViMexUtil_GetUIntValue(prhs[1]);
        EventString = ViMexUtil_GetStringValue(prhs[2], 0);
        Mechanism   = (ViUInt16)ViMexUtil_GetIntValue(prhs[3]);

        if(Mechanism != 1) 
        {
            mxFree(EventString);
            mexErrMsgTxt("EventMechanism must be VI_QUEUE = 1");
        }
        if(mexVISA_Get_VI_EVENT(EventString, &Event)) 
        {
            mxFree(EventString);
            mexErrMsgTxt("Unrecognized/unsupported VISA Event type.");
        }
        Status = viDisableEvent(Session, Event, Mechanism);
        ViMexUtil_PutIntValue((int)Status, &plhs[0]);
        mxFree(EventString);
        return 0;
    }
    // ---------------
    // viDiscardEvents
    // ---------------
    if(!strcmp(cmd, "viDiscardEvents()"))
    {
        ViSession Session;
        ViEventType Event; char *EventString;
        ViUInt16 Mechanism;

        ViMexUtil_CheckParameters(nlhs,nrhs,1,4);
        Session     = (ViSession)ViMexUtil_GetUIntValue(prhs[1]);
        EventString = ViMexUtil_GetStringValue(prhs[2], 0);
        Mechanism   = (ViUInt16)ViMexUtil_GetIntValue(prhs[3]);

        if(Mechanism != 1) 
        {
            mxFree(EventString);
            mexErrMsgTxt("EventMechanism must be VI_QUEUE = 1");
        }
        if(mexVISA_Get_VI_EVENT(EventString, &Event)) 
        {
            mxFree(EventString);
            mexErrMsgTxt("Unrecognized/unsupported VISA Event type.");
        }
        Status = viDiscardEvents(Session, Event, Mechanism);
        ViMexUtil_PutIntValue((int)Status, &plhs[0]);
        mxFree(EventString);
        return 0;
    }
    // -------------
    // viWaitOnEvent
    // -------------
    if(!strcmp(cmd, "viWaitOnEvent()"))
    {
        ViSession Session;
        ViEventType Event, OutEventType; char *EventString;
        ViEvent EventHandle;
        ViUInt32 Timeout;

        ViMexUtil_CheckParameters(nlhs,nrhs,3,4);
        Session     = (ViSession)ViMexUtil_GetUIntValue(prhs[1]);
        EventString = ViMexUtil_GetStringValue(prhs[2], 0);
        Timeout     = (ViUInt32)ViMexUtil_GetUIntValue(prhs[3]);

        if(mexVISA_Get_VI_EVENT(EventString, &Event)) 
        {
            mxFree(EventString);
            mexErrMsgTxt("Unrecognized/unsupported VISA Event type.");
        }
        Status = viWaitOnEvent(Session, Event, Timeout, &OutEventType, &EventHandle);
        ViMexUtil_PutIntValue((int)Status, &plhs[0]);
        ViMexUtil_PutUIntValue((unsigned)OutEventType, &plhs[1]);
        ViMexUtil_PutUIntValue((unsigned)EventHandle, &plhs[2]);
        mxFree(EventString);
        return 0;
    }
    // ******************************************************************
    // -------------------
    // No Command Found...
    // -------------------
    return -1;
}
// end of mexVISA()

// ================================================================================================
// FUNCTION  : mexVISA_Get_VI_EVENT()
// ------------------------------------------------------------------------------------------------
// Purpose   : Retrieve the numerical value specified by a string
// Parameters: String -- String version of the desired value
//             pValue -- ptr->Value returned by reference
// ================================================================================================
static int mexVISA_Get_VI_EVENT(char *String, ViEventType *pValue)
{
    const struct {
        ViEventType Value; ViString String;
    } VI_EVENT[] = {

        {VI_EVENT_IO_COMPLETION, "VI_EVENT_IO_COMPLETION"},
        {VI_EVENT_TRIG,          "VI_EVENT_TRIG"         },
        {VI_EVENT_SERVICE_REQ,   "VI_EVENT_SERVICE_REQ"  },
        {VI_EVENT_EXCEPTION,     "VI_EVENT_EXCEPTION"    },
        {VI_EVENT_VXI_SIGP,      "VI_EVENT_VXI_SIGP"     },
        {VI_EVENT_VXI_VME_INTR,  "VI_EVENT_VXI_VME_INTR" },
        {VI_ALL_ENABLED_EVENTS,  "VI_ALL_ENABLED_EVENTS" },
        {0, NULL}
    };
    int i;

    for(i=0; VI_EVENT[i].String; i++) {
        if(strcmp(VI_EVENT[i].String, String) == 0) {
            *pValue = VI_EVENT[i].Value;
            return 0;
        }
    }
    mexErrMsgTxt("Unrecognized VI_EVENT_* type");
    return -1;
}
// end of mexVISA_Get_VI_EVENT()

// ================================================================================================
// FUNCTION  : mexVISA_Get_VI_ATTR()
// ------------------------------------------------------------------------------------------------
//  Purpose   : Retrieve the numerical value specified by a string
//  Parameters: String -- String version of the desired value
//              pValue -- ptr->Value returned by reference
//              pType  -- ptr->Type returned by reference
// ================================================================================================
static int mexVISA_Get_VI_ATTR(char *String, ViUInt32 *pValue, int *pType)
{
    const struct {
        ViUInt32 Value; char *String; int Type;
    } VI_ATTR[] = {
        {VI_ATTR_RSRC_NAME          ,"VI_ATTR_RSRC_NAME",          TYPE_ViString},
        {VI_ATTR_RSRC_IMPL_VERSION  ,"VI_ATTR_RSRC_IMPL_VERSION",  TYPE_ViVersion},
        {VI_ATTR_RSRC_LOCK_STATE    ,"VI_ATTR_RSRC_LOCK_STATE",    TYPE_ViAccessMode},   
        {VI_ATTR_MAX_QUEUE_LENGTH   ,"VI_ATTR_MAX_QUEUE_LENGTH",   TYPE_ViUInt32},
        {VI_ATTR_FDC_CHNL           ,"VI_ATTR_FDC_CHNL",           TYPE_ViUInt16},
        {VI_ATTR_FDC_MODE           ,"VI_ATTR_FDC_MODE",           TYPE_ViUInt16},
        {VI_ATTR_FDC_GEN_SIGNAL_EN  ,"VI_ATTR_FDC_GEN_SIGNAL_EN",  TYPE_ViBoolean},
        {VI_ATTR_FDC_USE_PAIR       ,"VI_ATTR_FDC_USE_PAIR",       TYPE_ViBoolean},
        {VI_ATTR_SEND_END_EN        ,"VI_ATTR_SEND_END_EN",        TYPE_ViBoolean},
        {VI_ATTR_TERMCHAR           ,"VI_ATTR_TERMCHAR",           TYPE_ViUInt8},
        {VI_ATTR_TMO_VALUE          ,"VI_ATTR_TMO_VALUE",          TYPE_ViUInt32},
        {VI_ATTR_GPIB_READDR_EN     ,"VI_ATTR_GPIB_READDR_EN",     TYPE_ViBoolean},
        {VI_ATTR_IO_PROT            ,"VI_ATTR_IO_PROT",            TYPE_ViUInt16},
        {VI_ATTR_ASRL_BAUD          ,"VI_ATTR_ASRL_BAUD",          TYPE_ViUInt32},
        {VI_ATTR_ASRL_DATA_BITS     ,"VI_ATTR_ASRL_DATA_BITS",     TYPE_ViUInt16},
        {VI_ATTR_ASRL_PARITY        ,"VI_ATTR_ASRL_PARITY",        TYPE_ViUInt16},
        {VI_ATTR_ASRL_STOP_BITS     ,"VI_ATTR_ASRL_STOP_BITS",     TYPE_ViUInt16},
        {VI_ATTR_ASRL_FLOW_CNTRL    ,"VI_ATTR_ASRL_FLOW_CNTRL",    TYPE_ViUInt16},
        {VI_ATTR_RD_BUF_OPER_MODE   ,"VI_ATTR_RD_BUF_OPER_MODE",   TYPE_ViUInt16},
        {VI_ATTR_WR_BUF_OPER_MODE   ,"VI_ATTR_WR_BUF_OPER_MODE",   TYPE_ViUInt16},
        {VI_ATTR_SUPPRESS_END_EN    ,"VI_ATTR_SUPPRESS_END_EN",    TYPE_ViBoolean},
        {VI_ATTR_TERMCHAR_EN        ,"VI_ATTR_TERMCHAR_EN",        TYPE_ViBoolean},
        {VI_ATTR_DEST_ACCESS_PRIV   ,"VI_ATTR_DEST_ACCESS_PRIV",   TYPE_ViUInt16},
        {VI_ATTR_DEST_BYTE_ORDER    ,"VI_ATTR_DEST_BYTE_ORDER",    TYPE_ViUInt16},
        {VI_ATTR_SRC_ACCESS_PRIV    ,"VI_ATTR_SRC_ACCESS_PRIV",    TYPE_ViUInt16},
        {VI_ATTR_SRC_BYTE_ORDER     ,"VI_ATTR_SRC_BYTE_ORDER",     TYPE_ViUInt16},
        {VI_ATTR_SRC_INCREMENT      ,"VI_ATTR_SRC_INCREMENT",      TYPE_ViInt32},
        {VI_ATTR_DEST_INCREMENT     ,"VI_ATTR_DEST_INCREMENT",     TYPE_ViInt32},
        {VI_ATTR_WIN_ACCESS_PRIV    ,"VI_ATTR_WIN_ACCESS_PRIV",    TYPE_ViUInt16},
        {VI_ATTR_WIN_BYTE_ORDER     ,"VI_ATTR_WIN_BYTE_ORDER",     TYPE_ViUInt16},
        {VI_ATTR_CMDR_LA            ,"VI_ATTR_CMDR_LA",            TYPE_ViInt16},
        {VI_ATTR_MAINFRAME_LA       ,"VI_ATTR_MAINFRAME_LA",       TYPE_ViInt16},
        {VI_ATTR_WIN_BASE_ADDR      ,"VI_ATTR_WIN_BASE_ADDR",      TYPE_ViBusAddress},
        {VI_ATTR_WIN_SIZE           ,"VI_ATTR_WIN_SIZE",           TYPE_ViBusSize},
        {VI_ATTR_ASRL_AVAIL_NUM     ,"VI_ATTR_ASRL_AVAIL_NUM",     TYPE_ViUInt32},
        {VI_ATTR_MEM_BASE           ,"VI_ATTR_MEM_BASE",           TYPE_ViBusAddress},
        {VI_ATTR_ASRL_CTS_STATE     ,"VI_ATTR_ASRL_CTS_STATE",     TYPE_ViInt16},
        {VI_ATTR_ASRL_DCD_STATE     ,"VI_ATTR_ASRL_DCD_STATE",     TYPE_ViInt16},
        {VI_ATTR_ASRL_DSR_STATE     ,"VI_ATTR_ASRL_DSR_STATE",     TYPE_ViInt16}, 
        {VI_ATTR_ASRL_DTR_STATE     ,"VI_ATTR_ASRL_DTR_STATE",     TYPE_ViInt16},    
        {VI_ATTR_ASRL_END_IN        ,"VI_ATTR_ASRL_END_IN",        TYPE_ViUInt16}, 
        {VI_ATTR_ASRL_END_OUT       ,"VI_ATTR_ASRL_END_OUT",       TYPE_ViUInt16},
        {VI_ATTR_ASRL_REPLACE_CHAR  ,"VI_ATTR_ASRL_REPLACE_CHAR",  TYPE_ViUInt8},
        {VI_ATTR_ASRL_RI_STATE      ,"VI_ATTR_ASRL_RI_STATE",      TYPE_ViInt16},
        {VI_ATTR_ASRL_RTS_STATE     ,"VI_ATTR_ASRL_RTS_STATE",     TYPE_ViInt16},
        {VI_ATTR_ASRL_XON_CHAR      ,"VI_ATTR_ASRL_XON_CHAR",      TYPE_ViUInt8},
        {VI_ATTR_ASRL_XOFF_CHAR     ,"VI_ATTR_ASRL_XOFF_CHAR",     TYPE_ViUInt8},
        {VI_ATTR_WIN_ACCESS         ,"VI_ATTR_WIN_ACCESS",         TYPE_ViUInt16},
        {VI_ATTR_RM_SESSION         ,"VI_ATTR_RM_SESSION",         TYPE_ViSession},
        {VI_ATTR_VXI_LA             ,"VI_ATTR_VXI_LA",             TYPE_ViInt16},
        {VI_ATTR_MANF_ID            ,"VI_ATTR_MANF_ID",            TYPE_ViUInt16},
        {VI_ATTR_MEM_SIZE           ,"VI_ATTR_MEM_SIZE",           TYPE_ViBusSize},
        {VI_ATTR_MEM_SPACE          ,"VI_ATTR_MEM_SPACE",          TYPE_ViUInt16},
        {VI_ATTR_MODEL_CODE         ,"VI_ATTR_MODEL_CODE",         TYPE_ViUInt16},
        {VI_ATTR_SLOT               ,"VI_ATTR_SLOT",               TYPE_ViInt16},
        {VI_ATTR_INTF_INST_NAME     ,"VI_ATTR_INTF_INST_NAME",     TYPE_ViString},
        {VI_ATTR_IMMEDIATE_SERV     ,"VI_ATTR_IMMEDIATE_SERV",     TYPE_ViBoolean},
        {VI_ATTR_INTF_PARENT_NUM    ,"VI_ATTR_INTF_PARENT_NUM",    TYPE_ViUInt16},
        {VI_ATTR_RSRC_SPEC_VERSION  ,"VI_ATTR_RSRC_SPEC_VERSION",  TYPE_ViVersion},
        {VI_ATTR_INTF_TYPE          ,"VI_ATTR_INTF_TYPE",          TYPE_ViUInt16},
        {VI_ATTR_GPIB_PRIMARY_ADDR  ,"VI_ATTR_GPIB_PRIMARY_ADDR",  TYPE_ViUInt16},
        {VI_ATTR_GPIB_SECONDARY_ADDR,"VI_ATTR_GPIB_SECONDARY_ADDR",TYPE_ViUInt16},
        {VI_ATTR_RSRC_MANF_NAME     ,"VI_ATTR_RSRC_MANF_NAME",     TYPE_ViString},
        {VI_ATTR_RSRC_MANF_ID       ,"VI_ATTR_RSRC_MANF_ID",       TYPE_ViUInt16},
        {VI_ATTR_INTF_NUM           ,"VI_ATTR_INTF_NUM",           TYPE_ViUInt16},
        {VI_ATTR_TRIG_ID            ,"VI_ATTR_TRIG_ID",            TYPE_ViInt16},
        {VI_ATTR_GPIB_REN_STATE     ,"VI_ATTR_GPIB_REN_STATE",     TYPE_ViBoolean},
        {VI_ATTR_GPIB_UNADDR_EN     ,"VI_ATTR_GPIB_UNADDR_EN",     TYPE_ViBoolean},
        {VI_ATTR_JOB_ID             ,"VI_ATTR_JOB_ID",             TYPE_ViJobId},
        {VI_ATTR_EVENT_TYPE         ,"VI_ATTR_EVENT_TYPE",         TYPE_ViEventType},
        {VI_ATTR_SIGP_STATUS_ID     ,"VI_ATTR_SIGP_STATUS_ID",     TYPE_ViUInt16},
        {VI_ATTR_RECV_TRIG_ID       ,"VI_ATTR_RECV_TRIG_ID",       TYPE_ViInt16},
        {VI_ATTR_INTR_STATUS_ID     ,"VI_ATTR_INTR_STATUS_ID",     TYPE_ViUInt32},
        {VI_ATTR_STATUS             ,"VI_ATTR_STATUS",             TYPE_ViStatus},
        {VI_ATTR_RET_COUNT          ,"VI_ATTR_RET_COUNT",          TYPE_ViUInt32},
        {VI_ATTR_RECV_INTR_LEVEL    ,"VI_ATTR_RECV_INTR_LEVEL",    TYPE_ViInt16},
        {VI_ATTR_OPER_NAME          ,"VI_ATTR_OPER_NAME",          TYPE_ViString},
        {VI_ATTR_TCPIP_ADDR         ,"VI_ATTR_TCPIP_ADDR",         TYPE_ViString},
        {VI_ATTR_TCPIP_HOSTNAME     ,"VI_ATTR_TCPIP_HOSTNAME",     TYPE_ViString},
        {VI_ATTR_TCPIP_PORT         ,"VI_ATTR_TCPIP_PORT",         TYPE_ViUInt16},
        {VI_ATTR_TCPIP_NODELAY      ,"VI_ATTR_TCPIP_NODELAY",      TYPE_ViBoolean},
        {VI_ATTR_TCPIP_KEEPALIVE    ,"VI_ATTR_TCPIP_KEEPALIVE",    TYPE_ViBoolean},
        {0, NULL, 0}
    };
    int i;
    for(i=0; VI_ATTR[i].String; i++) 
    {
        if(strcmp(VI_ATTR[i].String, String) == 0) 
        {
            *pValue = VI_ATTR[i].Value;
            *pType  = VI_ATTR[i].Type;
            return 0;
        }
    }
    return -1;
}
// end of mexVISA_Get_VI_ATTR()

// ================================================================================================
// FUNCTION  : mexFunction
// ------------------------------------------------------------------------------------------------
// Purpose   : Entry point for calls from MATLAB
// Parameters: nlhs -- Number of passed left-hand-side parameters
//             plhs -- Pointer to the left-hand-side parameter list
//             nrhs -- Number of passed right-hand-side arguments
//             prhs -- Pointer to the right-hand-side argument list
// ================================================================================================
void __cdecl mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    char *cmd;  // passed command parameter

    // --------------
    // Check for Init
    // --------------
    if( !mexVISA_IsInitialized ) mexVISA_Init();

    // ------------------
    // Dummy library call
    // ------------------
    if( !nrhs && !nlhs ) 
    {
        mexPrintf("\n");
        mexPrintf("   MATLAB VISA/GPIB Toolbox (%s %s)\n",__DATE__,__TIME__);
        mexPrintf("   Copyright 2007 DSP Group, Inc. All Rights Reserved.\n");
        mexPrintf("\n");
        return;
    }
    // ----------------------
    // Get the command string
    // ----------------------
    if( nrhs<1 ) mexErrMsgTxt("A string argument must be given as the first parameter.");
    cmd = ViMexUtil_GetStringValue(prhs[0],0);

    // ******************************************
    // *****                                *****
    // *****   COMMAND LOOKUP-AND-EXECUTE   *****
    // *****                                *****
    // ******************************************

    //----- mexVISA_*() Functions ----------------
    //--------------------------------------------
    if(strstr(cmd, "vi"))
    {
        if(!mexVISA(nlhs, plhs, nrhs, prhs))
            return;
    }
    //----- mexVISA_DisplaySessionList() ---------
    //--------------------------------------------
    if(!strcmp(cmd, "mexVISA_DisplaySessionList()"))
    {
        mexVISA_DisplaySessionList();
        return;
    }
    //----- EnableResourceChecking ---------------
    //--------------------------------------------
    if(!strcmp(cmd, "EnableResourceChecking"))
    {
        ResourceChecking = VI_TRUE;
        return;
    }
    //----- DisableResourceChecking --------------
    //--------------------------------------------
    if(!strcmp(cmd, "DisableResourceChecking"))
    {
        ResourceChecking = VI_FALSE;
        return;
    }
    //----- EnableSharedSession ------------------
    //--------------------------------------------
    if(!strcmp(cmd, "EnableSharedSessions"))
    {
        AllowSharedSessions = VI_TRUE;
        return;
    }
    //----- DisableSharedSession -----------------
    //--------------------------------------------
    if(!strcmp(cmd, "DisableSharedSessions"))
    {
        AllowSharedSessions = VI_FALSE;
        return;
    }
    //----- CloseAllSessions ---------------------
    //--------------------------------------------
    if(!strcmp(cmd, "CloseAllSessions"))
    {
        if(!ResourceChecking) 
            mexErrMsgTxt("CloseAllSessions requires ResourceChecking to be enabled.");

        while( pSessionList )
        {
            if(pSessionList->Prev) free(pSessionList->Prev);
            mexPrintf(" mexVISA: closing 0x%08X \"%s\"\n",pSessionList->Session,pSessionList->Resource);
            viClose(pSessionList->Session);
            pSessionList = pSessionList->Next;
        }         
        return;
    }
    //----- mexVISA_ReadLeCroyWaveform() ---------
    //--------------------------------------------
    if(!strcmp(cmd,"mexVISA_ReadLeCroyWaveform()"))
    {
        ViSession rmSession, Session;
        int       i, pts, cnt, MaxLength = 16000000;
        char     *buf, *resource, *trace, readbuf[64];
        double   *y, ygain, yofs, dt, t0;

        //- Get input parameters ------------------------------------------------
        ViMexUtil_CheckParameters(nlhs,nrhs,3,3);
        resource = ViMexUtil_GetStringValue(prhs[1], NULL);
        trace    = ViMexUtil_GetStringValue(prhs[2], NULL);

        //- Open instrument session ---------------------------------------------
        mexVISA_GetViRM(&rmSession);
        VICHK( viOpen(rmSession, resource, VI_NULL, VI_NULL, &Session) );

        //- Configure VISA Formatted I/O ----------------------------------------
        VICHK( viSetAttribute(Session, VI_ATTR_TMO_VALUE, 1000) );
        VICHK( viSetBuf      (Session, VI_READ_BUF|VI_WRITE_BUF, 4000) );
        VICHK( viSetAttribute(Session, VI_ATTR_WR_BUF_OPER_MODE, VI_FLUSH_ON_ACCESS) );
        VICHK( viSetAttribute(Session, VI_ATTR_RD_BUF_OPER_MODE, VI_FLUSH_ON_ACCESS) );
        VICHK( viSetAttribute(Session, VI_ATTR_TERMCHAR_EN, VI_FALSE) );

        VICHK( viPrintf(Session,"COMM_ORDER HI\n") );
        VICHK( viPrintf(Session,"COMM_FORMAT DEF9,BYTE,BIN\n") );
        VICHK( viPrintf(Session,"WFSU SP,1,NP,%d,FP,0,SN,0\n",MaxLength) );
        
        MXERR(LeCroyInspectReal(Session, trace, "HORIZ_INTERVAL",  &dt   ) );
        MXERR(LeCroyInspectReal(Session, trace, "HORIZ_OFFSET",    &t0   ) );
        MXERR(LeCroyInspectReal(Session, trace, "VERTICAL_GAIN",   &ygain) );
        MXERR(LeCroyInspectReal(Session, trace, "VERTICAL_OFFSET", &yofs ) );
        MXERR(LeCroyInspectInt (Session, trace, "WAVE_ARRAY_COUNT",&pts  ) );

        if(pts > MaxLength) pts = MaxLength;
        if(!pts) 
        {
            viClose(Session); mxFree(resource); mxFree(trace);
            mexErrMsgTxt("Trace contains no data.");
        }
        buf = (char *)mxCalloc(pts+16, sizeof(char));
        if(!buf) mexErrMsgTxt("Memory allocation failed.");

        VICHK( viSetAttribute(Session, VI_ATTR_TMO_VALUE, 30000) );
        VICHK( viPrintf(Session, "CFMT IND0,BYTE,BIN;CHDR OFF;CORD LO\n") );
        VICHK( viPrintf(Session, "%s:WF? DAT1\n",trace) );
        VICHK( viRead  (Session, readbuf, 7, NULL) );
        VICHK( viRead  (Session, buf, pts, &cnt) );

        y = (double *)mxCalloc(pts, sizeof(double));
        if(!y) mexErrMsgTxt("Memory allocation failed.");

        // ------------------------------
        // Convert Sample Values to Volts
        // ------------------------------
        for(i=0; i<pts; i++)   
        {
            y[i] = ygain * (double)buf[i] - yofs;
        }
        mxFree(buf);
        ViMexUtil_PutRealArray(pts, y, plhs+0);
        ViMexUtil_PutRealValue(dt, plhs+1);
        ViMexUtil_PutRealValue(t0, plhs+2);
        mxFree(trace); mxFree(resource);
        viClose(Session);
        return;
    }
    // ******************************************************************
    // --------------------------------------------
    // No Command Found...Generate an Error Message
    // --------------------------------------------
    mexErrMsgTxt("mexLab: Unrecognized command in the first argument.");
}
// end of mexFunction()

// ================================================================================================
// FUNCTION  : mexVISA_InsertSession()
// ------------------------------------------------------------------------------------------------
// Purpose   : Add a session to the internal linked list
// Parameters: Session  -- VISA session handle
//             Resource -- Resource string
// ================================================================================================
void mexVISA_InsertSession(ViSession Session, const ViString Resource, const ViString Source)
{
    MexVisaSessionList_t *pNewItem = (MexVisaSessionList_t *)calloc(1, sizeof(MexVisaSessionList_t));
    if(pNewItem)
    {
        pNewItem->Session = Session;
        strncpy(pNewItem->Resource, Resource, sizeof(pNewItem->Resource)-1);
        strncpy(pNewItem->Source,   Source,   sizeof(pNewItem->Source  )-1);

        if(pSessionList) // add to list
        {
            MexVisaSessionList_t *pItem = pSessionList;
            while(pItem->Next)
                pItem = pItem->Next;

            pItem->Next = pNewItem;
            pNewItem->Prev = pItem;            
        }
        else // first item 
        {
            pSessionList = pNewItem;
        }
    }
    else mexErrMsgTxt("Memory allocation error in mexVISA_InsertSession()");
}
// end of mexVISA_InsertSession()

// ================================================================================================
// FUNCTION  : mexVISA_RemoveSession()
// ------------------------------------------------------------------------------------------------
// Purpose   : Add a session to the internal linked list
// Parameters: Session  -- VISA session handle
//             Resource -- Resource string
// ================================================================================================
void mexVISA_RemoveSession(ViSession Session)
{
    MexVisaSessionList_t* pItem = pSessionList;
    if( pSessionList )
    {
        // loop for the session record
        while(pItem)
        {
            if(pItem->Session == Session)
                break;

            pItem = pItem->Next;
        }            
        // remove the item from the list
        if(pItem->Session == Session)
        {
            if(pItem->Prev) (pItem->Prev)->Next = pItem->Next;
            if(pItem->Next) (pItem->Next)->Prev = pItem->Prev;
            if(pSessionList == pItem)
                pSessionList = pItem->Next;

            free(pItem);
        }
        else mexErrMsgTxt("Unable to find session in mexVISA_RemoveSession");
    }
    else mexErrMsgTxt("Empty list in mexVISA_RemoveSession");
}
// end of mexVISA_RemoveSession()

// ================================================================================================
// FUNCTION  : mexVISA_ResourceIsOpen()
// ------------------------------------------------------------------------------------------------
// Purpose   : Display the VISA session list
// ================================================================================================
ViBoolean mexVISA_ResourceIsOpen(ViString Resource, ViSession *pSession)
{
    MexVisaSessionList_t* pItem;

    if(pSession) *pSession = 0;

    for(pItem=pSessionList; pItem; pItem=pItem->Next)
    {
        if( !strcmp(Resource, pItem->Resource) )
        {
            if(pSession) *pSession = pItem->Session;
            if( !AllowSharedSessions )
            {
                mexPrintf(" Resource \"%s\" in use as session 0x%08X\n",Resource, pItem->Session);
                mexPrintf(" ...From: %s\n",pItem->Source);
            }
            return VI_TRUE;
        }
    }
    return VI_FALSE;
}
// end of mexVISA_DisplaySessionList()

// ================================================================================================
// FUNCTION  : mexVISA_DisplaySessionList()
// ------------------------------------------------------------------------------------------------
// Purpose   : Display the VISA session list
// Parameters: none
// ================================================================================================
void mexVISA_DisplaySessionList(void)
{
    MexVisaSessionList_t* pItem;

    if(pSessionList)
    {
        mexPrintf("mexVISA_Session_List_____________________________________________________________\n\n");
        for( pItem=pSessionList; pItem; pItem=pItem->Next )
            mexPrintf("SESSION: 0x%08X, RESOURCE: \"%s\"\n%s\n",pItem->Session,pItem->Resource,pItem->Source);
    }
    else mexPrintf(" mexVISA: No open sessions.\n");
}
// end of mexVISA_DisplaySessionList()

// ================================================================================================
// FUNCTION  : mexVISA_Init()
// ------------------------------------------------------------------------------------------------
// Purpose   : Initialization of the mexVISA library
// Parameters: none
// ================================================================================================
void __cdecl mexVISA_Init(void)
{
    if( !mexVISA_IsInitialized )
    {
        mexAtExit(mexVISA_Close); // register library close (DLL Detach) process
        mexVISA_IsInitialized = 1;
    }
}
// end of mexVISA_Init()

// ================================================================================================
// FUNCTION  : mexVISA_Close
// ------------------------------------------------------------------------------------------------
// Purpose   : Shutdown/cleanup the mexVISA library
// Parameters: none
// ================================================================================================
void __cdecl mexVISA_Close(void)
{
    if(ResourceChecking && pSessionList)
    {
        mexPrintf("\n");
        mexPrintf(" WARNING: mexVISA_Close: There are still open sessions.\n\n");
        mexVISA_DisplaySessionList();

        while(pSessionList)
        {
            mexPrintf(" Closing session 0x%08X \"%s\"\n",pSessionList->Session,pSessionList->Resource);
            viClose(pSessionList->Session);
            mexVISA_RemoveSession(pSessionList->Session);
        }
    }
    mexVISA_IsInitialized = 0;
}
// end of mexVISA_Close()

/*************************************************************************************************/
/*=== END OF SOURCE CODE ========================================================================*/
/*************************************************************************************************/

// 2007-06-27 Brickner: Added command "viReadBinary()"
// 2007-07-10 Brickner: Added VISA resource open/close checking with MATLAB stack trace
// 2007-09-10 Brickner: Replace "cmd" with a local array and free the dynamic version
