#
# (C) Copyright 2010
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

LIB	= $(obj)lib$(BOARD).o

COBJS	:= dmw.o clock.o pads.o gpio.o serial.o keypad.o dmw_timer.o timer.o lcd.o irq.o pm.o \
	   amoled.o ccu.o
SOBJS	:= lowlevel_init.o

obj-$(CONFIG_DMW_DP52)    += dp52.o
obj-$(CONFIG_DMW_CHARGER) += dp52-charger.o battery_gpx.o
obj-$(CONFIG_BOOTMENU)    += bootmenu.o
obj-$(CONFIG_NAND_DMW96)  += tnand.o

SRCS	:= $(SOBJS:.o=.S) $(COBJS:.o=.c) $(obj-y:.o=.c)
OBJS	:= $(addprefix $(obj),$(COBJS)) $(addprefix $(obj),$(obj-y))
SOBJS	:= $(addprefix $(obj),$(SOBJS))

all:	$(LIB)

$(LIB):	$(obj).depend $(OBJS) $(SOBJS)
	$(call cmd_link_o_target, $(OBJS) $(SOBJS))

clean:
	rm -f $(SOBJS) $(OBJS)

distclean: clean
	rm -f $(LIB) core *.bak .depend

.PHONY: FORCE

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################
