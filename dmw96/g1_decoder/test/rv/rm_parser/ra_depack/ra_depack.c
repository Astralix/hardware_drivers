/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ra_depack.c,v 1.1 2009-03-11 14:11:09 kima Exp $
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

//HPX
//#include <memory.h>
#include "helix_types.h"
#include "helix_result.h"
#include "ra_depack.h"
#include "ra_depack_internal.h"
#include "rm_memory_default.h"
#include "rm_error_default.h"
#include "memory_utils.h"

ra_depack *ra_depack_create3(void *pUserError,
							 rm_error_func_ptr fpError,
							 void *pUserMem,
							 rm_malloc_func_ptr fpMalloc,
							 rm_free_func_ptr fpFree)
{
	ra_depack *pRet = HXNULL;

	if (fpMalloc && fpFree)
	{
		/* Allocate space for the ra_depack_internal struct
		 * by using the passed-in malloc function
		 */
		ra_depack_internal *pInt = (ra_depack_internal *) fpMalloc(pUserMem, sizeof(ra_depack_internal));
		if (pInt)
		{
			/* Zero out the struct */
			memset((void *)pInt, 0, sizeof(ra_depack_internal));
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
			pRet = (ra_depack *) pInt;
		}
	}

	return pRet;
}

HX_RESULT ra_depack_init(ra_depack * pDepack, rm_stream_header * header)
{
	HX_RESULT retVal = HXR_FAIL;

	if (pDepack && header)
	{
		/* Get the internal struct */
		ra_depack_internal *pInt = (ra_depack_internal *) pDepack;
		/* Call the internal init */
		retVal = ra_depacki_init(pInt, header);
	}

	return retVal;
}

UINT32 ra_depack_get_num_substreams(ra_depack * pDepack)
{
	UINT32 ulRet = 0;

	if (pDepack)
	{
		/* Get the internal struct */
		ra_depack_internal *pInt = (ra_depack_internal *) pDepack;
		/* Return the number of substreams */
		ulRet = pInt->multiStreamHdr.ulNumSubStreams;
	}

	return ulRet;
}

UINT32 ra_depack_get_codec_4cc(ra_depack * pDepack, UINT32 ulSubStream)
{
	UINT32 ulRet = 0;

	if (pDepack)
	{
		/* Get the internal struct */
		ra_depack_internal *pInt = (ra_depack_internal *) pDepack;
		/* Make sure the substream index is legal */
		if (pInt->pSubStreamHdr && ulSubStream < pInt->multiStreamHdr.ulNumSubStreams)
		{
			ulRet = pInt->pSubStreamHdr[ulSubStream].ulCodecID;
		}
	}

	return ulRet;
}

HX_RESULT ra_depack_get_codec_init_info(ra_depack * pDepack,
	UINT32 ulSubStream, ra_format_info ** ppInfo)
{
	HX_RESULT retVal = HXR_FAIL;

	if (pDepack && ppInfo)
	{
		/* Init local variables */
		UINT32 ulSize = sizeof(ra_format_info);
		ra_format_info *pInfo = HXNULL;
		/* Get the internal struct */
		ra_depack_internal *pInt = (ra_depack_internal *) pDepack;
		/* Allocate space for the struct */
		pInfo = ra_depacki_malloc(pInt, ulSize);
		if (pInfo)
		{
			/* NULL out the memory */
			memset(pInfo, 0, ulSize);
			/* Fill in the init info struct */
			retVal = ra_depacki_get_format_info(pInt, ulSubStream, pInfo);
			if (retVal == HXR_OK)
			{
				/* Assign the out parameter */
				*ppInfo = pInfo;
			}
			else
			{
				/* We failed so free the memory we allocated */
				ra_depacki_free(pInt, pInfo);
			}
		}
	}

	return retVal;
}

void ra_depack_destroy_codec_init_info(ra_depack * pDepack, ra_format_info ** ppInfo)
{
	if (pDepack && ppInfo && *ppInfo)
	{
		/* Get the internal struct */
		ra_depack_internal *pInt = (ra_depack_internal *) pDepack;
		/* Clean up the format info struct */
		ra_depacki_cleanup_format_info(pInt, *ppInfo);
		/* Delete the memory associated with it */
		ra_depacki_free(pInt, *ppInfo);
		/* NULL the pointer out */
		*ppInfo = HXNULL;
	}
}

HX_RESULT ra_depack_add_packet(ra_depack * pDepack, rm_packet * packet)
{
	HX_RESULT retVal = HXR_FAIL;

	if (pDepack && packet)
	{
		/* Get the internal struct */
		ra_depack_internal *pInt = (ra_depack_internal *) pDepack;
		/* Call the internal function */
		retVal = ra_depacki_add_packet(pInt, packet);
	}

	return retVal;
}

void ra_depack_destroy_block(ra_depack * pDepack, ra_block ** ppBlock)
{
	if (pDepack && ppBlock && *ppBlock)
	{
		/* Get the internal struct */
		ra_depack_internal *pInt = (ra_depack_internal *) pDepack;
		/* Free the data */
		if ((*ppBlock)->pData)
		{
			ra_depacki_free(pInt, (*ppBlock)->pData);
			(*ppBlock)->pData = HXNULL;
		}
		/* Free the memory itself */
		ra_depacki_free(pInt, *ppBlock);
		/* Null out the pointer */
		*ppBlock = HXNULL;
	}
}

HX_RESULT ra_depack_seek(ra_depack * pDepack, UINT32 ulTime)
{
	HX_RESULT retVal = HXR_FAIL;

	if (pDepack)
	{
		/* Get the internal struct */
		ra_depack_internal *pInt = (ra_depack_internal *) pDepack;
		/* Call the internal seek function */
		retVal = ra_depacki_seek(pInt, ulTime);
	}

	return retVal;
}

void ra_depack_destroy(ra_depack ** ppDepack)
{
	if (ppDepack)
	{
		ra_depack_internal *pInt = (ra_depack_internal *) * ppDepack;
		/* Save a pointer to fpFree and pUserMem */
		rm_free_func_ptr fpFree = pInt->fpFree;
		void *pUserMem = pInt->pUserMem;

		if (pInt && pInt->fpFree)
		{
			/*
			 * zhang xuecheng,2008-2-18 17:42:14
			 * the rule2Flag of ra_depack wasn't free.
			 */
			/* Clean up the rule to flag map */
			if (pInt->rule2Flag.pulMap)
			{
				ra_depacki_free(pInt, pInt->rule2Flag.pulMap);
				pInt->rule2Flag.pulMap = HXNULL;
				pInt->rule2Flag.ulNumRules = 0;
			}
			/* Clean up the rule to header map */
			if (pInt->multiStreamHdr.rule2SubStream.pulMap)
			{
				ra_depacki_free(pInt, pInt->multiStreamHdr.rule2SubStream.pulMap);
				pInt->multiStreamHdr.rule2SubStream.pulMap = HXNULL;
				pInt->multiStreamHdr.rule2SubStream.ulNumRules = 0;
			}

			/* Clean up the substream header array */
			ra_depacki_cleanup_substream_hdr_array(pInt);
			/* Null everything out */
			memset(pInt, 0, sizeof(ra_depack_internal));
			/* Free the rm_parser_internal struct memory */
			fpFree(pUserMem, pInt);
			/* NULL out the pointer */
			*ppDepack = HXNULL;
		}
	}
}

/* Zhang Xuecheng,2008-3-5 10:26:01 */
UINT32 ra_depack_get_fifo_cnt(ra_depack * pDepack)
{
	ra_depack_internal *pInt = HXNULL;
	pInt = (ra_depack_internal *) pDepack;

	return pInt->uRaFifoCnt;
}

HX_RESULT ra_depack_attach_block(ra_depack * pDepack, ra_block * pBlock)
{
	ra_depack_internal *pInt = HXNULL;

	pInt = (ra_depack_internal *) pDepack;

	/* check whether the fifo has enough space */
	if (pInt->uRaFifoCnt >= RA_DEPACK_FIFO_LENGTH)
		return HXR_FAIL;

	pInt->pRaDepackFifo[pInt->uRaFifoHead] = (unsigned int)pBlock;
	pInt->uRaFifoHead++;
	pInt->uRaFifoHead %= RA_DEPACK_FIFO_LENGTH;
	pInt->uRaFifoCnt++;

	//printf("[ra_depack_attach_block]:pBlock:0x%08x,head:0x%08x,tail:0x%08x,cnt:0x%08x\n",
	//      (unsigned int)pBlock,pInt->uRaFifoHead,pInt->uRaFifoTail,pInt->uRaFifoCnt);

	return HXR_OK;
}

ra_block *ra_depack_deattach_block(ra_depack * pDepack)
{
	ra_block *pBlock = HXNULL;
	ra_depack_internal *pInt = HXNULL;

	pInt = (ra_depack_internal *) pDepack;

	if (pInt->uRaFifoCnt == 0)
	{
		printf("ra depack fifo is empty!!!\n");
		return HXNULL;
	}

	pBlock = (ra_block *) pInt->pRaDepackFifo[pInt->uRaFifoTail];
	pInt->uRaFifoTail++;
	pInt->uRaFifoTail %= RA_DEPACK_FIFO_LENGTH;
	pInt->uRaFifoCnt--;

	//printf("[ra_depack_deattach_block]:pBlock:0x%08x,head:0x%08x,tail:0x%08x,cnt:0x%08x\n",
	//      (unsigned int)pBlock,pInt->uRaFifoHead,pInt->uRaFifoTail,pInt->uRaFifoCnt);

	return pBlock;
}

HX_RESULT ra_depack_destroy_fifo(ra_depack * pDepack)
{
	ra_block *pBlock = HXNULL;
	UINT32 uFifoCnt = 0;
	ra_depack_internal *pInt = HXNULL;

	pInt = (ra_depack_internal *) pDepack;
	uFifoCnt = pInt->uRaFifoCnt;

	while (uFifoCnt)
	{
		pBlock = (ra_block *) pInt->pRaDepackFifo[pInt->uRaFifoTail];
		pInt->pRaDepackFifo[pInt->uRaFifoTail] = 0;
		ra_depack_destroy_block(pInt, &pBlock);
		pInt->uRaFifoTail++;
		pInt->uRaFifoTail %= RA_DEPACK_FIFO_LENGTH;
		uFifoCnt--;
	}

	pInt->uRaFifoCnt = 0;
	pInt->uRaFifoHead = 0;
	pInt->uRaFifoTail = 0;

	return HXR_OK;
}

UINT32 ra_depack_fifo_time(ra_depack * pDepack)
{
	ra_block *pBlockHead = HXNULL;
	ra_block *pBlockTail = HXNULL;
	ra_depack_internal *pInt = HXNULL;

	pInt = (ra_depack_internal *) pDepack;

	if (pInt->uRaFifoCnt)
	{
		pBlockHead = (ra_block *) pInt->pRaDepackFifo[pInt->uRaFifoHead - 1];
		pBlockTail = (ra_block *) pInt->pRaDepackFifo[pInt->uRaFifoTail];

		return (pBlockHead->ulTimestamp > pBlockTail->ulTimestamp) ?
			(pBlockHead->ulTimestamp - pBlockTail->ulTimestamp) : 0;
	}
	else
		return 0;
}
