/*
 * Copyright (C) 2011 DSP Group
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

#ifndef _DMW96_SPI_H_
#define _DMW96_SPI_H_

struct dmw96_spi_slave {
	struct spi_slave slave;
	unsigned long regs_base;
	unsigned int freq;
	unsigned int mode;
};

static inline struct dmw96_spi_slave *to_dmw96_spi(struct spi_slave *slave)
{
	return container_of(slave, struct dmw96_spi_slave, slave);
}

#endif /* _DMW96_SPI_H_ */
