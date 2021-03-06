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
--  $RCSfile: EncVlc.c,v $
--  $Revision: 1.3 $
--
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/

#include "EncVlc.h"
#include "EncVlcTable.h"
#include "encasiccontroller.h"
#include "enccommon.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/
#undef TCOEF_INTRA
#undef TCOEF_INTER
#undef ESCAPE
#undef VLC
#undef VLC_LAST
/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/
static i32 Inter0Vlc(u32 run, u32 level);
static i32 Inter1Vlc(u32 run, u32 level);
static i32 Inter1Rvlc(u32 run, u32 level);
static i32 Inter0Rvlc(u32 run, u32 level);
static i32 InterLmax(u32 last, u32 run);
static i32 InterRmax(u32 last, u32 level);
static void InterEscape(stream_s *stream, u32 last, u32 run, i32 level);
static void EscapeType3(stream_s *stream, u32 last, u32 run, i32 level);
static void EscapeType5(stream_s *stream, u32 last, u32 run, i32 level);
static i32 Intra0Vlc(u32 run, u32 level);
static i32 Intra1Vlc(u32 run, u32 level);
static i32 Intra0Rvlc(u32 run, u32 level);
static i32 Intra1Rvlc(u32 run, u32 level);
static i32 IntraRmax(u32 last, u32 level);
static i32 IntraLmax(u32 last, u32 run);
static void IntraEscape(stream_s *stream, u32 last, u32 run, i32 level);

/*------------------------------------------------------------------------------

    Function name: EncVlcInter

    Variable Length Coding of one INTER macroblock. Processes the RLC words
    generated by ASIC and writes the corresponding VLC words to output stream.

    NOTE: The code between defines and undefs is exactly the same in
    EncVlcInter, EncVlcInterSvh and EncRvlcInter functions

    Input:
        stream      Pointer to stream structure where output is written
        mb          Pointer to macroblock data structure

-----------------------------------------------------------------------------*/
void EncVlcInter(stream_s *stream, mb_s *mb)
{
    i32 i, j;
    u32 run;
    i32 index;
    i32 level;
    u32 tmp;
    const i32 *rlcCount = mb->rlcCount;
    const u32 *rlc;

#define TCOEF_INTER EncPutBits(stream, tcoefInter[index].value+(level<0?1:0),\
        tcoefInter[index].number); COMMENT("INTER TCOEF")
#define ESCAPE InterEscape
#define VLC Inter0Vlc
#define VLC_LAST Inter1Vlc

    /* Code every block of macroblock */
    for (i = 0; i < 6; i++) {

        rlc = mb->rlc[i];

        /* DC+AC components except the last one */
        for (j = 1; j < rlcCount[i]; ++j)
        {
            /* VLC for each RLC word */
            tmp = *rlc++;
            level = ((i32)(tmp<<16))>>16;
            run = (tmp >> 16);
            ASSERT(run >= 0);

#ifdef TRACE_RLC
            EncTraceRlc((u32*)(rlc-2), i, run, level);
#endif

            /* Try first vlc table, then ESCAPE */
            index = VLC(run, ABS(level));

            if (index >= 0) {
                TCOEF_INTER;
            } else {
                ESCAPE(stream, 0, run, level);
            }
        }

        /* Last AC component */
        if (rlcCount[i] != 0)
        {
            /* VLC for each RLC word */
            tmp = *rlc++;
            level = ((i32)(tmp<<16))>>16;
            run = (tmp >> 16);
            ASSERT(run >= 0);

#ifdef TRACE_RLC
            EncTraceRlc((u32*)(rlc-2), i, run, level);
#endif

            /* Try first vlc table, then ESCAPE */
            index = VLC_LAST(run, ABS(level));
            
            if (index >= 0) {
                TCOEF_INTER;
            } else {
                ESCAPE(stream, 1, run, level);
            }
        }
    }

#undef TCOEF_INTER
#undef ESCAPE
#undef VLC
#undef VLC_LAST
}

/*------------------------------------------------------------------------------

    Function name: EncRvlcInter

    Reversible Variable Length Coding of one INTER macroblock.
    Processes the RLC words generated by ASIC and writes the
    corresponding VLC words to output stream.

    NOTE: The code between defines and undefs is exactly the same in
    EncVlcInter, EncVlcInterSvh and EncRvlcInter functions

    Input:
        stream      Pointer to stream structure where output is written
        mb          Pointer to macroblock data structure

-----------------------------------------------------------------------------*/
void EncRvlcInter(stream_s *stream, mb_s *mb)
{
    i32 i, j;
    u32 run;
    i32 index;
    i32 level;
    u32 tmp;
    const i32 *rlcCount = mb->rlcCount;
    const u32 *rlc;

#define TCOEF_INTER EncPutBits(stream, tcoefInterRvlc[index].value+(level<0?1:0),\
        tcoefInterRvlc[index].number); COMMENT("INTER RVLC TCOEF")
#define ESCAPE EscapeType5
#define VLC Inter0Rvlc
#define VLC_LAST Inter1Rvlc

    /* Code every block of macroblock */
    for (i = 0; i < 6; i++) {

        rlc = mb->rlc[i];

        /* DC+AC components except the last one */
        for (j = 1; j < rlcCount[i]; ++j)
        {
            /* VLC for each RLC word */
            tmp = *rlc++;
            level = ((i32)(tmp<<16))>>16;
            run = (tmp >> 16);
            ASSERT(run >= 0);

#ifdef TRACE_RLC
            EncTraceRlc((u32*)(rlc-2), i, run, level);
#endif

            /* Try first vlc table, then ESCAPE */
            index = VLC(run, ABS(level));

            if (index >= 0) {
                TCOEF_INTER;
            } else {
                ESCAPE(stream, 0, run, level);
            }
        }

        /* Last AC component */
        if (rlcCount[i] != 0)
        {
            /* VLC for each RLC word */
            tmp = *rlc++;
            level = ((i32)(tmp<<16))>>16;
            run = (tmp >> 16);
            ASSERT(run >= 0);

#ifdef TRACE_RLC
            EncTraceRlc((u32*)(rlc-2), i, run, level);
#endif

            /* Try first vlc table, then ESCAPE */
            index = VLC_LAST(run, ABS(level));
            
            if (index >= 0) {
                TCOEF_INTER;
            } else {
                ESCAPE(stream, 1, run, level);
            }
        }
    }

#undef TCOEF_INTER
#undef ESCAPE
#undef VLC
#undef VLC_LAST
}
/*------------------------------------------------------------------------------

    Function name: Inter0Vlc

------------------------------------------------------------------------------*/
i32 Inter0Vlc(u32 run, u32 level)
{
    if ((run < 27) && (level == 1)) {
        return run;
    } else if ((run == 0) && (level < 13)) {    /* index 27 - 37 */
        return 25+level;
    } else if ((run < 11) && (level == 2)) {    /* index 38 - 47 */
        return 37+run;
    } else if ((run < 7) && (level == 3)) {     /* index 48 - 53 */
        return 47+run;
    } else if ((run < 3) && (level == 4)) {     /* index 54 - 55 */
        return 53+run;
    } else if ((run == 1) && (level < 7)) {     /* index 56 - 57 */
        return 51+level;
    } else {                    /* escape */
        return -1;
    }
}

/*------------------------------------------------------------------------------

    Function name: Inter1Vlc

------------------------------------------------------------------------------*/
i32 Inter1Vlc(u32 run, u32 level)
{
    if ((run < 41) && (level == 1)) {       /* index 58 - 98 */
        return 58+run;
    } else if ((run == 0) && (level < 4)) {     /* index 99 - 100 */
        return 97+level;
    } else if ((run == 1) && (level == 2)) {    /* index 101 */
        return 101;
    } else {                    /* escape */
        return -1;
    }
}

/*------------------------------------------------------------------------------

    Function name: Inter0Rvlc

------------------------------------------------------------------------------*/
i32 Inter0Rvlc(u32 run, u32 level)
{
    if ((run < 39) && (level == 1)) {       /* index 0 - 38 */
        return run;
    } else if ((run < 18) && (level == 2)) {    /* index 39 - 56 */
        return 39+run;
    } else if ((run == 0) && (level < 20)) {    /* index 57 - 73 */
        return 54+level;
    } else if ((run < 10) && (level == 3)) {    /* index 74 - 82 */
        return 73+run;
    } else if ((run < 8) && (level == 4))  {    /* index 83 - 89 */
        return 82+run;
    } else if ((run == 1) && (level < 11)) {    /* index 90 - 95 */
        return 85+level;
    } else if ((run == 2) && (level < 8))  {    /* index 96 - 98 */
        return 91+level;
    } else if ((run == 3) && (level < 8))  {    /* index 99 - 101 */
        return 94+level;
    } else if ((run == 4) && (level == 5)) {    /* index 102 */
        return 102;
    } else {                    /* escape */
        return -1;
    }
}

/*------------------------------------------------------------------------------

    Function name: Inter1Rvlc

------------------------------------------------------------------------------*/
i32 Inter1Rvlc(u32 run, u32 level)
{
    if ((run < 45) && (level == 1)) {       /* index 103 - 147 */
        return 103+run;
    } else if ((run < 14) && (level == 2)) {    /* index 148 - 161 */
        return 148+run;
    } else if ((run < 3) && (level == 3))  {    /* index 162 - 164 */
        return 162+run;
    } else if ((run < 2) && (level == 4))  {    /* index 165 - 166 */
        return 165+run;
    } else if ((run < 2) && (level == 5))  {    /* index 167 - 168 */
        return 167+run;
    } else {                    /* escape */
        return -1;
    }
}

/*------------------------------------------------------------------------------

    Function name: InterLmax

------------------------------------------------------------------------------*/
i32 InterLmax(u32 last, u32 run)
{
    static const i32 lmax0[27] = {
        12,6,4,3,3,3,3,2,2,2,2,1,1,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1};

    if (last == 0) {
        if (run < 27) {
            return lmax0[run];
        } else {
            return -1;
        }
    } else {
        if (run == 0) {
            return 3;
        } else if (run == 1) {
            return 2;
        } else if ((run > 1) && (run < 41)) {
            return 1;
        } else {
            return -1;
        }
    }
}

/*------------------------------------------------------------------------------

    Function name: InterRmax

------------------------------------------------------------------------------*/
i32 InterRmax(u32 last, u32 level)
{
    static const i32 rmax0[13] = {-1,26,10,6,2,1,1,0,0,0,0,0,0};
    static const i32 rmax1[4]  = {-1,40,1,0};

    if (last == 0) {
        if (level < 13) {
            return rmax0[level];
        } else {
            return -1;
        }
    } else {
        if (level < 4) {
            return rmax1[level];
        } else {
            return -1;
        }
    }
}

/*------------------------------------------------------------------------------

    Function name: InterEscape

------------------------------------------------------------------------------*/
void InterEscape(stream_s *stream, u32 last, u32 run, i32 level)
{
    i32 lmax;
    i32 rmax;
    i32 index;

    /* Escape type 1 */
    lmax = InterLmax(last, run);
    if (lmax != -1) {
        /* Find Inter TCOEF */
        if (last == 0) {
            index = Inter0Vlc(run, ABS(level-(SIGN(level)*lmax)));
        } else {
            index = Inter1Vlc(run, ABS(level-(SIGN(level)*lmax)));
        }
        if (index != -1) {
            EncPutBits(stream, 6, 8);
            COMMENT("ESCAPE INTER type 1");
            EncPutBits(stream,
                tcoefInter[index].value+(level<0?1:0),
                    tcoefInter[index].number);
            COMMENT("INTER TCOEF");
            return;
        }
    }

    /* Escape type 2 */
    rmax = InterRmax(last, ABS(level));
    if (rmax != -1) {
        ASSERT(((i32)run-rmax-1) >= 0);
        if (last == 0) {
            index = Inter0Vlc(run-rmax-1, ABS(level));
        } else {
            index = Inter1Vlc(run-rmax-1, ABS(level));
        }
        if (index != -1) {
            EncPutBits(stream, 14, 9);
            COMMENT("ESCAPE INTER type 2");
            EncPutBits(stream,
                tcoefInter[index].value+(level<0?1:0),
                    tcoefInter[index].number);
            COMMENT("INTER TCOEF");
            return;
        }
    }

    /* Escape type 3*/
    EscapeType3(stream, last, run, level);
}

/*------------------------------------------------------------------------------

    Function name: EscapeType3

------------------------------------------------------------------------------*/
void EscapeType3(stream_s *stream, u32 last, u32 run, i32 level)
{
    /* Escape 3 */
    EncPutBits(stream, 15, 9);
    COMMENT("ESCAPE type 3");

    /* Last */
    EncPutBits(stream, last, 1);
    COMMENT("Last");

    /* Run */
    EncPutBits(stream, run, 6);
    COMMENT("Run");

    /* Marker Bit */
    EncPutBits(stream, 1, 1);
    COMMENT("Marker Bit");

    /* level */
    /* See Table B-18, 2 complement number give right code, but */
    /* PutBits function need unsigned int, so we mask 12 lsb    */
    EncPutBits(stream, (u32) (level & 4095), 12);
    COMMENT("Level");

    /* Marker Bit */
    EncPutBits(stream, 1, 1);
    COMMENT("Marker Bit");
}

/*------------------------------------------------------------------------------

    Function name: EscapeType5

------------------------------------------------------------------------------*/
void EscapeType5(stream_s *stream, u32 last, u32 run, i32 level)
{
    /* Escape 5 */
    EncPutBits(stream, 1, 5);
    COMMENT("ESCAPE type 5");

    /* LAST */
    EncPutBits(stream, last, 1);
    COMMENT("LAST");

    /* RUN */
    EncPutBits(stream, run, 6);
    COMMENT("RUN");

    /* Marker Bit */
    EncPutBits(stream, 1, 1);
    COMMENT("Marker Bit");

    /* LEVEL */
    EncPutBits(stream, ABS(level), 11);
    COMMENT("LEVEL");

    /* Marker Bit */
    EncPutBits(stream, 1, 1);
    COMMENT("Marker Bit");

    /* ESCAPE */
    if (level > 0) {
        EncPutBits(stream, 0, 5);
    } else {
        EncPutBits(stream, 1, 5);
    }
    COMMENT("ESCAPE");
}


/*------------------------------------------------------------------------------

    Function name: EncVlcIntra

    Variable Length Coding of one INTRA macroblock. Processes the RLC words
    generated by ASIC and writes the corresponding VLC words to output stream.

    NOTE: The code between defines and undefs is exactly the same in
    EncVlcIntra and EncRvlcIntra functions

    Input:
        stream      Pointer to stream structure where output is written
        mb          Pointer to macroblock data structure
        dcCoding    DC component coding type

------------------------------------------------------------------------------*/
void EncVlcIntra(stream_s *stream, mb_s *mb, dcVlc_e dcCoding)
{
    i32 i, j;
    u32 run, runTmp;
    u32 last;
    i32 index;
    i32 level;
    u32 tmp;
    const i32 *rlcCount = mb->rlcCount;
    const i32 *dc = mb->dc;
    const u32 *rlc;

#define TCOEF_INTRA EncPutBits(stream, tcoefIntra[index].value+(level<0?1:0),\
        tcoefIntra[index].number); COMMENT("INTRA TCOEF")
#define ESCAPE IntraEscape
#define VLC Intra0Vlc
#define VLC_LAST Intra1Vlc

    /* Code every block of macroblock */
    for(i = 0; i < 6; i++) {

        run = 0;

        if (dcCoding == DC_VLC_INTRA_DC) {
            /* DC coded using DC Intra VLC */
            EncDcCoefficient(stream, dc[i], i);
        } else if (dcCoding == DC_VLC_INTRA_AC) {
            /* DC coded using AC Intra VLC  */
            level = dc[i];

            if (level != 0) {
                last = (rlcCount[i] == 0);    /* If zero RLC words DC is last */

                /* Try first vlc table, then ESCAPE */
                if (last) {
                    /* DC is the first and last */
                    index = VLC_LAST(run, ABS(level));
                } else {
                    index = VLC(run, ABS(level));
                }

                if (index >= 0) {
                    TCOEF_INTRA;
                } else {
                    ESCAPE(stream, last, run, level);
                }
            } else {
                run = 1;
            }
        }

        rlc = mb->rlc[i];

        /* AC components except the last one */
        for (j = 1; j < rlcCount[i]; ++j)
        {
            /* VLC for each RLC word */
            tmp = *rlc++;
            level = ((i32)(tmp<<16))>>16;
            runTmp = (tmp >> 16);
            ASSERT(run >= 0);

#ifdef TRACE_RLC
            EncTraceRlc((u32*)(rlc-2), i, runTmp, level);
#endif
            run += runTmp;

            /* Try first vlc table, then ESCAPE */
            index = VLC(run, ABS(level));

            if (index >= 0) {
                TCOEF_INTRA;
            } else {
                ESCAPE(stream, 0, run, level);
            }

            run = 0;
        }

        /* Last AC component */
        if (rlcCount[i] != 0)
        {
            /* VLC for each RLC word */
            tmp = *rlc++;
            level = ((i32)(tmp<<16))>>16;
            runTmp = (tmp >> 16);
            ASSERT(run >= 0);

#ifdef TRACE_RLC
            EncTraceRlc((u32*)(rlc-2), i, runTmp, level);
#endif
            run += runTmp;

            /* Try first vlc table, then ESCAPE */
            index = VLC_LAST(run, ABS(level));
            
            if (index >= 0) {
                TCOEF_INTRA;
            } else {
                ESCAPE(stream, 1, run, level);
            }
        }
    }

#undef TCOEF_INTRA
#undef ESCAPE
#undef VLC
#undef VLC_LAST
}

/*------------------------------------------------------------------------------

    Function name: EncRvlcIntra

    Reversible Variable Length Coding of one INTRA macroblock.
    Processes the RLC words generated by ASIC and writes the
    corresponding VLC words to output stream.

    Input:
        stream      Pointer to stream structure where output is written
        mb          Pointer to macroblock data structure
        dcCoding    DC component coding type

------------------------------------------------------------------------------*/
void EncRvlcIntra(stream_s *stream, mb_s *mb, dcVlc_e dcCoding)
{
    i32 i, j;
    u32 run, runTmp;
    u32 last;
    i32 index;
    i32 level;
    u32 tmp;
    const i32 *rlcCount = mb->rlcCount;
    const i32 *dc = mb->dc;
    const u32 *rlc;

#define TCOEF_INTRA EncPutBits(stream, tcoefIntraRvlc[index].value+(level<0?1:0),\
        tcoefIntraRvlc[index].number); COMMENT("INTRA RVLC TCOEF")
#define ESCAPE EscapeType5
#define VLC Intra0Rvlc
#define VLC_LAST Intra1Rvlc

    /* Code every block of macroblock */
    for(i = 0; i < 6; i++) {

        run = 0;

        if (dcCoding == DC_VLC_INTRA_DC) {
            /* DC coded using DC Intra VLC */
            EncDcCoefficient(stream, dc[i], i);
        } else if (dcCoding == DC_VLC_INTRA_AC) {
            /* DC coded using AC Intra VLC  */
            level = dc[i];

            if (level != 0) {
                last = (rlcCount[i] == 0);    /* If zero RLC words DC is last */

                /* Try first vlc table, then ESCAPE */
                if (last) {
                    /* DC is the first and last */
                    index = VLC_LAST(run, ABS(level));
                } else {
                    index = VLC(run, ABS(level));
                }

                if (index >= 0) {
                    TCOEF_INTRA;
                } else {
                    ESCAPE(stream, last, run, level);
                }
            } else {
                run = 1;
            }
        }

        rlc = mb->rlc[i];

        /* AC components except the last one */
        for (j = 1; j < rlcCount[i]; ++j)
        {
            /* VLC for each RLC word */
            tmp = *rlc++;
            level = ((i32)(tmp<<16))>>16;
            runTmp = (tmp >> 16);
            ASSERT(run >= 0);

#ifdef TRACE_RLC
            EncTraceRlc((u32*)(rlc-2), i, runTmp, level);
#endif
            run += runTmp;

            /* Try first vlc table, then ESCAPE */
            index = VLC(run, ABS(level));

            if (index >= 0) {
                TCOEF_INTRA;
            } else {
                ESCAPE(stream, 0, run, level);
            }

            run = 0;
        }

        /* Last AC component */
        if (rlcCount[i] != 0)
        {
            /* VLC for each RLC word */
            tmp = *rlc++;
            level = ((i32)(tmp<<16))>>16;
            runTmp = (tmp >> 16);
            ASSERT(run >= 0);

#ifdef TRACE_RLC
            EncTraceRlc((u32*)(rlc-2), i, runTmp, level);
#endif
            run += runTmp;

            /* Try first vlc table, then ESCAPE */
            index = VLC_LAST(run, ABS(level));
            
            if (index >= 0) {
                TCOEF_INTRA;
            } else {
                ESCAPE(stream, 1, run, level);
            }
        }
    }

#undef TCOEF_INTRA
#undef ESCAPE
#undef VLC
#undef VLC_LAST
}

/*------------------------------------------------------------------------------

    Function name: Intra0Vlc

------------------------------------------------------------------------------*/
i32 Intra0Vlc(u32 run, u32 level)
{
    if ((run == 0) && (level < 28)) {       /* index 0 - 26 */
        return level-1;
    } else if ((run < 15) && (level == 1)) {    /* index 27 - 40 */
        return 26+run;
    } else if ((run < 10) && (level == 2)){     /* index 41 - 49 */
        return 40+run;
    } else if ((run < 8) && (level == 3)) {     /* index 50 -  56 */
        return 49+run;
    } else if ((run == 1) && (level < 11)) {    /* index 57 -  63 */
        return 53+level;
    } else if ((run < 4) && (level == 4)) {     /* index 64 -  65 */
        return 62+run;
    } else if ((run == 2) && (level == 5)) {    /* index 66 */
        return 66;
    } else {                    /* escape */
        return -1;
    }
}

/*------------------------------------------------------------------------------

    Function name: Intra1Vlc

------------------------------------------------------------------------------*/
i32 Intra1Vlc(u32 run, u32 level)
{
    if ((run < 21) && (level == 1)) {       /* index 67 - 87 */
        return 67+run;
    } else if ((run < 7) && (level == 2)) {     /* index 88 - 94 */
        return 88+run;
    } else if ((run == 0) && (level < 9)) {     /* index 95 - 100 */
        return 92+level;
    } else if ((run == 1) && (level == 3)) {    /* index 101 */
        return 101;
    } else {                    /* escape */
        return -1;
    }
}

/*------------------------------------------------------------------------------

    Function name: Intra0Rvlc

------------------------------------------------------------------------------*/
i32 Intra0Rvlc(u32 run, u32 level)
{

    if ((run == 0) && (level < 28)) {       /* index 0 - 26 */
        return level-1;
    } else if ((run < 20) && (level == 1)) {    /* index 27 - 45 */
        return 26+run;
    } else if ((run < 13) && (level == 2)) {    /* index 46 - 57 */
        return 45+run;
    } else if ((run == 1) && (level < 14)) {    /* index 58 - 68 */
        return 55+level;
    } else if ((run == 2) && (level < 12)) {    /* index 69 - 77 */
        return (66 + level);
    } else if ((run == 3) && (level < 10)) {    /* index 78 - 84 */
        return 75+level;
    } else if ((run < 10) && (level == 3)) {    /* index 85 - 90 */
        return 81+run;
    } else if ((run < 10) && (level == 4)) {    /* index 91 - 96 */
        return 87+run;
    } else if ((run < 8) && (level == 5))  {    /* index 97 - 100 */
        return 93+run;
    } else if ((run < 6) && (level == 6))  {    /* index 101 - 102 */
        return 97+run;
    } else {                    /* escape */
        return -1;
    }
}

/*------------------------------------------------------------------------------

    Function name: Intra1Rvlc

------------------------------------------------------------------------------*/
i32 Intra1Rvlc(u32 run, u32 level)
{
    if ((run < 45) && (level == 1)) {       /* index 103 - 147 */
        return 103 + run;
    } else if ((run < 14) && (level == 2)) {    /* index 148 - 161 */
        return 148+run;
    } else if ((run < 3) && (level == 3 )) {    /* index 162 - 164 */
        return 162+run;
    } else if ((run < 2) && (level == 4 )) {    /* index 165 - 166 */
        return 165+run;
    } else if ((run < 2) && (level == 5 )) {    /* index 167 - 168 */
        return 167+run;
    } else {                    /* escape */
        return -1;
    }
}

/*------------------------------------------------------------------------------

    Function name: IntraLmax

------------------------------------------------------------------------------*/
i32 IntraLmax(u32 last, u32 run)
{
    static const i32 lmax0[15] = {27,10,5,4,3,3,3,3,2,2,1,1,1,1,1};
    static const i32 lmax1[21] = {8,3,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1};

    if (last == 0) {
        if (run < 15) {
            return lmax0[run];
        } else {
            return -1;
        }
    } else {
        if (run < 21) {
            return lmax1[run];
        } else {
            return -1;
        }
    }
}

/*------------------------------------------------------------------------------

    Function name: IntraRmax

------------------------------------------------------------------------------*/
i32 IntraRmax(u32 last, u32 level)
{
    static const i32 rmax1[9]  = {-1,20,6,1,0,0,0,0,0};
    static const i32 rmax0[28] = {-1,14,9,7,3,2,1,1,1,1,1,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

    if (last == 0) {
        if (level < 28) {
            return rmax0[level];
        } else {
            return -1;
        }
    } else {
        if (level < 9) {
            return rmax1[level];
        } else {
            return -1;
        }
    }
}

/*------------------------------------------------------------------------------

    Function name: IntraEscape

------------------------------------------------------------------------------*/
void IntraEscape(stream_s *stream, u32 last, u32 run, i32 level)
{
    i32 lmax;
    i32 rmax;
    i32 index;

    /* Try next Escape type 1 */
    lmax = IntraLmax(last, run);
    if (lmax != -1) {
        /* Find Intra TCOEF */
        if (last == 0) {
            index = Intra0Vlc(run, ABS(level-(SIGN(level)*lmax)));
        } else {
            index = Intra1Vlc(run, ABS(level-(SIGN(level)*lmax)));
        }
        if (index != -1) {
            EncPutBits(stream, 6, 8);
            COMMENT("ESCAPE INTRA type 1");
            EncPutBits(stream,
                tcoefIntra[index].value+(level<0?1:0),
                    tcoefIntra[index].number);
            COMMENT("INTRA TCOEF");
            return;
        }
    }

    /* Escape type 2 */
    /* Calculate RMAX, see Table B-21 */
    rmax = IntraRmax(last, ABS(level));
    if (rmax != -1) {
        /* Find Intra TCOEF */
        ASSERT((i32)run-rmax-1 >= 0);
        if (last == 0) {
            index = Intra0Vlc(run-rmax-1, ABS(level));
        } else {
            index = Intra1Vlc(run-rmax-1, ABS(level));
        }
        if (index != -1) {
            EncPutBits(stream, 14, 9);
            COMMENT("ESCAPE INTRA type 2");
            EncPutBits(stream,
                    tcoefIntra[index].value+(level<0?1:0),
                    tcoefIntra[index].number);
            COMMENT("INTRA TCOEF");
            return;
        }
    }

    /* Escape type 3*/
    EscapeType3(stream, last, run, level);
}

/*------------------------------------------------------------------------------

    Function name: EncDcCoefficient

------------------------------------------------------------------------------*/
void EncDcCoefficient(stream_s *stream, i32 dcCoeff, i32 block)
{
    u32 dcDiff;
    u32 size = 0;
    u32 min  = 0;
    u32 max  = 0;
    u32 tmp;

    /* Calculate size, see Table B-15 */
    dcDiff = ABS(dcCoeff);
    while ((dcDiff > max) || (dcDiff < min)) {
        min = 1<<size;
        max = (1<<(size+1))-1;
        size++;
    }

    /* luminance block */
    if (block < 4) {
        EncPutBits(stream, dctDcSizeLuminance[size].value,
                dctDcSizeLuminance[size].number);
        COMMENT("Dct Dc Size Luminance");
        if (size > 0) {
            if (dcCoeff > 0) {
                tmp = (u32)(dcDiff&((1<<size)-1));
                EncPutBits(stream, tmp, size);
                COMMENT("Additional code");
            } else {
                tmp = (u32)((~dcDiff) & ((1<<size)-1));
                EncPutBits(stream, tmp, size);
                COMMENT("Additional code");
            }
        }
        if (size > 8) {
            /* Marker Bit */
            EncPutBits(stream, 1, 1);
            COMMENT("Marker Bit");
        }
    /* Chrominance block */
    } else {
        EncPutBits(stream, DctDcSizeChrominance[size].value,
                DctDcSizeChrominance[size].number);
        COMMENT("Dct Dc Size Chrominance");
        if (size > 0) {
            if (dcCoeff > 0) {
                tmp = (u32)(dcDiff&((1<<size)-1));
                EncPutBits(stream, tmp, size);
                COMMENT("Additional code");
            } else {
                tmp = (u32)((~dcDiff) & ((1<<size)-1));
                EncPutBits(stream, tmp, size);
                COMMENT("Additional code");
            }
        }
        if (size > 8) {
            /* Marker Bit */
            EncPutBits(stream, 1, 1);
            COMMENT("Marker Bit");
        }
    }
}


