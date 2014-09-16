#include <common.h>
#include <asm/byteorder.h>
#include <asm/io.h>
#include <asm/arch/gpio.h>

#include "dmw.h"

#define GP_DATA			0x00
#define GP_DATA_SET		0x04
#define GP_DATA_CLR		0x08
#define GP_DIR			0x0C
#define GP_DIR_OUT		0x10
#define GP_DIR_IN		0x14
#define GP_EN			0x18
#define GP_EN_SET		0x1C
#define GP_EN_CLR		0x20
#define GP_PULL_CTRL		0x24
#define GP_PULL_EN		0x28
#define GP_PULL_DIS		0x2C
#define GP_PULL_TYPE		0x30
#define GP_PULL_UP		0x34
#define GP_PULL_DN		0x38
#define GP_KEEP			0x3C
#define GP_KEEP_EN		0x40
#define GP_KEEP_DIS		0x44
#define GP_OD			0x48
#define GP_OD_SET		0x4C
#define GP_OD_CLR		0x50
#define GP_IN			0x54

static int
gpio_register_offset(int gpio)
{
	return (DMW_GPIO_BASE + ((gpio >> 8) & 0xfff));
}

static int
gpio_mask(int gpio)
{
	return (1 << (gpio & 0xff));
}

void gpio_enable(int gpio)
{
	writel(gpio_mask(gpio), gpio_register_offset(gpio) + GP_EN_SET);
}

void gpio_disable(int gpio)
{
	writel(gpio_mask(gpio), gpio_register_offset(gpio) + GP_EN_CLR);
}

int gpio_get_value(int gpio)
{
	return (((readl(gpio_register_offset(gpio) + GP_DATA)) &
	        gpio_mask(gpio)) > 0) ? 1 : 0;
}

void gpio_set_value(int gpio, int value)
{
	if (value)
		writel(gpio_mask(gpio), gpio_register_offset(gpio) + GP_DATA_SET);
	else
		writel(gpio_mask(gpio), gpio_register_offset(gpio) + GP_DATA_CLR);
}

void gpio_direction_input(int gpio)
{
	writel(gpio_mask(gpio), gpio_register_offset(gpio) + GP_DIR_IN);
}

void gpio_direction_output(int gpio, int value)
{
	gpio_set_value(gpio, value);

	writel(gpio_mask(gpio), gpio_register_offset(gpio) + GP_DIR_OUT);
}

void gpio_pull_ctrl(int gpio, int enable, int up)
{
	if (enable)
		writel(gpio_mask(gpio), gpio_register_offset(gpio) + GP_PULL_EN);
	else
		writel(gpio_mask(gpio), gpio_register_offset(gpio) + GP_PULL_DIS);

	if (up)
		writel(gpio_mask(gpio), gpio_register_offset(gpio) + GP_PULL_UP);
	else
		writel(gpio_mask(gpio), gpio_register_offset(gpio) + GP_PULL_DN);
}

void gpio_keep(int gpio, int enable)
{
	if (enable)
		writel(gpio_mask(gpio), gpio_register_offset(gpio) + GP_KEEP_EN);
	else
		writel(gpio_mask(gpio), gpio_register_offset(gpio) + GP_KEEP_DIS);

}
void gpio_od(int gpio, int value)
{
	if (value)
		writel(gpio_mask(gpio), gpio_register_offset(gpio) + GP_OD_SET);
	else
		writel(gpio_mask(gpio), gpio_register_offset(gpio) + GP_OD_CLR);
}
