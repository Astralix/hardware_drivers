//
//  wiArray.h -- WISE Array Utilitiy Functions and Definitions
//  Copyright 2010 DSP Group, Inc. All rights reserved.
//

#ifndef __WIARRAY_H
#define __WIARRAY_H

#include "wiType.h"

#if defined(__cplusplus) || defined(__cplusplus__)
    extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//  ARRAY ACCESS
//  Manipulation of wiArray types
//
//  GET_WIARRAY -- Retrieve a pointer to memory in a wiArray structure. If the existing memory is
//                 too small, re-allocate as necessary.
//
//  ALLOCATE_WIARRAY -- (Re-)allocate memory in the wiArray to to provide the desired size.
//
//  WIFREE_ARRAY -- Free array memory allocated in the wiArray structure
//
#define GET_WIARRAY( ptr, Array, Length, Type )                                                  \
{                                                                                                \
    if ( WICHK ( wiArray_GetArray_##Type( &ptr, &Array, Length) ) < 0 )                          \
        return WI_ERROR_RETURNED;                                                                \
}

#define ALLOCATE_WIARRAY( Array, Length, Type )                                                  \
{                                                                                                \
    if ( WICHK ( wiArray_GetArray_##Type( NULL, &Array, Length) ) < 0 )                          \
        return WI_ERROR_RETURNED;                                                                \
}

#define WIFREE_ARRAY(Array) wiArray_FreeArray((void *)&Array)

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//  ARRAY RECORD
//  Type the basic type for generic array information storage. This type is used internally after
//  type casting from a type-specific array defined below.
//
typedef struct wiArray_S
{
    unsigned int Length;
    unsigned int LengthAllocated;
    unsigned int ElementSize;
    int ThreadIndex;
    void *ptr;
}  wiArray_t;

//  Type-Specific Array Definitions
//
//  The macro takes an an argument the name of a data type and then creates a new type with
//  the form <Type>Array_t and defines a function prototype with name wiArray_GetArray_<Type>
//  the supplies the appropriate types to the array structure and point.
//
//
#define WIARRAY_DEFINE_ARRAY_TYPE( Type )                      \
                                                               \
    typedef struct                                             \
    {                                                          \
        unsigned int Length;                                   \
        unsigned int LengthAllocated;                          \
        unsigned int ElementSize;                              \
        int ThreadIndex;                                       \
        Type *ptr;                                             \
                                                               \
    } Type##Array_t;                                           \
                                                               \
wiStatus wiArray_GetArray_##Type ( Type **ptr,                 \
                                   Type##Array_t *pArray,      \
                                   size_t ElementsNeeded       \
                                 );

//  The actual definitions for specific data types are made below
//
WIARRAY_DEFINE_ARRAY_TYPE( wiByte    )
WIARRAY_DEFINE_ARRAY_TYPE( wiInt     )
WIARRAY_DEFINE_ARRAY_TYPE( wiUInt    )
WIARRAY_DEFINE_ARRAY_TYPE( wiWORD    )
WIARRAY_DEFINE_ARRAY_TYPE( wiUWORD   )
WIARRAY_DEFINE_ARRAY_TYPE( wiReal    )
WIARRAY_DEFINE_ARRAY_TYPE( wiComplex )
WIARRAY_DEFINE_ARRAY_TYPE( wiBoolean )

//=================================================================================================
//
//  FUNCTION PROTOTYPES
//
//=================================================================================================
wiStatus wiArray_Init(void);
wiStatus wiArray_Close(void);

wiStatus wiArray_FreeArray(void *pArray);
//-------------------------------------------------------------------------------------------------
#if defined(__cplusplus) || defined(__cplusplus__)
    }
#endif

#endif // __WIARRAY_H
//-------------------------------------------------------------------------------------------------
//--- End of File ---------------------------------------------------------------------------------
