/*
 * DW UART support
 *
 * Hardcoded to UART 0 for now
 * Speed and options also hardcoded to 115200 8N1
 *
 * (C) Copyright 2006-2007
 * Moshe Lazarov, DSP Group, moshel@dsp.co.il
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
#include "dw.h"

#ifdef CFG_DSPG_DW_SERIAL

#include <common.h>
#include "serial.h"
#include "keypad.h"
#ifdef CONFIG_BOOTMENU
#include "bootmenu.h"
#endif

#define writel(v,p) (*(volatile unsigned long *)(p) = (v))
#define readl(a)    (*(volatile unsigned int *)(a))

static void dw_serial_putc (char c);
static int dw_serial_getc (void);
static int dw_serial_tstc (void);

static int dw_uart_base = DW_UART1_BASE;

/******************************************************************************
*
* serial_init - initialize a channel
*
* This routine initializes the number of data bits, parity
* and set the selected baud rate. Interrupts are disabled.
* Set the modem control signals if the option is selected.
*
* RETURNS: N/A
*/

int serial_init (void)
{
	unsigned int cfg = 0;
	unsigned int intgr, frac, baudval;
	unsigned int dw_curr_clk_rate;

#if CONFIG_USE_UART == 2
	dw_uart_base = DW_UART2_BASE;
#else
	if ( dw_detect_board() == DW_BOARD_IMH )
		dw_uart_base = DW_UART2_BASE;
#endif

	writel(0, IO_ADDRESS(dw_uart_base) + DW_UART_CTL);
	//Set board to 38400,8N1
	cfg = DW_UART_CFG_DATA_LENGTH_8_VAL;
	cfg |= DW_UART_CFG_BREAK_EN;

	dw_curr_clk_rate = dw_get_pclk();

	intgr = (dw_curr_clk_rate / (CONFIG_BAUDRATE << 4));
	frac = (dw_curr_clk_rate / CONFIG_BAUDRATE) & 0x000f;
	baudval = intgr|(frac << DW_UART_DIV_FRAC_RATE_LOC);

	writel(DW_UART_CTL_UART_DIS, IO_ADDRESS(dw_uart_base + DW_UART_CTL));
	writel(cfg, IO_ADDRESS(dw_uart_base) + DW_UART_CFG);
	writel(baudval, IO_ADDRESS(dw_uart_base) + DW_UART_DIV);
	writel(DW_UART_CLR_ICR, IO_ADDRESS(dw_uart_base) + DW_UART_FIFO_ICR);
	writel(DW_UART_FIFO_SIZE/2, IO_ADDRESS(dw_uart_base) + DW_UART_FIFO_AE_LEVEL); //TX FIFO
	writel(DW_UART_FIFO_SIZE/2, IO_ADDRESS(dw_uart_base) + DW_UART_FIFO_AF_LEVEL); //RX FIFO
	writel(DW_UART_CTL_UART_EN, IO_ADDRESS(dw_uart_base) + DW_UART_CTL);

	return 0;
}


void serial_putc (const char c)
{
	if (c == '\n')
		dw_serial_putc ('\r');

	dw_serial_putc (c);
}


void serial_puts (const char *s)
{
	while (*s) {
		serial_putc(*s++);
	}
}


int serial_getc (void)
{
	return dw_serial_getc();
}


int serial_tstc (void)
{
	return dw_serial_tstc();
}


void serial_setbrg (void)
{
}


static void dw_serial_putc (char c)
{
	//wait until there is space in TX FIFO
	while(readl(IO_ADDRESS(dw_uart_base) + DW_UART_TX_FIFO_LEVEL) == 0x80);

	writel(c, IO_ADDRESS(dw_uart_base) + DW_UART_TX_DATA);
}


static int dw_serial_tstc (void)
{
	int ret = 0;

	ret = !(readl(IO_ADDRESS(dw_uart_base) + DW_UART_STAT) & DW_UART_STAT_RX_EMPTY);
#ifdef CONFIG_USB_TTY
	extern void usbtty_poll(void);
	usbtty_poll();
#endif
#ifdef CONFIG_BOOTMENU
	dw_bootmenu_hook(ret);
#endif
	dw_keypad_debug();
	return ret;
}


static int dw_serial_getc (void)
{
	unsigned int data;

	//Wait until there is data in RX FIFO
	while(dw_serial_tstc() == 0);

	data = readl(IO_ADDRESS(dw_uart_base) + DW_UART_RX_DATA);

	//check for err
	if(data & DW_UART_RX_PE_FE)
		return -1;

	data &= DW_UART_RX_DATA_WORD;
	data &= 0xff; //limit to 8 caharacters

	return (int) data;
}

#endif /* CFG_DSPG_DW_SERIAL */
