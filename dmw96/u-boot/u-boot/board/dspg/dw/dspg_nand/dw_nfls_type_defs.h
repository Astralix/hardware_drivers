///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#ifndef _TYPEDEFS_H
#define _TYPEDEFS_H

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#include <linux/types.h>

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// DEBUG
///////////////////////////////////////////////////////////////////////////////

#ifndef _DEBUG
	#define _DEBUG
#endif // _DEBUG

///////////////////////////////////////////////////////////////////////////////
// ASSERT
///////////////////////////////////////////////////////////////////////////////

#ifndef ASSERT

	#ifdef _DEBUG
		#define ASSERT(x) assert(x)
	#else
		#define ASSERT(x)
	#endif // DEBUG

#endif // ASSERT

///////////////////////////////////////////////////////////////////////////////
// ENDIAN-NESS
///////////////////////////////////////////////////////////////////////////////

#define HOST_BIG_ENDIAN		0
#define HOST_LITTLE_ENDIAN	1
#define HOST_ENDIAN			HOST_LITTLE_ENDIAN

///////////////////////////////////////////////////////////////////////////////
// MIN / MAX / RANGE
///////////////////////////////////////////////////////////////////////////////

#ifndef MIN
#define MIN(a, b)	( (a) < (b) ? (a) : (b) )
#endif // MIN

#ifndef MAX
#define MAX(a, b)	( (a) > (b) ? (a) : (b) )
#endif // MAX

#ifndef RANGE
#define RANGE(min, x, max)	( (((min) > (x)) || ((x) > (max))) ? false : true )
#endif // RANGE

///////////////////////////////////////////////////////////////////////////////
// VARIOUS MACROS
///////////////////////////////////////////////////////////////////////////////

#ifndef IN
	#define IN
#endif // IN

#ifndef OUT
	#define OUT
#endif // OUT

#ifndef INOUT
	#define INOUT
#endif // INOUT



#define DRAG_INTO_LINKAGE(Symbol)					\
{													\
	volatile int DragIntoLinkage = (int)(Symbol);	\
	(void)DragIntoLinkage;							\
}



#define ARGUSED(x) (void)(x)

#define LENGTHOF(x) (sizeof(x) / sizeof((x)[0]))

#define SIZE_IN_BYTES(x) (sizeof(x)*CHAR_BIT/8)

#define ROUND_UP(x,n) 			( ( (uint32)(x) + (uint32)(n) - 1 ) & ( ~((n)-1)) )
#define ROUND_UP_TO_TYPE(x,n)	( ( (uint32)(x) + SIZE_IN_BYTES(n) - 1) & ~(SIZE_IN_BYTES(n)-1) )

#define ROUND_DOWN(x,n) 		( (uint32)(x) & ~((n)-1) )
#define ROUND_DOWN_TO_TYPE(x,n)	( (uint32)(x)  & ~(SIZE_IN_BYTES(n)-1) )

#define FIELD_MASK( Low, High ) ( ((1 << ((High)-(Low)+1)) - 1) << (Low) )

#define OFFSET_OF( StructName, FieldName ) \
	((int)(&(((StructName*)0)->FieldName)))

#define UPWARD_CAST( BaseClassName, DerivedClassName, BaseClassMemberName, BaseClassPtr ) \
	((DerivedClassName*)(((int)BaseClassPtr) - OFFSET_OF( DerivedClassName, BaseClassMemberName )))	

///////////////////////////////////////////////////////////////////////////////
// BOOLEANS
///////////////////////////////////////////////////////////////////////////////

typedef __u32 bool_type;
#define bool bool_type

#define true	1
#define false	0

///////////////////////////////////////////////////////////////////////////////
// MACROS FOR REGISTERS / FIELDS
///////////////////////////////////////////////////////////////////////////////

typedef volatile __u16* Reg16Ptr;
typedef volatile __u32* Reg32Ptr;

#define REGISTER16(x) (*(Reg16Ptr)(x))
#define REGISTER32(x) (*(Reg32Ptr)(x))

#define DEF_REG( RegName, RegAddr )											\
	enum																	\
	{																		\
		RegName = (RegAddr)													\
	};

#define DEF_FIELD( RegName, FieldName, Ofs, Width )							\
	enum																	\
	{																		\
		RegName##_##FieldName##_OFS			= (Ofs),						\
		RegName##_##FieldName##_WIDTH		= (Width),						\
		RegName##_##FieldName##_MASK		= ((1<<(Width))-1) << (Ofs),	\
		RegName##_##FieldName##_MASK_LSB	= (1<<(Width))-1				\
	};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#ifndef NULL
#define NULL  (void *)0
#endif

#endif // _TYPEDEFS_H
