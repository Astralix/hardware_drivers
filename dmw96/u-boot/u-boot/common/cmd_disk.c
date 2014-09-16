/*
 * (C) Copyright 2011
 * DSPG Technologies GmbH
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <command.h>
#include <errno.h>
#include <part.h>

static int do_disk(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	struct block_dev_desc *dev = NULL;
	int id, part = -1;
	unsigned long addr, start, size, blksz;
	char *ep;

	if (argc != 5 && argc != 6)
		goto usage;

	addr = simple_strtoul(argv[2], NULL, 16);

	id = (int)simple_strtoul(argv[4], &ep, 0);
	dev = get_dev(argv[3], id);
	if (dev == NULL) {
		puts("Invalid block device\n");
		return 1;
	}

	if (*ep) {
		if (*ep != ':')
			goto usage;

		part = (int)simple_strtoul(++ep, NULL, 0);
	}

	if (part >= 0) {
		struct disk_partition partinfo;

		if (get_partition_info(dev, part, &partinfo)) {
			printf("Invalid partition");
			return 1;
		}

		start = partinfo.start;
		size = partinfo.size;
		blksz = partinfo.blksz;
	} else {
		start = 0;
		size = dev->lba;
		blksz = dev->blksz;
	}

	if (argc >= 6) {
		unsigned long s = simple_strtoul(argv[5], NULL, 16);

		s = (s + blksz - 1) / blksz;
		if (s > size) {
			printf("Access beyond partition size!\n");
			return 1;
		}
		size = s;
	}

	if (strcmp(argv[1], "read") == 0) {
		return dev->block_read(dev->dev, start, size, (void *)addr) != size;
	} else if (strcmp(argv[1], "write") == 0) {
		if (!dev->block_write) {
			printf("Read only device!");
			return 1;
		}
		return dev->block_write(dev->dev, start, size, (void *)addr) != size;
	}

usage:
	printf("Usage: disk read|write <addr> <interface> <dev[:part]> [size]\n");
	return 1;
}

U_BOOT_CMD(
	disk, 6, 0, do_disk, "read/write to/from block device or partition",
	"read|write <addr> <interface> <dev[:part]> [size]\n"
	"    - read/write from 'dev' on 'interface'\n"
	"      e.g. disk read $loadaddr mmc 0:1 200"
);

