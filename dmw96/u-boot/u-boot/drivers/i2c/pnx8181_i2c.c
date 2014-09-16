/*
 * pnx8181 i2c driver
 *
 * (C) Copyright 2008-2009, emlix GmbH, Germany
 * Juergen Schoew <js@emlix.com>
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
 *
 */

#include <config.h>
#include <common.h>
#include <asm/io.h>
#include <pnx8181_i2c.h>

#define I2C_NUM_PROBES 3

static unsigned int pnx_i2c_base = CONFIG_PNX8181_I2C_ADDRESS_1;

void i2c_init(int speed, int slaveaddr)
{
	unsigned long timeout;
	unsigned long tmp;
	unsigned long freq;
	
	if (speed != 400000 && speed != 100000) {
		printf("Invalid i2c speed %d\n",speed);
		return;
	}

	freq = board_i2c_pnx_get_pclk();

	speed /= 1000;

	tmp = ((freq / 1000) / speed) / 2 - 2;
	if (tmp > 0x3FF)
		tmp = 0x3FF;

	writel( tmp, I2C_CLKHI_REG(pnx_i2c_base));
	
	writel( tmp, I2C_CLKLO_REG(pnx_i2c_base));
	
	tmp = (freq / 1000) / 1534;

	writel( tmp, I2C_HOLDDAT_REG(pnx_i2c_base));

	SOFT_I2C_RESET(pnx_i2c_base);
	timeout = 0;
	while ((readl(I2C_STS_REG(pnx_i2c_base)) & I2C_RESET) == I2C_RESET) {
		if (timeout > I2C_TIMEOUT)
			break;
		timeout++;
		udelay(I2C_DELAY);
	}

	debug("%s: speed: %d\n", __func__, speed);
}


int i2c_probe(uchar chip)
{
	unsigned long timeout;

	SEND_I2C_DEVICE_ADDRESS_START_STOP(chip,pnx_i2c_base);
	timeout = 0;
	while ((readl(I2C_STS_REG(pnx_i2c_base)) & I2C_TDI) != I2C_TDI) {
		if (timeout > I2C_TIMEOUT)
			return -1;
		timeout++;
		udelay(I2C_DELAY);
	}
	writel(readl(I2C_STS_REG(pnx_i2c_base)) | I2C_TDI, I2C_STS_REG(pnx_i2c_base));
	if ((readl(I2C_STS_REG(pnx_i2c_base)) & I2C_NAI) == I2C_NAI)
		return -1;
	return 0;
}

static int i2c_read_byte(u8 chip, u16 addr, u8 *byte)
{
	unsigned long timeout;

	timeout = 0;
	while ((readl(I2C_STS_REG(pnx_i2c_base)) & I2C_ACTIVE) == I2C_ACTIVE) {
		if (timeout > I2C_TIMEOUT)
			return -1;
		timeout++;
		udelay(I2C_DELAY);
	}
	SEND_I2C_START_ADDRESS(chip,pnx_i2c_base);
	/* loop if scl=1, active=0 or drmi=0 */
	timeout = 0;
	while (((readl(I2C_STS_REG(pnx_i2c_base)) & I2C_SCL) == I2C_SCL) ||
		((readl(I2C_STS_REG(pnx_i2c_base)) & I2C_ACTIVE) != I2C_ACTIVE) ||
		((readl(I2C_STS_REG(pnx_i2c_base)) & I2C_DRMI) != I2C_DRMI)) {
		if (timeout > I2C_TIMEOUT)
			return -1;
		timeout++;
		udelay(I2C_DELAY);
	}
#ifdef CONFIG_PNX8181_16BIT_ADDRESS 
	writel((addr & 0xFF00) >> 8, I2C_TX_REG(pnx_i2c_base));	/* ADDRESS_MODE16 */
#endif
	writel((addr & 0x00FF) >> 0, I2C_TX_REG(pnx_i2c_base));
	SEND_I2C_READ_ADDRESS(chip,pnx_i2c_base);			/* Dev Sel to Read */
	SEND_I2C_STOP_DATA(0x00,pnx_i2c_base);			/* dummy write */
	timeout = 0;
	while (((readl(I2C_STS_REG(pnx_i2c_base)) & I2C_NAI) == I2C_NAI) ||
		((readl(I2C_STS_REG(pnx_i2c_base)) & I2C_TDI) != I2C_TDI) ||
		((readl(I2C_STS_REG(pnx_i2c_base)) & I2C_SCL) != I2C_SCL) ||
		((readl(I2C_STS_REG(pnx_i2c_base)) & I2C_ACTIVE) == I2C_ACTIVE) ||
		((readl(I2C_STS_REG(pnx_i2c_base)) & I2C_DRMI) == I2C_DRMI)) {
		if (timeout > I2C_TIMEOUT)
			return -1;
		timeout++;
		udelay(I2C_DELAY);
	}
	writel(readl(I2C_STS_REG(pnx_i2c_base)) | I2C_TDI, I2C_STS_REG(pnx_i2c_base));
	timeout = 0;
	while ((readl(I2C_STS_REG(pnx_i2c_base)) & I2C_TDI) == I2C_TDI) {
		if (timeout > I2C_TIMEOUT)
			return -1;
		timeout++;
		udelay(I2C_DELAY);
	}
	*byte = readl(I2C_RX_REG(pnx_i2c_base)) & 0xff;
	return 0;
}

static int i2c_write_byte(u8 chip, u16 addr, u8 *byte)
{
	u8 dummy;
	unsigned long timeout;

	/* wait until data is written and eeprom back again */
	timeout = 0;
	dummy = 1;
	while (dummy != 0) {
		if (timeout > I2C_NUM_PROBES)
			return -1;
		dummy = -i2c_probe(chip);
		timeout++;
		udelay(I2C_DELAY);
	}
	timeout = 0;
	while ((readl(I2C_STS_REG(pnx_i2c_base)) & I2C_ACTIVE) == I2C_ACTIVE) {
		if (timeout > I2C_TIMEOUT)
			return -1;
		timeout++;
		udelay(I2C_DELAY);
	}
	SEND_I2C_START_ADDRESS(chip,pnx_i2c_base);
	/* loop if scl=1, active=0 or drmi=0 */
	timeout = 0;
	while (((readl(I2C_STS_REG(pnx_i2c_base)) & I2C_SCL) == I2C_SCL) ||
		  ((readl(I2C_STS_REG(pnx_i2c_base)) & I2C_ACTIVE) != I2C_ACTIVE) ||
		  ((readl(I2C_STS_REG(pnx_i2c_base)) & I2C_DRMI) != I2C_DRMI)) {
		if (timeout > I2C_TIMEOUT)
			return -2;
		timeout++;
		udelay(I2C_DELAY);
	}
#ifdef CONFIG_PNX8181_16BIT_ADDRESS 
	writel((addr & 0xFF00) >> 8, I2C_TX_REG(pnx_i2c_base));	/* ADDRESS_MODE16 */
#endif
	writel((addr & 0x00FF) >> 0, I2C_TX_REG(pnx_i2c_base));
	SEND_I2C_STOP_DATA(*byte,pnx_i2c_base);
	timeout = 0;
	while (((readl(I2C_STS_REG(pnx_i2c_base)) & I2C_NAI) == I2C_NAI) ||
		((readl(I2C_STS_REG(pnx_i2c_base)) & I2C_TDI) != I2C_TDI) ||
		((readl(I2C_STS_REG(pnx_i2c_base)) & I2C_SCL) != I2C_SCL) ||
		((readl(I2C_STS_REG(pnx_i2c_base)) & I2C_ACTIVE) == I2C_ACTIVE) ||
		((readl(I2C_STS_REG(pnx_i2c_base)) & I2C_DRMI) == I2C_DRMI)) {
		if (timeout > I2C_TIMEOUT)
			return -3;
		timeout++;
		udelay(I2C_DELAY);
	}
	writel(readl(I2C_STS_REG(pnx_i2c_base)) | I2C_TDI, I2C_STS_REG(pnx_i2c_base));
	timeout = 0;
	while ((readl(I2C_STS_REG(pnx_i2c_base)) & I2C_TDI) == I2C_TDI) {
		if (timeout > I2C_TIMEOUT)
			return -4;
		timeout++;
		udelay(I2C_DELAY);
	}
	dummy = readl(I2C_RX_REG(pnx_i2c_base)) & 0xff;
	return 0;
}

int i2c_read(uchar chip, uint addr, int alen, uchar *buf, int len)
{
	int ret;
	u8 byte;

	debug("%s chip: 0x%02x addr: 0x%04x alen: %d len: %d\n",
		__func__, chip, addr, alen, len);
	while (len--) {
		ret = i2c_read_byte(chip, addr, &byte);
		if (ret < 0)
			return -1;
		*buf++ = byte;
		addr++;
	}
	return 0;
}

int i2c_write(uchar chip, uint addr, int alen, uchar *buf, int len)
{
	int ret;

	debug("%s chip: 0x%02x addr: 0x%04x alen: %d len: %d\n",
		__func__, chip, addr, alen, len);
	while (len--) {
		ret = i2c_write_byte(chip, addr++, buf++);
		if (ret) {
			printf("i2c_write failed chip: 0x%x addr: "
				"0x%04x error %d\n", chip, addr, ret);
			return -1;
		}
	}
	return 0;
}


int i2c_set_bus_num(unsigned int bus)
{
	if ((bus < 0) || (bus >= CONFIG_PNX8181_I2C_BUS_MAX)) {
		printf("Bad bus: %d\n", bus);
		return -1;
	}

	if (bus == 0)
		pnx_i2c_base = CONFIG_PNX8181_I2C_ADDRESS_1;
	else
		pnx_i2c_base = CONFIG_PNX8181_I2C_ADDRESS_2;

	i2c_init(CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);

	return 0;
}


int i2c_get_bus_num(void)
{
	if (pnx_i2c_base == CONFIG_PNX8181_I2C_ADDRESS_1)
		return 0;

	if (pnx_i2c_base == CONFIG_PNX8181_I2C_ADDRESS_2)
		return 1;

	return -1;
}
