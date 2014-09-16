#include "common.h"
#include "hardware.h"
#include "gpio.h"

#define G(x) (x % 32)

static void gpio_writel(unsigned int gpio, uint32_t val, int reg)
{
	int bank = gpio / 32;

	writel(val, DMW96_GPIO_BASE + bank * DMW96_GPIO_BANKOFF + reg);
}

void gpio_enable(int gpio)
{
	gpio_writel(gpio, 1 << G(gpio), DMW96_GPIO_EN_SET);
}

void gpio_disable(int gpio)
{
	gpio_writel(gpio, 1 << G(gpio), DMW96_GPIO_EN_CLR);
}

void gpio_set_value(int gpio, int value)
{
	int reg = value ? DMW96_GPIO_DATA_SET : DMW96_GPIO_DATA_CLR;

	gpio_writel(gpio, 1 << G(gpio), reg);
}

void gpio_direction_output(int gpio, int value)
{
	gpio_writel(gpio, 1 << G(gpio), DMW96_GPIO_DIR_OUT);
	gpio_set_value(gpio, value);
}

