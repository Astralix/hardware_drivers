/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rv_decode.c,v 1.2 2009-05-04 11:29:14 juma Exp $
 * 
 * REALNETWORKS CONFIDENTIAL--NOT FOR DISTRIBUTION IN SOURCE CODE FORM
 * Portions Copyright (c) 1995-2005 RealNetworks, Inc.
 * All Rights Reserved.
 * 
 * The contents of this file, and the files included with this file,
 * are subject to the current version of the Real Format Source Code
 * Porting and Optimization License, available at
 * https://helixcommunity.org/2005/license/realformatsource (unless
 * RealNetworks otherwise expressly agrees in writing that you are
 * subject to a different license).  You may also obtain the license
 * terms directly from RealNetworks.  You may not use this file except
 * in compliance with the Real Format Source Code Porting and
 * Optimization License. There are no redistribution rights for the
 * source code of this file. Please see the Real Format Source Code
 * Porting and Optimization License for the rights, obligations and
 * limitations governing use of the contents of the file.
 * 
 * RealNetworks is the developer of the Original Code and owns the
 * copyrights in the portions it created.
 * 
 * This file, and the files included with this file, is distributed and
 * made available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL
 * SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT
 * OR NON-INFRINGEMENT.
 * 
 * Technology Compatibility Kit Test Suite(s) Location:
 * https://rarvcode-tck.helixcommunity.org
 * 
 * Contributor(s):
 * 
 * ***** END LICENSE BLOCK ***** */

/* Simple, unified decoder frontend for RealVideo */

#include <string.h>
#include "rv_decode.h"
#include "rv20backend.h"
#include "rv30backend.h"
#include "codec_defines.h"
#include "helix_utils.h"
#include "rm_memory_default.h"

static void rv_decode_recover_packets(rv_decode* pFrontEnd, UINT8* pData,
                                      rv_backend_in_params* pInputParams);


/* rv_decode_create()
 * Creates RV decoder frontend struct, copies memory utilities.
 * Returns struct pointer on success, NULL on failure. */
rv_decode*
rv_decode_create(void*              pUserError,
                 rm_error_func_ptr  fpError)
{
    return rv_decode_create2(pUserError,
                             fpError,
                             HXNULL,
                             rm_memory_default_malloc,
                             rm_memory_default_free);
}

rv_decode*
rv_decode_create2(void*               pUserError,
                  rm_error_func_ptr   fpError,
                  void*               pUserMem,
                  rm_malloc_func_ptr  fpMalloc,
                  rm_free_func_ptr    fpFree)
{
    rv_decode* pRet = HXNULL;

    if (fpMalloc && fpFree)
    {
        pRet = (rv_decode*) fpMalloc(pUserMem, sizeof(rv_decode));

        if (pRet)
        {
            /* Zero out the struct */
            memset((void*) pRet, 0, sizeof(rv_decode));
            /* Assign the error function */
            pRet->fpError = fpError;
            pRet->pUserError = pUserError;
            /* Assign the memory functions */
            pRet->fpMalloc = fpMalloc;
            pRet->fpFree = fpFree;
            pRet->pUserMem = pUserMem;

            /* Allocate the backend interface struct */
            pRet->pDecode = (rv_backend*) fpMalloc(pUserMem, sizeof(rv_backend));

            if (pRet->pDecode == HXNULL)
            {
                fpFree(pUserMem, pRet);
                pRet = HXNULL;
            }
        }
    }

    return pRet;
}

/* rv_decode_destroy()
 * Deletes decoder and backend instance, followed by frontend. */
void
rv_decode_destroy(rv_decode* pFrontEnd)
{
    rm_free_func_ptr fpFree;
    void* pUserMem;

    if (pFrontEnd && pFrontEnd->fpFree)
    {
        /* Save a pointer to fpFree and pUserMem */
        fpFree   = pFrontEnd->fpFree;
        pUserMem = pFrontEnd->pUserMem;

        if (pFrontEnd->pDecode)
        {
            if (pFrontEnd->pDecode->fpFree && pFrontEnd->pDecodeState)
            {
                /* Free the decoder instance */
                pFrontEnd->pDecode->fpFree(pFrontEnd->pDecodeState);
                pFrontEnd->pDecodeState = HXNULL;
            }
            /* Free the backend interface */
            fpFree(pUserMem, pFrontEnd->pDecode);
        }
        /* Free the rv_decode struct memory */
        fpFree(pUserMem, pFrontEnd);
    }
}


/* rv_decode_init()
 * Reads bitstream header, selects and initializes decoder backend.
 * Returns zero on success, negative result indicates failure. */
HX_RESULT
rv_decode_init(rv_decode* pFrontEnd, rv_format_info* pHeader)
{
    UINT8* baseptr;
    UINT8 ucTmp;
    UINT32 i;
    int w = 0xFF; /* For run-time endianness detection */
	UINT8 size[9] = {0,1,1,2,2,3,3,3,3};

    if (pHeader->ulMOFTag != HX_MEDIA_VIDEO)
    {
        return HXR_FAIL;
    }

    pFrontEnd->ulSPOExtra = 0;
    pFrontEnd->ulEncodeSize = 0;
    pFrontEnd->ulNumResampledImageSizes = 0;

	/* set stream specific params */
    switch (pHeader->ulSubMOFTag)
    {
    case HX_RV30VIDEO_ID:
		pFrontEnd->bIsRV8 = TRUE;
		pFrontEnd->ulECCMask = 0x20;
		break;
    case HX_RV40VIDEO_ID:
		pFrontEnd->bIsRV8 = FALSE;
		pFrontEnd->ulECCMask = 0x80;
		break;
    case HX_RV89COMBO_ID:
		pFrontEnd->bIsRV8 = FALSE;
		pFrontEnd->ulECCMask = 0;
        break;
    default:
        /* unknown format */
        printf("Unsupported codec ID: %8.8x\n", 
            pHeader->ulSubMOFTag );
        return HXR_NOT_SUPPORTED;
        break;
    }

    if (pHeader->ulOpaqueDataSize > 0)
    {
        /* Decode opaque data */
        baseptr = pHeader->pOpaqueData;

        /* ulSPOExtra contains CODEC options */
        memmove(&pFrontEnd->ulSPOExtra, baseptr, sizeof(UINT32));
        pFrontEnd->ulSPOExtra = IS_BIG_ENDIAN(w) ? pFrontEnd->ulSPOExtra :
                                BYTE_SWAP_UINT32(pFrontEnd->ulSPOExtra);
        baseptr += sizeof(UINT32);

        /* ulStreamVersion */
        memmove(&pFrontEnd->ulStreamVersion, baseptr, sizeof(UINT32));
        pFrontEnd->ulStreamVersion = IS_BIG_ENDIAN(w) ? pFrontEnd->ulStreamVersion :
                                     BYTE_SWAP_UINT32(pFrontEnd->ulStreamVersion);
        baseptr += sizeof(UINT32);

        pFrontEnd->ulMajorBitstreamVersion = HX_GET_MAJOR_VERSION(pFrontEnd->ulStreamVersion);
        pFrontEnd->ulMinorBitstreamVersion = HX_GET_MINOR_VERSION(pFrontEnd->ulStreamVersion);

        /* Decode extra opaque data */
        if (!(pFrontEnd->ulMinorBitstreamVersion & RAW_BITSTREAM_MINOR_VERSION))
        {
            if (pFrontEnd->ulMajorBitstreamVersion == RV20_MAJOR_BITSTREAM_VERSION ||
                pFrontEnd->ulMajorBitstreamVersion == RV30_MAJOR_BITSTREAM_VERSION)
            {
                /* RPR (Reference Picture Resampling) sizes */
                pFrontEnd->ulNumResampledImageSizes = (pFrontEnd->ulSPOExtra & RV40_SPO_BITS_NUMRESAMPLE_IMAGES)
                                                       >> RV40_SPO_BITS_NUMRESAMPLE_IMAGES_SHIFT;
                /* loop over dimensions of possible resampled images sizes              */
                /* This byzantine method of extracting bytes is required to solve       */
                /* misaligned write problems in UNIX                                    */
                /* note 2 byte offset in pDimensions buffer for resampled sizes         */
                /* these 2 bytes are later filled with the native pels and lines sizes. */
                for(i = 0; i < pFrontEnd->ulNumResampledImageSizes; i++)
                {
                    memmove(&ucTmp, baseptr, sizeof(UINT8));
                    *(pFrontEnd->pDimensions + 2*i + 2) = (UINT32)(ucTmp<<2); /* width */
                    baseptr+=sizeof(UINT8);
                    memmove(&ucTmp, baseptr, sizeof(UINT8));
                    *(pFrontEnd->pDimensions + 2*i + 3) = (UINT32)(ucTmp<<2); /* height */
                    baseptr+=sizeof(UINT8);
                }
            }
            else if (pFrontEnd->ulMajorBitstreamVersion == RV40_MAJOR_BITSTREAM_VERSION)
            {
                /* RV9 largest encoded dimensions */
                if (pHeader->ulOpaqueDataSize >= 12)
                {
                    memmove((UINT8*)pFrontEnd->ulEncodeSize, baseptr, sizeof(UINT32));
                    pFrontEnd->ulEncodeSize = IS_BIG_ENDIAN(w) ? pFrontEnd->ulEncodeSize :
                                              BYTE_SWAP_UINT32(pFrontEnd->ulEncodeSize);
                }
            }
        }
    }

    /* Set largest encoded dimensions */
    pFrontEnd->ulLargestPels = ((pFrontEnd->ulEncodeSize >> 14) & 0x3FFFC);
    if (pFrontEnd->ulLargestPels == 0)
        pFrontEnd->ulLargestPels = pHeader->usWidth;

    pFrontEnd->ulLargestLines = ((pFrontEnd->ulEncodeSize << 2) & 0x3FFFC);
    if (pFrontEnd->ulLargestLines == 0)
        pFrontEnd->ulLargestLines = pHeader->usHeight;

    /* Prepare decoder init parameters */
    pFrontEnd->pInitParams.usPels = (UINT16)pFrontEnd->ulLargestPels;
    pFrontEnd->pInitParams.usLines = (UINT16)pFrontEnd->ulLargestLines;
    pFrontEnd->pInitParams.usPadWidth = pHeader->usPadWidth;
    pFrontEnd->pInitParams.usPadHeight = pHeader->usPadHeight;
    pFrontEnd->pInitParams.bPacketization = TRUE;
    pFrontEnd->pInitParams.ulInvariants = pFrontEnd->ulSPOExtra;
    pFrontEnd->pInitParams.ulStreamVersion = pFrontEnd->ulStreamVersion;

    pFrontEnd->ulOutSize = WIDTHBYTES(pFrontEnd->ulLargestPels * pFrontEnd->ulLargestLines * 12);

	/* Send version-specific init messages, if required */
    if ((pFrontEnd->ulMajorBitstreamVersion == RV20_MAJOR_BITSTREAM_VERSION ||
         pFrontEnd->ulMajorBitstreamVersion == RV30_MAJOR_BITSTREAM_VERSION) &&
        (pFrontEnd->ulNumResampledImageSizes != 0))
    {
        pFrontEnd->ulNumImageSizes = pFrontEnd->ulNumResampledImageSizes + 1; /* includes native size */
        pFrontEnd->pDimensions[0] = pFrontEnd->pInitParams.usPels;    /* native width */
        pFrontEnd->pDimensions[1] = pFrontEnd->pInitParams.usLines;   /* native height */
    }

	pFrontEnd->ulPctszSize = size[pFrontEnd->ulNumImageSizes];

    return HXR_OK;
}

/* rv_decode_stream_input()
 * Reads frame header and fills decoder input parameters struct. If there
 * is packet loss and ECC packets exist, error correction is attempted.
 * Returns zero on success, negative result indicates failure. */
HX_RESULT
rv_decode_stream_input(rv_decode* pFrontEnd, rv_frame* pFrame)
{
    UINT32 ulSegOff;
    UINT32 i;

	// the ->pInputFrame only be modified here
    pFrontEnd->pInputFrame = pFrame;

	// the ->ulMinorBitstreamVersion only be valued in function decode_init
    if (!(pFrontEnd->ulMinorBitstreamVersion & RAW_BITSTREAM_MINOR_VERSION))
    {
        /* Identify frame type and determine quantization factor */
        pFrontEnd->bInputFrameIsReference = FALSE;

        for (i = 0; i < pFrame->ulNumSegments; i++)
        {
            if (pFrame->pSegment[i].bIsValid == TRUE)
            {
                ulSegOff = pFrame->pSegment[i].ulOffset;
                if (ulSegOff + 1 < pFrame->ulDataLen)
                {
                    if (pFrontEnd->ulMajorBitstreamVersion == RV20_MAJOR_BITSTREAM_VERSION)
                    {
                        pFrontEnd->ulInputFrameQuant = (pFrame->pData[ulSegOff] & 0x1F);
                        pFrontEnd->bInputFrameIsReference = ((pFrame->pData[ulSegOff] & 0xC0) == 0xC0)?(FALSE):(TRUE); 
                    }
                    else if (pFrontEnd->ulMajorBitstreamVersion == RV30_MAJOR_BITSTREAM_VERSION)
                    {
                        pFrontEnd->ulInputFrameQuant = ((pFrame->pData[ulSegOff + 1] & 0xe0)>>5)+((pFrame->pData[ulSegOff] & 0x03)<<3);
                        pFrontEnd->bInputFrameIsReference = ((pFrame->pData[ulSegOff] & 0x18) == 0x18)?(FALSE):(TRUE); 
                    }
                    else if (pFrontEnd->ulMajorBitstreamVersion == RV40_MAJOR_BITSTREAM_VERSION)
                    {
                        pFrontEnd->ulInputFrameQuant = (pFrame->pData[ulSegOff] & 0x1F);
                        pFrontEnd->bInputFrameIsReference = ((pFrame->pData[ulSegOff] & 0x60) == 0x60)?(FALSE):(TRUE); 
                    }
                }
                break;
            }
        }
    }

    /* Set decoder input parameters */
    pFrontEnd->pInputParams.dataLength = pFrame->ulDataLen;
    pFrontEnd->pInputParams.bInterpolateImage = FALSE;
    pFrontEnd->pInputParams.numDataSegments = pFrame->ulNumSegments - 1;
    pFrontEnd->pInputParams.pDataSegments = pFrame->pSegment;
    pFrontEnd->pInputParams.timestamp = pFrame->ulTimestamp;
    pFrontEnd->pInputParams.flags = 0;

    /* Use ECC to recover lost packets */
    rv_decode_recover_packets(pFrontEnd, pFrame->pData, &pFrontEnd->pInputParams);

    return HXR_OK;
}

/* rv_decode_stream_flush()
 * Flushes the latency frame from the decoder backend after the last frame
 * is delivered and decoded before a pause or the end-of-file.
 * Returns zero on success, negative result indicates failure. */
HX_RESULT
rv_decode_stream_flush(rv_decode* pFrontEnd, UINT8* pOutput)
{
    HX_RESULT result=0;
//	int	nSlices	=	pFrontEnd->pInputFrame->ulNumSegments;
//	int	maxBsSize	=	pFrontEnd->pInputFrame->ulDataLen;
	
	pOutput = pOutput;

    /* Set decoder input parameters for flush */
    pFrontEnd->pInputParams.dataLength = 0;
    pFrontEnd->pInputParams.bInterpolateImage = FALSE;
    pFrontEnd->pInputParams.numDataSegments = 0;
    pFrontEnd->pInputParams.pDataSegments = HXNULL;
    pFrontEnd->pInputParams.timestamp = 0;
    pFrontEnd->pInputParams.flags = RV_DECODE_LAST_FRAME;

//	result	=	Decode_Frame(pFrontEnd->pInputFrame->pData,nSlices,maxBsSize,pFrontEnd->pInputFrame->pSegment,framenum);
//	framenum++;
    //result = pFrontEnd->pDecode->fpDecode(HXNULL,
    //                                      pOutput,
    //                                      &pFrontEnd->pInputParams,
    //                                      &pFrontEnd->pOutputParams,
    //                                      pFrontEnd->pDecodeState);

    return result;
}

/* rv_decode_custom_message()
 * Sends a custom message to the decoder backend.
 * Returns zero on success, negative result indicates failure. */
HX_RESULT
rv_decode_custom_message(rv_decode* pFrontEnd, RV_Custom_Message_ID* pMsg_id)
{
    return pFrontEnd->pDecode->fpCustomMessage(pMsg_id, pFrontEnd->pDecodeState);
}

/**************** Accessor Functions *******************/
/* rv_decode_max_output_size()
 * Returns maximum size of YUV 4:2:0 output buffer in bytes. */
UINT32
rv_decode_max_output_size(rv_decode* pFrontEnd)
{
    return pFrontEnd->ulOutSize;
}

/* rv_decode_get_output_size()
 * Returns size of most recent YUV 4:2:0 output buffer in bytes. */
UINT32
rv_decode_get_output_size(rv_decode* pFrontEnd)
{
    return (pFrontEnd->pOutputParams.width * pFrontEnd->pOutputParams.height * 12) >> 3;
}

/* rv_decode_get_output_dimensions()
 * Returns width and height of most recent YUV output buffer. */
HX_RESULT
rv_decode_get_output_dimensions(rv_decode* pFrontEnd, UINT32* pWidth, UINT32* pHeight)
{
    *pWidth = pFrontEnd->pOutputParams.width;
    *pHeight = pFrontEnd->pOutputParams.height;

    return HXR_OK;
}

/* rv_decode_frame_valid()
 * Checks decoder output parameters to see there is a valid output frame.
 * Returns non-zero value if a valid output frame exists, else zero. */
UINT32
rv_decode_frame_valid(rv_decode* pFrontEnd)
{
    return !(pFrontEnd->pOutputParams.notes & RV_DECODE_DONT_DRAW);
}

/* rv_decode_more_frames()
 * Checks decoder output parameters to see if more output frames can be
 * produced without additional input frames.
 * Returns non-zero value if more frames can be
 * produced without additional input, else zero. */
UINT32
rv_decode_more_frames(rv_decode* pFrontEnd)
{
    return (pFrontEnd->pOutputParams.notes & RV_DECODE_MORE_FRAMES);
}

/**************** Private Functions *******************/
/* rv_decode_recover_packets()
 * Looks for lost packets and ECC packets. Reconstructs missing packets
 * with ECC data if possible. */
static void
rv_decode_recover_packets(rv_decode* pFrontEnd, UINT8* pData, rv_backend_in_params* pInputParams)
{
    UINT32 packet, byte;
    UINT32 lostPacket = 0;
    INT32 ECCPacket;

    UINT32 ulNumECCPackets = 0;
    UINT32 ulNumLostPackets = 0;
    UINT32 ulNumSegments = 0;

    UINT32 ulECCPacketSize;
    UINT32 ulLostPacketSize;
    UINT32 ulGoodPacketSize;

    UINT8 *pECCPacket;
    UINT8 *pLostPacket;
    UINT8 *pGoodPacket;

    /* Count the number of lost packets and ECC packets */
    for (packet = 0; packet < pInputParams->numDataSegments + 1; packet++)
    {
        if (pInputParams->pDataSegments[packet].bIsValid == FALSE) 
        {
            ulNumLostPackets++;
        }
        else if (pData[pInputParams->pDataSegments[packet].ulOffset] & pFrontEnd->ulECCMask)
        {
            pECCPacket = pData + pInputParams->pDataSegments[packet].ulOffset;
            if (packet == pInputParams->numDataSegments)
            {
                ulECCPacketSize = pInputParams->dataLength - pInputParams->pDataSegments[packet].ulOffset;
            }
            else
            {
                ulECCPacketSize = pInputParams->pDataSegments[packet+1].ulOffset - pInputParams->pDataSegments[packet].ulOffset;
            }
            ulNumECCPackets = (pECCPacket[ulECCPacketSize-1] & 0x0F);
        }
    }

    /* If there are any lost packets, we'll look for ECC packets and see what we can correct with them */
    if ((ulNumLostPackets > 0) && (ulNumECCPackets > 0))
    {
        ulNumSegments = pInputParams->numDataSegments + 1 - ulNumECCPackets;

        /* If we've lost B-frame packets, drop the whole frame */
        if (!(pFrontEnd->bInputFrameIsReference))
        {
            for (packet = 0; packet < pInputParams->numDataSegments + 1; packet++)
            {
                pInputParams->pDataSegments[packet].bIsValid = FALSE;
                pInputParams->pDataSegments[packet].ulOffset = 0;
            }
            memset(pData,0,pInputParams->dataLength);
            pInputParams->dataLength = 0;
            return;
        }

		/* forward ECC */
        for (ECCPacket = pInputParams->numDataSegments; ECCPacket >= 0; ECCPacket--)
        {
            if ((pInputParams->pDataSegments[ECCPacket].bIsValid == TRUE) &&
                (pData[pInputParams->pDataSegments[ECCPacket].ulOffset] & pFrontEnd->ulECCMask))
            {                
                UINT32 startPacket = 0;
                UINT32 stopPacket = 0;
                UINT32 numPacketsCovered = 0;
                UINT32 eccNum = 0;

                pECCPacket = pData + pInputParams->pDataSegments[ECCPacket].ulOffset;

                /* Read the last byte of the ECC for some info */
                if (ECCPacket == (INT32)pInputParams->numDataSegments)
                {
                    ulECCPacketSize = pInputParams->dataLength - pInputParams->pDataSegments[ECCPacket].ulOffset;
                }
                else
                {
                    ulECCPacketSize = pInputParams->pDataSegments[ECCPacket+1].ulOffset - pInputParams->pDataSegments[ECCPacket].ulOffset;
                }
                eccNum = ((pECCPacket[ulECCPacketSize-1] & 0xF0) >> 4) & 0x0F;
                
                /* Find packets covered */
                startPacket = (eccNum)*ulNumSegments/ulNumECCPackets;
                stopPacket = (eccNum+1)*ulNumSegments/ulNumECCPackets;

                if (eccNum == ulNumECCPackets - 1)
                {
                    stopPacket = ulNumSegments;
                }

                numPacketsCovered = stopPacket - startPacket;	// numPacketsCovered != ulNumECCPackets

                if (ulNumECCPackets > 1)
                {
                    /* Read the offsets for the packets covered and correct them */
                    byte = ulECCPacketSize-2;
                    packet = numPacketsCovered;
                    while (1)
                    {
                        UINT32 offset;
                        if (pECCPacket[byte] & 0x01)
                        {
                            offset =
                                ((pECCPacket[byte]) & 0x000000FF) |
                                ((pECCPacket[byte-1] << 8) & 0x0000FF00) |
                                ((pECCPacket[byte-2] << 16) & 0x00FF0000) |
                                ((pECCPacket[byte-3] << 24) & 0xFF000000);
                            pInputParams->pDataSegments[startPacket+packet].ulOffset = ((offset >> 1) & 0x7FFFFFFF); 
                            byte -= 4;
                        }
                        else
                        {
                            offset =
                                ((pECCPacket[byte]) & 0x000000FF) | 
                                ((pECCPacket[byte-1] << 8) & 0x0000FF00);
                            pInputParams->pDataSegments[startPacket+packet].ulOffset = (offset >> 1);
                            byte -= 2;
                        }
                        if (packet == 0)
                            break;
                        packet--;
                    }
                }

                /* Lets count the number of lost packets this ECC covers */
                ulNumLostPackets = 0;
                for (packet = startPacket; packet < stopPacket; packet++)
                {
                    if (pInputParams->pDataSegments[packet].bIsValid == FALSE) 
                    {
                        lostPacket = packet;
                        ulNumLostPackets++;
                    }
                }

                /* If there is a single lost packet that this ECC can correct, then correct it. */
                if (ulNumLostPackets == 1)
                {
                    pLostPacket = pData + pInputParams->pDataSegments[lostPacket].ulOffset;
                    if (lostPacket == pInputParams->numDataSegments)
                    {
                        ulLostPacketSize = pInputParams->dataLength - pInputParams->pDataSegments[lostPacket].ulOffset;
                    }
                    else
                    {
                        ulLostPacketSize = pInputParams->pDataSegments[lostPacket+1].ulOffset - pInputParams->pDataSegments[lostPacket].ulOffset;
                    }

                    /* The lost packet size can be zero if the following packet is also lost. */
                    /* In that case, we'll say that the lost packet size is the same as the ECC packet size. */
                    if (ulLostPacketSize == 0)
                    {
                        ulLostPacketSize = ulECCPacketSize;
                    }

                    /* Clear the lost packet */
                    memset(pLostPacket,0,ulLostPacketSize);

                    /* Correct the lost Packet */
                    for (packet = startPacket; packet < stopPacket; packet++)
                    {
                        if (pInputParams->pDataSegments[packet].bIsValid == TRUE)
                        {
                            if (packet == pInputParams->numDataSegments)
                            {
                                ulGoodPacketSize = pInputParams->dataLength - pInputParams->pDataSegments[packet].ulOffset;
                            }
                            else
                            {
                                ulGoodPacketSize = pInputParams->pDataSegments[packet+1].ulOffset - pInputParams->pDataSegments[packet].ulOffset;
                            }
                            if (ulLostPacketSize < ulGoodPacketSize)
                                ulGoodPacketSize = ulLostPacketSize;

                            pGoodPacket = pData + pInputParams->pDataSegments[packet].ulOffset;
                            for (byte = 0; byte < ulGoodPacketSize; byte++)
                            {
                                pLostPacket[byte] ^= pGoodPacket[byte];
                            }
                        }
                    }

                    pGoodPacket = pData + pInputParams->pDataSegments[ECCPacket].ulOffset;
                    for (byte = 0; byte < ulLostPacketSize; byte++)
                    {
                        pLostPacket[byte] ^= pGoodPacket[byte];
                    }

                    /* Assign the first byte of lost packet, and make it valid */
                    pLostPacket[0] = (UINT8)(pData[pInputParams->pDataSegments[ECCPacket].ulOffset] & (~pFrontEnd->ulECCMask));
                    pInputParams->pDataSegments[lostPacket].bIsValid = TRUE;
                }
            }
        }
    }

    /* Invalidate ECC packets at the end */
    while (((pInputParams->pDataSegments[pInputParams->numDataSegments].bIsValid == TRUE) &&
           (pData[pInputParams->pDataSegments[pInputParams->numDataSegments].ulOffset] & pFrontEnd->ulECCMask)) ||
           (pInputParams->pDataSegments[pInputParams->numDataSegments].bIsValid == FALSE))
    {
        pInputParams->pDataSegments[pInputParams->numDataSegments].bIsValid = FALSE;
        pInputParams->dataLength = pInputParams->pDataSegments[pInputParams->numDataSegments].ulOffset;
        if (pInputParams->numDataSegments > 0)
        {
            pInputParams->numDataSegments--;
        }
        else
        {
            break;
        }
    }
}
