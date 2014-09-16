#ifndef _CCU_H
#define _CCU_H

#include <common.h>

#ifdef CONFIG_HAVE_CCU
extern void ccu_barrier(void);
#else
#define ccu_barrier()   do { } while(0)
#endif

#endif

