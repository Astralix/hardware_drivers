#include <common.h>
#include <asm/byteorder.h>
#include <asm/io.h>
#include "dw.h"
#include "gpio.h"
#include "spi.h"
#include "dp52.h"

void dp52_select(void)
{
	gpio_set_value(AGPIO(26), 0);
}

void dp52_deselect(void)
{
	gpio_set_value(AGPIO(26), 1);
}

int dp52_direct_write(unsigned int reg, unsigned int value)
{
	unsigned char tmp[3];

	dp52_select();

	tmp[0] = reg;
	tmp[1] = (value >> 8) & 0xff;
	tmp[2] = value & 0xff;
	spi_write(tmp, 3);

	dp52_deselect();

	return 0;
}

int dp52_direct_read(unsigned int reg)
{
	unsigned char tmp[2];

	dp52_select();

	tmp[0] = (reg & 0x7f) | 0x80;
	spi_write(tmp, 1);

	spi_read(tmp, 2);

	dp52_deselect();

	return (tmp[0] << 8) | tmp[1];
}

int dp52_indirect_read(unsigned int reg)
{
	dp52_direct_write(DP52_SPI_INDADD, reg);
	return dp52_direct_read(DP52_SPI_INDDAT);
}

int dp52_indirect_write(unsigned int reg, unsigned int value)
{
	dp52_direct_write(DP52_SPI_INDADD, reg);
	dp52_direct_write(DP52_SPI_INDDAT, value);
	return 0;
}
