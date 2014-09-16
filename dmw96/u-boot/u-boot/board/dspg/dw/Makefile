#
# (C) Copyright 2000-2006
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# (C) Copyright 2006-2007
# Oded Golombek, DSP Group, odedg@dsp.co.il
#
# (C) Copyright 2008
# DSP Group
#
# (C) Copyright 2009
# DSPG Technologies GmbH
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

include $(TOPDIR)/config.mk
include $(TOPDIR)/include/config.mk

LIB	= $(obj)lib$(BOARD).a

COBJS	:= dw.o lowlevel.o serial.o clock.o gpio.o spi.o
COBJS	+= lcd.o lcd_bmp.o macb.o dp52.o
COBJS	+= tdt320t2g706.o bootmenu.o keypad.o

obj-$(CONFIG_DPCHARGE_EN)    += pm_core.o 
obj-$(CONFIG_CMD_DPCALIBRATION) += charger.o
obj-$(DW_CONFIG_NAND_SUPPORT) += nand.o \
				 dspg_nand/dw_nfls_cntl.o \
				 dspg_nand/dw_nand.o \
				 dspg_nand/dw_nand_bbt.o \
				 dspg_nand/dw_dma_cntl.o \
				 dspg_nand/dw_polynom.o

SOBJS	:= lowlevel_init.o dpm-idle.o mmu.o mmu-table.o

SRCS	:= $(SOBJS:.o=.S) $(COBJS:.o=.c) $(obj-y:.o=.c)
OBJS	:= $(addprefix $(obj),$(COBJS)) $(addprefix $(obj),$(obj-y))
SOBJS	:= $(addprefix $(obj),$(SOBJS))

all:	$(LIB)

$(LIB):	$(obj).depend $(OBJS) $(SOBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS) $(SOBJS)

clean:
	rm -f $(SOBJS) $(OBJS)

distclean:	clean
	rm -f $(LIB) core *.bak .depend

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################
