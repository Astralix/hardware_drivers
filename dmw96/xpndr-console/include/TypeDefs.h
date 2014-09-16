#ifndef TYPEDEFS_H
#define TYPEDEFS_H

///////////////////////////////////////////////////////////////////////////////
// Defines
///
/// @brief IN, OUT and INOUT macros
///
#ifndef IN
	#define IN
#endif // IN

#ifndef OUT
	#define OUT
#endif // OUT

#ifndef INOUT
	#define INOUT
#endif // INOUT


///////////////////////////////////////////////////////////////////////////////
// Typedefs
typedef unsigned char			uint8;
typedef unsigned short			uint16;
typedef unsigned long			uint32;
typedef unsigned long long		uint64;
typedef uint16				wchar;


typedef signed char			sint8;
typedef signed short			sint16;
typedef signed long			sint32;
typedef signed long long		sint64;
typedef signed int			sint;

typedef signed char			int8;
typedef signed short			int16;
typedef signed long			int32;
typedef signed long long		int64;


#endif //TYPEDEFS_H