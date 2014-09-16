#ifndef __DSPG_BUFFER_H__
#define __DSPG_BUFFER_H__

struct dspg_buffer_t {
	unsigned int index;
	unsigned int virt_address;
	unsigned int bus_address;
	unsigned int allow_zero_copy;
};

#endif
