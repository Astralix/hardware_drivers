/*
 * pnx8181 i2c header
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

#define I2C_TIMEOUT		10000
#define I2C_DELAY		10

#define I2C_CLKHI_OFFSET	(0x00C)
#define I2C_CLKHI_REG(a)		(void *)(a + I2C_CLKHI_OFFSET)
#define I2C_CLKLO_OFFSET	(0x010)
#define I2C_CLKLO_REG(a)		(void *)(a + I2C_CLKLO_OFFSET)
#define I2C_ADR_OFFSET		(0x014)
#define I2C_ADR_REG(a)			(void *)(a + I2C_ADR_OFFSET)
#define I2C_SADDR_FIELD		0xFFFFFF80
#define I2C_HOLDDAT_OFFSET	(0x018)
#define I2C_HOLDDAT_REG(a)		(void *)(a + I2C_HOLDDAT_OFFSET)
#define I2C_CLKDIV_FIELD	0xFFFFFC00
#define I2C_STS_OFFSET		(0x004)
#define I2C_STS_REG(a)			(void *)(a + I2C_STS_OFFSET)
#define I2C_CTL_OFFSET		(0x008)
#define I2C_CTL_REG(a)			(void *)(a + I2C_CTL_OFFSET)
#define I2C_TX_OFFSET		(0x000)
#define I2C_TX_REG(a)			(void *)(a + I2C_TX_OFFSET)
#define I2C_RX_OFFSET		(0x000)
#define I2C_RX_REG(a)			(void *)(a + I2C_RX_OFFSET)

#define I2C_TDI			0x00000001
#define I2C_AFI			0x00000002
#define I2C_NAI			0x00000004
#define I2C_DRMI		0x00000008
#define I2C_DRSI		0x00000010
#define I2C_ACTIVE		0x00000020
#define I2C_SCL			0x00000040
#define I2C_SDA			0x00000080
#define I2C_RFF			0x00000100
#define I2C_RFE			0x00000200
#define I2C_TFF			0x00000400
#define I2C_TFE			0x00000800
#define I2C_TFFS		0x00001000
#define I2C_TFES		0x00002000
#define I2C_MAST		0x00004000

#define I2C_RESET		0x00000100

#define SEND_I2C_DEVICE_ADDRESS_START_STOP(addr,a)	\
			writel(((0x80 + addr) << 1) + 0x200, I2C_TX_REG(a));
#define SEND_I2C_START_ADDRESS(addr,a)	writel((0x80 + addr) << 1, I2C_TX_REG(a));
#define SEND_I2C_STOP_DATA(data,a)	writel(0x200 + data, I2C_TX_REG(a));
#define SEND_I2C_READ_ADDRESS(addr,a)	\
			writel(((0x80 + addr) << 1) + 1, I2C_TX_REG(a));
#define SOFT_I2C_RESET(a)	\
			writel(readl(I2C_CTL_REG(a)) | I2C_RESET, I2C_CTL_REG(a));

/*should be implemented for each board to use this driver*/
unsigned long board_i2c_pnx_get_pclk(void);
