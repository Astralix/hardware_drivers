#include "common.h"

/*
 * dummy raise() required by __div0 which implements division in software, gets
 * called on division by zero
 */
int raise(int signal)
{
	while (1);
	return 0;
}

inline void delay (unsigned long loops)
{
	__asm__ volatile (
		"   adds %0, %1, #1\n"
		"1: subs %0, %1, #1\n"
		"   bne  1b"
		:"=r" (loops)
		:"0" (loops)
	);
}
