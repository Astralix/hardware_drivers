//
// RvSocket.h -- Socket Wrapper for Remote VWAL
// Copyright 2007 DSP Group, Inc. All rights reserved.
//
//

#ifndef __RVSOCKET_H
#define __RVSOCKET_H

#if defined(__cplusplus) || defined(__cplusplus__)
    extern "C" {
#endif

#include "RvTypes.h"

// ================================================================================================
// FUNCTION PROTOTYPES
// ================================================================================================
RvStatus_t RvSocketStartup(void);
RvStatus_t RvSocketShutdown(void);
        
RvStatus_t RvSocketCreateClient(char ServerAddr[], uint16_t Port);
RvStatus_t RvSocketCreateServer(uint16_t Port);
RvStatus_t RvSocketAcceptConnection(void);
RvStatus_t RvSocketCloseConnection(void);

int        RvSocketIsConnected(void);

RvStatus_t RvSocketSend            (char *Buffer, int32_t Length, int32_t *pSentLength   );
RvStatus_t RvSocketReceive         (char *Buffer, int32_t Length, int32_t *pReceiveLength);
RvStatus_t RvSocketReceiveRemainder(char *Buffer, int32_t Length, int32_t *pReceiveLength);

//-------------------------------------------------------------------------------------------------
#if defined(__cplusplus) || defined(__cplusplus__)
    }
#endif


#endif // __RVSOCKET_H

//-------------------------------------------------------------------------------------------------
//--- End of File ---------------------------------------------------------------------------------
