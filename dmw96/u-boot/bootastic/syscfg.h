#ifndef __SYSCFG_H
#define __SYSCFG_H

#define PAD_LCDGP0	( 0 +  0)
#define PAD_LCDGP1	( 0 +  1)
#define PAD_LCDGP2	( 0 +  2)
#define PAD_LCDGP3	( 0 +  3)
#define PAD_LCDGP4	( 0 +  4)
#define PAD_LCDGP5	( 0 +  5)
#define PAD_LCDGP6	( 0 +  6)
#define PAD_LCDGP7	( 0 +  7)
#define PAD_LCDGP8	( 0 +  8)
#define PAD_LCDC	( 0 + 10)
#define PAD_SLVMII	( 0 + 11)
#define PAD_EMMC	( 0 + 12)
#define PAD_MS		( 0 + 13)
#define PAD_EMACA	( 0 + 14)
#define PAD_SDMMC	( 0 + 15)
#define PAD_I2C2	( 0 + 16)
#define PAD_I2C1	( 0 + 17)
#define PAD_SLVRMII	( 0 + 18)
#define PAD_SPI2	( 0 + 19)
#define PAD_SPI1	( 0 + 20)
#define PAD_TDM3	( 0 + 21)
#define PAD_TDM2	( 0 + 22)
#define PAD_TDM1	( 0 + 23)
#define PAD_UART3	( 0 + 24)
#define PAD_UART2	( 0 + 25)
#define PAD_UART1	( 0 + 26)
#define PAD_FC		( 0 + 27)
#define PAD_IR		( 0 + 28)
#define PAD_BB		( 0 + 29)
#define PAD_MEMBOOT	( 0 + 30)
#define PAD_BTIF	( 0 + 31)
#define PAD_SIM		(32 +  0)
#define PAD_DPCLK	(32 +  1)
#define PAD_DECTIF	(32 +  2)
#define PAD_CIU		(32 +  3)
#define PAD_BMP		(32 +  6)
#define PAD_WIFISER	(32 + 16)

enum dram_pin_groups {
	DRAM_PIN_GROUP0,
	DRAM_PIN_GROUP1,
	DRAM_PIN_GROUP2,
	DRAM_PIN_GROUP3,
	DRAM_PIN_GROUP4,
	DRAM_PIN_GROUP5,
	DRAM_PIN_GROUP6,
	DRAM_PIN_GROUP7,
	DRAM_PIN_GROUP8,
	DRAM_PIN_GROUP9,
	DRAM_PIN_GROUP10,
	DRAM_PIN_GROUP11,
	DRAM_PIN_GROUP12,
};

void pad_enable(unsigned int pad);
void syscfg_set_romws(unsigned int waitstates);
void syscfg_dram_set_sus(enum dram_pin_groups group, int enable);
void syscfg_dram_set_coherency_control(void);

#endif
