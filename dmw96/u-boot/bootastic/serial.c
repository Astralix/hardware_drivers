#include "common.h"
#include "hardware.h"
#include "serial.h"
#include "cmu.h"
#include "romapi.h"
#include "syscfg.h"
#include "gpio.h"

#define UART_BASE			DMW96_UART1_BASE

#define UART_CTL			0x00
#define UART_CFG			0x04
#define UART_INT_DIV			0x08
#define UART_FRAC_DIV			0x0c
#define UART_TX_FIFO_WM			0x10
#define UART_RX_FIFO_WM			0x14
#define UART_FIFO_ICR			0x20
#define UART_TX_FIFO_LVL		0x24
#define UART_RX_FIFO_LVL		0x28
#define UART_TX_DATA			0x2c
#define UART_RX_DATA			0x30
#define UART_STAT			0x34

#define UART_CFG_8N1			((1 << 11) | (0 << 2))


void serial_enable(void)
{
	writel(1, UART_BASE + UART_CTL);
}

void serial_disable(void)
{
	writel(0, UART_BASE + UART_CTL);
}

void serial_init(unsigned int baudrate)
{
	unsigned int pclk = cmu_get_pclk();
	unsigned int integer, fraction;

	reset_release(BLOCK_UART1);
	clk_enable(BLOCK_UART1);

	if (!readl(UART_BASE + UART_CTL)) {
		pad_enable(PAD_UART1);
		gpio_disable(BGPIO(0));
		gpio_disable(BGPIO(1));
		writel(UART_CFG_8N1, UART_BASE + UART_CFG);
	} else {
		serial_disable();
	}

	integer = (pclk / baudrate) >> 4;
	fraction = (pclk / baudrate) & 0xf;

	writel(integer,  UART_BASE + UART_INT_DIV);
	writel(fraction, UART_BASE + UART_FRAC_DIV);

	serial_enable();
}

void serial_putc_hw(const char c)
{
	/* wait until there is space in TX FIFO */
	while (readl(UART_BASE + UART_TX_FIFO_LVL) == 0x80)
		;

	writel(c, UART_BASE + UART_TX_DATA);
}

void serial_putc(const char c)
{
	if (c == '\n')
		serial_putc_hw('\r');

	serial_putc_hw(c);
}

void serial_puts(const char *s)
{
	while (*s)
		serial_putc(*s++);
}

void serial_exit(void)
{
	/* UART running? */
	if (!readl(UART_BASE + UART_CTL))
		return;

	/* wait until the TX FIFO is emptied */
	while (readl(UART_BASE + UART_TX_FIFO_LVL) > 0)
		;

	/* the last character is still being shifted out */
	romapi->udelay(100);

	serial_disable();
}
