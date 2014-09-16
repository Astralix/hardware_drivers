#
# (C) Copyright 2000-2007
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
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
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA 02111-1307 USA
#
# Portions Copyright (c) 2011 Intelligraphics, Inc.
# stephen.smith@intelligraphics.com
#


include $(TOPDIR)/config.mk

LIB	:= $(obj)libusb_dwc.a

COBJS-$(CONFIG_DWC_OTG_UDC) += dwc_otg_udc.o
COBJS-$(CONFIG_DWC_OTG_UDC) += dwc_otg_driver.o
COBJS-$(CONFIG_DWC_OTG_UDC) += dwc_otg_cil.o
COBJS-$(CONFIG_DWC_OTG_UDC) += dwc_otg_cil_intr.o
COBJS-$(CONFIG_DWC_OTG_UDC) += dwc_otg_pcd.o
COBJS-$(CONFIG_DWC_OTG_UDC) += dwc_otg_pcd_intr.o
COBJS-$(CONFIG_DWC_OTG_UDC) += dwc_otg_pcd_uboot.o

# platforms
COBJS-$(CONFIG_DSPG_DMW) += dwc_otg_dmw96.o

COBJS	:= $(COBJS-y)
SRCS	:= $(COBJS:.o=.c)
OBJS	:= $(addprefix $(obj),$(COBJS))

CFLAGS += -DDWC_UBOOT
CFLAGS += -DDWC_DEVICE_ONLY
#CFLAGS += -DDEBUG
#CFLAGS += -DDEBUG_EP0
#CFLAGS += -DVERBOSE
## currently DMA not working: ERROR::dwc_otg_pcd_handle_out_ep_intr:3801: EP0 OUT AHB Error
CFLAGS += -DDWC_NO_DMA
CFLAGS += -DDWC_NO_ADP
ifeq ($(CONFIG_USB_HIGH_SPEED),)
CFLAGS += -DCONFIG_DWC_FULL_SPEED
endif

all:	$(LIB)

$(LIB):	$(obj).depend $(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################
