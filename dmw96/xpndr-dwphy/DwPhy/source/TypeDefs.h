///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#ifndef _TYPEDEFS_H
#define _TYPEDEFS_H

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#include <stddef.h>
#include <limits.h>
 
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
// INTEGER TYPES
///////////////////////////////////////////////////////////////////////////////

typedef unsigned char			uint8;
typedef unsigned short			uint16;
typedef unsigned int 			uint32;
typedef unsigned long long		uint64;
							
typedef signed char				sint8;
typedef signed short			sint16;
typedef signed long				sint32;
typedef signed long long	    sint64;

typedef signed char				int8;
typedef signed short			int16;
typedef signed long				int32;
typedef signed long long		int64;


///////////////////////////////////////////////////////////////////////////////
// BOOLEANS
///////////////////////////////////////////////////////////////////////////////


#ifndef __cplusplus
#define bool_type long
#define bool long
#define true 1
#define false 0
#endif

#ifndef TRUE
#define TRUE        1           
#endif
#ifndef FALSE
#define FALSE       0
#endif
#ifndef Bool
#define Bool		uint8 
#endif
#ifndef BOOLEAN
#define BOOLEAN		uint8 
#endif

/*
 *      BASIC DATA TYPE
 */
typedef unsigned char       uint8_t;
typedef unsigned char       uint8_t;
typedef unsigned char       *PUCHAR;
typedef char                S8;
typedef char                CHAR;
typedef unsigned short      U16;
typedef unsigned short      USHORT;
typedef short               S16;
typedef unsigned int        UINT;
typedef unsigned int        *PUINT;
typedef signed long         S32;
typedef long                LONG;
typedef long                LONG_PTR;
typedef long                *PLONG_PTR;
//QP
typedef unsigned int       U32;
typedef unsigned int       ULONG;
typedef unsigned int       *PULONG;
#if 0
typedef unsigned long       U32;
typedef unsigned long       ULONG;
typedef unsigned long       *PULONG;
#endif

///////////////////////////////////////////////////////////////////////////////
// MACROS FOR REGISTERS / FIELDS
///////////////////////////////////////////////////////////////////////////////

typedef volatile uint16* Reg16Ptr;
typedef volatile uint32* Reg32Ptr;

#define REGISTER16(x) ( *(Reg16Ptr) (x) )
#define REGISTER32(x) ( *(Reg32Ptr) (x) )

/*
#define DEF_REG( RegName, RegAddr )			
	#define	RegName  RegAddr			
#define DEF_REG( RegName, RegAddr )											\
	enum																	\
	{																		\
		RegName = (RegAddr)									                \
	};
	*/


/*
#define DEF_FIELD( RegName, FieldName, Ofs, Width )	   \
	enum											   \
	{												   \
		RegName##_##FieldName##_OFS			= (Ofs),   \
		RegName##_##FieldName##_WIDTH		= (Width), \
		RegName##_##FieldName##_MASK_LSB	= ((1<<(Width))-1) \
	};


#define DEF_FIELD( RegName, FieldName, Ofs, Width )	                        \
define RegName##_##FieldName##_OFS			 (Ofs)                        \
define RegName##_##FieldName##_WIDTH		 (Width)                      \
define RegName##_##FieldName##_MASK		 (((1UL<<(Width))-1)<<(Ofs))  \
define RegName##_##FieldName##_MASK_LSB	 ((1UL<<(Width))-1)            \




#define DEF_REG_EX( RegName, RegAddr, Default )								\
	enum																	\
	{																		\
		RegName = (RegAddr),    											\
		RegName##_##DEFAULT = (Default)										\
	};

*/


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#endif // _TYPEDEFS_H
