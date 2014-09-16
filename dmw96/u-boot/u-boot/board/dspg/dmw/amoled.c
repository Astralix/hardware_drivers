/*
 * (C) Copyright 2011, DSP Group
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
#include <asm/io.h>
#include <config.h>
#include <spi.h>
#include <asm/arch/gpio.h>

#include "dmw.h"
#include "lcd.h"

static struct spi_slave *amoled_spi;

struct amoled_commands {
	unsigned char command;
	unsigned char argument;
};

#define AMOLED_END_SEQ 0xff

static struct amoled_commands amoled_gamma_table_l_220[] =
{
	{0x40, 0x00},
	{0x41, 0x3f},
	{0x42, 0x28},
	{0x43, 0x28},
	{0x44, 0x28},
	{0x45, 0x20},
	{0x46, 0x40},
	{0x50, 0x00},
	{0x51, 0x00},
	{0x52, 0x11},
	{0x53, 0x25},
	{0x54, 0x27},
	{0x55, 0x20},
	{0x56, 0x3f},
	{0x60, 0x00},
	{0x61, 0x3F},
	{0x62, 0x27},
	{0x63, 0x26},
	{0x64, 0x26},
	{0x65, 0x1c},
	{0x66, 0x56},
	{AMOLED_END_SEQ,0}
};

static struct amoled_commands amoled_gamma_table_l_250[] =
{
	{0x40, 0x00},
	{0x41, 0x3f},
	{0x42, 0x2a},
	{0x43, 0x27},
	{0x44, 0x27},
	{0x45, 0x1f},
	{0x46, 0x44},
	{0x50, 0x00},
	{0x51, 0x00},
	{0x52, 0x17},
	{0x53, 0x24},
	{0x54, 0x26},
	{0x55, 0x1F},
	{0x56, 0x43},
	{0x60, 0x00},
	{0x61, 0x3F},
	{0x62, 0x2A},
	{0x63, 0x25},
	{0x64, 0x24},
	{0x65, 0x1B},
	{0x66, 0x5C},
	{AMOLED_END_SEQ,0}
};

static struct amoled_commands amoled_gamma_table_l_100[] =
{
	{0x40, 0x00},
	{0x41, 0x3f},
	{0x42, 0x30},
	{0x43, 0x2A},
	{0x44, 0x2B},
	{0x45, 0x24},
	{0x46, 0x2F},
	{0x50, 0x00},
	{0x51, 0x00},
	{0x52, 0x00},
	{0x53, 0x25},
	{0x54, 0x29},
	{0x55, 0x24},
	{0x56, 0x2E},
	{0x60, 0x00},
	{0x61, 0x3F},
	{0x62, 0x2F},
	{0x63, 0x29},
	{0x64, 0x29},
	{0x65, 0x21},
	{0x66, 0x3F},
	{AMOLED_END_SEQ,0}
};

static struct amoled_commands amoled_gamma_table_l_160[] =
{
	{0x40, 0x00},
	{0x41, 0x3f},
	{0x42, 0x2B},
	{0x43, 0x29},
	{0x44, 0x28},
	{0x45, 0x23},
	{0x46, 0x38},
	{0x50, 0x00},
	{0x51, 0x00},
	{0x52, 0x0B},
	{0x53, 0x25},
	{0x54, 0x27},
	{0x55, 0x23},
	{0x56, 0x37},
	{0x60, 0x00},
	{0x61, 0x3F},
	{0x62, 0x29},
	{0x63, 0x28},
	{0x64, 0x25},
	{0x65, 0x20},
	{0x66, 0x4B},
	{AMOLED_END_SEQ,0}
};

static void
amoled_write_spi_command(struct spi_slave *spi, unsigned char command, unsigned char arg)
{
	unsigned char spi_cmd[2];

	spi_cmd[0] = command;
	spi_cmd[1] = arg;

	/* SPI_MODE_1 (clk active high) */
	spi_claim_bus(spi);
	spi_xfer(spi, 8*2, spi_cmd, NULL, 0);
	spi_release_bus(spi);
}

#define AMOLED_COMMAND_ID 0x70
#define AMOLED_ARGUMET_ID 0x72

static void
amoled_write_command(struct spi_slave *spi, unsigned char command,
					unsigned char *arguments, unsigned int num_of_args)
{
	unsigned int i = 0;

	amoled_write_spi_command(spi, AMOLED_COMMAND_ID, command);

	for (i = 0; i < num_of_args; i++)
		amoled_write_spi_command(spi, AMOLED_ARGUMET_ID, arguments[i]);
}

static void
amoled_write_simple_command(struct spi_slave *spi, unsigned char command, unsigned char argument)
{
	amoled_write_command(spi, command, &argument, 1);
}

static void
amoled_run_sequence(struct spi_slave *spi, struct amoled_commands *seq)
{
	struct amoled_commands *cmd = seq;

	while (cmd->command != AMOLED_END_SEQ) {
		amoled_write_simple_command(spi, cmd->command, cmd->argument);
		cmd++;
	}
}

static void amoled_pentile_key(struct spi_slave *spi)
{
	unsigned char args[] = {0xd0, 0xe8};

	amoled_write_command(spi, 0xef, args, 2);
}

#define LED_RESET	0
#define LED_ACTIVE	1
static void dmw_amoled_reset(void)
{
	gpio_direction_output(CONFIG_DMW_GPIO_AMOLED_RESET, LED_ACTIVE);
	udelay(100);
	gpio_direction_output(CONFIG_DMW_GPIO_AMOLED_RESET, LED_RESET);
	udelay(100);
	gpio_direction_output(CONFIG_DMW_GPIO_AMOLED_RESET, LED_ACTIVE);
	udelay(100);
}

void dmw_amoled_init(void)
{
	amoled_spi = spi_setup_slave(CONFIG_DMW_AMOLED_SPI_UNIT, CONFIG_DMW_GPIO_AMOLED_CS, 0, 0);
	gpio_direction_output(CONFIG_DMW_GPIO_AMOLED_RESET, LED_ACTIVE);
	gpio_enable(CONFIG_DMW_GPIO_AMOLED_RESET);

	dmw_amoled_reset();

	amoled_write_simple_command(amoled_spi, 0x31, 0x08); // SCTE set
	amoled_write_simple_command(amoled_spi, 0x32, 0x14); // SCWE set
	amoled_write_simple_command(amoled_spi, 0x30, 0x02); // Gateless signal
	amoled_write_simple_command(amoled_spi, 0x27, 0x01); // Gateless signal

	/* Display condition set */
	amoled_write_simple_command(amoled_spi, 0x12, 0x08); // VBP set
	amoled_write_simple_command(amoled_spi, 0x13, 0x08); // VFP set
	amoled_write_simple_command(amoled_spi, 0x15, 0x00); // Display con
	amoled_write_simple_command(amoled_spi, 0x16, 0x00); // Color depth set

	amoled_pentile_key(amoled_spi);

	amoled_write_simple_command(amoled_spi, 0x39, 0x44); // Gamma set select

	/* Normal gamma table */
	amoled_run_sequence(amoled_spi, amoled_gamma_table_l_220);

	/* Analog power condition set */
	amoled_write_simple_command(amoled_spi, 0x17, 0x22); // VBP set
	amoled_write_simple_command(amoled_spi, 0x18, 0x33); // VFP set
	amoled_write_simple_command(amoled_spi, 0x19, 0x03); // Display con
	amoled_write_simple_command(amoled_spi, 0x1A, 0x01); // Color depth set
	amoled_write_simple_command(amoled_spi, 0x22, 0xA4);
	amoled_write_simple_command(amoled_spi, 0x23, 0x00); // Gamma set select
	amoled_write_simple_command(amoled_spi, 0x26, 0xA0); // Gamma set select

	/* STB off */
	amoled_write_simple_command(amoled_spi, 0x1D, 0xA0);

	mdelay(100);

	/* Display ON */
	amoled_write_simple_command(amoled_spi, 0x14, 0x03);
}


void dmw_amoled_standby(void){

	amoled_write_simple_command(amoled_spi, 0x14, 0x00);
	mdelay(50);
	amoled_write_simple_command(amoled_spi, 0x1D, 0xA1);
	mdelay(200);

	gpio_direction_output(CONFIG_DMW_GPIO_AMOLED_RESET, LED_RESET);
}

void dmw_amoled_resume(void){

	dmw_amoled_reset();
	dmw_amoled_init();

	mdelay(20);
	amoled_write_simple_command(amoled_spi, 0x1D,0xA0);
	mdelay(250);
	amoled_write_simple_command(amoled_spi, 0x14,0x03);
}
