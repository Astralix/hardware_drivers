/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rv_depack.c,v 1.1 2009-03-11 14:11:59 kima Exp $
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

// Wangxiaocheng 2007.06.21
//#include <memory.h>
#include "string.h"
#include "helix_types.h"
#include "helix_result.h"
#include "rv_depack.h"
#include "rv_depack_internal.h"
#include "rm_memory_default.h"
#include "rm_error_default.h"
#include "memory_utils.h"

rv_depack* rv_depack_create(void*                   pAvail,
                            rv_frame_avail_func_ptr fpAvail,
                            void*                   pUserError,
                            rm_error_func_ptr       fpError)
{
    return rv_depack_create2(pAvail,
                             fpAvail,
                             pUserError,
                             fpError,
                             HXNULL,
                             rm_memory_default_malloc,
                             rm_memory_default_free);
}

rv_depack* rv_depack_create2(void*                   pAvail,
                             rv_frame_avail_func_ptr fpAvail,
                             void*                   pUserError,
                             rm_error_func_ptr       fpError,
                             void*                   pUserMem,
                             rm_malloc_func_ptr      fpMalloc,
                             rm_free_func_ptr        fpFree)
{
    rv_depack* pRet = HXNULL;

    if (fpAvail && fpMalloc && fpFree)
    {
        /* Allocate space for the rv_depack_internal struct
         * by using the passed-in malloc function
         */
        rv_depack_internal* pInt =
            (rv_depack_internal*) fpMalloc(pUserMem, sizeof(rv_depack_internal));
        if (pInt)
        {
            /* Zero out the struct */
            memset((void*) pInt, 0, sizeof(rv_depack_internal));
            /* Assign the frame callback members */
            pInt->pAvail  = pAvail;
            pInt->fpAvail = fpAvail;
            /*
             * Assign the error members. If the caller did not
             * provide an error callback, then use the default
             * rm_error_default().
             */
            if (fpError)
            {
                pInt->fpError    = fpError;
                pInt->pUserError = pUserError;
            }
            else
            {
                pInt->fpError    = rm_error_default;
                pInt->pUserError = HXNULL;
            }
            /* Assign the memory functions */
            pInt->fpMalloc = fpMalloc;
            pInt->fpFree   = fpFree;
            pInt->pUserMem = pUserMem;
            /* Assign the return value */
            pRet = (rv_depack*) pInt;
        }
    }

    return pRet;
}

rv_depack* rv_depack_create2_ex(void*                   pAvail,
								rv_frame_avail_func_ptr fpAvail,
								void*                   pUserError,
								rm_error_func_ptr       fpError,
								void*                   pUserMem,
								rm_malloc_func_ptr      fpMalloc,
								rm_free_func_ptr        fpFree,
								rm_malloc_func_ptr      fpMalloc_ncnb,
								rm_free_func_ptr        fpFree_ncnb)
{
	rv_depack* pRet = HXNULL;

	if (fpMalloc && fpFree)
	{
		/* Allocate space for the rv_depack_internal struct
		* by using the passed-in malloc function
		*/
		rv_depack_internal* pInt =
			(rv_depack_internal*) fpMalloc(pUserMem, sizeof(rv_depack_internal));
		if (pInt)
		{
			/* Zero out the struct */
			memset((void*) pInt, 0, sizeof(rv_depack_internal));
			/* Assign the frame callback members */
			pInt->pAvail  = pAvail;
			pInt->fpAvail = fpAvail;
			/*
			* Assign the error members. If the caller did not
			* provide an error callback, then use the default
			* rm_error_default().
			*/
			if (fpError)
			{
				pInt->fpError    = fpError;
				pInt->pUserError = pUserError;
			}
			else
			{
				pInt->fpError    = rm_error_default;
				pInt->pUserError = HXNULL;
			}
			/* Assign the memory functions */
			pInt->fpMalloc = fpMalloc;
			pInt->fpFree   = fpFree;
			pInt->pUserMem = pUserMem;
			if (fpMalloc_ncnb && fpFree_ncnb)
			{
				pInt->fpMalloc_ncnb = fpMalloc_ncnb;
				pInt->fpFree_ncnb   = fpFree_ncnb;
			}
			else
			{
				pInt->fpMalloc_ncnb = fpMalloc;
				pInt->fpFree_ncnb   = fpFree;
			}
			/* Assign the return value */
			pRet = (rv_depack*) pInt;
		}
	}

	return pRet;
}

HX_RESULT rv_depack_init(rv_depack* pDepack, rm_stream_header* header)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pDepack && header)
    {
        /* Get the internal struct */
        rv_depack_internal* pInt = (rv_depack_internal*) pDepack;
        /* Call the internal init */
        retVal = rv_depacki_init(pInt, header);
    }

    return retVal;
}

UINT32 rv_depack_get_num_substreams(rv_depack* pDepack)
{
    UINT32 ulRet = 0;

    if (pDepack)
    {
        /* Get the internal struct */
        rv_depack_internal* pInt = (rv_depack_internal*) pDepack;
        /* Return the number of substreams */
        ulRet = pInt->multiStreamHdr.ulNumSubStreams;
    }

    return ulRet;
}

UINT32 rv_depack_get_codec_4cc(rv_depack* pDepack)
{
    UINT32 ulRet = 0;

    if (pDepack)
    {
        /* Get the internal struct */
        rv_depack_internal* pInt = (rv_depack_internal*) pDepack;
        if (pInt->pSubStreamHdr &&
            pInt->ulActiveSubStream < pInt->multiStreamHdr.ulNumSubStreams)
        {
            /* Copy the subMOF tag */
            ulRet = pInt->pSubStreamHdr[pInt->ulActiveSubStream].ulSubMOFTag;
        }
    }

    return ulRet;
}

HX_RESULT rv_depack_get_codec_init_info(rv_depack* pDepack, rv_format_info** ppInfo)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pDepack && ppInfo)
    {
        /* Get the internal struct */
        rv_depack_internal* pInt = (rv_depack_internal*) pDepack;
        if (pInt->pSubStreamHdr &&
            pInt->ulActiveSubStream < pInt->multiStreamHdr.ulNumSubStreams)
        {
            /* Clean up any existing format info */
            rv_depacki_cleanup_format_info(pInt, *ppInfo);
            /* Allocate memory for the format info */
            *ppInfo = rv_depacki_malloc(pInt, sizeof(rv_format_info));
            if (*ppInfo)
            {
                /* NULL out the memory */
                memset(*ppInfo, 0, sizeof(rv_format_info));
                /* Make a deep copy of the format info */
                retVal = rv_depacki_copy_format_info(pInt,
                                                     &pInt->pSubStreamHdr[pInt->ulActiveSubStream],
                                                     *ppInfo);
            }
        }
    }

    return retVal;
}

void rv_depack_destroy_codec_init_info(rv_depack* pDepack, rv_format_info** ppInfo)
{
    if (pDepack && ppInfo && *ppInfo)
    {
        /* Get the internal struct */
        rv_depack_internal* pInt = (rv_depack_internal*) pDepack;
        /* Clean up the format info's internal allocs */
        rv_depacki_cleanup_format_info(pInt, *ppInfo);
        /* NULL it out */
        memset(*ppInfo, 0, sizeof(rv_format_info));
        /* Delete the memory associated with it */
        rv_depacki_free(pInt, *ppInfo);
        /* NULL the pointer out */
        *ppInfo = HXNULL;
    }
}

HX_RESULT rv_depack_add_packet(rv_depack* pDepack, rm_packet* packet)
{
	HX_RESULT retVal = HXR_FAIL;

	if (pDepack && packet)
	{
		/* Get the internal struct */
		rv_depack_internal* pInt = (rv_depack_internal*) pDepack;
		/* Call the internal function */
		retVal = rv_depacki_add_packet(pInt, packet);
	}

	return retVal;
}

void rv_depack_destroy_frame(rv_depack* pDepack, rv_frame** ppFrame)
{
    if (pDepack && ppFrame && *ppFrame)
    {
        /* Get the internal struct */
        rv_depack_internal* pInt = (rv_depack_internal*) pDepack;       
        /* Call the internal function */
        rv_depacki_cleanup_frame(pInt, ppFrame);
    }
}

HX_RESULT rv_depack_seek(rv_depack* pDepack, UINT32 ulTime)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pDepack)
    {
        /* Get the internal struct */
        rv_depack_internal* pInt = (rv_depack_internal*) pDepack;
        pInt->ulLastReqTimeOffset = 0;
        pInt->ulKeyFrameTableOffset = 0;
        memset(pInt->pKeyFrameTable, 0, pInt->ulKeyFrameTableCount * 2);
        pInt->ulKeyFrameTableCount = 0;
        pInt->ulLastReqTime = 0; 
        /* Call the internal function */
        retVal = rv_depacki_seek(pInt, ulTime);
    }

    return retVal;
}

void rv_depack_destroy(rv_depack** ppDepack)
{
    if (ppDepack)
    {
        rv_depack_internal* pInt = (rv_depack_internal*) *ppDepack;
        if (pInt && pInt->fpFree)
        {
            /* Save a pointer to fpFree and pUserMem */
            rm_free_func_ptr fpFree   = pInt->fpFree;
            void*            pUserMem = pInt->pUserMem;
            /* Clean up the rule to flag map */
            if (pInt->rule2Flag.pulMap)
            {
                rv_depacki_free(pInt, pInt->rule2Flag.pulMap);
                pInt->rule2Flag.pulMap     = HXNULL;
                pInt->rule2Flag.ulNumRules = 0;
            }
            /* Clean up the rule to header map */
            if (pInt->multiStreamHdr.rule2SubStream.pulMap)
            {
                rv_depacki_free(pInt, pInt->multiStreamHdr.rule2SubStream.pulMap);
                pInt->multiStreamHdr.rule2SubStream.pulMap     = HXNULL;
                pInt->multiStreamHdr.rule2SubStream.ulNumRules = 0;
            }
            /* Clean up the format info array */
            rv_depacki_cleanup_format_info_array(pInt);
            /* Clean up ignore header array */
            if (pInt->bIgnoreSubStream)
            {
                rv_depacki_free(pInt, pInt->bIgnoreSubStream);
                pInt->bIgnoreSubStream = HXNULL;
            }
            /* Clean up any current frame */
            rv_depacki_cleanup_frame(pInt, &pInt->pCurFrame);
            /* Null everything out */
            memset(pInt, 0, sizeof(rv_depack_internal));
            /* Free the rm_parser_internal struct memory */
            fpFree(pUserMem, pInt);
            /* NULL out the pointer */
            *ppDepack = HXNULL;
        }
    }
}

UINT32 rv_depack_next_keyframe_sn(rv_depack * pDepack, UINT32 ulTimeNow)
{
	UINT32 i = 0;
	UINT32 ulLastReq = 0, ulLastReqOff = 0, ulNextKeyFrameSN = 0;
	UINT32 *pKFT = NULL;
	UINT32 bIsFound = 0;

	rv_depack_internal *pInt = (rv_depack_internal *) pDepack;

	ulLastReq = pInt->ulLastReqTime;
	ulLastReqOff = pInt->ulLastReqTimeOffset;
	pKFT = pInt->pKeyFrameTable;

	//printf("now:%d,last:%d\n",ulTimeNow,ulLastReq);

	if (ulTimeNow <= ulLastReq)
	{
		//printf("return,%d\n",ulLastReq);
		return ulLastReq;
	}

	for (i = pInt->ulLastReqTimeOffset; i < pInt->ulKeyFrameTableCount; i++)
	{
		if (pKFT[i] <= ulTimeNow && pKFT[i + 1] >= ulTimeNow)
		{
			bIsFound = 1;
			break;
		}
	}

	if (bIsFound == 1)
	{
		ulNextKeyFrameSN = pKFT[i + 1];
		pInt->ulLastReqTimeOffset = i;
	}
	else
	{
		ulNextKeyFrameSN = pInt->ulLastReqTime;
		//printf("!\n");
	}

	pInt->ulLastReqTime = ulNextKeyFrameSN;

	//printf("offset:%d,nextsn:%d\n",pInt->ulLastReqTimeOffset,ulNextKeyFrameSN);

	return ulNextKeyFrameSN;
}

/* Zhang Xuecheng,2008-3-5 10:26:01 */
UINT32 rv_depack_get_fifo_cnt(rv_depack * pDepack)
{
	rv_depack_internal *pInt = NULL;
	pInt = (rv_depack_internal *) pDepack;

	return pInt->uRvFifoCnt;
}

HX_RESULT rv_depack_attach_frame(rv_depack * pDepack, rv_frame * pFrame)
{
	rv_depack_internal *pInt = NULL;

	pInt = (rv_depack_internal *) pDepack;

	if (pInt)
	{
		/* check whether the fifo has enough space */
		if (pInt->uRvFifoCnt >= RV_DEPACK_FIFO_LENGTH)
			return HXR_FAIL;

		pInt->pRvDepackFifo[pInt->uRvFifoHead] = (unsigned int)pFrame;
		pInt->uRvFifoHead++;
		pInt->uRvFifoHead %= RV_DEPACK_FIFO_LENGTH;
		pInt->uRvFifoCnt++;
	}

	return HXR_OK;
}

rv_frame *rv_depack_deattach_frame(rv_depack * pDepack)
{
	rv_frame *pFrame = NULL;
	rv_depack_internal *pInt = NULL;

	pInt = (rv_depack_internal *) pDepack;

	if (pInt)
	{
		if (pInt->uRvFifoCnt == 0)
		{
			return HXNULL;
		}

		pFrame = (rv_frame *) pInt->pRvDepackFifo[pInt->uRvFifoTail];
		pInt->uRvFifoTail++;
		pInt->uRvFifoTail %= RV_DEPACK_FIFO_LENGTH;
		pInt->uRvFifoCnt--;
	}

	return pFrame;
}

HX_RESULT rv_depack_destroy_fifo(rv_depack * pDepack)
{
	rv_frame *pFrame = NULL;
	UINT32 uFifoCnt = 0;
	rv_depack_internal *pInt = NULL;

	pInt = (rv_depack_internal *) pDepack;

	if (pInt)
	{
		uFifoCnt = pInt->uRvFifoCnt;

		while (uFifoCnt)
		{
			pFrame = (rv_frame *) pInt->pRvDepackFifo[pInt->uRvFifoTail];
			pInt->pRvDepackFifo[pInt->uRvFifoTail] = 0;
			rv_depack_destroy_frame(pInt, &pFrame);
			pInt->uRvFifoTail++;
			pInt->uRvFifoTail %= RV_DEPACK_FIFO_LENGTH;
			uFifoCnt--;
		}

		pInt->uRvFifoCnt = 0;
		pInt->uRvFifoHead = 0;
		pInt->uRvFifoTail = 0;
	}

	return HXR_OK;
}

UINT32 rv_depack_fifo_time(rv_depack * pDepack)
{
	rv_frame *pFrameHead = HXNULL;
	rv_frame *pFrameTail = HXNULL;
	rv_depack_internal *pInt = HXNULL;

	pInt = (rv_depack_internal *) pDepack;
	if (pInt)
	{
		if (pInt->uRvFifoCnt)
		{
			pFrameHead = (rv_frame *) pInt->pRvDepackFifo[pInt->uRvFifoHead - 1];
			pFrameTail = (rv_frame *) pInt->pRvDepackFifo[pInt->uRvFifoTail];

			return (pFrameHead->ulTimestamp > pFrameTail->ulTimestamp) ?
				(pFrameHead->ulTimestamp - pFrameTail->ulTimestamp) : 0;
		}
	}

	return 0;
}

UINT32 rv_depack_destroy_to_next_key_frame(rv_depack * pDepack)
{
	unsigned int frmheader = 0;
	rv_frame* pFrame = NULL;
	rv_depack_internal *pInt = HXNULL;

	pInt = (rv_depack_internal *) pDepack;
	
	for(;;)
	{
		if (pInt)
		{
			if (pInt->uRvFifoCnt == 0)
			{
				//printf("rv depack fifo is empty!!!\n");
				return 1;
			}

			pFrame = (rv_frame *) pInt->pRvDepackFifo[pInt->uRvFifoTail];

			frmheader = (*(unsigned char*)(pFrame->pData))& 0x60;
			
			if (pInt->pSubStreamHdr[pInt->ulActiveSubStream].ulSubMOFTag == 0x52563330)
			{
				frmheader &= 0x18;
				frmheader <<= 2;
			}
			else
				frmheader &= 0x60;

			if((frmheader) == 0x20 ||(frmheader) == 0x00 ||(frmheader) == 0xff)
				return 0;
			
			pInt->uRvFifoTail++;
			pInt->uRvFifoTail %= RV_DEPACK_FIFO_LENGTH;
			pInt->uRvFifoCnt--;

			rv_depack_destroy_frame(pInt, &pFrame);

			return 0;
		}
		else
		{
			return 1;
		}
	}	
}

/*
* Return:	HXR_OK
*			HXR_FAIL
*/
HX_RESULT rv_depack_add_packet_ex(rv_depack * pDepack, rm_packet * pPacket, UINT32 ulDepackMode)
{
	HX_RESULT retVal = HXR_FAIL;

	if (pDepack && pPacket)
	{
		/* Get the internal struct */
		rv_depack_internal *pInt = (rv_depack_internal *) pDepack;
		/* Call the internal function */
		retVal = rv_depacki_add_packet_ex(pInt, pPacket, ulDepackMode);
	}

	return retVal;
}

UINT8 rv_depack_get_pictype(rv_depack *pDepack, rv_frame *pFrame)
{
	UINT8 pictype = 0xFF;

	if (pDepack && pFrame)
	{
		/* Get the internal struct */
		rv_depack_internal *pInt = (rv_depack_internal *) pDepack;
		/* Call the internal function */
		pictype = rv_depacki_get_pictype(pInt, pFrame);
	}

	return pictype;
}

HX_RESULT rv_depack_get_last_frame(rv_depack *pDepack, rv_frame **ppFrame)
{
	HX_RESULT retVal = HXR_FAIL;

	if (pDepack && ppFrame)
	{
		/* Get the internal struct */
		rv_depack_internal *pInt = (rv_depack_internal *) pDepack;
		/* Clear ppFrame */
		*ppFrame = HXNULL;
		/* Call the internal function */
		rv_depacki_get_current_frame(pInt, ppFrame);
		/* Clear the pCurFrame */
		pInt->pCurFrame = HXNULL;
		/* Set the return value */
		if (*ppFrame)
			retVal = HXR_OK;
	}

	return retVal;
}

HXBOOL rv_depack_is_frame_ready(rv_depack * pDepack)
{
	HXBOOL retVal = FALSE;

	if (pDepack)
	{
		rv_depack_internal *pInt  = (rv_depack_internal *) pDepack;
		if (pInt->pReadyFrame)
		{
			retVal = TRUE;
		}
	}

	return retVal;
}

HX_RESULT rv_depack_get_ready_frame(rv_depack *pDepack, rv_frame **ppFrame)
{
	HX_RESULT retVal = HXR_FAIL;

	if (pDepack && ppFrame)
	{
		/* Get the internal struct */
		rv_depack_internal *pInt = (rv_depack_internal *) pDepack;
		/* Call the internal function */
		retVal = rv_depacki_get_ready_frame(pInt, ppFrame);
		/* Clear the pCurFrame */
		pInt->pReadyFrame = HXNULL;
	}

	return retVal;
}

HXBOOL rv_depack_is_keyframe(rv_depack *pDepack, rv_frame *pFrame)
{
	HXBOOL retVal = FALSE;

	if (pDepack && pFrame)
	{
		rv_depack_internal *pInt  = (rv_depack_internal *) pDepack;
		UINT8 pictype = rv_depacki_get_pictype(pInt, pFrame);
		if (pictype == RV_INTRAPIC || pictype == RV_FORCED_INTRAPIC)
		{
			retVal = TRUE;
		}
	}

	return retVal;
}

void rv_depack_destroy_frame_ex(rv_depack* pDepack, rv_frame** ppFrame)
{
	if (pDepack && ppFrame && *ppFrame)
	{
		/* Get the internal struct */
		rv_depack_internal* pInt = (rv_depack_internal*) pDepack;       
		/* Call the internal function */
		rv_depacki_cleanup_frame_ex(pInt, ppFrame);
	}
}

HX_RESULT rv_depack_set_frame_buffer(rv_depack *pDepack, BYTE *bufAddr, UINT32 bufLen)
{
	HX_RESULT retVal = HXR_FAIL;

	if (pDepack && bufAddr)
	{
		/* Get the internal struct */
		rv_depack_internal *pInt = (rv_depack_internal *) pDepack;
		/* Call the internal function */
		retVal = rv_depacki_set_frame_buffer(pInt, bufAddr, bufLen);
	}

	return retVal;
}

void rv_depack_cleanup_frame_buffer(rv_depack *pDepack)
{
	if (pDepack)
	{
		/* Get the internal struct */
		rv_depack_internal *pInt = (rv_depack_internal *) pDepack;
		/* Call the internal function */
        rv_depacki_cleanup_frame_buffer(pInt);
	}
}

void rv_depack_reset(rv_depack *pDepack)
{
	if (pDepack)
	{
		/* Get the internal struct */
		rv_depack_internal *pInt = (rv_depack_internal *) pDepack;
		rv_depacki_cleanup_frame(pInt, &pInt->pCurFrame);
		pInt->bCreatedFirstFrame = FALSE;				
	}
}

/* the following will be removed later. -caijin @2008.06.01 */
#if 0
rv_depack *rv_depack_create3(void *pUserError,
							 rm_error_func_ptr fpError,
							 void *pUserMem,
							 rm_malloc_func_ptr fpMalloc,
							 rm_free_func_ptr fpFree)
{
	rv_depack *pRet = HXNULL;

	if (fpMalloc && fpFree)
	{
		/* Allocate space for the rv_depack_internal struct
		* by using the passed-in malloc function
		*/
		rv_depack_internal *pInt = (rv_depack_internal *) fpMalloc(pUserMem, sizeof(rv_depack_internal));
		if (pInt)
		{
			/* Zero out the struct */
			memset((void *)pInt, 0, sizeof(rv_depack_internal));
			/*
			* Assign the error members. If the caller did not
			* provide an error callback, then use the default
			* rm_error_default().
			*/
			if (fpError)
			{
				pInt->fpError = fpError;
				pInt->pUserError = pUserError;
			}
			else
			{
				pInt->fpError = rm_error_default;
				pInt->pUserError = HXNULL;
			}
			/* Assign the memory functions */
			pInt->fpMalloc = fpMalloc;
			pInt->fpFree = fpFree;
			pInt->pUserMem = pUserMem;
			/* Assign the return value */
			pRet = (rv_depack *) pInt;
		}
	}

	return pRet;
}

HX_RESULT rv_depack_get_tailframe_duration(rv_depack * pDepack, UINT32 * ulDuration)
{
	HX_RESULT retVal = HXR_FAIL;
	if (pDepack && ulDuration)
	{
		rv_depack_internal *pInt = (rv_depack_internal *) pDepack;
		if (pInt->uRvFifoCnt >= 2)
		{
			rv_frame *pFrameTail = (rv_frame *) pInt->pRvDepackFifo[pInt->uRvFifoTail];
			rv_frame *pFrameNext = (rv_frame *) pInt->pRvDepackFifo[(pInt->uRvFifoTail+1)%RV_DEPACK_FIFO_LENGTH];
			*ulDuration = pFrameNext->ulTimestamp - pFrameTail->ulTimestamp;
			retVal = HXR_OK;
		}
	}

	return retVal;
}

#endif
