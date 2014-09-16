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
#include "dwl_linux_lock_ioctl.h"

/* Wait on a binary semaphore.  Block until the semaphore value is
   positive, then decrement it by one.  */

int binary_semaphore_wait(int dec_fd, int sem_num)
{
	int ret = 0;
	unsigned long ioc_num;

	if (sem_num == 0) ioc_num = HX170DEC_IOC_DEC_LOCK;
	else ioc_num = HX170DEC_IOC_PP_LOCK;

	do {
		ret = ioctl(dec_fd, ioc_num, 0);
	} while (ret != 0);

	return 0;
}

/* Post to a binary semaphore: increment its value by one.  This
   returns immediately.  */

int binary_semaphore_post(int dec_fd, int sem_num)
{
	unsigned long ioc_num;

	if (sem_num == 0) ioc_num = HX170DEC_IOC_DEC_UNLOCK;
	else ioc_num = HX170DEC_IOC_PP_UNLOCK;

	return ioctl(dec_fd, ioc_num, 0);
}

/* Initialize a binary semaphore with a value of one.
   This is happen only on the first call */
int binary_semaphore_initialize(int dec_fd)
{
	return ioctl(dec_fd, HX170DEC_IOC_INIT_LOCKS, 0);
}
