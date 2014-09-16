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
-
-  Description : ...
-
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    Includes
------------------------------------------------------------------------------*/

#include "basetype.h"
#include "dwl.h"
#include "vp8hwd_error.h"
#include "vp8hwd_debug.h"

/*------------------------------------------------------------------------------
    Module defines
------------------------------------------------------------------------------*/

#define EXTRACT_BITS(x,cnt,pos) \
        (((x) & (((1<<(cnt))-1)<<(pos)))>>(pos))
        
#define MV_HOR(p) (((i32)EXTRACT_BITS((p), 14, 18)<<18)>>18)
#define MV_VER(p) (((i32)EXTRACT_BITS((p), 13, 5)<<19)>>19)
#define REF_PIC(p) ((p) & 0x7)

#define MAX3(a,b,c) ((b) > (a) ? ((c) > (b) ? 2 : 1) : ((c) > (a) ? 2 : 0))

static const u32 refMap[] = {};
/*------------------------------------------------------------------------------
    Functions
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    vp8hwdInitEc

        Allocate memory for error concealment, buffers shared by SW/HW
        are allocated elsewhere.
------------------------------------------------------------------------------*/
u32 vp8hwdInitEc(vp8ec_t *ec, u32 w, u32 h, u32 numMvsPerMb)
{

    ASSERT(ec);

    w /= 16;
    h /= 16;
    ec->width = w;
    ec->height = h;
    ec->numMvsPerMb = numMvsPerMb;
    ec->mvs = DWLmalloc(w * h * numMvsPerMb * sizeof(ecMv_t));
    if (ec->mvs == NULL)
        return HANTRO_NOK;

    return HANTRO_OK;
}

/*------------------------------------------------------------------------------
    vp8hwdReleaseEc
------------------------------------------------------------------------------*/
void vp8hwdReleaseEc(vp8ec_t *ec)
{

    ASSERT(ec);

    if (ec->mvs)
        DWLfree(ec->mvs);

}

/*------------------------------------------------------------------------------
    updateMv

        Add contribution of extrapolated mv to internal struct if inside
        picture area. Used reference picture determines which component is
        updated, weigth used in summing is w
------------------------------------------------------------------------------*/
void updateMv( vp8ec_t *ec, i32 x, i32 y, i32 hor, i32 ver, u32 ref, i32 w)
{

    u32 b;

    ASSERT(ec);

    if (x >= 0 && x < ec->width*4 &&
        y >= 0 && y < ec->height*4)
    {
        /* mbNum */
        b = (y & ~0x3) * ec->width * 4 + (x & ~0x3) * 4;
        /* mv/block within mb */
        b += (y & 0x3) * 4 + (x & 0x3);

        ec->mvs[b].totWeight[ref] += w;
        ec->mvs[b].totMv[ref].hor += w * hor;
        ec->mvs[b].totMv[ref].ver += w * ver;
    }

}

/*------------------------------------------------------------------------------
    getNeighbors

        Determine neighbor mvs of current mb. Return index of ref pic
        referenced most in the neighborhood
------------------------------------------------------------------------------*/
u32 getNeighbors(u32 *p, mv_t *nmv, u32 *nref, u32 row, u32 col,
    u32 height, u32 width)
{
    u32 i = 0, j;
    u32 refCnt[8] = {0, 0, 0, 0, 0, 0, 0, 0};

    DWLmemset(nmv, 0, sizeof(mv_t)*20);
    /* initialize to anything else but 0 */
    DWLmemset(nref, 0xFF, sizeof(u32)*20);

    /* above left */
    if (row)
    {
        if (col)
        {
            nref[i] = REF_PIC(p[-16*width-1]);
            nmv[i].hor = MV_HOR(p[-16*width-1]);
            nmv[i].ver = MV_VER(p[-16*width-1]);
            refCnt[nref[i]]++;
        }
        i++;

        for (j = 0; j < 4; j++, i++)
        {
            nref[i] = REF_PIC(p[-16*width+12+j]);
            nmv[i].hor = MV_HOR(p[-16*width+12+j]);
            nmv[i].ver = MV_VER(p[-16*width+12+j]);
        }
        refCnt[nref[i-1]] += 4;
    }
    else
        i += 5;

    if (col < width - 1)
    {
        if (row)
        {
            nref[i] = REF_PIC(p[16-16*width]);
            nmv[i].hor = MV_HOR(p[16-16*width]);
            nmv[i].ver = MV_VER(p[16-16*width]);
            refCnt[nref[i]]++;
        }
        i++;
        for (j = 0; j < 16; j += 4, i++)
        {
            nref[i] = REF_PIC(p[16+j]);
            nmv[i].hor = MV_HOR(p[16+j]);
            nmv[i].ver = MV_VER(p[16+j]);
        }
        refCnt[nref[i-1]] += 4;
    }
    else
        i += 5;

    if (row < height - 1)
    {
        if (col < width - 1)
        {
            nref[i] = REF_PIC(p[16+16*width]);
            nmv[i].hor = MV_HOR(p[16+16*width]);
            nmv[i].ver = MV_VER(p[16+16*width]);
            refCnt[nref[i]]++;
        }
        i++;
        for (j = 0; j < 4; j++, i++)
        {
            nref[i] = REF_PIC(p[16*width+j]);
            nmv[i].hor = MV_HOR(p[16*width+j]);
            nmv[i].ver = MV_VER(p[16*width+j]);
        }
        refCnt[nref[i-1]] += 4;
    }
    else
        i += 5;

    if (col)
    {
        if (row < height - 1)
        {
            nref[i] = REF_PIC(p[-13+16*width]);
            nmv[i].hor = MV_HOR(p[-13+16*width]);
            nmv[i].ver = MV_VER(p[-13+16*width]);
        }
        refCnt[nref[i]]++;
        i++;
        for (j = 0; j < 16; j+=4, i++)
        {
            nref[i] = REF_PIC(p[-13+j]);
            nmv[i].hor = MV_HOR(p[-13+j]);
            nmv[i].ver = MV_VER(p[-13+j]);
        }
        refCnt[nref[i-1]] += 4;
    }

    /* 0=last ref, 4=golden, 5=altref */
    return refCnt[0] >= refCnt[4] ? (refCnt[0] >= refCnt[5] ? 0 : 5) :
                                    (refCnt[4] >= refCnt[5] ? 4 : 5);

}

/*------------------------------------------------------------------------------
    vp8hwdEc

        Main error concealment function.
        
        Implements error concealment algorithm close to algorithm described
        in "Error Concealment Algorithm for VP8". (don't know how to properly
        refer to particular doc in Google Docs?).
        
        First loop goes through all the motion vectors of the reference picture
        and extrapolates motion to current picture (stored in internal
        structures). Latter loops go through motion vectors of the current
        picture that need to be concealed (first intra macroblocks whose
        residual is lost, then completely lost macroblocks).

        Function shall be generalized to compute numMvsPerMb motion vectors
        for each mb of the current picture, currently assumes that all
        16 mvs are computed (probably too CPU intensive with high resolution
        video).
        
------------------------------------------------------------------------------*/
void vp8hwdEc( vp8ec_t *ec, u32 *pRef, u32 *pOut, u32 startMb, u32 all)
{

    u32 i, j, tmp;
    u32 numMbs,numMvs;
    i32 hor, ver;
    i32 mbX, mbY, x, y;
    i32 wx, wy;
    u32 ref;
    u32 startAll;
    u32 *p = pRef;
    mv_t nmv[20];
    u32 nref[20];

    ASSERT(ec);
    ASSERT(pRef);
    ASSERT(pOut);
    ASSERT(ec->numMvsPerMb == 16); /* assumed here and there, fixed later */

    numMbs = ec->width * ec->height;
    numMvs = numMbs * ec->numMvsPerMb;

    /* motion vector extrapolation if part (or all) of control partition lost */
    if (all)
    {

        if (pRef == pOut)
            return;

        DWLmemset(ec->mvs, 0, numMvs * sizeof(ecMv_t));

        /* determine overlaps from ref mvs */
        mbX = mbY = x = y = 0;
        for (i = 0; i < numMbs; i++)
        {
            ref = REF_PIC(*p);
            /* only consider previous ref (index 0) */
            if (ref == 0)
            {
                for (j = 0; j < ec->numMvsPerMb; j++, p++)
                {
                    hor = MV_HOR(*p);
                    ver = MV_VER(*p);
                    /* (x,y) indicates coordinates of the top-left corner of
                     * the block to which mv points, 4pel units */
                    /* TODO: lut for x/y offsets, depends on numMvsPerMb,
                     * assumed
                     * 16 initially */
                    x = mbX*4 + (j&0x3) + ((-hor) >> 4);
                    y = mbY*4 + (j>>2)  + ((-ver) >> 4);

                    /* offset within 4x4 block, used to determine overlap
                     * which is used as weight in averaging */
                    wx = ((-hor) >> 2) & 0x3;
                    wy = ((-ver) >> 2) & 0x3;

                    /* update mv where top/left corner of the extrapolated
                     * block hits */
                    updateMv(ec, x    , y    , hor, ver, ref, (4-wx)*(4-wy));
                    /* if not aligned -> update neighbors on right and bottom */
                    if (wx || wy)
                    {
                        updateMv(ec, x + 1, y    , hor, ver, ref, (  wx)*(4-wy));
                        updateMv(ec, x    , y + 1, hor, ver, ref, (4-wx)*(  wy));
                        updateMv(ec, x + 1, y + 1, hor, ver, ref, (  wx)*(  wy));
                    }
                }
            }
            else
                p += ec->numMvsPerMb;
            mbX++;
            if (mbX == ec->width)
            {
                mbX = 0;
                mbY++;
            }
        }
    }

    /* determine final concealment mv and write to shared mem */
    /* if all is set -> error was found in control partition and we assume
     * that all residual was lost -> conceal all intra mbs and everything
     * starting from startMb */
    if (all)
    {
        startAll = startMb * ec->numMvsPerMb;
        i = 0;
        mbY = mbX = 0;
    }
    else
    {
        startAll = numMvs;
        i = startMb * ec->numMvsPerMb;
        mbY = startMb / ec->width;
        mbX = startMb - mbY * ec->width;
    }
    p = pOut + i;

    /* all intra mbs until startAll */
    for (; i < startAll; i+=16, p+=16)
    {
        /* intra is marked with refpic index 1 */
        if (REF_PIC(p[0]) == 1)
        {
            i32 w = 0;
            ref = getNeighbors(p, nmv, nref, mbY, mbX,
                startAll/16 > ec->width*2 ? startAll/16/ec->width-1 : 1,
                ec->width);
            /* sum of all neighbor mvs referencing the most referenced one */
            hor = ver = 0;
            for (j = 0; j < 20; j++)
            {
                if (nref[j] == ref)
                {
                    w++;
                    hor += nmv[j].hor;
                    ver += nmv[j].ver;
                }
            }
            if (w)
            {
                hor /= w;
                ver /= w;
            }
            tmp = ((hor & 0x3FFF) << 18) | ((ver & 0x1FFF) <<  5) | (ref);
            /* same vector for all mvs of concealed intra mb */
            for (j = 0; j < 16; j++)
                p[j] = tmp;
        }
        mbX++;
        if (mbX == ec->width)
        {
            mbX = 0;
            mbY++;
        }
    }
    /* all mbs starting from startAll */
    for (; i < numMvs; i++, p++)
    {
        /* always choose last ref, could probably drop all the computation
         * on golden and alt */
        ref = 0;

        /* any overlap in current block? */
        if ((x = ec->mvs[i].totWeight[ref]) != 0)
        {
            hor = ec->mvs[i].totMv[ref].hor / x;
            ver = ec->mvs[i].totMv[ref].ver / x;
        }
        else if (1)
            hor = ver = 0;
#if 0
            else /* no overlap -> average of left/above neighbors */
            {
                u32 b = i & 0xF; /* mv within current mb */
                u32 w = 0;
                u32 ref1 = 0, ref2 = 0;
                hor = ver = 0;
                if (i) /* either left or above exists */
                {
                    if (mbX || (b & 0x3))
                    {
                        /* left in current mb */
                        if (b&0x3)
                        {
                            ref1 = p[-1] & 0x3;
                            hor += MV_HOR(p[-1]);
                            ver += MV_VER(p[-1]);
                            w++;
                        }
                        /* left in previous mb, used if non-intra */
                        else if (p[-13] & 0x3)
                        {
                            ref1 = p[-13] & 0x3;
                            hor += MV_HOR(p[-13]);
                            ver += MV_VER(p[-13]);
                            w++;
                        }
                    }
                    if (mbY || b >= 4)
                    {
                        /* above in current mb */
                        if (b >= 4)
                        {
                            ref2 = p[-4] & 0x3;
                            if (!ref1 || ref2 == ref1)
                            {
                                hor += MV_HOR(p[-4]);
                                ver += MV_VER(p[-4]);
                                w++;
                            }
                            else if (ref2 == 1) /* favor last ref */
                            {
                                ref1 = 0;
                                hor = MV_HOR(p[-4]);
                                ver = MV_VER(p[-4]);
                            }
                        }
                        /* above in mb above, used if non-intra */
                        else if (p[12-16*ec->width] & 0x3)
                        {
                            ref2 = p[12-16*ec->width] & 0x3;
                            if (!ref1 || ref2 == ref1)
                            {
                                hor += MV_HOR(p[12-16*ec->width]);
                                ver += MV_VER(p[12-16*ec->width]);
                                w++;
                            }
                            else if (ref2 == 1)
                            {
                                ref1 = 0;
                                hor = MV_HOR(p[12-16*ec->width]);
                                ver = MV_VER(p[12-16*ec->width]);
                            }
                        }
                    }
                    /* both above and left exist -> average */
                    if (w == 2)
                    {
                        hor /= 2;
                        ver /= 2;
                    }
                    ref = ref1 ? ref1 : ref2;
                    ref--;
                }
            }
#endif

        tmp = ((hor & 0x3FFF) << 18) | ((ver & 0x1FFF) <<  5) | ref;

        *p = tmp;

        if ((i & 0xF) == 0xF)
        {
            mbX++;
            if (mbX == ec->width)
            {
                mbX = 0;
                mbY++;
            }
        }
    }

}
