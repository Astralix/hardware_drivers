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

#define MAX_PARTS 32

#define DOS_EXTENDED_PARTITION          0x05
#define LINUX_PARTITION                 0x83
#define LINUX_EXTENDED_PARTITION        0x85
#define WIN98_EXTENDED_PARTITION        0x0f

enum part_type {
	PRIMARY,
	LOGICAL
};

struct part {
	enum part_type type;
	unsigned char active;
	unsigned char slot;
	unsigned long start;    /* start LBA */
	unsigned long reserved; /* Reserved header blocks at start. At least 1 for EBRs */
	unsigned long size;     /* Overall size, including reserved blocks. */
};

/*
 * This is the central structure which holds the partitioning information.
 *
 * The partitions are ordered by their layout on the disk. The extended
 * partition is managed implicitly by calculating its position on the fly when
 * writing the partition table. It is required that any logical partitions are
 * adjacent.
 */
struct disk {
	struct block_dev_desc *dev;
	int align;
	int num_part;
	struct part parts[MAX_PARTS];
};

struct mbr_part {
	uint8_t status;
	uint8_t start_head;
	uint8_t start_sector;
	uint8_t start_cyl;
	uint8_t type;
	uint8_t end_head;
	uint8_t end_sector;
	uint8_t end_cyl;
	uint8_t start_block[4];
	uint8_t num_blocks[4];
} __attribute__((packed));

struct mbr {
	uint8_t ignore[446];
	struct mbr_part parts[4];
	uint8_t signature[2];
} __attribute__((packed));


static int part_is_extended(struct mbr_part *p)
{
	return (p->type == DOS_EXTENDED_PARTITION ||
	        p->type == WIN98_EXTENDED_PARTITION ||
	        p->type == LINUX_EXTENDED_PARTITION);
}

static unsigned long le32_to_ulong(uint8_t *le32)
{
	return (((unsigned long)le32[3] << 24) +
	        ((unsigned long)le32[2] << 16) +
	        ((unsigned long)le32[1] <<  8) +
	        (unsigned long)le32[0]);
}

static void ulong_to_le32(uint8_t *le32, unsigned long val)
{
	le32[0] = val;
	le32[1] = val >> 8;
	le32[2] = val >> 16;
	le32[3] = val >> 24;
}

static int disk_have_extended(struct disk *disk)
{
	int i;

	for (i = 0; i < disk->num_part; i++) {
		if (disk->parts[i].type == LOGICAL)
			return 1;
	}

	return 0;
}

static int disk_num_primaries(struct disk *disk)
{
	int i, cnt = 0;

	for (i = 0; i < disk->num_part; i++) {
		if (disk->parts[i].type == PRIMARY)
			cnt++;
	}

	return cnt;
}

static int disk_any_primary_free(struct disk *disk)
{
	return disk_num_primaries(disk) < (disk_have_extended(disk) ? 3 : 4);
}

static int disk_used_slots(struct disk *disk)
{
	int i, used = 0;

	for (i = 0; i < disk->num_part; i++) {
		if (disk->parts[i].type == PRIMARY)
			used |= 1 << disk->parts[i].slot;
	}

	return used;
}

static int disk_read_block(struct disk *disk, unsigned long lba, void *buf)
{
	if (disk->dev->block_read(disk->dev->dev, lba, 1, buf) == 1)
		return 0;
	else
		return -EIO;
}

static int disk_write_block(struct disk *disk, unsigned long lba, void *buf)
{
	if (disk->dev->block_write(disk->dev->dev, lba, 1, buf) == 1)
		return 0;
	else
		return -EIO;
}

static int disk_parse_add_part(struct disk *disk, struct part *p)
{
	int i, slot;

	/* some sanity checks */
	if (disk->num_part >= MAX_PARTS)
		return -ENOMEM;

	if (p->type == PRIMARY) {
		if (!disk_any_primary_free(disk))
			return -EINVAL;
		if (p->slot > 3)
			return -EINVAL;
		if (disk_used_slots(disk) & (1 << p->slot))
			return -EINVAL;
	}

	/* find the right slot */
	slot = 0;
	while (slot < disk->num_part) {
		if (disk->parts[slot].start >= p->start+p->size)
			break;

		/* check for overlap */
		if (disk->parts[slot].start+disk->parts[slot].size > p->start)
			return -EINVAL;

		slot++;
	}

	/* verify that logical partitions are adjacent */
	if (p->type == LOGICAL && disk_have_extended(disk)) {
		if (!((slot > 0 && disk->parts[slot-1].type == LOGICAL) ||
		      (slot < disk->num_part && disk->parts[slot].type == LOGICAL)))
			return -EINVAL;
	}

	/* insert into slot */
	for (i=disk->num_part; i>slot; i--)
		disk->parts[i] = disk->parts[i-1];

	disk->parts[slot] = *p;
	disk->num_part++;

	return 0;
}

static int disk_parse_extended(struct disk *disk, unsigned long ext_start,
                               unsigned long ext_size)
{
	unsigned long ebr_start, ebr_size;
	struct mbr ebr;
	int ret;

	ebr_start = ext_start;
	ebr_size = ext_size;

	for (;;) {
		unsigned long start, off, size;
		struct part p;

		if ((ret = disk_read_block(disk, ebr_start, &ebr)))
			return ret;

		if (ebr.signature[0] != 0x55 || ebr.signature[1] != 0xaa)
			break;

		/*
		 * No guessing here, just the normal case: the first entry is
		 * the actual logical partititon and the second entry points to
		 * the next EBR.
		 */
		off = le32_to_ulong(ebr.parts[0].start_block);
		size = le32_to_ulong(ebr.parts[0].num_blocks);
		start = ebr_start + off;

		if (off + size > ebr_size)
			return -EINVAL;
		if (start < ext_start)
			return -EINVAL;
		if (start+size > ext_start+ext_size)
			return -EINVAL;

		p.type     = LOGICAL;
		p.active   = ebr.parts[0].status == 0x80;
		p.start    = ebr_start;
		p.reserved = off;
		p.size     = size + off;
		if ((ret = disk_parse_add_part(disk, &p)))
			return ret;

		/*
		 * Now jump to the next EBR (if any).
		 */
		ebr_start = ext_start + le32_to_ulong(ebr.parts[1].start_block);
		ebr_size = le32_to_ulong(ebr.parts[1].num_blocks);
		if (!ebr_size)
			break;
	}

	return 0;
}

static int disk_parse(struct disk *disk)
{
	struct mbr mbr;
	int ret, slot;

	if ((ret = disk_read_block(disk, 0, &mbr)))
		return ret;

	/* Check for the boot signature */
	if (mbr.signature[0] != 0x55 || mbr.signature[1] != 0xaa)
		return -ENOENT;

	/* Coarsly verify that the partition table is valid */
	for (slot=0; slot<4; slot++) {
		if (mbr.parts[slot].status != 0x00 && mbr.parts[slot].status != 0x80)
			return -ENOENT;
	}

	/* reset structure */
	disk->num_part = 0;

	/* scan partition table */
	for (slot=0; slot<4; slot++) {
		unsigned long start = le32_to_ulong(mbr.parts[slot].start_block);
		unsigned long size = le32_to_ulong(mbr.parts[slot].num_blocks);

		if (!size)
			continue;

		if (part_is_extended(&mbr.parts[slot])) {
			if ((ret = disk_parse_extended(disk, start, size)))
				return ret;
		} else {
			struct part p;

			p.type     = PRIMARY;
			p.active   = mbr.parts[slot].status == 0x80;
			p.slot     = slot;
			p.start    = start;
			p.reserved = 0;
			p.size     = size;

			if ((ret = disk_parse_add_part(disk, &p)))
				return ret;
		}
	}

	return 0;
}

static int disk_save_extended(struct disk *disk, unsigned long ext_start,
                              struct part *p, struct part *next)
{
	struct mbr ebr;
	int ret;

	if ((ret = disk_read_block(disk, p->start, &ebr)))
		return ret;

	/* clear out unused fields */
	ebr.signature[0] = 0x55;
	ebr.signature[1] = 0xaa;
	memset(&ebr.parts[2], 0, sizeof(ebr.parts[2]));
	memset(&ebr.parts[3], 0, sizeof(ebr.parts[3]));

	/* first entry: the logical partition */
	ebr.parts[0].type = LINUX_PARTITION;
	ebr.parts[0].status = p->active ? 0x80 : 0x00;
	ulong_to_le32(ebr.parts[0].start_block, p->reserved);
	ulong_to_le32(ebr.parts[0].num_blocks, p->size - p->reserved);

	/* second entry: pointer to the next EBR */
	ebr.parts[1].status = 0;
	if (next) {
		ebr.parts[1].type = DOS_EXTENDED_PARTITION;
		ulong_to_le32(ebr.parts[1].start_block, next->start - ext_start);
		ulong_to_le32(ebr.parts[1].num_blocks, next->size);
	} else {
		ebr.parts[1].type = 0;
		ulong_to_le32(ebr.parts[1].start_block, 0);
		ulong_to_le32(ebr.parts[1].num_blocks, 0);
	}

	if ((ret = disk_write_block(disk, p->start, &ebr)))
		return ret;

	return 0;
}

static int disk_save(struct disk *disk)
{
	struct mbr mbr;
	struct part *p;
	int i, ret;
	unsigned long ext_start, ext_size;

	if ((ret = disk_read_block(disk, 0, &mbr)))
		goto error;

	/* clear out mbr partitions */
	mbr.signature[0] = 0x55;
	mbr.signature[1] = 0xaa;
	for (i = 0; i < 4; i++) {
		ulong_to_le32(mbr.parts[i].num_blocks, 0);
		mbr.parts[i].status = 0;
		mbr.parts[i].type = 0;
	}

	/* calculate extended partition */
	if (disk_have_extended(disk)) {

		i = 0;
		while (disk->parts[i].type == PRIMARY)
			i++;

		ext_start = disk->parts[i].start;

		while (disk->parts[i].type == LOGICAL && i < disk->num_part)
			i++;
		i--;

		ext_size = disk->parts[i].start + disk->parts[i].size - ext_start;

		i = 0;
		while (disk_used_slots(disk) & (1 << i))
			i++;

		mbr.parts[i].status = 0;
		mbr.parts[i].type = DOS_EXTENDED_PARTITION;
		ulong_to_le32(mbr.parts[i].start_block, ext_start);
		ulong_to_le32(mbr.parts[i].num_blocks, ext_size);
	}

	/* write out partition list */
	for (i = 0, p = disk->parts; i < disk->num_part; i++, p++) {
		if (p->type == PRIMARY) {
			mbr.parts[p->slot].status = p->active ? 0x80 : 0x00;
			mbr.parts[p->slot].type = LINUX_PARTITION;
			ulong_to_le32(mbr.parts[p->slot].start_block, p->start);
			ulong_to_le32(mbr.parts[p->slot].num_blocks, p->size);
		} else {
			struct part *next = (i+1 < disk->num_part) ? p+1 : NULL;
			if ((ret = disk_save_extended(disk, ext_start, p, next)))
				goto error;
		}
	}

	/* write back mbr */
	if ((ret = disk_write_block(disk, 0, &mbr)))
		goto error;

	return 0;

error:
	printf("Write back failed: %d\n", ret);
	return ret;
}

static int disk_part_adjust(struct disk *disk, struct part *p)
{
	unsigned long align = disk->align + 1;
	unsigned long from, len;

	from = (p->start+align-1) / align * align; /* round up */
	len = p->size / align * align; /* round down */

	if (!len)
		return 0;
	if (from+len > disk->dev->lba)
		return 0;

	if (p->type == LOGICAL) {
		if (len <= align)
			return 0;
		p->reserved = align;
	} else
		p->reserved = 0;

	p->start = from;
	p->size = len;
	return 1;
}

static int disk_part_allowed(struct disk *disk, struct part *prev,
                             struct part *next, enum part_type type)
{
	if (type == PRIMARY) {
		/* free primaries left? */
		if (!disk_any_primary_free(disk))
			return 0;

		/* surrounded by logical partitions? */
		if (prev && prev->type == LOGICAL && next && next->type == LOGICAL)
			return 0;

		return 1;
	} else {
		/* the first logical partition? */
		if (!disk_have_extended(disk)) {
			/* we need a free primary for the extended partition */
			if (disk_any_primary_free(disk))
				return 1;
			else
				return 0;
		}

		/* there must be an adjacent logical partition */
		if ((prev && prev->type == LOGICAL) || (next && next->type == LOGICAL))
			return 1;

		return 0;
	}
}

static int disk_part_add_between(struct disk *disk, int slot, struct part *prev,
                                 struct part *next, unsigned long size,
                                 enum part_type type)
{
	struct part p;
	int i;
	unsigned long start, avail;

	/* is there enough room? */
	start = prev ? prev->start + prev->size : 1 /*MBR!*/;
	avail = next ? next->start - start : disk->dev->lba - start;
	if (size > avail)
		return 0;

	/* see if we can add the type here at all */
	if (!disk_part_allowed(disk, prev, next, type))
		return 0;

	/* try to insert a new partition */
	p.type = type;
	p.active = 0;
	p.slot = 0;
	p.start = start;
	p.size = size ? size : avail;
	if (!disk_part_adjust(disk, &p))
		return 0;

	/* calculate a slot if its a primary partition */
	if (type == PRIMARY) {
		int used = disk_used_slots(disk);
		while (used & (1 << p.slot))
			p.slot++;
	}

	/* finally add it */
	for (i=disk->num_part; i>slot; i--)
		disk->parts[i] = disk->parts[i-1];

	disk->parts[slot] = p;
	disk->num_part++;

	return 1;
}

static int disk_part_add(struct disk *disk, unsigned long size, enum part_type type)
{
	struct part *p = NULL, *prev = NULL;
	int i, ret;

	if (disk->num_part >= MAX_PARTS) {
		printf("Error: reached MAX_PARTS\n");
		return 0;
	}

	if (size) {
		for (i = 0, p = disk->parts; i < disk->num_part; i++, p++) {
			if (disk_part_add_between(disk, i, prev, p, size, type))
				return 1;
			prev = p;
		}
	} else {
		/* no size -> use all available at end */
		i = disk->num_part;
		if (disk->num_part)
			prev = disk->parts + disk->num_part - 1;
	}

	ret = disk_part_add_between(disk, i, prev, NULL, size, type);
	if (!ret)
		printf("Could not add partition!\n");

	return ret;
}

static int disk_find_slot(struct disk *disk, int slot)
{
	int i = 0;

	slot -= 1;
	if (slot < 4) {
		while (i < disk->num_part && (disk->parts[i].slot != slot ||
		       disk->parts[i].type != PRIMARY))
			i++;
	} else {
		slot -= 3;
		while (i < disk->num_part && slot) {
			if (disk->parts[i].type == LOGICAL)
				slot--;
			if (slot)
				i++;
		}
	}

	return (i >= disk->num_part) ? -1 : i;
}

static void disk_print_unused(struct disk *disk, struct part *prev, struct part *next)
{
	struct part p;
	char *heading;
	int allow_pri, allow_log;

	allow_pri = disk_part_allowed(disk, prev, next, PRIMARY);
	allow_log = disk_part_allowed(disk, prev, next, LOGICAL);

	p.type = (allow_pri || !allow_log) ? PRIMARY : LOGICAL;
	p.start = prev ? prev->start + prev->size : 1 /*MBR!*/;
	p.size = next ? next->start - p.start : disk->dev->lba - p.start;

	if (allow_pri && allow_log)
		heading = "Log/Pri";
	else if (allow_pri)
		heading = "Pri";
	else if (allow_log)
		heading = "Log";
	else
		heading = "Unusable";

	if (!disk_part_adjust(disk, &p))
		return;

	printf("Free %-8s                       %10lu\n", heading, p.size);
}

static void disk_print(struct disk *disk)
{
	struct part *p = NULL, *prev = NULL;
	int i, log;

	printf("                First       Last\n");
	printf(" Id  Type       Sector     Sector      Length    Flag\n");
	printf("---- -------- ---------- ---------- ------------ ------\n");

	log = 5;
	for (i = 0, p = disk->parts; i < disk->num_part; i++, p++) {
		disk_print_unused(disk, prev, p);

		if (p->type == PRIMARY) {
			printf("%4d Primary  ", p->slot+1);
		} else {
			printf("%4d Logical  ", log++);
		}

		printf("%10lu %10lu ", p->start, p->start+p->size-1);
		if (p->reserved)
			printf("%9lu+%02lu ", p->size - p->reserved, p->reserved);
		else
			printf("%12lu ", p->size);

		if (p->active)
			printf("Active\n");
		else
			printf("\n");

		prev = p;
	}

	disk_print_unused(disk, prev, NULL);
}

/*****************************************************************************/

static struct disk image;

static int do_dospart(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	if (argc == 4 && !strcmp(argv[1], "dev")) {
		int ret, dev;

		dev = simple_strtoul(argv[3], NULL, 0);
		image.dev = get_dev(argv[2], dev);

		if (image.dev == NULL) {
			puts("\n** Invalid block device **\n");
			return 1;
		}

		printf("Size: %luMB (%lu blocks)\n", image.dev->lba / 2048, image.dev->lba);

		if ((ret = disk_parse(&image))) {
			printf("Could not parse disk: %d. Starting empty...\n", ret);
			image.num_part = 0;
		}

		/* TODO: query alignment */

		return 0;
	}

	if (!image.dev) {
		printf("No device set!\n");
		return 1;
	}

	if (argc <= 1) {
		disk_print(&image);
		return 0;
	}

	if (strcmp(argv[1], "add") == 0 && (argc == 3 || argc == 4)) {
		enum part_type type;
		unsigned long size;
		char *next;

		if (strcmp(argv[2], "primary") == 0)
			type = PRIMARY;
		else if (strcmp(argv[2], "logical") == 0)
			type = LOGICAL;
		else
			return 1;

		if (argc == 4) {
			size = simple_strtoul(argv[3], &next, 0);
			if (*next == 'k' || *next == 'K')
				size *= 2;
			else if (*next == 'm' || *next == 'M')
				size *= 2048;
			else
				return 1;

			/* Locial paritions have a header. Account for that. */
			if (type == LOGICAL)
				size += image.align + 1;
		} else
			size = 0;

		return !disk_part_add(&image, size, type);
	} else if (strcmp(argv[1], "del") == 0 && argc == 3) {
		if (strcmp(argv[2], "all") == 0) {
			image.num_part = 0;
		} else {
			int i = disk_find_slot(&image, simple_strtoul(argv[2], NULL, 0));
			if (i < 0) {
				printf("partition not found!\n");
				return 1;
			}

			image.num_part--;
			while (i < image.num_part) {
				image.parts[i] = image.parts[i+1];
				i++;
			}
		}
	} else if (strcmp(argv[1], "toggle") == 0 && argc == 3) {
		int id = simple_strtoul(argv[2], NULL, 0);
		int slot = disk_find_slot(&image, id);
		if (slot < 0) {
			printf("partition not found!\n");
			return 1;
		}
		image.parts[slot].active = !image.parts[slot].active;
		printf("Partition %d now %s\n", id, image.parts[slot].active
			? "active" : "inactive");
	} else if (strcmp(argv[1], "save") == 0 && argc == 2) {
		return !!disk_save(&image);
	} else
		return 1;

	return 0;
}

U_BOOT_CMD(
	dospart, 4, 0, do_dospart,
	"manage DOS partitions",
	"\n"
	"    - show partititon table\n"
	"dospart dev <interface> <dev>\n"
	"    - set device <dev> on <interface> as the current device and\n"
	"      read the partition table\n"
	"dospart save\n"
	"    - save the partition table\n"
	"dospart add <type> [<size>]\n"
	"    - add new partition of <type> with <size>\n"
	"    - if <size> is not given the remaining free space is used completely\n"
	"    - <type> := primary | logical\n"
	"    - <size> := nn[KM]\n"
	"dospart del <id>|all\n"
	"    - del partition <id> or the whole partition table\n"
	"dospart toggle <id>\n"
	"    - toggle active flag of partition <id>"
);
