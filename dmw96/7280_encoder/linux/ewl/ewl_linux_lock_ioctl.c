/*----------------------------------------------------------------------------
--
--  Description : Locking semaphore for hardware sharing using ioctl to the driver
--
------------------------------------------------------------------------------*/

#include <sys/ipc.h>
//#include <sys/sem.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "hx170dec.h"
#include "hx280enc.h"
#include "ewl_linux_lock_ioctl.h"

/* Wait on a binary semaphore.  Block until the semaphore value is
   positive, then decrement it by one.  */

int binary_semaphore_wait(int fd, int sem_num)
{
	int ret = 0;
	unsigned long ioc_num;

	if (sem_num == 0) ioc_num = HX280ENC_IOC_ENC_LOCK;  // lock the encoder (using the encoder device file)
	else ioc_num = HX170DEC_IOC_PP_LOCK;                // lock the post-processor (using the decoder device file)

	do {
		ret = ioctl(fd, ioc_num, 0);
	} while (ret != 0);

	return 0;
}

/* Post to a binary semaphore: increment its value by one.  This
   returns immediately.  */

int binary_semaphore_post(int fd, int sem_num)
{
	unsigned long ioc_num;

	if (sem_num == 0) ioc_num = HX280ENC_IOC_ENC_UNLOCK;    // release the encoder (using the encoder device file)
	else ioc_num = HX170DEC_IOC_PP_UNLOCK;                  // release the post-processor (using the decoder device file)

	return ioctl(fd, ioc_num, 0);
}

/* Initialize a binary semaphore with a value of one.
   This is happen only on the first call */
int binary_semaphore_initialize(int dec_fd)
{
	return ioctl(dec_fd, HX170DEC_IOC_INIT_LOCKS, 0);
}
