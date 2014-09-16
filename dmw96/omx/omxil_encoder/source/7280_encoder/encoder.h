/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Hantro Products Oy.                             --
--                                                                            --
--                   (C) COPYRIGHT 2006 HANTRO PRODUCTS OY                    --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Description :  OMX Encoder types
--
------------------------------------------------------------------------------*/

/******************************************************
    Table of contents
   
    1. Include headers
    2. Module defines
    3. Data types
    4. Function prototypes

******************************************************/


#ifndef ENCODER_H
#define ENCODER_H

/******************************************************
  1. Includes 
******************************************************/
#include <OSAL.h>
#include <basecomp.h>
#include <port.h>
#include <util.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <OMX_Core.h> //includes OMX_Component.h


#include "codec.h"

/******************************************************
 2. Module defines 
******************************************************/


/******************************************************
 3. Data types 
******************************************************/

typedef struct FRAME_BUFFER
{
    OSAL_BUS_WIDTH bus_address;
    OMX_U8*        bus_data;
    OMX_U32        capacity;   // buffer size
    OMX_U32        size;       // how many bytes is in the buffer currently
    OMX_U32        offset;
}FRAME_BUFFER;

#ifdef OMX_ENCODER_VIDEO_DOMAIN
typedef struct VIDEO_ENCODER_CONFIG
{
    OMX_VIDEO_PARAM_AVCTYPE             avc;
    OMX_VIDEO_CONFIG_AVCINTRAPERIOD     avcIdr; //Only nIDRPeriod is affecting for codec configuration.
                                                //When nPFrames is changed it's stored here and also in
                                                //OMX_VIDEO_PARAM_AVCTYPE structure above.  
    OMX_PARAM_DEBLOCKINGTYPE            deblocking;
    OMX_VIDEO_PARAM_H263TYPE            h263;
    OMX_VIDEO_PARAM_MPEG4TYPE           mpeg4;
    OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE ec;
    OMX_VIDEO_PARAM_BITRATETYPE         bitrate; //Only eControlRate is affecting for codec configuration!
                                                 //When target bitrate is changed it's stored here and
                                                 //also in output port configuration. PortChanged event
                                                 //must be created when target rate is changed.                                              
    OMX_CONFIG_FRAMESTABTYPE            stab;
    OMX_VIDEO_PARAM_QUANTIZATIONTYPE    videoQuantization;
    OMX_CONFIG_ROTATIONTYPE             rotation;
    OMX_CONFIG_RECTTYPE                 crop;
    OMX_CONFIG_INTRAREFRESHVOPTYPE      intraRefresh;
#ifdef ENCH1
    OMX_VIDEO_PARAM_VP8TYPE             vp8;
    OMX_VIDEO_VP8REFERENCEFRAMETYPE     vp8Ref;
#endif
}VIDEO_ENCODER_CONFIG;
#endif //OMX_ENCODER_VIDEO_DOMAIN

#ifdef OMX_ENCODER_IMAGE_DOMAIN
typedef struct IMAGE_ENCODER_CONFIG
{
    OMX_IMAGE_PARAM_QFACTORTYPE         imageQuantization;
    OMX_CONFIG_ROTATIONTYPE             rotation;
    OMX_CONFIG_RECTTYPE                 crop;   
}IMAGE_ENCODER_CONFIG;
#endif

typedef struct OMX_ENCODER
{
    BASECOMP                        base;
    volatile OMX_STATETYPE          state;
    volatile OMX_STATETYPE          statetrans;
    volatile OMX_BOOL               run;
    OMX_CALLBACKTYPE                app_callbacks;
    OMX_U32                         priority_group;
    OMX_U32                         priority_id;
    OMX_PTR                         app_data;
    PORT                            inputPort;      
    PORT                            outputPort;
    OMX_STRING                      name;
    FRAME_BUFFER                    frame_in;  
    FRAME_BUFFER                    frame_out;
    OMX_PORT_PARAM_TYPE             ports;
    OMX_HANDLETYPE                  self;  
    OSAL_ALLOCATOR                  alloc;
    ENCODER_PROTOTYPE*              codec;
    OMX_BOOL                        streamStarted;
    OMX_U32                         frameSize;
    OMX_U32                         frameCounter;
    OMX_U8                          role[128];
    FILE*                           log;    
    OMX_MARKTYPE                    marks[10];
    OMX_U32                         mark_read_pos;
    OMX_U32                         mark_write_pos;
#ifdef OMX_ENCODER_VIDEO_DOMAIN 
    VIDEO_ENCODER_CONFIG    encConfig;
#endif //OMX_ENCODER_VIDEO_DOMAIN
#ifdef OMX_ENCODER_IMAGE_DOMAIN
    IMAGE_ENCODER_CONFIG    encConfig;  
    OMX_BOOL                sliceMode;  
    OMX_U32                 sliceNum;
    OMX_U32                 numOfSlices;
#ifdef CONFORMANCE
    OMX_IMAGE_PARAM_QUANTIZATIONTABLETYPE quant_table;
#endif
#endif //OMX_ENCODER_IMAGE_DOMAIN
    
} OMX_ENCODER;

/******************************************************
 4. Function prototypes 
******************************************************/
OMX_ERRORTYPE HantroHwEncOmx_encoder_init( OMX_HANDLETYPE hComponent );

#endif //~ENCODER_H
