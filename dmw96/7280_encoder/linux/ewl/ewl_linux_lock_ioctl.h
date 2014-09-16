/*----------------------------------------------------------------------------
--
--  Description : Locking semaphore for hardware sharing using ioctl to the driver
--
------------------------------------------------------------------------------
*/

#ifndef __EWL_LINUX_LOCK_IOCTL_H__
#define __EWL_LINUX_LOCK_IOCTL_H__

#include <sys/types.h>
#include <sys/ipc.h>

int binary_semaphore_wait(int fd, int sem_num);
int binary_semaphore_post(int fd, int sem_num);

/* will only initialize one time */
int binary_semaphore_initialize(int dec_fd);

#endif /* __EWL_LINUX_LOCK_IOCTL_H__ */
