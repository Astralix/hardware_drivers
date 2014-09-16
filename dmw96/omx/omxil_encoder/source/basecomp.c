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
--  Description :  Basecomponent contains code for component thread handling
--                 and message sending.
--
------------------------------------------------------------------------------*/


#include "basecomp.h"
#include <assert.h>
#include <string.h>

typedef struct thread_param
{
    BASECOMP*       self;
    OMX_PTR         arg;
    thread_main_fun fun;
} thread_param;

static
OMX_U32 HantroOmx_basecomp_thread_main(void* unsafe)
{
    thread_param* param = (thread_param*)(unsafe);

    OMX_U32 ret = param->fun(param->self, param->arg);
    OSAL_Free(param);
    return ret;
}


OMX_ERRORTYPE HantroOmx_basecomp_init(BASECOMP* b, thread_main_fun f, OMX_PTR arg)
{
    assert(b);
    assert(f);
    memset(b, 0, sizeof(BASECOMP));

    OMX_ERRORTYPE err = HantroOmx_msgque_init(&b->queue);
    if (err != OMX_ErrorNone)
        return err;
      
    thread_param* param = (thread_param*)OSAL_Malloc(sizeof(thread_param));
    if (!param)
    {
        err = OMX_ErrorInsufficientResources;
        goto FAIL;
    }

    param->self = b;
    param->arg  = arg;
    param->fun  = f;

    err = OSAL_ThreadCreate(HantroOmx_basecomp_thread_main, param, 0, &b->thread);
    if (err != OMX_ErrorNone)
        goto FAIL;

    return OMX_ErrorNone;

 FAIL:
    HantroOmx_msgque_destroy(&b->queue);
    if (param)
        OSAL_Free(param);
    return err;
}


OMX_ERRORTYPE HantroOmx_basecomp_destroy(BASECOMP* b)
{
    assert(b);
    assert(b->thread);
    OMX_ERRORTYPE err = OSAL_ThreadDestroy(b->thread);
    assert(err == OMX_ErrorNone);
    HantroOmx_msgque_destroy(&b->queue);
    memset(b, 0, sizeof(BASECOMP));
    return OMX_ErrorNone;
}

OMX_ERRORTYPE HantroOmx_basecomp_send_command(BASECOMP* b, CMD* c)
{
    assert(b && c);
    
    CMD* ptr = (CMD*)OSAL_Malloc(sizeof(CMD));
    if (!ptr)
        return OMX_ErrorInsufficientResources;
    
    memcpy(ptr, c, sizeof(CMD));
    
    OMX_ERRORTYPE err = HantroOmx_msgque_push_back(&b->queue, ptr);
    if (err != OMX_ErrorNone)
    {
        OSAL_Free(ptr);
        return err;
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE HantroOmx_basecomp_recv_command(BASECOMP* b, CMD* c)
{
    assert(b && c);
    
    void* unsafe = 0;
    OMX_ERRORTYPE err = HantroOmx_msgque_get_front(&b->queue, &unsafe);
    if (err != OMX_ErrorNone)
        return err;
    
    assert(unsafe);
    CMD* ptr = (CMD*)unsafe;
    memcpy(c, ptr, sizeof(CMD));
    OSAL_Free(ptr);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE HantroOmx_basecomp_try_recv_command(BASECOMP* b, CMD* c, OMX_BOOL* ok)
{
    assert(b && c);
    void* unsafe = NULL;
    OMX_ERRORTYPE err = HantroOmx_msgque_get_front(&b->queue, &unsafe);
    if (err != OMX_ErrorNone)
        return err;
    
    *ok = OMX_FALSE;
    if (unsafe)
    {
        CMD* ptr = (CMD*)unsafe;
        memcpy(c, ptr, sizeof(CMD));
        OSAL_Free(ptr);
        *ok = OMX_TRUE;
    }
    return OMX_ErrorNone;
}


void HantroOmx_generate_uuid(OMX_HANDLETYPE comp, OMX_UUIDTYPE* uuid)
{
    assert(comp && uuid);
    
    int i;
    OMX_U32 ival = (OMX_U32)comp;

    for (i=0; i<sizeof(OMX_UUIDTYPE); ++i)
    {
        (*uuid)[i] = ival >> (i % sizeof(OMX_U32));
    }
}


OMX_ERRORTYPE HantroOmx_cmd_dispatch(CMD* cmd, OMX_PTR arg)
{
    assert(cmd);
    OMX_ERRORTYPE err = OMX_ErrorNone;
    switch (cmd->type)
    {
        case CMD_SEND_COMMAND:
            err = cmd->arg.fun(cmd->arg.cmd, cmd->arg.param1, cmd->arg.data, arg);
            break;
        default:
            assert(0);
            break;
    }
    return err;
}



