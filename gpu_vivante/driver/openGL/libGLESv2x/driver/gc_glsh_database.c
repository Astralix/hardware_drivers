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


/* Database management.
 *
 * TODO: Accumulate changes into a batch database for normal operations as well.
 * Add the batch database support into the HAL during draw time using a new draw
 * interface. This reduces compares and optimizes the HAL interface by having
 * all states together.
 *
 * For now - this is quick initial code to get GLBenchmark up and humming.
 */

#include "gc_glsh_precomp.h"

#if gldFBO_DATABASE

#undef  _GC_OBJ_ZONE
#define _GC_OBJ_ZONE    gldZONE_DATABASE

#define glmENABLE_DISABLE(__flag__, __enum__) \
    if (__flag__) glEnable(__enum__); else glDisable(__enum__)


static gceSTATUS glshNewDatabase(GLFramebuffer Framebuffer,
                                 glsDATABASE_PTR * Database)
{
    gceSTATUS       status;
    glsDATABASE_PTR db = gcvNULL;

    gcmHEADER_ARG("Framebuffer=%p", Framebuffer);

    if (Framebuffer->freeDBCount == 0)
    {
        /* Allocate a database structure. */
        gcmONERROR(gcoOS_Allocate(gcvNULL,
                                  gcmSIZEOF(glsDATABASE),
                                  (gctPOINTER *) &db));
    }
    else
    {
        /* Decrease number of free database structures. */
        Framebuffer->freeDBCount -= 1;

        /* Get database from the free list. */
        db = Framebuffer->freeDB[Framebuffer->freeDBCount];
    }

    /* Zero the database structure. */
    gcmONERROR(gcoOS_ZeroMemory(db, gcmSIZEOF(glsDATABASE)));

    /* Set the owner of the database structure. */
    db->owner = Framebuffer;

    /* Prepend the database structure to the linked list. */
    db->prev            = gcvNULL;
    db->next            = Framebuffer->listDB;
    Framebuffer->listDB = db;

    /* Check if we already have database structures in the list. */
    if (db->next != gcvNULL)
    {
        /* Update the current head of the linked list. */
        db->next->prev = db;
    }

    /* Return database. */
    *Database = db;

    /* Success. */
    gcmFOOTER_ARG("*Database=%p", db);
    return gcvSTATUS_OK;

OnError:
    /* Roll back the allocation. */
    if (db != gcvNULL)
    {
        gcmVERIFY_OK(gcoOS_Free(gcvNULL, db));
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}

static gceSTATUS glshDeleteDatabase(glsDATABASE_PTR Database)
{
    GLFramebuffer   framebuffer;
    gceSTATUS       status;

    gcmHEADER_ARG("Database=%p", Database);

    /* Get pointer to FBO. */
    framebuffer = Database->owner;

    /* Unlink the database structure from the linked list. */
    if (Database->prev != gcvNULL)
    {
        /* Simple unlink. */
        Database->prev->next = Database->next;
    }
    else
    {
        /* Head of list, so update FBO list pointer. */
        Database->owner->listDB = Database->next;
    }

    if (Database->next != gcvNULL)
    {
        /* Simple unlink. */
        Database->next->prev = Database->prev;
    }

    /* Check if wee are removing the current database. */
    if (framebuffer->currentDB == Database)
    {
        /* Remove the current database. */
        framebuffer->currentDB = gcvNULL;
    }

    /* Check if the FBO's free list is full. */
    if (framebuffer->freeDBCount < gcmCOUNTOF(framebuffer->freeDB))
    {
        /* Store the database in the next available free slot. */
        framebuffer->freeDB[framebuffer->freeDBCount] = Database;

        /* Increment number of free databases. */
        framebuffer->freeDBCount += 1;
    }
    else
    {
        /* Free database since the free list is full. */
        gcmONERROR(gcoOS_Free(gcvNULL, Database));
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status.*/
    gcmFOOTER();
    return status;
}

static gceSTATUS glshNewRecord(glsDATABASE_PTR Database,
                               glsDATABASE_RECORD_PTR * Record)
{
    gceSTATUS               status;
    GLFramebuffer           framebuffer;
    glsDATABASE_RECORD_PTR  record;

    gcmHEADER_ARG("Database=%p", Database);

    /* Get pointer to the FBO owing this database. */
    framebuffer = Database->owner;

    if (framebuffer->freeRecordCount == 0)
    {
        /* Allocate a record structure. */
        gcmONERROR(gcoOS_Allocate(gcvNULL,
                                  gcmSIZEOF(glsDATABASE_RECORD),
                                  (gctPOINTER *) &record));
    }
    else
    {
        /* Decrease number of free records. */
        framebuffer->freeRecordCount -= 1;

        /* Get record from the free list. */
        record = framebuffer->freeRecord[framebuffer->freeRecordCount];
    }

    /* Initialize the record pool. */
    record->next   = gcvNULL;
    record->offset = 0;

    /* Append the database to the end of the linked list. */
    if (Database->last == gcvNULL)
    {
        /* First database, so just initialize pointers. */
        Database->first = record;
        Database->last  = record;
    }
    else
    {
        /* Append to end of the linked list. */
        Database->last->next = record;
        Database->last       = record;
    }

    /* Adjust size. */
    Database->size += gcmSIZEOF(glsDATABASE_RECORD);

    /* Return the record. */
    *Record = record;

    /* Success. */
    gcmFOOTER_ARG("*Record=%p", record);
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

static gceSTATUS glshDeleteRecord(glsDATABASE_PTR Database)
{
    gceSTATUS               status;
    GLFramebuffer           framebuffer;
    glsDATABASE_RECORD_PTR  record;

    gcmHEADER_ARG("Database=%p", Database);

    /* Get the pointer to the FBO owning the database. */
    framebuffer = Database->owner;

    /* Get the current record array. */
    record = Database->first;

    /* Unlink the record array from the linked list. */
    Database->first = record->next;

    /* Check if the FBO's free record list is full or not. */
    if (framebuffer->freeRecordCount < gcmCOUNTOF(framebuffer->freeRecord))
    {
        /* Add the record to the free list. */
        framebuffer->freeRecord[framebuffer->freeRecordCount] = record;

        /* Increment number of free records. */
        framebuffer->freeRecordCount += 1;
    }
    else
    {
        /* Free record array since the free list is full. */
        gcmONERROR(gcoOS_Free(gcvNULL, record));
    }

    /* Success. */
    gcmFOOTER_NO();

    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

static gceSTATUS glshAddDependency(glsDATABASE_PTR Database,
                                   glsDATABASE_PTR Dependency)
{
    gceSTATUS                   status;
    glsDATABASE_DEPENDENCY_PTR  dependencies;
    gctINT                      i;

    gcmHEADER_ARG("Database=%p Dependency=%p", Database, Dependency);

    /* Process all dependency arrays. */
    for (dependencies = Database->dependencies;
         dependencies != gcvNULL;
         dependencies = dependencies->next
         )
    {
        /* Process all entries in the dependency array. */
        for (i = 0; i < dependencies->count; i++)
        {
            /* Check for a match. */
            if (dependencies->dependencies[i] == Dependency)
            {
                /* Matched, just return success. */
                gcmFOOTER_NO();
                return gcvSTATUS_OK;
            }
        }
    }

    /* Get the current dependency array. */
    dependencies = Database->dependencies;

    /* Check if there is not enough room in the current dependency array. */
    if ((dependencies == gcvNULL)
        ||
        (dependencies->count == gcmCOUNTOF(dependencies->dependencies))
        )
    {
        /* Allocate a new dependency array. */
        gcmONERROR(gcoOS_Allocate(gcvNULL,
                                  gcmSIZEOF(glsDATABASE_DEPENDENCY),
                                  (gctPOINTER *) &dependencies));

        /* Prepend the dependency array to the linked list. */
        dependencies->next     = Database->dependencies;
        Database->dependencies = dependencies;

        /* Empty the dependency array. */
        dependencies->count = 0;
    }

    /* Save new dependency. */
    dependencies->dependencies[dependencies->count++] = Dependency;

    /* Increment dependency reference counter. */
    Dependency->reference += 1;

    gcmTRACE_ZONE(gcvLEVEL_INFO, gldZONE_DATABASE,
                  "Database %p has reference %d",
                  Dependency, Dependency->reference);

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status.*/
    gcmFOOTER();
    return status;
}

static gceSTATUS glshRemoveReference(glsDATABASE_PTR Database)
{
    gceSTATUS   status;

    gcmHEADER_ARG("Database=%p", Database);

    /* Decrement reference counter. */
    if (Database->reference > 0)
    {
        Database->reference -= 1;
    }

    gcmTRACE_ZONE(gcvLEVEL_INFO, gldZONE_DATABASE,
                  "Database %p has reference %d",
                  Database, Database->reference);

    /* Check if reference counter has become zero and this is not the current
     * database. */
    if ((Database->reference == 0)
        &&
        (Database->owner->currentDB != Database)
        )
    {
        /* Delete the database. */
        gcmONERROR(glshDeleteDatabase(Database));
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

static gceSTATUS glshRemoveDependencies(glsDATABASE_PTR Database)
{
    glsDATABASE_DEPENDENCY_PTR  dependency;
    gceSTATUS                   status;
    gctINT                      i;

    gcmHEADER_ARG("Database=%p", Database);

    /* Remove all dependencies. */
    while (Database->dependencies != gcvNULL)
    {
        /* Get current dependency array. */
        dependency = Database->dependencies;

        for (i = 0; i < dependency->count; i++)
        {
            /* Remove dependecies for this database as well. */
            gcmONERROR(glshRemoveDependencies(dependency->dependencies[i]));

            /* Clear this database. */
            gcmONERROR(glshCleanDatabase(dependency->dependencies[i]));

            /* Remove the reference. */
            gcmONERROR(glshRemoveReference(dependency->dependencies[i]));
        }

        /* Remove dependency array from the linked list. */
        Database->dependencies = dependency->next;

        /* Free the dependency array. */
        gcmONERROR(gcoOS_Free(gcvNULL, dependency));
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}


static void glshGetStates(GLContext Context, glsDATABASE_STATES * States)
{
    gcmHEADER_ARG("Context=%p States=%p", Context, States);

    /* Copy intial scissors states. */
    States->scissors.enable     = Context->scissorEnable;
    States->scissors.box.x      = Context->scissorX;
    States->scissors.box.y      = Context->scissorY;
    States->scissors.box.width  = Context->scissorWidth;
    States->scissors.box.height = Context->scissorHeight;

    /* Copy intial polygon states. */
    States->polygon.cullEnable      = Context->cullEnable;
    States->polygon.cullMode        = Context->cullMode;
    States->polygon.cullFront       = Context->cullFront;
    States->polygon.lineWidth       = (GLint)(Context->lineWidth);
    States->polygon.viewport.x      = Context->viewportX;
    States->polygon.viewport.y      = Context->viewportY;
    States->polygon.viewport.width  = Context->viewportWidth;
    States->polygon.viewport.height = Context->viewportHeight;
    States->polygon.near            = Context->depthNear;
    States->polygon.far             = Context->depthFar;
    States->polygon.offsetEnable    = Context->offsetEnable;
    States->polygon.factor          = Context->offsetFactor;
    States->polygon.units           = Context->offsetUnits;

    /* Copy initial blending states. */
    States->blending.enable               = Context->blendEnable;
    States->blending.funcSourceRGB        = Context->blendFuncSourceRGB;
    States->blending.funcSourceAlpha      = Context->blendFuncSourceAlpha;
    States->blending.funcDestinationRGB   = Context->blendFuncTargetRGB;
    States->blending.funcDestinationAlpha = Context->blendFuncTargetAlpha;
    States->blending.modeRGB              = Context->blendModeRGB;
    States->blending.modeAlpha            = Context->blendModeAlpha;
    States->blending.red                  = Context->blendColorRed;
    States->blending.green                = Context->blendColorGreen;
    States->blending.blue                 = Context->blendColorBlue;
    States->blending.alpha                = Context->blendColorAlpha;

    /* Copy initial color enable states. */
    States->pixel.red   = Context->colorEnableRed;
    States->pixel.green = Context->colorEnableGreen;
    States->pixel.blue  = Context->colorEnableBlue;
    States->pixel.alpha = Context->colorEnableAlpha;

    /* Copy initial dither states. */
    States->pixel.dither = Context->ditherEnable;

    /* Copy initial depth states. */
    States->depth.test = Context->depthTest;
    States->depth.func = Context->depthFunc;
    States->depth.mask = Context->depthMask;

    /* Copy initial stencil states. */
    States->stencil.test             = Context->stencilEnable;
    States->stencil.frontFunction    = Context->stencilFuncFront;
    States->stencil.frontReference   = Context->stencilRefFront;
    States->stencil.frontMask        = Context->stencilMaskFront;
    States->stencil.frontOpFail      = Context->stencilOpFailFront;
    States->stencil.frontOpDepthFail = Context->stencilOpDepthFailFront;
    States->stencil.frontOpDepthPass = Context->stencilOpDepthPassFront;
    States->stencil.backFunction     = Context->stencilFuncBack;
    States->stencil.backReference    = Context->stencilRefBack;
    States->stencil.backMask         = Context->stencilMaskBack;
    States->stencil.backOpFail       = Context->stencilOpFailBack;
    States->stencil.backOpDepthFail  = Context->stencilOpDepthFailBack;
    States->stencil.backOpDepthPass  = Context->stencilOpDepthPassBack;

    gcmFOOTER_NO();
}

static gceSTATUS glshAddDatabase(glsDATABASE_PTR Database,
                                 gctSIZE_T Bytes,
                                 gctCONST_POINTER Data,
                                 gctPOINTER * Location)
{
    gceSTATUS               status;
    glsDATABASE_RECORD_PTR  record;
    gctPOINTER              location;

    gcmHEADER_ARG("Database=%p Bytes=%lu Data=%p", Database, Bytes, Data);

    /* Get the last record pool in the linked list. */
    record = Database->last;

    /* Test if there is no record pool or it is out of memory. */
    if ((record == gcvNULL)
        ||
        (record->offset + Bytes > gcmSIZEOF(record->data))
        )
    {
        /* Allocate a new record array. */
        gcmONERROR(glshNewRecord(Database, &record));
    }

    /* Compute the location. */
    location = record->data + record->offset;

    /* Copy the record into the database at the current location. */
    gcmONERROR(gcoOS_MemCopy(location, Data, Bytes));

    /* Adjust the database offset. */
    record->offset += Bytes;

    if (Location != gcvNULL)
    {
        /* Return the location. */
        *Location = location;
    }

    /* Success. */
    gcmFOOTER_ARG("*Location=%p", gcmOPT_POINTER(Location));
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

static gceSTATUS glshInitialStates(GLContext Context, glsDATABASE_PTR Database)
{
    gceSTATUS                       status;
    glsDATABASE_PROGRAM_ARRAY_PTR   programArray;
    glsDATABASE_TEXTURE_ARRAY_PTR   textureArray;

    gcmHEADER_ARG("Context=%p Database=%p", Context, Database);

    /* Get initial states. */
    glshGetStates(Context, &Database->initialStates);

    /* Copy initial states to current states. */
    Database->currentStates = Database->initialStates;

    /* Process all program arrays in the linked list. */
    while (Database->programs != gcvNULL)
    {
        /* Get the current program array. */
        programArray = Database->programs;

        /* Remove program array from the linked list. */
        Database->programs = programArray->next;

        /* Free the program array. */
        gcmONERROR(gcoOS_Free(gcvNULL, programArray));
    }

    /* Process all texture arrays in the linked list. */
    while (Database->textures != gcvNULL)
    {
        /* Get the current program array. */
        textureArray = Database->textures;

        /* Remove texture array from the linked list. */
        Database->textures = textureArray->next;

        /* Free the texture array. */
        gcmONERROR(gcoOS_Free(gcvNULL, textureArray));
    }

    /* No known program. */
    Database->currentProgram = gcvNULL;

    /* Remove all dependencies. */
    gcmONERROR(glshRemoveDependencies(Database));

    gcmASSERT(Database->first == gcvNULL);
    gcmASSERT(Database->last == gcvNULL);

    /* Allocate the first record pool. */
    gcmONERROR(glshNewRecord(Database, &Database->first));
    Database->size = gcmSIZEOF(glsDATABASE_RECORD);

    /* No draw elements record. */
    Database->lastDrawElements = gcvNULL;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

static gceSTATUS glshAddScissors(GLContext Context, glsDATABASE_PTR Database)
{
    gceSTATUS               status;
    glsDATABASE_SCISSORS    scissors;
    glsDATABASE_SCISSORS *  current;

    gcmHEADER_ARG("Context=%p Database=%p", Context, Database);

    /* Check if the database is empty. */
    if (Database->first == gcvNULL)
    {
        /* Get initial states. */
        gcmONERROR(glshInitialStates(Context, Database));
    }
    else
    {
        /* Get shortcut to current states. */
        current = &Database->currentStates.scissors;

        /* Initialize the scissors record. */
        scissors.opcode = glvDATABASE_OPCODE_SCISSORS;
        scissors.flags  = 0;

        /* Test for a change in the scissors enable flag. */
        if (current->enable != Context->scissorEnable)
        {
            /* Copy changes. */
            scissors.flags |= GLDB_SCISSOR_ENABLE;
            scissors.enable = Context->scissorEnable;
        }

        /* Test for a change in the scissors box. */
        if ((current->box.x != Context->scissorX)
            ||
            (current->box.y != Context->scissorY)
            ||
            (current->box.width != Context->scissorWidth)
            ||
            (current->box.height != Context->scissorHeight)
            )
        {
            /* Copy changes. */
            scissors.flags     |= GLDB_SCISSOR_BOX;
            scissors.box.x      = Context->scissorX;
            scissors.box.y      = Context->scissorY;
            scissors.box.width  = Context->scissorWidth;
            scissors.box.height = Context->scissorHeight;
        }

        /* Test for any changes. */
        if (scissors.flags != 0)
        {
            /* Add the record to the database. */
            gcmONERROR(glshAddDatabase(Database,
                                       gcmSIZEOF(scissors), &scissors,
                                       gcvNULL));

            /* Copy current states. */
            current->enable     = Context->scissorEnable;
            current->box.x      = Context->scissorX;
            current->box.y      = Context->scissorY;
            current->box.width  = Context->scissorWidth;
            current->box.height = Context->scissorHeight;
        }
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

static gceSTATUS glshAddPolygon(GLContext Context, glsDATABASE_PTR Database)
{
    gceSTATUS               status;
    glsDATABASE_POLYGON     polygon;
    glsDATABASE_POLYGON *   current;

    gcmHEADER_ARG("Context=%p Database=%p", Context, Database);

    /* Check if the database is empty. */
    if (Database->first == gcvNULL)
    {
        /* Get initial states. */
        gcmONERROR(glshInitialStates(Context, Database));
    }
    else
    {
        /* Get shortcut to current states. */
        current = &Database->currentStates.polygon;

        /* Initialize the polygcon record. */
        polygon.opcode = glvDATABASE_OPCODE_POLYGON;
        polygon.flags  = 0;

        /* Test for a change in the cull enable flag. */
        if (current->cullEnable != Context->cullEnable)
        {
            /* Copy changes. */
            polygon.flags     |= GLDB_POLYGON_CULL_ENABLE;
            polygon.cullEnable = Context->cullEnable;
        }

        /* Test for a change in the cull mode. */
        if (current->cullMode != Context->cullMode)
        {
            /* Copy changes. */
            polygon.flags   |= GLDB_POLYGON_CULL_MODE;
            polygon.cullMode = Context->cullMode;
        }

        /* Test for a change in the cull front type. */
        if (current->cullFront != Context->cullFront)
        {
            /* Copy changes. */
            polygon.flags    |= GLDB_POLYGON_CULL_FRONT;
            polygon.cullFront = Context->cullFront;
        }

        /* Test for a change in the line width. */
        if (current->lineWidth != Context->lineWidth)
        {
            /* Copy changes. */
            polygon.flags    |= GLDB_POLYGON_LINE_WIDTH;
            polygon.lineWidth = (GLint)(Context->lineWidth);
        }

        /* Test for a change in the viewport. */
        if ((current->viewport.x != Context->viewportX)
            ||
            (current->viewport.y != Context->viewportY)
            ||
            (current->viewport.width != Context->viewportWidth)
            ||
            (current->viewport.height != Context->viewportHeight)
            )
        {
            /* Copy changes. */
            polygon.flags          |= GLDB_POLYGON_VIEWPORT;
            polygon.viewport.x      = Context->viewportX;
            polygon.viewport.y      = Context->viewportY;
            polygon.viewport.width  = Context->viewportWidth;
            polygon.viewport.height = Context->viewportHeight;
        }

        /* Test for a change in the depth range. */
        if ((current->near != Context->depthNear)
            ||
            (current->far != Context->depthFar)
            )
        {
            /* Copy changes. */
            polygon.flags |= GLDB_POLYGON_DEPTH_RANGE;
            polygon.near   = Context->depthNear;
            polygon.far    = Context->depthFar;
        }

        /* Test for a change in the polygcon offset enable flag. */
        if (current->offsetEnable != Context->offsetEnable)
        {
            /* Copy changes. */
            polygon.flags       |= GLDB_POLYGON_OFFSET_ENABLE;
            polygon.offsetEnable = Context->offsetEnable;
        }

        /* Test for a change in the polygcon offset. */
        if ((current->factor != Context->offsetFactor)
            ||
            (current->units != Context->offsetUnits)
            )
        {
            /* Copy changes. */
            polygon.flags |= GLDB_POLYGON_OFFSET;
            polygon.factor = Context->offsetFactor;
            polygon.units  = Context->offsetUnits;
        }

        /* Test for any changes. */
        if (polygon.flags != 0)
        {
            /* Add the record to the database. */
            gcmONERROR(glshAddDatabase(Database,
                                       gcmSIZEOF(polygon), &polygon,
                                       gcvNULL));

            /* Copy current states. */
            current->cullEnable      = Context->cullEnable;
            current->cullMode        = Context->cullMode;
            current->cullFront       = Context->cullFront;
            current->lineWidth       = (GLint)(Context->lineWidth);
            current->viewport.x      = Context->viewportX;
            current->viewport.y      = Context->viewportY;
            current->viewport.width  = Context->viewportWidth;
            current->viewport.height = Context->viewportHeight;
            current->near            = Context->depthNear;
            current->far             = Context->depthFar;
            current->offsetEnable    = Context->offsetEnable;
            current->factor          = Context->offsetFactor;
            current->units           = Context->offsetUnits;
        }
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

static gceSTATUS glshAddBlending(GLContext Context, glsDATABASE_PTR Database)
{
    gceSTATUS           status;
    glsDATABASE_BLEND   blending;
    glsDATABASE_BLEND * current;

    gcmHEADER_ARG("Context=%p Database=%p", Context, Database);

    /* Check if the database is empty. */
    if (Database->first == gcvNULL)
    {
        /* Get initial states. */
        gcmONERROR(glshInitialStates(Context, Database));
    }
    else
    {
        /* Get shortcut to current states. */
        current = &Database->currentStates.blending;

        /* Initialize the blending record. */
        blending.opcode = glvDATABASE_OPCODE_BLEND;
        blending.flags  = 0;

        if (current->enable != Context->blendEnable)
        {
            /* Copy changes. */
            blending.flags |= GLDB_BLEND_ENABLE;
            blending.enable = Context->blendEnable;
        }

        if ((current->funcSourceRGB != Context->blendFuncSourceRGB)
            ||
            (current->funcSourceAlpha != Context->blendFuncSourceAlpha)
            ||
            (current->funcDestinationRGB != Context->blendFuncTargetRGB)
            ||
            (current->funcDestinationAlpha != Context->blendFuncTargetAlpha)
            )
        {
            /* Copy changes. */
            blending.flags               |= GLDB_BLEND_FUNCTION;
            blending.funcSourceRGB        = Context->blendFuncSourceRGB;
            blending.funcSourceAlpha      = Context->blendFuncSourceAlpha;
            blending.funcDestinationRGB   = Context->blendFuncTargetRGB;
            blending.funcDestinationAlpha = Context->blendFuncTargetAlpha;
        }

        if ((current->modeRGB != Context->blendModeRGB)
            ||
            (current->modeAlpha != Context->blendModeAlpha)
            )
        {
            /* Copy changes. */
            blending.flags    |= GLDB_BLEND_MODE;
            blending.modeRGB   = Context->blendModeRGB;
            blending.modeAlpha = Context->blendModeAlpha;
        }

        if ((current->red != Context->blendColorRed)
            ||
            (current->green != Context->blendColorGreen)
            ||
            (current->blue != Context->blendColorBlue)
            ||
            (current->alpha != Context->blendColorAlpha)
            )
        {
            /* Copy changes. */
            blending.flags |= GLDB_BLEND_COLOR;
            blending.red    = Context->blendColorRed;
            blending.green  = Context->blendColorGreen;
            blending.blue   = Context->blendColorBlue;
            blending.alpha  = Context->blendColorAlpha;
        }

        /* Test for any changes. */
        if (blending.flags != 0)
        {
            /* Add the record to the database. */
            gcmONERROR(glshAddDatabase(Database,
                                       gcmSIZEOF(blending), &blending,
                                       gcvNULL));

            /* Copy current states. */
            current->enable               = Context->blendEnable;
            current->funcSourceRGB        = Context->blendFuncSourceRGB;
            current->funcSourceAlpha      = Context->blendFuncSourceAlpha;
            current->funcDestinationRGB   = Context->blendFuncTargetRGB;
            current->funcDestinationAlpha = Context->blendFuncTargetAlpha;
            current->modeRGB              = Context->blendModeRGB;
            current->modeAlpha            = Context->blendModeAlpha;
            current->red                  = Context->blendColorRed;
            current->green                = Context->blendColorGreen;
            current->blue                 = Context->blendColorBlue;
            current->alpha                = Context->blendColorAlpha;
        }
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

static gceSTATUS glshAddPixel(GLContext Context, glsDATABASE_PTR Database)
{
    gceSTATUS           status;
    glsDATABASE_PIXEL   pixel;
    glsDATABASE_PIXEL * current;

    gcmHEADER_ARG("Context=%p Database=%p", Context, Database);

    /* Check if the database is empty. */
    if (Database->first == gcvNULL)
    {
        /* Get initial states. */
        gcmONERROR(glshInitialStates(Context, Database));
    }
    else
    {
        /* Get shortcut to current states. */
        current = &Database->currentStates.pixel;

        /* Initialize the pixel record. */
        pixel.opcode = glvDATABASE_OPCODE_PIXEL;
        pixel.flags  = 0;

        if ((current->red != Context->colorEnableRed)
            ||
            (current->green != Context->colorEnableGreen)
            ||
            (current->blue != Context->colorEnableBlue)
            ||
            (current->alpha != Context->colorEnableAlpha)
            )
        {
            /* Copy changes. */
            pixel.flags |= GLDB_PIXEL_ENABLE;
            pixel.red    = Context->colorEnableRed;
            pixel.green  = Context->colorEnableGreen;
            pixel.blue   = Context->colorEnableBlue;
            pixel.alpha  = Context->colorEnableAlpha;
        }

        if (current->dither != Context->ditherEnable)
        {
            /* Copy changes. */
            pixel.flags |= GLDB_PIXEL_DITHER;
            pixel.dither = Context->ditherEnable;
        }

        /* Test for any changes. */
        if (pixel.flags != 0)
        {
            /* Add the record to the database. */
            gcmONERROR(glshAddDatabase(Database,
                                       gcmSIZEOF(pixel), &pixel,
                                       gcvNULL));

            /* Copy current states. */
            current->red    = Context->colorEnableRed;
            current->green  = Context->colorEnableGreen;
            current->blue   = Context->colorEnableBlue;
            current->alpha  = Context->colorEnableAlpha;
            current->dither = Context->ditherEnable;
        }
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

static gceSTATUS glshAddDepth(GLContext Context, glsDATABASE_PTR Database)
{
    gceSTATUS           status;
    glsDATABASE_DEPTH   depth;
    glsDATABASE_DEPTH * current;

    gcmHEADER_ARG("Context=%p Database=%p", Context, Database);

    /* Check if the database is empty. */
    if (Database->first == gcvNULL)
    {
        /* Get initial states. */
        gcmONERROR(glshInitialStates(Context, Database));
    }
    else
    {
        /* Get shortcut to current states. */
        current = &Database->currentStates.depth;

        /* Initialize the depth record. */
        depth.opcode = glvDATABASE_OPCODE_DEPTH;
        depth.flags  = 0;

        if (current->test != Context->depthTest)
        {
            /* Copy changes. */
            depth.flags |= GLDB_DEPTH_TEST;
            depth.test   = Context->depthTest;
        }

        if (current->func != Context->depthFunc)
        {
            /* Copy changes. */
            depth.flags |= GLDB_DEPTH_FUNC;
            depth.func   = Context->depthFunc;
        }

        if (current->mask != Context->depthMask)
        {
            /* Copy changes. */
            depth.flags |= GLDB_DEPTH_MASK;
            depth.mask   = Context->depthMask;
        }

        /* Test for any changes. */
        if (depth.flags != 0)
        {
            /* Add the record to the database. */
            gcmONERROR(glshAddDatabase(Database,
                                       gcmSIZEOF(depth), &depth,
                                       gcvNULL));

            /* Copy current states. */
            current->test = Context->depthTest;
            current->func = Context->depthFunc;
            current->mask = Context->depthMask;
        }
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

static gceSTATUS glshAddStencil(GLContext Context, glsDATABASE_PTR Database)
{
    gceSTATUS               status;
    glsDATABASE_STENCIL     stencil;
    glsDATABASE_STENCIL *   current;

    gcmHEADER_ARG("Context=%p Database=%p", Context, Database);

    /* Check if the database is empty. */
    if (Database->first == gcvNULL)
    {
        /* Get initial states. */
        gcmONERROR(glshInitialStates(Context, Database));
    }
    else
    {
        /* Get shortcut to current states. */
        current = &Database->currentStates.stencil;

        /* Initialize the stencil record. */
        stencil.opcode = glvDATABASE_OPCODE_STENCIL;
        stencil.flags  = 0;

        if (current->test != Context->stencilEnable)
        {
            /* Copy changes. */
            stencil.flags |= GLDB_STENCIL_TEST;
            stencil.test   = Context->stencilEnable;
        }

        if ((current->frontFunction != Context->stencilFuncFront)
            ||
            (current->frontReference != Context->stencilRefFront)
            )
        {
            /* Copy changes. */
            stencil.flags         |= GLDB_STENCIL_FRONT_FUNCTION;
            stencil.frontFunction  = Context->stencilFuncFront;
            stencil.frontReference = Context->stencilRefFront;
        }

        if (current->frontMask != Context->stencilMaskFront)
        {
            /* Copy changes. */
            stencil.flags    |= GLDB_STENCIL_FRONT_MASK;
            stencil.frontMask = Context->stencilMaskFront;
        }

        if ((current->frontOpFail != Context->stencilOpFailFront)
            ||
            (current->frontOpDepthFail != Context->stencilOpDepthFailFront)
            ||
            (current->frontOpDepthPass != Context->stencilOpDepthPassFront)
            )
        {
            /* Copy changes. */
            stencil.flags           |= GLDB_STENCIL_FRONT_OPERATION;
            stencil.frontOpFail      = Context->stencilOpFailFront;
            stencil.frontOpDepthFail = Context->stencilOpDepthFailFront;
            stencil.frontOpDepthPass = Context->stencilOpDepthPassFront;
        }

        if ((current->backFunction != Context->stencilFuncBack)
            ||
            (current->backReference != Context->stencilRefBack)
            )
        {
            /* Copy changes. */
            stencil.flags        |= GLDB_STENCIL_BACK_FUNCTION;
            stencil.backFunction  = Context->stencilFuncBack;
            stencil.backReference = Context->stencilRefBack;
        }

        if (current->backMask != Context->stencilMaskBack)
        {
            /* Copy changes. */
            stencil.flags   |= GLDB_STENCIL_BACK_MASK;
            stencil.backMask = Context->stencilMaskBack;
        }

        if ((current->backOpFail != Context->stencilOpFailBack)
            ||
            (current->backOpDepthFail != Context->stencilOpDepthFailBack)
            ||
            (current->backOpDepthPass != Context->stencilOpDepthPassBack)
            )
        {
            /* Copy changes. */
            stencil.flags          |= GLDB_STENCIL_BACK_OPERATION;
            stencil.backOpFail      = Context->stencilOpFailBack;
            stencil.backOpDepthFail = Context->stencilOpDepthFailBack;
            stencil.backOpDepthPass = Context->stencilOpDepthPassBack;
        }

        /* Test for any changes. */
        if (stencil.flags != 0)
        {
            /* Add the record to the database. */
            gcmONERROR(glshAddDatabase(Database,
                                       gcmSIZEOF(stencil), &stencil,
                                       gcvNULL));

            /* Copy current states. */
            current->test             = Context->stencilEnable;
            current->frontFunction    = Context->stencilFuncFront;
            current->frontReference   = Context->stencilRefFront;
            current->frontMask        = Context->stencilMaskFront;
            current->frontOpFail      = Context->stencilOpFailFront;
            current->frontOpDepthFail = Context->stencilOpDepthFailFront;
            current->frontOpDepthPass = Context->stencilOpDepthPassFront;
            current->backFunction     = Context->stencilFuncBack;
            current->backReference    = Context->stencilRefBack;
            current->backMask         = Context->stencilMaskBack;
            current->backOpFail       = Context->stencilOpFailBack;
            current->backOpDepthFail  = Context->stencilOpDepthFailBack;
            current->backOpDepthPass  = Context->stencilOpDepthPassBack;
        }
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

static gceSTATUS glshNewProgramArray(glsDATABASE_PROGRAM_ARRAY_PTR * Array)
{
    gceSTATUS                       status;
    glsDATABASE_PROGRAM_ARRAY_PTR   memory = gcvNULL;

    gcmHEADER();

    /* Allocate a new array of programs. */
    gcmONERROR(gcoOS_Allocate(gcvNULL,
                              gcmSIZEOF(glsDATABASE_PROGRAM_ARRAY),
                              (gctPOINTER *) &memory));

    /* Zero the array of programs. */
    gcmONERROR(gcoOS_ZeroMemory(memory, gcmSIZEOF(glsDATABASE_PROGRAM_ARRAY)));

    /* Return the new array of programs. */
    *Array = memory;

    /* Success. */
    gcmFOOTER_ARG("*Array=%p", *Array);
    return gcvSTATUS_OK;

OnError:
    /* Roll back the allocation. */
    if (memory != gcvNULL)
    {
        gcmVERIFY_OK(gcoOS_Free(gcvNULL, memory));
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}

static gceSTATUS glshAddProgram(GLContext Context, glsDATABASE_PTR Database)
{
    gceSTATUS                       status;
    GLProgram                       program;
    glsDATABASE_PROGRAM             record;
    gctINT                          i;
    glsDATABASE_UNIFORM             uniform;
    glsDATABASE_PROGRAM_ARRAY_PTR   programArray;
    gctBOOL                         known;
    gctINT                          arrayIndex;
    gctINT                          programIndex;

    gcmHEADER_ARG("Context=%p Database=%p", Context, Database);

    /* Get current program. */
    program = Context->program;
    if (program == gcvNULL)
    {
        /* No program. */
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    /* Convert program name into array and program index. */
    arrayIndex   = program->object.name / gcmCOUNTOF(programArray->programs);
    programIndex = program->object.name % gcmCOUNTOF(programArray->programs);

    /* Find the correct array of programs. */
    for (i = 0, programArray = Database->programs; i < arrayIndex; i++)
    {
        /* Check if we have a valid array of programs. */
        if (programArray == gcvNULL)
        {
            break;
        }

        /* Get the next array of programs. */
        programArray = programArray->next;
    }

    /* Check if the program is known. */
    known = ((programArray != gcvNULL)
             &&
             (programArray->programs[programIndex] == program));

    /* Check if we need to add this program to the linked list. */
    if (!known)
    {
        /* Check if we don't have the correct array of program. */
        if (programArray == gcvNULL)
        {
            /* Check if we have any array of programs. */
            if (Database->programs == gcvNULL)
            {
                /* Create a new array of programs. */
                gcmONERROR(glshNewProgramArray(&Database->programs));
            }

            /* Find the correct array of programs. */
            for (i = 0, programArray = Database->programs; i < arrayIndex; i++)
            {
                /* Check if we have a linked array of programs. */
                if (programArray->next == gcvNULL)
                {
                    /* Create a new array of programs. */
                    gcmONERROR(glshNewProgramArray(&programArray->next));
                }

                /* Get the next array of programs. */
                programArray = programArray->next;
            }
        }

        /* Store the program in the array. */
        programArray->programs[programIndex] = program;
    }

    /* Check if the program has changed. */
    if (Database->currentProgram != program)
    {
        /* Initialize the program record. */
        record.opcode  = glvDATABASE_OPCODE_PROGRAM;
        record.program = program;

        /* Add the program record to the database. */
        gcmONERROR(glshAddDatabase(Database,
                                   gcmSIZEOF(record), &record,
                                   gcvNULL));

        /* Save current progrtam. */
        Database->currentProgram = program;
    }

    /* Process all uniforms. */
    for (i = 0; i < program->uniformCount; i++)
    {
        /* Test whether uniform has changed. */
        if (!known || program->uniforms[i].dbDirty)
        {
            /* Initialize the uniform record. */
            uniform.opcode  = glvDATABASE_OPCODE_UNIFORM;
            uniform.uniform = i;
            uniform.bytes   = program->uniforms[i].bytes;

            /* Add the uniform record to the database. */
            gcmONERROR(glshAddDatabase(Database,
                                       gcmSIZEOF(uniform), &uniform,
                                       gcvNULL));

            /* Add the uniform data to the database. */
            gcmONERROR(glshAddDatabase(Database,
                                       program->uniforms[i].bytes,
                                       program->uniforms[i].data,
                                       gcvNULL));

            /* Mark uniform as not dirty. */
            program->uniforms[i].dbDirty = GL_FALSE;
        }
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

static gceSTATUS glshNewTextureArray(glsDATABASE_TEXTURE_ARRAY_PTR * Array)
{
    gceSTATUS                       status;
    glsDATABASE_TEXTURE_ARRAY_PTR   memory = gcvNULL;

    gcmHEADER();

    /* Allocate a new array of textures. */
    gcmONERROR(gcoOS_Allocate(gcvNULL,
                              gcmSIZEOF(glsDATABASE_TEXTURE_ARRAY),
                              (gctPOINTER *) &memory));

    /* Zero the array of textures. */
    gcmONERROR(gcoOS_ZeroMemory(memory, gcmSIZEOF(glsDATABASE_TEXTURE_ARRAY)));

    /* Return the pointer to the texture array. */
    *Array = memory;

    /* Success. */
    gcmFOOTER_ARG("*Array=%p", memory);
    return gcvSTATUS_OK;

OnError:
    /* Roll back the allocation. */
    if (memory != gcvNULL)
    {
        gcmVERIFY_OK(gcoOS_Free(gcvNULL, memory));
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}

static gceSTATUS glshAddTexture(GLContext Context,
                                glsDATABASE_PTR Database,
                                GLTexture Texture,
                                GLuint Sampler)
{
    gceSTATUS                       status;
    GLProgram                       program;
    gctINT                          i;
    glsDATABASE_TEXTURE             record;
    glsDATABASE_TEXTURE_ARRAY_PTR   textureArray;
    gctBOOL                         known;
    gctINT                          arrayIndex;
    gctINT                          textureIndex;

    gcmHEADER_ARG("Context=%p Database=%p Texture=%p Sampler=%u",
                  Context, Database, Texture, Sampler);

    /* Convert program name into array and program index. */
    arrayIndex   = Texture->object.name / gcmCOUNTOF(textureArray->textures);
    textureIndex = Texture->object.name % gcmCOUNTOF(textureArray->textures);

    /* Find the correct array of textures. */
    for (i = 0, textureArray = Database->textures; i < arrayIndex; i++)
    {
        /* Check if we have a valid array of textures. */
        if (textureArray == gcvNULL)
        {
            break;
        }

        /* Get the next array of textures. */
        textureArray = textureArray->next;
    }

    /* Check if the texture is known. */
    known = ((textureArray != gcvNULL)
             &&
             (textureArray->textures[textureIndex] == Texture));

    /* Check if we need to add this texture to the linked list. */
    if (!known)
    {
        /* Check if we don't have the correct array of textures. */
        if (textureArray == gcvNULL)
        {
            /* Check if we have any array of textures. */
            if (Database->textures == gcvNULL)
            {
                /* Create a new array of textures. */
                gcmONERROR(glshNewTextureArray(&Database->textures));
            }

            /* Find the correct array of textures. */
            for (i = 0, textureArray = Database->textures; i < arrayIndex; i++)
            {
                /* Check if we have a linked array of textures. */
                if (textureArray->next == gcvNULL)
                {
                    /* Create a new array of textures. */
                    gcmONERROR(glshNewTextureArray(&textureArray->next));
                }

                /* Get the next array of textures. */
                textureArray = textureArray->next;
            }
        }

        /* Store the texture in the array. */
        textureArray->textures[textureIndex] = Texture;
    }

    /* Get current program. */
    program = Context->program;

    /* Initialize the texture record. */
    record.opcode  = glvDATABASE_OPCODE_TEXTURE;
    record.flags   = 0;
    record.sampler = Sampler;

    /* Check for changes in the texture object. */
    if (!known || (program->savedTextures[Sampler] != Texture))
    {
        /* Copy the changes. */
        record.flags  |= GLDB_TEXTURE_OBJECT;
        record.texture = Texture;
    }

    /* Only process the next changes when the texture object is valid. */
    if (Texture != gcvNULL)
    {
        /* Check for changes in the texture minification filter. */
        if (!known || (Texture->savedMinFilter != Texture->minFilter))
        {
            /* Copy the changes. */
            record.flags    |= GLDB_TEXTURE_MIN_FILTER;
            record.minFilter = Texture->minFilter;
        }

        /* Check for changes in the texture magnification filter. */
        if (!known || (Texture->savedMagFilter != Texture->magFilter))
        {
            /* Copy the changes. */
            record.flags    |= GLDB_TEXTURE_MAG_FILTER;
            record.magFilter = Texture->magFilter;
        }

        /* Check for changes in the texture anisotropic filter. */
        if (!known || (Texture->savedAnisoFilter != Texture->anisoFilter))
        {
            /* Copy the changes. */
            record.flags      |= GLDB_TEXTURE_ANISO_FILTER;
            record.anisoFilter = Texture->anisoFilter;
        }

        /* Check for changes in the texture s coordinate wrap. */
        if (!known || (Texture->savedWrapS != Texture->wrapS))
        {
            /* Copy the changes. */
            record.flags |= GLDB_TEXTURE_WRAP_S;
            record.wrapS  = Texture->wrapS;
        }

        /* Check for changes in the texture t coordinate wrap. */
        if (!known || (Texture->savedWrapT != Texture->wrapT))
        {
            /* Copy the changes. */
            record.flags |= GLDB_TEXTURE_WRAP_T;
            record.wrapT  = Texture->wrapT;
        }

        /* Check for changes in the texture r coordinate wrap. */
        if (!known || (Texture->savedWrapR != Texture->wrapR))
        {
            /* Copy the changes. */
            record.flags |= GLDB_TEXTURE_WRAP_R;
            record.wrapR  = Texture->wrapR;
        }

        /* Check for changes in the texture maximum level. */
        if (!known || (Texture->savedMaxLevel != Texture->maxLevel))
        {
            /* Copy the changes. */
            record.flags   |= GLDB_TEXTURE_MAX_LEVEL;
            record.maxLevel = Texture->maxLevel;
        }

        /* Check if this texture object is attached to a framebuffer
         * object. */
        if ((Texture->owner != gcvNULL)
            &&
            (Texture->owner->currentDB != gcvNULL)
            )
        {
            /* Add the dependency. */
            gcmONERROR(glshAddDependency(Database, Texture->owner->currentDB));
        }
    }

    /* Test for any changes. */
    if (record.flags != 0)
    {
        /* Add the record to the database. */
        gcmONERROR(glshAddDatabase(Database,
                                   gcmSIZEOF(record), &record,
                                   gcvNULL));

        /* Copy current states. */
        program->savedTextures[Sampler] = Texture;

        if (Texture != gcvNULL)
        {
            /* Copy current states. */
            Texture->savedMinFilter   = Texture->minFilter;
            Texture->savedMagFilter   = Texture->magFilter;
            Texture->savedAnisoFilter = Texture->anisoFilter;
            Texture->savedWrapS       = Texture->wrapS;
            Texture->savedWrapT       = Texture->wrapT;
            Texture->savedWrapR       = Texture->wrapR;
            Texture->savedMaxLevel    = Texture->maxLevel;
        }
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

static gceSTATUS glshAddTextures(GLContext Context, glsDATABASE_PTR Database)
{
    gceSTATUS       status;
    GLProgram       program;
    gctINT          samplers;
    gctINT          i;
    gctINT          unit;
    GLTexture       texture;

    gcmHEADER_ARG("Context=%p Database=%p", Context, Database);

    /* Get current program. */
    program = Context->program;
    if (program == gcvNULL)
    {
        /* No program. */
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    /* Get number of samplers. */
    samplers = program->vertexSamplers + program->fragmentSamplers;

    /* Process all samplers. */
    for (i = 0; i < samplers; i++)
    {
        /* Get the linked texture sampler number. */
        unit = program->sampleMap[i].unit;

        /* Dispatch on the sampler type. */
        switch (program->sampleMap[i].type)
        {
            case gcSHADER_SAMPLER_2D:
                /* Get the 2D sampler. */
                texture = Context->texture2D[unit];
                if (texture == gcvNULL)
                {
                    /* Revert to default sampler. */
                    texture = &Context->default2D;
                }
                break;

            case gcSHADER_SAMPLER_3D:
                /* Get the 3D sampler */
                texture = Context->texture3D[unit];
                if (texture == gcvNULL)
                {
                    /* Revert to default sampler */
                    texture = &Context->default3D;
                }
                break;

            case gcSHADER_SAMPLER_CUBIC:
                /* Get the cubic sampler. */
                texture = Context->textureCube[unit];
                if (texture == gcvNULL)
                {
                    /* Revert to default sampler. */
                    texture = &Context->defaultCube;
                }
                break;

            case gcSHADER_SAMPLER_EXTERNAL_OES:
                /* Get the external sampler. */
                texture = Context->textureExternal[unit];
                if (texture == gcvNULL)
                {
                    /* Revert to default sampler. */
                    texture = &Context->defaultExternal;
                }
                break;

            default:
                texture = gcvNULL;
                break;
        }

        /* Check for NULL textures or invalid target. */
        if ((texture == gcvNULL)
            ||
            (texture->texture == gcvNULL)
            ||
            ((texture->target != GL_TEXTURE_2D)
             &&
             (texture->target != GL_TEXTURE_CUBE_MAP)
             &&
             (texture->target != GL_TEXTURE_3D_OES)
             &&
             (texture->target != GL_TEXTURE_EXTERNAL_OES)
             )
            )
        {
            /* Invalid texture. */
            texture = gcvNULL;
        }

        /* Add this texture object to the database. */
        gcmONERROR(glshAddTexture(Context, Database, texture, i));
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

static gceSTATUS glshDeltaStates(GLContext Context, glsDATABASE_PTR Database)
{
    gceSTATUS   status;

    gcmHEADER_ARG("Context=%p Database=%p", Context, Database);

    /* Test for a change in the scissors. */
    gcmONERROR(glshAddScissors(Context, Database));

    /* Test for a change in the polygon. */
    gcmONERROR(glshAddPolygon(Context, Database));

    /* Test for a change in the blending. */
    gcmONERROR(glshAddBlending(Context, Database));

    /* Test for a change in the pixel. */
    gcmONERROR(glshAddPixel(Context, Database));

    /* Test for a change in the depth. */
    gcmONERROR(glshAddDepth(Context, Database));

    /* Test for a change in the stencil. */
    gcmONERROR(glshAddStencil(Context, Database));

    /* Add program. */
    gcmONERROR(glshAddProgram(Context, Database));

    /* Add textures. */
    gcmONERROR(glshAddTextures(Context, Database));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}


gceSTATUS glshAddFlush(GLContext Context)
{
    gceSTATUS           status;
    glsDATABASE_PTR     db;
    gleDATABASE_OPCODE  opcode;

    gcmHEADER_ARG("Context=%p", Context);

    /* Get the current database. */
    gcmONERROR(glshSelectDatabase(Context->framebuffer, &db));

    /* Flush requires just an opcode. */
    opcode = glvDATABASE_OPCODE_FLUSH;

    /* Add the flush record to the database. */
    gcmONERROR(glshAddDatabase(db, gcmSIZEOF(opcode), &opcode, gcvNULL));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS glshAddFinish(GLContext Context)
{
    gceSTATUS           status;
    glsDATABASE_PTR     db;
    gleDATABASE_OPCODE  opcode;

    gcmHEADER_ARG("Context=%p", Context);

    /* Get the current database. */
    gcmONERROR(glshSelectDatabase(Context->framebuffer, &db));

    /* Finish requires just an opcode. */
    opcode = glvDATABASE_OPCODE_FINISH;

    /* Add the flush record to the database. */
    gcmONERROR(glshAddDatabase(db, gcmSIZEOF(opcode), &opcode, gcvNULL));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS glshAddClear(GLContext Context, GLbitfield Mask)
{
    gceSTATUS           status;
    glsDATABASE_PTR     db;
    gctBOOL             partialColor;
    gctBOOL             partialStencil;
    gctBOOL             empty = gcvFALSE;
    glsDATABASE_CLEAR   clear;

    gcmHEADER_ARG("Context=%p Mask=0x%x", Context, Mask);

    /* Get the current database. */
    gcmONERROR(glshSelectDatabase(Context->framebuffer, &db));

    /* Determine if all color channels are enabled. */
    partialColor = (!Context->colorEnableRed
                    ||
                    !Context->colorEnableGreen
                    ||
                    !Context->colorEnableGreen
                    ||
                    !Context->colorEnableAlpha
                    );

    /* Determine if the stencil write mask is full. */
    partialStencil = (Context->stencilWriteMask & 0xFF) != 0xFF;

    /* This is a full-screen clear if the following conditions are met:
     * - Scissor is turned off.
     * - For a color buffer clear, all color channels are enabled.
     * - For a stencil clear, the stencil mask is set to full.
     *
     * If these conditions are met - we can clean up any records in the
     * database. */
    if (!Context->scissorEnable && (db->first != gcvNULL))
    {
        /* Check if there is a color attachment to the framebuffer. */
        if (Context->framebuffer->color.object != gcvNULL)
        {
            /* If we are clearing the color buffer, we can empty the
             * database if we don't have partial color channels. */
            empty = (Mask & GL_COLOR_BUFFER_BIT) ? !partialColor : GL_FALSE;
        }

        /* Check if there is a depth attachment to the framebuffer. */
        else if (Context->framebuffer->depth.object != gcvNULL)
        {
            /* If we are clearing the depth buffer, we can empty the
             * database. */
            empty = (Mask & GL_DEPTH_BUFFER_BIT) ? GL_TRUE : GL_FALSE;
        }

        /* Check if there is a stencil attachment to the framebuffer. */
        else if (Context->framebuffer->stencil.object != gcvNULL)
        {
            /* If we are clearing the stencil buffer, we can empty the
             * database if we don't have partial stencil mask. */
            empty = (Mask & GL_STENCIL_BUFFER_BIT) ? !partialStencil : GL_FALSE;
        }
    }

    /* Check if we can empty the database. */
    if (empty)
    {
        if (++db->counter == Context->dbCounter)
        {
            /* Play the database. */
            gcmONERROR(glshPlayDatabase(Context, db));

            /* Reset counter. */
            db->counter = 0;
        }

        /* Clean up the database. */
        gcmONERROR(glshCleanDatabase(db));
    }

    /* If scissors are enabled, or if we have partial color channel cleear or a
     * partial stencil write mask clear, we need to add them to the database. */
    if (Context->scissorEnable
        ||
        ((Mask & GL_COLOR_BUFFER_BIT) && partialColor)
        ||
        ((Mask & GL_STENCIL_BUFFER_BIT) && partialStencil)
        )
    {
        /* Set initial states. */
        gcmONERROR(glshInitialStates(Context, db));
    }

    /* If there is already batch data present, we have to append the clear as a
     * record. */
    if (db->first != gcvNULL)
    {
        /* Test if the scissors are different. */
        gcmONERROR(glshAddScissors(Context, db));

        /* Fill in the database record. */
        clear.opcode  = glvDATABASE_OPCODE_CLEAR;
        clear.mask    = Mask;
        clear.red     = Context->clearRed;
        clear.green   = Context->clearGreen;
        clear.blue    = Context->clearBlue;
        clear.alpha   = Context->clearAlpha;
        clear.depth   = Context->clearDepth;
        clear.stencil = Context->clearStencil;

        /* Add the record to the database. */
        gcmONERROR(glshAddDatabase(db, gcmSIZEOF(clear), &clear, gcvNULL));
    }
    else
    {
        if (Mask & GL_COLOR_BUFFER_BIT)
        {
            /* Enable clearing of color with current clear value. */
            db->clear.mask |= GL_COLOR_BUFFER_BIT;
            db->clear.red   = Context->clearRed;
            db->clear.green = Context->clearGreen;
            db->clear.blue  = Context->clearBlue;
            db->clear.alpha = Context->clearAlpha;
        }

        if (Mask & GL_DEPTH_BUFFER_BIT)
        {
            /* Enable clearing of depth with current clear value. */
            db->clear.mask |= GL_DEPTH_BUFFER_BIT;
            db->clear.depth = Context->clearDepth;
        }

        if (Mask & GL_STENCIL_BUFFER_BIT)
        {
            /* Enable clearing of stencil with current clear value. */
            db->clear.mask   |= GL_STENCIL_BUFFER_BIT;
            db->clear.stencil = Context->clearStencil;
        }
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS glshAddDrawElements(GLContext Context, GLenum Mode, GLsizei Count,
                              GLenum Type, const GLvoid * Indices)
{
    gceSTATUS                   status;
    gctINT                      i;
    glsDATABASE_DRAW_ELEMENTS   draw;
    GLsizei                     iElement;
    gctUINT                     minVertex = ~0U;
    gctUINT                     maxVertex = 0;
    gctUINT                     vertexCount = 0;
    gluELEMENT                  element;
    gctUINT                     stride;
    gctUINT8_PTR                data;
    glsDATABASE_PTR             db;
    gctPOINTER                  location;

    gcmHEADER_ARG("Context=%p Mode=0x%x Count=%d Type=0x%x Indices=%p",
                  Context, Mode, Count, Type, Indices);

    /* Get the current database. */
    gcmONERROR(glshSelectDatabase(Context->framebuffer, &db));

    /* Copy currently bound element buffer. */
    draw.elementArrayBuffer = Context->elementArrayBuffer;
    draw.elementBytes       = 0;

    /* See if we need to copy the element data. */
    if (draw.elementArrayBuffer == gcvNULL)
    {
        /* Set element pointer. */
        element.ptr = Indices;

        /* Dispatch on element type. */
        switch (Type)
        {
            case GL_UNSIGNED_BYTE:
                /* Number of bytes. */
                draw.elementBytes = Count;

                /* Process all elements. */
                for (iElement = 0; iElement < Count; iElement++)
                {
                    /* Check for a smaller element. */
                    if (*element.ptr8 < minVertex)
                    {
                        minVertex = *element.ptr8;
                    }

                    /* Check for a larger element. */
                    if (*element.ptr8 > maxVertex)
                    {
                        maxVertex = *element.ptr8;
                    }

                    /* Next element. */
                    element.ptr8++;
                }
                break;

            case GL_UNSIGNED_SHORT:
                /* Number of bytes. */
                draw.elementBytes = Count * 2;

                /* Process all elements. */
                for (iElement = 0; iElement < Count; iElement++)
                {
                    /* Check for a smaller element. */
                    if (*element.ptr16 < minVertex)
                    {
                        minVertex = *element.ptr16;
                    }

                    /* Check for a larger element. */
                    if (*element.ptr16 > maxVertex)
                    {
                        maxVertex = *element.ptr16;
                    }

                    /* Next element. */
                    element.ptr16++;
                }
                break;

            case GL_UNSIGNED_INT:
                /* Number of bytes. */
                draw.elementBytes = Count * 4;

                /* Process all elements. */
                for (iElement = 0; iElement < Count; iElement++)
                {
                    /* Check for a smaller element. */
                    if (*element.ptr32 < minVertex)
                    {
                        minVertex = *element.ptr32;
                    }

                    /* Check for a larger element. */
                    if (*element.ptr32 > maxVertex)
                    {
                        maxVertex = *element.ptr32;
                    }

                    /* Next element. */
                    element.ptr32++;
                }
                break;
        }
    }
    else
    {
        /* Dispatch on type. */
        switch (Type)
        {
            case GL_UNSIGNED_BYTE:
                /* Get min/max index range. */
                gcoINDEX_GetIndexRange(draw.elementArrayBuffer->index,
                                       gcvINDEX_8,
                                       (gctUINT32) Indices,
                                       Count,
                                       &minVertex,
                                       &maxVertex);
                break;

            case GL_UNSIGNED_SHORT:
                /* Get min/max index range. */
                gcoINDEX_GetIndexRange(draw.elementArrayBuffer->index,
                                       gcvINDEX_16,
                                       (gctUINT32) Indices,
                                       Count,
                                       &minVertex,
                                       &maxVertex);
                break;

            case GL_UNSIGNED_INT:
                /* Get min/max index range. */
                gcoINDEX_GetIndexRange(draw.elementArrayBuffer->index,
                                       gcvINDEX_32,
                                       (gctUINT32) Indices,
                                       Count,
                                       &minVertex,
                                       &maxVertex);
                break;
        }
    }

    /* Compute vertex count. */
    vertexCount = maxVertex - minVertex + 1;

    /* Copy all vertex information. */
    for (i = 0; i < 16; i++)
    {
        /* Copy vertex array information. */
        draw.vertices[i].vertexArray = Context->vertexArray[i];
        draw.vertices[i].bytes       = 0;

        /* Check if we need to copy the vertex data. */
        if (draw.vertices[i].vertexArray.enable
            &&
            (draw.vertices[i].vertexArray.stream == gcvNULL)
            )
        {
            /* Get the vertex stride. */
            stride = draw.vertices[i].vertexArray.stride;

            /* Save number of vertex bytes to copy. */
            draw.vertices[i].bytes = stride * vertexCount;

            /* Save the offset into the vertex buffer. */
            draw.vertices[i].offset = stride * minVertex;
        }
    }

    /* Fill in draw record. */
    draw.opcode  = glvDATABASE_OPCODE_DRAW_ELEMENTS;
    draw.mode    = Mode;
    draw.count   = Count;
    draw.type    = Type;
    draw.indices = Indices;

    /* Check if ths draw is the same as the last known draw and alpha blending
     * is turned off. In this case, we can just drop the draw, since all
     * vertices are the same, all states are the same, etc. */
    if ((db->lastDrawElements != gcvNULL)
        &&
        !db->initialStates.blending.enable
        &&
        !db->currentStates.blending.enable
        &&
        (db->initialStates.depth.test == GL_FALSE)
        &&
        (db->currentStates.depth.test == GL_FALSE)
        &&
        (db->initialStates.stencil.test == GL_FALSE)
        &&
        (db->currentStates.stencil.test == GL_FALSE)
        &&
        (db->first != gcvNULL)
        &&
        (db->clear.mask == 0)
        &&
        gcmIS_SUCCESS(gcoOS_MemCmp(db->lastDrawElements,
                                   &draw,
                                   gcmSIZEOF(draw)))
        )
    {
        /* Clean the database. */
        gcmONERROR(glshCleanDatabase(db));
    }

    /* Save delta of the batch in database. */
    gcmONERROR(glshDeltaStates(Context, db));

    /* Add draw record to the database. */
    gcmONERROR(glshAddDatabase(db, gcmSIZEOF(draw), &draw, &location));

    /* Check if any last draw elements matches this one and belnding is
     * enabled with GL_ONE equations for both source and destination. */
    if ((db->lastDrawElements != gcvNULL)
        &&
        db->currentStates.blending.enable
        &&
        (db->currentStates.blending.funcSourceRGB == GL_ONE)
        &&
        (db->currentStates.blending.funcDestinationRGB == GL_ONE)
        &&
        gcmIS_SUCCESS(gcoOS_MemCmp(db->lastDrawElements,
                                   &draw,
                                   gcmSIZEOF(draw)))
        )
    {
        /* This is for the GLBenchmark framebuffer pre test. It runs an
         * iteration of glClear(), glDrawElements(), and glFlush() without any
         * dependency. And they want to know how many we can send through in
         * 250ms. The result is an insane number of iterations which actually
         * will fail the test since the number overflows to negative. So, what
         * we do here (until a better solution comes into mind) is delay some
         * small time to reduce the required number of iterations. This might
         * cause delays in other programs - we will have to see. */

        gcoOS_Delay(gcvNULL, 10);
    }

    /* Store the pointer of this draw element record. */
    db->lastDrawElements = location;

    /* Check if we need to copy the element data. */
    if (draw.elementBytes > 0)
    {
        /* Add the element data to the database. */
        gcmONERROR(glshAddDatabase(db, draw.elementBytes, Indices, gcvNULL));
    }

    for (i = 0; i < 16; i++)
    {
        if (draw.vertices[i].bytes > 0)
        {
            /* Compute location of vertex data. */
            data = ((gctUINT8_PTR) draw.vertices[i].vertexArray.pointer +
                    draw.vertices[i].offset);

            /* Add the vertex data to the database. */
            gcmONERROR(glshAddDatabase(db,
                                       draw.vertices[i].bytes, data,
                                       gcvNULL));
        }
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS glshAddDrawArrays(GLContext Context, GLenum Mode, GLint First,
                            GLsizei Count)
{
    gceSTATUS               status;
    glsDATABASE_PTR         db;
    gctINT                  i;
    glsDATABASE_DRAW_ARRAYS draw;
    gctUINT                 stride;
    gctUINT8_PTR            data;

    gcmHEADER_ARG("Context=%p Mode=0x%x First=%d Count=%d",
                  Context, Mode, First, Count);

    /* Get the current database. */
    gcmONERROR(glshSelectDatabase(Context->framebuffer, &db));

    /* Copy all vertex information. */
    for (i = 0; i < 16; i++)
    {
        /* Copy vertex array information. */
        draw.vertices[i].vertexArray = Context->vertexArray[i];
        draw.vertices[i].bytes       = 0;

        /* Check if we need to copy the vertex data. */
        if (draw.vertices[i].vertexArray.enable
            &&
            (draw.vertices[i].vertexArray.stream == gcvNULL)
            )
        {
            /* Get the vertex stride. */
            stride = draw.vertices[i].vertexArray.stride;

            /* Save number of vertex bytes to copy. */
            draw.vertices[i].bytes = stride * Count;

            /* Save the offset into the vertex buffer. */
            draw.vertices[i].offset = stride * First;
        }
    }

    /* Save delta of the batch in database. */
    gcmONERROR(glshDeltaStates(Context, db));

    /* Fill in draw record. */
    draw.opcode = glvDATABASE_OPCODE_DRAW_ARRAYS;
    draw.mode   = Mode;
    draw.first  = First;
    draw.count  = Count;

    /* Add draw record to the database. */
    gcmONERROR(glshAddDatabase(db, gcmSIZEOF(draw), &draw, gcvNULL));

    for (i = 0; i < 16; i++)
    {
        if (draw.vertices[i].bytes > 0)
        {
            /* Compute location of vertex data. */
            data = ((gctUINT8_PTR) draw.vertices[i].vertexArray.pointer +
                    draw.vertices[i].offset);

            /* Add the vertex data to the database. */
            gcmONERROR(glshAddDatabase(db,
                                       draw.vertices[i].bytes, data,
                                       gcvNULL));
        }
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}


gceSTATUS glshSelectDatabase(GLFramebuffer Framebuffer,
                             glsDATABASE_PTR * Database)
{
    gceSTATUS   status;

    gcmHEADER_ARG("Framebuffer=%p", Framebuffer);

    /* If there is no current database or its reference is not zero, create a
     * new database. */
    if ((Framebuffer->currentDB == gcvNULL)
        ||
        (Framebuffer->currentDB->reference != 0)
        )
    {
        /* Create a new database. */
        gcmONERROR(glshNewDatabase(Framebuffer, &Framebuffer->currentDB));
    }

    /* Return the current database pointer. */
    *Database = Framebuffer->currentDB;

    /* Success. */
    gcmFOOTER_ARG("*Database=%p", *Database);
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS glshCleanDatabase(glsDATABASE_PTR Database)
{
    gceSTATUS                       status;
    glsDATABASE_PROGRAM_ARRAY_PTR   programArray;
    glsDATABASE_TEXTURE_ARRAY_PTR   textureArray;

    gcmHEADER_ARG("Database=%p", Database);

    /* Remove all program arrays. */
    while (Database->programs != gcvNULL)
    {
        /* Get current program array. */
        programArray = Database->programs;

        /* Remove program array from the linked list. */
        Database->programs = programArray->next;

        /* Free the program array. */
        gcmONERROR(gcoOS_Free(gcvNULL, programArray));
    }

    /* Remove all texture arrays. */
    while (Database->textures != gcvNULL)
    {
        /* Get current texture array. */
        textureArray = Database->textures;

        /* Remove texture array from the linked list. */
        Database->textures = textureArray->next;

        /* Free the texture array. */
        gcmONERROR(gcoOS_Free(gcvNULL, textureArray));
    }

    /* Remove all dependencies. */
    gcmONERROR(glshRemoveDependencies(Database));

    gcmTRACE_ZONE(gcvLEVEL_INFO, gldZONE_DATABASE,
                  "Cleaning database of %lu bytes.",
                  Database->size);

    /* Loop while there are databases in the linked list. */
    while (Database->first != gcvNULL)
    {
        /* Delete this record. */
        gcmONERROR(glshDeleteRecord(Database));
    }

    /* All record pools are removed. */
    Database->last = gcvNULL;
    Database->size = 0;

    /* Remove the clear from the database. */
    Database->clear.mask = 0;

    /* No draw elements record. */
    Database->lastDrawElements = gcvNULL;

    /* Clear reference. */
    Database->reference = 0;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS glshRemoveDatabase(GLFramebuffer Framebuffer)
{
    gceSTATUS   status;
    gctINT      i;

    gcmHEADER_ARG("Framebuffer=%p", Framebuffer);

    /* Process all databases. */
    while (Framebuffer->listDB != gcvNULL)
    {
        /* Clean the database. */
        gcmONERROR(glshCleanDatabase(Framebuffer->listDB));

        /* Delete the database. */
        gcmONERROR(glshDeleteDatabase(Framebuffer->listDB));
    }

    /* Process all databases in the free list. */
    for (i = 0; i < Framebuffer->freeDBCount; i++)
    {
        /* Free the database structure. */
        gcmONERROR(gcoOS_Free(gcvNULL, Framebuffer->freeDB[i]));
    }

    /* No more free databases. */
    Framebuffer->freeDBCount = 0;

    /* Process all records in the free list. */
    for (i = 0; i < Framebuffer->freeRecordCount; i++)
    {
        /* Free the record array. */
        gcmONERROR(gcoOS_Free(gcvNULL, Framebuffer->freeRecord[i]));
    }

    /* No more free records. */
    Framebuffer->freeRecordCount = 0;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}


static gceSTATUS glshGetRecord(glsDATABASE_PTR Database,
                               gctSIZE_T MoveBytes,
                               gctPOINTER * Location)
{
    gceSTATUS   status;

    gcmHEADER_ARG("Database=%p MoveBytes=%u", Database, MoveBytes);

    /* Adjust the current play offset. */
    Database->playOffset += MoveBytes;

    /* Check whether the offset is beyond the current record. */
    if ((Database->first != gcvNULL)
        &&
        (Database->playOffset >= Database->first->offset)
        )
    {
        /* Free the current database record. */
        gcmONERROR(glshDeleteRecord(Database));

        /* Reset the current offset. */
        Database->playOffset = 0;
    }

    /* Check if we have a valid record array. */
    if (Database->first == gcvNULL)
    {
        /* No more data. */
        gcmFOOTER_ARG("status=%s", "gcvSTATUS_NO_MORE_DATA");
        return gcvSTATUS_NO_MORE_DATA;
    }

    /* Return the current pointer. */
    *Location = Database->first->data + Database->playOffset;

    /* Success. */
    gcmFOOTER_ARG("*Location=%p", *Location);
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

static gctSIZE_T glshPlayClear(GLContext Context, glsDATABASE_CLEAR * Clear)
{
    GLbitfield  changed = 0;
    GLclampf    red = 0, green = 0, blue = 0, alpha = 0;
    GLclampf    depth = 0;
    GLint       stencil = 0;

    gcmHEADER_ARG("Context=%p Clear=%p", Context, Clear);

    /* Test for clearing the color buffer. */
    if (Clear->mask & GL_COLOR_BUFFER_BIT)
    {
        /* Save current clear color value. */
        red   = Context->clearRed;
        green = Context->clearGreen;
        blue  = Context->clearBlue;
        alpha = Context->clearAlpha;

        /* Test if the clear color value is different. */
        if ((red != Clear->red)
            ||
            (green != Clear->green)
            ||
            (blue != Clear->blue)
            ||
            (alpha != Clear->alpha)
            )
        {
            /* Change the color clear value. */
            glClearColor(Clear->red, Clear->green, Clear->blue, Clear->alpha);

            /* Mark color clear value as changed. */
            changed |= GL_COLOR_BUFFER_BIT;
        }
    }

    /* Test for clearing the depth buffer. */
    if (Clear->mask & GL_DEPTH_BUFFER_BIT)
    {
        /* Save the current depth clear value. */
        depth = Context->clearDepth;

        /* Test if the depth clear value is different. */
        if (depth != Clear->depth)
        {
            /* Change the depth clear value. */
            glClearDepthf(Clear->depth);

            /* Mark depth clear value as changed. */
            changed |= GL_DEPTH_BUFFER_BIT;
        }
    }

    /* Test for clearing the stencil buffer. */
    if (Clear->mask & GL_STENCIL_BUFFER_BIT)
    {
        /* Save the current stencil clear value. */
        stencil = Context->clearStencil;

        /* Test if the stencil clear value si different. */
        if (stencil != Clear->stencil)
        {
            /* Change the stencil clear value. */
            glClearStencil(Clear->stencil);

            /* Mark the stencil clear value as changed. */
            changed |= GL_STENCIL_BUFFER_BIT;
        }
    }

    /* Clear the framebuffer. */
    glClear(Clear->mask);

    if (changed & GL_COLOR_BUFFER_BIT)
    {
        /* Reset color clear value. */
        glClearColor(red, green, blue, alpha);
    }

    if (changed & GL_DEPTH_BUFFER_BIT)
    {
        /* Reset depth clear value. */
        glClearDepthf(depth);
    }

    if (changed & GL_STENCIL_BUFFER_BIT)
    {
        /* Reset stencil clear value. */
        glClearStencil(stencil);
    }

    /* Return size of record. */
    gcmFOOTER_ARG("%u", gcmSIZEOF(*Clear));
    return gcmSIZEOF(*Clear);
}

static gctSIZE_T glshPlayScissors(GLContext Context,
                                  glsDATABASE_SCISSORS * Scissors,
                                  gctBOOL AllStates)
{
    gcmHEADER_ARG("Context=%p Scissors=%p AllStates=%d",
                  Context, Scissors, AllStates);

    /* Test for a change in the scissors enable flag. */
    if ((AllStates || (Scissors->flags & GLDB_SCISSOR_ENABLE))
        &&
        (Scissors->enable != Context->scissorEnable)
        )
    {
        /* Set scissor test. */
        glmENABLE_DISABLE(Scissors->enable, GL_SCISSOR_TEST);
    }

    /* Test for a change in the scissors box. */
    if (AllStates || (Scissors->flags & GLDB_SCISSOR_BOX))
    {
        /* Set scissors box. */
        glScissor(Scissors->box.x, Scissors->box.y,
                  Scissors->box.width, Scissors->box.height);
    }

    /* Return size of record. */
    gcmFOOTER_ARG("%u", gcmSIZEOF(*Scissors));
    return gcmSIZEOF(*Scissors);
}

static gctSIZE_T glshPlayPolygon(GLContext Context,
                                 glsDATABASE_POLYGON * Polygon,
                                 gctBOOL AllStates)
{
    gcmHEADER_ARG("Context=%p Polygon=%p AllStates=%d",
                  Context, Polygon, AllStates);

    /* Test for a change in the cull enable flag. */
    if ((AllStates || (Polygon->flags & GLDB_POLYGON_CULL_ENABLE))
        &&
        (Polygon->cullEnable != Context->cullEnable)
        )
    {
        /* Set cull. */
        glmENABLE_DISABLE(Polygon->cullEnable, GL_CULL_FACE);
    }

    /* Test for a change in the cull mode. */
    if ((AllStates || (Polygon->flags & GLDB_POLYGON_CULL_MODE))
        &&
        (Polygon->cullMode != Context->cullMode)
        )
    {
        /* Set cull mode. */
        glCullFace(Polygon->cullMode);
    }

    /* Test for a change in the cull front face. */
    if ((AllStates || (Polygon->flags & GLDB_POLYGON_CULL_FRONT))
        &&
        (Polygon->cullFront != Context->cullFront)
        )
    {
        /* Set cull front face. */
        glFrontFace(Polygon->cullFront);
    }

    /* Test for a change in the line width. */
    if ((AllStates || (Polygon->flags & GLDB_POLYGON_LINE_WIDTH))
        &&
        (Polygon->lineWidth != Context->lineWidth)
        )
    {
        /* Set line width. */
        glLineWidth((GLfloat)(Polygon->lineWidth));
    }

    /* Test for a change in the viewport box. */
    if (AllStates || (Polygon->flags & GLDB_POLYGON_VIEWPORT))
    {
        /* Set viewport. */
        glViewport(Polygon->viewport.x, Polygon->viewport.y,
                   Polygon->viewport.width, Polygon->viewport.height);
    }

    /* Test for a change in the depth range. */
    if (AllStates || (Polygon->flags & GLDB_POLYGON_DEPTH_RANGE))
    {
        /* Set the depth range. */
        glDepthRangef(Polygon->near, Polygon->far);
    }

    /* Test for a change in the offset enable flag. */
    if ((AllStates || (Polygon->flags & GLDB_POLYGON_OFFSET_ENABLE))
        &&
        (Polygon->offsetEnable != Context->offsetEnable)
        )
    {
        /* Set polygon offset. */
        glmENABLE_DISABLE(Polygon->offsetEnable, GL_POLYGON_OFFSET_FILL);
    }

    /* Test for a change in the offset. */
    if (AllStates || (Polygon->flags & GLDB_POLYGON_OFFSET))
    {
        /* Set polygon offset. */
        glPolygonOffset(Polygon->factor, Polygon->units);
    }

    /* Return size of record. */
    gcmFOOTER_ARG("%u", gcmSIZEOF(*Polygon));
    return gcmSIZEOF(*Polygon);
}

static gctSIZE_T glshPlayBlend(GLContext Context,
                               glsDATABASE_BLEND * Blend,
                               gctBOOL AllStates)
{
    gcmHEADER_ARG("Context=%p Blend=%p AllStates=%d",
                  Context, Blend, AllStates);

    /* Test for a change in the blend enable flag. */
    if ((AllStates || (Blend->flags & GLDB_BLEND_ENABLE))
        &&
        (Blend->enable != Context->blendEnable)
        )
    {
        /* Set blending. */
        glmENABLE_DISABLE(Blend->enable, GL_BLEND);
    }

    /* Test for a change in the blend source function. */
    if (AllStates || (Blend->flags & GLDB_BLEND_FUNCTION))
    {
        /* Set blend function. */
        glBlendFuncSeparate(Blend->funcSourceRGB,
                            Blend->funcDestinationRGB,
                            Blend->funcSourceAlpha,
                            Blend->funcDestinationAlpha);
    }

    /* Test for a change in the blend mode. */
    if (AllStates || (Blend->flags & GLDB_BLEND_MODE))
    {
        /* Set blend equation. */
        glBlendEquationSeparate(Blend->modeRGB, Blend->modeAlpha);
    }

    /* Test for a change in the blend color. */
    if (AllStates || (Blend->flags & GLDB_BLEND_COLOR))
    {
        /* Set blend color. */
        glBlendColor(Blend->red, Blend->green, Blend->blue, Blend->alpha);
    }

    /* Return size of record. */
    gcmFOOTER_ARG("%u", gcmSIZEOF(*Blend));
    return gcmSIZEOF(*Blend);
}

static gctSIZE_T glshPlayPixel(GLContext Context,
                               glsDATABASE_PIXEL * Pixel,
                               gctBOOL AllStates)
{
    gcmHEADER_ARG("Context=%p Pixel=%p AllStates=%d",
                  Context, Pixel, AllStates);

    /* Test for a change in the pixel color enable flags. */
    if (AllStates || (Pixel->flags & GLDB_PIXEL_ENABLE))
    {
        /* Set color mask. */
        glColorMask(Pixel->red, Pixel->green, Pixel->blue, Pixel->alpha);
    }

    /* Test for a change in the dither enable flag. */
    if ((AllStates || (Pixel->flags & GLDB_PIXEL_DITHER))
        &&
        (Pixel->dither != Context->ditherEnable)
        )
    {
        /* Set dithering. */
        glmENABLE_DISABLE(Pixel->dither, GL_DITHER);
    }

    /* Return size of record. */
    gcmFOOTER_ARG("%u", gcmSIZEOF(*Pixel));
    return gcmSIZEOF(*Pixel);
}

static gctSIZE_T glshPlayDepth(GLContext Context,
                               glsDATABASE_DEPTH * Depth,
                               gctBOOL AllStates)
{
    gcmHEADER_ARG("Context=%p Depth=%p AllStates=%d",
                  Context, Depth, AllStates);

    /* Test for a change in the depth test enable flag. */
    if ((AllStates || (Depth->flags & GLDB_DEPTH_TEST))
        &&
        (Depth->test != Context->depthTest)
        )
    {
        /* Set depth test. */
        glmENABLE_DISABLE(Depth->test, GL_DEPTH_TEST);
    }

    /* Test for a change in the depth function. */
    if ((AllStates || (Depth->flags & GLDB_DEPTH_FUNC))
        &&
        (Depth->func != Context->depthFunc)
        )
    {
        /* Set depth function. */
        glDepthFunc(Depth->func);
    }

    /* Test for a change in the depth write mask. */
    if ((AllStates || (Depth->flags & GLDB_DEPTH_MASK))
        &&
        (Depth->mask != Context->depthMask)
        )
    {
        /* Set depth mask. */
        glDepthMask(Depth->mask);
    }

    /* Return size of record. */
    gcmFOOTER_ARG("%u", gcmSIZEOF(*Depth));
    return gcmSIZEOF(*Depth);
}

static gctSIZE_T glshPlayStencil(GLContext Context,
                                 glsDATABASE_STENCIL * Stencil,
                                 gctBOOL AllStates)
{
    gcmHEADER_ARG("Context=%p Stencil=%p AllStates=%d",
                  Context, Stencil, AllStates);

    /* Test for a change in the stencil test flag. */
    if ((AllStates || (Stencil->flags & GLDB_STENCIL_TEST))
        &&
        (Stencil->test != Context->stencilEnable)
        )
    {
        /* Set the stencil test. */
        glmENABLE_DISABLE(Stencil->test, GL_STENCIL_TEST);
    }

    /* Test for a change in the stencil front function. */
    if (AllStates || (Stencil->flags & GLDB_STENCIL_FRONT_FUNCTION))
    {
        /* Set the stencil front function, reference, and mask. */
        glStencilFuncSeparate(GL_FRONT,
                              Stencil->frontFunction,
                              Stencil->frontReference,
                              Stencil->frontMask);
    }

    /* Test for a change in the stencil front mask. */
    else if (Stencil->flags & GLDB_STENCIL_FRONT_MASK)
    {
        /* Set the stencil front mask. */
        glStencilMaskSeparate(GL_FRONT, Stencil->frontMask);
    }

    /* Test for a change in the stencil front operation. */
    if (AllStates || (Stencil->flags & GLDB_STENCIL_FRONT_OPERATION))
    {
        /* Set the stencil front operation. */
        glStencilOpSeparate(GL_FRONT,
                            Stencil->frontOpFail,
                            Stencil->frontOpDepthFail,
                            Stencil->frontOpDepthPass);
    }

    /* Test for a change in the stencil back function. */
    if (AllStates || (Stencil->flags & GLDB_STENCIL_BACK_FUNCTION))
    {
        /* Set the stencil back function, reference, and mask. */
        glStencilFuncSeparate(GL_BACK,
                              Stencil->backFunction,
                              Stencil->backReference,
                              Stencil->backMask);
    }

    /* Test for a change in the stencil back mask. */
    else if (Stencil->flags & GLDB_STENCIL_BACK_MASK)
    {
        /* Set the stencil back mask. */
        glStencilMaskSeparate(GL_BACK, Stencil->backMask);
    }

    /* Test for a change in the stencil back operation. */
    if (AllStates || (Stencil->flags & GLDB_STENCIL_BACK_OPERATION))
    {
        /* Set the stencil back operation. */
        glStencilOpSeparate(GL_BACK,
                            Stencil->backOpFail,
                            Stencil->backOpDepthFail,
                            Stencil->backOpDepthPass);
    }

    /* Return size of record. */
    gcmFOOTER_ARG("%u", gcmSIZEOF(*Stencil));
    return gcmSIZEOF(*Stencil);
}

static void glshPlayStates(GLContext Context,
                           glsDATABASE_STATES * States,
                           gctBOOL AllStates)
{
    gcmHEADER_ARG("Context=%p States=%p AllStates=%d",
                  Context, States, AllStates);

    /* Play all the states. */
    glshPlayScissors(Context, &States->scissors, AllStates);
    glshPlayPolygon(Context, &States->polygon, AllStates);
    glshPlayBlend(Context, &States->blending, AllStates);
    glshPlayPixel(Context, &States->pixel, AllStates);
    glshPlayDepth(Context, &States->depth, AllStates);
    glshPlayStencil(Context, &States->stencil, AllStates);

    gcmFOOTER_NO();
}

static gctSIZE_T glshPlayProgram(GLContext Context,
                                 glsDATABASE_PROGRAM * Program,
                                 GLProgram * ModifiedList)
{
    GLProgram   program;
    gctINT      i;
    GLUniform   uniform;

    gcmHEADER_ARG("Context=%p Program=%p ModifiedList=%p",
                  Context, Program, ModifiedList);

    /* Get requested program. */
    program = Program->program;

    /* Test if this program object needs to be saved. */
    if ((program != gcvNULL) && !program->modified)
    {
        /* Prepend this program object to the modified list. */
        program->nextModified = *ModifiedList;
        *ModifiedList         = program;
        program->modified     = GL_TRUE;

        /* Save current bound textures. */
        gcoOS_MemCopy(program->savedTextures,
                      program->textures,
                      gcmSIZEOF(program->savedTextures));

        /* Process all uniforms. */
        for (i = 0; i < program->uniformCount; i++)
        {
            /* Get the uniform. */
            uniform = &program->uniforms[i];

            /* Save the current uniform data. */
            gcoOS_MemCopy(uniform->savedData, uniform->data, uniform->bytes);
        }
    }

    /* Set program. */
    glshSetProgram(Context, Program->program);

    /* Return size of record. */
    gcmFOOTER_ARG("%u", gcmSIZEOF(*Program));
    return gcmSIZEOF(*Program);
}

static gctSIZE_T glshPlayUniform(GLContext Context,
                                 glsDATABASE_UNIFORM * Uniform,
                                 glsDATABASE_PTR Database)
{
    gceSTATUS   status;
    GLProgram   program;
    gctPOINTER  pointer;

    gcmHEADER_ARG("Context=%p Uniform=%p Database=%p",
                  Context, Uniform, Database);

    /* Get current program. */
    program = Context->program;

    /* Get the pointer to the uniform data. */
    gcmONERROR(glshGetRecord(Database, gcmSIZEOF(*Uniform), &pointer));

    /* Copy the data into the uniform. */
    gcoOS_MemCopy(program->uniforms[Uniform->uniform].data,
                  pointer,
                  Uniform->bytes);

    /* Mark the uniform dirty. */
    program->uniforms[Uniform->uniform].dirty = GL_TRUE;

    /* Return size of uniform data. */
    gcmFOOTER_ARG("%u", Uniform->bytes);
    return Uniform->bytes;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

static gctSIZE_T glshPlayTexture(GLContext Context,
                                 glsDATABASE_TEXTURE * Texture,
                                 GLTexture * ModifiedList)
{
    GLProgram   program;
    GLTexture   texture;

    gcmHEADER_ARG("Context=%p Texture=%p ModifiedList=%p",
                  Context, Texture, ModifiedList);

    /* Get the current program. */
    program = Context->program;

    /* Get the texture object. */
    texture = Texture->texture;

    /* Test if this texture object needs to be saved. */
    if ((texture != gcvNULL) && !texture->modified)
    {
        /* Prepend this texture object to the modified list. */
        texture->nextModified = *ModifiedList;
        *ModifiedList         = texture;
        texture->modified     = GL_TRUE;

        /* Save current states. */
        texture->savedMinFilter   = texture->minFilter;
        texture->savedMagFilter   = texture->magFilter;
        texture->savedAnisoFilter = texture->anisoFilter;
        texture->savedWrapS       = texture->wrapS;
        texture->savedWrapT       = texture->wrapT;
        texture->savedWrapR       = texture->wrapR;
        texture->savedMaxLevel    = texture->maxLevel;
    }

    /* Test for a change in the texture object. */
    if (Texture->flags & GLDB_TEXTURE_OBJECT)
    {
        /* Set the texture object. */
        program->textures[Texture->sampler] = texture;
    }

    /* Test for a change in the minification filter. */
    if (Texture->flags & GLDB_TEXTURE_MIN_FILTER)
    {
        /* Set the minification filter. */
        glshSetTexParameter(Context,
                            texture,
                            texture->target,
                            GL_TEXTURE_MIN_FILTER,
                            Texture->minFilter);
    }

    /* Test for a change in the magnifaction filter. */
    if (Texture->flags & GLDB_TEXTURE_MAG_FILTER)
    {
        /* Set the magnification filter. */
        glshSetTexParameter(Context,
                            texture,
                            texture->target,
                            GL_TEXTURE_MAG_FILTER,
                            Texture->magFilter);
    }

    /* Test for a change in the ansiotropic filter. */
    if (Texture->flags & GLDB_TEXTURE_ANISO_FILTER)
    {
        /* Set the anisotropic filter. */
        glshSetTexParameter(Context,
                            texture,
                            texture->target,
                            GL_TEXTURE_MAX_ANISOTROPY_EXT,
                            Texture->anisoFilter);
    }

    /* Test for a change in the S addressing mode. */
    if (Texture->flags & GLDB_TEXTURE_WRAP_S)
    {
        /* Set the S addressing mode. */
        glshSetTexParameter(Context,
                            texture,
                            texture->target,
                            GL_TEXTURE_WRAP_S,
                            Texture->wrapS);
    }

    /* Test for a change in the T addressing mode. */
    if (Texture->flags & GLDB_TEXTURE_WRAP_T)
    {
        /* Set the T addressing mode. */
        glshSetTexParameter(Context,
                            texture,
                            texture->target,
                            GL_TEXTURE_WRAP_T,
                            Texture->wrapT);
    }

    /* Test for a change in the R addressing mode. */
    if (Texture->flags & GLDB_TEXTURE_WRAP_R)
    {
        /* Set the R addressing mode. */
        glshSetTexParameter(Context,
                            texture,
                            texture->target,
                            GL_TEXTURE_WRAP_R_OES,
                            Texture->wrapR);
    }

    /* Test for a change in the maximum level. */
    if (Texture->flags & GLDB_TEXTURE_MAX_LEVEL)
    {
        /* Set maximum level. */
        glshSetTexParameter(Context,
                            texture,
                            texture->target,
                            GL_TEXTURE_MAX_LEVEL_APPLE,
                            Texture->maxLevel);
    }

    /* Return size of record. */
    gcmFOOTER_ARG("%u", gcmSIZEOF(*Texture));
    return gcmSIZEOF(*Texture);
}

static gctSIZE_T glshPlayDrawElements(GLContext Context,
                                      glsDATABASE_DRAW_ELEMENTS * Elements,
                                      glsDATABASE_PTR Database)
{
    gceSTATUS           status;
    gctINT              i;
    gctCONST_POINTER    elements;
    gctSIZE_T           bytes;
    gctUINT8_PTR        vertexBase;

    gcmHEADER_ARG("Context=%p Elements=%p Database=%p",
                  Context, Elements, Database);

    /* Check if the elements are local. */
    if (Elements->elementBytes > 0)
    {
        /* Unbind any current element buffer. */
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        /* Get pointer to elements. */
        gcmONERROR(glshGetRecord(Database,
                                 gcmSIZEOF(*Elements),
                                 (gctPOINTER *) &elements));

        /* Number of bytes in the elements. */
        bytes = Elements->elementBytes;
    }
    else
    {
        /* Bind the requested element array buffer. */
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,
                     Elements->elementArrayBuffer->object.name);

        /* Get offset into elememt array buffer. */
        elements = Elements->indices;

        /* Current size of record. */
        bytes = gcmSIZEOF(*Elements);
    }

    /* Process all elements. */
    for (i = 0; i < 16; i++)
    {
        /* Copy the vertex information. */
        Context->vertexArray[i] = Elements->vertices[i].vertexArray;

        /* Check if the vertex array is local. */
        if (Elements->vertices[i].bytes > 0)
        {
            /* Get the pointer to the vertex array. */
            gcmONERROR(glshGetRecord(Database,
                                     bytes,
                                     (gctPOINTER *) &vertexBase));

            /* Set the pointer to the vertex array. */
            Context->vertexArray[i].pointer = (vertexBase -
                                               Elements->vertices[i].offset);

            /* Number of bytes in the vertex array. */
            bytes = Elements->vertices[i].bytes;
        }
    }

    /* Draw the elements. */
    glDrawElements(Elements->mode, Elements->count, Elements->type, elements);

    /* Return the size of the record or last data. */
    gcmFOOTER_ARG("%u", bytes);
    return bytes;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;

}

static gctSIZE_T glshPlayDrawArrays(GLContext Context,
                                    glsDATABASE_DRAW_ARRAYS * Arrays,
                                    glsDATABASE_PTR Database)
{
    gceSTATUS       status;
    gctINT          i;
    gctSIZE_T       bytes;
    gctUINT8_PTR    vertexBase;

    gcmHEADER_ARG("Context=%p Arrays=%p Database=%p",
                  Context, Arrays, Database);

    /* Set size of record. */
    bytes = gcmSIZEOF(*Arrays);

    /* Process all elements. */
    for (i = 0; i < 16; i++)
    {
        /* Copy the vertex information. */
        Context->vertexArray[i] = Arrays->vertices[i].vertexArray;

        /* Check if the vertex array is local. */
        if (Arrays->vertices[i].bytes > 0)
        {
            /* Get the pointer to the vertex array. */
            gcmONERROR(glshGetRecord(Database,
                                     bytes,
                                     (gctPOINTER *) &vertexBase));

            /* Set the pointer to the vertex array. */
            Context->vertexArray[i].pointer = (vertexBase -
                                               Arrays->vertices[i].offset);

            /* Number of bytes in the vertex array. */
            bytes = Arrays->vertices[i].bytes;
        }
    }

    /* Draw the array. */
    glDrawArrays(Arrays->mode, Arrays->first, Arrays->count);

    /* Return the size of the record or last data. */
    gcmFOOTER_ARG("%u", bytes);
    return bytes;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

static void glshResetTextures(GLContext Context, GLTexture TextureList)
{
    GLTexture   texture;

    gcmHEADER_ARG("Context=%p TextureList=%p", Context, TextureList);

    /* Process all textures in the linked list. */
    for (texture = TextureList;
         texture != gcvNULL;
         texture = texture->nextModified
         )
    {
        /* Test if the minification filter has changed. */
        if (texture->minFilter != texture->savedMinFilter)
        {
            /* Restore the minification filter. */
            glshSetTexParameter(Context,
                                texture,
                                texture->target,
                                GL_TEXTURE_MIN_FILTER,
                                texture->savedMinFilter);
        }

        /* Test if the magnification filter has changed. */
        if (texture->magFilter != texture->savedMagFilter)
        {
            /* Restore the magnification filter. */
            glshSetTexParameter(Context,
                                texture,
                                texture->target,
                                GL_TEXTURE_MAG_FILTER,
                                texture->savedMagFilter);
        }

        /* Test if the anisotropic filter has changed. */
        if (texture->anisoFilter != texture->savedAnisoFilter)
        {
            /* Restore the anisotropic filter. */
            glshSetTexParameter(Context,
                                texture,
                                texture->target,
                                GL_TEXTURE_MAX_ANISOTROPY_EXT,
                                texture->savedAnisoFilter);
        }

        /* Test if the S addressing mode has changed. */
        if (texture->wrapS != texture->savedWrapS)
        {
            /* Restore the S addressing mode. */
            glshSetTexParameter(Context,
                                texture,
                                texture->target,
                                GL_TEXTURE_WRAP_S,
                                texture->savedWrapS);
        }

        /* Test if the T addressing mode has changed. */
        if (texture->wrapT != texture->savedWrapT)
        {
            /* Restore the T addressing mode. */
            glshSetTexParameter(Context,
                                texture,
                                texture->target,
                                GL_TEXTURE_WRAP_T,
                                texture->savedWrapT);
        }

        /* Test if the R addressing mode has changed. */
        if (texture->wrapR != texture->savedWrapR)
        {
            /* Restore the R addressing mode. */
            glshSetTexParameter(Context,
                                texture,
                                texture->target,
                                GL_TEXTURE_WRAP_R_OES,
                                texture->savedWrapR);
        }

        /* Test if the maximum mip level has changed. */
        if (texture->maxLevel != texture->savedMaxLevel)
        {
            /* Restore the maximum mip level. */
            glshSetTexParameter(Context,
                                texture,
                                texture->target,
                                GL_TEXTURE_MAX_LEVEL_APPLE,
                                texture->savedMaxLevel);
        }

        texture->savedMinFilter   = GL_NONE;
        texture->savedMagFilter   = GL_NONE;
        texture->savedAnisoFilter = GL_NONE;
        texture->savedWrapS       = GL_NONE;
        texture->savedWrapT       = GL_NONE;
        texture->savedWrapR       = GL_NONE;
        texture->savedMaxLevel    = GL_NONE;

        /* Reset modification flag. */
        texture->modified = GL_FALSE;
    }

    gcmFOOTER_NO();
}

static void glshResetPrograms(GLContext Context, GLProgram ModifiedList)
{
    GLProgram   program;
    gctINT      i;
    GLUniform   uniform;

    gcmHEADER_ARG("Context=%p ModifiedList=%p", Context, ModifiedList);

    /* Process all modified programs. */
    for (program = ModifiedList;
         program != gcvNULL;
         program = program->nextModified
         )
    {
        /* Restore the bound textures. */
        gcoOS_MemCopy(program->textures,
                      program->savedTextures,
                      gcmSIZEOF(program->textures));

        /* Reset the bound textures. */
        gcoOS_ZeroMemory(program->savedTextures,
                         gcmSIZEOF(program->savedTextures));

        /* Process all uniforms. */
        for (i = 0; i < program->uniformCount; i++)
        {
            /* Get pointer to the uniform. */
            uniform = &program->uniforms[i];

            /* Restore the uniform data. */
            gcoOS_MemCopy(uniform->data, uniform->savedData, uniform->bytes);
        }

        /* Program has been restored. */
        program->modified = GL_FALSE;
    }

    gcmFOOTER_NO();
}


gceSTATUS glshPlayDatabase(GLContext Context, glsDATABASE_PTR Database)
{
    glsDATABASE_DEPENDENCY_PTR  dependency;
    gctINT                      i;
    gceSTATUS                   status;
    GLProgram                   currentProgram;
    GLuint                      currentFramebuffer;
    glsDATABASE_STATES          currentStates;
    gcsVERTEXARRAY              currentVertexArray[16];
    GLBuffer                    currentElementBuffer;
    GLboolean                   scissor;
    GLboolean                   red, green, blue, alpha;
    GLboolean                   depthMask;
    GLuint                      stencilFront, stencilBack;
    gctSIZE_T                   bytes;
    gctPOINTER                  pointer;
    GLTexture                   modifiedTextures = gcvNULL;
    GLProgram                   modifiedPrograms = gcvNULL;

    gcmHEADER_ARG("Context=%p Database=%p", Context, Database);

    gcmTRACE_ZONE(gcvLEVEL_INFO, gldZONE_DATABASE,
                  "Playing back database %p from FBO %d",
                  Database, Database->owner->object.name);

    /* First, play all dependencies. */
    while ((dependency = Database->dependencies) != gcvNULL)
    {
        /* Process all entries in the depenency array. */
        for (i = 0; i < dependency->count; i++)
        {
            /* Play the database. */
            gcmONERROR(glshPlayDatabase(Context, dependency->dependencies[i]));
        }

        /* Get the next dependency array. */
        Database->dependencies = dependency->next;

        /* Free the current dependency array. */
        gcmONERROR(gcoOS_Free(gcvNULL, dependency));
    }

    /* Test if there is any recorded data in the database. */
    if ((Database->clear.mask != 0)
        ||
        (Database->first != gcvNULL)
        )
    {
        /* Save current framebuffer object. */
        currentFramebuffer = ((Context->framebuffer == gcvNULL) ? 0
                              : Context->framebuffer->object.name);

        /* Set new framebuffer object. */
        glBindFramebuffer(GL_FRAMEBUFFER, Database->owner->object.name);

        /* Playing. */
        Context->playing = gcvTRUE;

        /* Check if we have to clear anything. */
        if (Database->clear.mask != 0)
        {
            /* Save current states. */
            scissor      = Context->scissorEnable;
            red          = Context->colorEnableRed;
            green        = Context->colorEnableGreen;
            blue         = Context->colorEnableBlue;
            alpha        = Context->colorEnableAlpha;
            depthMask    = Context->depthMask;
            stencilFront = Context->stencilMaskFront & 0xFF;
            stencilBack  = Context->stencilMaskBack & 0xFF;

            if (scissor)
            {
                /* Disable scissor. */
                glDisable(GL_SCISSOR_TEST);
            }

            if (!red || !green || !blue || !alpha)
            {
                /* Enable all color channels. */
                glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            }

            if (!depthMask)
            {
                /* Enable depth write. */
                glDepthMask(GL_TRUE);
            }

            if ((stencilFront != 0xFF) || (stencilBack != 0xFF))
            {
                /* Enable stencil write. */
                glStencilMask(0xFF);
            }

            /* Play the initial clear. */
            glshPlayClear(Context, &Database->clear);

            /* Test if scissor was enabled. */
            if (scissor)
            {
                /* Reset scissors. */
                glEnable(GL_SCISSOR_TEST);
            }

            /* Test if any color channel was disabled. */
            if (!red || !green || !blue || !alpha)
            {
                /* Reset color channel enable flags. */
                glColorMask(red, green, blue, alpha);
            }

            /* Tets if depth mask was disabled. */
            if (!depthMask)
            {
                /* Reset depth mask. */
                glDepthMask(GL_FALSE);
            }

            /* Test if the stencil write mask was not complete. */
            if (stencilFront != 0xFF)
            {
                /* Reset stencil front write masks. */
                glStencilMaskSeparate(GL_FRONT, stencilFront);
            }

            if (stencilBack != 0xFF)
            {
                /* Reset stencil back write masks. */
                glStencilMaskSeparate(GL_BACK, stencilBack);
            }

            /* Done clearing. */
            Database->clear.mask = 0;
        }

        if (Database->first != gcvNULL)
        {
            /* Save current states. */
            glshGetStates(Context, &currentStates);

            /* Set initial states. */
            glshPlayStates(Context, &Database->initialStates, gcvTRUE);

            /* Save current program object. */
            currentProgram = Context->program;

            /* Save current element buffer. */
            currentElementBuffer = Context->elementArrayBuffer;

            /* Save the current vertex array. */
            gcoOS_MemCopy(currentVertexArray,
                          Context->vertexArray,
                          gcmSIZEOF(Context->vertexArray));

            /* Start from the beginning of the database. */
            Database->playOffset = 0;
            bytes                = 0;

            /* Process all record arrays. */
            for (;;)
            {
                gcmONERROR(glshGetRecord(Database, bytes, &pointer));

                if (status == gcvSTATUS_NO_MORE_DATA)
                {
                    break;
                }

                /* Dispatch based on the record type. */
                switch (*(gleDATABASE_OPCODE *) pointer)
                {
                    case glvDATABASE_OPCODE_SCISSORS:
                        bytes = glshPlayScissors(Context, pointer, gcvFALSE);
                        break;

                    case glvDATABASE_OPCODE_POLYGON:
                        bytes = glshPlayPolygon(Context, pointer, gcvFALSE);
                        break;

                    case glvDATABASE_OPCODE_BLEND:
                        bytes = glshPlayBlend(Context, pointer, gcvFALSE);
                        break;

                    case glvDATABASE_OPCODE_PIXEL:
                        bytes = glshPlayPixel(Context, pointer, gcvFALSE);
                        break;

                    case glvDATABASE_OPCODE_DEPTH:
                        bytes = glshPlayDepth(Context, pointer, gcvFALSE);
                        break;

                    case glvDATABASE_OPCODE_STENCIL:
                        bytes = glshPlayStencil(Context, pointer, gcvFALSE);
                        break;

                    case glvDATABASE_OPCODE_CLEAR:
                        bytes = glshPlayClear(Context, pointer);
                        break;

                    case glvDATABASE_OPCODE_DRAW_ELEMENTS:
                        bytes = glshPlayDrawElements(Context,
                                                     pointer,
                                                     Database);
                        break;

                    case glvDATABASE_OPCODE_DRAW_ARRAYS:
                        bytes = glshPlayDrawArrays(Context, pointer, Database);
                        break;

                    case glvDATABASE_OPCODE_TEXTURE:
                        bytes = glshPlayTexture(Context,
                                                pointer,
                                                &modifiedTextures);
                        break;

                    case glvDATABASE_OPCODE_PROGRAM:
                        bytes = glshPlayProgram(Context,
                                                pointer,
                                                &modifiedPrograms);
                        break;

                    case glvDATABASE_OPCODE_UNIFORM:
                        bytes = glshPlayUniform(Context, pointer, Database);
                        break;

                    case glvDATABASE_OPCODE_FLUSH:
                        glFlush();
                        bytes = gcmSIZEOF(gleDATABASE_OPCODE);
                        break;

                    case glvDATABASE_OPCODE_FINISH:
                        glFinish();
                        bytes = gcmSIZEOF(gleDATABASE_OPCODE);
                        break;

                    default:
                        /* Some database error. */
                        gcmONERROR(gcvSTATUS_INVALID_DATA);
                }
            }

		gco3D_Semaphore(gcvNULL, gcvWHERE_COMMAND, gcvWHERE_PIXEL, gcvHOW_SEMAPHORE_STALL);

            /* All record pools are removed. */
            Database->last = gcvNULL;

            /* Restore element buffer. */
            Context->elementArrayBuffer = currentElementBuffer;

            /* Restore the vertex array. */
            gcoOS_MemCopy(Context->vertexArray,
                          currentVertexArray,
                          gcmSIZEOF(Context->vertexArray));

            /* Reset program object. */
            glshSetProgram(Context, currentProgram);

            /* Reset the texture objects. */
            glshResetTextures(Context, modifiedTextures);

            /* Reset the program objects. */
            glshResetPrograms(Context, modifiedPrograms);

            /* Reset states. */
            glshPlayStates(Context, &currentStates, gcvTRUE);
        }

        /* Reset framebuffer object. */
        glBindFramebuffer(GL_FRAMEBUFFER, currentFramebuffer);

        /* All records have been processed. */
        Database->last = gcvNULL;

        /* Database has been played. */
        Database->counter = 0;

        /* Done playing. */
        Context->playing = gcvFALSE;
    }

    /* Decrease the reference for this database. */
    gcmONERROR(glshRemoveReference(Database));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Reset the texture objects. */
    glshResetTextures(Context, modifiedTextures);

    /* Reset the program objects. */
    glshResetPrograms(Context, modifiedPrograms);

    /* Done playing. */
    Context->playing = gcvFALSE;

    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS glshCheckDependency(GLContext Context)
{
    gceSTATUS   status;
    GLProgram   program;
    gctINT      samplers;
    gctINT      i;
    gctINT      unit;
    GLTexture   texture;

    gcmHEADER_ARG("Context=%p", Context);

    /* Get current program. */
    program = Context->program;
    if (program == gcvNULL)
    {
        /* No program. */
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    /* Get number of samplers. */
    samplers = program->vertexSamplers + program->fragmentSamplers;

    /* Process all samplers. */
    for (i = 0; i < samplers; i++)
    {
        if (!program->modified)
        {
            /* Get the linked texture sampler number. */
            unit = program->sampleMap[i].unit;

            /* Dispatch on the sampler type. */
            switch (program->sampleMap[i].type)
            {
                case gcSHADER_SAMPLER_2D:
                    /* Get the 2D sampler. */
                    texture = Context->texture2D[unit];
                    if (texture == gcvNULL)
                    {
                        /* Revert to default sampler. */
                        texture = &Context->default2D;
                    }
                    break;

                case gcSHADER_SAMPLER_3D:
                    /* Get the 3D sampler */
                    texture = Context->texture3D[unit];
                    if (texture == gcvNULL)
                    {
                        /* Revert to default sampler */
                        texture = &Context->default3D;
                    }
                    break;

                case gcSHADER_SAMPLER_CUBIC:
                    /* Get the cubic sampler. */
                    texture = Context->textureCube[unit];
                    if (texture == gcvNULL)
                    {
                        /* Revert to default sampler. */
                        texture = &Context->defaultCube;
                    }
                    break;

                case gcSHADER_SAMPLER_EXTERNAL_OES:
                    /* Get the external sampler. */
                    texture = Context->textureExternal[unit];
                    if (texture == gcvNULL)
                    {
                        /* Revert to default sampler. */
                        texture = &Context->defaultExternal;
                    }
                    break;

                default:
                    texture = gcvNULL;
                    break;
            }

            /* Check for NULL textures or invalid target. */
            if ((texture == gcvNULL)
                ||
                (texture->texture == gcvNULL)
                ||
                ((texture->target != GL_TEXTURE_2D)
                 &&
                 (texture->target != GL_TEXTURE_CUBE_MAP)
                 &&
                 (texture->target != GL_TEXTURE_3D_OES)
                 &&
                 (texture->target != GL_TEXTURE_EXTERNAL_OES)
                 )
                )
            {
                /* Invalid texture. */
                texture = gcvNULL;
            }

            /* Save bound texture for later. */
            program->textures[i] = texture;
        }
        else
        {
            /* Get bound texture. */
            texture = program->textures[i];
        }

        /* We need to play back the database if we are not playing back yet and
         * a valid database is attached to this texture's FBO owner. */
        if (!Context->playing
            &&
            (texture != gcvNULL)
            &&
            (texture->owner != gcvNULL)
            &&
            (texture->owner->currentDB != gcvNULL)
            )
        {
            /* Play back the FBO's database. */
            gcmONERROR(glshPlayDatabase(Context, texture->owner->currentDB));
        }
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the error. */
    gcmFOOTER();
    return status;
}

#endif /* gldFBO_DATABASE */
