#include <stdio.h>
#include "common.h"
#include "dp.h"

#define THREE_CELL 0
#define LION	   1

#define LI_PRECHARGE 	0x1
#define LI_FASTCHARGE 	0x2
#define LI_CONSVOLT		0x3

extern uint16 dp_read_reg (uint16 addr);
extern uint16 dp_write_reg (uint16 addr, uint16 data);
extern uint16 dp_clock_enable ();

//extern void DWT_PRINT(const char *string);

void dp_ldo_open(unsigned int ldo, unsigned int on_offn, float voltage);
void dp_sdout(uint16 direction, uint16 on_offn);
void dp_out_gain(uint16 direction, uint16 gain);
void dp_afeout_connect(uint16 direction,uint16 input);
void dp_headset_en(unsigned short on_offn);
void dp_line_en(uint16 on_offn);
void dp_micpwr_en(uint16 on_offn);
void dp_handset_en(uint16 on_offn);
void dp_mic_en(uint16 on_offn);
void dp_handset_gain(uint16 gain);
void dp_mic_gain(uint16 gain);
//void dp_aux_en(uint16 module);
void dp_mux(uint16 module);
void dp_aux_sample();
void dp_aux_clk(uint16 clock_source);
void dp_pwm_on(unsigned short pwm_num, unsigned short pwm_width);
void dp_pwm_off(unsigned short pwm_num);
void dp_keypad_off();
void dp_keypad_on(unsigned short pwm_width);
unsigned int dp_rtc_get_ticks();
unsigned int dp_rtc_activate();
unsigned int dp_rtc_set_alarm(unsigned int ticks);
void dp_hq_set(unsigned short mode);
void dp_headout_vol(unsigned short volume);
void dp_vref(unsigned short on_offn, unsigned short voltage);
void dp_classd_on(void);
void dp_classd_off(void);
void dp_aux_en(void);
unsigned short dp_read_aux();
void dp_aux_mux(unsigned short mux);
void dp_charger_init(unsigned short type);
void dp_ts_init();
unsigned short dp_ts_measure_x1();
unsigned short dp_ts_measure_x2();
unsigned short dp_ts_measure_y1();
unsigned short dp_ts_measure_y2();
unsigned short dp_bandgap_tune();
unsigned short dp_comparator_tune();
unsigned short dp_lqsd_en();


unsigned short dp_lqsd_en()
{

//CLOCK CONFIG
	dp_write_reg(PMU_SW_LVL3 , 0x99);
	dp_write_reg( CMU_PLLCR, 0xc06e);
	dp_clr_bits (CMU_CCR1, 0x803f);	//SDCLKSEL = FDSP, LQDIV = 1
	dp_set_bits (CMU_CCR0, 0x2000); //LQSDCLK Enable
	dp_set_bits(CMU_SYSCFG, 5); //	I2S Enable
	dp_write_reg (SDS_CFG, 0x7808);
//AFE CONFIG.
	dp_set_bits (PMU_SW_EN, 0x4);
	dp_set_bits (AFESDN, 0x10 );
	dp_write_reg (AFEMIGC, 0xf0f);	//MICGain = 26db, MICGAIN = 26db + 30db
	dp_write_reg (AFESDN, 0x1A);
	dp_write_reg (AFEIN, 0x31B);
	dp_write_reg (AFEOUT_CTL3, 0xC);
	dp_write_reg (AFEOUT_CTL2, 0X21);
	dp_write_reg (AFEOUT_CTL1 , 0X3);//SDOUTR_EN=SDOUTL_EN=1
	dp_set_bits (CMU_SYSCFG, 0x9);
//LQSD CONFIG
	dp_write_reg (SDCFG1, 0x3);//Enable transmit + receive path for the Codecs
	dp_write_reg (SDCFG2, 0x0);
	return 0;
}

void dp_tp_init()
{
	dp_vref(1, 0x0);
	dp_aux_en();
	dp_aux_mux(6);
	dp_write_reg(AUX_PMTPCTL2, 1);
	
}

unsigned short dp_tp_measure_x1()
{
	dp_write_reg (AUX_PMTPCTL1, 0xac08);
	return dp_read_aux();
}

unsigned short dp_tp_measure_x2()
{
	dp_write_reg (AUX_PMTPCTL1, 0xac40);
	return dp_read_aux();
}

unsigned short dp_tp_measure_y1()
{
	dp_write_reg (AUX_PMTPCTL1, 0x82b0);
	return dp_read_aux();
}

unsigned short dp_tp_measure_y2()
{
	dp_write_reg (AUX_PMTPCTL1, 0x90b0);
	return dp_read_aux();
}

unsigned short dp_bandgap_tune()
{
	int i;
	int t;

	dp_aux_en();
	dp_aux_mux(0x8);
	dp_write_reg(AUX_DC0GAINCNT1, 0x7e1);
	dp_clr_bits (AUX_ADCTRL,0xff00);
	dp_set_bits (AUX_EN,0x10);
	dp_set_bits (AUX_PMITMSKN,0x1);

	if (dp_read_reg(AUX_RAWPMINTSTAT)&0x1!=0x1)
	{
		return -1;
	}
	for (i=0;i<0xff;i++)
	{
		t = dp_read_aux();
		if (t<0x3fff)
			break;
/*		t = dp_read_reg(AUX_RAWPMINTSTAT) & 0x1;
		if ( t == 0x0)
		{
			break;
		}*/
		dp_clr_bits (AUX_ADCTRL,0xff00);
		dp_set_bits (AUX_ADCTRL,i<<8);
	}
	if (i==0xff)
	{
		return -2;	
	}
	else
	{
		return 	dp_read_aux();
	}
}


unsigned short dp_comparator_tune()
{
	int i;
	int t;

	dp_aux_en();
	dp_aux_mux(0x8);
	dp_set_bits (AUX_EN,0x80);
	dp_set_bits (AUX_DC3GAINCNT1,0x1);

	if (dp_read_reg(AUX_RAWPMINTSTAT)&0x8!=0x8)
	{
		return -1;
	}
	for (i=0;i<0xff;i++)
	{
		t = dp_read_reg(AUX_RAWPMINTSTAT) & 0x8;
		if ( t == 0x8)
		{
			break;
		}
		dp_clr_bits (AUX_DC3GAINCNT1,0x3f<<5);
		dp_set_bits (AUX_DC3GAINCNT1,i<<5);
		
	}
	if (i==0xff)
	{
		return -2;	
	}
	else
	{
		return 	i;
	}
}


void dp_charger_init(unsigned short type)
{
	//	Enable the DCINS interrupt, check if power jack is inserted.
	dp_clr_bits (PMU_STAT,0xffff);
	dp_clr_bits (PMU_STAT,0xffff);
	dp_clr_bits (AUX_PMINTSTAT,0xffff);
	dp_clr_bits (INT_STAT,0x8);//Enable comparator interrupts
	dp_set_bits (AUX_PMITMSKN,0x1); //Enable the DCINS interrupt
	dp_set_bits (PMU_INTMSK,0x2);
	dp_set_bits (INT_MASK,0X9);
	//	Open the analog init	
	dp_vref(1, 0x0);
	dp_set_bits (AUX_EN, 0x83);
	dp_write_reg(AUX_DC3GAINCNT1, 0x5a1);
	dp_write_reg(PWM0_CFG1, 0x107);
	dp_write_reg (PWM0_CFG2,0X2000);
	dp_write_reg (PWM0_CRG_TIMEOUT, 0x4);
	//Tune the charge timer to ~3.8seconds.
	dp_write_reg (PWM0_CRGTMR_CFG,0x3);
	dp_write_reg (PWM0_CRGTMR_CFG,0x183);

	//@@@ To be changed, should be read from somewhere in the flash
	dp_clr_bits (AUX_ADCTRL,0xff00);
	dp_set_bits (AUX_ADCTRL,0x6f<<8);
}


void dp_aux_en()
{
	dp_set_bits (CMU_CCR2, 0x1);
	dp_set_bits (CMU_CCR0, 0x800);
 	dp_set_bits (AUX_EN, 0x3);
	dp_write_reg (AUX_ADCTRL, 0x660e );
	dp_write_reg (AUX_ADCFG, 0x8);
}
void dp_aux_mux(unsigned short mux)
{
	dp_clr_bits (AUX_ADCFG , 0x1f);
	dp_set_bits (AUX_ADCFG , mux);
}

unsigned short dp_read_aux()
{	
	int n = 200;
	dp_set_bits (AUX_ADCTRL, 0x1);
	while (dp_read_reg(AUX_ADCTRL)&0x1)
	{ n--;
		if (n==0)break;
	};
	return dp_read_reg(AUX_ADDATA) & 0x7fff; 
}

void dp_classd_on(void)
{
	dp_write_reg(DCLASS_CONFIG , 0x2b0);
	dp_write_reg(0x25, 0x81f0);
}
void dp_classd_off(void)
{
	dp_write_reg(DCLASS_CONFIG, 0x90);
	dp_write_reg(DCLASS_SW_CTRL1, 0x0);
}

void dp_pwm_on(unsigned short pwm_num, unsigned short pwm_width)
{
	dp_set_bits (CMU_CCR2,0x800);

	switch (pwm_num)
	{
		case 0:
			dp_write_reg(PWM0_CFG1,0x4000 | pwm_width );
			dp_set_bits(PWM0_CFG2, 0x2000);
			break;
		case 1:
			dp_write_reg(PWM1_CFG1,0x4000 | pwm_width );
			dp_set_bits(PWM1_CFG2, 0x2000);
			break;
		case 2:
			dp_write_reg(PWM2_CFG1,0x4000 | pwm_width );
			dp_set_bits(PWM2_CFG2, 0x2000);
			break;
		default:
			break;
	}
}
void dp_pwm_off(unsigned short pwm_num)
{
	switch (pwm_num)
	{
		case 0:
			dp_clr_bits(PWM0_CFG2, 0x2000);
			break;
		case 1:
			dp_clr_bits(PWM1_CFG2, 0x2000);
			break;
		case 2:
			dp_clr_bits(PWM2_CFG2, 0x2000);
			break;
	}
}

void dp_keypad_on(unsigned short pwm_width)
{
	dp_set_bits (CMU_SYSCFG, 0x2);	
	dp_pwm_on(0x2, pwm_width);	
}

void dp_keypad_off()
{
	dp_clr_bits (CMU_SYSCFG, 0x2);	
	dp_pwm_off(0x2);	
}

void dp_hq_set(unsigned short mode)
{
	//Vref On, Vref = 1.5V
	dp_vref(1, 0x0);
	dp_headset_en(1);
	dp_write_reg (AFEOUT_CTL2,0x43);
	dp_set_bits (AFEOUT_CTL1, SDOUTL_EN | SDOUTR_EN);
	dp_set_bits (PMU_SW_EN, LDO_DP_IO_EN); 
	dp_set_bits (CMU_SYSCFG, I2S_EN | 0X4);
	
	DWT_PRINT("mode: %d\n",mode);
	switch (mode)
	{
		case PCM_44100_17:
			dp_write_reg (CMU_PLLCR, 0xc06e);
			dp_clr_bits (CMU_CCR1,SDCLKSEL);
			dp_set_bits (CMU_CCR0, PLLEN | HQSDCLK_EN);
			dp_clr_bits (CMU_CCR0, HQDIV |LQSDCLK_EN);
			dp_set_bits (CMU_CCR0, 0x1);
			dp_clr_bits (SDS_CFG, NH_COUNT | NS_COUNT);
			dp_set_bits (SDS_CFG, SDS_C_SHFT | SD_DACR_EN | SD_DACL_EN | 0x202);

			DWT_PRINT("set PCM_44100_17\n");
			break;
		case PCM_48000_20:
			break;
		case PCM_8000_20:
			break;
		default:
			DWT_PRINT("PCM format not supported");
			break;
	}
}


unsigned int dp_rtc_get_ticks()
{	unsigned int ticks;

	DWT_PRINT("dp_rtc_get_ticks\n");
	dp_set_bits (RTC_DRM_5, RTC_SAMPLE);
	dp_set_bits (0x6c, 0x8000);
	ticks = dp_read_reg(RTC_DRM_1)<<11;
	ticks = ticks | dp_read_reg(RTC_DRM_2)<<27;

	return ticks;
}
unsigned int dp_rtc_activate()
{
	dp_set_bits (DP_RTC_BASE + RTC_DRM_1, RTC_VALID);

	return 0;
}

unsigned int dp_rtc_set_alarm(unsigned int ticks)
{
	DWT_PRINT ("Ticks 0x%x\r\n", ticks);
	dp_clr_bits (RTC_DRM_5, 0x800);
	dp_clr_bits (RTC_DRM_5, 0x2000);
	dp_clr_bits (INT_STAT, 0x10);

	dp_write_reg (RTC_DRM_4, ticks>>11);
	DWT_PRINT ("Alarm high 0x%x\r\n", ticks>>27);
	dp_write_reg (RTC_DRM_3, ticks>>27);
	dp_set_bits (RTC_DRM_5, 0x1800);
	dp_set_bits (INT_MASK, 0x10);
	return 0;
}



void dp_ldo_open(unsigned int ldo, unsigned int on_offn, float voltage)
{
	DWT_PRINT("ldo %d on_offn %d\n",ldo,on_offn);
	switch (ldo)
	{
		case LDO_SP1:
			if (on_offn == 0x1)
			{
				dp_set_bits (PMU_SW_EN,0x40);
			}
			else
			{
				dp_clr_bits (PMU_SW_EN,0x40);
			}
			break;
		case LDO_SP2:
			if (on_offn == 0x1)
			{
				dp_set_bits (PMU_SW_EN,0x2);
			}
			else
			{
				dp_clr_bits (PMU_SW_EN,0x2);
			}
			break;
		case LDO_SP3:
			if (on_offn == 0x1)
			{
				dp_set_bits (PMU_SW_EN,0x0);
			}
			else
			{
				dp_clr_bits (PMU_SW_EN,0x0);
			}
			break;
		case LDO_IO:
			if (on_offn == 0x1)
			{
				dp_set_bits (PMU_SW_EN,1<<5);
			}
			else
			{
				dp_clr_bits (PMU_SW_EN,1<<5);
			}
			break;
		case LDO_CORE0:
			if (on_offn == 0x1)
			{
				dp_set_bits (PMU_SW_EN,1<<8);
			}
			else
			{
				dp_clr_bits (PMU_SW_EN,1<<8);
			}

			break;
		case LDO_CORE1:
			break;
		case LDO_MEM:
			if (on_offn == 0x1)
			{
				dp_set_bits (PMU_SW_EN,1<<7);
			}
			else
			{
				dp_clr_bits (PMU_SW_EN,1<<7);
			}
			break;
		case LDO_DPIO:
			if (on_offn == 0x1)
			{
				dp_set_bits (PMU_SW_EN,1<<2);
			}
			else
			{
				dp_clr_bits (PMU_SW_EN,1<<2);
			}
			break;
		default:
			break;
	}
}


void dp_vref(unsigned short on_offn, unsigned short voltage)
{
	if (on_offn==0x1)
	{
		DWT_PRINT("set vref\n");
		dp_clr_bits(AFESDN,0x7<<5);
		dp_set_bits(AFESDN,(voltage&7)<<5);
		dp_set_bits(AFESDN,VREFEN);

	}
	else
	{
		dp_clr_bits(AFESDN,1<<4);
	}
}
void dp_out_gain(uint16 direction, uint16 gain)
{
	gain = gain & 0x1f;
	switch (direction)	
	{
		case LEFT:
			dp_clr_bits (AFEOUT_CTL1, 0X1F<<7);
			dp_set_bits (AFEOUT_CTL1,gain<<7);
			break;
		case RIGHT:
			dp_clr_bits (AFEOUT_CTL1, 0X1F<<2);
			dp_set_bits (AFEOUT_CTL1,gain<<2);
			break;
		default:
			break;
	}
}

void dp_sdout(uint16 direction, uint16 on_offn)
{
	switch (direction)
	{
		case LEFT:
			if (on_offn == 1)
				{	dp_set_bits (AFEOUT_CTL1, 1);}
			else
				{	dp_clr_bits (AFEOUT_CTL1, 1);}

			break;
		case RIGHT:
			if (on_offn == 1)
				{	dp_set_bits (AFEOUT_CTL1, 1<<1);}
			else
				{	dp_clr_bits (AFEOUT_CTL1, 1<<1);}

			break;
	}
	
}
void dp_afeout_connect(unsigned short direction,unsigned short input)
{
	uint16 value;

	DWT_PRINT("dp_afeout_connect dir: %d, input %d",direction, input);
	switch(input)
	{
		case MUTE:
			value = 0x0;
			break;
		case LQDAC:
			value = 0x1;
			break;
		case LQSDO_L:
			value = 0x2;
			break;
		case LQSDO_R:
			value = 0x3;
			break;
		case INVMIC:
			value = 0x4;
			break;
		case INVHSMIC:
			value = 0x5;
			break;
		case MIC:
			value = 0x6;
			break;
		case HSMIC:
			value = 0x7;
			break;
		default:
			return ;
	}
	if (direction == RIGHT)
	{
		dp_clr_bits (AFEOUT_CTL2,0x1f);
		dp_set_bits (AFEOUT_CTL2, value);
	}
	else if (direction == LEFT)
	{
		dp_clr_bits (AFEOUT_CTL2,0x1f<<5);
		dp_set_bits (AFEOUT_CTL2, value<<5);
	}
	else
	{
		dp_clr_bits (AFEOUT_CTL2,0x1f);
		dp_set_bits (AFEOUT_CTL2, value);
		dp_clr_bits (AFEOUT_CTL2,0x1f<<5);
		dp_set_bits (AFEOUT_CTL2, value<<5);
	}

}

void dp_headset_en(unsigned short on_offn)
{
	if (on_offn == 1)
	{
		dp_set_bits (AFEOUT_CTL3, 3<<2);	
	}
	else
	{
		dp_clr_bits (AFEOUT_CTL3, 3<<2);	

	}
}

void dp_mic_en(uint16 on_offn)
{
	if (on_offn == 1)
	{
		dp_set_bits (AFESDN, 1<<3);	
	}
	else
	{
		dp_clr_bits (AFESDN, 1<<3);	

	}
}
void dp_headout_vol(unsigned short volume)
{
	volume = volume & 0x1f;
	if (volume > 0x18) volume = 0x18;
	dp_clr_bits (AFEOUT_CTL1,0x1f<<7 | 0x1f<<2);
	dp_set_bits (AFEOUT_CTL1, volume<<7 | volume<<2);
}

void dp_handset_en(uint16 on_offn)
{
	if (on_offn == 1)
	{
		dp_set_bits (AFESDN, 1<<2);	
	}
	else
	{
		dp_clr_bits (AFESDN, 1<<2);	
	}
}

void dp_micpwr_en(uint16 on_offn)
{
	if (on_offn == 1)
	{
		dp_set_bits (AFESDN, 1<<1);	
	}
	else
	{
		dp_clr_bits (AFESDN, 1<<1);	

	}
}
void dp_line_en(uint16 on_offn)
{
	if (on_offn == 1)
	{
		dp_set_bits (AFESDN, 1<<0);	
	}
	else
	{
		dp_clr_bits (AFESDN, 1<<0);	

	}
}

void dp_mic_gain(uint16 gain)
{
	gain = gain & 0x1f;
	dp_clr_bits (AFEMIGC,0x1f<<8);
	dp_set_bits (AFEMIGC,gain<<8);
}
void dp_handset_gain(uint16 gain)
{
	gain = gain & 0x1f;
	dp_clr_bits (AFEMIGC,0x1f);
	dp_set_bits (AFEMIGC,gain);
}

/*void dp_aux_en(uint16 module)
{
	dp_set_bits (AUX_EN, module);
}*/

void dp_mux(uint16 module)
{
	dp_clr_bits (AUX_ADCFG,0x1f);
	dp_set_bits (AUX_ADCFG, module);
}

void dp_aux_sample()
{
	dp_set_bits (AUX_ADCTRL,0x1);
}

void dp_aux_clk(uint16 clock_source)
{
	switch(clock_source)
	{
		case CLOCK_100K:
			dp_clr_bits (CMU_CCR0,0x3<<10);
			break;
		case CLOCK_1M:
			dp_clr_bits(CMU_CCR2,0x3);//Set 4MHz oscillator to 1M output
			dp_set_bits (CMU_CCR2,0x1);

			dp_clr_bits (CMU_CCR0,0x3<<10);
			dp_set_bits (CMU_CCR0,0x2<<10);
			break;
		case CLOCK_4M: 
		default:
			dp_clr_bits(CMU_CCR2,0x3);//Set 4MHz oscillator to 1M output
			dp_set_bits (CMU_CCR2,0x1);

			dp_clr_bits (CMU_CCR0,0x3<<10);
			dp_set_bits (CMU_CCR0,0x2<<10);
			break;
		
	}
		
}
