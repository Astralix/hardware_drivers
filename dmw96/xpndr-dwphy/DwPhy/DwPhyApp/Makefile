#
# DwPhy Application
#

SHELL = /bin/sh

CC = /opt/RC-Build-Environment/2010-12/arm-gcc-4.1.1-gnueabi/bin/arm-none-linux-gnueabi-gcc
CC_OPTIONS = -g3 -Wall 
CC_DEFINES = -DBER_LINUX -DBER_LINUX_DRIVER -DDWPHY_NOT_DRIVER

VPATH = ../source
INCLUDE = -I ../include
LIB = -lvwal -L/opt/arm/target/lib

NAME  = /opt/arm/target/opt/dwphy

FILES = DwPhy.c \
        DwPhyApp.c \
        DwPhyUtil.c

OBJ_FILES = $(subst ,,$(FILES:.c=.o))

# -----------
# Build Rules
# -----------
%.o: %.c
	$(CC) -c $(CC_OPTIONS) $(CC_DEFINES) $(INCLUDE) $+

all: $(OBJ_FILES)
	$(CC) $(OBJ_FILES) $(LIB) -o $(NAME)

clean:
	rm -f *.o 
