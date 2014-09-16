/*
 * (C) Copyright 2010
 * Dirk Hoerner, DSPG Technologies GmbH, <dirk.hoerner@dspg.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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
#include <malloc.h>
#include <jffs2/load_kernel.h>
#include <linux/list.h>
#include <linux/ctype.h>
#include <linux/err.h>
#include <linux/mtd/mtd.h>

#if defined(CONFIG_CMD_NAND)
#include <linux/mtd/nand.h>
#include <nand.h>
#endif

static int mtdprotect_markro(char *name)
{
	const char *mtdparts = getenv("mtdparts");
	char *p;
	char *new;
	
	/* if it does not exist yet, insert a "ro" after the partition name */
	new = malloc(strlen(mtdparts) + 3);
	if (!new)
		return 1;

	sprintf(new, "(%s)", name);
	p = strstr(mtdparts, new);
	p += strlen(new);

	if (strncmp(p, "ro", 2) == 0)
		return 0;

	strncpy(new, mtdparts, p - mtdparts);
	new[p - mtdparts] = '\0';

	strcat(new, "ro");
	strcat(new, p);

	setenv("mtdparts", new);
	free(new);

	return 0;
}

static int mtdprotect_unmarkro(char *name)
{
	const char *mtdparts = getenv("mtdparts");
	char *p;
	char *new;
	
	/* if it exists, remove the "ro" after the partion name */
	new = malloc(strlen(mtdparts) + 1);
	if (!new)
		return 1;

	sprintf(new, "(%s)", name);
	p = strstr(mtdparts, new);
	p += strlen(new);

	if (strncmp(p, "ro", 2) != 0)
		return 0;

	strncpy(new, mtdparts, p - mtdparts);
	new[p - mtdparts] = '\0';

	p += 2;
	strcat(new, p);

	setenv("mtdparts", new);
	free(new);

	return 0;
}

int do_mtdprotect(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	u8 pnum;
	struct mtd_device *dev;
	struct part_info *part;
	u8 buf[512];
	u32 empty = 0xffffffff;
	u32 i;
	int ret;
	size_t size = sizeof(buf);
	
	if (argc != 2) {
		cmd_usage(cmdtp);
		return 1;
	}

	if (find_dev_and_part(argv[1], &dev, &pnum, &part))
		return 1;

	ret = nand_read_skip_bad(&nand_info[dev->id->num], part->offset,
	                         &size, buf);
	if (ret)
		return 1;

	for (i = 0; i < sizeof(buf); i += sizeof(empty))
		if ((*(u32 *)&buf[i]) != empty)
			break;

	if (i < sizeof(buf))
		/* non-empty: mark read-only */
		ret = mtdprotect_markro(part->name);
	else
		/* empty: remove read-only mark */
		ret = mtdprotect_unmarkro(part->name);
	
	return ret;
}

U_BOOT_CMD(
	mtdprotect, 2, 0, do_mtdprotect,
	"protect given partition if it is non-empty",
	"part-id\n"
	"    - partition to protect (part-id = nand0,1)\n"
	"\n"
	"-----\n"
	"\n"
	"This command checks wether the first 512 bytes of the partition are\n"
	"empty, and makes the partition read-only if not. If they are empty,\n"
	"the partition is made writeable again. It works by modifying the\n"
	"mtdparts variable.\n"
	"Using this command a write-once partition can be implemented (since\n"
	"Linux respects the read-only flag in the kernel).\n"
);
