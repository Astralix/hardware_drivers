##############################################################################
#
#    Copyright (c) 2005 - 2012 by Vivante Corp.  All rights reserved.
#
#    The material in this file is confidential and contains trade secrets
#    of Vivante Corporation. This is proprietary information owned by
#    Vivante Corporation. No part of this work may be disclosed,
#    reproduced, copied, transmitted, or used in any way for any purpose,
#    without the express written permission of Vivante Corporation.
#
##############################################################################


# Source files.
SOURCES = gc_vdk_gl.c \
          gc_vdk_egl.c

SOURCES += $(if $(findstring Darwin,$(PLATFORM)),gc_vdk_x.c,gc_vdk_fbdev.c)

# Includes.
INCLUDES = ../inc $(AQROOT)/hal/inc

# Libraries.
LIBRARIES = GAL $(if $(findstring Darwin,$(PLATFORM)),X11)
LIB_DIR   = $(if $(findstring Darwin,$(PLATFORM)),/usr/X11R6/lib)

# Target.
TARGET = libVDK.so

# Include common makefile.
include $(AQROOT)/mak/common.mak
