#ifndef PNX8181_UDC_H
#define PNX8181_UDC_H

/*
 * Save the current interrupt enable state & disable IRQs
 */
#define local_irq_save(x)					\
	({							\
		unsigned long temp;				\
		(void) (&temp == &x);				\
	__asm__ __volatile__(					\
	"mrs	%0, cpsr		@ local_irq_save\n"	\
"	orr	%1, %0, #128\n"					\
"	msr	cpsr_c, %1"					\
	: "=r" (x), "=r" (temp)					\
	:							\
	: "memory", "cc");					\
	})

/*
 * restore saved IRQ & FIQ state
 */
#define local_irq_restore(x)					\
	__asm__ __volatile__(					\
	"msr	cpsr_c, %0		@ local_irq_restore\n"	\
	:							\
	: "r" (x)						\
	: "memory", "cc")

/*
 * Enable IRQs
 */
#define local_irq_enable()					\
({								\
	unsigned long temp;					\
	__asm__ __volatile__(					\
	"mrs	%0, cpsr		@ local_irq_enable\n"	\
	"bic	%0, %0, #128\n"					\
	"msr	cpsr_c, %0"					\
	: "=r" (temp)						\
	:							\
	: "memory", "cc");					\
})

/*
 * Disable IRQs
 */
#define local_irq_disable()					\
({								\
	unsigned long temp;					\
	__asm__ __volatile__(					\
	"mrs	%0, cpsr		@ local_irq_disable\n"	\
	"orr	%0, %0, #128\n"					\
	"msr	cpsr_c, %0"					\
	: "=r" (temp)						\
	:							\
	: "memory", "cc");					\
})

#endif
