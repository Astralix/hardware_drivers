/*
 * (C) Copyright 2002
 * Gary Jennejohn, DENX Software Engineering, <gj@denx.de>
 *
 * (C) Copyright 2006-2007
 * Oded Golombek, DSP Group, odedg@dsp.co.il
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

#include <config.h>

OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)
SECTIONS
{
	. = 0x00000000;
	. = ALIGN(4);
	.text	:
	{
		arch/arm/cpu/arm926ejs/start.o (.text)
	}

#ifdef RUN_CHARGER_FROM_SRAM
	. = ALIGN(4);
	.text_charger : {
		charger_start = .;
		board/dspg/dw/charger.o (.text)
		board/dspg/dw/charger.o (.data)
		charger_end = .;
	}
#endif

	. = ALIGN(4);
	.code	:
	{
		*(.text)
	}

	.rodata : { *(.rodata) }
	. = ALIGN(4);
	.data : { *(.data)
		  __relocation_table = .;
		}
	. = ALIGN(4);
	.got : { *(.got) }

	. = .;
	__u_boot_cmd_start = .;
	.u_boot_cmd : { *(.u_boot_cmd) }
	__u_boot_cmd_end = .;

	. = ALIGN(32768);
	.mmu_tables : { *(mmu_table) }

	. = ALIGN(4);
	__bss_start = .;
	.bss (NOLOAD) : { *(.bss) }
	_end = .;
}
