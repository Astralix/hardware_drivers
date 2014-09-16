#include <common.h>
#include <asm/byteorder.h>
#include <asm/io.h>

#include "dw.h"
#include "gpio.h"

#define AGPDATA				0x00
#define AGPDIR				0x04
#define AGPEN				0x08
#define AGPPULLEN			0x0C
#define AGPPULLTYPE			0x10
#define AGPPOD				0x14

#define BGPDATA				0x18
#define BGPDIR				0x1C
#define BGPEN				0x20
#define BGPPULLEN			0x24
#define BGPPOD				0x2C

#define CGPDATA				0x30
#define CGPDIR				0x34
#define CGPEN				0x38
#define CGPPULLEN			0x3C
#define CGPPOD				0x44

#define DGPDATA				0x78
#define DGPDIR				0x7C
#define DGPEN				0x80
#define DGPPULLEN			0x84
#define DGPPOD				0x88


static int
gpio_register_offset(int gpio)
{
	return (DW_GPIO_BASE + ((gpio >> 8) & 0xff));
}

static int
gpio_mask(int gpio)
{
	return (1 << (gpio & 0xff));
}

void gpio_enable(int gpio)
{
	unsigned int tmp;

	tmp = readl(gpio_register_offset(gpio) + AGPEN);
	tmp |= gpio_mask(gpio);
	writel(tmp, gpio_register_offset(gpio) + AGPEN);
}

void gpio_disable(int gpio)
{
	unsigned int tmp;

	tmp = readl(gpio_register_offset(gpio) + AGPEN);
	tmp &= ~(gpio_mask(gpio));
	writel(tmp, gpio_register_offset(gpio) + AGPEN);
}

int gpio_get_value(int gpio)
{
	return (((readl(gpio_register_offset(gpio) + AGPDATA)) &
	        gpio_mask(gpio)) > 0) ? 1 : 0;
}

void gpio_set_value(int gpio, int value)
{
	unsigned int tmp;

	tmp = readl(gpio_register_offset(gpio) + AGPDATA);
	if (value)
		tmp |= gpio_mask(gpio);
	else
		tmp &= ~gpio_mask(gpio);
	writel(tmp, gpio_register_offset(gpio) + AGPDATA);
}

void gpio_direction_input(int gpio)
{
	unsigned int tmp;

	tmp = readl(gpio_register_offset(gpio) + AGPDIR);
	tmp |= gpio_mask(gpio);
	writel(tmp, gpio_register_offset(gpio) + AGPDIR);
}

void gpio_direction_output(int gpio, int value)
{
	unsigned int tmp;

	tmp = readl(gpio_register_offset(gpio) + AGPDIR);
	tmp &= ~(gpio_mask(gpio));
	writel(tmp, gpio_register_offset(gpio) + AGPDIR);

	gpio_set_value(gpio, value);
}

void gpio_pullen(int gpio, int value)
{
	unsigned int tmp;

	tmp = readl(gpio_register_offset(gpio) + AGPPULLEN);
	if (value)
		tmp |= gpio_mask(gpio);
	else
		tmp &= ~gpio_mask(gpio);
	writel(tmp, gpio_register_offset(gpio) + AGPPULLEN);
}

void gpio_pod(int gpio, int value)
{
	unsigned int tmp;

	tmp = readl(gpio_register_offset(gpio) + AGPPOD);
	if (value)
		tmp |= gpio_mask(gpio);
	else
		tmp &= ~gpio_mask(gpio);
	writel(tmp, gpio_register_offset(gpio) + AGPPOD);
}
