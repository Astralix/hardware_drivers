/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Hantro Products Oy.                             --
--                                                                            --
--                   (C) COPYRIGHT 2006 HANTRO PRODUCTS OY                    --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Description : Encoder common configuration parameters
--
--------------------------------------------------------------------------------
--
--  Version control information, please leave untouched.
--
--  $RCSfile: enccfg.h,v $
--  $Revision: 1.4 $
--  $Date: 2008/10/07 12:35:51 $
--
------------------------------------------------------------------------------*/
#ifndef __ENCCFG_H__
#define __ENCCFG_H__

#include "basetype.h"


#define CONFIGURATION 60	/* 0 - 15 */

/* HW register interface bits DO NOT CHANGE! */
#define ASIC_LITTLE_ENDIAN                          1
#define ASIC_BIG_ENDIAN                             0

/* Here is defined the default values for the encoder build-time configuration.
 * You can override these settings by defining the values as compiler flags 
 * in the Makefile.
 */

/* The buffering mode for SW/HW interface, valid values are in range [1, 3]
 */
#ifndef ENC7280_BUFFER_AMOUNT
#define ENC7280_BUFFER_AMOUNT                       2
#endif

/* The buffer size for each macroblock in bytes.
 * The total buffer size is this value multiplied by macroblock amount.
 * Total of ENC7280_BUFFER_AMOUNT buffers of the same size are allocated.
 * Valid values are in range [1, 1536]
 */
#ifndef ENC7280_BUFFER_SIZE_MB
#define ENC7280_BUFFER_SIZE_MB                      (256)
#endif

/* The input image's 16-bit endianess swap: 0 or 1
 * This defines the 16-bit endianess of the ASIC input YUV/RGB
 */
#if 0
#ifndef ENC7280_INPUT_ENDIAN_16
#define ENC7280_INPUT_ENDIAN_16                     0
#endif
#endif

#if CONFIGURATION & 1
#define ENC7280_INPUT_ENDIAN_16                     1
#else
#define ENC7280_INPUT_ENDIAN_16                     0
#endif

/* The output data's 16-bit endianess swap: 0 or 1
 * This defines the 16-bit endianess of the ASIC output data.
 */
#if 0
#ifndef ENC7280_OUTPUT_ENDIAN_16
#define ENC7280_OUTPUT_ENDIAN_16                    0
#endif
#endif

#if CONFIGURATION & 2
#define ENC7280_OUTPUT_ENDIAN_16                    1
#else
#define ENC7280_OUTPUT_ENDIAN_16                    0
#endif

/* The input image's endianess mode: ASIC_LITTLE_ENDIAN or ASIC_BIG_ENDIAN
 * This defines the byte endianess of the ASIC input YUV
 */
#if 0
#ifndef ENC7280_INPUT_ENDIAN
#define ENC7280_INPUT_ENDIAN                        ASIC_BIG_ENDIAN /*ASIC_LITTLE_ENDIAN*/
#endif
#endif

#if CONFIGURATION & 4
#define ENC7280_INPUT_ENDIAN                        ASIC_LITTLE_ENDIAN
#else
#define ENC7280_INPUT_ENDIAN                        ASIC_BIG_ENDIAN
#endif

/* The output data's endianess mode: ASIC_LITTLE_ENDIAN or ASIC_BIG_ENDIAN
 * This defines the byte endianess of the ASIC output data. This MUST be the
 * same as the native system endianess, because the control software relies 
 * on system endianess when reading the data from the memory.
 */
#if 0
#ifndef ENC7280_OUTPUT_ENDIAN
#define ENC7280_OUTPUT_ENDIAN                       ASIC_BIG_ENDIAN /*ASIC_LITTLE_ENDIAN*/
#endif
#endif

#if CONFIGURATION & 8
#define ENC7280_OUTPUT_ENDIAN                       ASIC_LITTLE_ENDIAN
#else
#define ENC7280_OUTPUT_ENDIAN                       ASIC_BIG_ENDIAN
#endif


/* The width of endianess in input
 * 0 = 32-bit endianess
 * 1 = 64-bit endianess */
#if 0
#ifndef ENC7280_INPUT_ENDIAN_WIDTH
#define ENC7280_INPUT_ENDIAN_WIDTH                  0
#endif
#endif

#if CONFIGURATION & 16
#define ENC7280_INPUT_ENDIAN_WIDTH                  1
#else
#define ENC7280_INPUT_ENDIAN_WIDTH                  0
#endif

/* The width of endianess in input
 * 0 = 32-bit endianess
 * 1 = 64-bit endianess */
#if 0
#ifndef ENC7280_OUTPUT_ENDIAN_WIDTH
#define ENC7280_OUTPUT_ENDIAN_WIDTH                 0
#endif
#endif

#if CONFIGURATION & 32
#define ENC7280_OUTPUT_ENDIAN_WIDTH                 1
#else
#define ENC7280_OUTPUT_ENDIAN_WIDTH                 0
#endif


/* ASIC interrupt enable.
 * This enables/disables the ASIC to generate interrupts
 * If this is '1', the EWL must poll the registers to find out
 * when the HW is ready.
 */
#ifndef ENC7280_IRQ_DISABLE
#define ENC7280_IRQ_DISABLE                         0
#endif

/* ASIC bus interface configuration values                  */
/* DO NOT CHANGE IF NOT FAMILIAR WITH THE CONCEPTS INVOLVED */

/* Burst length. This sets the maximum length of a single ASIC burst in addresses.
 * Allowed values are:
 *          AHB {0, 4, 8, 16} ( 0 means incremental burst type INCR)
 *          OCP [1,63]
 *          AXI [1,16]
 */
#ifndef ENC7280_BURST_LENGTH
#define ENC7280_BURST_LENGTH                               16
#endif

/* INCR type burst mode                                                       */
/* 0 - enable INCR type bursts                                                */
/* 1 - disable INCR type and use SINGLE instead                               */
#ifndef ENC7280_BURST_INCR_TYPE_ENABLED
#define ENC7280_BURST_INCR_TYPE_ENABLED                    0
#endif

/* Data discard mode. When enabled read bursts of length 2 or 3 are converted */
/* to BURST4 and  useless data is discarded. Otherwise use INCR type for that */
/* kind  of read bursts */
/* 0 - disable data discard                                                   */
/* 1 - enable data discard                                                    */
#ifndef ENC7280_BURST_DATA_DISCARD_ENABLED
#define ENC7280_BURST_DATA_DISCARD_ENABLED                 0
#endif

/* AXI bus read and write ID values used by HW. 0 - 255 */
#ifndef ENC7280_AXI_READ_ID
#define ENC7280_AXI_READ_ID                                0
#endif

#ifndef ENC7280_AXI_WRITE_ID
#define ENC7280_AXI_WRITE_ID                               0
#endif

/* End of "ASIC bus interface configuration values"                           */

/* ASIC internal clock gating control. 0 - disabled, 1 - enabled              */
#ifndef ENC7280_ASIC_CLOCK_GATING_ENABLED
#define ENC7280_ASIC_CLOCK_GATING_ENABLED                  0
#endif

/* IRQ releated parameters below in use just for video packaged MPEG-4 streams!
 *
 * The ASIC will generate interrupts after a fixed amount of macroblocks to 
 * notify the SW how many macroblocks it has processed.
 * The IRQ interval defines the amount of macroblocks after which the HW
 * will generate an interrupt. 
 * Value 0 means no interrupts of this kind.
 * ENC7280_IRQ_INTERVAL_FRAME_START is used when first starting a frame and
 * ENC7280_IRQ_INTERVAL is used after the first interrupt.
 */

#ifndef ENC7280_IRQ_INTERVAL_FRAME_START
#define ENC7280_IRQ_INTERVAL_FRAME_START            0
#endif

#ifndef ENC7280_IRQ_INTERVAL
#define ENC7280_IRQ_INTERVAL                        0
#endif

#endif
