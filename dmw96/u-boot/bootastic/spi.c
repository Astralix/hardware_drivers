#include "common.h"
#include "hardware.h"
#include "cmu.h"
#include "syscfg.h"
#include "gpio.h"
#include "spi.h"

#define SPI_BASE	(DMW96_SPI1_BASE)
#define SPI_BLOCK	(BLOCK_SPI1)
#define SPI_PAD		(PAD_SPI1)

#define SPI_CFG		(SPI_BASE + 0x00)
#define SPI_RATE	(SPI_BASE + 0x04)
#define SPI_INTEN	(SPI_BASE + 0x08)
#define SPI_INTSTAT	(SPI_BASE + 0x0c)
#define SPI_INTCLR	(SPI_BASE + 0x10)
#define SPI_INTCAUSE	(SPI_BASE + 0x14)
#define SPI_TXDAT	(SPI_BASE + 0x18)
#define SPI_RXDAT	(SPI_BASE + 0x1c)
#define SPI_DELAY	(SPI_BASE + 0x20)

void spi_init(void)
{
	reset_release(SPI_BLOCK);
	clk_enable(SPI_BLOCK);

	gpio_disable(BGPIO(28)); /* sdo */
	gpio_disable(BGPIO(29)); /* sdi */
	gpio_disable(BGPIO(31)); /* sclk */

	gpio_enable(BGPIO(30)); /* cs1 */
	gpio_enable(GGPIO(21)); /* cs2 */

	/* chip selects are low-active */
	gpio_direction_output(BGPIO(30), 1);
	gpio_direction_output(GGPIO(21), 1);

	pad_enable(SPI_PAD);

	writel(0x08, SPI_RATE);
}

void spi_write(unsigned char *buf, int len)
{
	unsigned long cfg;
	int i;

	/* chip selects are controlled as GPIOs */
	cfg = (1 << 31) | /* SPI_EN */
	      (1 <<  7) | /* DIRECT_CS: CS line controlled by SW */
	      (0 <<  6) | /* SWCS: active low */
	      (1 <<  5) | /* DATA_ORD: MSB first */
	      (1 <<  3) | /* CS1_EN: disable CS1 */
	      (1 <<  1);  /* CS0_EN: disable CS0 */

	writel(cfg, SPI_CFG);

	for (i = 0; i < len; i++) {
		/* send out data */
		writel(buf[i], SPI_TXDAT);

		/* wait for the data to clock in/out */
		while (!(readl(SPI_INTSTAT) & (1<<10))) /* SPI_DONE */
			;

		/* read the byte we just got and return it */
		/* because we always read the byte, we don't have to use RX_IGNORE or RX_FIFO_FLUSH */
		readl(SPI_RXDAT);
	}

	/* Disable the SPI hardware */
	writel(0, SPI_CFG);
}

