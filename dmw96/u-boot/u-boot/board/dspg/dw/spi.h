#ifndef __BOARD_DSPG_DW_SPI_H
#define __BOARD_DSPG_DW_SPI_H

#define DW_SPI_CFG     0x05E00000
#define DW_SPI_INT_EN  0x05E00004
#define DW_SPI_INTSTAT 0x05E00008
#define DW_SPI_INT_CLR 0x05E0000C
#define DW_SPI_TXBUF   0x05E00010
#define DW_SPI_RXBUF   0x05E00014

void spi_read(unsigned char *buf, int len);
void spi_write(unsigned char *buf, int len);

/* implemented by the board */
void board_spi_init(void);

#endif
