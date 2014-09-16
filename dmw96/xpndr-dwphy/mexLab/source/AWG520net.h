#ifndef __AWG520NET_H
#define __AWG520NET_H


#if defined(__cplusplus) || defined(__cplusplus__)
    extern "C" {
#endif

#include "mex.h"

int  openAWG520(char *hostname);
void closeAWG520();
int  writeAWG520(char *buffer);
int  writeAWG520Binary(char *buffer, unsigned int length);
int  readAWG520(char *buffer, int len);
int  writeAWG520_PatternFile(int N, unsigned int *u);


//---------------------------------------------------------------------------------------
#if defined(__cplusplus) || defined(__cplusplus__)
    }
#endif


#endif
