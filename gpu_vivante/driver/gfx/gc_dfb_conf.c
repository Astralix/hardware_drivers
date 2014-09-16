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
 * \file gc_dfb_conf.c
 */

#include <string.h>

#include "gc_dfb.h"
#include "gc_dfb_conf.h"

D_DEBUG_DOMAIN(Gal_Config, "Gal/Config", "Feature configurations.");

/* GAL Default Configuration Flags */
#define GDCF_FILLRECTANGLE (VIVANTE_GAL_DRAWING_FLAGS)
#define GDCF_DRAWRECTANGLE (VIVANTE_GAL_DRAWING_FLAGS)
#define GDCF_DRAWLINE      (VIVANTE_GAL_DRAWING_FLAGS)
#define GDCF_FILLTRIANGLE  (VIVANTE_GAL_DRAWING_FLAGS)
#define GDCF_BLIT          (VIVANTE_GAL_BLITTING_FLAGS)
#define GDCF_STRETCHBLIT   (VIVANTE_GAL_BLITTING_FLAGS)

/* Primitive names. */
#define FILLRECTANGLE_NAME     "fillrectangle"
#define DRAWRECTANGLE_NAME     "drawrectangle"
#define DRAWLINE_NAME          "drawline"
#define FILLTRIANGLE_NAME      "filltriangle"

#define BLIT_NAME              "blit"
#define STRETCHBLIT_NAME       "stretchblit"

/* Flag names. */
#define RENDER_NONE_NAME       "none"

#define DRAW_XOR_NAME          "xor"
#define DRAW_BLEND_NAME        "blend"

#define BLIT_XOR_NAME          "xor"
#define BLIT_ALPHACHANNEL_NAME "alphachannel"
#define BLIT_COLORALPHA_NAME   "coloralpha"
#define BLIT_COLORIZE_NAME     "colorize"
#define BLIT_SRC_COLORKEY_NAME "src_colorkey"
#define BLIT_DST_COLORKEY_NAME "dst_colorkey"
#define BLIT_ROTATION180_NAME  "rotate180"

/**
 * Init the configurations.
 *
 * \param [in]    none
 *
 * \return    DFB_OK success
 *
 */
DFBResult
gal_config_init( void        *dev,
                 GALConfig *config )
{
    GalDeviceData *vdev = dev;
    DFBResult        ret  = DFB_OK;
    char *config_file;

    D_ASSERT( dev      != NULL );
    D_ASSERT( config != NULL );

    /* Initialize the config. */
    memset( config, 0, sizeof(GALConfig) );

    config->draw_flag_num = MAX_DRAW_FLAG;
    config->blit_flag_num = MAX_BLIT_FLAG;

    strcpy( config->draw_flags[0].name, DRAW_XOR_NAME );
    config->draw_flags[0].value = DSDRAW_XOR;

    strcpy( config->draw_flags[1].name, DRAW_BLEND_NAME );
    config->draw_flags[1].value = DSDRAW_BLEND;

    strcpy( config->blit_flags[0].name, BLIT_XOR_NAME );
    config->blit_flags[0].value = DSBLIT_XOR;

    strcpy( config->blit_flags[1].name, BLIT_ALPHACHANNEL_NAME );
    config->blit_flags[1].value = DSBLIT_BLEND_ALPHACHANNEL;

    strcpy( config->blit_flags[2].name, BLIT_COLORALPHA_NAME );
    config->blit_flags[2].value = DSBLIT_BLEND_COLORALPHA;

    strcpy( config->blit_flags[3].name, BLIT_COLORIZE_NAME );
    config->blit_flags[3].value = DSBLIT_COLORIZE;

    strcpy( config->blit_flags[4].name, BLIT_SRC_COLORKEY_NAME );
    config->blit_flags[4].value = DSBLIT_SRC_COLORKEY;

    strcpy( config->blit_flags[5].name, BLIT_DST_COLORKEY_NAME );
    config->blit_flags[5].value = DSBLIT_DST_COLORKEY;

    strcpy( config->blit_flags[6].name, BLIT_ROTATION180_NAME );
    config->blit_flags[6].value = DSBLIT_ROTATE180;

    strcpy( config->primitive_settings[GAL_CONF_FILLRECTANGLE].name, FILLRECTANGLE_NAME );
    config->primitive_settings[GAL_CONF_FILLRECTANGLE].flag_num     = config->draw_flag_num;
    config->primitive_settings[GAL_CONF_FILLRECTANGLE].render_flags = config->draw_flags;

    strcpy( config->primitive_settings[GAL_CONF_DRAWRECTANGLE].name, DRAWRECTANGLE_NAME );
    config->primitive_settings[GAL_CONF_DRAWRECTANGLE].flag_num     = config->draw_flag_num;
    config->primitive_settings[GAL_CONF_DRAWRECTANGLE].render_flags = config->draw_flags;

    strcpy( config->primitive_settings[GAL_CONF_DRAWLINE].name, DRAWLINE_NAME );
    config->primitive_settings[GAL_CONF_DRAWLINE].flag_num          = config->draw_flag_num;
    config->primitive_settings[GAL_CONF_DRAWLINE].render_flags      = config->draw_flags;

    strcpy( config->primitive_settings[GAL_CONF_FILLTRIANGLE].name, FILLTRIANGLE_NAME );
    config->primitive_settings[GAL_CONF_FILLTRIANGLE].flag_num      = config->draw_flag_num;
    config->primitive_settings[GAL_CONF_FILLTRIANGLE].render_flags  = config->draw_flags;

    strcpy( config->primitive_settings[GAL_CONF_BLIT].name, BLIT_NAME );
    config->primitive_settings[GAL_CONF_BLIT].flag_num              = config->blit_flag_num;
    config->primitive_settings[GAL_CONF_BLIT].render_flags          = config->blit_flags;

    strcpy( config->primitive_settings[GAL_CONF_STRETCHBLIT].name, STRETCHBLIT_NAME );
    config->primitive_settings[GAL_CONF_STRETCHBLIT].flag_num       = config->blit_flag_num;
    config->primitive_settings[GAL_CONF_STRETCHBLIT].render_flags   = config->blit_flags;

    /* Read the settings from the file pointed by environment variable. */
    config_file = getenv( "GAL_CONFIG_FILE" );
    if (config_file && config_file[0] != 0) {
        strncpy( config->filename, config_file, PATH_MAX );
        config->filename[PATH_MAX] = '\0';

        D_DEBUG_AT( Gal_Config, "Config file: %s.\n", config->filename );

        ret = gal_config_read( config );

        D_INFO( "Using the settings from %s.\n", config->filename);

        return ret;
    }

    config->filename[0] = '\0';

    /* Use the default settings. */
    config->primitive_settings[GAL_CONF_FILLRECTANGLE].none      = true;
    config->primitive_settings[GAL_CONF_FILLRECTANGLE].flags     = GDCF_FILLRECTANGLE;

    if (!vdev->hw_2d_pe20) {
        config->primitive_settings[GAL_CONF_DRAWRECTANGLE].none  = false;
        config->primitive_settings[GAL_CONF_DRAWRECTANGLE].flags = DSDRAW_XOR;
    }
    else {
        config->primitive_settings[GAL_CONF_DRAWRECTANGLE].none  = true;
        config->primitive_settings[GAL_CONF_DRAWRECTANGLE].flags = GDCF_DRAWRECTANGLE;
    }

    if (!vdev->hw_2d_pe20) {
        config->primitive_settings[GAL_CONF_DRAWLINE].none       = false;
        config->primitive_settings[GAL_CONF_DRAWLINE].flags      = DSDRAW_XOR;
    }
    else {
        config->primitive_settings[GAL_CONF_DRAWLINE].none       = true;
        config->primitive_settings[GAL_CONF_DRAWLINE].flags      = GDCF_DRAWLINE;
    }

    if (!vdev->hw_2d_pe20) {
        config->primitive_settings[GAL_CONF_FILLTRIANGLE].none   = false;
    }
    else {
        config->primitive_settings[GAL_CONF_FILLTRIANGLE].none   = true;
    }
    config->primitive_settings[GAL_CONF_FILLTRIANGLE].flags      = GDCF_FILLTRIANGLE;

    config->primitive_settings[GAL_CONF_BLIT].none               = true;
    config->primitive_settings[GAL_CONF_BLIT].flags              = GDCF_BLIT;

    config->primitive_settings[GAL_CONF_STRETCHBLIT].none        = true;
    config->primitive_settings[GAL_CONF_STRETCHBLIT].flags       = GDCF_STRETCHBLIT;


    D_INFO( "Using the default feature settings.\n");

    return ret;
}

DFBResult
gal_config_read( GALConfig *config )
{
    DFBResult ret = DFB_OK;
    char line[400];
    FILE *f;

    D_ASSERT( config != NULL );
    D_ASSERT( config->filename != NULL );

    if ( config->filename[0] == '\0' ) {
        D_INFO( "GAL/Config: the config file name is invalid.\n" );
        return DFB_IO;
    }

    f = fopen( config->filename, "r" );
    if (!f) {
        D_ERROR( "GAL/Config: Unable to open config file `%s'!\n", config->filename );
        return DFB_IO;
    } else {
        D_INFO( "GAL/Config: Parsing config file '%s'.\n", config->filename );
    }

    while (fgets( line, 400, f )) {
        char *name = line;
        char *comment = strchr( line, '#' );
        char *value;

        if (comment) {
             *comment = 0;
        }

        value = strchr( line, '=' );

        if (value) {
             *value++ = 0;
             direct_trim( &value );
        }

        direct_trim( &name );

        if (!*name || *name == '#')
             continue;

        ret = gal_config_set( config, name, value );

        if (ret) {
             if (ret == DFB_UNSUPPORTED) {
                  D_ERROR( "GAL/Config: *********** In config file `%s': "
                           "Invalid option `%s'! ***********\n", config->filename, name );
                  continue;
             }
             break;
        }
     }

     fclose( f );

     return ret;

}

DFBResult
gal_config_set( GALConfig *config, const char *name, const char *value )
{
    int i, j;

    for (i = 0; i < MAX_PRIMITIVES; i++)
    {
        if (strcmp (name, config->primitive_settings[i].name ) == 0) {
            if (value && value[0] != '\0') {
                if (strstr( value, RENDER_NONE_NAME ))
                {
                    config->primitive_settings[i].none = true;
                }

                for (j = 0; j < config->primitive_settings[i].flag_num; j++)
                {
                    if (strstr( value, config->primitive_settings[i].render_flags[j].name ))
                    {
                        config->primitive_settings[i].flags |= config->primitive_settings[i].render_flags[j].value;
                    }
                }
            }

            break;
        }
    }

     return DFB_OK;
}

bool
gal_config_accelerated( GALConfig *config,
                        CardState *state,
                        DFBAccelerationMask accel )
{
    bool ret = false;
    unsigned int flags;
    int index;

    D_ASSERT( config != NULL );
    D_ASSERT( state != NULL );

    switch (accel)
    {
    case DFXL_FILLRECTANGLE:
        index = GAL_CONF_FILLRECTANGLE;
        break;
    case DFXL_DRAWRECTANGLE:
        index = GAL_CONF_DRAWRECTANGLE;
        break;
    case DFXL_DRAWLINE:
        index = GAL_CONF_DRAWLINE;
        break;
    case DFXL_FILLTRIANGLE:
        index = GAL_CONF_FILLTRIANGLE;
        break;
    case DFXL_BLIT:
        index = GAL_CONF_BLIT;
        break;
    case DFXL_STRETCHBLIT:
        index = GAL_CONF_STRETCHBLIT;
        break;

    default:
        return false;
    }

    if (DFB_BLITTING_FUNCTION(accel))
    {
        flags = (unsigned int)state->blittingflags;
    } else {
        flags = (unsigned int)state->drawingflags;
    }

    if (flags == 0)
    {
        if (config->primitive_settings[index].none)
        {
            ret = true;
        } else {
            ret = false;
        }
    } else {
        if (!(flags & ~config->primitive_settings[index].flags))
        {
            ret = true;
        } else {
            ret = false;
        }
    }

    return ret;
}
