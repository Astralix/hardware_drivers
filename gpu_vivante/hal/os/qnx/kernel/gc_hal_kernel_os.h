/****************************************************************************
*
*    Copyright (C) 2005 - 2012 by Vivante Corp.
*
*    This program is free software; you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation; either version 2 of the license, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not write to the Free Software
*    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*
*****************************************************************************/





/*
 * Os.h
 *
 *  Created on: Feb 2, 2010
 *      Author: Tarang Vaish
 */

#ifndef __gc_hal_kernel_os_h_
#define __gc_hal_kernel_os_h_

typedef struct
{
    io_msg_t iomsg;
    gcsHAL_INTERFACE iface;
} gcsDRIVER_ARGS;

struct _gckPAGE_USAGE
{
    gctUINT16 pageCount;
};

struct _gckSHM_POOL
{
    gctUINT32 pid;                      /* Pid of the owner of this pool. */
    gctUINT32 freePage;                 /* Index of first free page. */
    gctUINT32 freePageCount;            /* Count of free pages. */
    gctUINT32 pageCount;                /* Number of pages. */
    gctUINT32 pageSize;                 /* Size of each page. */
    gctUINT32 poolSize;                 /* Size of this pool. */
    pthread_mutex_t mutex;              /* Mutex. */
    gctUINT32 Logical;                  /* Logical base address for galcore/user. */
    gctUINT32 Physical;                 /* Physical base address. */
    struct _gckPAGE_USAGE* pageUsage;   /* List of pageUsage arrays. */
    struct _gckSHM_POOL* nextPool;      /* Pointer to next pool. */
    struct _gckSHM_POOL* nextPoolPid;   /* Pointer to next pool for the same Pid. */
};

typedef struct _gckSHM_POOL* gckSHM_POOL;
typedef struct _gckPAGE_USAGE* gckPAGE_USAGE;

gceSTATUS
drv_mempool_init();

void
drv_mempool_destroy();

void
drv_mempool_alloc_contiguous(
    IN gctUINT32 Bytes,
    OUT gctPHYS_ADDR * Physical,
    OUT gctPOINTER * Logical
    );

int
drv_mempool_free(
    IN gctPOINTER Logical
    );

gctUINT32
drv_mempool_get_baseAddress();

gctUINT32
drv_mempool_get_basePAddress();

gctUINT32
drv_mempool_get_page_size();

gctINT
drv_mempool_get_fileDescriptor();

gceSTATUS
drv_mempool_mem_offset(
    IN gctPOINTER Logical,
    OUT gctUINT32 * Address);

/* Shared memory pool functions. */
gckSHM_POOL drv_shmpool_create(
    IN gctUINT32 Pid,
    IN gctUINT32 PoolSize,
    IN gctUINT32 PageSize);
void
drv_shmpool_destroy(
    IN gckSHM_POOL ShmPool
    );

gckSHM_POOL
drv_shm_acquire_pool(
    IN gctUINT32 Pid
    );

gckSHM_POOL
drv_shm_acquire_pool2(
    IN gctPOINTER Logical
    );

gceSTATUS
drv_shm_remove_pool(
    IN gctUINT32 Pid
    );

gceSTATUS
drv_shmpool_mem_offset(
    IN gctUINT32 Pid,
    IN gctPOINTER Logical,
    OUT gctUINT32 * Address);

gceSTATUS
drv_shmpool_mem_map(
    IN gctUINT32 Pid,
    IN gctUINT32 Address,
    OUT gctPOINTER* Logical);

gctPOINTER
drv_shmpool_alloc_contiguous(
    IN gctUINT32 Pid,
    IN gctUINT32 Bytes
    );

gctUINT32
drv_shmpool_free(
    IN gctPOINTER Logical
    );

gctUINT32 drv_get_user_pid( void );

gctUINT32
drv_get_user_pid( void );

gctUINT32
drv_get_user_tid( void );

int drv_thread_specific_key_assign(
    gctUINT32 Pid,
    gctUINT32 Tid
    );

void *
mmap64_join(pid_t pid, void *addr, size_t len, int prot, int flags, int fd, off64_t off);

int
mem_offset64_peer(pid_t pid, const uintptr_t addr, size_t len,
                off64_t *offset, size_t *contig_len);

int
munmap_peer(pid_t pid, void *addr, size_t len);

void *
mmap64_peer(pid_t pid, void *addr, size_t len, int prot, int flags, int fd, off64_t off);

#endif /* __gc_hal_kernel_os_h_ */

