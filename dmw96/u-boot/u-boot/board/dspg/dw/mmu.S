#include <config.h>
#include <common.h>
#include <version.h>

.equ C1_Base		, 0x00c5287a @0x00050078
.equ C1_V_Bit		, 13
.equ C1_C_Bit		, 2
.equ C1_M_Bit		, 0

.equ C1_MMU_ON_CACHE_ON_V_HIGH ,	(C1_Base |		\
					 (1 << C1_M_Bit) |	\
					 (1 << C1_C_Bit) |	\
					 (1 << C1_V_Bit))

.globl enable_mmu
enable_mmu:
	mov	r0, #0x3			@ set access for domain 0 to client
	mcr	p15, 0, r0, c3, c0, 0		@ write domain access register

	@ enable MMU with page table from DDR
	ldr	r0, =mmu_page_table
        @ coprocessor 2, additional coprocessor register name 0, opcode 0
	mcr	p15, 0, r0, c2, c0, 0			@ set TTBR to DDR RAM
	ldr	r0, =C1_MMU_ON_CACHE_ON_V_HIGH
	mcr	p15, 0, r0, c1, c0, 0			@ mmu on, cache off, vectors high

#if 0
	mov	r0, #0
	mcr	p15, 0, r0, c7, c10, 4	@ drain write buffer
	mcr	p15, 0, r0, c8, c7, 0	@ flush I,D TLBs
	mrc	p15, 0, r0, c1, c0, 0	@ read control reg
	orr	r0, r0, #0x5000		@ I-cache enable, RR cache replacement
	orr	r0, r0, #0x0030
	bl	__common_mmu_cache_on
#endif
	mov	r0, #0
	mcr	p15, 0, r0, c8, c7, 0	@ flush I,D TLBs
	mov	pc, lr

#if 0
__common_mmu_cache_on:
	orr	r0, r0, #0x000d		@ Write buffer, mmu
	mov	r1, #-1
	mcr	p15, 0, r3, c2, c0, 0	@ load page table pointer
	mcr	p15, 0, r1, c3, c0, 0	@ load domain access control
	b	1f
	.align	5			@ cache line aligned
1:	mcr	p15, 0, r0, c1, c0, 0	@ load control register
	mrc	p15, 0, r0, c1, c0, 0	@ and read it back to
	sub	pc, lr, r0, lsr #32	@ properly flush pipeline
#endif
