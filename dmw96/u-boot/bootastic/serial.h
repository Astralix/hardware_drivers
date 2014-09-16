#ifndef __SERIAL_H
#define __SERIAL_H

void serial_init(unsigned int baudrate);
void serial_puts(const char *s);
void serial_exit(void);


#endif
