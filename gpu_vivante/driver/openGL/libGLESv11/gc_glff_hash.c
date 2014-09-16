/****************************************************************************
*
*    Copyright (c) 2005 - 2012 by Vivante Corp.  All rights reserved.
*
*    The material in this file is confidential and contains trade secrets
*    of Vivante Corporation. This is proprietary information owned by
*    Vivante Corporation. No part of this work may be disclosed,
*    reproduced, copied, transmitted, or used in any way for any purpose,
*    without the express written permission of Vivante Corporation.
*
*****************************************************************************/




#include "gc_glff_precomp.h"

#define _GC_OBJ_ZONE glvZONE_TRACE

/*******************************************************************************
** Hash table constants.
*/

typedef enum _gleENTRYSIZES
{
    glvHASHTABLEENTRIES
        = 32,

    glvMAXCOLLISIONS
        = 32,

    glvHASHTABLESIZE
        = sizeof(glsHASHTABLE) * glvHASHTABLEENTRIES,

    glvHASHENTRYSIZE
        = gcmALIGN(sizeof(glsHASHTABLEENTRY), 4),

    glvHASHKEYSIZE
        = sizeof(glsHASHKEY),

    glvHASHKEYDWORDCOUNT
        = sizeof(glsHASHKEY) / sizeof(gctUINT32),

    glvVSUNIFORMSIZE
        = gcmALIGN(glvUNIFORM_VS_COUNT   * sizeof(glsUNIFORMWRAP),   4),

    glvVSATTRIBUTESIZE
        = gcmALIGN(glvATTRIBUTE_VS_COUNT * sizeof(glsATTRIBUTEWRAP), 4),

    glvFSUNIFORMSIZE
        = gcmALIGN(glvUNIFORM_FS_COUNT   * sizeof(glsUNIFORMWRAP),   4),

    glvFSATTRIBUTESIZE
        = gcmALIGN(glvATTRIBUTE_FS_COUNT * sizeof(glsATTRIBUTEWRAP), 4)
}
gleENTRYSIZES;


/*******************************************************************************
**
**  _CreateShaderEntry
**
**  Create and initialize shader hash entry.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**  OUTPUT:
**
**      Entry
**          Allocated shader hash entry.
*/

static gceSTATUS _CreateShaderEntry(
    glsCONTEXT_PTR Context,
    glsHASHTABLEENTRY_PTR* Entry
    )
{
    gceSTATUS status;

    static gctUINT timestamp = 0;

    gcmHEADER_ARG("Context=0x%x Entry=0x%x", Context, Entry);

    do
    {
        gctUINT8_PTR buffer;
        glsHASHTABLEENTRY_PTR entry;
        gctPOINTER pointer = gcvNULL;

        /* Determine the size of the entry. */
        gctUINT size
            = glvHASHENTRYSIZE
            + glvHASHKEYSIZE
            + glvVSUNIFORMSIZE
            + glvVSATTRIBUTESIZE
            + glvFSUNIFORMSIZE
            + glvFSATTRIBUTESIZE;

        /* Allocate entry. */
        gcmERR_BREAK(gcoOS_Allocate(
            gcvNULL,
            size,
            &pointer
            ));

        buffer = pointer;

        /* Reset to zero. */
        gcoOS_ZeroMemory(buffer, size);

        /* Set the pointers. */
        *Entry = entry = (glsHASHTABLEENTRY_PTR) buffer;
        buffer += glvHASHENTRYSIZE;

        entry->key = buffer;
        buffer += glvHASHKEYSIZE;

        entry->program.vs.uniforms = (glsUNIFORMWRAP_PTR) buffer;
        buffer += glvVSUNIFORMSIZE;

        entry->program.vs.attributes = (glsATTRIBUTEWRAP_PTR) buffer;
        buffer += glvVSATTRIBUTESIZE;

        entry->program.fs.uniforms = (glsUNIFORMWRAP_PTR) buffer;
        buffer += glvFSUNIFORMSIZE;

        entry->program.fs.attributes = (glsATTRIBUTEWRAP_PTR) buffer;

        /* Construct the vertex shader object. */
        gcmERR_BREAK(gcSHADER_Construct(
            Context->hal,
            gcSHADER_TYPE_VERTEX,
            &entry->program.vs.shader
            ));

        /* Construct the fragment shader object. */
        gcmERR_BREAK(gcSHADER_Construct(
            Context->hal,
            gcSHADER_TYPE_FRAGMENT,
            &entry->program.fs.shader
            ));

        /* Set the timestamp. */
        entry->program.timestamp = ++timestamp;
    }
    while (gcvFALSE);

    gcmFOOTER();

    return status;
}


/*******************************************************************************
**
**  _FreeShaderEntry
**
**  Free allocated shader hash entry.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      Entry
**          Pointer to the shader entry to be freed.
**
**  OUTPUT:
**
**      Nothing.
*/

static gceSTATUS _FreeShaderEntry(
    glsCONTEXT_PTR Context,
    glsHASHTABLEENTRY_PTR Entry
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gceSTATUS last;

    gcmHEADER_ARG("Context=0x%x Entry=0x%x", Context, Entry);

    /* Free the vertex shader object. */
    if (Entry->program.vs.shader)
    {
        last = gcSHADER_Destroy(Entry->program.vs.shader);
        if (gcmIS_ERROR(last))
        {
            status = last;
        }
    }

    /* Free the fragment shader object. */
    if (Entry->program.fs.shader)
    {
        last = gcSHADER_Destroy(Entry->program.fs.shader);
        if (gcmIS_ERROR(last))
        {
            status = last;
        }
    }

    /* Free the program. */
    if (Entry->program.programBuffer)
    {
        last = gcmOS_SAFE_FREE(gcvNULL, Entry->program.programBuffer);
        if (gcmIS_ERROR(last))
        {
            status = last;
        }
    }

    /* Free the hints. */
    if (Entry->program.hints)
    {
        last = gcmOS_SAFE_FREE(gcvNULL, Entry->program.hints);
        if (gcmIS_ERROR(last))
        {
            status = last;
        }
    }

    /* Free the entry. */
    last = gcmOS_SAFE_FREE(gcvNULL, Entry);
    if (gcmIS_ERROR(last))
    {
        status = last;
    }

    gcmFOOTER();
    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  _GetHashBucket
**
**  Compute the hash table index (or hash bucket).
**
**  INPUT:
**
**      HashKey
**          Pointer to the hash key.
**
**      HashKeySize
**          Size of the key in bytes.
**
**  OUTPUT:
**
**      Computed bucket.
*/

static gctUINT32 _GetHashBucket(
    glsCONTEXT_PTR Context
    )
{
    gctUINT32_PTR hashKey = (gctUINT32_PTR) &Context->hashKey;
    gctUINT32 temp;
    gctUINT32 result = 0;
    gctUINT i;
    gcmHEADER_ARG("Context=0x%x", Context);

    for (i = 0; i < glvHASHKEYSIZE/4; i++)
    {
        temp = *hashKey++;
        result += temp & 0xFF00FF;
        result += (temp >> 8) & 0xFF00FF;
    }

    result = ((result & 0xFFFF) + (result >> 16)) * 31;
    gcmFOOTER_ARG("result=%u", result);
    return result;
}


/*******************************************************************************
**
**  glfInitializeHashTable
**
**  Create and initialize context shader hash tables.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**  OUTPUT:
**
**      Nothing.
*/

gceSTATUS glfInitializeHashTable(
    glsCONTEXT_PTR Context
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Context=0x%x", Context);

    gcmASSERT((glvHASHKEYSIZE & 3) == 0);

    do
    {
        gctPOINTER pointer = gcvNULL;

        /* Allocate table. */
        gcmERR_BREAK(gcoOS_Allocate(
            gcvNULL,
            glvHASHTABLESIZE,
            &pointer
            ));

        Context->hashTable = pointer;

        /* Reset to zero. */
        gcoOS_ZeroMemory(Context->hashTable, glvHASHTABLESIZE);
    }
    while (gcvFALSE);

    gcmFOOTER();
    return status;
}


/*******************************************************************************
**
**  glfFreeHashTable
**
**  Free context shader hash tables.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**  OUTPUT:
**
**      Nothing.
*/

gceSTATUS glfFreeHashTable(
    glsCONTEXT_PTR Context
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gceSTATUS last;

    gctUINT i;
    glsHASHTABLE_PTR table = Context->hashTable;

    gcmHEADER_ARG("Context=0x%x", Context);

    /* Is there a table allocated? */
    if (table)
    {
        /* Free hashed entries. */
        for (i = 0; i < glvHASHTABLEENTRIES; i++)
        {
            glsHASHTABLEENTRY_PTR entry = table->chain;

            while (entry)
            {
                glsHASHTABLEENTRY_PTR temp = entry;
                entry = entry->next;

                /* Free the entry. */
                last = _FreeShaderEntry(Context, temp);
                if (gcmIS_ERROR(last))
                {
                    status = last;
                }
            }

            table++;
        }

        /* Free table. */
        last = gcmOS_SAFE_FREE(gcvNULL, Context->hashTable);
        if (gcmIS_ERROR(last))
        {
            status = last;
        }
    }

    gcmFOOTER();
    return status;
}


/*******************************************************************************
**
**  glfGetHashedProgram
**
**  Find hashed program entry; allocate if does not exist.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**  OUTPUT:
**
**      Program
**          Pointer to the hashed program.
*/

gceSTATUS glfGetHashedProgram(
    glsCONTEXT_PTR Context,
    glsPROGRAMINFO_PTR* Program
    )
{
    gceSTATUS status;

    /* Performance counting. */
#if gcmIS_DEBUG(gcdDEBUG_TRACE)
    gctUINT attempt = 0;
#endif

    /* Compute the bucket. */
    gctUINT32 bucket = _GetHashBucket(Context);

    /* Determine the hash index. */
    gctINT index = bucket % glvHASHTABLEENTRIES;

    /* Determine the hash entry. */
    glsHASHTABLE_PTR hashEntry = Context->hashTable + index;

    /* Shortcut to the first entry. */
    glsHASHTABLEENTRY_PTR firstEntry = hashEntry->chain;

    /* Keep history. */
    glsHASHTABLEENTRY_PTR prevEntry = gcvNULL;

    /* Start from the top. */
    glsHASHTABLEENTRY_PTR currEntry = firstEntry;

    gcmHEADER_ARG("Context=0x%x Program=0x%x", Context, Program);

#if gcmIS_DEBUG(gcdDEBUG_TRACE)
    gcmTRACE_ZONE(
        gcvLEVEL_VERBOSE, glvZONE_HASH,
        "[HASH] hash bucket = 0x%08X",
        bucket
        );

    gcmTRACE_ZONE(
        gcvLEVEL_VERBOSE, glvZONE_HASH,
        "       table index = %d",
        index
        );

    gcmTRACE_ZONE(
        gcvLEVEL_VERBOSE, glvZONE_HASH,
        "       number of collisions = %d",
        hashEntry->entryCount ? hashEntry->entryCount - 1 : 0
        );
#endif

    /* Are there entries in the list? */
    if (firstEntry)
    {
        /* Try to locate the entry. */
        do
        {
#if glvENABLE_HASHING
            /* Compare keys. */
            status = gcoOS_MemCmp(&Context->hashKey,
                                  currEntry->key,
                                  gcmSIZEOF(glsHASHKEY));

            /* Key match? */
            if (gcmIS_SUCCESS(status))
            {
                /* Is the current entry at the top of the list? */
                if (currEntry != firstEntry)
                {
                    /* Move to the top. */
                    prevEntry->next = currEntry->next;
                    currEntry->next = firstEntry;
                    hashEntry->chain = currEntry;
                }

                /* Return the found entry. */
                *Program = &currEntry->program;

                /* Print statistics. */
#if gcmIS_DEBUG(gcdDEBUG_TRACE)
                gcmTRACE_ZONE(
                    gcvLEVEL_VERBOSE, glvZONE_HASH,
                    "       found on attempt: %d",
                    ++attempt
                    );
#endif

                gcmFOOTER_NO();
                return gcvSTATUS_OK;
            }
#endif

            /* End of list? */
            if (currEntry->next == gcvNULL)
            {
                break;
            }

            /* The current becomes previous. */
            prevEntry = currEntry;

            /* Go to the next chained entry. */
            currEntry = currEntry->next;
        }
        while (gcvTRUE);
    }

    do
    {
        /* Nothing found, a new entry will be created. Check the number
           items in the list and delete the least recently used if the
           limit of items had been reached. */

        if (hashEntry->entryCount == glvMAXCOLLISIONS)
        {
#if gcmIS_DEBUG(gcdDEBUG_TRACE)
            gcmTRACE_ZONE(
                gcvLEVEL_VERBOSE, glvZONE_HASH,
                "       deleting the least used entry (timestamp = %d).",
                currEntry->program.timestamp
                );
#endif

            if (prevEntry)
            {
                /* Remove from the chain. */
                prevEntry->next = gcvNULL;
            }
            else
            {
                /* This is the only item. */
                firstEntry = gcvNULL;
            }

            gcmERR_BREAK(_FreeShaderEntry(Context, currEntry));
            hashEntry->entryCount--;
        }

        /* Create new entry. */
        gcmERR_BREAK(_CreateShaderEntry(Context, &currEntry));

#if gcmIS_DEBUG(gcdDEBUG_TRACE)
        gcmTRACE_ZONE(
            gcvLEVEL_VERBOSE, glvZONE_HASH,
            "       created new entry (timestamp = %d).",
            currEntry->program.timestamp
            );
#endif

        /* Link in the new entry. */
        currEntry->next = firstEntry;
        hashEntry->chain = currEntry;

        /* Copy the key. */
        {
            gcoOS_MemCopy(currEntry->key,
                          &Context->hashKey,
                          gcmSIZEOF(glsHASHKEY));
        }

        /* Update entry count. */
        hashEntry->entryCount++;

        /* Return result. */
        *Program = &currEntry->program;
    }
    while (gcvFALSE);

    /* Return status. */
    gcmFOOTER();
    return status;
}
