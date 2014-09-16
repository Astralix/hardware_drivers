#ifndef __COMMON_H__
#define __COMMON_H__

#include "types.h"

/* External RAM technology */
#define RAMTYPE_LPDDR2	1
#define RAMTYPE_MDDR	2
#define RAMTYPE_DDR2	3

/* External RAM size (in Giga bits) */
#define RAMSIZE_2Gb		2
#define RAMSIZE_4Gb		4

#define __entry __attribute__((section(".entry")))
#define __noreturn __attribute__((noreturn))

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

#define IO_ADDRESS(a) ((void *)a)

#define writel(v, a)  (*(volatile unsigned long *)(a) = (v))
#define readl(a)      (*(volatile unsigned long *)(a))
#define readb(a)      (*(volatile unsigned char *)(a))

typedef unsigned short u16;
typedef unsigned int u32;

inline void delay(unsigned long loops);

static inline void *ERR_PTR(long err)
{
	return (void *)err;
}

/* right now we only allow -1 as error code */
static inline long IS_ERR(const void *p)
{
	return ((unsigned long)p == (unsigned long)-1);
}

#endif
