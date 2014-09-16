/*
 *  DMW UART support
 *
 *  Speed and options are hardcoded to 115200 8N1.
 *
 *  (C) Copyright 2006-2007, 2010, DSP Group
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <config.h>
#include <common.h>
#include <command.h>
#include <environment.h>
#include <asm/byteorder.h>
#include <asm/io.h>
#include <asm/arch/gpio.h>

#include "dmw.h"
#include "clock.h"
#include "pads.h"

#ifdef CFG_DSPG_DMW_SERIAL

#define DMW_UART_FIFO_SIZE	(128)

#define DMW_UART_CTL		0x00
#define DMW_UART_CFG		0x04
#define DMW_UART_INT_DIV	0x08
#define DMW_UART_FRAC_DIV	0x0C
#define DMW_UART_TX_FIFO_WM	0x10
#define DMW_UART_RX_FIFO_WM	0x14
#define DMW_UART_FIFO_INTSTAT	0x18
#define DMW_UART_FIFO_INTEN	0x1C
#define DMW_UART_FIFO_INT_CLR	0x20
#define DMW_UART_TX_FIFO_LVL	0x24
#define DMW_UART_RX_FIFO_LVL	0x28
#define DMW_UART_TX_DAT		0x2C
#define DMW_UART_RX_DAT		0x30
#define DMW_UART_STAT		0x34

#define RX_EMPTY		(1<<8)
#define TX_FULL			(1<<1)

#include <common.h>
#include "keypad.h"
#ifdef CONFIG_BOOTMENU
#include "bootmenu.h"
#endif

#define BAUDRATE 115200

static void dmw_serial_putc(char c);
static int  dmw_serial_getc(void);
static int  dmw_serial_tstc(void);

static int dmw_uart_base = DMW_UART1_BASE;

void platform_serial_init(void)
{
	serial_init();
}

int serial_init(void)
{
	unsigned int intgr, frac;
	unsigned int dmw_curr_clk_rate;

#if CONFIG_USE_UART == 2
	dmw_uart_base = DMW_UART2_BASE;

	reset_release("uart2");
	clk_enable("uart2");

	gpio_disable(BGPIO(4));  /* TX */
	gpio_disable(BGPIO(5));  /* RX */

	pads_enable("uart2");
#elif CONFIG_USE_UART == 3
	dmw_uart_base = DMW_UART3_BASE;

	reset_release("uart3");
	clk_enable("uart3");

	gpio_disable(BGPIO(8));  /* TX? */
	gpio_disable(BGPIO(9));  /* RX? */

	pads_enable("uart3");
#else
	reset_release("uart1");
	clk_enable("uart1");

	gpio_disable(BGPIO(0));  /* TX */
	gpio_disable(BGPIO(1));  /* RX */

	pads_enable("uart1");
#endif

	dmw_curr_clk_rate = dmw_get_pclk();

	intgr = (dmw_curr_clk_rate / (CONFIG_BAUDRATE << 4));
	frac = (dmw_curr_clk_rate / CONFIG_BAUDRATE) & 0xf;

	writel(0, dmw_uart_base + DMW_UART_CTL); /* disable */
	/* data length: 8 bits; enable break detection logic */
	writel(1<<11, dmw_uart_base + DMW_UART_CFG);
	writel(intgr, dmw_uart_base + DMW_UART_INT_DIV);
	writel(frac, dmw_uart_base + DMW_UART_FRAC_DIV);
	writel((1<<12)|(1<<9)|(1<<8)|1, dmw_uart_base + DMW_UART_FIFO_INT_CLR);
	writel(DMW_UART_FIFO_SIZE/2, dmw_uart_base + DMW_UART_TX_FIFO_WM);
	writel(DMW_UART_FIFO_SIZE/2, dmw_uart_base + DMW_UART_RX_FIFO_WM);
	writel(1, dmw_uart_base + DMW_UART_CTL); /* enable */

	return 0;
}

void serial_putc(const char c)
{
	if (c == '\n')
		dmw_serial_putc('\r');

	dmw_serial_putc(c);
}

void serial_puts(const char *s)
{
	while (*s)
		serial_putc(*s++);
}

int serial_getc(void)
{
	return dmw_serial_getc();
}

int serial_tstc(void)
{
	return dmw_serial_tstc();
}

void serial_setbrg(void)
{
}

static void dmw_serial_putc(char c)
{
	while (readl(dmw_uart_base + DMW_UART_STAT) & TX_FULL)
		;

	writel(c, dmw_uart_base + DMW_UART_TX_DAT);
}

static int dmw_serial_tstc(void)
{
	if (!(readl(dmw_uart_base + DMW_UART_STAT) & RX_EMPTY))
		return 1;
#ifdef CONFIG_USB_TTY
	extern void usbtty_poll(void);
	usbtty_poll();
#endif
#ifdef CONFIG_BOOTMENU
	dmw_bootmenu_hook(0);
#endif
	dmw_keypad_debug();

	return 0;
}

static int dmw_serial_getc(void)
{
	unsigned int data;

	while (dmw_serial_tstc() == 0)
		;

	data = readl(dmw_uart_base + DMW_UART_RX_DAT);
	if (data & 0x600) /* parity or frame error */
		return -1;

	data &= 0xff; /* limit to 8 characters */

	return (int)data;
}
#endif /* CFG_DSPG_DMW_SERIAL */
