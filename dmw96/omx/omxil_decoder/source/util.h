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
--  Description :  Utility functions
--
------------------------------------------------------------------------------*/


#ifndef HANTRO_UTIL_H
#define HANTRO_UTIL_H

#include <OMX_Types.h>
#include <OMX_Core.h>
#include <OMX_Component.h>

// Qm.n conversion macros
#define Q16_FLOAT(a) ((float)(a) / 65536.0)
#define FLOAT_Q16(a) ((OMX_U32) ((float)(a) * 65536.0))

#ifdef __cplusplus
extern "C" {
#endif

const char* HantroOmx_str_omx_state(OMX_STATETYPE s);
const char* HantroOmx_str_omx_err(OMX_ERRORTYPE e);
const char* HantroOmx_str_omx_cmd(OMX_COMMANDTYPE c);
const char* HantroOmx_str_omx_event(OMX_EVENTTYPE e);
const char* HantroOmx_str_omx_index(OMX_INDEXTYPE i);
const char* HantroOmx_str_omx_color(OMX_COLOR_FORMATTYPE f);
const char* HantroOmx_str_omx_supplier(OMX_BUFFERSUPPLIERTYPE bst);

OMX_U32 HantroOmx_make_int_ver(OMX_U8 major, OMX_U8 minor);

#ifdef __cplusplus
}
#endif
#endif // HANTRO_UTIL_H
