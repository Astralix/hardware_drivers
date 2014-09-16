#include <stdio.h>
#include <string.h>
#include "common.h"

void dw74_help_cmu(void);

void strlwr( char* string )
{

	char *ret = string;
	if(string)
	{
		for( ;*string; ++string)
		{
			*string = tolower((unsigned char)*string);
		}
	}

//	while ( 0 != ( *string++ = (char)tolower( *string ) ) )		;
}

void dw74_help_cmu(void)
{
	DWT_PRINT("Csr0 ( 0x0004)\n");
	DWT_PRINT("0 - MCU_DIV_EN, 1 - etm_en	,2 - intmem_en, 3 - shmem_en\n");
	DWT_PRINT("4 - mpmc_en	, 5 - pci_en	,8 - usb_en	   ,9 - Mac_en\n");
	DWT_PRINT("10 - bb_en	,12 - ge_fifo_en,13 - ge_dma_en,14 - fc_en,15 - sim_en\n\n");
	
	DWT_PRINT("Csr1  (0x0008)\n");
	DWT_PRINT("0 - DSP_EN	,1 - slvmii_en,	2- rtc_en,	4 -Uart1_en\n");	
	DWT_PRINT("5 - Uart2_en,6 - Sbus_en,	7 - Tmr1_en,8 - Tmr2_en	\n");
	DWT_PRINT("9 - Tmr3_en	,10 - Tmr4_en	,11 - Spi_en	,12 - Tdm_en\n");
	DWT_PRINT("13 - Emaca_en,14 - Emacb_en	,15 - Ahb3_en	\n\n");

	DWT_PRINT("Lpcr (0x0018)\n");
	DWT_PRINT("0 - Xtalon_32k,	1 - SCLKSEL,	4 - Lowpwr_en,	5 - Wifi_on,	6 - Host_on\n\n");
	
	DWT_PRINT("Cdr1  (0x0020)\n");
	DWT_PRINT("0 - dpclk_en, 3:1 - dspdivsrc_sel, 10:4 - dspclkdiv\n\n");

	DWT_PRINT("Cdr2  (0x0024)\n");
	DWT_PRINT("0 - pll2_prediv_en, 8:4 - pll2_preclkdiv, 11:9 MCU_CLK_DIV, 14:12 - usbclkdiv, 15 - USBCLK_SEL\n\n");

	DWT_PRINT("Cdr3 (0x0028)\n");
	DWT_PRINT("11:0 - tdmclkdiv, 15 - TDM_CLK_MSRT_SLV\n\n");

	DWT_PRINT("Rstsr    (0x0040)\n");
	DWT_PRINT("11:8 - UDS[3..0], 5 - Swrstind, 4 - Wdrstind, 2 - WIFIENSTAT, 1 - DECTENSTAT, 0 - BOOTSEL\n\n");

	DWT_PRINT("OSCR (0x60)\n");
	DWT_PRINT("15:10 - OSCR_KEY, 8 - DIGITAL_32K, 7 - OSC12EN, 6 - WIFIEN\n");
	DWT_PRINT("5 - OSCEN, 4 - OSCSTAT	, 3:0 - OSCSW	\n\n");
	
	DWT_PRINT("CSR2 (0x64)\n");
	DWT_PRINT("8 - BMP_EN, 7 - USB2_EN, 6 - SDMMC_EN , 5 - MS_EN\n");
	DWT_PRINT("4 - LCDC_EN , 3 - OSDM_EN, 2 - SCALER_EN  , 1 - JPEGW_EN, 0 - MAC_SLOW_EN\n\n");

	DWT_PRINT("CDR6 (0x80)\n");
	DWT_PRINT("1 - 240_CLK_SRC_SEL, 0 - 240MODE_EN\n");
	
		
	
/*

	
	
 	

	
	PLL3CR0 (0x68)
12:8 - PLL3_R	
7:0 - PLL3_F	
	
	PLL3CR1 ( 0x6c)
9:8 -PLL3_OD	
3 - PLL3_OE	
2 - PLL3_BP	
1 - PLL3_SRC_SEL	
0 - PLL3_EN	
	
	PLL4CR1 (0x74)
8 PLL4_OD	
3 PLL4_OE	
2 PLL4_BP	
1 PLL4_SRC_SEL	
0 PLL4_EN	
	
	CDR4 (0x78)
15 MS_CLKDIV_EN	
14:12 MS_CLKDIV_VAL	
11 LCDC_DIV_BP_EN	
10:9 LCDC_DIV_SRC_SEL	
8 LCDC_CLKDIV_EN	
7:0 LCDC_CLKDIV_VAL	
	
	CDR5 (0x7c)
12 - PLL4_CLKDIV_EN	
10:8 PLL4_CLKDIV_VAL	
7 - SDMMC_DIV_BP_EN	
5 - SDMMC_DIV_SRC_SEL	
	
*/
}
void dw74_help_syscfg()
{
	DWT_PRINT("REG GCR0	(0x00)\n");
	DWT_PRINT("0 -USBPHYEN, 2 - MEMA21_EN, 3 - MEMA22_EN, 4 - MEMA23_EN\n");
	DWT_PRINT("5 - MEMA24_EN, 7:6 - ROM_DELAY_SEL, 8 - ROM_DELAY_DIS, 9 - MEM_KE_EN\n");
	DWT_PRINT("10 - NFLD0TO7_KE_EN, 11 - NFLD8TO15_KE_EN, 12 - TESTBUS_EN, 13 - BTIF_EN\n");
	DWT_PRINT("14 - DPCLK_EN, 15 _MEMCS3_EN\n\n");

	DWT_PRINT("REG GCR1	(0x04)\n");
	DWT_PRINT("12 - DSP_SIO1_BER_EN, 11 -DISABLE_SPI_CS, 10 - DECT_LOC, 9 - DECT_TRIG\n");
	DWT_PRINT("8 - CS3_MODE, 3 - DSP_TDM_EN, 2 - SPC_OSC32K_LOW_I\n");
}

void dw74_grep_reg (char *find_string)
{
	FILE *fp;
	char string1[0xff];
	char module[0xff];
	int in_reg;

	in_reg = 0;

	fp = fopen ("dw74_regs.txt","rt");
	if (fp ==NULL)
	{
		DWT_PRINT("Didn't find DW74 Register text file\n");
		return ;
	}
	while (fgets(string1, sizeof (string1),fp))
	{
		strlwr (string1);
		strlwr (find_string);
		if (strstr (string1,"module:"))
		{
			strcpy(module,string1);
		}
		if (strchr(string1,'(') && strstr (string1,find_string))
		{
			in_reg = 1;
			DWT_PRINT("%s",module);
			DWT_PRINT("%s",string1);
		}
		else
		{
			if (strchr(string1,'('))
			{
				in_reg = 0;
			}
		}
		if (in_reg == 0x1)
		{
			DWT_PRINT("%s",string1);
		}
	};
	fclose (fp);
}


void dw74_grep (char *find_string)
{
	FILE *fp;
	char line1[0xff];
	char line2[0xff];
	char line3[0xff];
	char line4[0xff];
	char line5[0xff];

	char string1[0xff];
	char module[0xff];
	int in_reg;

	in_reg = 0;

	fp = fopen ("dw74_bits.txt","rt");
	if (fp ==NULL)
	{
		DWT_PRINT("Didn't find DW74 bits text file\n");
		return ;
	}
	while (fgets(string1, sizeof (string1),fp))
	{
		strlwr (string1);
		strlwr (find_string);
		
		strcpy(line5,line4);
		strcpy(line4,line3);
		strcpy(line3,line2);
		strcpy(line2,line1);
		strcpy(line1,string1);

		if (strstr (string1,find_string))
		{
			DWT_PRINT("\n\n\n");

			DWT_PRINT("%s",line5);			
			DWT_PRINT("%s",line4);			
			DWT_PRINT("%s",line3);			
			DWT_PRINT("%s",line2);			
			DWT_PRINT("%s",line1);			

			fgets(string1, sizeof (string1),fp);
			DWT_PRINT("%s",string1);
			fgets(string1, sizeof (string1),fp);
			DWT_PRINT("%s",string1);
			fgets(string1, sizeof (string1),fp);
			DWT_PRINT("%s",string1);
			fgets(string1, sizeof (string1),fp);
			DWT_PRINT("%s",string1);
			fgets(string1, sizeof (string1),fp);
			DWT_PRINT("%s",string1);

		}
	};
	fclose (fp);
}

