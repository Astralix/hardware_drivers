/*-----------------------------------------------------------------------------
-- Project: DP56KDWA
--
-- Module Name: dp_common_def.v
--
-- Related Modules: 
--
-- $Source: /projects/cddb/dp56k/RAZOR_UNIVERSE/DOMAIN_01/dp56kdwa/Archive/RZ_VCS/dp56kdwa/blk/common/gendef/rtl/dp_common_def.v,v $ 
--
-- $Revision: 1.32 $
--
-- $Date: 2008-03-16 12:22:39+02 $
-
-- Author: $
--
--  Copyright (C) 1999 DSP Inc. All Rights Reserved
--
--  Reference:       File Name              : dp_common_def.v
--                   
--                   Release Information    : DB56000B00 TO release
--
-- Functional description: definition file for all dp digital components
--
-----------------------------------------------------------------------------*/

//////////////////////////////////////////////////////////////////////////
#define CLOCK_100K 1
#define CLOCK_1M   2
#define CLOCK_4M   3


#define LDO_SP1 	0x1
#define LDO_SP2 	0x2
#define LDO_SP3 	0x3
#define LDO_IO  	0X4
#define LDO_CORE0 	0x5
#define LDO_CORE1 	0x6
#define LDO_MEM		0x7
#define LDO_DPIO 	0X8

#define TEMP2CUR	1<<14
#define TEMP1CUR	1<<13
#define TEMP0CUR	1<<12
#define TEMP2EN		1<<11
#define TEMP1EN		1<<10
#define TEMP0EN		1<<9
#define DCIN3COMPEN 1<<7
#define DCIN2COMPEN 1<<6
#define DCIN1COMPEN 1<<5
#define DCIN0COMPEN 1<<4
#define HSDETCOMPEN 1<<3
#define AUXADEN     1<<1
#define AUXBGEN		1<<0

#define VILED 			0x0
#define VCC_DIV2		0x1
#define VIBAT 			0x2
#define VTEMP0			0x3
#define VTEMP1			0x4
#define VTEMP2 			0x5
#define VBGP			0x7
#define DCIN0			0x8
#define DCIN0_DIV2  	0X9
#define DCIN1			0xa
#define DCIN1_DIV2		0xb
#define DCIN2			0xc
#define DCIN2_DIV2		0xd
#define APDN_DCIN3		0xe
#define APDN_DCIN3_DIV2 0xf
#define LED_ATTEN		0x10
#define VDD 			0x11
#define VIBAT_ATTEN 	0x12

//PWM

 
#define PWM_BASE			0x60

#define PWM0_CFG1        PWM_BASE + 0x0
#define PWM0_CFG2        PWM_BASE +  1
#define PWM0_CRGTMR_CFG  PWM_BASE +   2
#define PWM0_CRG_TIMEOUT PWM_BASE +   3
#define PWM1_CFG1        PWM_BASE +   4
#define PWM1_CFG2        PWM_BASE +   5
#define PWM2_CFG1        PWM_BASE +   6
#define PWM2_CFG2        PWM_BASE +   7

#define RIGHT_LEFT 0
#define RIGHT 1
#define LEFT  2
//////////////////////////////////////////////////////////////////////////

//RTC

/*#define RTC_START              2'd0
#define RTC_SMP                2'd1
#define RTC_CMP                2'd2
#define RTC_WAIT_STATE         2'd3

#define RTC_BIST_START         3'd0
#define RTC_BIST_RESET_1       3'd1
#define RTC_BIST_RESET_2       3'd2
#define RTC_BIST_SET           3'd3
#define RTC_BIST_WAIT          3'd4
*/
 
#define DP_RTC_BASE				0x68
#define RTC_DRM_1               DP_RTC_BASE + 0 
#define RTC_DRM_2               DP_RTC_BASE + 1
#define RTC_DRM_3               DP_RTC_BASE + 2
#define RTC_DRM_4               DP_RTC_BASE + 3
#define RTC_DRM_5               DP_RTC_BASE + 4
#define RTC_DRM_6               DP_RTC_BASE + 5

#define RTC_VALID 	0<<1
#define RTC_SAMPLE 	15<<1


//////////////////////////////////////////////////////////////////////////
// CMU
#define	CMU_BASE 0x0

#define CMU_CCR0      CMU_BASE + 0
#define CMU_CCR1      CMU_BASE + 1
#define CMU_CCR2      CMU_BASE + 2
#define CMU_PLLCR     CMU_BASE + 3
#define CMU_CSTSR     CMU_BASE + 4
#define CMU_SWRST     CMU_BASE + 5
#define CMU_SYSCFG    CMU_BASE + 6
#define CMU_CMUTEST   CMU_BASE + 7
#define CMU_SWRST_VAL		0xC674

//////////////////////////////////////////////////////////////////////////

// SIMIF

#define SIM_BASE			0x20

#define SIM_CTRL_ADDR      SIM_BASE + 0
	
/////////////////////////////////////////////////////////////////////////

//AUX

//FSM states

/*
#define TP_IDLE  3'd0
#define TP_TIMER 3'd1
#define TP_SAMP  3'd2
#define TP_ITER  3'd3
#define TP_END   3'd4

 

#define TP_SELECT 5'b00110
*/
#define AUX_BASE 0x40

// registets 
#define AUX_EN                 AUX_BASE + 0 
#define AUX_ADCTRL             AUX_BASE + 1
#define AUX_ADCFG              AUX_BASE + 2
#define AUX_ADDATA             AUX_BASE + 3
#define AUX_RAWPMINTSTAT       AUX_BASE + 4
#define AUX_PMITMSKN           AUX_BASE + 5
#define AUX_PMINTPOL           AUX_BASE + 6
#define AUX_PMINTSTAT          AUX_BASE + 7
#define AUX_DC0GAINCNT1        AUX_BASE + 8
#define AUX_DC1GAINCNT1        AUX_BASE + 9
#define AUX_DC2GAINCNT0        AUX_BASE + 10
#define AUX_DC2GAINCNT1        AUX_BASE + 11
#define AUX_PMTPCTL1           AUX_BASE + 12
#define AUX_PMTPCTL2           AUX_BASE + 13
#define AUX_TPDX               AUX_BASE + 14
#define AUX_TPDY               AUX_BASE + 15
#define AUX_TPDZ1              AUX_BASE + 16
#define AUX_TPDZ2              AUX_BASE + 17
#define AUX_DC0GAINBP1         AUX_BASE + 18
#define AUX_DC0GAINBP2         AUX_BASE + 19
#define AUX_DC1GAINBP1         AUX_BASE + 20
#define AUX_DC1GAINBP2         AUX_BASE + 21
#define AUX_DC2GAINBP1         AUX_BASE + 22
#define AUX_DC2GAINBP2         AUX_BASE + 23
#define AUX_DC3GAINBP1         AUX_BASE + 24
#define AUX_DC3GAINBP2         AUX_BASE + 25
#define AUX_TEST               AUX_BASE + 26

 
//////////////////////////////////////////////////////////////////////////

//DCLASS
#define DCLASS_BASE				0x23

#define DCLASS_CONFIG           DCLASS_BASE + 0
#define DCLASS_SW_CTRL1         DCLASS_BASE + 1
#define DCLASS_SW_CTRL2         DCLASS_BASE + 2
#define DCLASS_SW_CTRL3         DCLASS_BASE + 3

 
//////////////////////////////////////////////////////////////////////////
// PMU CONTROL 
// PMU SHELL
// retained direct map

#define PMU_CTRL               0x70
#define PMU_SW_EN              0x71
#define PMU_SW_LVL1            0x72
#define PMU_SW_LVL2            0x73 
#define PMU_SW_LVL3            0x74 
#define PMU_STAT               0x75
#define PMU_INTMSK             0x76
#define PMU_RAWSTAT            0x77
#define PMU_SW_RFDC_CFG1       0x78
#define PMU_SW_RFDC_CFG2       0x79
#define PMU_SW_CRDC_CFG1       0x7a
#define PMU_SW_CRDC_CFG2       0x7b
#define PMU_OFF                0x7c                          
#define PMU_PM_ON_DEB          0x7d

 
// retained indirect map

#define PMU_GROUP1             0x180
#define PMU_GROUP2             0x181
#define PMU_WIN                0x182
#define PMU_RFDC_FCFG1         0x183
#define PMU_RFDC_FCFG2         0x184
#define PMU_RFDC_FCFG3         0x185
#define PMU_CRDC_FCFG1         0x186
#define PMU_CRDC_FCFG2         0x187
#define PMU_CRDC_FCFG3         0x188
#define PMU_LDO_CFG1           0x189
#define PMU_LDO_CFG2           0x18a
#define PMU_LDO_CFG3           0x18b
#define PMU_LDO_CFG4           0x18c
#define PMU_LDO_CFG5           0x18d
#define PMU_BAKPORLVL          0x18e
#define PMU_POR_BP             0x18f


// PMU CORE
// non retained indirect map

#define PMUCORE_ENSET0         0x100
#define PMUCORE_ENSET1         0x101
#define PMUCORE_ENSET2         0x102
#define PMUCORE_ENSET3         0x103
#define PMUCORE_LVL1SET0       0x104
#define PMUCORE_LVL2SET0       0x105
#define PMUCORE_LVL3SET0       0x106
#define PMUCORE_LVL1SET1       0x107
#define PMUCORE_LVL2SET1       0x108
#define PMUCORE_LVL3SET1       0x109
#define PMUCORE_LVL1SET2       0x10a
#define PMUCORE_LVL2SET2       0x10b
#define PMUCORE_LVL3SET2       0x10c
#define PMUCORE_LVL1SET3       0x10d
#define PMUCORE_LVL2SET3       0x10e
#define PMUCORE_LVL3SET3       0x10f
#define PMUCORE_RFDC_CFG1SET0  0x110
#define PMUCORE_RFDC_CFG2SET0  0x111
#define PMUCORE_CRDC_CFG1SET0  0x112
#define PMUCORE_CRDC_CFG2SET0  0x113
#define PMUCORE_RFDC_CFG1SET1  0x114
#define PMUCORE_RFDC_CFG2SET1  0x115
#define PMUCORE_CRDC_CFG1SET1  0x116
#define PMUCORE_CRDC_CFG2SET1  0x117
#define PMUCORE_RFDC_CFG1SET2  0x118
#define PMUCORE_RFDC_CFG2SET2  0x119
#define PMUCORE_CRDC_CFG1SET2  0x11a
#define PMUCORE_CRDC_CFG2SET2  0x11b
#define PMUCORE_RFDC_CFG1SET3  0x11c
#define PMUCORE_RFDC_CFG2SET3  0x11d
#define PMUCORE_CRDC_CFG1SET3  0x11e
#define PMUCORE_CRDC_CFG2SET3  0x11f
 

/*
#define PMU_OFF_IDLE           2'd0
#define PMU_OFF_STATE1         2'd1 
#define PMU_OFF_STATE2         2'd2 
#define PMU_SEQ_IDLE           3'd0
#define PMU_SEQ_SMP1           3'd1
#define PMU_SEQ_GROUP0         3'd2
#define PMU_SEQ_GROUP1         3'd3 
#define PMU_SEQ_GROUP2         3'd4 
#define PMU_SEQ_GROUP3         3'd5 
*/

// registets 
#define AFE_BASE			  0x28

#define AFEOUT_CTL1           AFE_BASE + 0 
#define AFEOUT_CTL2           AFE_BASE + 1
#define AFEOUT_CTL3           AFE_BASE + 2
#define AFEIN                 AFE_BASE + 3
#define AFESDN                AFE_BASE + 4
#define AFEMIGC               AFE_BASE + 5

 
#define MUTE 1
#define LQDAC 2
#define LQSDO_L 3
#define LQSDO_R 4
#define HQSD_OL 5
#define HQSD_OR 6
#define INVMIC 7
#define INVHSMIC 8
#define MIC	9
#define HSMIC 10

