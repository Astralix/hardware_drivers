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




#include "gc_hal_user_precomp.h"

#ifndef VIVANTE_NO_3D

#if gcdNULL_DRIVER < 2

/* Zone used for header/footer. */
#define _GC_OBJ_ZONE    gcvZONE_TEXTURE

/******************************************************************************\
|********************************* Structures *********************************|
\******************************************************************************/

typedef struct _gcsMIPMAP *     gcsMIPMAP_PTR;

/* The internal gcsMIPMAP structure. */
struct _gcsMIPMAP
{
    /* Surface format of texture. */
    gceSURF_FORMAT              format;

    /* Width of the mipmap. */
    gctUINT                     width;

    /* Height of the mipmap.  Only used for 2D and 3D textures. */
    gctUINT                     height;

    /* Depth of the mipmap.  Only used for 3D textures. */
    gctUINT                     depth;

    /* Number of faces.  6 for cubic maps. */
    gctUINT                     faces;

    /* Size per slice. */
    gctSIZE_T                   sliceSize;

    /* Horizontal alignment in pixels of the surface. */
    gceSURF_ALIGNMENT           hAlignment;

    /* Surface allocated for mipmap. */
    gcePOOL                     pool;
    gctBOOL                     fromClient;
    gcoSURF                     surface;
    gctPOINTER                  locked;
    gctUINT32                   address;

    /* Next mipmap level. */
    gcsMIPMAP_PTR               next;
};

/* The gcoTEXTURE object. */
struct _gcoTEXTURE
{
    /* The object. */
    gcsOBJECT                   object;

    /* Surface format of texture. */
    gceSURF_FORMAT              format;

    /* Endian hint of texture. */
    gceENDIAN_HINT              endianHint;

    /* Block size for compressed and YUV textures. */
    gctUINT                     blockWidth;
    gctUINT                     blockHeight;

    /* Pointer to head of mipmap chain. */
    gcsMIPMAP_PTR               maps;
    gcsMIPMAP_PTR               tail;

	/* Texture info. */
    gcsTEXTURE                  Info;

    gctINT                      levels;
    gctINT                      lodMaxValue;

    /* Texture validation. */
    gctBOOL                     complete;
    gctINT                      completeMax;
    gctINT                      completeLevels;
};


/******************************************************************************\
|******************************** Support Code ********************************|
\******************************************************************************/

/*******************************************************************************
**
**  _DestroyMaps
**
**  Destroy all gcsMIPMAP structures attached to an gcoTEXTURE object.
**
**  INPUT:
**
**      gcsMIPMAP_PTR MipMap
**          Pointer to the gcsMIPMAP structure that is the head of the linked
**          list.
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**  OUTPUT:
**
**      Nothing.
**
**  RETURNS:
**
**      Nothing.
*/
static gceSTATUS _DestroyMaps(
    IN gcsMIPMAP_PTR MipMap,
    IN gcoOS Os
    )
{
    gcsMIPMAP_PTR next;
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("MipMap=0x%x Os=0x%x", MipMap, Os);

    /* Loop while we have valid gcsMIPMAP structures. */
    while (MipMap != gcvNULL)
    {
        /* Get pointer to next gcsMIPMAP structure. */
        next = MipMap->next;

        if (MipMap->locked != gcvNULL)
        {
            /* Unlock MipMap. */
            gcmONERROR(
                gcoSURF_Unlock(MipMap->surface, MipMap->locked));
        }

        if (!MipMap->fromClient && (MipMap->surface != gcvNULL))
        {
            /* Destroy the attached gcoSURF object. */
            gcmONERROR(gcoSURF_Destroy(MipMap->surface));
        }

        /* Free the gcsMIPMAP structure. */
        gcmONERROR(gcmOS_SAFE_FREE(Os, MipMap));

        /* Continue with next mip map. */
        MipMap = next;
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/******************************************************************************\
|**************************** gcoTEXTURE Object Code ***************************|
\******************************************************************************/

/*******************************************************************************
**
**  gcoTEXTURE_Construct
**
**  Construct a new gcoTEXTURE object.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**  OUTPUT:
**
**      gcoTEXTURE * Texture
**          Pointer to a variable receiving the gcoTEXTURE object pointer.
*/
gceSTATUS
gcoTEXTURE_Construct(
    IN gcoHAL Hal,
    OUT gcoTEXTURE * Texture
    )
{
    gcoTEXTURE texture;
    gceSTATUS status;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER();

    /* Veriffy the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Texture != gcvNULL);

    /* Allocate memory for the gcoTEXTURE object. */
    status = gcoOS_Allocate(gcvNULL,
                            sizeof(struct _gcoTEXTURE),
                            &pointer);

    if (gcmIS_ERROR(status))
    {
        /* Error. */
        gcmFOOTER();
        return status;
    }

    texture = pointer;

    /* Initialize the gcoTEXTURE object. */
    texture->object.type = gcvOBJ_TEXTURE;

    /* Unknown texture format. */
    texture->format = gcvSURF_UNKNOWN;

    /* Default endian hint */
    texture->endianHint  = gcvENDIAN_NO_SWAP;

    /* No mip map allocated yet. */
    texture->maps           = gcvNULL;
    texture->tail           = gcvNULL;
    texture->levels         = 0;
    texture->complete       = gcvFALSE;
    texture->completeMax    = -1;
    texture->completeLevels = 0;
    texture->lodMaxValue    = -1;

    /* Return the gcoTEXTURE object pointer. */
    *Texture = texture;

    gcmPROFILE_GC(GLTEXTURE_OBJECT, 1);

    /* Success. */
    gcmFOOTER_ARG("*Texture=0x%x", *Texture);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoTEXTURE_ConstructSized
**
**  Construct a new sized gcoTEXTURE object.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**      gceSURF_FORMAT Format
**          Format of the requested texture.
**
**      gctUINT Width
**          Requested width of the texture.
**
**      gctUINT Height
**          Requested height of the texture.
**
**      gctUINT Depth
**          Requested depth of the texture.  If 'Depth' is not 0, the texture
**          is a volume map.
**
**      gctUINT Faces
**          Requested faces of the texture.  If 'Faces' is not 0, the texture
**          is a cubic map.
**
**      gctUINT MipMapCount
**          Number of requested mip maps for the texture.  'MipMapCount' must be
**          at least 1.
**
**      gcePOOL Pool
**          Pool to allocate tetxure surfaces from.
**
**  OUTPUT:
**
**      gcoTEXTURE * Texture
**          Pointer to a variable receiving the gcoTEXTURE object pointer.
*/
gceSTATUS
gcoTEXTURE_ConstructSized(
    IN gcoHAL Hal,
    IN gceSURF_FORMAT Format,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT Depth,
    IN gctUINT Faces,
    IN gctUINT MipMapCount,
    IN gcePOOL Pool,
    OUT gcoTEXTURE * Texture
    )
{
    gcoTEXTURE texture;
    gceSTATUS status;
    gcsMIPMAP_PTR map = gcvNULL;
    gctSIZE_T sliceSize;
    gctUINT level;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER_ARG("Format=%d Width=%d Height=%d Depth=%d "
                    "Faces=%d MipMapCount=%d Pool=%d",
                    Format, Width, Height, Depth,
                    Faces, MipMapCount, Pool);

    /* Veriffy the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(MipMapCount > 0);
    gcmDEBUG_VERIFY_ARGUMENT(Texture != gcvNULL);

    /* Allocate memory for the gcoTEXTURE object. */
    status = gcoOS_Allocate(gcvNULL,
                            sizeof(struct _gcoTEXTURE),
                            &pointer);

    if (gcmIS_ERROR(status))
    {
        /* Error. */
        gcmFOOTER();
        return status;
    }

    texture = pointer;

    /* Initialize the gcoTEXTURE object. */
    texture->object.type = gcvOBJ_TEXTURE;

    /* Format of texture. */
    texture->format = Format;

    /* Default endian hint */
    texture->endianHint = gcvENDIAN_NO_SWAP;

    /* No mip map allocated yet. */
    texture->maps           = gcvNULL;
    texture->tail           = gcvNULL;
    texture->levels         = 0;
    texture->complete       = gcvFALSE;
    texture->completeMax    = -1;
    texture->completeLevels = 0;

    /* Loop through all mip map. */
    for (level = 0; MipMapCount-- > 0; ++level)
    {
        /* Query texture support */
        status = gcoHARDWARE_QueryTexture(Format,
                                          level,
                                          Width,
                                          Height,
                                          Depth,
                                          Faces,
                                          &texture->blockWidth,
                                          &texture->blockHeight);

        if (status < 0)
        {
            /* Error. */
            break;
        }

        if (status == gcvSTATUS_OK)
        {
            /* Create an gcsMIPMAP structure. */
            status = gcoOS_Allocate(gcvNULL,
                                   sizeof(struct _gcsMIPMAP),
                                   &pointer);

            if (status < 0)
            {
                /* Error. */
                break;
            }

            map = pointer;

            /* Initialize the gcsMIPMAP structure. */
            map->format     = Format;
            map->width      = Width;
            map->height     = Height;
            map->depth      = Depth;
            map->faces      = Faces;
            map->pool       = Pool;
            map->fromClient = gcvFALSE;
            map->locked     = gcvNULL;
            map->next       = gcvNULL;

            /* Construct the surface. */
            status = gcoSURF_Construct(gcvNULL,
                                      gcmALIGN(Width, texture->blockWidth),
                                      gcmALIGN(Height, texture->blockHeight),
                                      gcmMAX(gcmMAX(Depth, Faces), 1),
                                      gcvSURF_TEXTURE,
                                      Format,
                                      Pool,
                                      &map->surface);

            if (status < 0)
            {
                /* Roll back. */
                gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, map));

                /* Error. */
                break;
            }

            /* Recompute slice size. In gcoSURF_Construct, driver just return the size calculated through
               stride * alignedHeight. Therefore, we don't div depth to get sliceSize.*/
            sliceSize      = map->surface->info.size;
            map->sliceSize = sliceSize;

            /* Query texture horizontal alignment. */
            status = gcoHARDWARE_QueryTileAlignment(gcvSURF_TEXTURE,
                                                    map->format,
                                                    &map->hAlignment,
                                                    gcvNULL);
            if (status < 0)
            {
                /* Roll back. */
                gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, map));

                /* Error. */
                break;
            }

            /* Append the gcsMIPMAP structure to the end of the chain. */
            if (texture->maps == gcvNULL)
            {
                texture->maps = map;
                texture->tail = map;
            }
            else
            {
                texture->tail->next = map;
                texture->tail       = map;
            }
        }

        /* Increment number of levels. */
        texture->levels ++;
        texture->completeLevels ++;

        /* Move to next mip map level. */
        Width = gcmMAX(Width / 2, 1);
        Height = gcmMAX(Height / 2, 1);
        if (Depth > 0)
        {
            Depth = gcmMAX(Depth / 2, 1);
        }
    }

    if ( (status == gcvSTATUS_OK) && (texture->maps == gcvNULL) )
    {
        /* No maps created, so texture is not supported. */
        status = gcvSTATUS_NOT_SUPPORTED;
    }

    if (status < 0)
    {
        /* Roll back. */
        gcmVERIFY_OK(_DestroyMaps(texture->maps, gcvNULL));

        texture->object.type = gcvOBJ_UNKNOWN;

        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, texture));

        /* Error. */
        gcmFOOTER();
        return status;
    }

    /* Set texture completeness. */
    gcmASSERT(texture->levels > 0);
    gcmASSERT(texture->levels == texture->completeLevels);
    texture->complete    = gcvTRUE;
    texture->completeMax = texture->completeLevels - 1;

    /* Return the gcoTEXTURE object pointer. */
    *Texture = texture;
    gcmPROFILE_GC(GLTEXTURE_OBJECT, 1);

    /* Success. */
    gcmFOOTER_ARG("*Texture=0x%x", *Texture);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoTEXTURE_Destroy
**
**  Destroy an gcoTEXTURE object.
**
**  INPUT:
**
**      gcoTEXTURE Texture
**          Pointer to an gcoTEXTURE object.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoTEXTURE_Destroy(
    IN gcoTEXTURE Texture
    )
{
    gcmHEADER_ARG("Texture=0x%x", Texture);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Texture, gcvOBJ_TEXTURE);

    gcmPROFILE_GC(GLTEXTURE_OBJECT, -1);

    /* Free all maps. */
    gcmVERIFY_OK(_DestroyMaps(Texture->maps, gcvNULL));

    /* Mark the gcoTEXTURE object as unknown. */
    Texture->object.type = gcvOBJ_UNKNOWN;

    /* Free the gcoTEXTURE object. */
    gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Texture));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoTEXTURE_Upload
**
**  Upload data for a mip map to an gcoTEXTURE object.
**
**  INPUT:
**
**      gcoTEXTURE Texture
**          Pointer to an gcoTEXTURE object.
**
**      gceTEXTURE_FACE Face
**          Face of mip map to upload.  If 'Face' is gcvFACE_NONE, the data does
**          not represend a face of a cubic map.
**
**      gctUINT Width
**          Width of mip map to upload.
**
**      gctUINT Height
**          Height of mip map to upload.
**
**      gctUINT Slice
**          Slice of mip map to upload (only valid for volume maps).
**
**      gctPOINTER Memory
**          Pointer to mip map data to upload.
**
**      gctINT Stride
**          Stride of mip map data to upload.  If 'Stride' is 0, the stride will
**          be computed.
**
**      gceSURF_FORMAT Format
**          Format of the mip map data to upload.  Color conversion might be
**          required if tehe texture format and mip map data format are
**          different (24-bit RGB would be one example).
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoTEXTURE_Upload(
    IN gcoTEXTURE Texture,
    IN gceTEXTURE_FACE Face,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT Slice,
    IN gctCONST_POINTER Memory,
    IN gctINT Stride,
    IN gceSURF_FORMAT Format
    )
{
    gcsMIPMAP_PTR map;
    gctUINT index;
    gctUINT32 address[3] = {0};
    gctPOINTER memory[3] = {gcvNULL};
    gceSTATUS status;
    gctUINT32 offset;

    gcmHEADER_ARG("Texture=0x%x Face=%d Width=%d Height=%d "
                    "Slice=%d Memory=0x%x Stride=%d Format=%d",
                    Texture, Face, Width, Height,
                    Slice, Memory, Stride, Format);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Texture, gcvOBJ_TEXTURE);
    gcmDEBUG_VERIFY_ARGUMENT(Memory != gcvNULL);

    /* Walk to the proper mip map level. */
    for (map = Texture->maps; map != gcvNULL; map = map->next)
    {
        /* See if the map matches the requested size. */
        if ( (Width == map->width) && (Height == map->height) )
        {
            break;
        }
    }

    if (map == gcvNULL)
    {
		status = gcvSTATUS_MIPMAP_TOO_LARGE;
        gcmFOOTER();
        /* Requested map might be too large. */
        return status;
    }

    /* Convert face into index. */
    switch (Face)
    {
    case gcvFACE_NONE:
        /* Use slice for volume maps. */
        index = Slice;
        if ((index != 0) && (index >= map->depth))
        {
            /* The specified slice is outside the allocated texture depth. */
			status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
            return status;
        }
        break;

    case gcvFACE_POSITIVE_X:
    case gcvFACE_NEGATIVE_X:
    case gcvFACE_POSITIVE_Y:
    case gcvFACE_NEGATIVE_Y:
    case gcvFACE_POSITIVE_Z:
    case gcvFACE_NEGATIVE_Z:
        /* Get index from Face. */
        index = Face - gcvFACE_POSITIVE_X;
        if (index > map->faces)
        {
            /* The specified face is outside the allocated texture faces. */
			status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
            return status;
        }
        break;

    default:
        index = 0;
        break;
    }

    /* Lock the surface. */
    status = gcoSURF_Lock(map->surface, address, memory);

    if (gcmIS_ERROR(status))
    {
        /* Error. */
        gcmFOOTER();
        return status;
    }

    /* Compute offset. */
    offset = index * map->sliceSize;

    /* Copy the data. */
    status = gcoHARDWARE_UploadTexture(map->format,
                                       address[0],
                                       memory[0],
                                       offset,
                                       map->surface->info.stride,
                                       0,
                                       0,
                                       Width,
                                       Height,
                                       Memory,
                                       Stride,
                                       Format);

    if (gcmIS_SUCCESS(status))
    {
        /* Flush the CPU cache. */
        status = gcoSURF_NODE_Cache(&map->surface->info.node,
                                  memory[0],
                                  map->surface->info.size,
                                  gcvCACHE_CLEAN);
    }

    /* Dump the buffer. */
    gcmDUMP_BUFFER(gcvNULL,
                   "texture",
                   address[0],
                   memory[0],
                   offset,
                   map->sliceSize);

    if (gcPLS.hal->dump != gcvNULL)
    {
        /* Dump the texture data. */
        gcmVERIFY_OK(gcoDUMP_DumpData(gcPLS.hal->dump,
                                      gcvTAG_TEXTURE,
                                      address[0] + offset,
                                      map->sliceSize,
                                      (char *) memory[0] + offset));
    }

    /* Unlock the surface. */
    gcmVERIFY_OK(gcoSURF_Unlock(map->surface, memory[0]));

    gcmPROFILE_GC(GLTEXTURE_OBJECT_BYTES, map->sliceSize);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoTEXTURE_UploadSub
**
**  Upload data for a mip map to an gcoTEXTURE object.
**
**  INPUT:
**
**      gcoTEXTURE Texture
**          Pointer to an gcoTEXTURE object.
**
**      gctUINT MipMap
**          Mip map level to upload.
**
**      gceTEXTURE_FACE Face
**          Face of mip map to upload.  If 'Face' is gcvFACE_NONE, the data does
**          not represend a face of a cubic map.
**
**      gctUINT XOffset
**          XOffset offset into mip map to upload.
**
**      gctUINT YOffset
**          YOffset offset into mip map to upload.
**
**      gctUINT Width
**          Width of mip map to upload.
**
**      gctUINT Height
**          Height of mip map to upload.
**
**      gctUINT Slice
**          Slice of mip map to upload (only valid for volume maps).
**
**      gctPOINTER Memory
**          Pointer to mip map data to upload.
**
**      gctINT Stride
**          Stride of mip map data to upload.  If 'Stride' is 0, the stride will
**          be computed.
**
**      gceSURF_FORMAT Format
**          Format of the mip map data to upload.  Color conversion might be
**          required if tehe texture format and mip map data format are
**          different (24-bit RGB would be one example).
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoTEXTURE_UploadSub(
    IN gcoTEXTURE Texture,
    IN gctUINT MipMap,
    IN gceTEXTURE_FACE Face,
    IN gctUINT XOffset,
    IN gctUINT YOffset,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT Slice,
    IN gctCONST_POINTER Memory,
    IN gctINT Stride,
    IN gceSURF_FORMAT Format
    )
{
    gcsMIPMAP_PTR map;
    gctUINT index;
    gctUINT32 address[3] = {0};
    gctPOINTER memory[3] = {gcvNULL};
    gceSTATUS status;
    gctUINT32 offset;

    gcmHEADER_ARG("Texture=0x%x MipMap=%d Face=%d XOffset=%d YOffset=%d Width=%d Height=%d "
                    "Slice=%d Memory=0x%x Stride=%d Format=%d",
                    Texture, MipMap, Face, XOffset, YOffset, Width, Height,
                    Slice, Memory, Stride, Format);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Texture, gcvOBJ_TEXTURE);
    gcmDEBUG_VERIFY_ARGUMENT(Memory != gcvNULL);

    /* Walk to the proper mip map level. */
    for (map = Texture->maps; map != gcvNULL; map = map->next)
    {
        /* See if we have reached the proper level. */
        if (MipMap-- == 0)
        {
            break;
        }
    }

    if (map == gcvNULL)
    {
        /* Requested map might be too large. */
		status = gcvSTATUS_MIPMAP_TOO_LARGE;
        gcmFOOTER();
        return status;
    }

    if (map->surface == gcvNULL)
    {
		status = gcvSTATUS_MIPMAP_TOO_LARGE;
        gcmFOOTER();
        /* Requested map has not been created yet. */
        return status;
    }

    if ((XOffset + Width > map->width) || (YOffset + Height > map->height))
    {
        /* Requested upload does not match the mip map. */
		status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    /* Convert face into index. */
    switch (Face)
    {
    case gcvFACE_NONE:
        /* Use slice for volume maps. */
        index = Slice;
        if ((index != 0) && (index >= map->depth))
        {
            /* The specified slice is outside the allocated texture depth. */
			status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
            return status;
        }
        break;

    case gcvFACE_POSITIVE_X:
        /* Positive x face. */
        index = 0;
        if (index >= map->faces)
        {
            /* The specified face is outside the allocated texture faces. */
			status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
            return status;
        }
        break;

    case gcvFACE_NEGATIVE_X:
        /* Negative x face. */
        index = 1;
        if (index >= map->faces)
        {
            /* The specified face is outside the allocated texture faces. */
			status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
            return status;
        }
        break;

    case gcvFACE_POSITIVE_Y:
        /* Positive y face. */
        index = 2;
        if (index >= map->faces)
        {
            /* The specified face is outside the allocated texture faces. */
			status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
            return status;
        }
        break;

    case gcvFACE_NEGATIVE_Y:
        /* Negative y face. */
        index = 3;
        if (index >= map->faces)
        {
            /* The specified face is outside the allocated texture faces. */
			status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
            return status;
        }
        break;

    case gcvFACE_POSITIVE_Z:
        /* Positive z face. */
        index = 4;
        if (index >= map->faces)
        {
            /* The specified face is outside the allocated texture faces. */
			status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
            return status;
        }
        break;

    case gcvFACE_NEGATIVE_Z:
        /* Negative z face. */
        index = 5;
        if (index >= map->faces)
        {
            /* The specified face is outside the allocated texture faces. */
			status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
            return status;
        }
        break;

    default:
        index = 0;
        break;
    }

    /* Lock the surface. */
    status = gcoSURF_Lock(map->surface, address, memory);

    if (gcmIS_ERROR(status))
    {
        /* Error. */
        gcmFOOTER();
        return status;
    }

    offset = index * map->sliceSize;

    /* Copy the data. */
    status = gcoHARDWARE_UploadTexture(Texture->format,
                                      address[0],
                                      memory[0],
                                      offset,
                                      map->surface->info.stride,
                                      XOffset,
                                      YOffset,
                                      Width,
                                      Height,
                                      Memory,
                                      Stride,
                                      Format);

    if (gcmIS_SUCCESS(status))
    {
        /* Flush the CPU cache. */
        status = gcoSURF_NODE_Cache(&map->surface->info.node,
                                  memory[0],
                                  map->surface->info.size,
                                  gcvCACHE_CLEAN);
    }

    /* Dump the buffer. */
    gcmDUMP_BUFFER(gcvNULL,
                   "texture",
                   address[0],
                   memory[0],
                   offset,
                   map->sliceSize);

    /* Unlock the surface. */
    gcmVERIFY_OK(gcoSURF_Unlock(map->surface, memory[0]));

    gcmPROFILE_GC(GLTEXTURE_OBJECT_BYTES, map->sliceSize);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoTEXTURE_GetMipMap
**
**  Get the gcoSURF object pointer for a certain mip map level.
**
**  INPUT:
**
**      gcoTEXTURE Texture
**          Pointer to an gcoTEXTURE object.
**
**      gctUINT MipMap
**          Mip map level to get the gcoSURF object pointer for.
**
**  OUTPUT:
**
**      gcoSURF * Surface
**          Pointer to a variable that receives the gcoSURF object pointer.
*/
gceSTATUS
gcoTEXTURE_GetMipMap(
    IN gcoTEXTURE Texture,
    IN gctUINT MipMap,
    OUT gcoSURF * Surface
    )
{
    gcsMIPMAP_PTR map;
	gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Texture=0x%x MipMap=%d", Texture, MipMap);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Texture, gcvOBJ_TEXTURE);
    gcmDEBUG_VERIFY_ARGUMENT(Surface != gcvNULL);

    /* Get the pointer to the first gcsMIPMAP. */
    map = Texture->maps;

    /* Loop until we reached the desired mip map. */
    while (MipMap-- != 0)
    {
        /* Bail out if we are beyond the mip map chain. */
        if (map == gcvNULL)
        {
            break;
        }

        /* Get next gcsMIPMAP_PTR pointer. */
        map = map->next;
    }

    if ((map == gcvNULL) || (map->surface == gcvNULL))
    {
		status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        /* Could not find requested mip map. */
        return status;
    }

    /* Return pointer to gcoSURF object tied to the gcsMIPMAP. */
    *Surface = map->surface;

    /* Success. */
    gcmFOOTER_ARG("*Surface=0x%x", *Surface);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoTEXTURE_GetMipMapFace
**
**  Get the gcoSURF object pointer an offset into the memory for a certain face
**  inside a certain mip map level.
**
**  INPUT:
**
**      gcoTEXTURE Texture
**          Pointer to an gcoTEXTURE object.
**
**      gctUINT MipMap
**          Mip map level to get the gcoSURF object pointer for.
**
**      gceTEXTURE_FACE
**          Face to compute offset for.
**
**  OUTPUT:
**
**      gcoSURF * Surface
**          Pointer to a variable that receives the gcoSURF object pointer.
**
**      gctUINT32_PTR Offset
**          Pointer to a variable that receives the offset for the face.
*/
gceSTATUS
gcoTEXTURE_GetMipMapFace(
    IN gcoTEXTURE Texture,
    IN gctUINT MipMap,
    IN gceTEXTURE_FACE Face,
    OUT gcoSURF * Surface,
    OUT gctUINT32_PTR Offset
    )
{
    gcsMIPMAP_PTR map;
	gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Texture=0x%x MipMap=%d Face=%d", Texture, MipMap, Face);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Texture, gcvOBJ_TEXTURE);
    gcmDEBUG_VERIFY_ARGUMENT(Surface != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(Offset != gcvNULL);

    /* Get the pointer to the first gcsMIPMAP. */
    map = Texture->maps;

    /* Loop until we reached the desired mip map. */
    while (MipMap-- != 0)
    {
        /* Bail out if we are beyond the mip map chain. */
        if (map == gcvNULL)
        {
            break;
        }

        /* Get next gcsMIPMAP_PTR pointer. */
        map = map->next;
    }

    if ((map == gcvNULL) || (map->surface == gcvNULL))
    {
        /* Could not find requested mip map. */
        gcmFOOTER();
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    if ( (Face != gcvFACE_NONE) && (map->faces != 6) )
    {
        /* Could not find requested mip map. */
		status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    /* Return pointer to gcoSURF object tied to the gcsMIPMAP. */
    *Surface = map->surface;

    /* Return offset to face. */
    *Offset = (Face == gcvFACE_NONE)
                  ? 0
                  : ((Face - 1) * map->sliceSize);

    /* Success. */
    gcmFOOTER_ARG("*Surface=0x%x *Offset=%d", *Surface, *Offset);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoTEXTURE_AddMipMap
**
**  Add a new mip map to the gcoTEXTURE object.
**
**  INPUT:
**
**      gcoTEXTURE Texture
**          Pointer to an gcoTEXTURE object.
**
**      gctINT Level
**          Mip map level to add.
**
**      gceSURF_FORMAT Format
**          Format of mip map level to add.
**
**      gctUINT Width
**          Width of mip map to add.  Should be larger or equal than 1.
**
**      gctUINT Height
**          Height of mip map to add.  Can be 0 for 1D texture maps, or larger
**          or equal than 1 for 2D, 3D, or cubic maps.
**
**      gctUINT Depth
**          Depth of mip map to add.  Can be 0 for 1D, 2D, or cubic maps, or
**          larger or equal than 1 for 3D texture maps.
**
**      gctUINT Faces
**          Number of faces of the mip map to add.  Can be 0 for 1D, 2D, or 3D
**          texture maps, or 6 for cubic maps.
**
**      gcePOOL Pool
**          Pool to allocate memory from for the mip map.
**
**  OUTPUT:
**
**      gcoSURF * Surface
**          Pointer to a variable that receives the gcoSURF object pointer.
**          'Surface' can be gcvNULL, in which case no surface will be returned.
*/
gceSTATUS
gcoTEXTURE_AddMipMap(
    IN gcoTEXTURE Texture,
    IN gctINT Level,
    IN gceSURF_FORMAT Format,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT Depth,
    IN gctUINT Faces,
    IN gcePOOL Pool,
    OUT gcoSURF * Surface
    )
{
    gctINT level;
    gcsMIPMAP_PTR map;
    gcsMIPMAP_PTR next;
    gceSTATUS status;
    gctSIZE_T sliceSize;

    gcmHEADER_ARG("Texture=0x%x Level=%d Format=%d Width=%d Height=%d "
                    "Depth=%d Faces=0x%x Pool=%d",
                    Texture, Level, Format, Width, Height,
                    Depth, Faces, Pool);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Texture, gcvOBJ_TEXTURE);

    if (Level < 0)
    {
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    /* Set initial level. */
    map  = gcvNULL;
    next = Texture->maps;

    /* Find the correct mip level. */
    for (level = 0; level <= Level; level += 1)
    {
        /* Create gcsMIPMAP structure if doesn't yet exist. */
        if (next == gcvNULL)
        {
            gctPOINTER pointer = gcvNULL;

            gcmONERROR(
                gcoOS_Allocate(gcvNULL,
                               sizeof(struct _gcsMIPMAP),
                               &pointer));

            next = pointer;

            /* Initialize the gcsMIPMAP structure. */
            next->format     = gcvSURF_UNKNOWN;
            next->width      = ~0U;
            next->height     = ~0U;
            next->depth      = ~0U;
            next->faces      = ~0U;
            next->sliceSize  = ~0U;
            next->pool       = gcvPOOL_UNKNOWN;
            next->fromClient = gcvFALSE;
            next->surface    = gcvNULL;
            next->locked     = gcvNULL;
            next->next       = gcvNULL;
            next->hAlignment = gcvSURF_FOUR;

            /* Append the gcsMIPMAP structure to the end of the chain. */
            if (Texture->maps == gcvNULL)
            {
                /* Save texture format. */
                Texture->format = Format;

                /* Update map pointers. */
                Texture->maps = next;
                Texture->tail = next;
            }
            else
            {
                /* Update map pointers. */
                Texture->tail->next = next;
                Texture->tail       = next;
            }

            /* Increment the number of levels. */
            Texture->levels ++;
        }

        /* Move to the next level. */
        map  = next;
        next = next->next;
    }

    if (map == gcvNULL)
    {
        status = gcvSTATUS_HEAP_CORRUPTED;
        gcmFOOTER();
        return status;
    }

    /* Query texture support */
    gcmONERROR(
        gcoHARDWARE_QueryTexture(Format,
                                 Level,
                                 Width, Height, Depth,
                                 Faces,
                                 &Texture->blockWidth,
                                 &Texture->blockHeight));

    if (gcmIS_SUCCESS(status))
    {
        /* Free the surface (and all the texture's mipmaps) if it exists and is different. */
        if
        (
            (map != gcvNULL) &&
            (map->surface != gcvNULL)   &&
            (
                (map->format != Format) ||
                (map->width  != Width)  ||
                (map->height != Height) ||
                (map->depth  != Depth)  ||
                (map->faces  != Faces)  ||
                (map->pool   != Pool)
            )
        )
        {
            /* Unlock the surface. */
            if (map->locked != gcvNULL)
            {
                status = gcoSURF_Unlock(map->surface, map->locked);

                if (gcmIS_ERROR(status))
                {
                    /* Error. */
                    gcmFOOTER();
                    return status;
                }

                map->locked = gcvNULL;
            }

            /* Destroy the surface. */
            if (!map->fromClient)
            {
                status = gcoSURF_Destroy(map->surface);

                if (gcmIS_ERROR(status))
                {
                    /* Error. */
                    gcmFOOTER();
                    return status;
                }
            }

            /* Reset the surface pointer. */
            map->surface = gcvNULL;

            /* A surface has been removed. */
            gcmASSERT(Texture->completeLevels > 0);
            Texture->completeLevels --;
        }

        if (map != gcvNULL && map->surface == gcvNULL)
        {
            /* Construct the surface. */
            gcmONERROR(
                gcoSURF_Construct(gcvNULL,
                                  gcmALIGN(Width, Texture->blockWidth),
                                  gcmALIGN(Height, Texture->blockHeight),
                                  gcmMAX(gcmMAX(Depth, Faces), 1),
                                  gcvSURF_TEXTURE,
                                  Format,
                                  Pool,
                                  &map->surface));

            /* Recompute slice size. In gcoSURF_Construct, driver just return the size calculated through
               stride * alignedHeight. Therefore, we don't div depth to get sliceSize.*/
            sliceSize = map->surface->info.size;

            /* Initialize the gcsMIPMAP structure. */
            map->format    = Format;
            map->width     = Width;
            map->height    = Height;
            map->depth     = Depth;
            map->faces     = Faces;
            map->sliceSize = sliceSize;
            map->pool      = Pool;
            gcmONERROR(
                gcoHARDWARE_QueryTileAlignment(gcvSURF_TEXTURE,
                                               map->format,
                                               &map->hAlignment,
                                               gcvNULL));

            /* Update the valid surface count. */
            gcmASSERT(Texture->completeLevels < Texture->levels);
            Texture->completeLevels ++;

            /* Invalidate completeness status. */
            Texture->completeMax = -1;
        }

        /* Return the gcoSURF pointer. */
        if (Surface != gcvNULL)
        {
            *Surface = map->surface;
        }
    }

OnError:
    /* Return the status. */
    gcmFOOTER_ARG("*Surface=0x%08x status=%d", gcmOPT_VALUE(Surface), status);
    return status;
}

/*******************************************************************************
**
**  gcoTEXTURE_AddMipMapFromClient
**
**  Add a new mip map by wrapping a client surface to the gcoTEXTURE object.
**
**  INPUT:
**
**      gcoTEXTURE Texture
**          Pointer to an gcoTEXTURE object.
**
**      gctINT Level
**          Mip map level to add.
**
**     gcoSURF Surface
**          Client surface to be wrapped
**
**  OUTPUT:
**
**      Nothing.
*/

gceSTATUS
gcoTEXTURE_AddMipMapFromClient(
    IN gcoTEXTURE Texture,
    IN gctINT Level,
    IN gcoSURF Surface
    )
{
    gceSTATUS status;
	gcsMIPMAP_PTR map;

    gcmHEADER_ARG("Texture=0x%x Level=%d Surface=0x%x", Texture, Level, Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Texture, gcvOBJ_TEXTURE);
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    /* Test for VAA surface. */
    if (Surface->info.vaa)
    {
        if (Level != 0)
        {
            /* Only supports level 0. */
            gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
        }

        if (Texture->maps != gcvNULL)
        {
            /* Mipmap already defined. */
            gcmONERROR(gcvSTATUS_INVALID_MIPMAP);
        }

        gcmONERROR(
            gcoTEXTURE_AddMipMap(Texture,
                                 Level,
                                 Surface->info.format,
                                 Surface->info.rect.right / 2,
                                 Surface->info.rect.bottom,
                                 Surface->depth,
                                 0,
                                 gcvPOOL_DEFAULT,
                                 gcvNULL));

        /* Set texture completeness. */
        gcmASSERT(Texture->levels > 0);
        gcmASSERT(Texture->levels == Texture->completeLevels);
        Texture->complete    = gcvTRUE;
        Texture->completeMax = 0;
    }
    else
    {
        gcmONERROR(
            gcoTEXTURE_AddMipMapFromSurface(Texture,
                                            Level,
                                            Surface));
        /* Override fromClient field */

		map = Texture->maps;
        map->fromClient = gcvTRUE;
    }

    /* Copy surface format to texture. */
    Texture->format = Surface->info.format;

    /* Success. */
    status = gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoTEXTURE_AddMipMapFromSurface
**
**  Add a new mip map from an existing surface to the gcoTEXTURE object.
**
**  INPUT:
**
**      gcoTEXTURE Texture
**          Pointer to an gcoTEXTURE object.
**
**      gctINT Level
**          Mip map level to add.
**
**     gcoSURF    Surface
**          Surface which will be added to mipmap
**
**  OUTPUT:
**
**      Nothing.
*/

gceSTATUS
gcoTEXTURE_AddMipMapFromSurface(
    IN gcoTEXTURE Texture,
    IN gctINT     Level,
    IN gcoSURF    Surface
    )
{
    gcsMIPMAP_PTR map;
    gceSTATUS status;
    gctSIZE_T sliceSize;
    gctUINT width, height, face;
    gceSURF_FORMAT format;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER_ARG("Texture=0x%x Level=%d Surface=0x%x", Texture, Level, Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Texture, gcvOBJ_TEXTURE);

    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    if (Level != 0)
    {
		status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    if (Texture->maps != gcvNULL)
    {
        /* Oops, map already defined. */
		status = gcvSTATUS_INVALID_MIPMAP;
        gcmFOOTER();
        return status;
    }

    status = gcoSURF_GetFormat(Surface, gcvNULL, &format);
    if (gcmIS_ERROR(status))
    {
        /* Error. */
        gcmFOOTER();
        return status;
    }

    width  = Surface->info.rect.right;
    height = Surface->info.rect.bottom;
    face  = Surface->depth;

    /* Query texture support */
    status = gcoHARDWARE_QueryTexture(format,
                                      Level,
                                      width, height, 0,
                                      face,
                                      &Texture->blockWidth,
                                      &Texture->blockHeight);

    if (gcmIS_ERROR(status))
    {
        /* Error. */
        gcmFOOTER();
        return status;
    }

    /* Create an gcsMIPMAP structure. */
    status = gcoOS_Allocate(gcvNULL,
                               gcmSIZEOF(struct _gcsMIPMAP),
                               &pointer);

    if (gcmIS_ERROR(status))
    {
        /* Error. */
        gcmFOOTER();
        return status;
    }

    /* Save texture format. */
    Texture->format = format;

    map = pointer;

    /* Recompute slice size. In gcoSURF_Construct, driver just return the size calculated through
       stride * alignedHeight. Therefore, we don't div depth to get sliceSize.*/
    sliceSize = Surface->info.size;

    /* Initialize the gcsMIPMAP structure. */
    map->width      = width;
    map->height     = height;
    map->depth      = 0;
    map->faces      = 0;
    map->sliceSize  = sliceSize;
    map->pool       = Surface->info.node.pool;
    map->fromClient = gcvFALSE;
    map->surface    = Surface;
    map->locked     = gcvNULL;
    map->next       = gcvNULL;
    map->format     = format;
    gcoHARDWARE_QueryTileAlignment(gcvSURF_TEXTURE,
                                   format,
                                   &map->hAlignment,
                                   gcvNULL);

    /* Append the gcsMIPMAP structure to the end of the chain. */
    Texture->maps = map;
    Texture->tail = map;

    /* Increment the number of levels. */
    Texture->levels ++;
    Texture->completeLevels ++;

    /* Set texture completeness. */
    gcmASSERT(Texture->levels > 0);
    gcmASSERT(Texture->levels == Texture->completeLevels);
    Texture->complete    = gcvTRUE;
    Texture->completeMax = 0;

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoTEXTURE_SetEndianHint
**
**  Set the endian hint.
**
**  INPUT:
**
**      gcoTEXTURE Texture
**          Pointer to an gcoTEXTURE object.
**
**      gctUINT Red, Green, Blue, Alpha
**          The border color for the respective color channel.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoTEXTURE_SetEndianHint(
    IN gcoTEXTURE Texture,
    IN gceENDIAN_HINT EndianHint
    )
{
    gcmHEADER_ARG("Texture=0x%x EndianHint=%d", Texture, EndianHint);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Texture, gcvOBJ_TEXTURE);

    /* Save the endian hint. */
    Texture->endianHint = EndianHint;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoTEXTURE_Disable
**
**  Disable a texture sampler.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**      gctINT Sampler
**          The physical sampler to disable.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoTEXTURE_Disable(
    IN gcoHAL Hal,
    IN gctINT Sampler
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Sampler=%d", Sampler);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Sampler >= 0);

    /* Disable the texture. */
    status = gcoHARDWARE_DisableTextureSampler(Sampler);

    /* Return status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoTEXTURE_Flush
**
**  Flush the texture cache.
**
**  INPUT:
**
**      gcoTEXTURE Texture
**          Pointer to an gcoTEXTURE object.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoTEXTURE_Flush(
    IN gcoTEXTURE Texture
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Texture=0x%x", Texture);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Texture, gcvOBJ_TEXTURE);

    /* Flush the texture cache. */
    status = gcoHARDWARE_FlushTexture(gcvNULL);

    /* Return status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoTEXTURE_QueryCaps
**
**  Query the texture capabilities.
**
**  INPUT:
**
**      Nothing.
**
**  OUTPUT:
**
**      gctUINT * MaxWidth
**          Pointer to a variable receiving the maximum width of a texture.
**
**      gctUINT * MaxHeight
**          Pointer to a variable receiving the maximum height of a texture.
**
**      gctUINT * MaxDepth
**          Pointer to a variable receiving the maximum depth of a texture.  If
**          volume textures are not supported, 0 will be returned.
**
**      gctBOOL * Cubic
**          Pointer to a variable receiving a flag whether the hardware supports
**          cubic textures or not.
**
**      gctBOOL * NonPowerOfTwo
**          Pointer to a variable receiving a flag whether the hardware supports
**          non-power-of-two textures or not.
**
**      gctUINT * VertexSamplers
**          Pointer to a variable receiving the number of sampler units in the
**          vertex shader.
**
**      gctUINT * PixelSamplers
**          Pointer to a variable receiving the number of sampler units in the
**          pixel shader.
*/
gceSTATUS
gcoTEXTURE_QueryCaps(
    IN  gcoHAL    Hal,
    OUT gctUINT * MaxWidth,
    OUT gctUINT * MaxHeight,
    OUT gctUINT * MaxDepth,
    OUT gctBOOL * Cubic,
    OUT gctBOOL * NonPowerOfTwo,
    OUT gctUINT * VertexSamplers,
    OUT gctUINT * PixelSamplers
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("MaxWidth=0x%x MaxHeight=0x%x MaxDepth=0x%x Cubic=0x%x "
                    "NonPowerOfTwo=0x%x VertexSamplers=0x%x PixelSamplers=0x%x",
                    MaxWidth, MaxHeight, MaxDepth, Cubic,
                    NonPowerOfTwo, VertexSamplers, PixelSamplers);

    /* Route to gcoHARDWARE function. */
    status = gcoHARDWARE_QueryTextureCaps(MaxWidth,
                                       MaxHeight,
                                       MaxDepth,
                                       Cubic,
                                       NonPowerOfTwo,
                                       VertexSamplers,
                                       PixelSamplers,
                                       gcvNULL);

    /* Return status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoTEXTURE_GetClosestFormat
**
**  Returns the closest supported format to the one specified in the call.
**
**  INPUT:
**
**      gceSURF_FORMAT APIValue
**          API value.
**
**  OUTPUT:
**
**      gctUINT32 * HwValue
**          Closest supported API value.
*/
gceSTATUS
gcoTEXTURE_GetClosestFormat(
    IN gcoHAL Hal,
    IN gceSURF_FORMAT InFormat,
    OUT gceSURF_FORMAT* OutFormat
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("InFormat=%d", InFormat);

    /* Route to gcoHARDWARE function. */
    status = gcoHARDWARE_GetClosestTextureFormat(InFormat,
                                                 OutFormat);

    /* Return status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoTEXTURE_UploadCompressed
**
**  Upload compressed data for a mip map to an gcoTEXTURE object.
**
**  INPUT:
**
**      gcoTEXTURE Texture
**          Pointer to an gcoTEXTURE object.
**
**      gceTEXTURE_FACE Face
**          Face of mip map to upload.  If 'Face' is gcvFACE_NONE, the data does
**          not represend a face of a cubic map.
**
**      gctUINT Width
**          Width of mip map to upload.
**
**      gctUINT Height
**          Height of mip map to upload.
**
**      gctUINT Slice
**          Slice of mip map to upload (only valid for volume maps).
**
**      gctPOINTER Memory
**          Pointer to mip map data to upload.
**
**      gctSIZE_T Size
**          Size of the compressed data to upload.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoTEXTURE_UploadCompressed(
    IN gcoTEXTURE Texture,
    IN gceTEXTURE_FACE Face,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT Slice,
    IN gctCONST_POINTER Memory,
    IN gctSIZE_T Size
    )
{
    gcsMIPMAP_PTR map;
    gctUINT index;
    gctUINT32 address[3] = {0};
    gctPOINTER memory[3] = {gcvNULL};
    gceSTATUS status;
    gctUINT32 offset;

    gcmHEADER_ARG("Texture=0x%x Face=%d Width=%d Height=%d Slice=%d Memory=0x%x Size=%d",
                    Texture, Face, Width, Height, Slice, Memory, Size);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Texture, gcvOBJ_TEXTURE);
    gcmDEBUG_VERIFY_ARGUMENT(Memory != gcvNULL);

    /* Walk to the proper mip map level. */
    for (map = Texture->maps; map != gcvNULL; map = map->next)
    {
        /* See if the map matches the requested size. */
        if ( (Width == map->width) && (Height == map->height) )
        {
            break;
        }
    }

    if (map == gcvNULL)
    {
        /* Requested map might be too large. */
		status = gcvSTATUS_MIPMAP_TOO_LARGE;
        gcmFOOTER();
        return status;
    }

    /* Convert face into index. */
    switch (Face)
    {
    case gcvFACE_NONE:
        /* Use slice for volume maps. */
        index = Slice;
        if ((index != 0) && (index >= map->depth))
        {
            /* The specified slice is outside the allocated texture depth. */
			status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
            return status;
        }
        break;

    case gcvFACE_POSITIVE_X:
        /* Positive x face. */
        index = 0;
        if (index >= map->faces)
        {
            /* The specified face is outside the allocated texture faces. */
			status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
            return status;
        }
        break;

    case gcvFACE_NEGATIVE_X:
        /* Negative x face. */
        index = 1;
        if (index >= map->faces)
        {
            /* The specified face is outside the allocated texture faces. */
			status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
            return status;
        }
        break;

    case gcvFACE_POSITIVE_Y:
        /* Positive y face. */
        index = 2;
        if (index >= map->faces)
        {
            /* The specified face is outside the allocated texture faces. */
			status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
            return status;
        }
        break;

    case gcvFACE_NEGATIVE_Y:
        /* Negative y face. */
        index = 3;
        if (index >= map->faces)
        {
            /* The specified face is outside the allocated texture faces. */
			status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
            return status;
        }
        break;

    case gcvFACE_POSITIVE_Z:
        /* Positive z face. */
        index = 4;
        if (index >= map->faces)
        {
            /* The specified face is outside the allocated texture faces. */
            status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
            return status;
        }
        break;

    case gcvFACE_NEGATIVE_Z:
        /* Negative z face. */
        index = 5;
        if (index >= map->faces)
        {
            /* The specified face is outside the allocated texture faces. */
			status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
            return status;
        }
        break;

    default:
        index = 0;
        break;
    }

    /* Lock the surface. */
    status = gcoSURF_Lock(map->surface, address, memory);

    if (status < 0)
    {
        /* Error. */
        gcmFOOTER();
        return status;
    }

    /* Compute offset. */
    offset = index * map->sliceSize;

    /* Copy the data. */
    status = gcoHARDWARE_CopyData(&map->surface->info.node,
                                  offset,
                                  Memory,
                                  Size);

    /* Dump the buffer. */
    gcmDUMP_BUFFER(gcvNULL,
                   "texture",
                   address[0], memory[0], offset, map->sliceSize);

    if (gcPLS.hal->dump != gcvNULL)
    {
        /* Dump the texture data. */
        gcmVERIFY_OK(gcoDUMP_DumpData(gcPLS.hal->dump,
                                      gcvTAG_TEXTURE,
                                      address[0] + offset,
                                      Size,
                                      (char *) memory[0] + offset));
    }

    /* Unlock the surface. */
    gcmVERIFY_OK(gcoSURF_Unlock(map->surface, memory[0]));

    gcmPROFILE_GC(GLTEXTURE_OBJECT_BYTES, Size);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoTEXTURE_RenderIntoMipMap(
    IN gcoTEXTURE Texture,
    IN gctINT Level
    )
{
    gcsMIPMAP_PTR map;
    gceSURF_TYPE type;
    gceSTATUS status = gcvSTATUS_OK;
    gcoSURF surface;
    gctBOOL hasTextureTiledRead;

    gcmHEADER_ARG("Texture=0x%x Level=%d", Texture, Level);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Texture, gcvOBJ_TEXTURE);

    /* Get the pointer to the first gcsMIPMAP. */
    map = Texture->maps;

    /* Loop until we reached the desired mip map. */
    while (Level-- != 0)
    {
        /* Bail out if we are beyond the mip map chain. */
        if (map == gcvNULL)
        {
            break;
        }

        /* Get next gcsMIPMAP_PTR pointer. */
        map = map->next;
    }

    if ((map == gcvNULL) || (map->surface == gcvNULL))
    {
        /* Could not find requested mip map. */
		status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    hasTextureTiledRead = gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_TEXTURE_TILED_READ)
                          == gcvSTATUS_TRUE;

    switch (Texture->format)
    {
    case gcvSURF_D16:
        /* fall through */
    case gcvSURF_D24S8:
        /* fall through */
    case gcvSURF_D24X8:
        /* fall through */
    case gcvSURF_D32:
        /* This is a depth texture. */
        if (hasTextureTiledRead)
        {
            type = gcvSURF_DEPTH_NO_TILE_STATUS;
        }
        else
        {
            type = gcvSURF_DEPTH_NO_TILE_STATUS;
        }
        break;

    default:
        /* This is a render target texture. */
        if (hasTextureTiledRead)
        {
            type = gcvSURF_RENDER_TARGET_NO_TILE_STATUS; /*gcvSURF_RENDER_TARGET;*/
        }
        else
        {
            type = gcvSURF_RENDER_TARGET_NO_TILE_STATUS;
        }
        break;
    }

    if (map->surface->info.type == gcvSURF_TEXTURE)
    {
        if (map->locked != gcvNULL)
        {
                status = gcoSURF_Unlock(map->surface, map->locked);

                if (gcmIS_ERROR(status))
                {
                    /* Error. */
                    gcmFOOTER();
                    return status;
                }

                map->locked = gcvNULL;
        }

        /* Construct a new surface. */
        status = gcoSURF_Construct(gcvNULL,
                                   gcmALIGN(map->width, Texture->blockWidth),
                                   gcmALIGN(map->height, Texture->blockHeight),
                                   gcmMAX(gcmMAX(map->depth, map->faces), 1),
                                   type,
                                   Texture->format,
                                   gcvPOOL_DEFAULT,
                                   &surface);


        if (gcmIS_SUCCESS(status))
        {
            /* Copy the data. */
            status = gcoSURF_Resolve(map->surface, surface);
            if (gcmIS_ERROR(status))
            {
                gcmVERIFY_OK(gcoSURF_Destroy(surface));
                gcmFOOTER();
                return status;
            }

            /* Destroy the old surface. */
            gcmVERIFY_OK(gcoSURF_Destroy(map->surface));

            /* Set the new surface. */
            map->surface = surface;

            /* Mark the mipmap as not resolvable. */
            gcmVERIFY_OK(
                gcoSURF_SetResolvability(map->surface, gcvFALSE));
        }
    }

    /* Success. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoTEXTURE_IsRenderable(
    IN gcoTEXTURE Texture,
    IN gctUINT Level
    )
{
    gcsMIPMAP_PTR map;
    gcoSURF surface;
    gceSTATUS status;

    gcmHEADER_ARG("Texture=0x%x Level=%d", Texture, Level);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Texture, gcvOBJ_TEXTURE);

    /* Get the pointer to the first gcsMIPMAP. */
    map = Texture->maps;

    /* Loop until we reached the desired mip map. */
    while (Level-- != 0)
    {
        /* Bail out if we are beyond the mip map chain. */
        if (map == gcvNULL)
        {
            break;
        }

        /* Get next gcsMIPMAP_PTR pointer. */
        map = map->next;
    }

    if ((map == gcvNULL) || (map->surface == gcvNULL))
    {
        /* Could not find requested mip map. */
		status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    /* Get mipmap surface. */
    surface = map->surface;

    /* Check whether the surface is renderable. */
    status = gcoHARDWARE_IsSurfaceRenderable(&surface->info);

    /* Return status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoTEXTURE_IsComplete(
    IN gcoTEXTURE Texture,
    IN gctINT MaxLevel
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Texture=0x%x", Texture);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Texture, gcvOBJ_TEXTURE);

    /* Determine the completeness. */
    if (MaxLevel > Texture->completeMax)
    {
        gctINT level;
        gceSURF_FORMAT format = gcvSURF_UNKNOWN;
        gctUINT width  = ~0U;
        gctUINT height = ~0U;
        gctUINT depth  = ~0U;
        gctUINT faces  = ~0U;
        gcsMIPMAP_PTR prev;
        gcsMIPMAP_PTR curr;

        /* Assume complete texture. */
        Texture->complete = gcvTRUE;

        /* Set initial level. */
        prev = gcvNULL;
        curr = Texture->maps;

        for (level = 0; level <= MaxLevel; level += 1)
        {
            /* Does the level exist? */
            if (curr == gcvNULL)
            {
                /* Incomplete. */
                Texture->complete = gcvFALSE;
                break;
            }

            /* Is there a surface attached? */
            if (curr->surface == gcvNULL)
            {
                /* Incomplete. */
                Texture->complete = gcvFALSE;
                break;
            }

            /* Set texture parameters if we are at level 0. */
            if (prev == gcvNULL)
            {
                format = curr->format;
                width  = curr->width;
                height = curr->height;
                depth  = curr->depth;
                faces  = curr->faces;
            }
            else
            {
                /* Does the level match the expected? */
                if ((format != curr->format) ||
                    (width  != curr->width)  ||
                    (height != curr->height) ||
                    (depth  != curr->depth)  ||
                    (faces  != curr->faces))
                {
                    /* Incomplete/invalid. */
                    Texture->complete = gcvFALSE;
                    break;
                }
            }

            /* Compute the size of the next level. */
            width  = gcmMAX(width  / 2, 1);
            height = gcmMAX(height / 2, 1);

            /* Compute the depth of the next level. */
            depth = (depth > 0) ? gcmMAX(depth / 2, 1) : 0;

            /* Move to the next level. */
            prev = curr;
            curr = curr->next;
        }

        if (Texture->complete)
        {
            /* Set texture format. */
            Texture->format = format;

            /* Validate completeness. */
            Texture->completeMax = MaxLevel;
        }
        else
        {
            /* Reset completeMax to initial value. */
            Texture->completeMax = -1;
        }
    }

    /* Determine the status. */
    status = Texture->complete
        ? gcvSTATUS_OK
        : gcvSTATUS_INVALID_MIPMAP;

    if (status == gcvSTATUS_OK)
    {
        switch (Texture->format)
        {
        case gcvSURF_A32F:
            /* Fall through */
        case gcvSURF_L32F:
            /* Fall through */
        case gcvSURF_A32L32F:
            if ((Texture->Info.magFilter != gcvTEXTURE_POINT)
             || (Texture->Info.minFilter != gcvTEXTURE_POINT)
             || (
                    (Texture->Info.mipFilter != gcvTEXTURE_NONE)
                 && (Texture->Info.mipFilter != gcvTEXTURE_POINT)
                )
               )
            {
                Texture->complete = gcvFALSE;

                status = gcvSTATUS_NOT_SUPPORTED;
            }
            break;

        default:
            break;
        }
    }

    /* Return status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoTEXTURE_BindTexture(
    IN gcoTEXTURE Texture,
    IN gctINT Target,
    IN gctINT Sampler,
    IN gcsTEXTURE_PTR Info
    )
{
    gceSTATUS status;
    gctINT maxLevel;
    gcsMIPMAP_PTR map;
    gctINT lod;
    gcsSAMPLER samplerInfo;
    gctUINT32 address[3] = {0};
    gctPOINTER memory[3] = {gcvNULL};

    gcmHEADER_ARG("Texture=0x%x Sampler=%d", Texture, Sampler);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Texture, gcvOBJ_TEXTURE);

    /* Copy info into texture object. */
    Texture->Info = *Info;

    if (Info->lodMax == -1)
    {
        Texture->Info.lodMax      = (Texture->levels - 1) << 16;
        Texture->lodMaxValue = Texture->levels - 1;
    }
    else
    {
        Texture->lodMaxValue = (gctINT) gcmMIN((Info->lodMax >> 16),Texture->levels-1);
        Texture->Info.lodMax      = Texture->lodMaxValue << 16;
    }

    /* Determine the maximum level number. */
    maxLevel = (Texture->lodMaxValue == -1)
        ? Texture->levels - 1
        : Texture->lodMaxValue;

    /* Make sure we have maps defined. */
    status = gcoTEXTURE_IsComplete(Texture, maxLevel);

    /* Special case for external texture. */
    if ((status == gcvSTATUS_INVALID_MIPMAP)
     && (Texture->levels == 0))
    {
        /* For external textures, allow sampler to be
        still bound when no texture is bound. */
        status = gcvSTATUS_OK;
        gcmASSERT(Texture->maps == gcvNULL);
    }

    if (gcmIS_ERROR(status))
    {
        gcmFOOTER_ARG("status=%d", status);
        return status;
    }

    /* Check if the texture object is resident. */
    if (Sampler >= 0)
    {
        /* Set format and dimension info. */
        samplerInfo.endianHint  = Texture->endianHint;

        if (Texture->maps != gcvNULL)
        {
            samplerInfo.format      = Texture->maps->surface->info.format;
            samplerInfo.width       = Texture->maps->width;
            samplerInfo.height      = Texture->maps->height;
            samplerInfo.depth       = Texture->maps->depth;
            samplerInfo.faces       = Texture->maps->faces;
            samplerInfo.hAlignment  = Texture->maps->hAlignment;
            samplerInfo.roundUV     = 0;
            samplerInfo.hasTileStatus = 0;
            samplerInfo.tileStatusAddr = 0;
            samplerInfo.tileStatusClearValue = 0;
        }
        else
        {
            /* Set dummy values, to sample black (0,0,0,1). */
            samplerInfo.format      = gcvSURF_A8R8G8B8;
            samplerInfo.width       = 0;
            samplerInfo.height      = 0;
            samplerInfo.depth       = 0;
            samplerInfo.faces       = 1;
            samplerInfo.hAlignment  = gcvSURF_FOUR;
            samplerInfo.lodNum      = 0;
            samplerInfo.roundUV     = 0;
            samplerInfo.hasTileStatus = 0;
            samplerInfo.tileStatusAddr = 0;
            samplerInfo.tileStatusClearValue = 0;
        }

        /* Does this texture map have a valid tilestatus node attached to it?. */
        if ((Texture->maps != gcvNULL)
         && (Texture->maps->surface != gcvNULL))
        {
            gcsSURF_INFO_PTR info = &Texture->maps->surface->info;

            /* Update horizontal alignment to the surface's alignment. */
            switch(info->tiling)
            {
            case gcvLINEAR: /* gc860 and gc1000 doesn't support supertile texture */
            case gcvTILED:
                samplerInfo.hAlignment = Texture->maps->hAlignment;
                samplerInfo.lodNum = 1;
                break;

            case gcvSUPERTILED:
                samplerInfo.hAlignment = gcvSURF_SUPER_TILED;
                samplerInfo.lodNum = 1;
                break;

            case gcvMULTI_TILED:
                samplerInfo.hAlignment = gcvSURF_SPLIT_TILED;
                samplerInfo.lodNum = 2;
                break;

            case gcvMULTI_SUPERTILED:
                samplerInfo.hAlignment = gcvSURF_SPLIT_SUPER_TILED;
                samplerInfo.lodNum = 2;
                break;

            default:
                gcmASSERT(gcvFALSE);
                status = gcvSTATUS_NOT_SUPPORTED;
                gcmFOOTER();
                return status;
            }

            /* Set all the LOD levels. */
            samplerInfo.lodAddr[0] = info->node.physical;

            /* Do we need to program the second lod? */
            if (samplerInfo.lodNum == 2)
            {
                gctUINT32 bankOffsetBytes = 0;

                gctUINT32 topBufferSize =
                    gcmALIGN(info->alignedHeight / 2, info->superTiled ? 64 : 4) * info->stride;

#if gcdENABLE_BANK_ALIGNMENT
                status = gcoSURF_GetBankOffsetBytes(gcvNULL, info->type, topBufferSize, &bankOffsetBytes);
                if (gcmIS_ERROR(status))
                {
                    gcmFOOTER();
                    return status;
                }

                /* If surface doesn't have enough padding, then don't offset it. */
                if (info->size < (info->alignedHeight * info->stride) + bankOffsetBytes)
                {
                    bankOffsetBytes = 0;
                }
#endif
                samplerInfo.lodAddr[1] = info->node.physical + topBufferSize + bankOffsetBytes;
            }

            if (Texture->maps->surface->info.tileStatusNode.pool != gcvPOOL_UNKNOWN)
            {
                if (Sampler > 7)
                {
                    /* Error. */
                    gcmFOOTER();
                    return gcvSTATUS_NOT_SUPPORTED;
                }

                info->tileStatusNode.pool = gcvPOOL_SYSTEM;
                samplerInfo.hasTileStatus = !info->tileStatusDisabled;;
                samplerInfo.tileStatusAddr = info->tileStatusNode.physical;
                samplerInfo.tileStatusClearValue = info->clearValue;
            }
            else
            {
                samplerInfo.hasTileStatus = gcvFALSE;

                for (map = Texture->maps, lod = 0; map != gcvNULL; map = map->next, lod++)
                {
                    if (map->locked == gcvNULL)
                    {
                        /* Lock the texture surface. */
                        status = gcoSURF_Lock(map->surface, address, memory);
                        map->address = address[0];
                        map->locked = memory[0];

                        if (gcmIS_ERROR(status))
                        {
                            /* Error. */
                            gcmFOOTER();
                            return status;
                        }
                    }

                    samplerInfo.lodAddr[lod]   = map->address;
                    samplerInfo.lodStride[lod] = map->surface->info.stride;
                }

                /* Number of LODs.
                   If lodNum is 2, then the surface was split, and can have only 2 lods. */
                if (samplerInfo.lodNum != 2)
                {
                    samplerInfo.lodNum = lod;
                }
            }
        }

        /* Copy the texture info. */
        samplerInfo.textureInfo = &Texture->Info;

        /* Bind to hardware. */
        status = gcoHARDWARE_BindTexture(Sampler,
                                         &samplerInfo);

        if (gcmIS_ERROR(status) && (status != gcvSTATUS_NOT_SUPPORTED))
        {
            /* Error. */
            gcmFOOTER();
            return status;
        }
    }
    else
    {
        /* Unlock the texture if locked. */
        for (map = Texture->maps; map != gcvNULL; map = map->next)
        {
            if (map->locked != gcvNULL)
            {
                /* Unlock the texture surface. */
                status = gcoSURF_Unlock(map->surface, memory[0]);

                if (gcmIS_ERROR(status))
                {
                    /* Error. */
                    gcmFOOTER();
                    return status;
                }

                /* Mark the surface as unlocked. */
                map->locked = gcvNULL;
            }
        }
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

#else /* gcdNULL_DRIVER < 2 */


/*******************************************************************************
** NULL DRIVER STUBS
*/

gceSTATUS gcoTEXTURE_Construct(
    IN gcoHAL Hal,
    OUT gcoTEXTURE * Texture
    )
{
    *Texture = gcvNULL;
    return gcvSTATUS_OK;
}

gceSTATUS gcoTEXTURE_ConstructSized(
    IN gcoHAL Hal,
    IN gceSURF_FORMAT Format,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT Depth,
    IN gctUINT Faces,
    IN gctUINT MipMapCount,
    IN gcePOOL Pool,
    OUT gcoTEXTURE * Texture
    )
{
    *Texture = gcvNULL;
    return gcvSTATUS_OK;
}

gceSTATUS gcoTEXTURE_Destroy(
    IN gcoTEXTURE Texture
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoTEXTURE_Upload(
    IN gcoTEXTURE Texture,
    IN gceTEXTURE_FACE Face,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT Slice,
    IN gctCONST_POINTER Memory,
    IN gctINT Stride,
    IN gceSURF_FORMAT Format
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoTEXTURE_UploadSub(
    IN gcoTEXTURE Texture,
    IN gctUINT MipMap,
    IN gceTEXTURE_FACE Face,
    IN gctUINT XOffset,
    IN gctUINT YOffset,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT Slice,
    IN gctCONST_POINTER Memory,
    IN gctINT Stride,
    IN gceSURF_FORMAT Format
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoTEXTURE_GetMipMap(
    IN gcoTEXTURE Texture,
    IN gctUINT MipMap,
    OUT gcoSURF * Surface
    )
{
    *Surface = gcvNULL;
    return gcvSTATUS_OK;
}

gceSTATUS gcoTEXTURE_GetMipMapFace(
    IN gcoTEXTURE Texture,
    IN gctUINT MipMap,
    IN gceTEXTURE_FACE Face,
    OUT gcoSURF * Surface,
    OUT gctUINT32_PTR Offset
    )
{
    *Surface = gcvNULL;
    *Offset = 0;
    return gcvSTATUS_OK;
}

gceSTATUS gcoTEXTURE_AddMipMap(
    IN gcoTEXTURE Texture,
    IN gctINT Level,
    IN gceSURF_FORMAT Format,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT Depth,
    IN gctUINT Faces,
    IN gcePOOL Pool,
    OUT gcoSURF * Surface
    )
{
    if (Surface)
    {
        *Surface = gcvNULL;
    }
    return gcvSTATUS_OK;
}

gceSTATUS gcoTEXTURE_AddMipMapFromClient(
    IN gcoTEXTURE Texture,
    IN gctINT Level,
    IN gcoSURF Surface
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoTEXTURE_AddMipMapFromSurface(
    IN gcoTEXTURE Texture,
    IN gctINT     Level,
    IN gcoSURF    Surface
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoTEXTURE_SetEndianHint(
    IN gcoTEXTURE Texture,
    IN gceENDIAN_HINT EndianHint
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoTEXTURE_Disable(
    IN gcoHAL Hal,
    IN gctINT Sampler
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoTEXTURE_Flush(
    IN gcoTEXTURE Texture
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoTEXTURE_QueryCaps(
    IN  gcoHAL    Hal,
    OUT gctUINT * MaxWidth,
    OUT gctUINT * MaxHeight,
    OUT gctUINT * MaxDepth,
    OUT gctBOOL * Cubic,
    OUT gctBOOL * NonPowerOfTwo,
    OUT gctUINT * VertexSamplers,
    OUT gctUINT * PixelSamplers
    )
{
    *MaxWidth = 8192;
    *MaxHeight = 8192;
    *MaxDepth = 8192;
    *Cubic = gcvTRUE;
    *NonPowerOfTwo = gcvTRUE;
    if (VertexSamplers != gcvNULL)
    {
        *VertexSamplers = 4;
    }
    *PixelSamplers = 8;
    return gcvSTATUS_OK;
}

gceSTATUS gcoTEXTURE_GetClosestFormat(
    IN gcoHAL Hal,
    IN gceSURF_FORMAT InFormat,
    OUT gceSURF_FORMAT* OutFormat
    )
{
    *OutFormat = InFormat;
    return gcvSTATUS_OK;
}

gceSTATUS gcoTEXTURE_UploadCompressed(
    IN gcoTEXTURE Texture,
    IN gceTEXTURE_FACE Face,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT Slice,
    IN gctCONST_POINTER Memory,
    IN gctSIZE_T Size
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoTEXTURE_RenderIntoMipMap(
    IN gcoTEXTURE Texture,
    IN gctINT Level
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoTEXTURE_IsRenderable(
    IN gcoTEXTURE Texture,
    IN gctUINT Level
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoTEXTURE_IsComplete(
    IN gcoTEXTURE Texture,
    IN gctINT MaxLevel
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoTEXTURE_BindTexture(
    IN gcoTEXTURE Texture,
    IN gctINT Target,
    IN gctINT Sampler,
    IN gcsTEXTURE_PTR Info
    )
{
    return gcvSTATUS_OK;
}

#endif /* gcdNULL_DRIVER < 2 */
#endif /*VIVANTE_NO_3D*/
