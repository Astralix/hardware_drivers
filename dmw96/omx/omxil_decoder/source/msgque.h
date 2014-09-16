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
--  Description :  OpenMAX message queue
--
------------------------------------------------------------------------------*/


#ifndef HANTRO_MSGQUE_H
#define HANTRO_MSGQUE_H

#include <OMX_Types.h>
#include <OMX_Core.h>
#include "OSAL.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct msg_node msg_node;

struct msg_node
{
    msg_node* next;
    msg_node* prev;
    OMX_PTR   data;
};

typedef struct msgque
{ 
    msg_node*      head;
    msg_node*      tail;
    OMX_U32        size;
    OMX_HANDLETYPE mutex; 
    OMX_HANDLETYPE event;
} msgque;


// Initialize a new message queue instance 
OMX_ERRORTYPE HantroOmx_msgque_init(OMX_IN msgque* q);

// Destroy the message queue instance, free allocated resources
void HantroOmx_msgque_destroy(OMX_IN msgque* q);


// Push a new message at the end of the queue. 
// Function provides commit/rollback semantics.
OMX_ERRORTYPE HantroOmx_msgque_push_back(OMX_IN msgque* q, OMX_IN OMX_PTR ptr);

// Get a message from the front, returns always immediately but 
// ptr will point to NULL if the queue is empty. 
// Function provides commit/rollback semantics.
OMX_ERRORTYPE HantroOmx_msgque_get_front(OMX_IN msgque* q, OMX_OUT OMX_PTR* ptr);

// Get current queue size
OMX_ERRORTYPE HantroOmx_msgque_get_size(OMX_IN msgque* q, OMX_OUT OMX_U32* size);



#ifdef __cplusplus
}
#endif
#endif // HANTRO_MSGQUE_H


