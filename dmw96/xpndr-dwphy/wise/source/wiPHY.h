//
//  wiPHY.h -- WiSE Physical Layer Interface
//  Copyright 2001-2002 Bermai, 2007-2011 DSP Group, Inc. All rights reserved.
//
//  This module provides an interface to the actual physical layer functions. It should be used
//  in place of direct calls to the underlying functions in order to provide a generic interface.
//  By using function calls here, high level test routines can be written to work with multiple 
//  instances of physical layers without regard to the fact that they will be implemented with 
//  difference source code.
//

#ifndef __WIPHY_H
#define __WIPHY_H

#include "wiType.h"

#if defined(__cplusplus) || defined(__cplusplus__)
    extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                      
//  INTERFACE INSTRUCTION FORMAT (32-bit)                                               
//                                                                                      
//  Instructions are sent as an integer the upper 16-bit word is the instruction and the lower
//  16 bits is a parameter code to be used with the set/get data commands.

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//  PHY Function Command Codes
//  These are used as the first argument to to interface function
//
#define WIPHY_ADD_METHOD         (0x01 << 16) // register/add any relavent methods            
#define WIPHY_REMOVE_METHOD      (0x02 << 16) // remove any relavent methods                  
#define WIPHY_SET_DATA           (0x11 << 16) // set a value/array in the PHY                 
#define WIPHY_GET_DATA           (0x12 << 16) // get a value/array from the PHY               
#define WIPHY_RESET              (0x21 << 16) // reset the PHY                                
#define WIPHY_POWER_ON           (0x22 << 16) // power on the PHY layer                       
#define WIPHY_POWER_OFF          (0x23 << 16) // power off the PHY layer                      
#define WIPHY_CLOSE_ALL_THREADS  (0x24 << 16) // close all threads (except base thread 0)
#define WIPHY_START_TEST         (0x25 << 16) // execute at the start of a test (wiTest_StartTest)
#define WIPHY_END_TEST           (0x26 << 16) // execute at the end of a test (wiTest_EndTest)
#define WIPHY_WRITE_CONFIG       (0x30 << 16) // write configuration information              
#define WIPHY_WRITE_REGISTER     (0x41 << 16) // write a configuration register
#define WIPHY_READ_REGISTER      (0x42 << 16) // read a configuration register

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//  PHY FUNCTION PARAMETER CODES                                                         
//  These are ORed with WIPHY_SET_VALUE or WIPHY_GET_VALUE                               
//
#define WIPHY_TX_DATARATE_BPS     0x3001 // [get] (wiReal) configured data rate (bps)        
#define WIPHY_TX_BANDWIDTH_HZ     0x3002 // [get] (wiReal) aggregate bandwidth (-10 dB)      
#define WIPHY_RX_NUM_PATHS        0x1003 // [set] (int   ) number of RX paths in channel
#define WIPHY_RX_METRIC           0x3004 // [get] (wiReal) implementation dependent receiver metric

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Method Interface Functions
//  Function pointer types for the Method general command and Method TX and RX functions.
//
typedef wiStatus (*wiPhyMethodFn_t  )(int command, void *pdata);
typedef wiStatus (*wiPhyTxMethodFn_t)(wiReal bps, wiInt Nbytes, wiUWORD  *data, int *Ns, wiComplex ***S,
                                      double *Fs, double *Prms);
typedef wiStatus (*wiPhyRxMethodFn_t)(int *Nbytes, wiUWORD **data, int  Nr, wiComplex **r);

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Method Record
//  Used internally to described methods added to the simulator via wiPHY_AddMethod
//
typedef struct wiPhyMethodS
{
    char                *name;    // Name of the method
    wiPhyMethodFn_t      fn;      // Method interface function
    wiPhyTxMethodFn_t    TxFn;    // Transmit function
    wiPhyRxMethodFn_t    RxFn;    // Receive function
    struct wiPhyMethodS *next;    // Ptr -> Next method item
    struct wiPhyMethodS *prev;    // Ptr -> Prev method item

} wiPhyMethod_t;

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Register Data
//  Used with WIPHY_WRITE_REGISTER and WIPHY_READ_REGISTER
//
typedef struct wiPhyRegisterS
{
    wiUInt Addr;
    wiUInt Data;

} wiPhyRegister_t;

//=================================================================================================
//
//  USER-CALLABLE FUNCTION DEFINITIONS (Exportable Functions)
//
//=================================================================================================
wiStatus wiPHY_Init(void);
wiStatus wiPHY_Close(void);

//---- PHY Transceiver Functions ------------------------------------------------------------------
wiStatus wiPHY_TX(double bps, int nbytes, wiUWORD  *data, int *Ns, wiComplex **S[], double *Fs, double *Prms);
wiStatus wiPHY_RX(int *nBytes, wiUWORD *data[], int  Nr, wiComplex *R[]);

//---- PHY Configuration Register I/O -------------------------------------------------------------
wiStatus wiPHY_WriteRegister(wiUInt Addr, wiUInt  Data);
wiStatus wiPHY_ReadRegister (wiUInt Addr, wiUInt *Data);

//---- PHY Interface Functions ---------------------------------------------------------------------
wiStatus wiPHY_Command(int command);
wiStatus wiPHY_GetData(int param, void *pdata);
wiStatus wiPHY_SetData(int param, ...);
wiStatus wiPHY_WriteConfig(wiMessageFn MsgFunc);
wiStatus wiPHY_StartTest(int NumberOfThreads);
wiStatus wiPHY_EndTest(wiMessageFn MsgFunc);

//---- PHY Interface Configuration Functions ------------------------------------------------------
wiStatus wiPHY_AddMethod         (wiString name, wiPhyMethodFn_t fn, wiPhyMethod_t **method);
wiStatus wiPHY_RemoveMethod      (wiPhyMethod_t  *method);
wiStatus wiPHY_MethodList        (wiPhyMethod_t **methodlist);
wiStatus wiPHY_GetMethodByName   (wiPhyMethod_t **method, wiString name);
wiStatus wiPHY_SelectMethod      (wiPhyMethod_t  *method);
wiStatus wiPHY_SelectMethodByName(wiString name);
wiString wiPHY_ActiveMethodName  (void);
//-------------------------------------------------------------------------------------------------
#if defined(__cplusplus) || defined(__cplusplus__)
    }
#endif

#endif
//-------------------------------------------------------------------------------------------------
//--- End of File ---------------------------------------------------------------------------------
