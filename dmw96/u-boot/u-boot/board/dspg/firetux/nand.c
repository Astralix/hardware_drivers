/*
 * pnx8181 nandflash driver
 *
 * (C) Copyright 2008-2009, emlix GmbH, Germany
 * Juergen Schoew <js@emlix.com>
 *
 * (C) Copyright 2008, DSPG Technologies GmbH, Germany
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <nand.h>
#include <asm/io.h>

static void firetux_nand_hwcontrol(struct mtd_info *mtd, int dat,
						unsigned int ctrl)
{
	struct nand_chip *chip = mtd->priv;
	void *IO_ADDR_W = chip->IO_ADDR_W;

	if (ctrl & NAND_CLE)
		IO_ADDR_W += CONFIG_SYS_NAND_CLE_ADDR;

	if (ctrl & NAND_ALE)
		IO_ADDR_W += CONFIG_SYS_NAND_ALE_ADDR;

	if (dat != NAND_CMD_NONE) {
		if (chip->options & NAND_BUSWIDTH_16)
			writew((unsigned short)dat, IO_ADDR_W);
		else
			writeb((unsigned char)dat, IO_ADDR_W);
	}
}


static int firetux_nand_readybusy(struct mtd_info *mtd)
{
	return (readl((void *)CONFIG_SYS_NAND_RB_PORT)
					>> CONFIG_SYS_NAND_RB_BIT) & 1;
}


int board_nand_init(struct nand_chip *nand)
{
	nand->cmd_ctrl	 = firetux_nand_hwcontrol;
	nand->dev_ready	 = firetux_nand_readybusy;
	nand->chip_delay = 20;
	nand->ecc.mode	 = NAND_ECC_SOFT;
	nand->options	 = NAND_USE_FLASH_BBT;
	return 0;
}
