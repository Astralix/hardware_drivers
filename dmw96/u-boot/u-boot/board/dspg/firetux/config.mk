# firetux compiler config
#
# (C) Copyright 2007-2009, emlix GmbH, Germany
# Juergen Schoew <js@emlix.com>
#
# (C) Copyright 2008, DSPG Technologies GmbH, Germany
# (C) Copyright 2007, NXP Semiconductors Germany GmbH
# Matthias Wenzel, <nxp@mazzoo.de>
#
# See file CREDITS for list of people who contributed to this
# project.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA 02111-1307 USA
#

ifndef CONFIG_SYS_PSRAM
# SDRAM
TEXT_BASE = 0x20780000
else
# mobile pSRAM
TEXT_BASE = 0x90700000
endif

PLATFORM_CPPFLAGS += -fPIC -fPIE -fno-jump-tables # -msingle-pic-base

LDSCRIPT := $(OBJTREE)/board/$(BOARDDIR)/u-boot.lds
