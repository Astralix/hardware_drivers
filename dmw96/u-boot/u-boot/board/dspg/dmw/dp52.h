#ifndef __DP52_H__
#define __DP52_H__

#include <asm/errno.h>

struct dp52_dev {
	void *spi_dev;
	int  type;
	void *priv;
};


struct tsc_dp52_measure
{
	unsigned int x;
	unsigned int y;
	unsigned int z1;
	unsigned int z2;
};

enum DP52_DEVICE_TYPE{
	DP52_CHARGER,
	DP52_TOUCH_SCREEN,
	DP52_RTC,
	DP52_CODEC,
	DP52_PMU,
	DP52_MAX_DEVICE_TYPE,
};

enum DP52_EVENTS{
	DP52_EVENT_CRGTMREXP,
	DP52_EVENT_CRGTHRREACHED,
	DP52_EVENT_AUXDATRDY,
	DP52_EVENT_EXTERNAL_POWER,
	DP52_EVENT_NO_EXTERNAL_POWER,
	DP52_MAX_EVENT,
};
//////////////////////////////////////////////////////////////////////////
// base adresses
//
#define	DP52_CMU_BASE			0x00	/* Clock and PLL (00 - 07) */ 
#define DP52_CODEC_BASE			0x08	/* Codecs I/F (08 - 0F) */
#define DP52_SPI_BASE			0x10	/* SPI (10 - 17) */
#define DP52_ICU_BASE			0x18	/* ICU (18 - 1F) */
#define DP52_SIM_BASE			0x20	/* SIM IF Registers (20 - 23) */
#define DP52_DCLASS_BASE		0x24	/* D-Class Registers (24 - 27) */
#define DP52_AFE_BASE			0x28	/* Anolog Front End (AFE) (28 - 2F) */
#define	DP52_LQ_SD_BASE			0x30	/* Low quality Sigma-Delta (30 - 37) */
#define DP52_HQ_SD_BASE			0x38	/* High quality Sigma-Delta (38 - 3F) */
#define DP52_AUX_BASE			0x40	/* Auxiliary ADC (40 - 5F ) */
#define DP52_PWM_BASE			0x60	/* PWM Generators and Charge Timer (60 - 67) */
#define DP52_RTC_BASE			0x68	/* DRM RTC (68 - 6F) */
#define DP52_PMU_DIRECT_BASE	0x70	/* PMU registers (70 - 7F) */		

/* Indirect address space bases */
#define DP52_VOLATILE_BASE		0x00	/* PMU Volatile Register (00 - 7F) */
#define DP52_PMU_RETAINED_BASE	0x80	/* PMU Retained Register (80 - FF) */

/************* DIRECT REGISTERS OFFSET **************/
/* CMU (00 - 07) */

#define DP52_CMU_CCR0      		(DP52_CMU_BASE + 0)
#define DP52_CMU_CCR1      		(DP52_CMU_BASE + 1)
#define DP52_CMU_CCR2      		(DP52_CMU_BASE + 2)
#define DP52_CMU_PLLCR     		(DP52_CMU_BASE + 3)
#define DP52_CMU_CSTSR     		(DP52_CMU_BASE + 4)
#define DP52_CMU_SWRST     		(DP52_CMU_BASE + 5)
#define DP52_CMU_SYSCFG    		(DP52_CMU_BASE + 6)
#define DP52_CMU_CMUTEST   		(DP52_CMU_BASE + 7)

/* CODECS I/F (08 - 0F) */
#define DP52_CODIF_CTRL			(DP52_CODEC_BASE + 0)
#define DP52_CODIF_STAT			(DP52_CODEC_BASE + 1)

/* SPI (10 - 17) */
#define DP52_SPI_INDADD 		(DP52_SPI_BASE + 0)
#define DP52_SPI_INDDAT 		(DP52_SPI_BASE + 1)

/* ICU (18 - 1F) */
#define DP52_ICU_MASK			(DP52_ICU_BASE + 0)
#define DP52_ICU_STAT			(DP52_ICU_BASE + 1)

/* SIM (20 - 23) */
#define DP52_SIM_CTRL_ADDR		(DP52_SIM_BASE + 0)

/* DCLASS (24 - 27) */
#define DP52_DCLASS_CFG			(DP52_DCLASS_BASE + 0)
#define DP52_DCLASS_SW_CTL1		(DP52_DCLASS_BASE + 1)
#define DP52_DCLASS_SW_CTL2		(DP52_DCLASS_BASE + 2)
#define DP52_DCLASS_SW_CTL3		(DP52_DCLASS_BASE + 3)

/* AFE (28 - 2F) */ 
#define DP52_AFEOUT_CTL1		(DP52_AFE_BASE + 0)
#define DP52_AFEOUT_CTL2		(DP52_AFE_BASE + 1)
#define DP52_AFEOUT_CTL3		(DP52_AFE_BASE + 2)
#define DP52_AFEIN				(DP52_AFE_BASE + 3)
#define DP52_AFESDN				(DP52_AFE_BASE + 4)
#define DP52_AFEMIGC			(DP52_AFE_BASE + 5)

/* LQ Sigma-Delta Codecs (30 - 37) */
#define DP52_LQ_SDCFG1			(DP52_LQ_SD_BASE + 0)
#define DP52_LQ_SDCFG_RSRV		(DP52_LQ_SD_BASE + 1)
#define DP52_LQ_SDCFG3			(DP52_LQ_SD_BASE + 2)
#define DP52_LQ_SDCFG4			(DP52_LQ_SD_BASE + 3)

/* HQ Sigma-Delta Codec (38 - 3F) */
#define DP52_HQ_SDS_CFG			(DP52_HQ_SD_BASE + 0)

/* AUX (40 - 5F) */
#define DP52_AUX_EN					(DP52_AUX_BASE + 0) 
#define DP52_AUX_ADCTRL				(DP52_AUX_BASE + 1)
#define DP52_AUX_ADCFG				(DP52_AUX_BASE + 2)
#define DP52_AUX_ADDATA				(DP52_AUX_BASE + 3)
#define DP52_AUX_RAWPMINTSTAT		(DP52_AUX_BASE + 4)
#define DP52_AUX_INTMSKN			(DP52_AUX_BASE + 5)
#define DP52_AUX_INTPOL				(DP52_AUX_BASE + 6)
#define DP52_AUX_INTSTAT			(DP52_AUX_BASE + 7)
#define DP52_AUX_DC0GAINCNT1		(DP52_AUX_BASE + 8)
#define DP52_AUX_DC1GAINCNT1		(DP52_AUX_BASE + 9)
#define DP52_AUX_DC2GAINCNT1		(DP52_AUX_BASE + 10)
#define DP52_AUX_DC3GAINCNT1		(DP52_AUX_BASE + 11)
#define DP52_TP_CTL1				(DP52_AUX_BASE + 12)
#define DP52_TP_CTL2				(DP52_AUX_BASE + 13)
#define DP52_AUX_TPDX				(DP52_AUX_BASE + 14)
#define DP52_AUX_TPDY				(DP52_AUX_BASE + 15)
#define DP52_AUX_TPDZ1				(DP52_AUX_BASE + 16)
#define DP52_AUX_TPDZ2				(DP52_AUX_BASE + 17)
#define DP52_AUX_DC0GAINBP1			(DP52_AUX_BASE + 18)
#define DP52_AUX_DC0GAINBP2			(DP52_AUX_BASE + 19)
#define DP52_AUX_DC1GAINBP1			(DP52_AUX_BASE + 20)
#define DP52_AUX_DC1GAINBP2			(DP52_AUX_BASE + 21)
#define DP52_AUX_DC2GAINBP1			(DP52_AUX_BASE + 22)
#define DP52_AUX_DC2GAINBP2			(DP52_AUX_BASE + 23)
#define DP52_AUX_DC3GAINBP1			(DP52_AUX_BASE + 24)
#define DP52_AUX_DC3GAINBP2			(DP52_AUX_BASE + 25)
#define DP52_AUX_TEST				(DP52_AUX_BASE + 26)

/* PWM (60 - 67) */
#define DP52_PWM0_CFG1        		(DP52_PWM_BASE + 0)
#define DP52_PWM0_CFG2        		(DP52_PWM_BASE + 1)
#define DP52_PWM0_CRGTMR_CFG  		(DP52_PWM_BASE + 2)
#define DP52_PWM0_CRG_TIMEOUT 		(DP52_PWM_BASE + 3)
#define DP52_PWM1_CFG1        		(DP52_PWM_BASE + 4)
#define DP52_PWM1_CFG2        		(DP52_PWM_BASE + 5)
#define DP52_PWM2_CFG1        		(DP52_PWM_BASE + 6)
#define DP52_PWM2_CFG2        		(DP52_PWM_BASE + 7)

/* RTC (68 - 6F) */
#define DP52_RTC_1				(DP52_RTC_BASE + 0)
#define DP52_RTC_2				(DP52_RTC_BASE + 1)
#define DP52_RTC_3				(DP52_RTC_BASE + 2)
#define DP52_RTC_4				(DP52_RTC_BASE + 3)
#define DP52_RTC_5				(DP52_RTC_BASE + 4)
#define DP52_RTC_6				(DP52_RTC_BASE + 5)

/* PMU Direct (70 - 7F) */
#define DP52_PMU_CTRL				(DP52_PMU_DIRECT_BASE + 0)
#define DP52_PMU_EN_SW				(DP52_PMU_DIRECT_BASE + 1)
#define DP52_PMU_LVL1_SW			(DP52_PMU_DIRECT_BASE + 2)
#define DP52_PMU_LVL2_SW			(DP52_PMU_DIRECT_BASE + 3)
#define DP52_PMU_LVL3_SW			(DP52_PMU_DIRECT_BASE + 4)
#define DP52_PMUSTAT				(DP52_PMU_DIRECT_BASE + 5)
#define DP52_PMINTMSK				(DP52_PMU_DIRECT_BASE + 6)
#define DP52_PMURAWSTAT				(DP52_PMU_DIRECT_BASE + 7)
#define DP52_PMU_RFDC_CFG1_SW 		(DP52_PMU_DIRECT_BASE + 8)
#define DP52_PMU_RFDC_CFG2_SW		(DP52_PMU_DIRECT_BASE + 9)
#define DP52_PMU_CRDC_CFG1_SW    	(DP52_PMU_DIRECT_BASE + 10)
#define DP52_PMU_CRDC_CFG2_SW		(DP52_PMU_DIRECT_BASE + 11)
#define DP52_PMU_OFF				(DP52_PMU_DIRECT_BASE + 12)
#define DP52_PMU_PM_ON_DEB			(DP52_PMU_DIRECT_BASE + 13)

/************* INDIRECT REGISTERS OFFSET **************/

/* retained indirect map */
#define DP52_PMU_IND_GROUP1				(DP52_PMU_RETAINED_BASE + 0)
#define DP52_PMU_IND_GROUP2				(DP52_PMU_RETAINED_BASE + 1)
#define DP52_PMU_IND_WIN				(DP52_PMU_RETAINED_BASE + 2)
#define DP52_PMU_IND_RFDC_FCFG1			(DP52_PMU_RETAINED_BASE + 3)
#define DP52_PMU_IND_RFDC_FCFG2			(DP52_PMU_RETAINED_BASE + 4)
#define DP52_PMU_IND_RFDC_FCFG3			(DP52_PMU_RETAINED_BASE + 5)
#define DP52_PMU_IND_CRDC_FCFG1			(DP52_PMU_RETAINED_BASE + 6)
#define DP52_PMU_IND_CRDC_FCFG2 		(DP52_PMU_RETAINED_BASE + 7)
#define DP52_PMU_IND_CRDC_FCFG3			(DP52_PMU_RETAINED_BASE + 8)
#define DP52_PMU_IND_LDO_CFG1			(DP52_PMU_RETAINED_BASE + 9)
#define DP52_PMU_IND_LDO_CFG2			(DP52_PMU_RETAINED_BASE + 10)
#define DP52_PMU_IND_LDO_CFG3			(DP52_PMU_RETAINED_BASE + 11)
#define DP52_PMU_IND_LDO_CFG4			(DP52_PMU_RETAINED_BASE + 12)
#define DP52_PMU_IND_LDO_CFG5			(DP52_PMU_RETAINED_BASE + 13)
#define DP52_PMU_IND_BAKPORLVL			(DP52_PMU_RETAINED_BASE + 14)
#define DP52_PMU_IND_POR_BP				(DP52_PMU_RETAINED_BASE + 15)

/*  non retained indirect map */
#define DP52_PMUCORE_IND_EN_SET_0			(DP52_VOLATILE_BASE + 0)
#define DP52_PMUCORE_IND_EN_SET_1			(DP52_VOLATILE_BASE + 1)
#define DP52_PMUCORE_IND_EN_SET_2			(DP52_VOLATILE_BASE + 2)
#define DP52_PMUCORE_IND_EN_SET_3			(DP52_VOLATILE_BASE + 3)
#define DP52_PMUCORE_IND_LVL1_SET_0			(DP52_VOLATILE_BASE + 4)
#define DP52_PMUCORE_IND_LVL2_SET_0			(DP52_VOLATILE_BASE + 5)
#define DP52_PMUCORE_IND_LVL3_SET_0			(DP52_VOLATILE_BASE + 6)
#define DP52_PMUCORE_IND_LVL1_SET_1			(DP52_VOLATILE_BASE + 7)
#define DP52_PMUCORE_IND_LVL2_SET_1			(DP52_VOLATILE_BASE + 8)
#define DP52_PMUCORE_IND_LVL3_SET_1			(DP52_VOLATILE_BASE + 9)
#define DP52_PMUCORE_IND_LVL1_SET_2			(DP52_VOLATILE_BASE + 10)
#define DP52_PMUCORE_IND_LVL2_SET_2 		(DP52_VOLATILE_BASE + 11)
#define DP52_PMUCORE_IND_LVL3_SET_2			(DP52_VOLATILE_BASE + 12)
#define DP52_PMUCORE_IND_LVL1_SET_3			(DP52_VOLATILE_BASE + 13)
#define DP52_PMUCORE_IND_LVL2_SET_3			(DP52_VOLATILE_BASE + 14)
#define DP52_PMUCORE_IND_LVL3_SET_3			(DP52_VOLATILE_BASE + 15)
#define DP52_PMUCORE_IND_RFDC_CFG1_SET_0	(DP52_VOLATILE_BASE + 16)
#define DP52_PMUCORE_IND_RFDC_CFG2_SET_0	(DP52_VOLATILE_BASE + 17)
#define DP52_PMUCORE_IND_CRDC_CFG1_SET_0	(DP52_VOLATILE_BASE + 18)
#define DP52_PMUCORE_IND_CRDC_CFG2_SET_0	(DP52_VOLATILE_BASE + 19)
#define DP52_PMUCORE_IND_RFDC_CFG1_SET_1	(DP52_VOLATILE_BASE + 20)
#define DP52_PMUCORE_IND_RFDC_CFG2_SET_1	(DP52_VOLATILE_BASE + 21)
#define DP52_PMUCORE_IND_CRDC_CFG1_SET_1	(DP52_VOLATILE_BASE + 22)
#define DP52_PMUCORE_IND_CRDC_CFG2_SET_1	(DP52_VOLATILE_BASE + 23)
#define DP52_PMUCORE_IND_RFDC_CFG1_SET_2	(DP52_VOLATILE_BASE + 24)
#define DP52_PMUCORE_IND_RFDC_CFG2_SET_2	(DP52_VOLATILE_BASE + 25)
#define DP52_PMUCORE_IND_CRDC_CFG1_SET_2	(DP52_VOLATILE_BASE + 26)
#define DP52_PMUCORE_IND_CRDC_CFG2_SET_2	(DP52_VOLATILE_BASE + 27)
#define DP52_PMUCORE_IND_RFDC_CFG1_SET_3	(DP52_VOLATILE_BASE + 28)
#define DP52_PMUCORE_IND_RFDC_CFG2_SET_3	(DP52_VOLATILE_BASE + 29)
#define DP52_PMUCORE_IND_CRDC_CFG1_SET_3	(DP52_VOLATILE_BASE + 30)
#define DP52_PMUCORE_IND_CRDC_CFG2_SET_3	(DP52_VOLATILE_BASE + 31)

#define DP52_PMU_LDO_DP_CORE_EN	(1<<10)
#define DP52_PMU_CORE_HI_LO	(1<<9)
#define DP52_PMU_CORE_EN	(1<<8)
#define DP52_PMU_LDO_MEM_EN	(1<<7)
#define DP52_PMU_LDO_SP1_EN	(1<<6)
#define DP52_PMU_LDO_IO_EN	(1<<5)
#define DP52_PMU_RF_HI_LO	(1<<4)
#define DP52_PMU_RF_EN		(1<<3)
#define DP52_PMU_LDO_DP_IO_EN	(1<<2)
#define DP52_PMU_LDO_SP2_EN	(1<<1)
#define DP52_PMU_LDO_SP3_EN	(1<<0)

#define DP52_INT_TP 		(1<<11)
#define DP52_INT_PMUSEQ    	(1<<10)
#define DP52_INT_SD1    	(1<<9)
#define DP52_INT_SD0    	(1<<8)
#define DP52_INT_CODEIF 	(1<<7)
#define DP52_INT_CHRGTIMER 	(1<<6)
#define DP52_INT_LQSD 		(1<<5)
#define DP52_INT_RTC 		(1<<4)
#define DP52_INT_AUXCOMP 	(1<<3)
#define DP52_INT_AUXADC 	(1<<2)
#define DP52_INT_PDN 		(1<<1)
#define DP52_INT_PMUEN		(1)

#define DP52_AUX_INT_TPEN		(1<<6)
#define DP52_AUX_INT_HSDETEN 	(1<<5)
#define DP52_AUX_INT_DCIN3EN 	(1<<3)
#define DP52_AUX_INT_DCIN2EN 	(1<<2)
#define DP52_AUX_INT_DCIN1EN    (1<<1)
#define DP52_AUX_INT_DCIN0EN    (1)

#define DP52_AUX_INT_TPPOL		(1<<6)
#define DP52_AUX_INT_HSDETPOL 	(1<<5)
#define DP52_AUX_INT_DCIN3POL 	(1<<3)
#define DP52_AUX_INT_DCIN2POL 	(1<<2)
#define DP52_AUX_INT_DCIN1POL   (1<<1)
#define DP52_AUX_INT_DCIN0POL   (1)

#define DP52_AUX_INT_ADEN		(1<<7)
#define DP52_AUX_INT_TPSTAT     (1<<6)
#define DP52_AUX_INT_HSDETSTAT 	(1<<5)
#define DP52_AUX_INT_PDNSTAT    (1<<4)
#define DP52_AUX_INT_DCIN3STAT 	(1<<3)
#define DP52_AUX_INT_DCIN2STAT 	(1<<2)
#define DP52_AUX_INT_DCIN1STAT  (1<<1)
#define DP52_AUX_INT_DCIN0STAT  (1)

#define DP52_AUXADCTRL_SAMPLE   (1)
#define DP52_TP_CTL2_AUTOMODE   (1<<4)
#define DP52_TP_RDY   			(1<<1)
#define DP52_AUX_ADCFG_DCIN1	(0xA)
#define DP52_PMUSTAT_DDCINS		(1<<1)

#define DP52_AUX_MUX_DCIN2_GAIN		(0x0)
#define DP52_AUX_MUX_VCC		(0x1)
#define DP52_AUX_MUX_DCIN1_GAIN		(0x2)
#define DP52_AUX_MUX_VTEMP0		(0x3)
#define DP52_AUX_MUX_VTEMP1		(0x4)
#define DP52_AUX_MUX_VTEMP2		(0x5)
#define DP52_AUX_MUX_VBGP		(0x7)
#define DP52_AUX_MUX_DCIN0_16V		(0x8)
#define DP52_AUX_MUX_DCIN0_32V		(0x9)
#define DP52_AUX_MUX_DCIN1_16V		(0xA)
#define DP52_AUX_MUX_DCIN1_32V		(0xB)
#define DP52_AUX_MUX_DCIN2_16V		(0xC)
#define DP52_AUX_MUX_DCIN2_32V		(0xD)
#define DP52_AUX_MUX_DCIN3_16V		(0xE)
#define DP52_AUX_MUX_DCIN3_32V		(0xF)
#define DP52_AUX_MUX_DCIN2_ATT		(0x10)
#define DP52_AUX_MUX_VDD		(0x11)
#define DP52_AUX_MUX_DCIN1_ATT		(0x12)
#define DP52_AUX_MUX_VBG_12V		(0x17)
#define DP52_AUX_MUX_VBAK		(0x19)
#define DP52_AUX_MUX_DCIN0_GAIN		(0x1A)
#define DP52_AUX_MUX_DCIN3_GAIN		(0x1B)
#define DP52_AUX_MUX_DCIN0_ATT		(0x1C)
#define DP52_AUX_MUX_DCIN3_ATT		(0x1D)

#define DP52_CMU_SWRST_VAL			0xC674	/* Put this value to CMURST register in order to reset */

#define DP52_CRGTMR_CFG_TIMER_DONE 	(1 << 4)


/* Sound-related register-field masks: */
/* ----------------------------------- */

#define DP52_CCR0_PLLEN_MASK				(0x1   <<  15) /* bit  15      */
#define DP52_CCR0_HQSDCLK_EN_MASK			(0x1   <<  14) /* bit  14      */
#define DP52_CCR0_LQSDCLK_EN_MASK			(0x1   <<  13) /* bit  13      */
#define DP52_CCR0_DSPDIVCLKEN_MASK			(0x1   <<  12) /* bit  12      */
#define DP52_CCR0_AUXSRC_MASK				(0x3   <<  10) /* bits 11 : 10 */
#define DP52_CCR0_PMUSRC_MASK				(0x3   <<   8) /* bits  9 :  8 */
#define DP52_CCR0_AUXCLKDIV_MASK			(0x7   <<   5) /* bits  7 :  5 */
#define DP52_CCR0_HQDIV_MASK				(0xF   <<   0) /* bits  4 :  0 */

#define DP52_CCR1_SDCLKSEL_MASK				(0x1   <<  15) /* bit  15      */
#define DP52_CCR1_AUXADCRST_MASK			(0x1   <<  14) /* bit  14      */
#define DP52_CCR1_DSPDIV_MASK				(0xFF  <<   6) /* bits 13 :  6 */
#define DP52_CCR1_LQDIV_MASK				(0x3F  <<   0) /* bits  5 :  0 */

#define DP52_PLLCR_OD_MASK					(0x3   <<  14) /* bits 15 : 14 */
#define DP52_PLLCR_R_MASK					(0x1F  <<   9) /* bits 13 :  9 */
#define DP52_PLLCR_F_MASK					(0x1FF <<   0) /* bits  8 :  0 */

#define DP52_SYSCFG_OLD__RSRV_MASK			(0x3FF <<   6) /* bits 15 :  6 */ // In Old DP52. In New DP52: 15-5 are Reserved
#define DP52_SYSCFG_OLD__AFEHQSD_POL_MASK	(0x1   <<   5) /* bit   5      */ // In Old DP52. In New DP52: is Reserved !
#define DP52_SYSCFG_I2S_CLK32_EN_MASK		(0x1   <<   4) /* bit   4      */
#define DP52_SYSCFG_OLD__AFE_PHASE_MASK		(0x3   <<   2) /* bits  3 :  2 */  // In Old DP52. In New DP52: is Reserved !
#define DP52_SYSCFG_KYDRV_EN_MASK			(0x1   <<   1) /* bit   1      */
#define DP52_SYSCFG_I2S_EN_MASK				(0x1   <<   0) /* bit   0      */

#define DP52_CODIF_CTRL_RSRV_MASK			(0x7FFF <<  1) /* bits 15 :  1 */
#define DP52_CODIF_CTRL_LQONLY_MASK			(0x1    <<  0) /* bit   0      */

#define DP52_CODIF_STAT_LQADC1_FIFO_OR_MASK	(0x1    << 15) /* bit  15      */
#define DP52_CODIF_STAT_LQADC1_FIFO_UR_MASK	(0x1    << 14) /* bit  14      */
#define DP52_CODIF_STAT_LQADC0_FIFO_OR_MASK	(0x1    << 13) /* bit  13      */
#define DP52_CODIF_STAT_LQADC0_FIFO_UR_MASK	(0x1    << 12) /* bit  12      */
#define DP52_CODIF_STAT_RSRV1_MASK			(0x3    << 10) /* bits 11 : 10 */
#define DP52_CODIF_STAT_LQDAC_FIFO_OR_MASK	(0x1    <<  9) /* bit   9      */
#define DP52_CODIF_STAT_LQDAC_FIFO_UR_MASK	(0x1    <<  8) /* bit   8      */
#define DP52_CODIF_STAT_RSRV2_MASK			(0xF    <<  4) /* bits  7 :  4 */
#define DP52_CODIF_STAT_HQDAC_LFIFO_OR_MASK	(0x1    <<  3) /* bit   3      */
#define DP52_CODIF_STAT_HQDAC_LFIFO_UR_MASK	(0x1    <<  2) /* bit   2      */
#define DP52_CODIF_STAT_HQDAC_RFIFO_OR_MASK	(0x1    <<  1) /* bit   1      */
#define DP52_CODIF_STAT_HQDAC_RFIFO_UR_MASK	(0x1    <<  0) /* bit   0      */

#define DP52_LQ_SDCFG1_SDFB_MASK			(0x1	<< 15) /* bit  15      */
#define DP52_LQ_SDCFG1_RSRV1_MASK			(0x7FF  <<  4) /* bits 14 :  4 */
#define DP52_LQ_SDCFG1_SD1RX_MASK			(0x1    <<  3) /* bit   3      */
#define DP52_LQ_SDCFG1_RSRV2_MASK			(0x1    <<  2) /* bit   2      */
#define DP52_LQ_SDCFG1_SD0RX_MASK			(0x1    <<  1) /* bit   1      */
#define DP52_LQ_SDCFG1_SD0TX_MASK			(0x1    <<  0) /* bit   0      */

#define DP52_LQ_SDCFG3_RSRV1_MASK			(0x3FF  <<  6) /* bits  6 : 15 */
#define DP52_LQ_SDCFG3_DITHERA_MASK			(0x1    <<  5) /* bit   5      */
#define DP52_LQ_SDCFG3_DITHERD_MASK			(0x1    <<  4) /* bit   4      */
#define DP52_LQ_SDCFG3_DITHERVAL_MASK		(0x3    <<  2) /* bits  3 :  2 */
#define DP52_LQ_SDCFG3_RSRV2_MASK			(0x3    <<  0) /* bits  1 :  0 */

#define DP52_LQ_SDCFG4_SDINT_MASK			(0x1    << 15) /* bit  15      */
#define DP52_LQ_SDCFG4_RSRV_MASK			(0x1FFF <<  2) /* bits 14 :  2 */
#define DP52_LQ_SDCFG4_SD1FAIL_MASK			(0x1    <<  1) /* bit   1      */
#define DP52_LQ_SDCFG4_SD0FAIL_MASK			(0x1    <<  0) /* bit   0      */

#define DP52_HQ_SDS_CFG_RSRV_MASK			(0x1    << 15) /* bit  15      */
#define DP52_HQ_SDS_CFG_SD_DACL_EN_MASK		(0x1    << 14) /* bit  14      */
#define DP52_HQ_SDS_CFG_SD_DACR_EN_MASK		(0x1    << 13) /* bit  13      */
#define DP52_HQ_SDS_CFG_SDS_C_SHFT_MASK		(0x1    << 12) /* bit  12      */
#define DP52_HQ_SDS_CFG_NS_COUNT_MASK		(0x7FF  <<  4) /* bits 11 :  4 */
#define DP52_HQ_SDS_CFG_NH_COUNT_MASK		(0xF    <<  0) /* bits  3 :  0 */

#define DP52_AFEOUT_CTL1_RSRV_MASK			(0xF    << 12) /* bits 15 : 12 */ 
#define DP52_AFEOUT_CTL1_GAIN_L_MASK		(0x1F   <<  7) /* bits 11 :  7 */
#define DP52_AFEOUT_CTL1_GAIN_R_MASK		(0x1F   <<  2) /* bits  6 :  2 */
#define DP52_AFEOUT_CTL1_SDOUTR_EN_MASK		(0x1    <<  1) /* bit   1      */
#define DP52_AFEOUT_CTL1_SDOUTL_EN_MASK		(0x1    <<  0) /* bit   0      */

#define DP52_AFEOUT_CTL2_RSRV_MASK			(0x3F   << 10) /* bits 15 : 10 */
#define DP52_AFEOUT_CTL2_OUTL_SEL_MASK		(0x1F   <<  5) /* bits  9 :  5 */
#define DP52_AFEOUT_CTL2_OUTR_SEL_MASK		(0x1F   <<  0) /* bits  4 :  0 */

#define DP52_AFEOUT_CTL3_RSRV_MASK			(0x1FF  <<  7) /* bits 15 :  7 */
#define DP52_AFEOUT_CTL3_HSCOM_EN_MASK		(0x1    <<  6) /* bit   6      */
#define DP52_AFEOUT_CTL3_COM_SOFT_CTL_MASK	(0x3    <<  4) /* bits  5 :  4 */
#define DP52_AFEOUT_CTL3_HSOUTL_EN_MASK		(0x1    <<  3) /* bit   3      */
#define DP52_AFEOUT_CTL3_HSOUTR_EN_MASK		(0x1    <<  2) /* bit   2      */
#define DP52_AFEOUT_CTL3_HS_SOFT_CTL_MASK	(0x3    <<  0) /* bits  1 :  0 */

#define DP52_AFEIN_RSRV_MASK				(0xF    << 12) /* bits 15 : 12 */
#define DP52_AFEIN_LIN0GC_MASK				(0x1F   <<  7) /* bits 11 :  7 */
#define DP52_AFEIN_LINRBP_MASK				(0x1    <<  6) /* bit   6      */
#define DP52_AFEIN_ADC1IN_MASK				(0x7    <<  3) /* bits  5 :  3 */
#define DP52_AFEIN_ADC0IN_MASK				(0x7    <<  0) /* bits  2 :  0 */

#define DP52_AFESDN_RSRV_MASK				(0xFF   <<  8) /* bits 15 :  8 */
#define DP52_AFESDN_VREFLVL_MASK			(0x7    <<  5) /* bits  7 :  5 */
#define DP52_AFESDN_VREFEN_MASK				(0x1    <<  4) /* bit   4      */
#define DP52_AFESDN_MISDN_MASK				(0x1    <<  3) /* bit   3      */
#define DP52_AFESDN_HSMISDN_MASK			(0x1    <<  2) /* bit   2      */
#define DP52_AFESDN_MPWREN_MASK				(0x1    <<  1) /* bit   1      */
#define DP52_AFESDN_LIN0SDN					(0x1    <<  0) /* bit   0      */

#define DP52_AFEMIGC_RSRV1_MASK				(0x7    << 13) /* bits 15 : 13 */
#define DP52_AFEMIGC_MINGC_MASK				(0x1F   <<  8) /* bits 12 :  8 */
#define DP52_AFEMIGC_RSRV2_MASK				(0x1    <<  7) /* bit   7      */
#define DP52_AFEMIGC_HSMICGAIN_MASK			(0x1    <<  6) /* bit   6	   */
#define DP52_AFEMIGC_MICGAIN_MASK			(0x1    <<  5) /* bit   5	   */
#define DP52_AFEMIGC_HSINGC_MASK			(0x1F   <<  0) /* bits  4 :  0 */

#define DP52_DCLASS_CFG_RSRV1_MASK				(0x3F   << 10) /* bits 15 : 10 */
#define DP52_DCLASS_CFG_PWM_EN_MASK				(0x1    <<  9) /* bit   9      */
#define DP52_DCLASS_CFG_OLD__SHAPER_START_MASK	(0x3    <<  7) /* bits  8 :  7 */ // In Old DP52. In New DP52: is Reserved !
#define DP52_DCLASS_CFG_SHAPER_EN_MASK			(0x1    <<  6) /* bit   6      */
#define DP52_DCLASS_CFG_SWITCH_EN_MASK			(0x1    <<  5) /* bit   5      */
#define DP52_DCLASS_CFG_DEC_EN_MASK				(0x1    <<  4) /* bit   4      */
#define DP52_DCLASS_CFG_DCLASSJOIN_MASK			(0x3    <<  2) /* bits  3 :  2 */
#define DP52_DCLASS_CFG_DCLASSMODE_MASK			(0x3    <<  0) /* bits  1 :  0 */

/* For chip testing only (!) : */
#define DP52_DCLASS_SW_CTL1_C_DIFF_MASK			(0x1    << 15) /* bit  15      */
#define DP52_DCLASS_SW_CTL1_DELAY_N_MASK		(0x7    << 12) /* bits 14 : 12 */
#define DP52_DCLASS_SW_CTL1_DELAY_P_MASK		(0x7    <<  9) /* bits 11 :  9 */
#define DP52_DCLASS_SW_CTL1_PDR_FALL_MASK		(0x3    <<  7) /* bits  8 :  7 */
#define DP52_DCLASS_SW_CTL1_NDR_FALL_MASK		(0x3    <<  5) /* bits  6 :  5 */
#define DP52_DCLASS_SW_CTL1_PROT_FORCE_MASK		(0x1    <<  4) /* bit   4      */
#define DP52_DCLASS_SW_CTL1_RSRV_MASK			(0x3    <<  2) /* bits  3 :  2 */
#define DP52_DCLASS_SW_CTL1_ERR_RANGE_MASK		(0x3    <<  0) /* bits  1 :  0 */

/* For chip testing only (!) : */
#define DP52_DCLASS_SW_CTL2_MASK COIL_MASK		(0x1    << 15) /* bit  15      */
#define DP52_DCLASS_SW_CTL2_EN_SC_MASK          (0x1    << 14) /* bit  14      */
#define DP52_DCLASS_SW_CTL2_RESM_20K_MASK		(0x1    << 13) /* bit  13      */
#define DP52_DCLASS_SW_CTL2_RESM_10K_MASK		(0x1    << 12) /* bit  12      */
#define DP52_DCLASS_SW_CTL2_RESM_50K_MASK		(0x3    << 10) /* bits 11 : 10 */
#define DP52_DCLASS_SW_CTL2_IBIAS_SHAPER_MASK	(0x3    <<  8) /* bits  9 :  8 */
#define DP52_DCLASS_SW_CTL2_IBIAS_ERR_MASK		(0xF    <<  4) /* bits  7 :  4 */
#define DP52_DCLASS_SW_CTL2_PDR_RISE_MASK		(0x3    <<  2) /* bits  3 :  2 */
#define DP52_DCLASS_SW_CTL2_NDR_RISE_MASK		(0x3    <<  0) /* bits  1 :  0 */

/* For chip testing only (!) : */
#define DP52_DCLASS_SW_CTL3_RSRV1_MASK			(0xFF   <<  8) /* bits 15 :  8 */
#define DP52_DCLASS_SW_CTL3_EN_TEST_DIG_MASK	(0x1    <<  7) /* bit   7      */
#define DP52_DCLASS_SW_CTL3_RSRV2_MASK			(0x1    <<  6) /* bit   6      */
#define DP52_DCLASS_SW_CTL3_SEL_DIG_MASK		(0x3    <<  4) /* bit   5 :  4 */
#define DP52_DCLASS_SW_CTL3_RSRV3_MASK			(0x1    <<  2) /* bit   2      */
#define DP52_DCLASS_SW_CTL3_SEL_ANA_MASK		(0x3    <<  0) /* bits  1  : 0 */

/* End Sound-related register-field masks */
/* -------------------------------------- */


#ifdef CONFIG_DMW_DP52
int dp52_init(void);
int dp52_late_init(void);

int dp52_direct_write( unsigned int reg, unsigned int value );
int dp52_direct_read( unsigned int reg );
int dp52_indirect_read( unsigned int reg );
int dp52_indirect_write( unsigned int reg, unsigned int value );

void dp52_direct_write_field(int reg, int offset, int size, unsigned int value);
int dp52_direct_read_field(int reg, int offset, int size);
void dp52_aux_enable(unsigned int bandgap);
int dp52_compute_initial_gain_attn(int vref, int *gain, int *attn, int exact);
int dp52_get_calibrated_parameters(int *gain, int *attn);
void dp52_auxcmp_enable(int dcin, int gain, int attn);
void dp52_auxcmp_disable(int dcin);
int dp52_auxadc_measure(int dcin);

void dp52_power_off(void);
#else
static inline int  dp52_init(void) { return 0; }
static inline int  dp52_late_init(void) { return -ENODEV; }
static inline int  dp52_direct_write(unsigned int reg, unsigned int value) { return -ENODEV; }
static inline int  dp52_direct_read(unsigned int reg) { return -ENODEV; }
static inline int  dp52_indirect_read(unsigned int reg) { return -ENODEV; }
static inline int  dp52_indirect_write(unsigned int reg, unsigned int value) { return -ENODEV; }
static inline void dp52_direct_write_field(int reg, int offset, int size, unsigned int value) { }
static inline int  dp52_direct_read_field(int reg, int offset, int size) { return -ENODEV; }
static inline void dp52_aux_enable(unsigned int bandgap) { }
static inline int  dp52_compute_initial_gain_attn(int vref, int *gain, int *attn, int exact) { return -ENODEV; }
static inline int  dp52_get_calibrated_parameters(int *gain, int *attn) { return -ENODEV; }
static inline void dp52_auxcmp_enable(int dcin, int gain, int attn) { }
static inline void dp52_auxcmp_disable(int dcin) { }
static inline int  dp52_auxadc_measure(int dcin) { return -ENODEV; }
static inline void dp52_power_off(void) { }
#endif

#if defined(CONFIG_DMW_DP52) && defined(CONFIG_DMW_CHARGER)
void dp52_charge(int turn_off_charger_out, unsigned int is_fast_charge);
int dp52_check_battery(void);

static inline int dp52_charger_connected(void)
{
	return dp52_direct_read(DP52_PMURAWSTAT) & (1 << 1);
}
#else
static inline void dp52_charge(int turn_off_charger_out, unsigned int is_fast_charge) { }
static inline int dp52_check_battery(void) { return 0; }
static inline int dp52_charger_connected(void) { return 0; }
#endif

#endif /* __DP52_H__ */
