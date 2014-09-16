#include "dw.h"
#include <common.h>
#include <asm/byteorder.h>
#include <asm/io.h>
#include "spi.h"
#include "gpio.h"

#define mdelay(n) udelay((n)*1000)

/* When sending commands to ILI9481 via SPI interface, each unit contains 9 bits: */
/* first bit selects whether this is opcode (0) or parameter (1), and the rest 8 bits are the value itself */
#define ILI9481_SPI_PARAMETER_BIT			0x100

/* ILI9481 Internal registers */
#define ILI9481_CMD_NOP					0x00
#define ILI9481_CMD_ENTER_SLEEP_MODE			0x10
#define ILI9481_CMD_EXIT_SLEEP_MODE			0x11
#define ILI9481_CMD_ENTER_PARTIAL_MODE			0x12
#define ILI9481_CMD_ENTER_NORMAL_MODE			0x13
#define ILI9481_CMD_EXIT_INVERT_MODE			0x20
#define ILI9481_CMD_ENTER_INVERT_MODE			0x21
#define ILI9481_CMD_SET_GAMMA_CURVE			0x26
#define ILI9481_CMD_SET_DISPLAY_OFF			0x28
#define ILI9481_CMD_SET_DISPLAY_ON			0x29
#define ILI9481_CMD_SET_COLUMN_ADDRESS			0x2A
#define ILI9481_CMD_SET_PAGE_ADDRESS			0x2B
#define ILI9481_CMD_WRITE_MEMORY_START			0x2C
#define ILI9481_CMD_WRITE_LUT				0x2D
#define ILI9481_CMD_SET_PARTIAL_AREA			0x30
#define ILI9481_CMD_SET_SCROLL_AREA			0x33
#define ILI9481_CMD_SET_TEAR_OFF			0x34
#define ILI9481_CMD_SET_TEAR_ON				0x35
#define ILI9481_CMD_SET_ADRESS_MODE			0x36
#define ILI9481_CMD_SET_SCROLL_START			0x37
#define ILI9481_CMD_EXIT_IDLE_MODE			0x38
#define ILI9481_CMD_ENTER_IDLE_MODE			0x39
#define ILI9481_CMD_SET_PIXEL_FORMAT			0x3A
#define ILI9481_CMD_WRITE_MEMORY_CONTINUE		0x3C
#define ILI9481_CMD_SET_TEAR_SCANLINE			0x44
#define ILI9481_CMD_COMMAND_ACCESS_PROTECT		0xB0
#define ILI9481_CMD_LOW_POWER_MODE_CONTROL		0xB1
#define ILI9481_CMD_FRAME_MEMORY_ACCESS			0xB3
#define ILI9481_CMD_DISPLAY_MODE_SETTING		0xB4
#define ILI9481_CMD_PANEL_DRIVING_SETTING		0xC0
#define ILI9481_CMD_DISPLAY_TIMING_SETTING_N		0xC1
#define ILI9481_CMD_DISPLAY_TIMING_SETTING_P		0xC2
#define ILI9481_CMD_DISPLAY_TIMING_SETTING_I		0xC3
#define ILI9481_CMD_FRAME_RATE_INV_CONTROL		0xC5
#define ILI9481_CMD_INTERFACE_CONTROL			0xC6
#define ILI9481_CMD_GAMMA_SETTING			0xC8
#define ILI9481_CMD_POWER_SETTING			0xD0
#define ILI9481_CMD_VCOM_CONTROL			0xD1
#define ILI9481_CMD_POWER_SETTING_MODE_N		0xD2
#define ILI9481_CMD_POWER_SETTING_MODE_P		0xD3
#define ILI9481_CMD_POWER_SETTING_MODE_I		0xD4

static void
ili9481_spi_deselect(void)
{
	gpio_set_value(AGPIO(25), 1);
}

static void
ili9481_spi_select(void)
{
	gpio_set_value(AGPIO(25), 0);
}

/* write 9bits value into array of usigned char variables, in position "index" */
static void
write_9bits( unsigned int value, unsigned char *buf, unsigned int index )
{
	unsigned int	buf_index;
	unsigned char	mask;
	unsigned char	shift;
	unsigned int	first_bit;

	first_bit = index * 9;
	buf_index = first_bit / 8;
	shift = first_bit % 8;
	mask = 0xff >> shift;

//	bits_in_first_byte = 8 - shift;
//	bits_in_second_byte = 9 - bits_in_first_byte; = 9 - (8 - shift) = 9 - 8 + shift = shift + 1;

	/* clear mask in first byte */
	buf[buf_index] &= ~mask;

	/* write upper bits of value into first byte */
	buf[buf_index] |= (value >> (shift + 1)) & mask;

	/* update variable for second byte */
	buf_index++;
	shift = 7 - shift;
	mask = 0xff << shift;	// 8 - bits_in_second_byte = 8 - (shift + 1) = 7 - shift 

	/* clear mask in second byte */
	buf[buf_index] &= ~mask;

	/* write lower bits of value into second byte */
	buf[buf_index] |= (value << shift) & mask;
}

/* write command to ili9481 using 9bits SPI interface */
static void
ili9481_spi_write_command(unsigned char command, const unsigned char *parameters, int len)
{
	unsigned char buf[18];
	int i;
	int index;
	const unsigned char *pparam;
	u8 *pbuf;

	/* buffer of 18 bytes can hold 16*9bits, so total of one command and 15 parameters */
	if ( len > 15 ) {
		printf("ili9481_spi_write_command: error - too many parameters\n");
		return;
	}

	/* since the HW supports only word-len of 8 bits, we are placing our 9bits words in array */
	/* of unsigned char variables, and padding to multiples of 9x8bits with NOP command (0x0) */

	/* fill the whole buffer with 0x0 = NOP command */
	memset( buf, 0, sizeof(buf) );

	/* write the parameters into the end of the buffer */
	index = 15;
	pparam = parameters + len - 1;
	for (i = 0; i < len ; i++, index--, pparam-- ) {
		write_9bits( ILI9481_SPI_PARAMETER_BIT | *pparam, buf, index );
	}

	/* write the command before the first parameter */
	write_9bits( command, buf, index );

	/* send via spi - we are sending either 9 or 18 bytes (depends on number of parameters) */
	if (len <= 7) {
		len = 9;
		pbuf = buf + 9;
	} else {
		len = 18;
		pbuf = buf;
	}

	ili9481_spi_select();
	spi_write(pbuf, len);
	ili9481_spi_deselect();
}

/*************************************************************************************************/

static void
tdt320t2g706_reset(void)
{
	/* LCD_GPIO_RESET */
	gpio_enable(BGPIO(4));
	gpio_direction_output(BGPIO(4), 1);

	mdelay(1);

	gpio_set_value(BGPIO(4), 0);

	mdelay(10);

	gpio_set_value(BGPIO(4), 1);

	mdelay(50);

	return;
}

int
dw74_tdt320_init(void)
{
	unsigned char cmds[16];

	/* Reset LCD */
	tdt320t2g706_reset();

	ili9481_spi_write_command( ILI9481_CMD_EXIT_SLEEP_MODE, NULL, 0 );

	mdelay(50);

	cmds[0] = 0x07;
	cmds[1] = 0x42;
	cmds[2] = 0x18;
	ili9481_spi_write_command( ILI9481_CMD_POWER_SETTING, cmds, 3 );

	cmds[0] = 0x00;
	cmds[1] = 0x09;
	cmds[2] = 0x0E;
	ili9481_spi_write_command( ILI9481_CMD_VCOM_CONTROL, cmds, 3 );

	cmds[0] = 0x01;
	cmds[1] = 0x02;
	ili9481_spi_write_command( ILI9481_CMD_POWER_SETTING_MODE_N, cmds, 2 );

	cmds[0] = 0x10;
	cmds[1] = 0x3B;
	cmds[2] = 0x00;
	cmds[3] = 0x02;
	cmds[4] = 0x11;
	ili9481_spi_write_command( ILI9481_CMD_PANEL_DRIVING_SETTING, cmds, 5 );

	cmds[0] = 0x10;
	cmds[1] = 0x10;
	cmds[2] = 0x22;
	ili9481_spi_write_command( ILI9481_CMD_DISPLAY_TIMING_SETTING_N, cmds, 3 );

	cmds[0] = 0x02;
	ili9481_spi_write_command( ILI9481_CMD_FRAME_RATE_INV_CONTROL, cmds, 1 );

	cmds[0] = 0x81;
	ili9481_spi_write_command( ILI9481_CMD_INTERFACE_CONTROL, cmds, 1 );

	cmds[0] = 0x00;
	cmds[1] = 0x32;
	cmds[2] = 0x36;
	cmds[3] = 0x45;
	cmds[4] = 0x06;
	cmds[5] = 0x16;
	cmds[6] = 0x37;
	cmds[7] = 0x75;
	cmds[8] = 0x77;
	cmds[9] = 0x54;
	cmds[10] = 0x0C;
	cmds[11] = 0x00;
	ili9481_spi_write_command( ILI9481_CMD_GAMMA_SETTING, cmds, 12 );

	cmds[0] = 0x00;
	cmds[1] = 0x00;
	cmds[2] = 0x01;
	cmds[3] = 0x3F;
	ili9481_spi_write_command( ILI9481_CMD_SET_COLUMN_ADDRESS, cmds, 4 );

	cmds[0] = 0x00;
	cmds[1] = 0x00;
	cmds[2] = 0x01;
	cmds[3] = 0xDF;
	ili9481_spi_write_command( ILI9481_CMD_SET_PAGE_ADDRESS, cmds, 4 );

	cmds[0] = 0x66;
	ili9481_spi_write_command( ILI9481_CMD_SET_PIXEL_FORMAT, cmds, 1 );

	//printf("0xB4: 0x%x\n", ili9481_spi_read_command(ILI9481_CMD_DISPLAY_MODE_SETTING));
	cmds[0] = 0x11;
	ili9481_spi_write_command( ILI9481_CMD_DISPLAY_MODE_SETTING, cmds, 1 );
	//printf("0xB4: 0x%x\n", ili9481_spi_read_command(ILI9481_CMD_DISPLAY_MODE_SETTING));

	cmds[0] = 0x9B;
	ili9481_spi_write_command( ILI9481_CMD_SET_ADRESS_MODE, cmds, 1 );

	mdelay(50);

	ili9481_spi_write_command( ILI9481_CMD_ENTER_NORMAL_MODE, NULL, 0 );
	ili9481_spi_write_command( ILI9481_CMD_SET_DISPLAY_ON, NULL, 0 );
	ili9481_spi_write_command( ILI9481_CMD_WRITE_MEMORY_START, NULL, 0 );

	return 0;
}
