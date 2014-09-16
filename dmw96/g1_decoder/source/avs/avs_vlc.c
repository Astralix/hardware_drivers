/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Hantro Products Oy.                             --
--                                                                            --
--                   (C) COPYRIGHT 2011 HANTRO PRODUCTS OY                    --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Abstract : Decode Exp-Golomb code words
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/

#include "avs_vlc.h"
#include "basetype.h"
#include "avs_utils.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

/* definition of special code num, this along with the return value is used
 * to handle code num in the range [0, 2^32] in the DecodeExpGolombUnsigned
 * function */
#define BIG_CODE_NUM 0xFFFFFFFFU

/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------

   5.1  Function: AvsDecodeExpGolombUnsigned

        Functional description:
            Decode unsigned Exp-Golomb code. This is the same as codeNum used
            in other Exp-Golomb code mappings. Code num (i.e. the decoded
            symbol) is determined as

                codeNum = 2^leadingZeros - 1 + GetBits(leadingZeros)

            Normal decoded symbols are in the range [0, 2^32 - 2]. Symbol
            2^32-1 is indicated by BIG_CODE_NUM with return value HANTRO_OK
            while symbol 2^32  is indicated by BIG_CODE_NUM with return value
            HANTRO_NOK.  These two symbols are special cases with code length
            of 65, i.e.  32 '0' bits, a '1' bit, and either 0 or 1 represented
            by 32 bits.

            Symbol 2^32 is out of unsigned 32-bit range but is needed for
            DecodeExpGolombSigned to express value -2^31.

        Inputs:
            pDecCont       pointer to stream data structure

        Outputs:
            codeNum         decoded code word is stored here

        Returns:
            HANTRO_OK       success
            HANTRO_NOK      failure, no valid code word found, note exception
                            with BIG_CODE_NUM

------------------------------------------------------------------------------*/

u32 AvsDecodeExpGolombUnsigned(DecContainer *pDecCont, u32 *codeNum)
{

/* Variables */

    u32 bits, numZeros;

/* Code */

    ASSERT(pDecCont);
    ASSERT(codeNum);

    bits = AvsStrmDec_ShowBits(pDecCont, 32);

    /* first bit is 1 -> code length 1 */
    if (bits >= 0x80000000)
    {
        if (AvsStrmDec_FlushBits(pDecCont, 1)== END_OF_STREAM)
            return(HANTRO_NOK);
        *codeNum = 0;
        return(HANTRO_OK);
    }
    /* second bit is 1 -> code length 3 */
    else if (bits >= 0x40000000)
    {
        if (AvsStrmDec_FlushBits(pDecCont, 3) == END_OF_STREAM)
            return(HANTRO_NOK);
        *codeNum = 1 + ((bits >> 29) & 0x1);
        return(HANTRO_OK);
    }
    /* third bit is 1 -> code length 5 */
    else if (bits >= 0x20000000)
    {
        if (AvsStrmDec_FlushBits(pDecCont, 5) == END_OF_STREAM)
            return(HANTRO_NOK);
        *codeNum = 3 + ((bits >> 27) & 0x3);
        return(HANTRO_OK);
    }
    /* fourth bit is 1 -> code length 7 */
    else if (bits >= 0x10000000)
    {
        if (AvsStrmDec_FlushBits(pDecCont, 7) == END_OF_STREAM)
            return(HANTRO_NOK);
        *codeNum = 7 + ((bits >> 25) & 0x7);
        return(HANTRO_OK);
    }
    /* other code lengths */
    else
    {
        numZeros = 4 + AvsStrmDec_CountLeadingZeros(bits, 28);

        /* all 32 bits are zero */
        if (numZeros == 32)
        {
            *codeNum = 0;
            if (AvsStrmDec_FlushBits(pDecCont,32) == END_OF_STREAM)
                return(HANTRO_NOK);
            bits = AvsStrmDec_GetBits(pDecCont, 1);
            /* check 33rd bit, must be 1 */
            if (bits == 1)
            {
                /* cannot use AvsGetBits, limited to 31 bits */
                bits = AvsStrmDec_ShowBits(pDecCont,32);
                if (AvsStrmDec_FlushBits(pDecCont, 32) == END_OF_STREAM)
                    return(HANTRO_NOK);
                /* code num 2^32 - 1, needed for unsigned mapping */
                if (bits == 0)
                {
                    *codeNum = BIG_CODE_NUM;
                    return(HANTRO_OK);
                }
                /* code num 2^32, needed for unsigned mapping
                 * (results in -2^31) */
                else if (bits == 1)
                {
                    *codeNum = BIG_CODE_NUM;
                    return(HANTRO_NOK);
                }
            }
            /* if more zeros than 32, it is an error */
            return(HANTRO_NOK);
        }
        else
            if (AvsStrmDec_FlushBits(pDecCont,numZeros+1) == END_OF_STREAM)
                return(HANTRO_NOK);


        bits = AvsStrmDec_GetBits(pDecCont, numZeros);
        if (bits == END_OF_STREAM)
            return(HANTRO_NOK);

        *codeNum = (1 << numZeros) - 1 + bits;

    }

    return(HANTRO_OK);

}

/*------------------------------------------------------------------------------

   5.2  Function: AvsDecodeExpGolombSigned

        Functional description:
            Decode signed Exp-Golomb code. Code num is determined by
            AvsDecodeExpGolombUnsigned and then mapped to signed
            representation as

                symbol = (-1)^(codeNum+1) * (codeNum+1)/2

            Signed symbols shall be in the range [-2^31, 2^31 - 1]. Symbol
            -2^31 is obtained when codeNum is 2^32, which cannot be expressed
            by unsigned 32-bit value. This is signaled as a special case from
            the AvsDecodeExpGolombUnsigned by setting codeNum to
            BIG_CODE_NUM and returning HANTRO_NOK status.

        Inputs:
            pDecCont       pointer to stream data structure

        Outputs:
            value           decoded code word is stored here

        Returns:
            HANTRO_OK       success
            HANTRO_NOK      failure, no valid code word found

------------------------------------------------------------------------------*/

u32 AvsDecodeExpGolombSigned(DecContainer *pDecCont, i32 *value)
{

/* Variables */

    u32 status, codeNum = 0;

/* Code */

    ASSERT(pDecCont);
    ASSERT(value);

    status = AvsDecodeExpGolombUnsigned(pDecCont, &codeNum);

    if (codeNum == BIG_CODE_NUM)
    {
        /* BIG_CODE_NUM and HANTRO_OK status means codeNum 2^32-1 which would
         * result in signed integer valued 2^31 (i.e. out of 32-bit signed
         * integer range) */
        if (status == HANTRO_OK)
            return(HANTRO_NOK);
        /* BIG_CODE_NUM and HANTRO_NOK status means codeNum 2^32 which results
         * in signed integer valued -2^31 */
        else
        {
            *value = (i32)(2147483648U);
            return (HANTRO_OK);
        }
    }
    else if (status == HANTRO_OK)
    {
        /* (-1)^(codeNum+1) results in positive sign if codeNum is odd,
         * negative when it is even. (codeNum+1)/2 is obtained as
         * (codeNum+1)>>1 when value is positive and as (-codeNum)>>1 for
         * negative value */
        /*lint -e702 */
        *value = (codeNum & 0x1) ? (i32)((codeNum + 1) >> 1) :
                                  -(i32)((codeNum + 1) >> 1);
        /*lint +e702 */
        return(HANTRO_OK);
    }

    return(HANTRO_NOK);

}
