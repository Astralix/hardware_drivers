#include "common.h"
#include "romapi.h"
#include "bootrom.h"
#include "serial.h"

static struct image_header header;

void *load_loader(void) {
	uint32_t crc;
	void *entry;
	int ret;

	ret = romapi->read(&header, 0, sizeof(header));
	if (ret) {
		serial_puts("error reading header\n");
		return ERR_PTR(-1);
	}

	crc = romapi->crc32(&header, sizeof(header) - sizeof(uint32_t));
	if (crc != header.crc32) {
		serial_puts("error in header");
		return ERR_PTR(-1);
	}

	entry = (void *)header.load_addr;

	ret = romapi->read(entry, header.load_offset, header.load_size);
	if (ret) {
		serial_puts("error reading loader\n");
		return ERR_PTR(-1);
	}

	if (header.flags & HDRFLAG_CRC32) {
		crc = romapi->crc32(entry, header.load_size);
		if (crc != header.load_crc32) {
			serial_puts("error in loader\n");
			return ERR_PTR(-1);
		}
	}

	return entry;
}
