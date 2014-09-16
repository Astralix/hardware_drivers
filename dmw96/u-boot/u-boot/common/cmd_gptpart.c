/*
 * (C) Copyright 2012
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

#include <stdlib.h>
#include <common.h>
#include <command.h>
#include <errno.h>
#include <part.h>
#include <malloc.h>
#include <linux/bitops.h>

#define DEFAULT_ALIGNMENT 12 /* 2 megabytes */
#define MAX_PARTS 16

#define MBR_LINUX_PARTITION       0x83
#define MBR_PROTECTIVE_PARTITION  0xee

#define GPT_ATTR_ACTIVE	(1 << 2)

struct part {
	unsigned char slot;
	char name[37]; /* support only 7bit ASCII */

	uint8_t type_guid[16];
	uint8_t part_guid[16];
	uint64_t start;
	uint64_t size;
	uint64_t attr;
};

/*
 * This is the central structure which holds the partitioning information,
 * ordered by the layout on the disk.
 */
struct disk {
	struct block_dev_desc *dev;
	unsigned int align; /* actual alignment is 2^align blocks */
	uint64_t first;
	uint64_t last;
	uint8_t guid[16];
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

struct gpt_part {
	uint8_t type_guid[16];
	uint8_t part_guid[16];
	uint8_t first[8];
	uint8_t last[8];
	uint8_t attr[8];
	uint8_t name[72];
} __attribute__((packed));

struct gpt_header {
	uint8_t signature[8];
	uint8_t revision[4];
	uint8_t header_size[4];
	uint8_t header_crc32[4];
	uint8_t reserved1[4];
	uint8_t my_lba[8];
	uint8_t alternate_lba[8];
	uint8_t first_usable_lba[8];
	uint8_t last_usable_lba[8];
	uint8_t disk_guid[16];
	uint8_t partition_entry_lba[8];
	uint8_t num_partition_entries[4];
	uint8_t sizeof_partition_entry[4];
	uint8_t partition_entry_array_crc32[4];
	uint8_t reserved2[512 - 92];
} __attribute__ ((packed));

#define EFI_GUID(a,b,c,d0,d1,d2,d3,d4,d5,d6,d7) \
	{ (a) & 0xff, ((a) >> 8) & 0xff, ((a) >> 16) & 0xff, ((a) >> 24) & 0xff, \
		(b) & 0xff, ((b) >> 8) & 0xff, \
		(c) & 0xff, ((c) >> 8) & 0xff, \
		(d0), (d1), (d2), (d3), (d4), (d5), (d6), (d7) }

static uint8_t partition_linux_data_guid[16] = EFI_GUID(0x0FC63DAF, 0x8483,
	0x4772, 0x8E, 0x79, 0x3D, 0x69, 0xD8, 0x47, 0x7D, 0xE4);

static uint8_t partition_env_guid[16] = EFI_GUID(0x1be46979, 0x3291, 0x4728,
	0x8c, 0xb6, 0xa2, 0x05, 0x85, 0x12, 0x1a, 0x0c);

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

static uint64_t le64_to_ull(uint8_t *le64)
{
	return (((uint64_t)le64[7] << 56) +
	        ((uint64_t)le64[6] << 48) +
	        ((uint64_t)le64[5] << 40) +
	        ((uint64_t)le64[4] << 32) +
	        ((uint64_t)le64[3] << 24) +
	        ((uint64_t)le64[2] << 16) +
	        ((uint64_t)le64[1] <<  8) +
	         (uint64_t)le64[0]);
}

static void ull_to_le64(uint8_t *le64, uint64_t val)
{
	le64[0] = val;
	le64[1] = val >> 8;
	le64[2] = val >> 16;
	le64[3] = val >> 24;
	le64[4] = val >> 32;
	le64[5] = val >> 40;
	le64[6] = val >> 48;
	le64[7] = val >> 56;
}

static uint32_t gpt_crc32(const void *buf, unsigned long len)
{
	return crc32(0, buf, len);
}

static int disk_used_slots(struct disk *disk)
{
	int i, used = 0;

	for (i = 0; i < disk->num_part; i++)
		used |= 1 << disk->parts[i].slot;

	return used;
}

static int disk_read_blocks(struct disk *disk, unsigned long lba,
			    unsigned long blocks, void *buf)
{
	if (disk->dev->block_read(disk->dev->dev, lba, blocks, buf) == blocks)
		return 0;
	else
		return -EIO;
}

static int disk_write_blocks(struct disk *disk, unsigned long lba,
			     unsigned long blocks, void *buf)
{
	if (disk->dev->block_write(disk->dev->dev, lba, blocks, buf) == blocks)
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

	/* insert into slot */
	for (i=disk->num_part; i>slot; i--)
		disk->parts[i] = disk->parts[i-1];

	disk->parts[slot] = *p;
	disk->num_part++;

	return 0;
}

static int is_gpt_part_valid(struct gpt_part *part)
{
	int i;
	uint8_t sum = 0;

	for (i=0; i<16; i++)
		sum |= part->type_guid[i];

	return sum != 0;
}

static int disk_parse(struct disk *disk)
{
	struct mbr mbr;
	struct gpt_header gpt;
	int ret, slot, size;
	uint32_t crc;
	struct gpt_part *parts;

	/*
	 * Check PMBR, that is look for the boot signature and for the
	 * protective partition.
	 */
	if ((ret = disk_read_blocks(disk, 0, 1, &mbr)))
		return ret;

	if (mbr.signature[0] != 0x55 || mbr.signature[1] != 0xaa)
		return -ENOENT;

	for (slot=0; slot<4; slot++) {
		if (mbr.parts[slot].type == MBR_PROTECTIVE_PARTITION &&
		    le32_to_ulong(mbr.parts[slot].start_block) == 1)
			goto pmbr_valid;
	}
	return -ENOENT;

pmbr_valid:
	/*
	 * Read and validate the GPT. For simplicity we don't look at the
	 * alternate GPT.
	 */
	if ((ret = disk_read_blocks(disk, 1, 1, &gpt)))
		return ret;

	if (memcmp(gpt.signature, "EFI PART", 8))
		return -EINVAL;

	crc = le32_to_ulong(gpt.header_crc32);
	ulong_to_le32(gpt.header_crc32, 0);
	if (crc != gpt_crc32(&gpt, le32_to_ulong(gpt.header_size)))
		return -EINVAL;

	if (le32_to_ulong(gpt.sizeof_partition_entry) != 128) {
		printf("Unsupported entry size: %lu\n",
			le32_to_ulong(gpt.sizeof_partition_entry));
		return -EINVAL;
	}

	if (le64_to_ull(gpt.my_lba) != 1)
		return -EINVAL;

	disk->first = le64_to_ull(gpt.first_usable_lba);
	if (disk->first > disk->dev->lba)
		return -EINVAL;
	disk->last  = le64_to_ull(gpt.last_usable_lba);
	if (disk->last > disk->dev->lba)
		return -EINVAL;

	memcpy(disk->guid, gpt.disk_guid, 16);

	/*
	 * Looks good so far. Read the partition table and continue
	 * verification.
	 */
	size = le32_to_ulong(gpt.num_partition_entries) * 128;
	parts = malloc(size);
	if (!parts)
		return -ENOMEM;

	if ((ret = disk_read_blocks(disk, le64_to_ull(gpt.partition_entry_lba),
				    (size + 511) / 512, parts)))
		goto error;

	if (le32_to_ulong(gpt.partition_entry_array_crc32) != gpt_crc32(parts, size)) {
		ret = -EINVAL;
		goto error;
	}

	/* scan partition table */
	disk->num_part = 0;
	size = le32_to_ulong(gpt.num_partition_entries);
	for (slot = 0; slot < size; slot++) {
		struct part p;
		int i;
		uint8_t *pn;

		if (!is_gpt_part_valid(parts + slot))
			continue;

		p.slot = slot;
		memcpy(p.type_guid, parts[slot].type_guid, 16);
		memcpy(p.part_guid, parts[slot].part_guid, 16);
		p.start = le64_to_ull(parts[slot].first);
		p.size = le64_to_ull(parts[slot].last) - p.start + 1;
		p.attr = le64_to_ull(parts[slot].attr);
		memset(p.name, 0, sizeof(p.name));

		/* parse name and 7 bit ASCII */
		for (i = 0, pn = parts[slot].name; i < 36 && (pn[0] || pn[1]); i++, pn += 2) {
			if (pn[1] || pn[0] < 0x20 || pn[0] > 0x7f)
				p.name[i] = '?';
			else
				p.name[i] = pn[0];
		}

		if ((ret = disk_parse_add_part(disk, &p)))
			goto error;

		/* adjust alignment downwards if needed */
		i = ffs(p.start) - 1;
		if (i < disk->align)
			disk->align = i;
	}

	ret = 0;

error:
	free(parts);
	return ret;
}

static int disk_save_hybrid_mbr(struct disk *disk)
{
	struct mbr mbr;
	int i, ret;
	unsigned long lba;

	if ((ret = disk_read_blocks(disk, 0, 1, &mbr)))
		return ret;

	/* clear out mbr partitions */
	mbr.signature[0] = 0x55;
	mbr.signature[1] = 0xaa;
	for (i = 1; i < 4; i++) {
		ulong_to_le32(mbr.parts[i].num_blocks, 0);
		mbr.parts[i].status = 0;
		mbr.parts[i].type = 0;
	}

	/* find the last block of the protective partition */
	lba = disk->dev->lba;
	i = 0;
	while (i < disk->num_part) {
		if (disk->parts[i].attr & GPT_ATTR_ACTIVE) {
			lba = disk->parts[i].start;
			break;
		}
		i++;
	}

	/* first the protective partition */
	mbr.parts[0].status = 0x00;
	mbr.parts[0].type = MBR_PROTECTIVE_PARTITION;
	ulong_to_le32(mbr.parts[0].start_block, 1);
	ulong_to_le32(mbr.parts[0].num_blocks, lba - 1);

	/* create active MBR partition */
	if (i < disk->num_part) {
		mbr.parts[1].status = 0x80;
		mbr.parts[1].type = MBR_LINUX_PARTITION;
		ulong_to_le32(mbr.parts[1].start_block, disk->parts[i].start);
		ulong_to_le32(mbr.parts[1].num_blocks, disk->parts[i].size);

		/* also protect the rest */
		lba = disk->parts[i].start + disk->parts[i].size;
		if (lba < disk->dev->lba) {
			mbr.parts[2].status = 0x00;
			mbr.parts[2].type = MBR_PROTECTIVE_PARTITION;
			ulong_to_le32(mbr.parts[2].start_block, lba);
			ulong_to_le32(mbr.parts[2].num_blocks, disk->dev->lba - lba);
		}
	}

	/* write back mbr */
	return disk_write_blocks(disk, 0, 1, &mbr);
}

/*
 * We're always writing out 128 partition entries because GNU parted somehow
 * seems to get confused if we write less than that.
 */

static int disk_save_gpt_parts(struct disk *disk, uint64_t lba, uint32_t *crc)
{
	struct gpt_part *parts;
	int size = 128 * 128 /*disk->num_part*/;
	int ret = 0;
	int i;

	if (!size) {
		*crc = 0;
		return 0;
	}

	parts = malloc(size);
	if (!parts)
		return -ENOMEM;
	memset(parts, 0, size);

	for (i = 0; i < disk->num_part; i++) {
		struct part *part = disk->parts + i;
		struct gpt_part *gpt = parts + i;
		char *name;
		int j;

		memcpy(gpt->type_guid, part->type_guid, 16);
		memcpy(gpt->part_guid, part->part_guid, 16);
		ull_to_le64(gpt->first, part->start);
		ull_to_le64(gpt->last, part->start + part->size - 1);
		ull_to_le64(gpt->attr, part->attr);

		name = part->name;
		for (j = 0; j < 72 && *name; j += 2, name++)
			gpt->name[j] = *name;
	}

	*crc = gpt_crc32(parts, size);

	ret = disk_write_blocks(disk, lba, (size+511)/512, parts);

	free(parts);
	return ret;
}

static int disk_save_gpt(struct disk *disk, uint64_t header_lba,
			 uint64_t backup_lba, uint32_t parts_lba)
{
	struct gpt_header header;
	int ret;
	uint32_t parts_crc;

	ret = disk_save_gpt_parts(disk, parts_lba, &parts_crc);
	if (ret)
		return ret;

	memcpy(header.signature, "EFI PART", 8);
	ulong_to_le32(header.revision,  0x010000);
	ulong_to_le32(header.header_size, 92);
	ulong_to_le32(header.header_crc32, 0); /* zero for the CRC calculation */
	memset(header.reserved1, 0, sizeof(header.reserved1));
	ull_to_le64(header.my_lba, header_lba);
	ull_to_le64(header.alternate_lba, backup_lba);
	ull_to_le64(header.first_usable_lba, disk->first);
	ull_to_le64(header.last_usable_lba, disk->last);
	memcpy(header.disk_guid, disk->guid, 16);
	ull_to_le64(header.partition_entry_lba, parts_lba);
	ulong_to_le32(header.num_partition_entries, 128 /*disk->num_part*/);
	ulong_to_le32(header.sizeof_partition_entry, 128);
	ulong_to_le32(header.partition_entry_array_crc32, parts_crc);
	memset(header.reserved2, 0, sizeof(header.reserved2));

	ulong_to_le32(header.header_crc32, gpt_crc32(&header, 92));

	return disk_write_blocks(disk, header_lba, 1, &header);
}

static int disk_save(struct disk *disk)
{
	int ret;

	ret = disk_save_hybrid_mbr(disk);
	if (ret)
		goto error;

	ret = disk_save_gpt(disk, 1, disk->dev->lba-1, 2);
	if (ret)
		goto error;

	ret = disk_save_gpt(disk, disk->dev->lba-1, 1, disk->last+1);
	if (ret)
		goto error;

	return 0;

error:
	printf("Write back failed: %d\n", ret);
	return ret;
}

static int disk_part_adjust(struct disk *disk, struct part *p, struct part *next)
{
	uint64_t from, len, max;
	uint64_t align = (1ull << disk->align) - 1;

	max = next ? next->start : disk->last + 1;
	from = (p->start+align) & ~align; /* round up */

	len = p->size;
	if (len) {
		if (from+len > max)
			return 0;
	} else {
		if (from >= max)
			return 0;
		len = max - from;
	}

	p->start = from;
	p->size = len;
	return 1;
}

static int disk_part_add_between(struct disk *disk, int slot, struct part *prev,
                                 struct part *next, uint8_t *type_guid,
				 unsigned long size, char *name)
{
	struct part p;
	int i;
	int used = disk_used_slots(disk);

	/* try to insert a new partition */
	p.slot = 0;
	p.start = prev ? prev->start + prev->size : disk->first;
	p.size = size;
	memcpy(p.type_guid, type_guid, 16);
	for (i = 0; i < 16; i++)
		p.part_guid[i] = rand();
	p.attr = 0;
	strncpy(p.name, name, sizeof(p.name));
	while (used & (1 << p.slot))
		p.slot++;

	if (!disk_part_adjust(disk, &p, next))
		return 0;

	/* finally add it */
	for (i=disk->num_part; i>slot; i--)
		disk->parts[i] = disk->parts[i-1];

	disk->parts[slot] = p;
	disk->num_part++;

	return 1;
}

static int disk_part_add(struct disk *disk, uint8_t *type, unsigned long size,
			 char *name)
{
	struct part *p = NULL, *prev = NULL;
	int i, ret;

	if (disk->num_part >= MAX_PARTS) {
		printf("Error: reached MAX_PARTS\n");
		return 0;
	}

	if (size) {
		for (i = 0, p = disk->parts; i < disk->num_part; i++, p++) {
			if (disk_part_add_between(disk, i, prev, p, type, size, name))
				return 1;
			prev = p;
		}
	} else {
		/* no size -> use all available at end */
		i = disk->num_part;
		if (disk->num_part)
			prev = disk->parts + disk->num_part - 1;
	}

	ret = disk_part_add_between(disk, i, prev, NULL, type, size, name);
	if (!ret)
		printf("Could not add partition!\n");

	return ret;
}

static int disk_find_slot(struct disk *disk, int slot)
{
	int i = 0;

	slot -= 1;
	while (i < disk->num_part && disk->parts[i].slot != slot)
		i++;

	return (i >= disk->num_part) ? -1 : i;
}

static void disk_print_unused(struct disk *disk, struct part *prev, struct part *next)
{
	struct part p;

	p.start = prev ? prev->start + prev->size : disk->first;
	p.size = 0; /* use all available space */

	if (!disk_part_adjust(disk, &p, next))
		return;

	printf("Free                       ");
	if (p.size >= 20480)
		printf("%5lluM\n", p.size/2048);
	else
		printf("%5lluK\n", p.size/2);
}

static void print_alignment(struct disk *disk)
{
	printf("Alignment: ");
	if (disk->align >= 11)
		printf("%dM\n", 1 << (disk->align-11));
	else if (disk->align)
		printf("%dK\n", 1 << (disk->align-1));
	else
		printf("none\n");
}

static void disk_print(struct disk *disk)
{
	struct part *p = NULL, *prev = NULL;
	int i;

	printf("       First       Last\n");
	printf(" Id    Sector     Sector   Length  Type    Flag   Name\n");
	printf("---- ---------- ---------- ------ ------- ------ -------\n");

	for (i = 0, p = disk->parts; i < disk->num_part; i++, p++) {
		disk_print_unused(disk, prev, p);

		printf("%4d %10llu %10llu ", p->slot+1, p->start,
		       p->start+p->size-1);

		if (p->size >= 20480)
			printf("%5lluM ", p->size/2048);
		else
			printf("%5lluK ", p->size/2);

		if (!memcmp(p->type_guid, partition_linux_data_guid, 16))
			printf("data    ");
		else if (!memcmp(p->type_guid, partition_env_guid, 16))
			printf("env     ");
		else
			printf("unknown ");

		if (p->attr & GPT_ATTR_ACTIVE)
			printf("Active ");
		else
			printf("       ");

		printf("%s\n", p->name);

		prev = p;
	}

	disk_print_unused(disk, prev, NULL);
	printf("\n");
	print_alignment(disk);
}

/*****************************************************************************/

static struct disk *image;

static int do_gptpart(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	if (!image) {
		image = malloc(sizeof(struct disk));
		if (!image) {
			printf("OOM!\n");
			return 1;
		}
		memset(image, 0, sizeof(struct disk));
	}

	/* we need real random numbers! */
	srand(get_timer(0));

	if (argc == 4 && !strcmp(argv[1], "dev")) {
		int ret, dev;

		dev = simple_strtoul(argv[3], NULL, 0);
		image->dev = get_dev(argv[2], dev);

		if (image->dev == NULL) {
			puts("\n** Invalid block device **\n");
			return 1;
		}

		printf("Size: %luMB (%lu blocks)\n", image->dev->lba / 2048,
		       image->dev->lba);

		/* TODO: query device alignment */
		image->align = DEFAULT_ALIGNMENT;

		if ((ret = disk_parse(image))) {
			int i;

			printf("Could not parse disk: %d. Starting empty...\n", ret);
			image->num_part = 0;
			image->first = 34;
			image->last  = image->dev->lba - 34;
			for (i=0; i<16; i++)
				image->guid[i] = rand();
		}

		return 0;
	}

	if (!image->dev) {
		printf("No device set!\n");
		return 1;
	}

	if (argc <= 1) {
		disk_print(image);
		return 0;
	}

	if (strcmp(argv[1], "add") == 0 && (argc == 3 || argc == 4 || argc == 5)) {
		unsigned long size;
		char *next, *name = "";
		uint8_t *type_guid;

		if (strcmp(argv[2], "data") == 0)
			type_guid = partition_linux_data_guid;
		else if (strcmp(argv[2], "env") == 0)
			type_guid = partition_env_guid;
		else
			return 1;

		if (argc >= 4)
			name = argv[3];

		if (argc == 5) {
			size = simple_strtoul(argv[4], &next, 0);
			if (*next == 'k' || *next == 'K')
				size *= 2;
			else if (*next == 'm' || *next == 'M')
				size *= 2048;
			else
				return 1;
		} else
			size = 0;

		return !disk_part_add(image, type_guid, size, name);
	} else if (strcmp(argv[1], "del") == 0 && argc == 3) {
		if (strcmp(argv[2], "all") == 0) {
			image->num_part = 0;
		} else {
			int i = disk_find_slot(image, simple_strtoul(argv[2], NULL, 0));
			if (i < 0) {
				printf("partition not found!\n");
				return 1;
			}

			image->num_part--;
			while (i < image->num_part) {
				image->parts[i] = image->parts[i+1];
				i++;
			}
		}
	} else if (strcmp(argv[1], "toggle") == 0 && argc == 3) {
		int id = simple_strtoul(argv[2], NULL, 0);
		int slot = disk_find_slot(image, id);
		if (slot < 0) {
			printf("partition not found!\n");
			return 1;
		}
		image->parts[slot].attr ^= GPT_ATTR_ACTIVE;
		printf("Partition %d now %s\n", id,
			(image->parts[slot].attr & GPT_ATTR_ACTIVE)
			? "active" : "inactive");
	} else if (strcmp(argv[1], "save") == 0 && argc == 2) {
		return !!disk_save(image);
	} else if (strcmp(argv[1], "align") == 0 && argc == 3) {
		char *next;
		unsigned long align;

		align = simple_strtoul(argv[2], &next, 0);
		align = ffs(align);

		if (*next == 'm' || *next == 'M')
			align = align ? align+10 : 0;
		else if (*next != 'k' && *next != 'K')
			return 1;

		image->align = align;
		print_alignment(image);
	} else
		return 1;

	return 0;
}

U_BOOT_CMD(
	gptpart, 5, 0, do_gptpart,
	"manage GPT partitions",
	"\n"
	"    - show partititon table\n"
	"gptpart dev <interface> <dev>\n"
	"    - set device <dev> on <interface> as the current device and\n"
	"      read the partition table\n"
	"gptpart save\n"
	"    - save the partition table\n"
	"gptpart add <type> [<name>] [<size>]\n"
	"    - add new partition of <type> with <size>\n"
	"    - if <size> is not given the remaining free space is used completely\n"
	"    - <type> := data|env\n"
	"    - <name> := partition label\n"
	"    - <size> := nn[KM]\n"
	"gptpart del <id>|all\n"
	"    - del partition <id> or the whole partition table\n"
	"gptpart toggle <id>\n"
	"    - toggle active flag of partition <id>\n"
	"    - only up to three partitions may be activated\n"
	"gptpart align nn[KM]\n"
	"    - change alignment for new partitions"
);

