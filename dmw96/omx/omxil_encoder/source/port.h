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
--  Description :  Port component
--
------------------------------------------------------------------------------*/


#ifndef HANTRO_PORT_H
#define HANTRO_PORT_H

#include <OMX_Types.h>
#include <OMX_Component.h>
#include <OMX_Video.h>
#include "OSAL.h"

#ifdef __cplusplus
extern "C" {
#endif 

#define BUFFER_FLAG_IN_USE      0x1
#define BUFFER_FLAG_MY_BUFFER   0x2
#define BUFFER_FLAG_IS_TUNNELED 0x4
#define BUFFER_FLAG_MARK        0x8

typedef struct BUFFER
{
    OMX_BUFFERHEADERTYPE* header;
    OMX_BUFFERHEADERTYPE  headerdata;
    OMX_U32               flags;
    OMX_U32               allocsize;
    OSAL_BUS_WIDTH        bus_address;
    OMX_U8*               bus_data;
} BUFFER;


typedef struct BUFFERLIST
{
    BUFFER** list;
    OMX_U32  size; // list size
    OMX_U32  capacity;
}BUFFERLIST;

OMX_ERRORTYPE HantroOmx_bufferlist_init(BUFFERLIST* list, OMX_U32 size);
OMX_ERRORTYPE HantroOmx_bufferlist_reserve(BUFFERLIST* list, OMX_U32 newsize);
void          HantroOmx_bufferlist_destroy(BUFFERLIST* list);
OMX_U32       HantroOmx_bufferlist_get_size(BUFFERLIST* list);
OMX_U32       HantroOmx_bufferlist_get_capacity(BUFFERLIST* list);
BUFFER**      HantroOmx_bufferlist_at(BUFFERLIST* list, OMX_U32 i);
void          HantroOmx_bufferlist_remove(BUFFERLIST* list, OMX_U32 i);
void          HantroOmx_bufferlist_clear(BUFFERLIST* list);
OMX_BOOL      HantroOmx_bufferlist_push_back(BUFFERLIST* list, BUFFER* buff);



typedef struct PORT
{
    OMX_PARAM_PORTDEFINITIONTYPE def;          // OMX port definition
    OMX_TUNNELSETUPTYPE          tunnel;
    OMX_HANDLETYPE               tunnelcomp;   // handle to the tunneled component
    OMX_U32                      tunnelport;   // port index of the tunneled components port
    BUFFERLIST                   buffers;      // buffers for this port
    BUFFERLIST                   bufferqueue;  // buffers queued up for processing
    OMX_HANDLETYPE               buffermutex;  // mutex to protect the buffer queue
    OMX_HANDLETYPE               bufferevent;  // event object for buffer queue
} PORT;


// nBufferCountMin is a read-only field that specifies the minimum number of
// buffers that the port requires. The component shall define this non-zero default
// value.

// nBufferCountActual represents the number of buffers that are required on
// this port before it is populated, as indicated by the bPopulated field of this
// structure. The component shall set a default value no less than
// nBufferCountMin for this field.

// nBufferSize is a read-only field that specifies the minimum size in bytes for
// buffers that are allocated for this port. .
OMX_ERRORTYPE HantroOmx_port_init(PORT* p, OMX_U32 nBufferCountMin, OMX_U32 nBufferCountActual, OMX_U32 nBuffers, OMX_U32 buffersize);

void     HantroOmx_port_destroy(PORT* p);

OMX_BOOL HantroOmx_port_is_allocated(PORT* p);

OMX_BOOL HantroOmx_port_is_ready(PORT* p);

OMX_BOOL HantroOmx_port_is_enabled(PORT* p);

// Return true if port has allocated buffers, otherwise false.
OMX_BOOL HantroOmx_port_has_buffers(PORT* p);

OMX_BOOL HantroOmx_port_is_supplier(PORT* p);
OMX_BOOL HantroOmx_port_is_tunneled(PORT* p);

OMX_BOOL HantroOmx_port_has_all_supplied_buffers(PORT* p);

void     HantroOmx_port_setup_tunnel(PORT* p, OMX_HANDLETYPE comp, OMX_U32 port, OMX_BUFFERSUPPLIERTYPE type);


BUFFER*  HantroOmx_port_find_buffer(PORT* p, OMX_BUFFERHEADERTYPE* header);


// Try to allocate next available buffer from the array of buffers associated with
// with the port. The finding is done by looking at the associated buffer flags and
// checking the BUFFER_FLAG_IN_USE flag.
//
// Returns OMX_TRUE if next buffer could be found. Otherwise OMX_FALSE, which
// means that all buffer headers are in use. 
OMX_BOOL HantroOmx_port_allocate_next_buffer(PORT* p, BUFFER** buff);



// 
OMX_BOOL HantroOmx_port_release_buffer(PORT* p, BUFFER* buff);

//
OMX_BOOL HantroOmx_port_release_all_allocated(PORT* p);


// Return how many buffers are allocated for this port.
OMX_U32 HantroOmx_port_buffer_count(PORT* p);


// Get an allocated buffer. 
OMX_BOOL HantroOmx_port_get_allocated_buffer_at(PORT* p, BUFFER** buff, OMX_U32 i);


/// queue functions


// Push next buffer into the port's buffer queue. 
OMX_ERRORTYPE HantroOmx_port_push_buffer(PORT* p, BUFFER* buff);


// Get next buffer from the port's buffer queue. 
OMX_BOOL HantroOmx_port_get_buffer(PORT* p, BUFFER** buff);


// Get a buffer at a certain location from port's buffer queue
OMX_BOOL HantroOmx_port_get_buffer_at(PORT* P, BUFFER** buff, OMX_U32 i);


// Pop off the first buffer from the port's buffer queue
OMX_BOOL      HantroOmx_port_pop_buffer(PORT* p);

// Lock the buffer queue
OMX_ERRORTYPE HantroOmx_port_lock_buffers(PORT* p);
// Unlock the buffer queue
OMX_ERRORTYPE HantroOmx_port_unlock_buffers(PORT* p);


// Return how many buffers are queued for this port.
OMX_U32 HantroOmx_port_buffer_queue_count(PORT* p);

void HantroOmx_port_buffer_queue_clear(PORT* p);

#ifdef __cplusplus
} 
#endif
#endif // HANTRO_PORT_H
