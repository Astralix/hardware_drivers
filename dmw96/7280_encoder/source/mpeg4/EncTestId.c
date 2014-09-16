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
--  Abstract : Encoder setup according to a test vector
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "EncTestId.h"

#include "encbuffering.h"

#include <stdio.h>

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/
static void EncFrameQuantizationTest(instance_s *inst);
static void EncMbQuantizationTest(instance_s *inst);
static void EncMbPerInterruptTest(instance_s *inst);
static void EncRlcBufferFullTest(instance_s *inst);
static void EncRandomInterruptTest(instance_s *inst);
static void EncVpSizeTest(instance_s *inst);


/*------------------------------------------------------------------------------

    TestId defines a test configuration for the encoder. If the encoder control
    software is compiled with INTERNAL_TEST flag the test ID will force the 
    encoder operation according to the test vector. 

    TestID  Description
    0       No action, normal encoder operation
    1       Frame quantization test, qp = 1..31
    3       Interrupt interval test, 0..10
    4       RLC buffer limit test
    5       Interrupt interval and RLC buffer limit test
    6       Checkpoint quantization test.
            Checkpoints with minimum and maximum values. Gradual increase of
            checkpoint values.
    7       Video packet size test, vpSize = 10, 20, 30, ....


------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------

    EncConfigureTestBeforeStream

    Function configures the encoder instance before starting stream encoding

------------------------------------------------------------------------------*/
void EncConfigureTestBeforeStream(instance_s * inst)
{
    ASSERT(inst);

    switch(inst->testId)
    {
        case 4:
            EncRlcBufferFullTest(inst);
            break;
        default:
            break;
    }

}

/*------------------------------------------------------------------------------

    EncConfigureTestBeforeFrame

    Function configures the encoder instance before starting frame encoding

------------------------------------------------------------------------------*/
void EncConfigureTestBeforeFrame(instance_s * inst)
{
    ASSERT(inst);

    switch(inst->testId)
    {
        case 1:
            EncFrameQuantizationTest(inst);
            break;
        case 3:
            EncMbPerInterruptTest(inst);
            break;
        case 5:
            EncRandomInterruptTest(inst);
            break;
        case 6:
            EncMbQuantizationTest(inst);
            break;
        case 7:
            EncVpSizeTest(inst);
            break;
        default:
            break;
    }

}

/*------------------------------------------------------------------------------
  EncFrameQuantizationTest
------------------------------------------------------------------------------*/
void EncFrameQuantizationTest(instance_s *inst)
{
    i32 vopNum = inst->frameCnt;

    /* Inter frame qp start zero */
    inst->rateControl.qpHdr = MIN(31, MAX(1, vopNum%32));
    inst->rateControl.qpMin = inst->rateControl.qpHdr;
    inst->rateControl.qpMax = inst->rateControl.qpHdr;

    printf("EncFrameQuantTest# qpHdr %d\n", inst->rateControl.qpHdr);
}

/*------------------------------------------------------------------------------
  EncQuantizationTest()
  NOTE: ASIC resolution is value/4, see EncMapControlParameters()
------------------------------------------------------------------------------*/
void EncMbQuantizationTest(instance_s *inst)
{
        mb_s *mb = &inst->macroblock;
        rateControl_s *rc = &inst->rateControl;
	i32 vopNum = inst->frameCnt;
	qpCtrl_s *qc = &rc->qpCtrl;
	i32 tmp, i;
	i32 bitCnt = 0xffff / 200;

	rc->qpHdr = mb->qp = 15;
	rc->qpMin = 1 + vopNum/4;
	rc->qpMax = 31 - vopNum/4;

	for (i = 0; i < CHECK_POINTS; i++) {
		if (vopNum < 2) {
			tmp = 1;
		} else if (vopNum < 4) {
			tmp = 0xffff;
		} else {
			tmp = (vopNum * bitCnt)/4;
		}
		qc->wordCntTarget[i] = MIN(0xffff, tmp) & -0x4;
	}

	for (i = 0; i < CTRL_LEVELS; i++) {
		if (vopNum < 2) {
			tmp = -0x8000;
		} else if (vopNum < 4) {
			tmp = 0x7fff;
		} else {
			tmp = (-bitCnt/2*vopNum + vopNum * i * bitCnt/5)/4;
		}
		qc->wordError[i] = MIN(0x7fff, MAX(-0x8000, tmp));

		tmp = -8 + i*3;
		if ((vopNum&4) == 0) {
			tmp = -tmp;
		}
		qc->qpChange[i] = MIN(0x7, MAX(-0x8, tmp));
	}

    printf("EncMbQuantTest# wordCntTarget[] %d  wordError[] %d  qpChange[] %d\n", 
            qc->wordCntTarget[0], qc->wordError[0], qc->qpChange[0]);
}


/*------------------------------------------------------------------------------

	MbPerInterruptTest

------------------------------------------------------------------------------*/
void EncMbPerInterruptTest(instance_s *inst)
{
    i32 vopNum = inst->frameCnt;

    /*cmd->mbPerInterrupt = vopNum;*/

    inst->asic.regs.irqInterval = vopNum;

    printf("EncMbPerInterruptTest# irqInterval %d\n", 
            inst->asic.regs.irqInterval);
}

/*------------------------------------------------------------------------------

	RlcBufferFullTest

------------------------------------------------------------------------------*/
void EncRlcBufferFullTest(instance_s *inst)
{
    i32 vopNum = inst->frameCnt;
    i32 tmp;

    tmp = (64 * 6) / 4;
    tmp = vopNum / 2 * tmp;

    /* Free and reallocate ASIC output RLC buffers */
    EncFreeBuffers(&inst->asic);

    if(EncAllocateBuffers(&inst->asic, ENC7280_BUFFER_AMOUNT, tmp, 
                inst->macroblock.mbPerVop) != ENCHW_OK) {
        printf("ERROR: Couldn't allocate RLC buffers for testing!\n");
        ASSERT(0);
    }
    
    printf("EncRlcBufferFullTest# RLC buffers %dx%d bytes\n", 
            ENC7280_BUFFER_AMOUNT, tmp);
}

/*------------------------------------------------------------------------------

	RandomInterruptTest

------------------------------------------------------------------------------*/
void EncRandomInterruptTest(instance_s *inst)
{
    i32 vopNum = inst->frameCnt;

    if(vopNum < 2)
    {
        /*cmd->rlcBufferSize = 1600;*/
        inst->asic.regs.irqInterval = 11;
    }
    else if(vopNum < 4)
    {
        /*cmd->rlcBufferSize = 1600;*/
        inst->asic.regs.irqInterval = 22;
    }
    else if(vopNum < 6)
    {
        /*cmd->rlcBufferSize = 3200;*/
        inst->asic.regs.irqInterval = 33;
    }
    else if(vopNum < 8)
    {
        /*cmd->rlcBufferSize = 0;*/
        inst->asic.regs.irqInterval = 0;
    }
    else if(vopNum < 10)
    {
        /*cmd->rlcBufferSize = 0;*/
        inst->asic.regs.irqInterval = 1;
    }

    printf("EncRandomInterruptTest# irqInterval %d\n", 
            inst->asic.regs.irqInterval);
}

/*------------------------------------------------------------------------------

	EncVpSizeTest

------------------------------------------------------------------------------*/
void EncVpSizeTest(instance_s *inst)
{
    inst->macroblock.vpSize = 10 + 10*inst->frameCnt;

    printf("EncVpSizeTest# vpSize %d\n", 
            inst->macroblock.vpSize);
}

