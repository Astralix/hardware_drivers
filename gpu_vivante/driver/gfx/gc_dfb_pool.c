/****************************************************************************
*
*    Copyright (c) 2005 - 2012 by Vivante Corp.  All rights reserved.
*
*    The material in this file is confidential and contains trade secrets
*    of Vivante Corporation. This is proprietary information owned by
*    Vivante Corporation. No part of this work may be disclosed,
*    reproduced, copied, transmitted, or used in any way for any purpose,
*    without the express written permission of Vivante Corporation.
*
*****************************************************************************/


/**
 * \file gc_dfb_pool.c
 */

#include <asm/types.h>

#include <direct/debug.h>
#include <direct/mem.h>

#include <core/gfxcard.h>
#include <core/surface_pool.h>

#include <gfx/convert.h>

#include "gc_dfb.h"
#include "gc_dfb_utils.h"

D_DEBUG_DOMAIN( Gal_Pool, "Gal/Pool", "Surface pool management" );

typedef struct {
    int magic;
} GalPoolData;

typedef struct {
    int magic;

    CoreDFB *core;
} GalPoolLocalData;

typedef struct {
    int magic;

    int offset;
    int pitch;
    int size;

    /* GAL native surface. */
    gcoSURF surf;
} GalAllocationData;

static int
galPoolDataSize()
{
    return sizeof( GalPoolData );
}

static int
galPoolLocalDataSize()
{
    return sizeof( GalPoolLocalData );
}

static int
galAllocationDataSize()
{
    return sizeof( GalAllocationData );
}

static DFBResult
galInitPool( CoreDFB                    *core,
             CoreSurfacePool            *pool,
             void                       *pool_data,
             void                       *pool_local,
             void                       *system_data,
             CoreSurfacePoolDescription *ret_desc )
{
    GalPoolData       *data = pool_data;
    GalPoolLocalData *local = pool_local;

    D_ASSERT( core       != NULL );
    D_ASSERT( pool       != NULL );
    D_ASSERT( pool_data  != NULL );
    D_ASSERT( pool_local != NULL );
    D_ASSERT( ret_desc   != NULL );

    D_MAGIC_ASSERT( pool, CoreSurfacePool );

    D_DEBUG_ENTER( Gal_Pool,
                   "core: %p, pool: %p, pool_data: %p, pool_local: %p,system_data: %p, ret_desc: %p\n",
                   core, pool, pool_data, pool_local, system_data, ret_desc );

#if (DIRECTFB_MAJOR_VERSION >= 1) && (DIRECTFB_MINOR_VERSION >= 4)
    ret_desc->caps     = CSPCAPS_NONE;
    ret_desc->types    = CSTF_LAYER | CSTF_WINDOW | CSTF_CURSOR | CSTF_FONT | CSTF_EXTERNAL;
    ret_desc->priority = CSPP_PREFERED;

    ret_desc->access[CSAID_CPU]    = CSAF_READ | CSAF_WRITE;
    ret_desc->access[CSAID_GPU]    = CSAF_READ | CSAF_WRITE;
    ret_desc->access[CSAID_LAYER1] = CSAF_READ | CSAF_WRITE;
#else
    ret_desc->caps     = CSPCAPS_NONE;
    ret_desc->access   = CSAF_CPU_READ | CSAF_CPU_WRITE | CSAF_GPU_READ | CSAF_GPU_WRITE;
    ret_desc->types    = CSTF_LAYER | CSTF_WINDOW | CSTF_CURSOR | CSTF_FONT | CSTF_EXTERNAL;
    ret_desc->priority = CSPP_PREFERED;
#endif

    snprintf( ret_desc->name, DFB_SURFACE_POOL_DESC_NAME_LENGTH, "GAL Surface Pool" );

    local->core = core;

    D_MAGIC_SET( data, GalPoolData );
    D_MAGIC_SET( local, GalPoolLocalData );

    D_DEBUG_EXIT( Gal_Pool, "\n" );

    return DFB_OK;
}

static DFBResult
galJoinPool( CoreDFB         *core,
             CoreSurfacePool *pool,
             void            *pool_data,
             void            *pool_local,
             void            *system_data )
{
    GalPoolLocalData *local = pool_local;

    D_ASSERT( core       != NULL );
    D_ASSERT( pool       != NULL );
    D_ASSERT( pool_data  != NULL );
    D_ASSERT( pool_local != NULL );

    D_MAGIC_ASSERT( pool, CoreSurfacePool );

    D_DEBUG_ENTER( Gal_Pool,
                   "core: %p, pool: %p, pool_data: %p, pool_local: %p,system_data: %p\n",
                   core, pool, pool_data, pool_local, system_data );

    D_MAGIC_SET( local, GalPoolLocalData );

    D_DEBUG_EXIT( Gal_Pool, "\n" );

    return DFB_OK;
}

static DFBResult
galDestroyPool( CoreSurfacePool *pool,
                void            *pool_data,
                void            *pool_local )
{
    GalPoolData      *data  = pool_data;
    GalPoolLocalData *local = pool_local;

    D_ASSERT( pool       != NULL );
    D_ASSERT( pool_data  != NULL );
    D_ASSERT( pool_local != NULL );

    D_MAGIC_ASSERT( pool,  CoreSurfacePool );
    D_MAGIC_ASSERT( data,  GalPoolData );
    D_MAGIC_ASSERT( local, GalPoolLocalData );

    D_DEBUG_ENTER( Gal_Pool,
                   "pool: %p, pool_data: %p, pool_local: %p\n",
                   pool, pool_data, pool_local );

    D_MAGIC_CLEAR( data );
    D_MAGIC_CLEAR( local );

    D_DEBUG_EXIT( Gal_Pool, "\n" );

    return DFB_OK;
}

static DFBResult
galLeavePool( CoreSurfacePool *pool,
              void            *pool_data,
              void            *pool_local )
{
    GalPoolLocalData *local = pool_local;

    D_ASSERT( pool       != NULL );
    D_ASSERT( pool_data  != NULL );
    D_ASSERT( pool_local != NULL );

    D_MAGIC_ASSERT( pool, CoreSurfacePool );
    D_MAGIC_ASSERT( local, GalPoolLocalData );

    D_DEBUG_ENTER( Gal_Pool,
                   "pool: %p, pool_data: %p, pool_local: %p\n",
                   pool, pool_data, pool_local );

    D_MAGIC_CLEAR( local );

    D_DEBUG_EXIT( Gal_Pool, "\n" );

    return DFB_OK;
}

static DFBResult
galTestConfig( CoreSurfacePool         *pool,
               void                    *pool_data,
               void                    *pool_local,
               CoreSurfaceBuffer       *buffer,
               const CoreSurfaceConfig *config )
{
    CoreSurface *surface;

    D_ASSERT( pool       != NULL );
    D_ASSERT( pool_data  != NULL );
    D_ASSERT( pool_local != NULL );
    D_ASSERT( buffer     != NULL );
    D_ASSERT( config     != NULL );

    D_MAGIC_ASSERT( pool, CoreSurfacePool );
    D_MAGIC_ASSERT( buffer, CoreSurfaceBuffer );

    D_DEBUG_ENTER( Gal_Pool,
                   "pool: %p, pool_data: %p, pool_local: %p, buffer: %p, config: %p\n",
                   pool, pool_data, pool_local, buffer, config );

    surface = buffer->surface;
    D_MAGIC_ASSERT( surface, CoreSurface );

    D_DEBUG_AT( Gal_Pool,
                "surface->type: 0x%08X, surface->resource_id: %lu\n",
                surface->type, surface->resource_id );

    if ((surface->type & CSTF_LAYER) && surface->resource_id == DLID_PRIMARY)
    {
        D_DEBUG_EXIT( Gal_Pool, "Primary layer is not supported by GAL pool.\n" );

        return DFB_UNSUPPORTED;
    }

    D_DEBUG_EXIT( Gal_Pool, "\n" );

    return DFB_OK;
}

static DFBResult
galAllocateBuffer( CoreSurfacePool       *pool,
                   void                  *pool_data,
                   void                  *pool_local,
                   CoreSurfaceBuffer     *buffer,
                   CoreSurfaceAllocation *allocation,
                   void                  *alloc_data )
{
    CoreSurface       *surface;
    GalAllocationData *alloc  = alloc_data;
    GalDriverData     *vdrv   = dfb_gfxcard_get_driver_data();
    gceSTATUS          status = gcvSTATUS_OK;
    gcoSURF            surf   = NULL;

    gceSURF_FORMAT     format;
    bool               ret;
    unsigned int       aligned_width;
    unsigned int       aligned_height;
    int                aligned_stride;

    D_ASSERT( pool        != NULL );
    D_ASSERT( pool_data   != NULL );
    D_ASSERT( pool_local  != NULL );
    D_ASSERT( allocation  != NULL );
    D_ASSERT( alloc_data  != NULL );
    D_ASSERT( alloc->surf == NULL );

    D_MAGIC_ASSERT( pool, CoreSurfacePool );
    D_MAGIC_ASSERT( buffer, CoreSurfaceBuffer );

    D_DEBUG_ENTER( Gal_Pool,
                   "pool: %p, pool_data: %p, pool_local: %p, buffer: %p, allocation: %p, alloc_data: %p\n",
                   pool, pool_data, pool_local, buffer, allocation, alloc_data );

    do {
        surface = buffer->surface;
        D_MAGIC_ASSERT( surface, CoreSurface );

        if ((surface->type & CSTF_LAYER) && surface->resource_id == DLID_PRIMARY ) {
            D_DEBUG_AT( Gal_Pool, "Primary surface is not supported by GAL pool.\n" );

            status = gcvSTATUS_NOT_SUPPORTED;
            break;
        }

        ret = gal_get_native_format( surface->config.format, &format );
        if (!ret) {
            D_DEBUG_AT( Gal_Pool, "Unsupported color format.\n" );

            status = gcvSTATUS_NOT_SUPPORTED;
            break;
        }

        D_DEBUG_AT( Gal_Pool,
                    "surface->config.size.w: %d\n"
                    "surface->config.size.h: %d\n",
                    surface->config.size.w,
                    surface->config.size.h );

        /* Create a gcoSURF surface. */
        gcmERR_BREAK(gcoSURF_Construct( vdrv->hal,
                                        surface->config.size.w,
                                        surface->config.size.h,
                                        1,
                                        gcvSURF_BITMAP,
                                        format,
                                        gcvPOOL_DEFAULT,
                                        &surf ));

        gcmERR_BREAK(gcoSURF_GetAlignedSize( surf,
                                             &aligned_width,
                                             &aligned_height,
                                             &aligned_stride ));

        D_DEBUG_AT( Gal_Pool,
                    "aligned_width: %u, aligned_height: %u, aligned_stride: %d\n",
                    aligned_width, aligned_height, aligned_stride );


        alloc->surf   = surf;
        alloc->pitch  = aligned_stride;
        alloc->size   = aligned_height * alloc->pitch;
        alloc->offset = 0;

        D_DEBUG_AT( Gal_Pool,
                    "pitch: %d, size: %d\n",
                    alloc->pitch, alloc->size );

        allocation->size   = alloc->size;
        allocation->offset = alloc->offset;

        D_MAGIC_SET( alloc, GalAllocationData );
    } while (0);

    if (gcmIS_ERROR( status )) {
        D_DEBUG_AT( Gal_Pool, "Failed to alloc buffer.\n" );

        if (surf) {
            gcoSURF_Destroy( surf );
        }

        return DFB_FAILURE;
    }

    D_DEBUG_EXIT( Gal_Pool, "\n" );

    return DFB_OK;
}

static DFBResult
galDeallocateBuffer( CoreSurfacePool       *pool,
                     void                  *pool_data,
                     void                  *pool_local,
                     CoreSurfaceBuffer     *buffer,
                     CoreSurfaceAllocation *allocation,
                     void                  *alloc_data )
{
    GalAllocationData *alloc = alloc_data;

    D_ASSERT( pool        != NULL );
    D_ASSERT( pool_data   != NULL );
    D_ASSERT( pool_local  != NULL );
    D_ASSERT( buffer      != NULL );
    D_ASSERT( allocation  != NULL );
    D_ASSERT( alloc_data  != NULL );
    D_ASSERT( alloc->surf != NULL );

    D_MAGIC_ASSERT( pool, CoreSurfacePool );
    D_MAGIC_ASSERT( buffer, CoreSurfaceBuffer );
    D_MAGIC_ASSERT( alloc, GalAllocationData );

    D_DEBUG_ENTER( Gal_Pool,
                   "pool: %p, pool_data: %p, pool_local: %p, buffer: %p, allocation: %p, alloc_data: %p\n",
                   pool, pool_data, pool_local, buffer, allocation, alloc_data );

    /* Destroy the gcoSURF surface. */
    gcmVERIFY_OK(gcoSURF_Destroy( alloc->surf ));
    alloc->surf = NULL;

    D_MAGIC_CLEAR( alloc );

    D_DEBUG_EXIT( Gal_Pool, "\n" );

    return DFB_OK;
}

static DFBResult
galLock( CoreSurfacePool       *pool,
         void                  *pool_data,
         void                  *pool_local,
         CoreSurfaceAllocation *allocation,
         void                  *alloc_data,
         CoreSurfaceBufferLock *lock )
{
    GalAllocationData *alloc  = alloc_data;
    gceSTATUS          status = gcvSTATUS_OK;
    GalDeviceData     *vdev   = dfb_gfxcard_get_device_data();

    void              *addr[3];
    unsigned int       phys[3];


    D_ASSERT( pool        != NULL );
    D_ASSERT( pool_data   != NULL );
    D_ASSERT( pool_local  != NULL );
    D_ASSERT( allocation  != NULL );
    D_ASSERT( alloc_data  != NULL );
    D_ASSERT( lock        != NULL );
    D_ASSERT( alloc->surf != NULL );
    D_ASSERT( vdev        != NULL );

    D_MAGIC_ASSERT( pool, CoreSurfacePool );
    D_MAGIC_ASSERT( allocation, CoreSurfaceAllocation );
    D_MAGIC_ASSERT( alloc, GalAllocationData );
    D_MAGIC_ASSERT( lock, CoreSurfaceBufferLock );

    D_DEBUG_ENTER( Gal_Pool,
                   "pool: %p, pool_data: %p, pool_local: %p, allocation: %p, alloc_data: %p, lock: %p\n",
                   pool, pool_data, pool_local, allocation, alloc_data, lock );

    do {
        /* Lock the surface. */
        gcmERR_BREAK(gcoSURF_Lock( alloc->surf, phys, addr ));

        lock->pitch  = alloc->pitch;
        lock->offset = alloc->offset;
        lock->addr   = addr[0];
        if (phys[0] >= vdev->baseAddress) {
            /* It's a virtual address. */
            lock->phys = phys[0] - vdev->baseAddress;
        } else {
            lock->phys = phys[0] + vdev->baseAddress;
        }

        D_DEBUG_AT( Gal_Pool,
                    "lock->offset: %lu, lock->pitch: %d, lock->addr: %p, lock->phys: 0x%08lx\n",
                    lock->offset, lock->pitch, lock->addr, lock->phys );
    } while (0);

    if (gcmIS_ERROR( status )) {
        D_DEBUG_AT( Gal_Pool, "Failed to lock the surface.\n" );
        return DFB_FAILURE;
    }

    D_DEBUG_EXIT( Gal_Pool, "\n" );

    return DFB_OK;
}

static DFBResult
galUnlock( CoreSurfacePool       *pool,
           void                  *pool_data,
           void                  *pool_local,
           CoreSurfaceAllocation *allocation,
           void                  *alloc_data,
           CoreSurfaceBufferLock *lock )
{
    GalAllocationData *alloc  = alloc_data;
    gceSTATUS          status = gcvSTATUS_OK;

    D_ASSERT( alloc       != NULL );
    D_ASSERT( pool        != NULL );
    D_ASSERT( pool_data   != NULL );
    D_ASSERT( pool_local  != NULL );
    D_ASSERT( allocation  != NULL );
    D_ASSERT( alloc_data  != NULL );
    D_ASSERT( lock        != NULL );
    D_ASSERT( alloc->surf != NULL );

    D_MAGIC_ASSERT( pool, CoreSurfacePool );
    D_MAGIC_ASSERT( allocation, CoreSurfaceAllocation );
    D_MAGIC_ASSERT( alloc, GalAllocationData );
    D_MAGIC_ASSERT( lock, CoreSurfaceBufferLock );

    D_DEBUG_ENTER( Gal_Pool,
                   "pool: %p, pool_data: %p, pool_local: %p, allocation: %p, alloc_data: %p, lock: %p\n",
                   pool, pool_data, pool_local, allocation, alloc_data, lock );

    do {
        gcmERR_BREAK(gcoSURF_Unlock( alloc->surf, lock->addr ));
    } while (0);

    if (gcmIS_ERROR( status )) {
        D_DEBUG_AT( Gal_Pool, "Failed to unlock the surface.\n" );
        return DFB_FAILURE;
    }

    D_DEBUG_EXIT( Gal_Pool, "\n" );

    return DFB_OK;
}

const SurfacePoolFuncs galSurfacePoolFuncs = {
    PoolDataSize:       galPoolDataSize,
    PoolLocalDataSize:  galPoolLocalDataSize,
    AllocationDataSize: galAllocationDataSize,

    InitPool:           galInitPool,
    JoinPool:           galJoinPool,
    DestroyPool:        galDestroyPool,
    LeavePool:          galLeavePool,

    TestConfig:         galTestConfig,
    AllocateBuffer:     galAllocateBuffer,
    DeallocateBuffer:   galDeallocateBuffer,

    Lock:               galLock,
    Unlock:             galUnlock
};
