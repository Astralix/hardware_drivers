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
--  Abstract :
--
--------------------------------------------------------------------------------
--
--  Version control information, please leave untouched.
--
--  $RCSfile: EncMbHeader.c,v $
--  $Revision: 1.3 $
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "enccommon.h"
#include "EncMbHeader.h"
#include "EncDifferentialMvCoding.h"
#include "EncVlc.h"
#include "ewl.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/
/* Table B-6: VLC table for mcbpc for I-VOP */
static const table_s mcbpcIVop[9] = {
    {1, 1}, /* mbtype 3, cbpc (00) */
    {1, 3}, /* mbtype 3, cbpc (01) */
    {2, 3}, /* mbtype 3, cbpc (10) */
    {3, 3}, /* mbtype 3, cbpc (11) */
    {1, 4}, /* mbtype 4, cbpc (00) */
    {1, 6}, /* mbtype 4, cbpc (01) */
    {2, 6}, /* mbtype 4, cbpc (10) */
    {3, 6}, /* mbtype 4, cbpc (11) */
    {1, 9}  /* Stuffing, cbpc (--) */
};

/* Table B-7: VLC table for mcbpc for P-VOP */
static const table_s mcbpcPVop[21] = {
    {1, 1}, /* mbtype 0, cbpc (00) */
    {3, 4}, /* mbtype 0, cbpc (01) */
    {2, 4}, /* mbtype 0, cbpc (10) */
    {5, 6}, /* mbtype 0, cbpc (11) */
    {3, 3}, /* mbtype 1, cbpc (00) */
    {7, 7}, /* mbtype 1, cbpc (01) */
    {6, 7}, /* mbtype 1, cbpc (10) */
    {5, 9}, /* mbtype 1, cbpc (11) */
    {2, 3}, /* mbtype 2, cbpc (00) */
    {5, 7}, /* mbtype 2, cbpc (01) */
    {4, 7}, /* mbtype 2, cbpc (10) */
    {5, 8}, /* mbtype 2, cbpc (11) */
    {3, 5}, /* mbtype 3, cbpc (00) */
    {4, 8}, /* mbtype 3, cbpc (01) */
    {3, 8}, /* mbtype 3, cbpc (10) */
    {3, 7}, /* mbtype 3, cbpc (11) */
    {4, 6}, /* mbtype 4, cbpc (00) */
    {4, 9}, /* mbtype 4, cbpc (01) */
    {3, 9}, /* mbtype 4, cbpc (10) */
    {2, 9}, /* mbtype 4, cbpc (11) */
    {1, 9}  /* Stuffing, cbpc (--) */
};

/* Table B-8: VLC table for cbpy  */
static const table_s cbpy[16] = {
    {3, 4}, /* cbpy(0000) */
    {5, 5}, /* cbpy(0001) */
    {4, 5}, /* cbpy(0010) */
    {9, 4}, /* cbpy(0011) */
    {3, 5}, /* cbpy(0100) */
    {7, 4}, /* cbpy(0101) */
    {2, 6}, /* cbpy(0110) */
    {11, 4},    /* cbpy(0111) */
    {2, 5}, /* cbpy(1000) */
    {3, 6}, /* cbpy(1001) */
    {5, 4}, /* cbpy(1010) */
    {10, 4},    /* cbpy(1011) */
    {4, 4}, /* cbpy(1100) */
    {8, 4}, /* cbpy(1101) */
    {6, 4}, /* cbpy(1110) */
    {3, 2}  /* cbpy(1111) */
};

/* Table 6-27: dquant codes and corresponding values  */
static const table_s dQuant[5] = {
    {1, 2}, /* -2 */
    {0, 2}, /* -1 */
    {0, 2}, /* Forbidden" */
    {2, 2}, /* 1 */
    {3, 2}  /* 2 */
};

/* Table B-12: VLC table for MVD (Motion vector difference) */
static const table_s mvd[65] = {
    {5, 13},    /* -16  */
    {7, 13},    /* -15.5 */
    {5, 12},    /* -15  */
    {7, 12},    /* -14.5 */
    {9, 12},    /* -14  */
    {11, 12},   /* -13.5 */
    {13, 12},   /* -13  */
    {15, 12},   /* -12.5 */
    {9, 11},    /* -12  */
    {11, 11},   /* -11.5 */
    {13, 11},   /* -11  */
    {15, 11},   /* -10.5 */
    {17, 11},   /* -10  */
    {19, 11},   /* -9.5 */
    {21, 11},   /* -9   */
    {23, 11},   /* -8.5 */
    {25, 11},   /* -8   */
    {27, 11},   /* -7.5 */
    {29, 11},   /* -7   */
    {31, 11},   /* -6.5 */
    {33, 11},   /* -6   */
    {35, 11},   /* -5.5 */
    {19, 10},   /* -5   */
    {21, 10},   /* -4.5 */
    {23, 10},   /* -4   */
    {7, 8}, /* -3.5 */
    {9, 8}, /* -3   */
    {11, 8},    /* -2.5 */
    {7, 7}, /* -2   */
    {3, 5}, /* -1.5 */
    {3, 4}, /* -1   */
    {3, 3}, /* -0.5 */
    {1, 1}, /* 0    */
    {2, 3}, /* 0.5  */
    {2, 4}, /* 1    */
    {2, 5}, /* 1.5  */
    {6, 7}, /* 2    */
    {10, 8},    /* 2.5  */
    {8, 8}, /* 3    */
    {6, 8}, /* 3.5  */
    {22, 10},   /* 4    */
    {20, 10},   /* 4.5  */
    {18, 10},   /* 5    */
    {34, 11},   /* 5.5  */
    {32, 11},   /* 6    */
    {30, 11},   /* 6.5  */
    {28, 11},   /* 7    */
    {26, 11},   /* 7.5  */
    {24, 11},   /* 8    */
    {22, 11},   /* 8.5  */
    {20, 11},   /* 9    */
    {18, 11},   /* 9.5  */
    {16, 11},   /* 10   */
    {14, 11},   /* 10.5 */
    {12, 11},   /* 11   */
    {10, 11},   /* 11.5 */
    {8, 11},    /* 12   */
    {14, 12},   /* 12.5 */
    {12, 12},   /* 13   */
    {10, 12},   /* 13.5 */
    {8, 12},    /* 14   */
    {6, 12},    /* 14.5 */
    {4, 12},    /* 15   */
    {6, 13},    /* 15.5 */
    {4, 13} /* 16   */
};

/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/
static void Mcbpc(stream_s *, vopType_e, type_e, i32 *);
static void Cbpy(stream_s *, type_e, i32 *);
static void MvForward(stream_s *, mvData_s, i32);

/*------------------------------------------------------------------------------

	EncMbAlloc

	Perform memory allocation needed by macroblock data and set some
	parameters.

	Input	mb	Pointer to mb_s structure.

	Return	ENCHW_OK	Memory allocation OK.
		    ENCHW_NOK	Memory allocation failed.

------------------------------------------------------------------------------*/
bool_e EncMbAlloc(mb_s * mb)
{
    i32 tmp;
    bool_e status = ENCHW_OK;
    i32 width, height;

    width = mb->width;
    height = mb->height;

    tmp = (width / 16) * (height / 16);
    if((mb->mv = (mv_s *) EWLcalloc(1, sizeof(mv_s) * tmp)) == NULL)
    {
        status = ENCHW_NOK;
    }

    tmp = sizeof(pred_s) * width / 16;

    if((mb->predAbove = (pred_s *) EWLcalloc(1, tmp)) == NULL)
    {
        status = ENCHW_NOK;
    }
    if((mb->predCurrent = (pred_s *) EWLcalloc(1, tmp)) == NULL)
    {
        status = ENCHW_NOK;
    }

    tmp = sizeof(type_e) * width / 16;

    if((mb->typeAbove = (type_e *) EWLcalloc(1, tmp)) == NULL)
    {
        status = ENCHW_NOK;
    }
    if((mb->typeCurrent = (type_e *) EWLcalloc(1, tmp)) == NULL)
    {
        status = ENCHW_NOK;
    }

    /* Defaul parameter */
    mb->mbPerVop = width / 16 * height / 16;
    mb->lastColumn = width / 16;

    /* Length of the macroblock number code in VP headers */
    mb->vpMbNumBits = 0;
    while((1U << ++mb->vpMbNumBits) < mb->mbPerVop) ;

    mb->mbNum = 0;

    if(status != ENCHW_OK)
    {
        EncMbFree(mb);
    }
    else
    {
        mb->scan[0] = mb->scan[1] = mb->scan[2] = mb->scan[3] =
            mb->scan[4] = mb->scan[5] = ZIGZAG;
    }

    return status;
}

/*------------------------------------------------------------------------------

	EncMbFree

	Free memory allocated by EncMbAlloc.

	Input	mb	Pointer to mb_s structure.

------------------------------------------------------------------------------*/
void EncMbFree(mb_s * mb)
{
    EWLfree(mb->mv);
    mb->mv = NULL;

    EWLfree(mb->predAbove);
    mb->predAbove = NULL;

    EWLfree(mb->predCurrent);
    mb->predCurrent = NULL;

    EWLfree(mb->typeAbove);
    mb->typeAbove = NULL;

    EWLfree(mb->typeCurrent);
    mb->typeCurrent = NULL;

    return;
}

/*------------------------------------------------------------------------------

	EncMbHeader

------------------------------------------------------------------------------*/
void EncMbHeader(stream_s * stream, mb_s * mb)
{
    type_e type = mb->type;
    dmv_s dmv;
    i32 i;

    /* STUFFING macroblock is not implemented here */
    ASSERT(type != STUFFING);

    /* Not Coded */
    if(mb->vopType != IVOP)
    {
        EncPutBits(stream, mb->notCoded, 1);
        COMMENT("Not Coded");
    }

    if((mb->notCoded == 0) || (mb->vopType == IVOP))
    {
        /* Rate control need quantization parameter of last coded
         * macroblock */
        mb->qpLastCoded = mb->qp;

        /* mcbpc */
        Mcbpc(stream, mb->vopType, type, mb->zeroBlock);

        /* AC-prediction flag */
        if((mb->svHdr == ENCHW_NO) && ((type == INTRA) || (type == INTRA_Q)))
        {
            EncPutBits(stream, mb->acPredFlag, 1);
            COMMENT("AC-prediction flag");
        }

        /* Cbpy */
        Cbpy(stream, type, mb->zeroBlock);

        /* Dquant */
        if((type == INTER_Q) || (type == INTRA_Q))
        {
            EncPutBits(stream, dQuant[mb->dQuant].value,
                       dQuant[mb->dQuant].number);
            COMMENT("Dquant");
        }

        /* Differential Motion Vector Data */
        if((type == INTER) || (type == INTER_Q))
        {
            dmv = EncDifferentialMvEncoding(mb, 0);
            MvForward(stream, dmv.x, mb->fCode);
            MvForward(stream, dmv.y, mb->fCode);
        }
        else if(type == INTER4V)
        {
            for(i = 0; i < 4; i++)
            {
                dmv = EncDifferentialMvEncoding(mb, i);
                MvForward(stream, dmv.x, mb->fCode);
                MvForward(stream, dmv.y, mb->fCode);
            }
        }
    }
}

/*------------------------------------------------------------------------------

	Mcbpc

------------------------------------------------------------------------------*/
void Mcbpc(stream_s * stream, vopType_e vopType, type_e type, i32 * zeroBlock)
{
    i32 cbpc;
    i32 tmp;

    /* Calculate cbpc (coded block per crominance) */
    cbpc = (zeroBlock[4] << 1) + zeroBlock[5];

    /* IVOP, index to Table B-6 */
    if(vopType == IVOP)
    {
        if(type == INTRA)
        {
            tmp = cbpc;
        }
        else
        {
            tmp = cbpc + 4;
        }
        ASSERT(type == INTRA || type == INTRA_Q);
        EncPutBits(stream, mcbpcIVop[tmp].value, mcbpcIVop[tmp].number);
        COMMENT("Mcbpc (IVOP)");
        /* P_VOP, index to Table B-6 */
    }
    else
    {
        tmp = cbpc + 4 * type;
        EncPutBits(stream, mcbpcPVop[tmp].value, mcbpcPVop[tmp].number);
        COMMENT("Mcbpc (PVOP)");
    }

    return;
}

/*------------------------------------------------------------------------------

	Cbpy

------------------------------------------------------------------------------*/
void Cbpy(stream_s * stream, type_e type, i32 * zeroBlock)
{
    i32 tmp;    /* Index to Table B-8 */

    tmp = (zeroBlock[0] << 3) + (zeroBlock[1] << 2) + (zeroBlock[2] << 1) +
        zeroBlock[3];
    if((type == INTER) || (type == INTER_Q) || (type == INTER4V))
    {
        tmp = 15 - tmp;
    }

    EncPutBits(stream, cbpy[tmp].value, cbpy[tmp].number);
    COMMENT("Cbpy");

    return;
}

/*------------------------------------------------------------------------------

	MvForward

------------------------------------------------------------------------------*/
void MvForward(stream_s * stream, mvData_s mvData, i32 vopFcode)
{
    i32 tmp;

    /* Motion Vector Data, make index to table B-12 */
    tmp = mvData.data + 32;
    EncPutBits(stream, mvd[tmp].value, mvd[tmp].number);
    COMMENT("Motion Vector Data");

    /* Residual Data */
    if((vopFcode != 1) && (mvData.data != 0))
    {
        EncPutBits(stream, mvData.residual, vopFcode - 1);
        COMMENT("Residual Data");
    }

    return;
}

/*------------------------------------------------------------------------------

	EncMbHeaderIntraDp

------------------------------------------------------------------------------*/
void EncMbHeaderIntraDp(stream_s * stream, stream_s * stream1, mb_s * mb,
                        true_e dcSeparately)
{
    i32 i;

    /* Rate control need quantization parameter of last coded macroblock */
    mb->qpLastCoded = mb->qp;
    ASSERT((mb->type == INTRA) || (mb->type == INTRA_Q));

    /* Current stream buffer is stream */
    /* Mcbpc */
    Mcbpc(stream, IVOP, mb->type, mb->zeroBlock);

    /* Dquant */
    if(mb->type == INTRA_Q)
    {
        EncPutBits(stream, dQuant[mb->dQuant].value, dQuant[mb->dQuant].number);
        COMMENT("Dquant");
    }

    /* Dc coefficient if coded separately */
    if(dcSeparately == ENCHW_YES)
    {
        for(i = 0; i < 6; i++)
        {
            EncDcCoefficient(stream, mb->dc[i], i);
        }
    }

    /* Current stream buffer is stream1 */
    /* AC-prediction flag */
    EncPutBits(stream1, mb->acPredFlag, 1);
    COMMENT("AC-prediction flag");

    /* Cbpy */
    Cbpy(stream1, mb->type, mb->zeroBlock);

    return;
}

/*------------------------------------------------------------------------------

	EncMbHeaderInterDp

------------------------------------------------------------------------------*/
void EncMbHeaderInterDp(stream_s * stream, stream_s * stream1, mb_s * mb,
                        true_e dcSeparately)
{
    type_e type = mb->type;
    dmv_s dmv;
    i32 i;

    /* Current stream buffer is stream */
    /* Not Coded */
    EncPutBits(stream, mb->notCoded, 1);
    COMMENT("Not Coded");

    if(mb->notCoded != 0)
    {
        return;
    }

    /* Rate control need quantization parameter of last coded macroblock */
    mb->qpLastCoded = mb->qp;

    /* mcbpc */
    Mcbpc(stream, PVOP, type, mb->zeroBlock);

    /* Differential Motion Vector Data */
    if((type == INTER) || (type == INTER_Q))
    {
        dmv = EncDifferentialMvEncoding(mb, 0);
        MvForward(stream, dmv.x, mb->fCode);
        MvForward(stream, dmv.y, mb->fCode);
    }
    else if(type == INTER4V)
    {
        for(i = 0; i < 4; i++)
        {
            dmv = EncDifferentialMvEncoding(mb, i);
            MvForward(stream, dmv.x, mb->fCode);
            MvForward(stream, dmv.y, mb->fCode);
        }
    }

    /* Current stream buffer is stream1 */
    /* AC-prediction flag */
    if((type == INTRA) || (type == INTRA_Q))
    {
        EncPutBits(stream1, mb->acPredFlag, 1);
        COMMENT("AC-prediction flag");
    }

    /* Cbpy */
    Cbpy(stream1, type, mb->zeroBlock);

    /* Dquant */
    if((type == INTER_Q) || (type == INTRA_Q))
    {
        EncPutBits(stream1, dQuant[mb->dQuant].value,
                   dQuant[mb->dQuant].number);
        COMMENT("Dquant");
    }

    /* Dc coefficient if coded separately */
    if((type == INTRA) || (type == INTRA_Q))
    {
        if(dcSeparately == ENCHW_YES)
        {
            for(i = 0; i < 6; i++)
            {
                EncDcCoefficient(stream1, mb->dc[i], i);
            }
        }
    }

    return;
}

/*------------------------------------------------------------------------------

   5.6  Function name: EncStuffing

------------------------------------------------------------------------------*/
void EncStuffing(stream_s * stream, vopType_e vopType)
{
    if(vopType == IVOP)
    {
        EncPutBits(stream, 1, 9);
        COMMENT("IVOP Stuffing");
    }
    else
    {
        EncPutBits(stream, 1, 10);
        COMMENT("PVOP Stuffing");
    }

    return;
}
