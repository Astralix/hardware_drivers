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
--  Description :  message queue
--
------------------------------------------------------------------------------*/


#include "msgque.h"
#include <assert.h>

OMX_ERRORTYPE HantroOmx_msgque_init(OMX_IN msgque* q)
{
    assert(q);
    q->head = 0;
    q->tail = 0;
    q->size = 0;
    
    OMX_ERRORTYPE err = OMX_ErrorNone;
    err = OSAL_MutexCreate(&q->mutex);
    if (err != OMX_ErrorNone)
        return err;
    
    err = OSAL_EventCreate(&q->event);
    if (err != OMX_ErrorNone)
        OSAL_MutexDestroy(q->mutex);

    return err;
}

void HantroOmx_msgque_destroy(OMX_IN msgque* q)
{
    assert(q);
    OMX_ERRORTYPE err = OMX_ErrorNone;

    err = OSAL_MutexLock(q->mutex);
    assert(err == OMX_ErrorNone);

    while (q->tail)
    {
        msg_node* next = q->tail->next;
        OSAL_Free(q->tail);
        q->tail = next;
    }

    err = OSAL_MutexUnlock(q->mutex);  assert(err == OMX_ErrorNone);
    err = OSAL_MutexDestroy(q->mutex); assert(err == OMX_ErrorNone);
    err = OSAL_EventDestroy(q->event); assert(err == OMX_ErrorNone);
}

OMX_ERRORTYPE HantroOmx_msgque_push_back(OMX_IN msgque* q, OMX_IN OMX_PTR ptr)
{ 
    assert(q);
    assert(ptr);
    
    msg_node* tail = (msg_node*)OSAL_Malloc(sizeof(msg_node));
    if(!tail)
        return OMX_ErrorInsufficientResources;
    
    tail->next = q->tail;
    tail->prev = 0;
    tail->data = ptr;
    
    // get mutex now
    OMX_ERRORTYPE err = OMX_ErrorNone;
    err = OSAL_MutexLock(q->mutex);
    if (err != OMX_ErrorNone)
    {
        OSAL_Free(tail);
        return err;
    }

    // first set the signal if needed and once that is allright
    // only then change the queue, cause that cant fail
    
    if (q->size == 0)
    {
        err = OSAL_EventSet(q->event);
        if (err != OMX_ErrorNone)
        {
            OSAL_MutexUnlock(q->mutex);
            return err;
        }
    }
    
    q->size += 1;
    if (q->tail)
        q->tail->prev = tail;
    q->tail  = tail;
    if (!q->head)
        q->head = q->tail;

    err = OSAL_MutexUnlock(q->mutex); 
    assert(err == OMX_ErrorNone);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE HantroOmx_msgque_get_front(OMX_IN msgque* q, OMX_OUT OMX_PTR* ptr)
{
    assert(q);
    assert(ptr);

    // get mutex now
    OMX_ERRORTYPE err = OMX_ErrorNone;
    err = OSAL_MutexLock(q->mutex);
    if (err != OMX_ErrorNone)
        return err;

    if (q->size - 1 == 0)
    {
        // reset the signal to not set
        err = OSAL_EventReset(q->event);
        if (err != OMX_ErrorNone)
        {
            OSAL_MutexUnlock(q->mutex);
            return err;
        }
    }
    if (q->size == 0)
    {
        assert(q->head == 0);
        assert(q->tail == 0);
        *ptr = 0;
    }
    else
    {
        msg_node* head = q->head;
        assert(head->next == 0);
        *ptr = head->data;
        q->head  = head->prev;
        q->size -= 1;
        if (q->head)
            q->head->next = 0;
        else
            q->tail = 0;

        OSAL_Free(head);
    }
    err =  OSAL_MutexUnlock(q->mutex); assert(err == OMX_ErrorNone);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE HantroOmx_msgque_get_size(OMX_IN msgque* q, OMX_OUT OMX_U32* size)
{
    assert(q);
    assert(size);
    
    OMX_ERRORTYPE err = OMX_ErrorNone;
    err = OSAL_MutexLock(q->mutex);
    if (err != OMX_ErrorNone)
        return err;
    
    *size = q->size;
    err = OSAL_MutexUnlock(q->mutex);
    assert(err == OMX_ErrorNone);

    return OMX_ErrorNone;
}

