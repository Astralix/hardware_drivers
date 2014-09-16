/*
 * Copyright (C) 2009 Texas Instruments Incorporated - http://www.ti.com/
 *
 * Driver for SPI controller on dmw96. Based on atmel_spi.c
 * by Atmel Corporation
 *
 * Copyright (C) 2007 Atmel Corporation
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
#include <common.h>
#include <spi.h>
#include <malloc.h>
#include <asm/io.h>
#include "dmw96_spi.h"

#define DMW_SPI1_BASE		0x05D00000
#define DMW_SPI2_BASE		0x05E00000

#define DMW_SPI_CFG_REG		0x00
#define DMW_SPI_RATE_CNTL	0x04
#define DMW_SPI_INT_EN		0x08
#define DMW_SPI_INT_ST		0x0C
#define DMW_SPI_INT_CLR		0x10
#define DMW_SPI_CAUSE_REG	0x14
#define DMW_SPI_TX_DAT		0x18
#define DMW_SPI_RX_DAT		0x1C
#define DMW_SPI_DEL_VAL		0x20

#define BIT(x)			(1 << (x))

/* SPI_CFG_REG */
#define CLKPOL_MASK				BIT(0)
#define CS0_DIS_MASK			BIT(1)
#define CS0_HOLD_MASK			BIT(2)
#define CS1_DIS_MASK			BIT(3)
#define CS1_HOLD_MASK			BIT(4)
#define DATA_ORD_MASK			BIT(5)
#define SWCS_MASK				BIT(6)
#define DIRECT_CS_MASK			BIT(7)
#define RX_IGNORE_MASK			BIT(8)
#define RX_IGNORE_N_MASK		(0xf << 9)
#define RX_TO_DURATION_MASK		(0x3 << 13)
#define RX_DMA_EN_MASK			BIT(16)
#define RX_FIFO_FLUSH_MASK		BIT(17)
#define TX_FIFO_FLUSH_MASK		BIT(18)
#define SPI_EN_MASK				BIT(31)

void spi_init()
{
	/* Nothing to do */
}

struct spi_slave *spi_setup_slave(unsigned int bus, unsigned int cs,
			unsigned int max_hz, unsigned int mode)
{
	struct dmw96_spi_slave	*ds;

	if (!spi_cs_is_valid(bus, cs))
		return NULL;

	ds = malloc(sizeof(*ds));
	if (!ds)
		return NULL;

	ds->slave.bus = bus;
	ds->slave.cs = cs;

	if (bus == 0)
		ds->regs_base = DMW_SPI1_BASE;
	else
		ds->regs_base = DMW_SPI2_BASE;

	ds->freq = max_hz;
	ds->mode = mode;

	return &ds->slave;
}

void spi_free_slave(struct spi_slave *slave)
{
	struct dmw96_spi_slave *ds = to_dmw96_spi(slave);

	free(ds);
}

int spi_claim_bus(struct spi_slave *slave)
{
	struct dmw96_spi_slave *ds = to_dmw96_spi(slave);
	unsigned int cfg;

	/* Set rate - currently fixed... */
	writel(0x08, ds->regs_base + DMW_SPI_RATE_CNTL); /* FIXME */

	/* no interrupts */
	writel(0x00, ds->regs_base + DMW_SPI_INT_EN);
	writel(0x26, ds->regs_base + DMW_SPI_INT_CLR);

	cfg = DIRECT_CS_MASK | DATA_ORD_MASK | SPI_EN_MASK;	// SWCS = 0, so selected CS is asserted

	if (slave->cs == 0)
		cfg |= CS1_DIS_MASK;
	else
		cfg |= CS0_DIS_MASK;

	/* Set configuration register:	*/
	/*		cs is controlled by sw	*/
	/*		data order: MSB first	*/
	/*		enable SPI module		*/
	/* Note: currently we are not using the mode parameter	*/
	writel(cfg, ds->regs_base + DMW_SPI_CFG_REG);

	return 0;
}

void spi_release_bus(struct spi_slave *slave)
{
	struct dmw96_spi_slave *ds = to_dmw96_spi(slave);
	unsigned int cfg;

	/* Disable CS (might be not needed since disabling of the SPI will do the same anyway... */
	cfg = readl(ds->regs_base + DMW_SPI_CFG_REG);
	cfg |= SWCS_MASK;
	writel(cfg, ds->regs_base + DMW_SPI_CFG_REG);

	/* Disable the SPI hardware */
	writel(0, ds->regs_base + DMW_SPI_CFG_REG);
}

static inline unsigned int spi_write_read_byte(struct dmw96_spi_slave *ds, unsigned int data)
{
	/* send out data */
	writel(data, ds->regs_base + DMW_SPI_TX_DAT);

	/* wait for the data to clock in/out */
	while (!(readl(ds->regs_base + DMW_SPI_INT_ST) & (1<<10))) /* SPI_DONE */
		;

	/* read the byte we just got and return it */
	/* because we always read the byte, we don't have to use RX_IGNORE or RX_FIFO_FLUSH */
	return readl(ds->regs_base + DMW_SPI_RX_DAT);
}

int spi_xfer(struct spi_slave *slave, unsigned int bitlen,
	     const void *dout, void *din, unsigned long flags)
{
	struct dmw96_spi_slave *ds = to_dmw96_spi(slave);
	unsigned int len;
	unsigned int i;
	unsigned char *buf;

	/* No support for full-duplex, and only support full bytes */
	if ((dout && din) || (bitlen % 8))
		return -1;
		
	len = bitlen / 8;
	
	if (dout) {
		/* Write */
		buf = (unsigned char*)dout;

		for (i = 0; i < len; i++) {
			spi_write_read_byte(ds, buf[i]);
		}
	}
	else if (din) {
		/* Read */
		buf = (unsigned char*)din;

		for (i = 0; i < len; i++) {
			buf[i] = spi_write_read_byte(ds, 0x00);
		}
	}

	return 0;
}

int spi_cs_is_valid(unsigned int bus, unsigned int cs)
{
	return ((bus == 0) || (bus == 1)) && ((cs == 0) || (cs == 1));
}

void spi_cs_activate(struct spi_slave *slave)
{
	/* do nothing */
}

void spi_cs_deactivate(struct spi_slave *slave)
{
	/* do nothing */
}
