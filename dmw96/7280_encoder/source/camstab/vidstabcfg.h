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
--  Description : Camera stabilization standalone configuration
--
--------------------------------------------------------------------------------
--
--  Version control information, please leave untouched.
--
--  $RCSfile: vidstabcfg.h,v $
--  $Revision: 1.3 $
--  $Date: 2008/05/23 11:50:53 $
--
------------------------------------------------------------------------------*/
#ifndef __VIDSTBCFG_H__
#define __VIDSTBCFG_H__

#include "basetype.h"

/* HW register interface defined values, DO NOT CHANGE! */
#define ASIC_LITTLE_ENDIAN                          1
#define ASIC_BIG_ENDIAN                             0

/* Here is defined the default values for the encoder build-time configuration.
 * You can override these settings by defining the values as compiler flags 
 * in the Makefile.
 */

/* The input image's endianess mode: ASIC_LITTLE_ENDIAN or ASIC_BIG_ENDIAN
 * This defines the endianess of the ASIC input YUV
 */
#ifndef VS7280_INPUT_ENDIAN
#define VS7280_INPUT_ENDIAN                        ASIC_LITTLE_ENDIAN
#endif

/* The width of endianess in input
 * 0 = 32-bit endianess
 * 1 = 64-bit endianess */
#ifndef VS7280_INPUT_ENDIAN_WIDTH
#define VS7280_INPUT_ENDIAN_WIDTH                  0
#endif

/* ASIC interrupt enable.
 * This enables/disables the ASIC to generate interrupts
 * If this is '1', the EWL must poll the registers to find out
 * when the HW is ready.
 */
#ifndef VS7280_IRQ_DISABLE
#define VS7280_IRQ_DISABLE                         0
#endif

/* ASIC bus interface configuration values                  */
/* DO NOT CHANGE IF NOT FAMILIAR WITH THE CONCEPTS INVOLVED */

/* Burst length. This sets the maximum length of a single ASIC burst in addresses.
 * Allowed values are:
 *          AHB {0, 4, 8, 16} ( 0 means incremental burst type INCR)
 *          OCP [1,63]
 *          AXI [1,16]
 */
#ifndef VS7280_BURST_LENGTH
#define VS7280_BURST_LENGTH                               16
#endif

/* INCR type burst mode                          */
/* 0 allowe INCR type bursts                     */
/* 1 disable INCR type and use SINGLE instead    */
#ifndef VS7280_BURST_INCR_TYPE_ENABLED
#define VS7280_BURST_INCR_TYPE_ENABLED                    0
#endif

/* Data discard mode. When enabled read bursts of length 2 or 3 are converted to */
/* BURST4 and  useless data is discarded. Otherwise use INCR type for that kind  */
/* of read bursts */
/* 0 disable data discard                   */
/* 1 enable data discard                    */
#ifndef VS7280_BURST_DATA_DISCARD_ENABLED
#define VS7280_BURST_DATA_DISCARD_ENABLED                 0
#endif

/* AXI bus read and write ID values used by HW. 0 - 255 */
#ifndef VS7280_AXI_READ_ID
#define VS7280_AXI_READ_ID                                0
#endif

#ifndef VS7280_AXI_WRITE_ID
#define VS7280_AXI_WRITE_ID                              0
#endif

/* End of "ASIC bus interface configuration values" */

/* ASIC internal clock gating control. 0 - disabled, 1 - enabled */
#ifndef VS7280_ASIC_CLOCK_GATING_ENABLED
#define VS7280_ASIC_CLOCK_GATING_ENABLED                  0
#endif

#endif
