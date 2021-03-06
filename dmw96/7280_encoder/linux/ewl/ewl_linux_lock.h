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
--  Description : Locking semaphore for hardware sharing
--
------------------------------------------------------------------------------
--
--  Version control information, please leave untouched.
--
--  $RCSfile: ewl_linux_lock.h,v $
--  $Revision: 1.1 $
--  $Date: 2007/07/16 09:27:54 $
--
------------------------------------------------------------------------------*/

#ifndef __EWL_LINUX_LOCK_H__
#define __EWL_LINUX_LOCK_H__

#include <sys/types.h>
#include <sys/ipc.h>

int binary_semaphore_allocation(key_t key, int nsem, int sem_flags);
int binary_semaphore_deallocate(int semid);
int binary_semaphore_wait(int semid, int sem_num);
int binary_semaphore_post(int semid, int sem_num);
int binary_semaphore_initialize(int semid, int sem_num);

#endif /* __EWL_LINUX_LOCK_H__ */
