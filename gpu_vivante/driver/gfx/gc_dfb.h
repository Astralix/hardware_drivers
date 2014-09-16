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
 * \file gc_dfb.h
 */

#ifndef __gc_dfb_h_
#define __gc_dfb_h_

#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#include <dfb_types.h>

#include <core/state.h>
#include <core/screens.h>
#include <core/layers.h>

#include <gc_hal.h>
#include <gc_hal_raster.h>

#if GAL_SURFACE_POOL
#include "gc_dfb_pool.h"
#endif /* GAL_SURFACE_POOL */

#include "gc_dfb_conf.h"
#include "gc_dfb_state.h"

#define MAX_PENDING_NUM  252
#define GARD_PENDING_NUM 4

#define VIVANTE_GAL_DRAWING_FUNCTIONS ( DFXL_FILLRECTANGLE | \
                                        DFXL_DRAWRECTANGLE | \
                                        DFXL_DRAWLINE      | \
                                        DFXL_FILLTRIANGLE )

#define VIVANTE_GAL_DRAWING_FLAGS ( DSDRAW_XOR             | \
                                    DSDRAW_DST_COLORKEY    | \
                                    DSDRAW_SRC_PREMULTIPLY | \
                                    DSDRAW_DST_PREMULTIPLY | \
                                    DSDRAW_DEMULTIPLY      | \
                                    DSDRAW_BLEND )

#define VIVANTE_GAL_BLITTING_FUNCTIONS ( DFXL_BLIT        | \
                                         DFXL_STRETCHBLIT )

#if (DIRECTFB_MAJOR_VERSION >= 1) && (DIRECTFB_MINOR_VERSION >= 4)
#define VIVANTE_GAL_BLITTING_FLAGS ( DSBLIT_BLEND_ALPHACHANNEL | \
                                     DSBLIT_BLEND_COLORALPHA   | \
                                     DSBLIT_COLORIZE           | \
                                     DSBLIT_SRC_PREMULTIPLY    | \
                                     DSBLIT_DST_PREMULTIPLY    | \
                                     DSBLIT_DEMULTIPLY         | \
                                     DSBLIT_XOR                | \
                                     DSBLIT_SRC_COLORKEY       | \
                                     DSBLIT_DST_COLORKEY       | \
                                     DSBLIT_ROTATE90           | \
                                     DSBLIT_ROTATE180          | \
                                     DSBLIT_ROTATE270 )
#else
#define VIVANTE_GAL_BLITTING_FLAGS ( DSBLIT_BLEND_ALPHACHANNEL | \
                                     DSBLIT_BLEND_COLORALPHA   | \
                                     DSBLIT_COLORIZE           | \
                                     DSBLIT_SRC_PREMULTIPLY    | \
                                     DSBLIT_DST_PREMULTIPLY    | \
                                     DSBLIT_DEMULTIPLY         | \
                                     DSBLIT_XOR                | \
                                     DSBLIT_SRC_COLORKEY       | \
                                     DSBLIT_DST_COLORKEY       | \
                                     DSBLIT_ROTATE180 )
#endif

#define GAL_HSVF_IS_VALID( flag )   ( (vdrv->valid & (flag)) == (flag) )
#define GAL_HSVF_VALIDATE( flag )   ( vdrv->valid |= (flag) )
#define GAL_HSVF_INVALIDATE( flag ) ( vdrv->valid &= ~(flag) )

#define GAL_PIXELPITCH_ALIGNMENT 16
#define GAL_BYTEOFFSET_ALIGNMENT 64

#define GAL_WIDTH_ALIGNMENT      16
#define GAL_HEIGHT_ALIGNMENT     4

typedef enum {
    GAL_PENDING_NONE,
    GAL_PENDING_BLIT,
    GAL_PENDING_STRETCH,
    GAL_PENDING_LINE,
    GAL_PENDING_FILL,
} GALPendingType;

typedef struct _GalBlendFunc {
    gceSURF_PIXEL_ALPHA_MODE  alpha_mode;
    gceSURF_GLOBAL_ALPHA_MODE global_alpha_mode;
    gceSURF_BLEND_FACTOR_MODE factor_mode;
    gceSURF_PIXEL_COLOR_MODE  color_mode;
} GalBlendFunc;

typedef struct _GalDeviceData {
    int              accelerator;                /**< accelerator */

    /* HW info */
    gceCHIPMODEL     chipModel;         /**< chip model */
    unsigned int     chipRevision;      /**< chip revision */
    unsigned int     chipFeatures;      /**< chip features */
    unsigned int     chipMinorFeatures; /**< chip minor features */

    unsigned int     baseAddress;       /**< memory base address */

#if GAL_SURFACE_POOL
    CoreSurfacePool *pool;              /**< surface pool */
#endif /* GAL_SURFACE_POOL */

    /* HW specific features. */
    bool             hw_2d_pe20;        /**< 2D PE 2.0 is available */
    bool             hw_2d_full_dfb;    /**< 2D core for DFB */

    /* Pending primitives. */
    int              max_pending_num;   /**< the maximum pending number */
    int              gard_pending_num;  /**< the gard pending number */

    /* Index color table. */
    CorePalette     *palette;           /**< the DFB palette */
    int              color_table_len;   /**< the color table length */
    unsigned int     color_table[256];  /**< the color table */

    /* Configuration. */
    GALConfig        config;            /**< configurations according to primitives and features */

    /* Threshold for rendering small objects. */
    bool             turn_on_threshold; /**< use the threshold mechanism */
    bool             disable_threshold; /**< disable threshold checking for some rendering primitives */
    unsigned int     threshold_value;   /**< threshold value in pixels */
} GalDeviceData;

typedef struct _GalDriverData {
    CoreDFB                         *core;                       /**< core object */
    bool                             master;                     /**< master */

    gcoOS                            os;                         /**< OS object */
    gcoHAL                           hal;                        /**< HAL object */
    gco2D                            engine_2d;                  /**< 2D engine object */

    HWStateValidateFlags             valid;                      /**< validation flags */

    /* For blending, 1 pixel color source based on state->color. It is used for old PE (<2.0). */
    gcoSURF                          pool_surf;                  /**< the pool surface */
    unsigned int                     pool_src_phys_addr;         /**< the start physical of the pool surface */
    void                            *pool_src_logic_addr;        /**< the start logical of the pool surface */
    int                              pool_src_pitch;             /**< the pitch of the pool surface */
    DFBSurfacePixelFormat            pool_src_format;            /**< pool surface format in DFB format */
    gceSURF_FORMAT                   pool_src_native_format;     /**< pool surface format in native format */
    gcsRECT                          pool_src_rect;              /**< the pool surface rectangle */
    unsigned int                     pool_src_width;             /**< the width of the pool surface */
    unsigned int                     pool_src_height;            /**< the height of the pool surface */
    unsigned int                     pool_src_aligned_width;     /**< the width of the pool surface */
    unsigned int                     pool_src_aligned_height;    /**< the height of the pool surface */
    bool                             use_pool_source;            /**< use the pool surface */

    /* Source surface. */
    unsigned int                     src_phys_addr[3];           /**< the physical address(es) of the source surface */
    unsigned int                     last_src_phys_addr;         /**< the saved start physical address of the source surface */
    void                            *src_logic_addr[3];          /**< the logical address(es) of the source surface */
    unsigned int                     src_width;                  /**< the width of the source surface in pixels. */
    unsigned int                     src_height;                 /**< the height of the source surface in pixels. */
    unsigned int                     src_aligned_width;          /**< the aligned width of the src surface in pixels */
    unsigned int                     src_aligned_height;         /**< the aligned height of the src surface in pixels */
    int                              src_pitch[3];               /**< the ptich(es) of the source surface */
    DFBSurfacePixelFormat            src_format;                 /**< source surface format in DFB format */
    gceSURF_FORMAT                   src_native_format;          /**< source surface format in native format */
    gceSURF_ROTATION                 src_rotation;               /**< source rotation */
    bool                             use_source;                 /**< source is in use. */

    /* Dest surface. */
    unsigned int                     dst_phys_addr;              /**< the physical address of the dest surface */
    unsigned int                     last_dst_phys_addr;         /**< the saved physical address of the dest surface */
    void                            *dst_logic_addr;             /**< the logical address of the dest surface */
    unsigned int                     dst_width;                  /**< the width of the dest surface in pixels */
    unsigned int                     dst_aligned_width;          /**< the aligned width of the dest surface in pixels */
    unsigned int                     dst_aligned_height;         /**< the aligned height of the dest surface in pixels */
    unsigned int                     dst_height;                 /**< the width of the dest surface in pixels */
    int                              dst_pitch;                  /**< the ptich of the dest surface */
    DFBSurfacePixelFormat            dst_format;                 /**< dest surface format in DFB format */
    gceSURF_FORMAT                   dst_native_format;          /**< dest surface format in navtive format */
    gceSURF_ROTATION                 dst_rotation;               /**< dest rotation */

    /* Color. */
    unsigned int                     brush_color;                /**< the color of the brush */
    unsigned int                     color;                      /**< the global color */
    unsigned int                     modulate_color;             /**< the global color used for modulation */
    unsigned int                     saved_modulate_flag;        /**< saved modulation flag */


    bool                             color_is_changed;           /**< the color has been changed */

    /* Rendering flags. */
    DFBSurfaceDrawingFlags           draw_flags;                 /**< saved drawing flags */
    DFBSurfaceBlittingFlags          blit_flags;                 /**< saved blitting flags */

    /* ROP. */
    unsigned char                    draw_fg_rop;                /**< forground ROP for drawing */
    unsigned char                    draw_bg_rop;                /**< background ROP for drawing */

    unsigned char                    blit_fg_rop;                /**< forground ROP for blitting */
    unsigned char                    blit_bg_rop;                /**< background ROP for blitting */

    unsigned char                    src_ckey_blit_rop;          /**< ROP for src color key blitting. */
    unsigned char                    dst_ckey_blit_rop;          /**< ROP for dst color key blitting. */

    DFBSurfaceBlendFunction          src_blend;                  /**< source blending function */
    DFBSurfaceBlendFunction          dst_blend;                  /**< dest blending function */

    unsigned char                    global_alpha_value;         /**< global alpha value */

    gceSURF_GLOBAL_ALPHA_MODE        draw_src_global_alpha_mode; /**< source alpha mode */
    gceSURF_GLOBAL_ALPHA_MODE        draw_dst_global_alpha_mode; /**< dest alpha mode */

    gceSURF_GLOBAL_ALPHA_MODE        blit_src_global_alpha_mode; /**< source alpha mode */
    gceSURF_GLOBAL_ALPHA_MODE        blit_dst_global_alpha_mode; /**< dest alpha mode */

    /* Clipping. */
    gcsRECT                          clip;                       /**< clipping region */

    /* Color key. */
    gceSURF_TRANSPARENCY             draw_trans;                 /**< whether use color key for drawing */
    gceSURF_TRANSPARENCY             blit_trans;                 /**< whether use color key for blitting */

    unsigned int                     trans_color;                /**< color key */

    gce2D_TRANSPARENCY               src_trans_mode;             /**< source color key mode. */
    DFBColor                         src_ckey;                   /**< source color key. */
    unsigned int                     src_ckey32;                 /**< source color key in A8R8G8B8 format */

    gce2D_TRANSPARENCY               dst_trans_mode;             /**< dest color key mode. */
    DFBColor                         dst_ckey;                   /**< dest color key. */
    unsigned int                     dst_ckey32;                 /**< dest color key in A8R8G8B8 format */

    gce2D_TRANSPARENCY               pat_trans_mode;             /**< pattern color key mode. */

    /* Mirror. */
    bool                             draw_hor_mirror;            /**< horizontal mirror for drawing */
    bool                             draw_ver_mirror;            /**< vertical mirror for drawing */

    bool                             blit_hor_mirror;            /**< horizontal mirror for blitting */
    bool                             blit_ver_mirror;            /**< vertical mirror for blitting */

    /* Workaround. */
    bool                             dst_blend_no_alpha;         /**< workaround for 16-bit alpha blending */
    bool                             need_rop_blend_workaround;  /**< workaround for dst_colorkey/xor/blending issue */

    /*
     * The HW should be reprogramed
     * when shifting between drawing and blitting.
     */
    bool                             need_reprogram;             /**< Need to reprogram the HW. */

    /* Enable alpha blending. */
    bool                             draw_blend;                 /**< enable drawing alpha blending */
    bool                             blit_blend;                 /**< enable blitting alpha blending */

    /* Workarounds for DSBF_SRCALPHA blending. */
    bool                             need_src_alpha_multiply;    /**< need to enable src alpha multiply */
    bool                             need_glb_alpha_multiply;    /**< need to enable glb alpha multiply */

    /* Source is YUV color format. */
    bool                             src_is_yuv_format;          /**< source is YUV color format */

    /* Need smooth scaling. */
    bool                             smooth_scale;               /**< need smooth scaling */

    /* Accel function. */
    DFBAccelerationMask              accel;                      /**< accel function */

    /* HW specific features. */
    bool                             ckey_workaround;            /**< color key is not supported with FilterBlit */

    /* Modified state. */
    StateModificationFlags           state_modified;             /**< modified state */

    /* Premultiply/Demultiply. */
    gce2D_PIXEL_COLOR_MULTIPLY_MODE  src_pmp_src_alpha;          /**< source premultiply with source alpha */
    gce2D_PIXEL_COLOR_MULTIPLY_MODE  dst_pmp_dst_alpha;          /**< dest premultiply with dest alpha */
    gce2D_GLOBAL_COLOR_MULTIPLY_MODE src_pmp_glb_mode;           /**< source premultiply with global color */
    gce2D_PIXEL_COLOR_MULTIPLY_MODE  dst_dmp_dst_alpha;          /**< dest demultiply with dest alpha */

    /* Contiguous memory. */
    gctPHYS_ADDR                     contiguous_phys_addr;       /**< contiguous video memory physical address */
    void                            *contiguous_logic_addr;      /**< contiguous video memory logical address */
    gctSIZE_T                        contiguous_size;            /**< contiguous video memory size */

    /* Pending primitives. */
    int                              pending_num;                /**< pending numbers */
    GALPendingType                   pending_type;               /**< primitive type of pending list */
    gcsRECT                         *pending_src_rects;          /**< pending source rectangle array */
    gcsRECT                         *pending_dst_rects;          /**< pending source rectangle array */
    unsigned int                    *pending_colors;             /**< pending color array */

    /* Tmp dest surface. */
    gcoSURF                          tmp_dst_surf;
    unsigned int                     tmp_dst_phys_addr;
    void                            *tmp_dst_logic_addr;
    unsigned int                     tmp_dst_width;
    unsigned int                     tmp_dst_aligned_width;
    unsigned int                     tmp_dst_height;
    unsigned int                     tmp_dst_aligned_height;
    int                              tmp_dst_pitch;
    gceSURF_FORMAT                   tmp_dst_format;

    /* Alpha blending. */
    GalBlendFunc                    *src_blend_funcs;            /**< source blending function array */
    GalBlendFunc                    *dst_blend_funcs;            /**< dest blending function array */

    /* Dirty flag. */
    bool                             dirty;                      /**< need synchronization */

    /* Device data. */
    GalDeviceData                   *vdev;
} GalDriverData;

#endif /* __gc_dfb_h_ */
