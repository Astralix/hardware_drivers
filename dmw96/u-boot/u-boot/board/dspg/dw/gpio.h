#ifndef DW_GPIO_H
#define DW_GPIO_H

#define AGPIO(x)			((0x00<<8)|(x))
#define BGPIO(x)			((0x18<<8)|(x))
#define CGPIO(x)			((0x30<<8)|(x))
#define DGPIO(x)			((0x78<<8)|(x))

#define DW_GPIO_BASE			0x05000000

void gpio_enable(int gpio);
void gpio_disable(int gpio);
int  gpio_get_value(int gpio);
void gpio_set_value(int gpio, int value);
void gpio_direction_input(int gpio);
void gpio_direction_output(int gpio, int value);
void gpio_pullen(int gpio, int value);
void gpio_pod(int gpio, int value);

#endif
