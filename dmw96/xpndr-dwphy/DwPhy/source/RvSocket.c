//--< RvSocket.c >---------------------------------------------------------------------------------
//=================================================================================================
//
//  Remote VWAL Sockets Wrapper
//  Copyright 2007 DSP Group, Inc. All rights reserved.
//
//  Written by Barrett Brickner
//
//  Notes: This module has been tested as a Windows client and Linux server. I am unsure that the
//         sever code would work on Windows due to the way shutdown is handled.
//
//=================================================================================================
//-------------------------------------------------------------------------------------------------

#ifdef WIN32

    #include <Winsock2.h>
    typedef int socklen_t;

#else

    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <fcntl.h>
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR   -1

#endif

#include <stdio.h>
#include <memory.h>
#include "RvTypes.h"
#include "RvSocket.h"
#include "RvUtil.h"

// ================================================================================================
// MODULE-SPECIFIC PARAMETERS
// ================================================================================================
static int ServerID = INVALID_SOCKET;
static int SocketID = INVALID_SOCKET;

static int RvSocketStarted = 0;
static struct sockaddr_in RvSocketAddr;

const int MAXCONNECTIONS = 5;
const int MAXRECV        = 4096;

#define Invalid(arg) ((arg)==INVALID_SOCKET ? 1:0)
#define DEBUG RvPrintf("### FILE: %s, LINE: %d\n",__FILE__,__LINE__);
#define VPRINTF if(RvUtil_Verbose()) RvPrintf

// ================================================================================================
// FUNCTION  : RvSocketStartup()
// ------------------------------------------------------------------------------------------------
// Purpose   : Startup the sockets library (windows)
// Parameters: none
// ================================================================================================
RvStatus_t RvSocketStartup(void)
{
    #ifdef WIN32

        if(!RvSocketStarted)
        {
            WORD    wVersion = MAKEWORD(2,2);
            WSADATA wsaData;

            if( WSAStartup(wVersion, &wsaData) == 0 )
                RvSocketStarted = 1;
            else
                return RVWAL_ERROR;
        }

    #endif

    RvSocketStarted = 1;
    return RVWAL_SUCCESS;
}
// end of RvSocketStartup()
    
// ================================================================================================
// FUNCTION  : RvSocketShutdown()
// ------------------------------------------------------------------------------------------------
// Purpose   : Shutdown any open connections and clean up socket resources
// Parameters: none
// ================================================================================================
RvStatus_t RvSocketShutdown(void)
{
    VPRINTF(" > RvSocketShutdown()\n");
    if(RvSocketStarted)
    {
        RvSocketCloseConnection();

        #ifdef WIN32
            WSACleanup();
        #else
            if( !Invalid(ServerID) )
                close( ServerID );
            ServerID = INVALID_SOCKET;
        #endif
    }
    RvSocketStarted = 0;
    return RVWAL_SUCCESS;
}
// end of RvSocketShutdown()
    
// ================================================================================================
// FUNCTION  : RvSocketCreateClient()
// ------------------------------------------------------------------------------------------------
// Purpose   : Creates a connection to the Remote VWAL Server
// Parameters: ServerAddr -- Server IP address as a string, e.g., "10.82.0.9"
//             Port       -- Port number to use, e.g., 30000
// ================================================================================================
RvStatus_t RvSocketCreateClient(char ServerAddr[], uint16_t Port)
{
    // ------------------------------
    // Attempt to lock the connection
    // ------------------------------
    if(RvUtil_MutexLock() != RVWAL_SUCCESS)
        return RVWAL_ERROR_CONNECTION_LOCKED; // unable to lock connection

    // --------------------------------
    // Start Sockets (Needed for Win32)
    // --------------------------------
    if(!RvSocketStarted)
        RvSocketStartup();

    // -----------------
    // Create the socket
    // -----------------
    SocketID = socket(AF_INET, SOCK_STREAM, 0);
    if(Invalid(SocketID)) return RVWAL_ERROR;

    #ifdef WIN32 
        if (setsockopt(SocketID, SOL_SOCKET, SO_REUSEADDR|SO_RCVBUF, (const char*) &MAXRECV, sizeof(MAXRECV) ) )
            return RVWAL_ERROR;
    #else
        if (setsockopt(SocketID, IPPROTO_TCP, SO_REUSEADDR|SO_RCVBUF, (const char*) &MAXRECV, sizeof(MAXRECV) ) )
            return RVWAL_ERROR;
    #endif

    // ------------------
    // Address the server
    // ------------------
    RvSocketAddr.sin_family = AF_INET;
    RvSocketAddr.sin_port   = htons(Port);

    #ifdef WIN32

        RvSocketAddr.sin_addr.S_un.S_addr = inet_addr(ServerAddr);
        if(RvSocketAddr.sin_addr.S_un.S_addr == INADDR_NONE)
            return RVWAL_ERROR;

    #else

        RvSocketAddr.sin_addr.s_addr = inet_addr(ServerAddr);
        if(RvSocketAddr.sin_addr.s_addr == INADDR_NONE)
            return RVWAL_ERROR;

    #endif

    // -------------------
    // Make the connection
    // -------------------
    if(connect(SocketID, (struct sockaddr *)&RvSocketAddr, sizeof(RvSocketAddr)) == SOCKET_ERROR)
    {
        RvSocketCloseConnection();
        return RVWAL_ERROR;
    }

    return RVWAL_SUCCESS;
}
// end of RvSocketCreateClient()

// ================================================================================================
// FUNCTION  : RvSocketCreateServer()
// ------------------------------------------------------------------------------------------------
// Purpose   : Open a socket and bind it to the specified port
// Parameters: Port -- Port number to use, e.g., 30000
// ================================================================================================
int32_t RvSocketCreateServer(uint16_t Port)
{
    VPRINTF(" RvSocketCreateServer(%u)\n",Port);

    // --------------------------------
    // Start Sockets (Needed for Win32)
    // --------------------------------
    if(!RvSocketStarted)
        RvSocketStartup();

    // -----------------
    // Create the socket
    // -----------------
    ServerID = socket(AF_INET, SOCK_STREAM, 0);
    if(Invalid(ServerID)) return RVWAL_ERROR;

    #ifdef WIN32 
        if (setsockopt(ServerID, SOL_SOCKET, SO_REUSEADDR|SO_RCVBUF, (const char*) &MAXRECV, sizeof(MAXRECV) ) )
            return RVWAL_ERROR;
    #endif

    // ------------------
    // Address the server
    // ------------------
    RvSocketAddr.sin_family = AF_INET;
    RvSocketAddr.sin_port   = htons(Port);

    #ifdef WIN32
        RvSocketAddr.sin_addr.S_un.S_addr = INADDR_ANY;
    #else
        RvSocketAddr.sin_addr.s_addr = INADDR_ANY;
    #endif

    // ---------------
    // Bind the Socket
    // ---------------
    if(bind(ServerID, (struct sockaddr *)&RvSocketAddr, sizeof(RvSocketAddr)) == SOCKET_ERROR)
    {
        RvSocketCloseConnection();
        return RVWAL_ERROR;
    }
    // ----------------------
    // Listen for Connections
    // ----------------------
    if(listen(ServerID, MAXCONNECTIONS) == SOCKET_ERROR)
        return RVWAL_ERROR;

    return RVWAL_SUCCESS;
}
// end of RvSocketCreateServer()

// ================================================================================================
// FUNCTION  : RvSocketAcceptConnection()
// ------------------------------------------------------------------------------------------------
// Purpose   : Wait until a connection is made, then accept and return
// Parameters: none
// ================================================================================================
RvStatus_t RvSocketAcceptConnection(void)
{
    socklen_t AddrLen = sizeof(RvSocketAddr);

    VPRINTF(" RvSocketAcceptConnection: Waiting...\n");

    if( Invalid(ServerID)) return RVWAL_ERROR;
    if(!Invalid(SocketID)) RvSocketCloseConnection();

    SocketID = accept(ServerID, (struct sockaddr *)&RvSocketAddr, &AddrLen);
    if(Invalid(SocketID)) return RVWAL_ERROR;

    #ifndef WIN32
        RvPrintf(" RVWAL Connection %d Accepted from %s\n",SocketID,inet_ntoa(RvSocketAddr.sin_addr));
    #endif

    return RVWAL_SUCCESS;
}
// end of RvSocketAcceptConnection()

// ================================================================================================
// FUNCTION  : RvSocketCloseConnection()
// ------------------------------------------------------------------------------------------------
// Purpose   : Close an active socket connection
// Parameters: none
// ================================================================================================
RvStatus_t RvSocketCloseConnection(void)
{
    if( !Invalid(SocketID) )
    {
        #ifdef WIN32
            closesocket(SocketID);
        #else
            close(SocketID);
            RvPrintf(" RVWAL Connection %d Closed\n",SocketID);
        #endif
    }
    SocketID = INVALID_SOCKET;
    RvUtil_MutexUnlock();

    return RVWAL_SUCCESS;
}
// end of RvSocketCloseConnection()

// ================================================================================================
// FUNCTION  : RvSocketIsConnected()
// ------------------------------------------------------------------------------------------------
// Purpose   : Returns the status of the RvSocket connection
// Parameters: none
// Returns   : 0 = Not Connected; 1 = Connected
// ================================================================================================
int RvSocketIsConnected(void)
{
    return Invalid(SocketID)? 0:1;
}
// end of RvSocketIsConnected()

// ================================================================================================
// FUNCTION  : RvSocketSend()
// ------------------------------------------------------------------------------------------------
// Purpose   : Send a message on a socket connection
// Parameters: Buffer -- Frame data buffer
//             Length -- Length of the message (bytes)
// ================================================================================================
RvStatus_t RvSocketSend(char *Buffer, int32_t Length, int32_t *pSentLength)
{
    int status = send(SocketID, Buffer, Length, 0);
    if(pSentLength) *pSentLength = status;

    return (status == -1)? RVWAL_ERROR : RVWAL_SUCCESS;
}
// end of RvSocketSend()

// ================================================================================================
// FUNCTION  : RvSocketReceive()
// ------------------------------------------------------------------------------------------------
// Purpose   : Receive a message from a socket connection
// Parameters: Buffer -- Frame data buffer
//             Length -- Length of the buffer
// ================================================================================================
RvStatus_t RvSocketReceive(char *Buffer, int32_t Length, int32_t *pReceiveLength)
{
    int ReceiveLength;

    // clear the input buffer
    //
    memset(Buffer, 0, Length);

    // receive the RVWAL frame
    //
    ReceiveLength = recv(SocketID, Buffer, Length, 0);
    if(pReceiveLength) *pReceiveLength = ReceiveLength;
    
    return ( ReceiveLength > 0) ? RVWAL_SUCCESS : RVWAL_ERROR;
}
// end of RvSocketReceive()

// ================================================================================================
// FUNCTION  : RvSocketReceiveRemainder()
// ------------------------------------------------------------------------------------------------
// Purpose   : Receive the remainder of a fragmented frame
// Parameters: Buffer         -- Frame data buffer
//             Length         -- Length of the total frame (must not exceed Buffer length)
//             pReceiveLength -- Number of bytes received so far (input and output)
// ================================================================================================
RvStatus_t RvSocketReceiveRemainder(char *Buffer, int32_t Length, int32_t *pReceiveLength)
{
    int FragmentLength;

    while( *pReceiveLength < Length )
    {
        FragmentLength = recv(SocketID, Buffer+(*pReceiveLength), Length-(*pReceiveLength), 0);

        if(FragmentLength > 0)
            *pReceiveLength += FragmentLength;
        else
            return RVWAL_ERROR;

        VPRINTF(">>> Received FragmentLength = %d, ReceiveLength = %d -----------------\n",
                 FragmentLength, *pReceiveLength);
    }
    return RVWAL_SUCCESS;
}
// end of RvSocketReceiveRemainder()

//-------------------------------------------------------------------------------------------------
//--- End of File ---------------------------------------------------------------------------------
