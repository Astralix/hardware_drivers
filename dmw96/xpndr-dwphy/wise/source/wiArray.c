//--< wiArray.c >----------------------------------------------------------------------------------
//=================================================================================================
//
//  WiSE Array Utilities
//  Copyright 2010 DSP Group, Inc. All rights reserved.
//
//=================================================================================================

#include <stdlib.h>
#include "wiUtil.h"

static wiBoolean ModuleIsInitialized = WI_FALSE;
#define  STATUS(Status) WICHK((Status))

// ================================================================================================
// FUNCTION  : wiArray_Init()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiArray_Init(void)
{
    if (ModuleIsInitialized) return STATUS(WI_ERROR_MODULE_ALREADY_INIT);

    ModuleIsInitialized = WI_TRUE;
    return WI_SUCCESS;
}
// end of wiArray_Init()

// ================================================================================================
// FUNCTION  : wiArray_Close()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiArray_Close(void)
{
    if (!ModuleIsInitialized) return STATUS(WI_ERROR_MODULE_NOT_INITIALIZED);

	ModuleIsInitialized = WI_FALSE;
    return WI_SUCCESS;
}
// end of wiArray_Close()

// ================================================================================================
// FUNCTION  : wiArray_ReallocArray()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiArray_ReallocArray( void **ptr, 
                               wiArray_t *pArray, 
                               size_t ElementsNeeded, 
                               size_t ElementSize
                               )
{
    wiInt ThreadIndex = wiProcess_ThreadIndex();

    //  If the array was not allocated by this thread, clear the pointer so new memory can be
    //  allocated for this thread instance. This may not be the best policy, so the clause
    //  below may be removed at a later point
    //
    if (ThreadIndex && (pArray->ThreadIndex != ThreadIndex))
        pArray->ptr = NULL;

    //  Resize memory if needed. The behavior is to reallocate if (a) no memory was allocated, or
    //  (b) the allocated size is less than the requested amount. This allows arrays to grow, but
    //  not reduce in size.
    //
    if (!pArray->ptr || (pArray->LengthAllocated < ElementsNeeded))
    {
        if (pArray->ptr && (!ThreadIndex || ThreadIndex == pArray->ThreadIndex)) 
            wiFree(pArray->ptr);
        pArray->LengthAllocated = 0;

        pArray->ptr = wiCalloc(ElementsNeeded, ElementSize);
        if (!pArray->ptr) return WI_ERROR_MEMORY_ALLOCATION;
    
        pArray->ElementSize     = ElementSize;
        pArray->LengthAllocated = ElementsNeeded;
        pArray->ThreadIndex     = ThreadIndex;
    }
    pArray->Length = ElementsNeeded;
    
    if (ptr) *ptr = pArray->ptr;
    return WI_SUCCESS;
}
// end of wiArray_ReallocArray


// ================================================================================================
// FUNCTION  : wiArray_FreeArray()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiArray_FreeArray(void *ptr)
{
    wiArray_t *pArray = (wiArray_t *)ptr;
    wiInt ThreadIndex = wiProcess_ThreadIndex();

    if (pArray->ptr && (!ThreadIndex || ThreadIndex == pArray->ThreadIndex)) 
        wiFree(pArray->ptr);
    pArray->ptr = NULL;
    pArray->Length = 0;
    pArray->LengthAllocated = 0;

    return WI_SUCCESS;
}
// end of wiArray_FreeArray()

///////////////////////////////////////////////////////////////////////////////////////////////////
///
///  wiArray_Get*****Array()
///  Individual functions to enfore typing on the array and pointer elements
///
#define WIARRAY_GETARRAY_TYPE( Type )                        \
                                                             \
wiStatus wiArray_GetArray_##Type (Type **ptr,                \
                                  Type##Array_t *pArray,     \
                                  size_t ElementsNeeded      \
                                 )                           \
{                                                            \
    return wiArray_ReallocArray( (void **)ptr,               \
                                 (wiArray_t *)pArray,        \
                                 ElementsNeeded,             \
                                 sizeof( Type )              \
                                 );                          \
}                                                          
///////////////////////////////////////////////////////////////////////////////////////////////////
//
WIARRAY_GETARRAY_TYPE( wiByte )
WIARRAY_GETARRAY_TYPE( wiInt )
WIARRAY_GETARRAY_TYPE( wiUInt )
WIARRAY_GETARRAY_TYPE( wiWORD )
WIARRAY_GETARRAY_TYPE( wiUWORD )
WIARRAY_GETARRAY_TYPE( wiReal )
WIARRAY_GETARRAY_TYPE( wiComplex )
WIARRAY_GETARRAY_TYPE( wiBoolean )
//
///////////////////////////////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------
