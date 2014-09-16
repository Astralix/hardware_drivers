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


/**
 * \file gc_dfb_state.c
 */

#include <directfb.h>
#include <direct/messages.h>

#include <core/coredefs.h>
#include <core/coretypes.h>
#include <core/state.h>
#include <core/gfxcard.h>
#include <core/surface.h>
#include <core/palette.h>

#include <gfx/convert.h>

#include "gc_dfb_state.h"
#include "gc_dfb_raster.h"
#include "gc_dfb_utils.h"
#include "gc_dfb.h"

D_DEBUG_DOMAIN( Gal_State, "Gal/State", "State management." );

typedef enum {
    GMI_NONE               = 0x00,
    GMI_SRC_PREMULTIPLY    = 0x01,
    GMI_BLEND_ALPHACHANNEL = 0x02,
    GMI_BLEND_COLORALPHA   = 0x04,
    GMI_COLORIZE           = 0x08,
    GMI_SRC_PREMULTCOLOR   = 0x10,
} GALModulateIndex;

typedef enum {
    GMF_GLOBAL_COLOR_MULTIPLY_COLOR = 0x01,
    GMF_GLOBAL_COLOR_MULTIPLY_ALPHA = 0x02,
    GMF_SRC_PREMULTIPLY             = 0x04,
    GMF_GLOBAL_COLOR_MODE_CgAg      = 0x08,
    GMF_GLOBAL_COLOR_MODE_AgAg      = 0x10,
    GMF_GLOBAL_COLOR_MODE_CgAgAg    = 0x20,
} GALModulateFlag;

#define GAL_MODULATE_MODE_MASK ( GMF_GLOBAL_COLOR_MULTIPLY_COLOR | \
                                 GMF_GLOBAL_COLOR_MULTIPLY_ALPHA | \
                                 GMF_SRC_PREMULTIPLY )

#define GAL_MODULATE_COLOR_MASK ( GMF_GLOBAL_COLOR_MODE_CgAg | \
                                  GMF_GLOBAL_COLOR_MODE_AgAg | \
                                  GMF_GLOBAL_COLOR_MODE_CgAgAg )


static const unsigned int modulate_flags[] = {
    /* None. */
    0x00,

    /* SRC_PREMULTIPLY */
    GMF_SRC_PREMULTIPLY,

    /* BLEND_ALPHACHANNEL */
    0x00,

    /* SRC_PREMULTIPLY | BLEND_ALPHACHANNEL */
    GMF_SRC_PREMULTIPLY,

    /* BLEND_COLORALPHA */
    0x00,

    /* SRC_PREMULTIPLY | BLEND_COLORALPHA */
    GMF_GLOBAL_COLOR_MULTIPLY_ALPHA,

    /* BLEND_ALPHACHANNEL | BLEND_COLORALPHA */
    0x00,

    /* SRC_PREMULTIPLY | BLEND_ALPHACHANNEL | BLEND_COLORALPHA */
    GMF_GLOBAL_COLOR_MULTIPLY_ALPHA | GMF_SRC_PREMULTIPLY,

    /* COLORIZE */
    GMF_GLOBAL_COLOR_MULTIPLY_COLOR,

    /* SRC_PREMULTIPLY | COLORIZE */
    GMF_GLOBAL_COLOR_MULTIPLY_COLOR | GMF_SRC_PREMULTIPLY,

    /* BLEND_ALPHACHANNEL | COLORIZE */
    GMF_GLOBAL_COLOR_MULTIPLY_COLOR,

    /* SRC_PREMULTIPLY | BLEND_ALPHACHANNEL | COLORIZE */
    GMF_GLOBAL_COLOR_MULTIPLY_COLOR | GMF_SRC_PREMULTIPLY,

    /* BLEND_COLORALPHA | COLORIZE */
    GMF_GLOBAL_COLOR_MULTIPLY_COLOR,

    /* SRC_PREMULTIPLY | BLEND_COLORALPHA | COLORIZE */
    GMF_GLOBAL_COLOR_MULTIPLY_COLOR | GMF_GLOBAL_COLOR_MODE_CgAg,

    /* BLEND_ALPHACHANNEL | BLEND_COLORALPHA | COLORIZE */
    GMF_GLOBAL_COLOR_MULTIPLY_COLOR,

    /* SRC_PREMULTIPLY | BLEND_ALPHACHANNEL | BLEND_COLORALPHA | COLORIZE */
    GMF_GLOBAL_COLOR_MULTIPLY_COLOR | GMF_SRC_PREMULTIPLY | GMF_GLOBAL_COLOR_MODE_CgAg,

    /* SRC_PREMULTCOLOR */
    GMF_GLOBAL_COLOR_MULTIPLY_ALPHA,

    /* SRC_PREMULTIPLY | SRC_PREMULTCOLOR */
    GMF_GLOBAL_COLOR_MULTIPLY_ALPHA | GMF_SRC_PREMULTIPLY,

    /* BLEND_ALPHACHANNEL | SRC_PREMULTCOLOR */
    GMF_GLOBAL_COLOR_MULTIPLY_ALPHA,

    /* SRC_PREMULTIPLY | BLEND_ALPHACHANNEL | SRC_PREMULTCOLOR */
    GMF_GLOBAL_COLOR_MULTIPLY_ALPHA | GMF_SRC_PREMULTIPLY,

    /* BLEND_COLORALPHA | SRC_PREMULTCOLOR */
    GMF_GLOBAL_COLOR_MULTIPLY_ALPHA,

    /* SRC_PREMULTIPLY | BLEND_COLORALPHA | SRC_PREMULTCOLOR */
    GMF_GLOBAL_COLOR_MULTIPLY_COLOR | GMF_GLOBAL_COLOR_MODE_AgAg,

    /* BLEND_ALPHACHANNEL | BLEND_COLORALPHA | SRC_PREMULTCOLOR */
    GMF_GLOBAL_COLOR_MULTIPLY_ALPHA,

    /* SRC_PREMULTIPLY | BLEND_ALPHACHANNEL | BLEND_COLORALPHA | SRC_PREMULTCOLOR */
    GMF_GLOBAL_COLOR_MULTIPLY_COLOR | GMF_SRC_PREMULTIPLY | GMF_GLOBAL_COLOR_MODE_AgAg,

    /* COLORIZE | SRC_PREMULTCOLOR */
    GMF_GLOBAL_COLOR_MULTIPLY_COLOR | GMF_GLOBAL_COLOR_MODE_CgAg,

    /* SRC_PREMULTIPLY | COLORIZE | SRC_PREMULTCOLOR */
    GMF_GLOBAL_COLOR_MULTIPLY_COLOR | GMF_SRC_PREMULTIPLY | GMF_GLOBAL_COLOR_MODE_CgAg,

    /* BLEND_ALPHACHANNEL | COLORIZE | SRC_PREMULTCOLOR */
    GMF_GLOBAL_COLOR_MULTIPLY_COLOR | GMF_GLOBAL_COLOR_MODE_CgAg,

    /* SRC_PREMULTIPLY | BLEND_ALPHACHANNEL | COLORIZE | SRC_PREMULTCOLOR */
    GMF_GLOBAL_COLOR_MULTIPLY_COLOR | GMF_SRC_PREMULTIPLY | GMF_GLOBAL_COLOR_MODE_CgAg,

    /* BLEND_COLORALPHA | COLORIZE | SRC_PREMULTCOLOR */
    GMF_GLOBAL_COLOR_MULTIPLY_COLOR | GMF_GLOBAL_COLOR_MODE_CgAg,

    /* SRC_PREMULTIPLY | BLEND_COLORALPHA | COLORIZE | SRC_PREMULTCOLOR */
    GMF_GLOBAL_COLOR_MULTIPLY_COLOR | GMF_GLOBAL_COLOR_MODE_CgAgAg,

    /* BLEND_ALPHACHANNEL | BLEND_COLORALPHA | COLORIZE | SRC_PREMULTCOLOR */
    GMF_GLOBAL_COLOR_MULTIPLY_COLOR | GMF_GLOBAL_COLOR_MODE_CgAg,

    /* SRC_PREMULTIPLY | BLEND_ALPHACHANNEL | BLEND_COLORALPHA | COLORIZE | SRC_PREMULTCOLOR */
    GMF_GLOBAL_COLOR_MULTIPLY_COLOR | GMF_SRC_PREMULTIPLY | GMF_GLOBAL_COLOR_MODE_CgAgAg,
};

static gceSTATUS
_AllocateTempSurf( GalDriverData *vdrv,
                   GalDeviceData *vdev )
{
    gceSTATUS status = gcvSTATUS_OK;

    D_ASSERT( vdrv != NULL);
    D_ASSERT( vdev != NULL);

    do {
	    /* Workaround for dest without alpha channel. */
	    if (vdrv->tmp_dst_surf == NULL ||
	        ((vdrv->tmp_dst_width < vdrv->dst_width) ||
	         (vdrv->tmp_dst_height < vdrv->dst_height))) {

	        /* Destroy the old surface. */
	        if (vdrv->tmp_dst_surf != NULL) {
	            if (vdrv->tmp_dst_logic_addr != NULL) {
	                gcmERR_BREAK(gcoSURF_Unlock( vdrv->tmp_dst_surf,
	                                             vdrv->tmp_dst_logic_addr ));

	                vdrv->tmp_dst_logic_addr = NULL;
	            }

	            gcmERR_BREAK(gcoSURF_Destroy( vdrv->tmp_dst_surf ));

	            vdrv->tmp_dst_surf = NULL;
	        }

	        vdrv->tmp_dst_width  = MAX( vdrv->tmp_dst_width, vdrv->dst_width );
	        vdrv->tmp_dst_height = MAX( vdrv->tmp_dst_height, vdrv->dst_height );

	        /* Construct the tmp dest surface. */
	        gcmERR_BREAK(gcoSURF_Construct( vdrv->hal,
	                                        vdrv->tmp_dst_width,
	                                        vdrv->tmp_dst_height,
	                                        1,
	                                        gcvSURF_BITMAP,
	                                        vdrv->tmp_dst_format,
	                                        gcvPOOL_DEFAULT,
	                                        &vdrv->tmp_dst_surf ));

	        gcmERR_BREAK(gcoSURF_Lock( vdrv->tmp_dst_surf,
	                                   &vdrv->tmp_dst_phys_addr,
	                                   &vdrv->tmp_dst_logic_addr ));

	        gcmERR_BREAK(gcoSURF_GetAlignedSize( vdrv->tmp_dst_surf,
	                                             &vdrv->tmp_dst_aligned_width,
	                                             &vdrv->tmp_dst_aligned_height,
	                                             &vdrv->tmp_dst_pitch ));
	    }
    } while (0);

    if (gcmIS_ERROR( status )) {
        D_DEBUG_AT( Gal_State,
                    "Failed to allocate temp surface.\n" );
    }

    return status;
}

/* Vaidate state. */
static inline void
gal_validate_clip( GalDriverData             *vdrv,
                   CardState              *state,
                   StateModificationFlags  modified )
{
    D_ASSERT( vdrv  != NULL);
    D_ASSERT( state != NULL);

    if (!(modified & SMF_CLIP)) {
        D_DEBUG_AT( Gal_State,
                    "CLIP is not changed.\n" );

        return;
    }

    D_DEBUG_ENTER( Gal_State, "\n" );

    vdrv->clip.left   = state->clip.x1;
    vdrv->clip.top       = state->clip.y1;
    vdrv->clip.right  = state->clip.x2 + 1;
    vdrv->clip.bottom = state->clip.y2 + 1;

    D_DEBUG_AT( Gal_State,
                "clip: (%u, %u, %u, %u).\n",
                GAL_RECTANGLE_VALS(&vdrv->clip) );

    GAL_HSVF_INVALIDATE( HSVF_CLIP );

    D_DEBUG_EXIT( Gal_State, "\n\n" );

    return;
}

static inline void
gal_validate_color( GalDriverData          *vdrv,
                    CardState              *state,
                    StateModificationFlags  modified )
{
    gceSTATUS status = gcvSTATUS_OK;

    D_ASSERT( vdrv  != NULL);
    D_ASSERT( state != NULL);

    if (!(modified & SMF_COLOR)) {
        D_DEBUG_AT( Gal_State, "COLOR is not changed.\n");

        return;
    }

    D_DEBUG_ENTER( Gal_State, "\n" );

    do {
        D_DEBUG_AT( Gal_State,
                    "state->color (r:0x%02X g:0x%02X b:0x%02X a:0x%02X)\n",
                    state->color.r,
                    state->color.g,
                    state->color.b,
                    state->color.a );

        vdrv->color = dfb_pixel_from_color( DSPF_ARGB,
                                            &state->color );

        if (!vdrv->vdev->hw_2d_pe20) {
            /* Global color only affects drawing functions in old chip. */
            if (DFB_DRAWING_FUNCTION( vdrv->accel )) {
                GAL_HSVF_INVALIDATE( HSVF_COLOR );
            }

            if (vdrv->global_alpha_value != state->color.a) {
                vdrv->global_alpha_value = state->color.a;

                if ((DFB_DRAWING_FUNCTION( vdrv->accel ) && (state->drawingflags & DSDRAW_BLEND)) ||
                    (DFB_BLITTING_FUNCTION( vdrv->accel ) && (state->blittingflags & DSBLIT_BLEND_COLORALPHA))) {
                    GAL_HSVF_INVALIDATE( HSVF_BLEND );
                }
            }
        }
        else {
            vdrv->color_is_changed = true;

            if (DFB_BLITTING_FUNCTION( vdrv->accel )) {
                DFBColor color = state->color;

                unsigned int modulate_flag;
                unsigned int index = 0;

                /* Translate the multiply flags. */
                if (state->blittingflags & DSBLIT_COLORIZE) {
                    index |= GMI_COLORIZE;
                }

                if (state->blittingflags & DSBLIT_SRC_PREMULTCOLOR) {
                    index |= GMI_SRC_PREMULTCOLOR;
                }

                if (state->blittingflags & DSBLIT_SRC_PREMULTIPLY) {
                    index |= GMI_SRC_PREMULTIPLY;
                }

                if (state->blittingflags & DSBLIT_BLEND_ALPHACHANNEL) {
                    index |= GMI_BLEND_ALPHACHANNEL;
                }

                if (state->blittingflags & DSBLIT_BLEND_COLORALPHA) {
                    index |= GMI_BLEND_COLORALPHA;
                }

                modulate_flag = modulate_flags[index];

                /* Set the global color value. */
                if (modulate_flag & GMF_GLOBAL_COLOR_MODE_CgAg) {
                    D_DEBUG_AT( Gal_State,
                                "modulate_flag is GMF_GLOBAL_COLOR_MODE_CgAg\n" );

                    color.r = ((unsigned short)state->color.r * (unsigned short)state->color.a) >> 8;
                    color.g = ((unsigned short)state->color.g * (unsigned short)state->color.a) >> 8;
                    color.b = ((unsigned short)state->color.b * (unsigned short)state->color.a) >> 8;
                }
                else if (modulate_flag & GMF_GLOBAL_COLOR_MODE_AgAg) {
                    D_DEBUG_AT( Gal_State,
                                "modulate_flag is GMF_GLOBAL_COLOR_MODE_AgAg\n" );

                    color.r = ((unsigned short)state->color.a * (unsigned short)state->color.a) >> 8;
                    color.g = ((unsigned short)state->color.a * (unsigned short)state->color.a) >> 8;
                    color.b = ((unsigned short)state->color.a * (unsigned short)state->color.a) >> 8;
                }
                else if (modulate_flag & GMF_GLOBAL_COLOR_MODE_CgAgAg) {
                    D_DEBUG_AT( Gal_State,
                                "modulate_flag is GMF_GLOBAL_COLOR_MODE_CgAgAg\n" );

                    color.r = ((((unsigned short)state->color.r * (unsigned short)state->color.a) >> 8) * (unsigned short)state->color.a) >> 8;
                    color.g = ((((unsigned short)state->color.g * (unsigned short)state->color.a) >> 8) * (unsigned short)state->color.a) >> 8;
                    color.b = ((((unsigned short)state->color.b * (unsigned short)state->color.a) >> 8) * (unsigned short)state->color.a) >> 8;
                }

                vdrv->modulate_color = dfb_pixel_from_color( DSPF_ARGB,
                                                             &color );
            }

            GAL_HSVF_INVALIDATE( HSVF_COLOR );
        }
    } while (0);

    if (gcmIS_ERROR( status )) {
        D_DEBUG_AT( Gal_State,
                    "Failed to validate the color.\n" );

        return;
    }

    D_DEBUG_EXIT( Gal_State, "\n" );

    return;
}

static inline void
gal_validate_blend_funcs( GalDriverData          *vdrv,
                          CardState              *state,
                          StateModificationFlags  modified )
{
    D_ASSERT( vdrv  != NULL);
    D_ASSERT( state != NULL);

    if (!(modified & (SMF_SRC_BLEND | SMF_DST_BLEND))) {
        D_DEBUG_AT( Gal_State,
                    "BLEND_FUNCS are not changed.\n" );

        return;
    }

    D_DEBUG_ENTER( Gal_State,
                   "src blend: %d, dst blend: %d.\n",
                    state->src_blend, state->dst_blend );

    vdrv->src_blend = state->src_blend;
    vdrv->dst_blend = state->dst_blend;

    /* Invalidate the relatives of the HW. */
    GAL_HSVF_INVALIDATE( HSVF_BLEND );

    D_DEBUG_EXIT( Gal_State, "\n" );

    return;
}

static inline void
gal_validate_src_colorkey( GalDriverData          *vdrv,
                           CardState              *state,
                           StateModificationFlags  modified )
{
    D_ASSERT( vdrv  != NULL);
    D_ASSERT( state != NULL);

    if (!(modified & (SMF_SRC_COLORKEY | SMF_SOURCE))) {
        D_DEBUG_AT( Gal_State, "SRC COLORKEY is not changed.\n" );

        return;
    }

    /* We'll return if it's a drawing function. */
    if (DFB_DRAWING_FUNCTION( vdrv->accel )) {
        return;
    }

    D_DEBUG_ENTER( Gal_State, "\n" );

    if (!vdrv->vdev->hw_2d_pe20) {
        vdrv->trans_color = state->src_colorkey;

        D_DEBUG_AT( Gal_State, "Source color key is 0x%08X.\n", vdrv->trans_color );

        /* Invalidate the relatives of the HW. */
        GAL_HSVF_INVALIDATE( HSVF_SOURCE );
    }
    else {
        if (state->source) {
            gal_pixel_to_color_wo_expantion( state->source->config.format,
                                             state->src_colorkey,
                                             &vdrv->src_ckey );

            vdrv->src_ckey32 = dfb_pixel_from_color( DSPF_ARGB,
                                                     &vdrv->src_ckey );

            /* Invalidate the relatives of the HW. */
            GAL_HSVF_INVALIDATE( HSVF_SRC_CKEY_VALUE );
        }
    }

    D_DEBUG_EXIT( Gal_State, "\n" );

    return;
}

static inline void
gal_validate_dst_colorkey( GalDriverData          *vdrv,
                           CardState              *state,
                           StateModificationFlags  modified )
{
    D_ASSERT( vdrv  != NULL);
    D_ASSERT( state != NULL);

    if (!(modified & (SMF_DST_COLORKEY | SMF_DESTINATION))) {
        D_DEBUG_AT( Gal_State, "DST COLORKEY is not changed.\n");

        return;
    }

    D_DEBUG_ENTER( Gal_State, "\n" );

    gal_pixel_to_color_wo_expantion( state->destination->config.format,
                                     state->dst_colorkey,
                                     &vdrv->dst_ckey );

    vdrv->dst_ckey32 = dfb_pixel_from_color( DSPF_ARGB,
                                             &vdrv->dst_ckey );

    D_DEBUG_AT( Gal_State,
                "Dest color key is set to (%02X, %02X, %02X, %02X).\n",
                vdrv->dst_ckey.a,
                vdrv->dst_ckey.r,
                vdrv->dst_ckey.g,
                vdrv->dst_ckey.b );

    /* Invalidate the relatives of the HW. */
    GAL_HSVF_INVALIDATE( HSVF_DST_CKEY_VALUE );

    D_DEBUG_EXIT( Gal_State, "\n" );

    return;
}

static inline void
gal_validate_destination( GalDriverData          *vdrv,
                          CardState              *state,
                          StateModificationFlags  modified )
{
    gceSTATUS status = gcvSTATUS_OK;

    D_ASSERT( vdrv  != NULL);
    D_ASSERT( state != NULL);

    if (!(modified & SMF_DESTINATION) &&
        /* For the memory from virtual pool, the physical address is maybe changed after lock. */
        (state->dst.phys == vdrv->last_dst_phys_addr)) {
        D_DEBUG_AT( Gal_State, "DESTINATION is not changed.\n");

        return;
    }

    D_DEBUG_ENTER( Gal_State, "\n" );

    do {
        if (!gal_get_native_format( state->destination->config.format,
                                    &vdrv->dst_native_format )) {
            D_DEBUG_AT( Gal_State,
                        "Failed to get native format for %s.\n",
                        dfb_pixelformat_name(state->destination->config.format) );

            status = gcvSTATUS_NOT_SUPPORTED;

            break;
        }
        else {
            D_DEBUG_AT( Gal_State,
                        "vdrv->dst_format: %s (%u)\n",
                        dfb_pixelformat_name(state->destination->config.format),
                        vdrv->dst_format );
        }

        if (state->dst.phys <= vdrv->vdev->baseAddress) {
            /* It's a virtual address. */
            vdrv->dst_phys_addr  = state->dst.phys + vdrv->vdev->baseAddress;
        } else {
            vdrv->dst_phys_addr  = state->dst.phys - vdrv->vdev->baseAddress;
        }
        vdrv->last_dst_phys_addr = state->dst.phys;
        vdrv->dst_logic_addr     = state->dst.addr;
        vdrv->dst_pitch          = state->dst.pitch;
        vdrv->dst_width          = state->destination->config.size.w;
        vdrv->dst_aligned_width  = gcmALIGN( state->destination->config.size.w, GAL_WIDTH_ALIGNMENT );
        vdrv->dst_height         = state->destination->config.size.h;
        vdrv->dst_aligned_height = gcmALIGN( state->destination->config.size.h, GAL_HEIGHT_ALIGNMENT );
        vdrv->dst_format         = state->destination->config.format;

        D_DEBUG_AT( Gal_State,
                    "vdrv->dst_phys_addr: 0x%08X\n"
                    "vdrv->dst_logic_addr: %p\n"
                    "vdrv->dst_pitch: %u\n"
                    "vdrv->dst_width: %u\n"
                    "vdrv->dst_format: %s\n",
                    vdrv->dst_phys_addr,
                    vdrv->dst_logic_addr,
                    vdrv->dst_pitch,
                    vdrv->dst_width,
                    dfb_pixelformat_name(vdrv->dst_format) );

        /* Invalidate the relatives of the HW. */
        GAL_HSVF_INVALIDATE( HSVF_TARGET );
    } while (0);

    if (gcmIS_ERROR( status )) {
        D_DEBUG_AT( Gal_State, "Failed to validate target.\n" );

        return;
    }

    D_DEBUG_EXIT( Gal_State, "\n" );

    return;
}

static inline void
gal_validate_source( GalDriverData          *vdrv,
                     CardState              *state,
                     StateModificationFlags  modified )
{
    gceSTATUS status = gcvSTATUS_OK;

    D_DEBUG_ENTER( Gal_State, " %s: %d\n", __FILE__, __LINE__ );

    D_ASSERT( vdrv  != NULL);
    D_ASSERT( state != NULL);

    if (!state->source ||
        (!(modified & SMF_SOURCE) &&
        /* For the memory from virtual pool, the physical address is maybe changed after lock. */
         (state->src.phys == vdrv->last_src_phys_addr))) {
        D_DEBUG_AT( Gal_State, "SOURCE is not changed.\n");

        return;
    }

    do {
        if (state->source) {
            if (!gal_get_native_format( state->source->config.format,
                                        &vdrv->src_native_format )) {
                D_DEBUG_AT( Gal_State,
                            "Failed to get the format: %s.\n",
                            dfb_pixelformat_name(state->source->config.format) );

                status = gcvSTATUS_NOT_SUPPORTED;

                break;
            }
            else {
                D_DEBUG_AT( Gal_State,
                            "The source format: %s.\n",
                            dfb_pixelformat_name(state->source->config.format) );
            }

            if (state->src.phys <= vdrv->vdev->baseAddress) {
                /* It's a virtual address. */
                vdrv->src_phys_addr[0] = state->src.phys + vdrv->vdev->baseAddress;
            } else {
                vdrv->src_phys_addr[0] = state->src.phys - vdrv->vdev->baseAddress;
            }
            vdrv->last_src_phys_addr = state->src.phys;
            vdrv->src_logic_addr[0]  = state->src.addr;
            vdrv->src_pitch[0]       = state->src.pitch;
            vdrv->src_width          = state->source->config.size.w;
            vdrv->src_aligned_width  = gcmALIGN( state->source->config.size.w, GAL_WIDTH_ALIGNMENT );
            vdrv->src_height         = state->source->config.size.h;
            vdrv->src_aligned_height = gcmALIGN( state->source->config.size.h, GAL_HEIGHT_ALIGNMENT );
            vdrv->src_format         = state->source->config.format;

            if (gal_is_yuv_format( state->source->config.format )) {
                vdrv->src_is_yuv_format = true;

                switch (state->source->config.format)
                {
                case DSPF_YV12:
                    vdrv->src_pitch[1]      = vdrv->src_pitch[0] / 2;
                    vdrv->src_pitch[2]      = vdrv->src_pitch[0] / 2;

                    vdrv->src_phys_addr[2]  = vdrv->src_phys_addr[0] + vdrv->src_height * vdrv->src_pitch[0];
                    vdrv->src_phys_addr[1]  = vdrv->src_phys_addr[2] + vdrv->src_height/2 * vdrv->src_pitch[2];

                    vdrv->src_logic_addr[2] = (void *)((unsigned int)vdrv->src_logic_addr[0] + vdrv->src_height * vdrv->src_pitch[0]);
                    vdrv->src_logic_addr[1] = (void *)((unsigned int)vdrv->src_logic_addr[2] + vdrv->src_height/2 * vdrv->src_pitch[2]);
                    break;
                case DSPF_I420:
                    vdrv->src_pitch[1]      = vdrv->src_pitch[0] / 2;
                    vdrv->src_pitch[2]      = vdrv->src_pitch[0] / 2;

                    vdrv->src_phys_addr[1]  = vdrv->src_phys_addr[0] + vdrv->src_height * vdrv->src_pitch[0];
                    vdrv->src_phys_addr[2]  = vdrv->src_phys_addr[1] + vdrv->src_height/2 * vdrv->src_pitch[1];

                    vdrv->src_logic_addr[1] = (void *)((unsigned int)vdrv->src_logic_addr[0] + vdrv->src_height * vdrv->src_pitch[0]);
                    vdrv->src_logic_addr[2] = (void *)((unsigned int)vdrv->src_logic_addr[1] + vdrv->src_height/2 * vdrv->src_pitch[1]);
                    break;
                case DSPF_NV12:
                case DSPF_NV21:
                    vdrv->src_pitch[1]      = vdrv->src_pitch[0];

                    vdrv->src_phys_addr[1]  = vdrv->src_phys_addr[0] + vdrv->src_height * vdrv->src_pitch[0];

                    vdrv->src_logic_addr[1] = (void *)((unsigned int)vdrv->src_logic_addr[0] + vdrv->src_height * vdrv->src_pitch[0]);
                    break;
                case DSPF_NV16:
                    vdrv->src_pitch[1]      = vdrv->src_pitch[0];

                    vdrv->src_phys_addr[1]  = vdrv->src_phys_addr[0] + vdrv->src_height * vdrv->src_pitch[0];

                    vdrv->src_logic_addr[1] = (void *)((unsigned int)vdrv->src_logic_addr[0] + vdrv->src_height * vdrv->src_pitch[0]);
                    break;
                case DSPF_YUY2:
                    break;
                case DSPF_UYVY:
                    break;
                default:
                    D_WARN( "Not supported YUV format.\n" );
                    break;
                }
            }
            else {
                vdrv->src_is_yuv_format = false;
            }

            D_DEBUG_AT( Gal_State,
                        "vdrv->src_phys_addr: 0x%08X\n"
                        "vdrv->src_logic_addr: %p\n"
                        "vdrv->src_pitch: %u\n"
                        "vdrv->src_width: %u\n"
                        "vdrv->src_aligned_width: %u\n"
                        "vdrv->src_height: %u\n"
                        "vdrv->src_aligned_height: %u\n"
                        "vdrv->src_format: %s\n",
                        vdrv->src_phys_addr[0],
                        vdrv->src_logic_addr[0],
                        vdrv->src_pitch[0],
                        vdrv->src_width,
                        vdrv->src_aligned_width,
                        vdrv->src_height,
                        vdrv->src_aligned_height,
                        dfb_pixelformat_name(vdrv->src_format) );

            if (DFB_BLITTING_FUNCTION( vdrv->accel )) {
                /* Invalidate the relatives of the HW. */
                GAL_HSVF_INVALIDATE( HSVF_SOURCE );
            }

            vdrv->use_source = true;
        }
        else {
            vdrv->use_source = false;
        }
    } while (0);

    if (gcmIS_ERROR( status )) {
            D_DEBUG_AT( Gal_State, "Failed to validate the source.\n" );

            return;
    }

    D_DEBUG_EXIT(Gal_State, "\n" );

    return;
}

static inline void
gal_validate_source_mask( GalDriverData          *vdrv,
                          CardState              *state,
                          StateModificationFlags  modified )
{
    /* Not supported by HW. */
    return;
}

static inline void
gal_validate_source_mask_vals( GalDriverData          *vdrv,
                               CardState              *state,
                               StateModificationFlags  modified )
{
    /* Not supported by HW. */
    return;
}

static inline void
gal_validate_index_translation( GalDriverData          *vdrv,
                                CardState              *state,
                                StateModificationFlags  modified )
{
    /* Not supported by HW. */

    return;
}

static inline void
gal_validate_colorkey( GalDriverData          *vdrv,
                       CardState              *state,
                       StateModificationFlags  modified )
{
    /* It's not used by DirectFB. */

    return;
}

static inline void
gal_validate_render_options( GalDriverData          *vdrv,
                             CardState              *state,
                             StateModificationFlags  modified )
{
    D_ASSERT( vdrv  != NULL);
    D_ASSERT( state != NULL);

    D_DEBUG_ENTER( Gal_State, "\n" );

    if (state->render_options & (DSRO_SMOOTH_UPSCALE |
                                 DSRO_SMOOTH_DOWNSCALE)) {
        D_DEBUG_AT( Gal_State, "Smooth scaling is enalbed.\n" );
        vdrv->smooth_scale = true;
    }
    else {
        D_DEBUG_AT( Gal_State, "Smooth scaling is disabled.\n" );
        vdrv->smooth_scale = false;
    }

    return;
}

static inline void
gal_validate_matrix( GalDriverData          *vdrv,
                     CardState              *state,
                     StateModificationFlags  modified )
{
    /* Not supported by HW. */

    return;
}

static inline void
gal_validate_palette( GalDriverData *vdrv,
                      CardState     *state )
{
    GalDeviceData *vdev = vdrv->vdev;

    D_ASSERT( vdrv  != NULL);
    D_ASSERT( vdev  != NULL);
    D_ASSERT( state != NULL);

    D_DEBUG_ENTER( Gal_State, "\n" );

    if (DFB_BLITTING_FUNCTION( vdrv->accel ) &&
        (state->source && state->source->palette) &&
        (vdev->palette != state->source->palette) ) {
        CorePalette *palette = NULL;
        int          i;

        palette = state->source->palette;

        vdev->color_table_len = palette->num_entries <= 256 ? palette->num_entries : 256;

        for (i = 0; i < vdev->color_table_len; i++) {
            vdev->color_table[i] = palette->entries[i].a << 24 |
                                   palette->entries[i].r << 16 |
                                   palette->entries[i].g << 8  |
                                   palette->entries[i].b;
        }

        vdev->palette = state->source->palette;

        GAL_HSVF_INVALIDATE( HSVF_PALETTE );
    }

    return;
}

static inline void
gal_validate_df_blend( GalDriverData          *vdrv,
                       CardState              *state,
                       DFBSurfaceDrawingFlags  modified )
{
    D_ASSERT( vdrv  != NULL);
    D_ASSERT( state != NULL);

    if (!(modified & DSDRAW_BLEND)) {
        D_DEBUG_AT( Gal_State, "Draw blend is not changed.\n");

        return;
    }

    D_DEBUG_ENTER( Gal_State, "\n" );

    /* Set blending functions. */
    if (state->drawingflags & DSDRAW_BLEND) {
        D_DEBUG_AT( Gal_State, "Draw blend is enabled.\n" );

        if (!vdrv->vdev->hw_2d_pe20) {
            vdrv->draw_src_global_alpha_mode = gcvSURF_GLOBAL_ALPHA_OFF;
            vdrv->draw_dst_global_alpha_mode = gcvSURF_GLOBAL_ALPHA_OFF;
        }
        else {
            vdrv->draw_src_global_alpha_mode = gcvSURF_GLOBAL_ALPHA_ON;
        }

        vdrv->draw_blend = true;
    }
    else {
        D_DEBUG_AT( Gal_State, "Draw blend is disabled.\n" );

        if (vdrv->vdev->hw_2d_pe20) {
            vdrv->draw_src_global_alpha_mode = gcvSURF_GLOBAL_ALPHA_OFF;
        }

        vdrv->draw_blend = false;
    }

    /* Invalidate the relatives of the HW. */
    GAL_HSVF_INVALIDATE( HSVF_BLEND );

    return;
}

static inline void
gal_validate_df_dst_colorkey( GalDriverData          *vdrv,
                              CardState              *state,
                              DFBSurfaceDrawingFlags  modified )
{
    D_ASSERT( vdrv  != NULL);
    D_ASSERT( state != NULL);

    if (!(modified & DSDRAW_DST_COLORKEY)) {
        D_DEBUG_AT( Gal_State, "Drawing dest colorkey is not changed.\n");

        return;
    }

    D_DEBUG_ENTER( Gal_State, "\n" );

    if (!vdrv->vdev->hw_2d_pe20) {
        D_DEBUG_AT( Gal_State, "Drawing dest colorkey is not supported.\n");

        return;
    }

    if (state->drawingflags & DSDRAW_DST_COLORKEY) {
        /* Enable dest color key. */
        vdrv->dst_trans_mode = gcv2D_KEYED;
        vdrv->draw_bg_rop      = 0xAA;
    }
    else {
        /* Disable source color key. */
        vdrv->dst_trans_mode = gcv2D_OPAQUE;
        vdrv->draw_bg_rop      = 0xCC;
    }

    GAL_HSVF_INVALIDATE( HSVF_CKEY_MODE );

    D_DEBUG_EXIT( Gal_State, "\n" );

    return;
}

static inline void
gal_validate_df_src_premultiply( GalDriverData          *vdrv,
                                 CardState              *state,
                                 DFBSurfaceDrawingFlags  modified )
{
    D_ASSERT( vdrv  != NULL);
    D_ASSERT( state != NULL);

    if (!(modified & DSDRAW_SRC_PREMULTIPLY)) {
        D_DEBUG_AT( Gal_State, "DSDRAW_SRC_PREMULTIPLY is not changed.\n" );

        return;
    }

    D_DEBUG_ENTER( Gal_State, "\n" );

    if (!vdrv->vdev->hw_2d_pe20) {
        D_DEBUG_AT( Gal_State, "DSDRAW_SRC_PREMULTIPLY is not supported.\n" );

        return;
    }

    if (state->drawingflags & DSDRAW_SRC_PREMULTIPLY) {
        vdrv->src_pmp_src_alpha    = gcv2D_COLOR_MULTIPLY_ENABLE;
        vdrv->saved_modulate_flag |= GMF_SRC_PREMULTIPLY;
    }
    else {
        vdrv->src_pmp_src_alpha    = gcv2D_COLOR_MULTIPLY_DISABLE;
        vdrv->saved_modulate_flag &= ~GMF_SRC_PREMULTIPLY;
    }

    /* Disable global color multiply for drawing. */
    vdrv->src_pmp_glb_mode = gcv2D_GLOBAL_COLOR_MULTIPLY_DISABLE;

    vdrv->saved_modulate_flag &= ~GMF_GLOBAL_COLOR_MULTIPLY_ALPHA;
    vdrv->saved_modulate_flag &= ~GMF_GLOBAL_COLOR_MULTIPLY_COLOR;

    GAL_HSVF_INVALIDATE( HSVF_MULTIPLY );

    D_DEBUG_EXIT( Gal_State, "\n" );

    return;
}

static inline void
gal_validate_df_dst_premultiply( GalDriverData          *vdrv,
                                 CardState              *state,
                                 DFBSurfaceDrawingFlags  modified )
{
    D_ASSERT( vdrv  != NULL);
    D_ASSERT( state != NULL);

    if (!(modified & DSDRAW_DST_PREMULTIPLY)) {
        D_DEBUG_AT( Gal_State, "DSDRAW_DST_PREMULTIPLY is not changed.\n" );

        return;
    }

    D_DEBUG_ENTER( Gal_State, "\n" );

    if (!vdrv->vdev->hw_2d_pe20) {
        D_DEBUG_AT( Gal_State, "DSDRAW_DST_PREMULTIPLY is not supported.\n" );

        return;
    }

    if (state->drawingflags & DSDRAW_DST_PREMULTIPLY) {
        vdrv->dst_pmp_dst_alpha = gcv2D_COLOR_MULTIPLY_ENABLE;
    }
    else {
        vdrv->dst_pmp_dst_alpha = gcv2D_COLOR_MULTIPLY_DISABLE;
    }

    GAL_HSVF_INVALIDATE( HSVF_MULTIPLY );

    D_DEBUG_EXIT( Gal_State, "\n" );

    return;
}

static inline void
gal_validate_df_demultiply( GalDriverData          *vdrv,
                            CardState              *state,
                            DFBSurfaceDrawingFlags  modified )
{
    D_ASSERT( vdrv  != NULL);
    D_ASSERT( state != NULL);

    if (!(modified & DSDRAW_DEMULTIPLY)) {
        D_DEBUG_AT( Gal_State, "DSDRAW_DEMULTIPLY is not changed.\n" );

        return;
    }

    D_DEBUG_ENTER( Gal_State, "\n" );

    if (!vdrv->vdev->hw_2d_pe20) {
        D_DEBUG_AT( Gal_State, "DSDRAW_DEMULTIPLY is not supported.\n" );

        return;
    }

    if (state->drawingflags & DSDRAW_DEMULTIPLY) {
        vdrv->dst_dmp_dst_alpha = gcv2D_COLOR_MULTIPLY_ENABLE;
    }
    else {
        vdrv->dst_dmp_dst_alpha = gcv2D_COLOR_MULTIPLY_DISABLE;
    }

    GAL_HSVF_INVALIDATE( HSVF_MULTIPLY );

    D_DEBUG_EXIT( Gal_State, "\n" );

    return;
}

static inline void
gal_validate_df_xor( GalDriverData          *vdrv,
                     CardState              *state,
                     DFBSurfaceDrawingFlags  modified )
{
    D_ASSERT( vdrv  != NULL);
    D_ASSERT( state != NULL);

    if (!(modified & DSDRAW_XOR)) {
        D_DEBUG_AT( Gal_State, "Draw XOR is not changed.\n" );

        return;
    }

    D_DEBUG_ENTER( Gal_State, "\n" );

    if (state->drawingflags & DSDRAW_XOR) {
        /* Enable XOR. */
        vdrv->draw_fg_rop = 0x66;
    } else {
        /* Disable XOR. */
        vdrv->draw_fg_rop = 0xCC;
    }

    D_DEBUG_EXIT( Gal_State,
                  "drawing fg rop: 0x%X.\n",
                  vdrv->draw_fg_rop );

    return;
}

/* Validate blend flags. */
static inline void
gal_validate_bf_blend( GalDriverData           *vdrv,
                       CardState               *state,
                       DFBSurfaceBlittingFlags  modified )
{
    D_ASSERT( vdrv  != NULL );
    D_ASSERT( state != NULL );

    if (!(modified & (DSBLIT_BLEND_ALPHACHANNEL |
                      DSBLIT_BLEND_COLORALPHA))) {
        D_DEBUG_AT( Gal_State, "Blit blend is not changed.\n" );

        return;
    }

    D_DEBUG_ENTER( Gal_State, "\n" );

    if (state->blittingflags & DSBLIT_BLEND_ALPHACHANNEL) {
        if (state->blittingflags & DSBLIT_BLEND_COLORALPHA) {
            D_DEBUG_AT( Gal_State, "blending flag: DSBLIT_BLEND_COLORALPHA | DSBLIT_BLEND_ALPHACHANNEL\n" );

            vdrv->blit_src_global_alpha_mode = gcvSURF_GLOBAL_ALPHA_SCALE;
            vdrv->blit_dst_global_alpha_mode = gcvSURF_GLOBAL_ALPHA_SCALE;
        }
        else {
            D_DEBUG_AT( Gal_State, "blending flag: DSBLIT_BLEND_COLORALPHA\n" );

            vdrv->blit_src_global_alpha_mode = gcvSURF_GLOBAL_ALPHA_OFF;
            vdrv->blit_dst_global_alpha_mode = gcvSURF_GLOBAL_ALPHA_OFF;
        }

        vdrv->blit_blend = true;
    }
    else if (state->blittingflags & DSBLIT_BLEND_COLORALPHA) {
        D_DEBUG_AT( Gal_State, "blending flag: DSBLIT_BLEND_COLORALPHA\n" );

        vdrv->blit_src_global_alpha_mode = gcvSURF_GLOBAL_ALPHA_ON;
        vdrv->blit_dst_global_alpha_mode = gcvSURF_GLOBAL_ALPHA_ON;

        vdrv->blit_blend = true;
    }
    else {
        vdrv->blit_blend = false;

        D_DEBUG_AT( Gal_State, "Disable blending.\n" );
    }

    GAL_HSVF_INVALIDATE( HSVF_BLEND );

    D_DEBUG_EXIT( Gal_State, "\n" );

    return;
}

static inline void
gal_validate_bf_glb_and_src_modulate( GalDriverData           *vdrv,
                                      CardState               *state,
                                      DFBSurfaceBlittingFlags  modified )
{
    unsigned int index = 0, modulate_flag, changed_modulate_flag;

    D_ASSERT( vdrv  != NULL);
    D_ASSERT( state != NULL);

    D_DEBUG_ENTER( Gal_State, "\n" );

    if (!vdrv->vdev->hw_2d_pe20) {
        D_DEBUG_AT( Gal_State, "Global and src multiply is not supported.\n" );

        return;
    }

    /* Translate the multiply flags. */
    if (state->blittingflags & DSBLIT_COLORIZE) {
        index |= GMI_COLORIZE;
    }

    if (state->blittingflags & DSBLIT_SRC_PREMULTCOLOR) {
        index |= GMI_SRC_PREMULTCOLOR;
    }

    if (state->blittingflags & DSBLIT_SRC_PREMULTIPLY) {
        index |= GMI_SRC_PREMULTIPLY;
    }

    if (state->blittingflags & DSBLIT_BLEND_ALPHACHANNEL) {
        index |= GMI_BLEND_ALPHACHANNEL;
    }

    if (state->blittingflags & DSBLIT_BLEND_COLORALPHA) {
        index |= GMI_BLEND_COLORALPHA;
    }

    modulate_flag = modulate_flags[index];

    if ((modulate_flag == vdrv->saved_modulate_flag) &&
        !vdrv->color_is_changed) {

        D_DEBUG_AT( Gal_State, "Global and src multiply is not changed.\n" );

        return;
    }

    changed_modulate_flag = modulate_flag ^ vdrv->saved_modulate_flag;

    /* Modulation mode is changed. */
    if (changed_modulate_flag & GAL_MODULATE_MODE_MASK) {
        /* Set global color multiply flag. */
        if (modulate_flag & GMF_GLOBAL_COLOR_MULTIPLY_COLOR) {
            vdrv->src_pmp_glb_mode = gcv2D_GLOBAL_COLOR_MULTIPLY_COLOR;
        }
        else if (modulate_flag & GMF_GLOBAL_COLOR_MULTIPLY_ALPHA) {
            vdrv->src_pmp_glb_mode = gcv2D_GLOBAL_COLOR_MULTIPLY_ALPHA;
        }
        else {
            vdrv->src_pmp_glb_mode = gcv2D_GLOBAL_COLOR_MULTIPLY_DISABLE;
        }

        /* Set source premultiply flag. */
        if (modulate_flag & GMF_SRC_PREMULTIPLY) {
            vdrv->src_pmp_src_alpha = gcv2D_COLOR_MULTIPLY_ENABLE;
        }
        else {
            vdrv->src_pmp_src_alpha = gcv2D_COLOR_MULTIPLY_DISABLE;
        }

        GAL_HSVF_INVALIDATE( HSVF_MULTIPLY );
    }

    /* Global color is changed. */
    if ((changed_modulate_flag & GAL_MODULATE_COLOR_MASK) ||
        vdrv->color_is_changed) {
        DFBColor color = state->color;

        /* Set the global color value. */
        if (modulate_flag & GMF_GLOBAL_COLOR_MODE_CgAg) {
            D_DEBUG_AT( Gal_State,
                        "modulate_flag is GMF_GLOBAL_COLOR_MODE_CgAg\n" );

            color.r = ((unsigned short)state->color.r * (unsigned short)state->color.a) >> 8;
            color.g = ((unsigned short)state->color.g * (unsigned short)state->color.a) >> 8;
            color.b = ((unsigned short)state->color.b * (unsigned short)state->color.a) >> 8;
        }
        else if (modulate_flag & GMF_GLOBAL_COLOR_MODE_AgAg) {
            D_DEBUG_AT( Gal_State,
                        "modulate_flag is GMF_GLOBAL_COLOR_MODE_AgAg\n" );

            color.r = ((unsigned short)state->color.a * (unsigned short)state->color.a) >> 8;
            color.g = ((unsigned short)state->color.a * (unsigned short)state->color.a) >> 8;
            color.b = ((unsigned short)state->color.a * (unsigned short)state->color.a) >> 8;
        }
        else if (modulate_flag & GMF_GLOBAL_COLOR_MODE_CgAgAg) {
            D_DEBUG_AT( Gal_State,
                        "modulate_flag is GMF_GLOBAL_COLOR_MODE_CgAgAg\n" );

                color.r = ((((unsigned short)state->color.r * (unsigned short)state->color.a) >> 8) * (unsigned short)state->color.a) >> 8;
                color.g = ((((unsigned short)state->color.g * (unsigned short)state->color.a) >> 8) * (unsigned short)state->color.a) >> 8;
                color.b = ((((unsigned short)state->color.b * (unsigned short)state->color.a) >> 8) * (unsigned short)state->color.a) >> 8;
        }

        vdrv->modulate_color = dfb_pixel_from_color( DSPF_ARGB,
                                                     &color );

        vdrv->color_is_changed = false;

        GAL_HSVF_INVALIDATE( HSVF_COLOR );
    }

    vdrv->saved_modulate_flag = modulate_flag;

    D_DEBUG_EXIT( Gal_State, "\n" );

    return;
}

static inline void
gal_validate_bf_src_colorkey( GalDriverData           *vdrv,
                              CardState               *state,
                              DFBSurfaceBlittingFlags  modified )
{
    D_ASSERT( vdrv  != NULL);
    D_ASSERT( state != NULL);

    if (!(modified & DSBLIT_SRC_COLORKEY)) {
        D_DEBUG_AT( Gal_State, "Blit src colorkey is not changed.\n");

        return;
    }

    D_DEBUG_ENTER( Gal_State, "\n" );

    if (!vdrv->vdev->hw_2d_pe20) {
        if (state->blittingflags & DSBLIT_SRC_COLORKEY) {
            /* Enable source color key. */
            vdrv->blit_trans  = gcvSURF_SOURCE_MATCH;
            vdrv->blit_bg_rop = 0xAA;
        }
        else {
            /* Disable source color key. */
            vdrv->blit_trans  = gcvSURF_OPAQUE;
            vdrv->blit_bg_rop = 0xCC;
        }

        GAL_HSVF_INVALIDATE( HSVF_SOURCE );
    } else {
        if (state->blittingflags & DSBLIT_SRC_COLORKEY) {
            /* Enable source color key. */
            vdrv->src_trans_mode    = gcv2D_KEYED;
            vdrv->src_ckey_blit_rop = 0xAA;
        }
        else {
            /* Disable source color key. */
            vdrv->src_trans_mode     = gcv2D_OPAQUE;
            vdrv->src_ckey_blit_rop = 0xCC;
        }
        GAL_HSVF_INVALIDATE( HSVF_CKEY_MODE );
    }

    D_DEBUG_EXIT( Gal_State, "\n" );

    return;
}

static inline void
gal_validate_bf_dst_colorkey( GalDriverData           *vdrv,
                              CardState               *state,
                              DFBSurfaceBlittingFlags  modified )
{
    D_ASSERT( vdrv  != NULL);
    D_ASSERT( state != NULL);

    if (!(modified & DSBLIT_DST_COLORKEY)) {
        D_DEBUG_AT( Gal_State, "Blit dest colorkey is not changed.\n" );

        return;
    }

    D_DEBUG_ENTER( Gal_State, "\n" );

    if (!vdrv->vdev->hw_2d_pe20) {
        D_DEBUG_AT( Gal_State, "Blit dest colorkey is not supported.\n" );

        return;
    }

    if (state->blittingflags & DSBLIT_DST_COLORKEY) {
        /* Enable dest color key. */
        vdrv->dst_trans_mode     = gcv2D_KEYED;
        vdrv->dst_ckey_blit_rop = 0xAA;
    }
    else {
        /* Disable dest color key. */
        vdrv->dst_trans_mode     = gcv2D_OPAQUE;
        vdrv->dst_ckey_blit_rop = 0xCC;
    }

    GAL_HSVF_INVALIDATE( HSVF_CKEY_MODE );

    D_DEBUG_EXIT( Gal_State, "\n" );

    return;
}

static inline void
gal_validate_bf_dst_premultiply( GalDriverData           *vdrv,
                                 CardState               *state,
                                 DFBSurfaceBlittingFlags  modified )
{
    D_ASSERT( vdrv  != NULL);
    D_ASSERT( state != NULL);

    if (!(modified & DSBLIT_DST_PREMULTIPLY)) {
        D_DEBUG_AT( Gal_State, "DSBLIT_DST_PREMULTIPLY is not changed.\n" );

        return;
    }

    D_DEBUG_ENTER( Gal_State, "\n" );

    if (!vdrv->vdev->hw_2d_pe20) {
        D_DEBUG_AT( Gal_State, "DSBLIT_DST_PREMULTIPLY is not supported.\n" );

        return;
    }

    if (state->blittingflags & DSBLIT_DST_PREMULTIPLY) {
        vdrv->dst_pmp_dst_alpha = gcv2D_COLOR_MULTIPLY_ENABLE;
    }
    else {
        vdrv->dst_pmp_dst_alpha = gcv2D_COLOR_MULTIPLY_DISABLE;
    }

    GAL_HSVF_INVALIDATE( HSVF_MULTIPLY );

    D_DEBUG_EXIT( Gal_State, "\n" );

    return;
}

static inline void
gal_validate_bf_demultiply( GalDriverData           *vdrv,
                            CardState               *state,
                            DFBSurfaceBlittingFlags  modified )
{
    D_ASSERT( vdrv  != NULL);
    D_ASSERT( state != NULL);

    if (!(modified & DSBLIT_DEMULTIPLY)) {
        D_DEBUG_AT( Gal_State, "DSBLIT_DEMULTIPLY is not changed.\n" );

        return;
    }

    D_DEBUG_ENTER( Gal_State, "\n" );

    if (!vdrv->vdev->hw_2d_pe20) {
        D_DEBUG_AT( Gal_State, "DSBLIT_DEMULTIPLY is not supported.\n" );

        return;
    }

    if (state->blittingflags & DSBLIT_DEMULTIPLY) {
        vdrv->dst_dmp_dst_alpha = gcv2D_COLOR_MULTIPLY_ENABLE;
    }
    else {
        vdrv->dst_dmp_dst_alpha = gcv2D_COLOR_MULTIPLY_DISABLE;
    }

    GAL_HSVF_INVALIDATE( HSVF_MULTIPLY );

    D_DEBUG_EXIT( Gal_State, "\n" );

    return;
}

static inline void
gal_validate_bf_deinterlace( GalDriverData           *vdrv,
                             CardState               *state,
                             DFBSurfaceBlittingFlags  modified )
{
    return;
}

static inline void
gal_validate_bf_xor( GalDriverData           *vdrv,
                     CardState               *state,
                     DFBSurfaceBlittingFlags  modified )
{
    D_ASSERT( vdrv  != NULL);
    D_ASSERT( state != NULL);

    if (!(modified & DSBLIT_XOR)) {
        D_DEBUG_AT( Gal_State, "Blit XOR is not changed.\n");

        return;
    }

    D_DEBUG_ENTER( Gal_State, "\n" );

    if (state->blittingflags & DSBLIT_XOR) {
        /* Enable XOR. */
        vdrv->blit_fg_rop = 0x66;
    } else {
        /* Disable XOR. */
        vdrv->blit_fg_rop = 0xCC;
    }

    D_DEBUG_EXIT( Gal_State,
                  "blitting rop fg: 0x%X.\n",
                  vdrv->blit_fg_rop );

    return;
}

static inline void
gal_validate_bf_index_translation( GalDriverData           *vdrv,
                                   CardState               *state,
                                   DFBSurfaceBlittingFlags  modified )
{
    return;
}

static inline void
gal_validate_bf_rotation( GalDriverData           *vdrv,
                          CardState               *state,
                          DFBSurfaceBlittingFlags  modified )
{
    D_ASSERT( vdrv  != NULL);
    D_ASSERT( state != NULL);

#if (DIRECTFB_MAJOR_VERSION >= 1) && (DIRECTFB_MINOR_VERSION >= 4)
    if (!(modified & (DSBLIT_ROTATE90  |
                      DSBLIT_ROTATE180 |
                      DSBLIT_ROTATE270))) {
        D_DEBUG_AT( Gal_State, "Blit rotation is not changed.\n");

        return;
    }
#else
    if (!(modified & DSBLIT_ROTATE180)) {
        D_DEBUG_AT( Gal_State, "Blit rotation is not changed.\n");

        return;
    }
#endif

#if (DIRECTFB_MAJOR_VERSION >= 1) && (DIRECTFB_MINOR_VERSION >= 4)
    /* Rotate 90 degree. (CCW) */
    if (state->blittingflags & DSBLIT_ROTATE90) {
        vdrv->blit_hor_mirror = false;
        vdrv->blit_ver_mirror = false;

        vdrv->src_rotation    = gcvSURF_90_DEGREE;
        vdrv->dst_rotation    = gcvSURF_0_DEGREE;
    }
    /* Rotate 180 degree. (CCW) */
    else if (state->blittingflags & DSBLIT_ROTATE180) {
        vdrv->blit_hor_mirror = true;
        vdrv->blit_ver_mirror = true;

        vdrv->src_rotation    = gcvSURF_0_DEGREE;
        vdrv->dst_rotation    = gcvSURF_0_DEGREE;
    }
    /* Rotate 270 degree. (CCW) */
    else if (state->blittingflags & DSBLIT_ROTATE270) {
        vdrv->blit_hor_mirror = false;
        vdrv->blit_ver_mirror = false;

        vdrv->src_rotation    = gcvSURF_0_DEGREE;
        vdrv->dst_rotation    = gcvSURF_90_DEGREE;
    }
    /* No rotation. */
    else {
        vdrv->blit_hor_mirror = false;
        vdrv->blit_ver_mirror = false;

        vdrv->src_rotation    = gcvSURF_0_DEGREE;
        vdrv->dst_rotation    = gcvSURF_0_DEGREE;
    }

    if (modified & DSBLIT_ROTATE270) {
        if (state->blittingflags & DSBLIT_ROTATE270) {
            vdrv->clip.left = state->clip.y1;
            vdrv->clip.top = vdrv->dst_width - state->clip.x2 - 1;
            vdrv->clip.right = state->clip.y2 + 1;
            vdrv->clip.bottom = vdrv->dst_width - state->clip.x1;
        }
        else {
            vdrv->clip.left = state->clip.x1;
            vdrv->clip.top = state->clip.y1;
            vdrv->clip.right = state->clip.x2 + 1;
            vdrv->clip.bottom = state->clip.y2 + 1;
        }

        GAL_HSVF_INVALIDATE( HSVF_CLIP );
    }
#else
    if (state->blittingflags & DSBLIT_ROTATE180) {
        vdrv->blit_hor_mirror = true;
        vdrv->blit_ver_mirror = true;

        vdrv->src_rotation    = gcvSURF_0_DEGREE;
        vdrv->dst_rotation    = gcvSURF_0_DEGREE;
    }
    else {
        vdrv->blit_hor_mirror = false;
        vdrv->blit_ver_mirror = false;

        vdrv->src_rotation    = gcvSURF_0_DEGREE;
        vdrv->dst_rotation    = gcvSURF_0_DEGREE;
    }
#endif

    GAL_HSVF_INVALIDATE( HSVF_MIRROR );
    GAL_HSVF_INVALIDATE( HSVF_ROTATION );

    return;
}

static inline void
gal_validate_bf_colorkey_protect( GalDriverData           *vdrv,
                                  CardState               *state,
                                  DFBSurfaceBlittingFlags  modified )
{
    return;
}

static inline void
gal_validate_bf_src_mask_alpha( GalDriverData           *vdrv,
                                CardState               *state,
                                DFBSurfaceBlittingFlags  modified )
{
    return;
}

static inline void
gal_validate_bf_src_mask_color( GalDriverData           *vdrv,
                                CardState               *state,
                                DFBSurfaceBlittingFlags  modified )
{
    return;
}

/**
 * Validate the changed state.
 *
 * \param [in]    vdrv:  driver specific data
 * \param [in]    vdev:  device specific data
 * \param [in]    state: the card state
 * \param [in]    accel: the function to be accelerated
 *
 * \return true:  the state is changed and validated
 * \return false: the state is not changed
 */
static inline bool
gal_validate_state( GalDriverData       *vdrv,
                    GalDeviceData       *vdev,
                    CardState           *state,
                    DFBAccelerationMask  accel )
{
    StateModificationFlags state_modified;

    D_ASSERT( vdrv  != NULL );
    D_ASSERT( vdev  != NULL );
    D_ASSERT( state != NULL );

    if ((vdrv->accel == DFXL_NONE) ||
        (DFB_DRAWING_FUNCTION( vdrv->accel ) != DFB_DRAWING_FUNCTION( accel ))) {

        if (!vdrv->vdev->hw_2d_pe20) {
            /* Use pool source surface as brush. */
            if (DFB_DRAWING_FUNCTION( accel )) {
                vdrv->use_pool_source = true;
            }
            else {
                vdrv->use_pool_source = false;
            }
        }

        state_modified = SMF_ALL & (~(SMF_DRAWING_FLAGS | SMF_BLITTING_FLAGS));

        if (DFB_DRAWING_FUNCTION( accel )) {
            state_modified |= SMF_DRAWING_FLAGS;
        }
        else {
            state_modified |= SMF_BLITTING_FLAGS;
        }

        vdrv->need_reprogram = true;
    }
    else {
        state_modified = state->mod_hw;

        vdrv->need_reprogram = false;
    }

    if (state_modified == 0) {
        /* State is not changed. */
        return false;
    }

    /* Flush the pending primitives if there is any. */
    _FlushPendingPrimitives( vdrv, vdev );

    vdrv->accel = accel;

    if (state_modified & ~(SMF_DRAWING_FLAGS | SMF_BLITTING_FLAGS)) {
        gal_validate_clip( vdrv, state, state_modified );
        gal_validate_color( vdrv, state, state_modified );
        gal_validate_blend_funcs( vdrv, state, state_modified );
        gal_validate_src_colorkey( vdrv, state, state_modified );
        gal_validate_dst_colorkey( vdrv, state, state_modified );
        gal_validate_destination( vdrv, state, state_modified );
        if (DFB_BLITTING_FUNCTION( accel )) {
            gal_validate_source( vdrv, state, state_modified );
        }
        gal_validate_source_mask( vdrv, state, state_modified );
        gal_validate_source_mask_vals( vdrv, state, state_modified );
        gal_validate_index_translation( vdrv, state, state_modified );
        gal_validate_colorkey( vdrv, state, state_modified );
        gal_validate_render_options( vdrv, state, state_modified );
        gal_validate_matrix( vdrv, state, state_modified );
    }

    /* Validate drawing flags. */
    if (state_modified & SMF_DRAWING_FLAGS) {
        DFBSurfaceDrawingFlags modified;

        if (vdrv->need_reprogram) {
            modified = ~0;
        }
        else {
            modified = vdrv->draw_flags ^ state->drawingflags;
        }

        D_DEBUG_AT( Gal_State, "Modified drawing flags: 0x%08X.\n", modified );

        if (modified != DSDRAW_NOFX) {
            D_DEBUG_AT( Gal_State, "Validate draing flags.\n" );

            gal_validate_df_blend( vdrv, state, modified );
            gal_validate_df_dst_colorkey( vdrv, state, modified );
            gal_validate_df_src_premultiply( vdrv, state, modified );
            gal_validate_df_dst_premultiply( vdrv, state, modified );
            gal_validate_df_demultiply( vdrv, state, modified );
            gal_validate_df_xor( vdrv, state, modified );

            vdrv->draw_flags = state->drawingflags;
        }
        else {
            D_DEBUG_AT( Gal_State, "Drawing state is not changed.\n" );
        }
    }

    if (state_modified & SMF_BLITTING_FLAGS) {
        DFBSurfaceBlittingFlags modified;

        if (vdrv->need_reprogram) {
            modified = ~0;
        }
        else {
            modified = vdrv->blit_flags ^ state->blittingflags;
        }

        D_DEBUG_AT( Gal_State, "Modified blitting flags: 0x%08X.\n", modified );

        if (modified != DSBLIT_NOFX) {
            D_DEBUG_AT( Gal_State, "Validate blitting flags.\n" );

            gal_validate_bf_blend( vdrv, state, modified );
            gal_validate_bf_glb_and_src_modulate( vdrv, state, modified );
            gal_validate_bf_src_colorkey( vdrv, state, modified );
            gal_validate_bf_dst_colorkey( vdrv, state, modified );
            gal_validate_bf_dst_premultiply( vdrv, state, modified );
            gal_validate_bf_demultiply( vdrv, state, modified );
            gal_validate_bf_deinterlace( vdrv, state, modified );
            gal_validate_bf_xor( vdrv, state, modified );
            gal_validate_bf_index_translation( vdrv, state, modified );
            gal_validate_bf_rotation( vdrv, state, modified );
            gal_validate_bf_colorkey_protect( vdrv, state, modified );
            gal_validate_bf_src_mask_alpha( vdrv, state, modified );
            gal_validate_bf_src_mask_color( vdrv, state, modified );

            vdrv->blit_flags = state->blittingflags;
        }
        else {
            D_DEBUG_AT( Gal_State, "Blitting state is not changed.\n" );
        }
    }

    gal_validate_palette( vdrv, state );

    return true;
}

static inline void
gal_post_validate_state( GalDriverData       *vdrv,
                         GalDeviceData       *vdev,
                         CardState           *state,
                         DFBAccelerationMask  accel )
{
    D_ASSERT( vdrv  != NULL );
    D_ASSERT( vdev  != NULL );
    D_ASSERT( state != NULL );

    if (vdrv->vdev->hw_2d_pe20) {
        if (DFB_BLITTING_FUNCTION( accel )) {
            bool need_src_alpha_multiply_old, need_glb_alpha_multiply_old;

            /* Combine the ROP flag for src and dst color key operations. */
            if (vdrv->src_ckey_blit_rop == 0xAA ||
                vdrv->dst_ckey_blit_rop == 0xAA) {
                vdrv->blit_bg_rop = 0xAA;
            }
            else {
                vdrv->blit_bg_rop = 0xCC;
            }

            /* Disable smooth scale when color key is in use. */
            if (vdrv->src_trans_mode == gcv2D_KEYED ||
                vdrv->dst_trans_mode == gcv2D_KEYED) {
                vdrv->ckey_workaround = true;
            }
            else {
                vdrv->ckey_workaround = false;
            }

            if (!vdrv->vdev->hw_2d_full_dfb) {
	            /* Workarounds for DSBF_SRCALPHA blending. */
	            need_src_alpha_multiply_old = vdrv->need_src_alpha_multiply;
	            need_glb_alpha_multiply_old = vdrv->need_glb_alpha_multiply;

	            if (state->src_blend == DSBF_SRCALPHA) {
	                if (state->blittingflags & DSBLIT_BLEND_ALPHACHANNEL) {
	                    vdrv->need_src_alpha_multiply = true;
	                }
	                else {
	                    vdrv->need_src_alpha_multiply = false;
	                }

	                if (state->blittingflags & DSBLIT_BLEND_COLORALPHA) {
	                    vdrv->need_glb_alpha_multiply = true;
	                }
	                else {
	                    vdrv->need_glb_alpha_multiply = false;
	                }
	            }
	            else {
	                vdrv->need_src_alpha_multiply = false;
	                vdrv->need_glb_alpha_multiply = false;
	            }

	            if ((vdrv->need_src_alpha_multiply != need_src_alpha_multiply_old) ||
	                (vdrv->need_glb_alpha_multiply != need_glb_alpha_multiply_old)) {
	                if (vdrv->need_src_alpha_multiply) {
	                    vdrv->saved_modulate_flag |= GMF_SRC_PREMULTIPLY;
	                }

	                GAL_HSVF_INVALIDATE( HSVF_MULTIPLY );
	            }

	            /* Workarounds for DST_COLORKEY/XOR/BLENDING issue. */
	            if ((state->blittingflags & (DSBLIT_BLEND_ALPHACHANNEL | DSBLIT_BLEND_COLORALPHA)) &&
	                (state->blittingflags & (DSBLIT_DST_COLORKEY | DSBLIT_XOR))) {
	                vdrv->need_rop_blend_workaround = true;

	                _AllocateTempSurf( vdrv, vdev );
	            }
	            else {
	                vdrv->need_rop_blend_workaround = false;
	            }
            }

            /* Workarounds for A8 blit. */
            if (state->source->config.format == DSPF_A8) {
                if (vdrv->src_pmp_glb_mode == gcv2D_GLOBAL_COLOR_MULTIPLY_COLOR) {
                    vdrv->src_pmp_glb_mode = gcv2D_GLOBAL_COLOR_MULTIPLY_DISABLE;
                    vdrv->saved_modulate_flag &= ~GMF_GLOBAL_COLOR_MULTIPLY_COLOR;
                }
            }
        }
        else if (!vdrv->vdev->hw_2d_full_dfb) {
            bool need_glb_alpha_multiply_old;

            /* Workarounds for DSBF_SRCALPHA blending. */
            need_glb_alpha_multiply_old = vdrv->need_glb_alpha_multiply;

            if (state->src_blend == DSBF_SRCALPHA) {
                if (state->drawingflags & DSDRAW_BLEND) {
                    vdrv->need_glb_alpha_multiply = true;
                }
                else {
                    vdrv->need_glb_alpha_multiply = false;
                }
            }
            else {
                vdrv->need_glb_alpha_multiply = false;
            }

            if ((vdrv->need_glb_alpha_multiply != need_glb_alpha_multiply_old) ||
                (vdrv->need_src_alpha_multiply != false)) {
                vdrv->need_src_alpha_multiply = false;

                GAL_HSVF_INVALIDATE( HSVF_MULTIPLY );
            }
        }
    }

    /* Check for threshold settings. */
    if (vdev->turn_on_threshold) {
        if ((DFB_BLITTING_FUNCTION( accel ) && state->blittingflags != DSBLIT_NOFX) ||
            (DFB_DRAWING_FUNCTION( accel ) && state->drawingflags != DSDRAW_NOFX)) {
            vdev->disable_threshold = true;
        }
        else {
            vdev->disable_threshold = false;
        }
    }

    return;
}

static inline void
gal_program_src_ckey_value( GalDriverData *vdrv,
                            GalDeviceData *vdev )
{
    gceSTATUS status;

    D_ASSERT( vdrv != NULL );
    D_ASSERT( vdev != NULL );

    if (GAL_HSVF_IS_VALID( HSVF_SRC_CKEY_VALUE )) {
        D_DEBUG_AT( Gal_State,
                    "HSVF_SRC_CKEY_VALUE has already been validated.\n" );
        return;
    }

    do {
        D_DEBUG_AT( Gal_State,
                    "Set source color key: (%02X, %02X, %02X, %02X).\n",
                    GAL_COLOR_VALS(&vdrv->src_ckey));

        gcmERR_BREAK(gco2D_SetSourceColorKeyAdvanced( vdrv->engine_2d,
                                                        vdrv->src_ckey32 ));

        GAL_HSVF_VALIDATE( HSVF_SRC_CKEY_VALUE );
    } while (0);

    if (gcmIS_ERROR( status )) {
        D_DEBUG_AT( Gal_State, "Failed to set source color key value.\n" );
    }

    D_DEBUG_EXIT( Gal_State, "\n" );

    return;
}

static inline void
gal_program_dst_ckey_value( GalDriverData *vdrv,
                            GalDeviceData *vdev )
{
    gceSTATUS status;

    D_ASSERT( vdrv != NULL );
    D_ASSERT( vdev != NULL );

    if (GAL_HSVF_IS_VALID( HSVF_DST_CKEY_VALUE )) {
        D_DEBUG_AT( Gal_State,
                    "HSVF_DST_CKEY_VALUE has already been validated.\n" );
        return;
    }

    do {
        D_DEBUG_AT( Gal_State,
                    "Set dest color key: (%02X, %02X, %02X, %02X).\n",
                    GAL_COLOR_VALS(&vdrv->dst_ckey));

        gcmERR_BREAK(gco2D_SetTargetColorKeyAdvanced( vdrv->engine_2d,
                                                        vdrv->dst_ckey32 ));

        GAL_HSVF_VALIDATE( HSVF_DST_CKEY_VALUE );
    } while (0);

    if (gcmIS_ERROR( status )) {
        D_DEBUG_AT( Gal_State, "Failed to set dest color key value.\n" );
    }

    D_DEBUG_EXIT( Gal_State, "\n" );

    return;
}

static inline void
gal_program_ckey_mode( GalDriverData *vdrv,
                       GalDeviceData *vdev )
{
    gceSTATUS status = gcvSTATUS_OK;

    D_ASSERT( vdrv != NULL );
    D_ASSERT( vdev != NULL );

    if (GAL_HSVF_IS_VALID( HSVF_CKEY_MODE )) {
        D_DEBUG_AT( Gal_State,
                    "HSVF_CKEY_MODE has already been validated.\n" );
        return;
    }

    do {

        D_DEBUG_AT( Gal_State,
                    "gco2D_SetTransparencyAdvanced: (0x%08X, 0x%08X, 0x%08X).\n",
                    vdrv->src_trans_mode,
                    vdrv->dst_trans_mode,
                    vdrv->pat_trans_mode );

        gcmERR_BREAK(gco2D_SetTransparencyAdvanced( vdrv->engine_2d,
                                                    vdrv->src_trans_mode,
                                                    vdrv->dst_trans_mode,
                                                    vdrv->pat_trans_mode ));

        GAL_HSVF_VALIDATE( HSVF_CKEY_MODE );

    } while (0);

    if (gcmIS_ERROR( status )) {
        D_DEBUG_AT( Gal_State, "Failed to set color key mode.\n" );
    }

    D_DEBUG_EXIT( Gal_State, "\n" );

    return;
}

static inline void
gal_program_color( GalDriverData *vdrv,
                   GalDeviceData *vdev )
{
    gceSTATUS status = gcvSTATUS_NOT_SUPPORTED;

    D_ASSERT( vdrv != NULL );
    D_ASSERT( vdev != NULL );

    if (GAL_HSVF_IS_VALID( HSVF_COLOR )) {
        D_DEBUG_AT( Gal_State, "HSVF_COLOR has already been validated.\n" );

        return;
    }

    D_DEBUG_ENTER( Gal_State, "\n" );

    do {
        bool no_validate_color = false;

        if (!vdrv->vdev->hw_2d_pe20) {
            /* Clear the pool surface. */
            if (vdrv->use_pool_source) {
                gcmERR_BREAK(gco2D_SetClipping( vdrv->engine_2d,
                                                &vdrv->pool_src_rect ));

                gcmERR_BREAK(gco2D_SetTarget( vdrv->engine_2d,
                                              vdrv->pool_src_phys_addr,
                                              vdrv->pool_src_pitch,
                                              gcvSURF_0_DEGREE,
                                              0 ));

                gcmERR_BREAK(gco2D_Clear( vdrv->engine_2d,
                                          1,
                                          &vdrv->pool_src_rect,
                                          vdrv->color,
                                          0xCC,
                                          0xCC,
                                          vdrv->pool_src_native_format ));

                GAL_HSVF_INVALIDATE( HSVF_CLIP );
                GAL_HSVF_INVALIDATE( HSVF_TARGET );
                GAL_HSVF_INVALIDATE( HSVF_SOURCE );
            }
        }
        else {
            if ((DFB_DRAWING_FUNCTION( vdrv->accel ))) {
                D_DEBUG_AT( Gal_State,
                            "[gco2D_SetSourceGlobalColorAdvanced] color: 0x%08X.\n",
                            vdrv->color );

                gcmERR_BREAK(gco2D_SetSourceGlobalColorAdvanced( vdrv->engine_2d,
                                                                 vdrv->color ));
            } else {
                if ((vdrv->src_format == DSPF_A8) &&
                    !(vdrv->blit_flags & DSBLIT_COLORIZE)) {
                    gcmERR_BREAK(gco2D_SetSourceGlobalColorAdvanced( vdrv->engine_2d,
                                                                     (vdrv->modulate_color & 0xFF000000) | 0x00FFFFFF ));

                    no_validate_color = true;
                }
                else {
                    D_DEBUG_AT( Gal_State,
                                "[gco2D_SetSourceGlobalColorAdvanced] modulate_color: 0x%08X.\n",
                                vdrv->modulate_color );

                    gcmERR_BREAK(gco2D_SetSourceGlobalColorAdvanced( vdrv->engine_2d,
                                                                        vdrv->modulate_color ));
                }
            }
        }

        if (!no_validate_color) {
            GAL_HSVF_VALIDATE( HSVF_COLOR );
        }
    } while (0);

    if (gcmIS_ERROR( status )) {
        D_DEBUG_AT( Gal_State, "Failed to program color.\n" );
        return;
    }

    D_DEBUG_EXIT( Gal_State, "\n" );

    return;
}

static inline void
gal_program_source( GalDriverData *vdrv,
                    GalDeviceData *vdev )
{
    gceSTATUS status = gcvSTATUS_OK;

    D_ASSERT( vdrv != NULL );
    D_ASSERT( vdev != NULL );

    if (GAL_HSVF_IS_VALID( HSVF_SOURCE )) {
        D_DEBUG_AT( Gal_State, "HSVF_SOURCE has already been validated.\n" );

        return;
    }

    D_DEBUG_ENTER( Gal_State, "\n" );

    do {
        if (!vdrv->vdev->hw_2d_pe20) {
            unsigned int         src_phys_addr;
            int                  src_pitch;
            gceSURF_FORMAT       src_format;
            gceSURF_ROTATION     src_rotation;
            unsigned int         src_aligned_width;
            unsigned char        fg_rop, bg_rop;
            gceSURF_TRANSPARENCY trans;

            if (vdrv->use_pool_source) {
                src_phys_addr     = vdrv->pool_src_phys_addr;
                src_pitch         = vdrv->pool_src_pitch;
                src_format        = vdrv->pool_src_native_format;
                src_rotation      = gcvSURF_0_DEGREE;
                src_aligned_width = vdrv->pool_src_aligned_width;

                fg_rop            = vdrv->draw_fg_rop;
                bg_rop            = vdrv->draw_bg_rop;
                trans             = vdrv->draw_trans;
            }
            else {
                src_phys_addr     = vdrv->src_phys_addr[0];
                src_pitch         = vdrv->src_pitch[0];
                src_format        = vdrv->src_native_format;
                src_rotation      = vdrv->src_rotation;
                src_aligned_width = vdrv->src_aligned_width;

                fg_rop            = vdrv->blit_fg_rop;
                bg_rop            = vdrv->blit_bg_rop;
                trans             = vdrv->blit_trans;
            }

            gcmERR_BREAK(gco2D_SetColorSource( vdrv->engine_2d,
                                               src_phys_addr,
                                               src_pitch,
                                               src_format,
                                               src_rotation,
                                               src_aligned_width,
                                               gcvFALSE,
                                               trans,
                                               vdrv->trans_color ));
        }
        else {
            if (vdrv->use_source) {
                gcmERR_BREAK(gco2D_SetColorSourceAdvanced( vdrv->engine_2d,
                                                           vdrv->src_phys_addr[0],
                                                           vdrv->src_pitch[0],
                                                           vdrv->src_native_format,
                                                           vdrv->src_rotation,
                                                           vdrv->src_aligned_width,
                                                           vdrv->src_aligned_height,
                                                           gcvFALSE ));
            }
        }

        GAL_HSVF_VALIDATE( HSVF_SOURCE );
    } while (0);

    if (gcmIS_ERROR( status )) {
        D_DEBUG_AT( Gal_State, "Failed to set color source.\n" );
        return;
    }

    D_DEBUG_EXIT( Gal_State, "\n" );

    return;
}

static inline void
gal_program_target( GalDriverData *vdrv,
                    GalDeviceData *vdev )
{
    gceSTATUS status = gcvSTATUS_OK;

    D_ASSERT( vdrv != NULL );
    D_ASSERT( vdev != NULL );

    if (GAL_HSVF_IS_VALID( HSVF_TARGET )) {
        D_DEBUG_AT( Gal_State, "HSVF_TARGET has already been validated.\n" );

        return;
    }

    D_DEBUG_ENTER( Gal_State, "\n" );

    do {
        D_DEBUG_AT( Gal_State,
                    "dst_native_format: 0x%08X, dst_pitch: %u, dst_logic_addr: %p, dst_phys_addr: 0x%08X.\n",
                    vdrv->dst_native_format, vdrv->dst_pitch, vdrv->dst_logic_addr, vdrv->dst_phys_addr );

        gcmERR_BREAK(gco2D_SetTarget( vdrv->engine_2d,
                                      vdrv->dst_phys_addr,
                                      vdrv->dst_pitch,
                                      vdrv->dst_rotation,
                                      vdrv->dst_aligned_width ));

        GAL_HSVF_VALIDATE( HSVF_TARGET );
    } while (0);

    if (gcmIS_ERROR( status )) {
        D_DEBUG_AT( Gal_State, "Set target failed.\n" );
        return;
    }

    D_DEBUG_EXIT( Gal_State, "\n" );

    return;
}

static inline void
gal_program_clip( GalDriverData *vdrv,
                  GalDeviceData *vdev )
{
    gceSTATUS status = gcvSTATUS_OK;

    D_ASSERT( vdrv != NULL );
    D_ASSERT( vdev != NULL );

    if (GAL_HSVF_IS_VALID( HSVF_CLIP )) {
        D_DEBUG_AT( Gal_State, "HSVF_CLIP has already been validated.\n" );

        return;
    }

    D_DEBUG_ENTER( Gal_State, "\n" );

    D_DEBUG_AT( Gal_State,
                "clip [%u, %u, %u, %u]\n",
                vdrv->clip.left, vdrv->clip.top,
                vdrv->clip.right, vdrv->clip.bottom );

    do {
        gcmERR_BREAK(gco2D_SetClipping( vdrv->engine_2d,
                                        &vdrv->clip ));

        GAL_HSVF_VALIDATE( HSVF_CLIP );
    } while (0);

    if (gcmIS_ERROR( status )) {
        D_DEBUG_AT( Gal_State, "Set clipping failed.\n" );
        return;
    }

    D_DEBUG_EXIT( Gal_State, "\n" );

    return;
}

static inline void
gal_program_palette( GalDriverData *vdrv,
                     GalDeviceData *vdev )
{
    gceSTATUS status = gcvSTATUS_OK;

    D_DEBUG_ENTER( Gal_State, "\n" );

    D_ASSERT( vdrv != NULL );
    D_ASSERT( vdev != NULL );

    if (GAL_HSVF_IS_VALID( HSVF_PALETTE )) {
        D_DEBUG_AT( Gal_State, "HSVF_PALETTE has already been validated.\n" );

        return;
    }

    do {
        gcmERR_BREAK(gco2D_LoadPalette( vdrv->engine_2d,
                                        0,
                                        vdev->color_table_len,
                                        vdev->color_table,
                                        gcvTRUE ));

        GAL_HSVF_VALIDATE( HSVF_PALETTE );
    } while (0);

    if (gcmIS_ERROR( status )) {
        D_DEBUG_AT( Gal_State, "Failed to set the palette.\n" );

        return;
    }

    D_DEBUG_EXIT( Gal_State, "\n" );

    return;
}

static inline void
gal_program_blend( GalDriverData *vdrv,
                   GalDeviceData *vdev )
{
    gceSTATUS status = gcvSTATUS_OK;

    D_ASSERT( vdrv != NULL );
    D_ASSERT( vdev != NULL );

    if (GAL_HSVF_IS_VALID( HSVF_BLEND )) {
        D_DEBUG_AT( Gal_State, "HSVF_BLEND has already been validated.\n" );

        return;
    }

    D_DEBUG_ENTER( Gal_State, "\n" );

    do {
        /* Disable workaround for 16-bit blending by default. */
        vdrv->dst_blend_no_alpha = false;

        if ((DFB_DRAWING_FUNCTION( vdrv->accel ) && vdrv->draw_blend) ||
            (DFB_BLITTING_FUNCTION( vdrv->accel ) && vdrv->blit_blend)) {
            gceSURF_GLOBAL_ALPHA_MODE src_global_alpha_mode;
            gceSURF_GLOBAL_ALPHA_MODE dst_global_alpha_mode;

            D_DEBUG_AT( Gal_State, "Enable blending.\n");

            if (DFB_DRAWING_FUNCTION( vdrv->accel )) {
                src_global_alpha_mode  = vdrv->draw_src_global_alpha_mode;
                dst_global_alpha_mode  = vdrv->draw_dst_global_alpha_mode;
            }
            else {
                src_global_alpha_mode  = vdrv->blit_src_global_alpha_mode;
                dst_global_alpha_mode  = vdrv->blit_dst_global_alpha_mode;
            }

            if (!vdrv->vdev->hw_2d_pe20) {
                D_DEBUG_AT( Gal_State,
                            "src_blend: %d, dst_blend: %d\n"
                            "global_alpha_value: 0x%08X\n"
                            "src_alpha_mode: 0x%08X, dst_alpha_mode: 0x%08X\n"
                            "src_global_alpha_mode: 0x%08X, dst_global_alpha_mode: 0x%08X\n"
                            "src_factor_mode: 0x%08X, dst_factor_mode: 0x%08X\n"
                            "src_color_mode: 0x%08X, dst_color_mode: 0x%08X\n",
                            vdrv->src_blend,
                            vdrv->dst_blend,
                            vdrv->global_alpha_value,
                            vdrv->src_blend_funcs[vdrv->src_blend - 1].alpha_mode,
                            vdrv->dst_blend_funcs[vdrv->dst_blend - 1].alpha_mode,
                            src_global_alpha_mode,
                            dst_global_alpha_mode,
                            vdrv->src_blend_funcs[vdrv->src_blend - 1].factor_mode,
                            vdrv->dst_blend_funcs[vdrv->dst_blend - 1].factor_mode,
                            vdrv->src_blend_funcs[vdrv->src_blend - 1].color_mode,
                            vdrv->dst_blend_funcs[vdrv->dst_blend - 1].color_mode );

                if (!DFB_PIXELFORMAT_HAS_ALPHA( vdrv->dst_format )) {
                    /* Workaround for dest without alpha channel. */
                    gcmERR_BREAK(_AllocateTempSurf( vdrv, vdev ));

                    vdrv->dst_blend_no_alpha = true;
                }
                else {
                    gcmERR_BREAK(gco2D_EnableAlphaBlend( vdrv->engine_2d,
                                                         vdrv->global_alpha_value,
                                                         vdrv->global_alpha_value,
                                                         gcvSURF_PIXEL_ALPHA_STRAIGHT,
                                                         gcvSURF_PIXEL_ALPHA_STRAIGHT,
                                                         src_global_alpha_mode,
                                                         gcvSURF_GLOBAL_ALPHA_OFF,
                                                         vdrv->src_blend_funcs[vdrv->src_blend - 1].factor_mode,
                                                         vdrv->dst_blend_funcs[vdrv->dst_blend - 1].factor_mode,
                                                         vdrv->src_blend_funcs[vdrv->src_blend - 1].color_mode,
                                                         vdrv->dst_blend_funcs[vdrv->dst_blend - 1].color_mode ));
                }
            }
            else {
                gcmERR_BREAK(gco2D_EnableAlphaBlendAdvanced( vdrv->engine_2d,
                                                             gcvSURF_PIXEL_ALPHA_STRAIGHT,
                                                             gcvSURF_PIXEL_ALPHA_STRAIGHT,
                                                             src_global_alpha_mode,
                                                             gcvSURF_GLOBAL_ALPHA_OFF,
                                                             vdrv->src_blend_funcs[vdrv->src_blend - 1].factor_mode,
                                                             vdrv->dst_blend_funcs[vdrv->dst_blend - 1].factor_mode ));
            }
        }
        else {
            D_DEBUG_AT( Gal_State, "Disable blending.\n" );

            gcmERR_BREAK(gco2D_DisableAlphaBlend( vdrv->engine_2d ));
        }

        GAL_HSVF_VALIDATE( HSVF_BLEND );

    } while (0);

    if (gcmIS_ERROR( status )) {
        D_DEBUG_AT( Gal_State, "Set blend failed.\n" );

        return;
    }

    D_DEBUG_EXIT( Gal_State, "\n" );

    return;
}

static inline void
gal_program_multiply( GalDriverData *vdrv,
                      GalDeviceData *vdev )
{
    gceSTATUS status = gcvSTATUS_OK;

    D_ASSERT( vdrv != NULL );
    D_ASSERT( vdev != NULL );

    if (GAL_HSVF_IS_VALID( HSVF_MULTIPLY )) {
        D_DEBUG_AT( Gal_State, "HSVF_MULTIPLY has already been validated.\n" );

        return;
    }

    D_DEBUG_ENTER( Gal_State, "\n" );

    do {
        gce2D_PIXEL_COLOR_MULTIPLY_MODE src_pmp_src_alpha, dst_pmp_dst_alpha, dst_dmp_dst_alpha;
        gce2D_GLOBAL_COLOR_MULTIPLY_MODE src_pmp_glb_mode;

        if (vdrv->need_src_alpha_multiply) {
            if (vdrv->src_pmp_src_alpha != gcv2D_COLOR_MULTIPLY_DISABLE) {
                D_WARN( "Conflict src premultiply flag. The previous one is %d and the new one is %d.\n",
                        vdrv->src_pmp_src_alpha, gcv2D_COLOR_MULTIPLY_ENABLE );
            }

            src_pmp_src_alpha = gcv2D_COLOR_MULTIPLY_ENABLE;
            vdrv->saved_modulate_flag |= GMF_SRC_PREMULTIPLY;
        }
        else {
            src_pmp_src_alpha = vdrv->src_pmp_src_alpha;
        }

        dst_pmp_dst_alpha = vdrv->dst_pmp_dst_alpha;

        if (vdrv->need_glb_alpha_multiply) {
            if (vdrv->src_pmp_glb_mode != gcv2D_GLOBAL_COLOR_MULTIPLY_DISABLE) {
                D_WARN( "Conflict global alpha multiply flag. The previous one is %d and the new one is %d.\n",
                        vdrv->src_pmp_glb_mode, gcv2D_GLOBAL_COLOR_MULTIPLY_ALPHA );
            }

            src_pmp_glb_mode = gcv2D_GLOBAL_COLOR_MULTIPLY_ALPHA;
            vdrv->saved_modulate_flag |= GMF_GLOBAL_COLOR_MULTIPLY_ALPHA;
        }
        else {
            src_pmp_glb_mode = vdrv->src_pmp_glb_mode;
        }

        dst_dmp_dst_alpha = vdrv->dst_dmp_dst_alpha;

        D_DEBUG_AT( Gal_State,
                    "gco2D_SetPixelMultiplyModeAdvanced: (%d, %d, %d, %d)\n",
                    src_pmp_src_alpha,
                    dst_pmp_dst_alpha,
                    src_pmp_glb_mode,
                    dst_dmp_dst_alpha );

        gcmERR_BREAK(gco2D_SetPixelMultiplyModeAdvanced( vdrv->engine_2d,
                                                         src_pmp_src_alpha,
                                                         dst_pmp_dst_alpha,
                                                         src_pmp_glb_mode,
                                                         dst_dmp_dst_alpha ));

        GAL_HSVF_VALIDATE( HSVF_MULTIPLY );
    } while (0);

    if (gcmIS_ERROR( status )) {
        D_DEBUG_AT( Gal_State, "Set multiply failed.\n" );

        return;
    }

    D_DEBUG_EXIT( Gal_State, "\n" );

    return;
}

static inline void
gal_program_mirror( GalDriverData *vdrv,
                    GalDeviceData *vdev )
{
    gceSTATUS status = gcvSTATUS_OK;

    bool hor_mirror, ver_mirror;

    D_ASSERT( vdrv != NULL );
    D_ASSERT( vdev != NULL );

    if (GAL_HSVF_IS_VALID( HSVF_MIRROR )) {
        D_DEBUG_AT( Gal_State, "HSVF_MIRROR has already been validated.\n" );

        return;
    }

    D_DEBUG_ENTER( Gal_State, "\n" );

    do {
        if (DFB_DRAWING_FUNCTION( vdrv->accel )) {
            hor_mirror = vdrv->draw_hor_mirror;
            ver_mirror = vdrv->draw_ver_mirror;
        }
        else {
            hor_mirror = vdrv->blit_hor_mirror;
            ver_mirror = vdrv->blit_ver_mirror;
        }

        D_DEBUG_AT( Gal_State,
                    "hor mirror: %s, ver mirror: %s\n",
                    hor_mirror ? "true" : "false",
                    ver_mirror ? "true" : "false" );

        gcmERR_BREAK(gco2D_SetBitBlitMirror( vdrv->engine_2d,
                                             hor_mirror,
                                             ver_mirror ));

        GAL_HSVF_VALIDATE( HSVF_MIRROR );
    } while (0);

    if (gcmIS_ERROR( status )) {
        D_DEBUG_AT( Gal_State, "Set mirror failed.\n" );

        return;
    }

    D_DEBUG_EXIT( Gal_State, "\n" );

    return;
}

static void
gal_program_state( GalDriverData *vdrv,
                   GalDeviceData *vdev )
{
    D_ASSERT( vdrv != NULL );
    D_ASSERT( vdev != NULL );

    D_DEBUG_ENTER( Gal_State, "\n" );

    D_DEBUG_AT( Gal_State,
                "The state of the vdev.\n"
                "clip: (%u, %u, %u, %u)\n"
                "brush color: 0x%08X\n"
                "color: 0x%08X\n"
                "src_blend: %u, dst_blend: %u\n"
                "src_ckey: 0x%08X\n"
                "destination (phys, pitch, format): (0x%08X, %u, %u)\n",
                GAL_RECTANGLE_VALS(&vdrv->clip),
                vdrv->brush_color,
                vdrv->color,
                vdrv->src_blend, vdrv->dst_blend,
                vdrv->trans_color,
                vdrv->dst_phys_addr, vdrv->dst_pitch, vdrv->dst_native_format );

    if (vdrv->use_pool_source) {
        D_DEBUG_AT( Gal_State,
                    "use_pool_source is true\n"
                    "source (phys, pitch, format): (0x%08X, %u, %u)\n"
                    "draw trans: 0x%08X\n"
                    "draw fg_rop: 0x%08X, bg rop: 0x%08X\n"
                    "draw_blend: %u\n",
                    vdrv->pool_src_phys_addr, vdrv->pool_src_pitch, vdrv->pool_src_native_format,
                    vdrv->draw_trans,
                    vdrv->draw_fg_rop, vdrv->draw_bg_rop,
                    vdrv->draw_blend );
    }
    else {
        D_DEBUG_AT( Gal_State,
                    "use_pool_source is false\n"
                    "source (phys, pitch, format): (0x%08X, %u, %u)\n"
                    "blit trans: 0x%08X\n"
                    "blit fg_rop: 0x%08X, bg rop: 0x%08X\n"
                    "blit_blend: %u\n",
                    vdrv->src_phys_addr[0], vdrv->src_pitch[0], vdrv->src_native_format,
                    vdrv->blit_trans,
                    vdrv->blit_fg_rop, vdrv->blit_bg_rop,
                    vdrv->blit_blend );
    }

    gal_program_blend( vdrv, vdev );
    gal_program_color( vdrv, vdev );
    gal_program_clip( vdrv, vdev );
    gal_program_mirror( vdrv, vdev );
    gal_program_palette( vdrv, vdev );
    gal_program_source( vdrv, vdev );
    gal_program_target( vdrv, vdev );
    if (vdrv->vdev->hw_2d_pe20) {
        gal_program_multiply( vdrv, vdev );
        gal_program_src_ckey_value( vdrv, vdev );
        gal_program_dst_ckey_value( vdrv, vdev );
        gal_program_ckey_mode( vdrv, vdev );
    }

    D_DEBUG_EXIT( Gal_State, "\n" );

    return;
}

/**
 * State management.
 */

/**
 * Check if the accel is supported
 * according to the modified flags.
 *
 * Set the coresponding bits in state->accel
 * if it is supported.
 *
 * \param [in] drv: driver specific data
 * \param [in] dev: device specific data
 * \param [in] state: the card state
 * \param [in] accel: the function to be accelerated
 *
 * \return     none
 */
void galCheckState( void                 *drv,
                    void                 *dev,
                    CardState             *state,
                    DFBAccelerationMask  accel )
{
    GalDriverData *vdrv   = drv;
    GalDeviceData *vdev   = dev;
    gceSTATUS      status = gcvSTATUS_NOT_SUPPORTED;

    D_ASSERT( drv   != NULL);
    D_ASSERT( dev   != NULL);
    D_ASSERT( state != NULL);

    D_DEBUG_ENTER( Gal_State,
                   "drv: %p, dev: %p, state: %p, accel: 0x%08X.\n",
                   drv, dev, state, accel );

    do {
        /* Check if it is accelerated according to the configuration. */
        if (!gal_config_accelerated( &vdev->config, state, accel )) {
            break;
        }

        /* Check the destination color formats. */
        if (!gal_is_dest_format( vdev, state->destination->config.format )) {
            D_DEBUG_AT( Gal_State,
                        "Color format: %s is not supported by the destination surface.\n",
                        dfb_pixelformat_name(state->destination->config.format) );

            break;
        }

        if (DFB_BLITTING_FUNCTION( accel )) {
            /* Check the source color formats. */
            if (!gal_is_source_format( vdev, state->source->config.format )) {
                D_DEBUG_AT( Gal_State,
                            "Color format: %s is not supported by the source surface.\n",
                            dfb_pixelformat_name(state->source->config.format) );
                break;
            }
        }

        /* Check drawing functions and flags. */
        if (DFB_DRAWING_FUNCTION( accel )) {
            if (!(accel & ~VIVANTE_GAL_DRAWING_FUNCTIONS) &&
                !(state->drawingflags & ~VIVANTE_GAL_DRAWING_FLAGS)) {
                if ((state->drawingflags & DSDRAW_BLEND) &&
                    (state->src_blend == DSBF_SRCALPHA) &&
                    (state->drawingflags & DSDRAW_SRC_PREMULTIPLY)) {
                    D_DEBUG_AT( Gal_State,
                                "SRCALPHA blend and SRC_PREMULTIPLY are not supported simutaneously.\n" );

                    break;
                }

                if (!vdev->hw_2d_pe20) {
                    /*
                     * ROP and alpha blending could not be supported
                     * simultaneously.
                     */
                    if ((state->drawingflags & DSDRAW_XOR) &&
                        (state->drawingflags & DSDRAW_BLEND)) {
                        D_DEBUG_AT( Gal_State,
                                    "ROP and BLEND are not supported simutaneously.\n" );

                        break;
                    }

                    if (state->drawingflags & DSDRAW_BLEND) {
                        /* The following alpha blending functions are not supported. */
                        if ((state->src_blend == DSBF_SRCCOLOR)        ||
                            (state->src_blend == DSBF_INVSRCCOLOR)    ||
                            (state->src_blend == DSBF_DESTCOLOR)    ||
                            (state->src_blend == DSBF_INVDESTCOLOR)    ||
                            (state->src_blend == DSBF_SRCALPHASAT)) {

                            D_DEBUG_AT( Gal_State,
                                        "BLEND function 0x%08X is not supported.\n",
                                        state->src_blend );

                            break;
                        }
                    }

                    if (state->drawingflags & DSDRAW_DST_COLORKEY) {
                        D_DEBUG_AT( Gal_State,
                                    "DSDRAW_DST_COLORKEY is not supported.\n" );

                        break;
                    }

                    if (state->drawingflags & (DSDRAW_SRC_PREMULTIPLY |
                                               DSDRAW_DST_PREMULTIPLY |
                                               DSDRAW_DEMULTIPLY)) {
                        D_DEBUG_AT( Gal_State,
                                    "DRAW PREMULTIPLY/DEMULTIPLY is not supported.\n" );
                        break;
                    }
                }

                state->accel |= accel;

                status = gcvSTATUS_OK;
            }
            else {
                D_DEBUG_AT( Gal_State,
                            "accel(0x%08X) is not accelerated.\n"
                            "drawingflags: 0x%08X\n",
                            accel,
                            state->drawingflags );
            }
        }
        else {
            /* Check blitting functions and flags. */
            if (!(accel & ~VIVANTE_GAL_BLITTING_FUNCTIONS) &&
                !(state->blittingflags & ~VIVANTE_GAL_BLITTING_FLAGS)) {
                if (!vdrv->vdev->hw_2d_pe20) {
                    /* Check for filter blit limitations */
                    bool use_filter_blit = false;
                    bool use_rotation    = false;
                    bool use_blend       = false;
                    bool use_blit_xor    = false;

                    if (gal_is_yuv_format( state->source->config.format )) {
                        use_filter_blit = true;
                    }

                    if ((state->render_options & DSRO_SMOOTH_UPSCALE) ||
                        (state->render_options & DSRO_SMOOTH_DOWNSCALE)) {
                        use_filter_blit = true;
                    }

                    if (state->blittingflags & (DSBLIT_BLEND_ALPHACHANNEL |
                                                DSBLIT_BLEND_COLORALPHA)) {
                        use_blend = true;
                    }

                    if (state->blittingflags & DSBLIT_XOR) {
                        use_blit_xor = true;
                    }

                    if (use_filter_blit && (use_rotation || use_blend || use_blit_xor)) {
                        status = gcvSTATUS_NOT_SUPPORTED;
                        D_DEBUG_AT( Gal_State,
                                    "Filter blit doesn't support rotation, alpha blending and xor.\n" );

                        break;
                    }

                    /*
                     * ROP and alpha blending could not be supported
                     * simultaneously.
                     */
                    if ((state->blittingflags & DSBLIT_XOR) &&
                        (state->blittingflags & (DSBLIT_BLEND_ALPHACHANNEL | DSBLIT_BLEND_COLORALPHA))) {
                        status = gcvSTATUS_NOT_SUPPORTED;
                        D_DEBUG_AT( Gal_State,
                                    "ROP and BLEND are not supported simutaneously.\n" );

                        break;
                    }

                    /* The following blending functions are not supported. */
                    if (state->blittingflags & (DSBLIT_BLEND_ALPHACHANNEL | DSBLIT_BLEND_COLORALPHA)) {
                        if ((state->src_blend == DSBF_SRCCOLOR)     ||
                            (state->src_blend == DSBF_INVSRCCOLOR)  ||
                            (state->src_blend == DSBF_DESTCOLOR)    ||
                            (state->src_blend == DSBF_INVDESTCOLOR) ||
                            (state->src_blend == DSBF_SRCALPHASAT)) {

                            D_DEBUG_AT( Gal_State,
                                        "BLEND function 0x%08X is not supported.\n",
                                        state->src_blend );

                            break;
                        }
                    }

                    if (state->blittingflags & DSBLIT_DST_COLORKEY) {
                        D_DEBUG_AT( Gal_State,
                                    "DSBLIT_DST_COLORKEY is not supported.\n" );

                        break;
                    }

                    if (state->blittingflags & DSBLIT_COLORIZE) {
                        D_DEBUG_AT( Gal_State,
                                    "DSBLIT_COLORIZE is not supported.\n" );

                        break;
                    }

                    if (state->blittingflags & (DSBLIT_SRC_PREMULTIPLY |
                                                DSBLIT_DST_PREMULTIPLY |
                                                DSBLIT_DEMULTIPLY)) {
                        D_DEBUG_AT( Gal_State,
                                    "BLIT PREMULTIPLY/DEMULTIPLY is not supported.\n" );

                        break;
                    }
                }
                else if (!vdrv->vdev->hw_2d_full_dfb) {
	                if (state->src_blend == DSBF_SRCALPHA) {
		                unsigned int modulate_flag;
		                unsigned int index = 0;

		                /* Translate the multiply flags. */
		                if (state->blittingflags & DSBLIT_COLORIZE) {
		                    index |= GMI_COLORIZE;
		                }

		                if (state->blittingflags & DSBLIT_SRC_PREMULTCOLOR) {
		                    index |= GMI_SRC_PREMULTCOLOR;
		                }

		                if (state->blittingflags & DSBLIT_SRC_PREMULTIPLY) {
		                    index |= GMI_SRC_PREMULTIPLY;
		                }

		                if (state->blittingflags & DSBLIT_BLEND_ALPHACHANNEL) {
		                    index |= GMI_BLEND_ALPHACHANNEL;
		                }

		                if (state->blittingflags & DSBLIT_BLEND_COLORALPHA) {
		                    index |= GMI_BLEND_COLORALPHA;
		                }

		                modulate_flag = modulate_flags[index];

	                    if ((state->blittingflags & DSBLIT_BLEND_ALPHACHANNEL) &&
	                        (modulate_flag & GMF_SRC_PREMULTIPLY)) {
	                        D_DEBUG_AT( Gal_State,
	                                    "SRCALPHA blend and SRC_PREMULTIPLY are not supported simutaneously.\n" );

	                        break;
	                    }

	                    if ((state->blittingflags & DSBLIT_BLEND_COLORALPHA) &&
	                        ((modulate_flag & GMF_GLOBAL_COLOR_MULTIPLY_ALPHA) ||
	                         ((modulate_flag & GMF_GLOBAL_COLOR_MULTIPLY_COLOR) &&
                              (state->source && state->source->config.format != DSPF_A8)))) {
	                        D_DEBUG_AT( Gal_State,
	                                    "SRCALPHA blend and (DSBLIT_SRC_PREMULTCOLOR or COLORIZE) are not supported simutaneously.\n" );

	                        break;
	                    }
	                }

                    /* Don't support SRC_COLORKEY and BLEND simulteniously. */
                    if ((state->blittingflags & DSBLIT_SRC_COLORKEY) &&
                        (state->blittingflags & (DSBLIT_BLEND_ALPHACHANNEL |
                                                 DSBLIT_BLEND_COLORALPHA))) {
                        D_DEBUG_AT( Gal_State,
                                    "Don't support SRC_COLORKEY and BLEND simulteniously.\n" );

                        break;
                    }
                }

                state->accel |= accel;

                status = gcvSTATUS_OK;
            }
            else {
                D_DEBUG_AT( Gal_State,
                            "accel(0x%08X) is not accelerated.\n"
                            "blittingflags: 0x%08X\n",
                               accel,
                               state->blittingflags );
            }
        }
    } while (0);

    if (gcmIS_ERROR( status )) {
        D_DEBUG_AT( Gal_State, "0x%08X is not accelerated.\n", accel );

        state->accel &= ~accel;
    }

    D_DEBUG_EXIT( Gal_State, "\n" );

    return;
}

/**
 * Set the hardware state according to the DFB state.
 *
 * \param [in]    drv:   driver specific data
 * \param [in]    dev:   device specific data
 * \param [out]    funcs: you could change the funcs to
 *                       fit specific state
 * \param [in]    state: the card state
 * \param [in]    accel: the function to be accelerated
 *
 * \return    none
 */
void galSetState( void                *drv,
                  void                *dev,
                  GraphicsDeviceFuncs *funcs,
                  CardState           *state,
                  DFBAccelerationMask  accel )
{
    GalDriverData *vdrv = drv;
    GalDeviceData *vdev = dev;

    D_ASSERT( drv   != NULL );
    D_ASSERT( dev   != NULL );
    D_ASSERT( state != NULL );

    D_DEBUG_ENTER( Gal_State,
                   "drv: %p, dev: %p, funcs: %p, "
                   "state: %p, accel: 0x%08X.\n",
                   drv, dev, funcs, state, accel );

    D_DEBUG_AT( Gal_State,
                "accel: 0x%08X\n"
                "mod_hw: 0x%08X\n"
                "drawingflags: 0x%08X\n"
                "blittingflags: 0x%08X\n"
                "clip: (%u, %u, %u, %u)\n"
                "color (A-R-G-B): (0x%02X-0x%02X-0x%02X-0x%02X)\n"
                "src_blend: %u, dst_blend: %u\n"
                "src_ckey: 0x%08X\n"
                "dst_ckey: 0x%08X\n",
                accel,
                state->mod_hw,
                state->drawingflags,
                state->blittingflags,
                DFB_REGION_VALS( &state->clip ),
                state->color.a, state->color.r, state->color.g, state->color.b,
                state->src_blend, state->dst_blend,
                state->src_colorkey,
                state->dst_colorkey );

    D_DEBUG_AT( Gal_State,
                "destination (phys, pitch, format): (0x%08lX, %u, %s)\n",
                state->dst.phys, state->dst.pitch, dfb_pixelformat_name(state->destination->config.format) );

    if (DFB_BLITTING_FUNCTION( accel )) {
        D_DEBUG_AT( Gal_State,
                    "source (phys, pitch, format): (0x%08lX, %u, %s)\n",
                    state->src.phys, state->src.pitch, dfb_pixelformat_name(state->source->config.format) );
    }

    /* Validate the state. */
    if (gal_validate_state( vdrv, vdev, state, accel )) {
        gal_post_validate_state( vdrv, vdev, state, accel );
        gal_program_state( vdrv, vdev );
    }

    state->mod_hw = 0;

    /* Set the flag. */
    state->set |= accel;

    D_DEBUG_EXIT( Gal_State, "\n" );

    return;
}
