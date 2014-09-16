#ifndef __GPIO_H
#define __GPIO_H

#define AGPIO(x) (0 * 32 + (x % 32))
#define BGPIO(x) (1 * 32 + (x % 32))
#define CGPIO(x) (2 * 32 + (x % 32))
#define DGPIO(x) (3 * 32 + (x % 32))
#define EGPIO(x) (4 * 32 + (x % 32))
#define FGPIO(x) (5 * 32 + (x % 32))
#define GGPIO(x) (6 * 32 + (x % 32))

void gpio_enable(int gpio);
void gpio_disable(int gpio);
void gpio_set_value(int gpio, int value);
void gpio_direction_output(int gpio, int value);

#endif
