#ifndef DMW_GPIO_H
#define DMW_GPIO_H

#define AGPIO(x)			((0x000<<8)|(x))
#define BGPIO(x)			((0x060<<8)|(x))
#define CGPIO(x)			((0x0C0<<8)|(x))
#define DGPIO(x)			((0x120<<8)|(x))
#define EGPIO(x)			((0x180<<8)|(x))
#define FGPIO(x)			((0x1E0<<8)|(x))
#define GGPIO(x)			((0x240<<8)|(x))

#define DMW_GPIO_BASE			0x05000000

#ifndef __ASSEMBLY__

void gpio_enable(int gpio);
void gpio_disable(int gpio);
int  gpio_get_value(int gpio);
void gpio_set_value(int gpio, int value);
void gpio_direction_input(int gpio);
void gpio_direction_output(int gpio, int value);
void gpio_pull_ctrl(int gpio, int enable, int up);
void gpio_keep(int gpio, int enable);
void gpio_od(int gpio, int value);

#endif

#endif
