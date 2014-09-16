#include <common.h>
#include <asm/io.h>
#include "dw.h"
#include "spi.h"

void spi_read(unsigned char *buf, int len)
{
	int i;

	writel(0x03, DW_SPI_INT_CLR);
	writel(0x01, DW_SPI_INT_EN);

	for (i = 0; i < len; i++) {
		writel(0x00, DW_SPI_TXBUF);
		while (!(readl(DW_SPI_INTSTAT) & 0x1))
			;
		buf[i] = readl(DW_SPI_RXBUF);
		writel(0x03, DW_SPI_INT_CLR);
	}
}

void spi_write(unsigned char *buf, int len)
{
	int i;

	writel(0x03, DW_SPI_INT_CLR);
	writel(0x01, DW_SPI_INT_EN);

	for (i = 0; i < len; i++) {
		writel(buf[i], DW_SPI_TXBUF);
		while (!(readl(DW_SPI_INTSTAT) & 0x1))
			;
		writel(0x3, DW_SPI_INT_CLR);
	}
}
