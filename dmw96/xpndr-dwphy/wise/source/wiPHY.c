//--< wiPHY.c >------------------------------------------------------------------------------------
//=================================================================================================
//
//  Physical Layer (Abstraction) Interface
//  Copyright 2001-2002 Bermai, 2005-2011 DSP Group, Inc. All rights reserved.
//
//=================================================================================================

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "wiParse.h"
#include "wiPHY.h"
#include "wiUtil.h"
#include "wise.h"

//=================================================================================================
//--  PARAMETERS WITH MODULE (FILE) SCOPE
//=================================================================================================
static int            Verbose      = 0;
static wiPhyMethod_t *MethodList   = NULL;
static wiPhyMethod_t *ActiveMethod = NULL;
static wiBoolean      Initialized  = 0;

//=================================================================================================
//--  ERROR HANDLING
//=================================================================================================
#define  STATUS(arg)  WICHK((arg))
#define XSTATUS(arg) if (WICHK(arg)<0) return WI_ERROR_RETURNED;

wiStatus wiPHY_ParseLine(wiString text);

// ================================================================================================
// FUNCTION  : wiPHY_Init
// ------------------------------------------------------------------------------------------------
// Purpose   : Initialize the wiPHY module
// Parameters: none
// ================================================================================================
wiStatus wiPHY_Init(void)
{
    if (Verbose) wiPrintf(" wiPHY_Init()\n");
    if (Initialized) return STATUS(WI_ERROR_MODULE_ALREADY_INIT);

    Initialized = 1;
    
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Add built-in methods to the list
    //
    //  New "PHY" methods must be manually added to this list below. Inclusion of the PHY
    //  method is conditioned on a declaration in the associated header file. This allows
    //  PHY modules to be added or removed at compile time with conditional inclusion of
    //  the header in wise.h.
    //
    #ifdef __PHY_802_11A_H
        #pragma message("...Include PHY Method - PHY_802_11a")
        XSTATUS(wiPHY_AddMethod("PHY_802_11a", PHY_802_11a, NULL));
    #endif

    #ifdef __PHY11N_H
        #pragma message("...Include PHY Method - Phy11n")
        XSTATUS(wiPHY_AddMethod("Phy11n",      Phy11n, NULL));
        XSTATUS(wiPHY_AddMethod("PHY_802_11n", Phy11n, NULL)); // old name (WiSE 3.X)
    #endif

    #ifdef __NPHY_H
        #pragma message("...Include PHY Method - nPHY")
        XSTATUS(wiPHY_AddMethod("nPHY", nPHY, NULL));
    #endif

    #ifdef __BER7211A1_H
        #pragma message("...Include PHY Method - BER7211a1")
        XSTATUS(wiPHY_AddMethod("BER7211a1", BER7211a1, NULL));
    #endif

    #ifdef __DW52_H
        #pragma message("...Include PHY Method - DW52")
        XSTATUS(wiPHY_AddMethod("DW52",DW52,      NULL));
    #endif

    #ifdef __DW74_H
        #pragma message("...Include PHY Method - DW74")
        XSTATUS(wiPHY_AddMethod("DW74",DW74,      NULL));
    #endif

    #ifdef __NEVADAPHY_H
        #pragma message ("...Include PHY Method - NevadaPHY")
        XSTATUS(wiPHY_AddMethod("NevadaPHY",NevadaPHY,      NULL));
    #endif

    #ifdef __TAMALEPHY_H
        #pragma message("...Include PHY Method - TamalePhy")
        XSTATUS(wiPHY_AddMethod("TamalePHY",TamalePHY,      NULL));
    #endif

    #ifdef __VERONAPHYTX_H
        #pragma message("...Include PHY Method - VeronaPhyTx")
        XSTATUS(wiPHY_AddMethod("VeronaPhyTx",VeronaPhyTx,  NULL));
    #endif

    #ifdef __CALCEVM_H
        #pragma message("...Include PHY Method - CalcEVM")
        XSTATUS(wiPHY_AddMethod("CalcEVM",CalcEVM, NULL));
    #endif

    // -----------------------------
    // Add the line parsing function
    // -----------------------------
    XSTATUS(wiParse_AddMethod(wiPHY_ParseLine));

    return WI_SUCCESS;
}
// end of wiPHY_Init()

// ================================================================================================
// FUNCTION  : wiPHY_Close
// ------------------------------------------------------------------------------------------------
// Purpose   : Close the wiPHY module
// Parameters: none
// ================================================================================================
wiStatus wiPHY_Close(void)
{
    wiPhyMethod_t *method = MethodList;

    if (Verbose) wiPrintf(" wiPHY_Close()\n");
    if (!Initialized)  return WI_ERROR_MODULE_NOT_INITIALIZED;

    while (method)
    {
        wiPhyMethod_t *next = method->next;

        XSTATUS( method->fn(WIPHY_REMOVE_METHOD, NULL) );
        if (method->name) wiFree(method->name);
        wiFree(method);
        method = next;
    }
    MethodList   = NULL;
    ActiveMethod = NULL;

    STATUS( wiParse_RemoveMethod(wiPHY_ParseLine) ); // Remove the line parsing function
    Initialized = 0; // Mark module not initialized

    return WI_SUCCESS;
}
// end of wiPHY_Close()

// ================================================================================================
// FUNCTION  : wiPHY_AddMethod
// ------------------------------------------------------------------------------------------------
// Purpose   : Add a PHY method to the internal list
// Parameters: name   -- Descriptive name for the method
//             fn     -- Pointer to the function used to implement the method
//             method -- Return a pointer to the new record (by reference)
// ================================================================================================
wiStatus wiPHY_AddMethod(char *name, wiPhyMethodFn_t fn, wiPhyMethod_t **method)
{
    wiPhyMethod_t *list, *item;

    if (Verbose) wiPrintf(" wiPHY_AddMethod(\"%s\",*,*)\n",name);

    // --------------------
    // Range/Error Checking
    // --------------------
    if (!Initialized) return WI_ERROR_MODULE_NOT_INITIALIZED;
    if (!name)  return WI_ERROR_PARAMETER1;
    if (!fn)    return WI_ERROR_PARAMETER2;

    // ---------------------
    // Create the new record
    // ---------------------
    WICALLOC( item, 1, wiPhyMethod_t );
    WICALLOC( item->name, strlen(name)+1, char );
    sprintf(item->name, name);

    item->fn = fn;

    // ------------------------------
    // Add the new record to the list
    // ------------------------------
    if (!MethodList)
    {
        MethodList   = item; // item is the first item in the list
        ActiveMethod = item; // set default method to be the first in the list
    }
    else 
    {
        for (list=MethodList; list->next; list=list->next);
        list->next = item;
        item->prev = list;
    }
    if (method) *method = item;

    // -----------------------------------
    // Call the method's addition function
    // -----------------------------------
    XSTATUS( fn(WIPHY_ADD_METHOD, item) );

    return WI_SUCCESS;
}
// end of wiPHY_AddMethod()

// ================================================================================================
// FUNCTION  : wiPHY_MethodList
// ------------------------------------------------------------------------------------------------
// Purpose   : Returns a pointer to the internal method list.
// Parameters: ListPtr -- Pointer to the method list (by reference)
// ================================================================================================
wiStatus wiPHY_MethodList(wiPhyMethod_t **ListPtr)
{
    if (Verbose)  wiPrintf(" wiPHY_MethodList(*)\n");

    if (!Initialized) return WI_ERROR_MODULE_NOT_INITIALIZED;
    if (!ListPtr)     return WI_ERROR_PARAMETER1;

    *ListPtr = MethodList;

    return WI_SUCCESS;
}
// end of wiPHY_MethodList()

// ================================================================================================
// FUNCTION  : wiPHY_SelectMethod
// ------------------------------------------------------------------------------------------------
// Purpose   : Selects the active method
// Parameters: method -- Method to be selected
// ================================================================================================
wiStatus wiPHY_SelectMethod(wiPhyMethod_t *method)
{
    if (Verbose) wiPrintf(" wiPHY_SelectMethod(*)\n");

    if (!Initialized) return WI_ERROR_MODULE_NOT_INITIALIZED;
    if (!method)      return WI_ERROR_PARAMETER1;

    // -----------------------------------------------------------
    // If the selected method is different than the current one...
    // -----------------------------------------------------------
    if (method != ActiveMethod)
    {
        // Close the previous method
        // -------------------------
        if (ActiveMethod)
        {
            STATUS(ActiveMethod->fn(WIPHY_CLOSE_ALL_THREADS, NULL));
            STATUS(ActiveMethod->fn(WIPHY_POWER_OFF, NULL));
        }
        // Open the new method
        // -------------------
        ActiveMethod = method;
        STATUS( ActiveMethod->fn(WIPHY_POWER_ON, NULL));
    }
    return WI_SUCCESS;
}
// end of wiPHY_SelectMethod()

// ================================================================================================
// FUNCTION  : wiPHY_GetMethodByName
// ------------------------------------------------------------------------------------------------
// Purpose   : Gets a method handle by name
// Parameters: method -- Method handle (by reference)
//             name   -- Name of the method
// ================================================================================================
wiStatus wiPHY_GetMethodByName(wiPhyMethod_t **method, wiString name)
{
    if (Verbose) wiPrintf(" wiPHY_GetMethodByName(**,\"%s\")\n",name);

    if (!method) return WI_ERROR_PARAMETER1;
    if (!name)   return WI_ERROR_PARAMETER2;

    // Step through the list until a name match is found
    // -------------------------------------------------
    for (*method = MethodList; *method; *method = (*method)->next)
    {
        if (strcmp(name, (*method)->name) == 0)
            break;
    }
    if (!*method) return WI_ERROR_PARAMETER2;

    return WI_SUCCESS;
}
// end of wiPHY_GetMethodByName()

// ================================================================================================
// FUNCTION  : wiPHY_SelectMethodByName
// ------------------------------------------------------------------------------------------------
// Purpose   : Selects the active method by name
// Parameters: name -- Name of the method to select
// ================================================================================================
wiStatus wiPHY_SelectMethodByName(wiString name)
{
    wiPhyMethod_t *method;

    if (Verbose) wiPrintf(" wiPHY_SelectMethodByName(\"%s\")\n",name);
    if (!name)   return WI_ERROR_PARAMETER1;

    XSTATUS( wiPHY_GetMethodByName(&method, name) );
    XSTATUS( wiPHY_SelectMethod(method) );

    return WI_SUCCESS;
}
// end of wiPHY_SelectMethodByName()

// ================================================================================================
// FUNCTION  : wiPHY_RemoveMethod
// ------------------------------------------------------------------------------------------------
// Purpose   : Removes a PHY method from the list
// Parameters: method -- Method to be removed
// ================================================================================================
wiStatus wiPHY_RemoveMethod(wiPhyMethod_t *method)
{
    if (Verbose) wiPrintf(" wiPHY_RemoveMethod(*)\n");

    if (!Initialized) return WI_ERROR_MODULE_NOT_INITIALIZED;
    if (!method)      return WI_ERROR_PARAMETER1;

    // -----------------------------------------------------------------------
    // If the current method is the one to be removed, clear the active method
    // -----------------------------------------------------------------------
    if (method == ActiveMethod)
    {
        STATUS(ActiveMethod->fn(WIPHY_CLOSE_ALL_THREADS, NULL));
        STATUS(ActiveMethod->fn(WIPHY_POWER_OFF, NULL));
        ActiveMethod = NULL;
    }
    // ----------------------------------
    // Call the method's removal function
    // ----------------------------------
    XSTATUS( method->fn(WIPHY_REMOVE_METHOD, NULL) );

    // -------------------------------
    // Remove the method from the list
    // -------------------------------
    if (method == MethodList) 
    {
        MethodList = MethodList->next;
        MethodList->prev = NULL;
    }
    else
    {
        wiPhyMethod_t *item;

        for (item = MethodList; (item && (item!=method)); item = item->next) 
            ;
        if (method == item) 
        {
            if (item->prev) item->prev->next = item->next;
            if (item->next) item->next->prev = item->prev;
        }
    }
    if (method->name) wiFree(method->name);
    wiFree(method);

    if (!method && MethodList) STATUS(wiPHY_SelectMethod(MethodList));

    return WI_SUCCESS;
}
// end of wiPHY_RemoveMethod()

// ================================================================================================
// FUNCTION  : wiPHY_ActiveMethodName
// ------------------------------------------------------------------------------------------------
// Purpose   : Returns a the active method name
// Parameters: none
// Returns   : String pointer with the active method name
// ================================================================================================
wiString wiPHY_ActiveMethodName(void)
{
    if (!ActiveMethod) return NULL;
    return ActiveMethod->name;
}
// end of wiPHY_ActiveMethodName()

// ================================================================================================
// FUNCTION  : wiPHY_Command
// ------------------------------------------------------------------------------------------------
// Purpose   : Execute a method command
// Parameters: command -- WIPHY_* instruction from wiPHY.h
// ================================================================================================
wiStatus wiPHY_Command(int command)
{
    if (Verbose) wiPrintf(" wiPHY_Command(0x%08X)\n",command);

    if (InvalidRange(command, 0x00010000, 0x7FFF0000)) return WI_ERROR_PARAMETER2;
    if (!ActiveMethod) return WI_ERROR_INVALID_METHOD;

    XSTATUS( ActiveMethod->fn(command, ActiveMethod) );
    return WI_SUCCESS;
}
// end of wiPHY_Command()

// ================================================================================================
// FUNCTION  : wiPHY_GetData
// ------------------------------------------------------------------------------------------------
// Purpose   : Retrieve the value of a data element from the method
// Parameters: param   -- WIPHY_* parameter code from wiPHY.h
//             pValue  -- Ptr->variable in which the value is returned
// ================================================================================================
wiStatus wiPHY_GetData(int param, void *pdata)
{
    if (Verbose) wiPrintf(" wiPHY_GetData(0x%08X, *)\n",param);

    if (!ActiveMethod)                       return WI_ERROR_INVALID_METHOD;
    if (InvalidRange(param, 0x0001, 0xFFFF)) return WI_ERROR_PARAMETER1;
    if (!pdata)                              return WI_ERROR_PARAMETER3;

    XSTATUS( ActiveMethod->fn(WIPHY_GET_DATA | param, pdata) );
    return WI_SUCCESS;
}
// end of wiPHY_GetData()

// ================================================================================================
// FUNCTION  : wiPHY_SetData
// ------------------------------------------------------------------------------------------------
// Purpose   : Set the value for a data element of the method
// Parameters: param   -- WIPHY_* parameter code from wiPHY.h
//             pValue  -- Ptr->variable in which the value is returned
// ================================================================================================
wiStatus wiPHY_SetData(int param, ...)
{
    va_list ap; void *ptr;
    int ival; wiUInt uval; wiReal rval; wiComplex cval; wiBoolean bval;

    if (Verbose) wiPrintf(" wiPHY_SetData(0x%04X,,...)\n",param);

    if (!ActiveMethod) return WI_ERROR_INVALID_METHOD;
    if (InvalidRange(param, 0x0001, 0xFFFF)) return WI_ERROR_PARAMETER1;

    va_start(ap, param);
    switch (param >> 12)
    {
        //-- Pointers/Arrays --------
        case WITYPE_ADDR:
            ptr = va_arg(ap, void *);
            va_end(ap);
            break;
                        
        //-- Integer ----------------
        case WITYPE_INT:
            ival = va_arg(ap, int);
            va_end(ap);
            ptr = &ival;
            break;

        //-- Unsigned Integer -------
        case WITYPE_UINT:
            uval = va_arg(ap, wiUInt);
            va_end(ap);
            ptr = &uval;
            break;

        //-- Real/Floating Point ----
        case WITYPE_REAL:
            rval = va_arg(ap, wiReal);
            va_end(ap);
            ptr = &(rval);

        //-- Complex Value ----------
        case WITYPE_COMPLEX:
            cval = va_arg(ap, wiComplex);
            va_end(ap);
            ptr = &cval;
            break;

        //-- Boolean ----------------
        case WITYPE_BOOLEAN:
            bval = va_arg(ap, wiBoolean);
            va_end(ap);
            ptr = &bval;
            break;

        //-- Unsupported ------------
        default:
            va_end(ap);
            return WI_ERROR_PARAMETER1;
    }
    XSTATUS(ActiveMethod->fn((WIPHY_SET_DATA | param), ptr));
    return WI_SUCCESS;
}
// end of wiPHY_SetData()

// ================================================================================================
// FUNCTION  : wiPHY_WriteConfig
// ------------------------------------------------------------------------------------------------
// Purpose   : Write configuration data for the current method
// Parameters: MsgFunc -- Message handling function (pointer)
// ================================================================================================
wiStatus wiPHY_WriteConfig(wiMessageFn MsgFunc)
{
    // Cast MsgFunc to a generic function pointer Fn so we can pass a pointer to Fn. This prevents
    // issues if there is an implementation-dependent difference in the size of data and function
    // pointers.
    //
    wiFunction Fn = (wiFunction)MsgFunc;

    if (!MsgFunc)      return WI_ERROR_PARAMETER1;
    if (!ActiveMethod) return WI_ERROR_INVALID_METHOD;

    XSTATUS( ActiveMethod->fn(WIPHY_WRITE_CONFIG, &Fn) );

    return WI_SUCCESS;
}
// end of wiPHY_WriteConfig()

// ================================================================================================
// FUNCTION  : wiPHY_StartTest
// ------------------------------------------------------------------------------------------------
// Purpose   : Generic start-of-test call
// Parameters: NumberOfThreads : (0 = Single Threaded, 1-N = Multithreaded with specified threads)
// ================================================================================================
wiStatus wiPHY_StartTest(wiInt NumberOfThreads)
{
    XSTATUS( ActiveMethod->fn(WIPHY_START_TEST, &NumberOfThreads) );
    return WI_SUCCESS;
}
// end of wiPHY_StartTest()

// ================================================================================================
// FUNCTION  : wiPHY_EndTest
// ------------------------------------------------------------------------------------------------
// Purpose   : Generic end-of-test call
// Parameters: MsgFunc -- Message handling function (pointer)
// ================================================================================================
wiStatus wiPHY_EndTest(wiMessageFn MsgFunc)
{
    // Cast MsgFunc to a generic function pointer Fn so we can pass a pointer to Fn. This prevents
    // issues if there is an implementation-dependent difference in the size of data and function
    // pointers.
    //
    wiFunction Fn = (wiFunction)(MsgFunc == NULL ? wiPrintf : MsgFunc);

    if (!MsgFunc)      return WI_ERROR_PARAMETER1;
    if (!ActiveMethod) return WI_ERROR_INVALID_METHOD;

    XSTATUS( ActiveMethod->fn(WIPHY_END_TEST, &Fn) );

    return WI_SUCCESS;
}
// end of wiPHY_EndTest()

// ================================================================================================
// FUNCTION  : wiPHY_TX
// ------------------------------------------------------------------------------------------------
// Purpose   : Call the PHY transmit function
// Parameters: bps     -- PHY transmission rate (bps)
//             nBytes  -- Number of octets (bytes) in the data array
//             data    -- Data byte buffer
//             Ns      -- Number of samples in s (by reference)
//             S       -- Pointer to internal array s (by reference)
//             Fs      -- Sample rate for channel model (Hz)
//             Prms    -- Complex transmit power (rms)
// ================================================================================================
wiStatus wiPHY_TX(double bps, int nBytes, wiUWORD *data, int *Ns, wiComplex **S[], double *Fs, double *Prms)
{
    if (Verbose) wiPrintf(" wiPHY_TX(%d, *, *, *)\n",nBytes);

    if (!Initialized)                   return WI_ERROR_MODULE_NOT_INITIALIZED;
    if (!ActiveMethod)                  return WI_ERROR_INVALID_METHOD;
    if (InvalidRange(nBytes, 1, 65536)) return WI_ERROR_PARAMETER1;
    if (!data)                          return WI_ERROR_PARAMETER2;
    if (!Ns)                            return WI_ERROR_PARAMETER3;
    if (!S)                             return WI_ERROR_PARAMETER4;
    if (!Fs)                            return WI_ERROR_PARAMETER5;

    XSTATUS( ActiveMethod->TxFn(bps, nBytes, data, Ns, S, Fs, Prms) );
    
    return WI_SUCCESS;
}
// end of wiPHY_TX()

// ================================================================================================
// FUNCTION  : wiPHY_RX
// ------------------------------------------------------------------------------------------------
// Purpose   : Call the PHY receiver function
// Parameters: nBytes   -- Number of octets (bytes) in the data array
//             data     -- Data byte buffer (by reference)
//             Nr       -- Number of samples in r
//             R        -- Array of sample arrays
// ================================================================================================
wiStatus wiPHY_RX(int *nBytes, wiUWORD *data[], int Nr, wiComplex *R[])
{
    if (Verbose) wiPrintf(" wiPHY_RX(*, *, %d, *)\n",Nr);

    if (!Initialized)                 return WI_ERROR_MODULE_NOT_INITIALIZED;
    if (!ActiveMethod)                return WI_ERROR_INVALID_METHOD;
    if (!data)                        return WI_ERROR_PARAMETER2;
    if (InvalidRange(Nr, 100, 1<<30)) return WI_ERROR_PARAMETER3;
    if (!R)                           return WI_ERROR_PARAMETER4;
    if (!R[0])                        return WI_ERROR_PARAMETER4;

    XSTATUS( ActiveMethod->RxFn(nBytes, data, Nr, R) );

    return WI_SUCCESS;
}
// end of wiPHY_RX()

// ================================================================================================
// FUNCTION  : wiPHY_WriteRegister
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiPHY_WriteRegister(wiUInt Addr, wiUInt Data)
{
    wiPhyRegister_t Reg;

    if (Verbose) wiPrintf(" wiPHY_WriteRegister(%u, %u)\n",Addr,Data);

    if (!ActiveMethod) return WI_ERROR_INVALID_METHOD;

    Reg.Addr = Addr;
    Reg.Data = Data;

    XSTATUS( ActiveMethod->fn(WIPHY_WRITE_REGISTER, &Reg) );
    return WI_SUCCESS;
}
// end of wiPHY_WriteRegister()

// ================================================================================================
// FUNCTION  : wiPHY_ReadRegister
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiPHY_ReadRegister(wiUInt Addr, wiUInt *pData)
{
    wiPhyRegister_t Reg;

    if (Verbose) wiPrintf(" wiPHY_WriteRegister(%u, *)\n",Addr);

    if (!ActiveMethod) return WI_ERROR_INVALID_METHOD;
    if (!pData) return WI_ERROR_PARAMETER2;

    Reg.Addr = Addr;
    Reg.Data = 0;

    XSTATUS( ActiveMethod->fn(WIPHY_READ_REGISTER, &Reg) );
    *pData = Reg.Data;

    return WI_SUCCESS;
}
// end of wiPHY_ReadRegister()

// ================================================================================================
// FUNCTION  : wiPHY_ParseLine
// ------------------------------------------------------------------------------------------------
// Purpose   : Parse a line of text for information relavent to this module.
// Parameters: text -- A line of text to examine
// ================================================================================================
wiStatus wiPHY_ParseLine(wiString text)
{
    int status;
    char buf[128];

    if (Verbose) wiPrintf(" wiPHY_ParseLine(\"%s\")\n",text);
    if (!Initialized) return WI_ERROR_MODULE_NOT_INITIALIZED;

    status = STATUS(wiParse_Function(text,"wiPHY_SelectMethodByName(%127s)",&buf));
    if (status == WI_SUCCESS)
    {
        XSTATUS(wiPHY_SelectMethodByName(buf));
        return WI_SUCCESS;
    }
    status = STATUS(wiParse_Command(text,"wiPHY_Reset()"));
    if (status == WI_SUCCESS)
    {
        XSTATUS( wiPHY_Command(WIPHY_RESET) );   
        return WI_SUCCESS;
    }
    return WI_WARN_PARSE_FAILED;
}
// end of wiPHY_ParseLine()

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------
