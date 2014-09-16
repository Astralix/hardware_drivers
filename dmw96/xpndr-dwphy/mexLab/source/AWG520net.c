#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <winsock2.h>
#include <errno.h>
#include "AWG520net.h"
#include "mex.h"

/*===========================================================================*/
/* global and static global variables and functions                          */
/*===========================================================================*/

static int InstrumentSocket;    /* File descriptor to open socket connection */
static char textbuffer[16*1024];          /* Circular read buffer (16 kbytes) */
static int textbuflen = 0;
static char *textptr = textbuffer;

static int getMoreDataIfNeeded();

/*==========================================================================*/
/*--  INTERNAL ERROR HANDLING                                               */
/*==========================================================================*/
#define DEBUG  mexPrintf("### FILE: %s, LINE: %d\n",__FILE__,__LINE__);


/*==========================================================================*/
/* external variables                                                       */
/*==========================================================================*/


/*===========================================================================
 *
 * Function:     read
 * Description:  BSD Unix sockets functions renamed to match Windows
 *               
 *==========================================================================*/
static int read(SOCKET s, char FAR *buf, int len)
{
    int r = recv(s, buf, len, 0);
    return r;
}

static int getMoreDataIfNeeded()
{
  if (textbuflen <= 0)
  {
    if ((textbuflen=read(InstrumentSocket,textbuffer,sizeof(textbuffer))) <= 0)
    {
        return(-1);
    }
    textptr = textbuffer;
  }
  return 0;
}

int openAWG520(char *hostname)
{

  struct hostent *hostEntry;
  struct sockaddr_in sockaddr;
  int tmp = 1;

{
WORD wVersionRequested;
WSADATA wsaData;
int err;
 
wVersionRequested = MAKEWORD( 2, 2 );
 
err = WSAStartup( wVersionRequested, &wsaData );
if ( err != 0 ) {
    mexPrintf("Unable to find a usable WinSock DLL\n");
    return -1;
}
 }

  if ((hostEntry=gethostbyname(hostname)) == NULL)
  {
    mexPrintf("%s:%d>Unknown host %s\n",__FILE__,__LINE__,hostname);
    return(-1);
  }
  InstrumentSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (InstrumentSocket < 0)
  {
        mexPrintf("Call to socket() failed");
        perror("Call to socket() failed");
        return(-1);
  }
  sockaddr.sin_family = AF_INET;
  sockaddr.sin_port   = htons(4000);
  sockaddr.sin_addr.s_addr = *((unsigned long*)(hostEntry->h_addr_list[0]));
  if (connect(InstrumentSocket, &sockaddr, sizeof(sockaddr)) < 0)
  {
    mexPrintf("Connect Failed: WSAGetLastError = %d\n",WSAGetLastError());
    perror("Connect failed");
    return(-1);
  }

  if (setsockopt(InstrumentSocket, IPPROTO_TCP, TCP_NODELAY, &tmp, sizeof(tmp)))
  {
    mexPrintf("setsockopt(TCP_NODELAY) failed");
    perror("setsockopt(TCP_NODELAY) failed");
  }

  return(0);
}



void closeAWG520()
{
  if (InstrumentSocket < 0)
  {
    mexPrintf("closeConnection: no socket connection to close!\n");
    return;
  }
  closesocket(InstrumentSocket);
}


int readAWG520(char *line, int len)
{
    int i=0;

    if (InstrumentSocket < 0)
    {
        mexPrintf("getLine: no open socket connection!\n");
        return -1;
    }

    if (getMoreDataIfNeeded() < 0) return -2;

    while ((textbuflen > 0) && (i < len-1) && (*textptr != '\r') && (*textptr != '\n'))
    {
        line[i] = *textptr++;
        textbuflen--;
        i++;
    }

    line[i] = 0;
    if (i < len-1)
    {
        textptr++;
        textbuflen--;
        if (textbuflen >= 1 && (*textptr == '\r' || *textptr == '\n'))
        {
            textptr++;
            textbuflen--;
        }
        return(i);
    }
    else
    {
        return(-3);
    }
}


int writeAWG520(char *str)
{
  int todo, wrote;
  char localcommand[1024];
  int len;

  if (InstrumentSocket < 0)
  {
    mexPrintf("putLine: no active connection!\n");
    return(-1);
  }

  len = strlen(str);
  strcpy(localcommand,str);
  if (localcommand[len-1] != '\n')
  {
    localcommand[len] = '\n';
    len++;
    localcommand[len] = 0;
  }

  todo = strlen(localcommand);
  wrote = send(InstrumentSocket,localcommand,todo, 0);
  if (wrote != todo)
  {
    mexPrintf("Partial string write to socket: %d of %d bytes!\n",
                wrote, todo);
    return(-1);
  }

  return(0);
}
