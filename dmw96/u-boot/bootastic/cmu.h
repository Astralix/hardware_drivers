#ifndef __CMU_H
#define __CMU_H

void cmu_setup(void);
unsigned long cmu_get_sysclk(void);
unsigned long cmu_get_pclk(void);

void reset_release(unsigned int block);
void clk_enable(unsigned int block);

#define BLOCK_ARM926     ( 0 +  1)
#define BLOCK_ETHMAC     ( 0 +  3)
#define BLOCK_SECURE     ( 0 +  4)
#define BLOCK_UART1      ( 0 +  6)
#define BLOCK_TDM1       ( 0 +  9)
#define BLOCK_TDM2       ( 0 + 10)
#define BLOCK_TDM3       ( 0 + 11)
#define BLOCK_SONYMS     ( 0 + 12)
#define BLOCK_SDMMC      ( 0 + 13)
#define BLOCK_CIU        ( 0 + 19)
#define BLOCK_LCDC       ( 0 + 21)
#define BLOCK_CSS        ( 0 + 23)
#define BLOCK_GPU        ( 0 + 24)
#define BLOCK_VIDDE      ( 0 + 25)
#define BLOCK_VIDEN      ( 0 + 26)
#define BLOCK_SIM        ( 0 + 27)
#define BLOCK_DP         ( 0 + 28)
#define BLOCK_TIMER1     (32 +  0)
#define BLOCK_SPI1       (32 +  6)
#define BLOCK_SPI2       (32 +  7)
#define BLOCK_DRAMCTL    (32 + 12)
#define BLOCK_PLL4PREDIV (32 + 19)

#endif
