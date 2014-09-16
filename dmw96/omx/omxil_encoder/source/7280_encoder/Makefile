BELLAGIO_ROOT =../../../omxil/libomxil-bellagio-0.9.3/

# If the machine is 64-bit and we are compiling for PC, force 32-bit compilation
ifeq ($(shell uname -m),x86_64)
	ifeq (,$(findstring pclinux,$(MAKECMDGOALS)))
		export M32 = -m32
	endif
	ifeq (,$(findstring install,$(MAKECMDGOALS)))
		export M32 = -m32
	endif
endif

# for SW model
INSTALL_DIR=$(BELLAGIO_ROOT)/build/lib/bellagio
CC=gcc
CFLAGS=$(M32) -O0 -finline-functions -Wno-inline -Wall -fPIC -Wunused-variable -DNDEBUG -pthread
LDFLAGS=$(M32) -Wl
ENCODER_API_VERSION = 7280
CFLAGS+= -DENC7280
INCDIR=software/inc

base_SRCS = ../msgque.c ../OSAL.c ../basecomp.c ../port.c ../util.c

libhantrovideoenc_SRCS = encoder_constructor_video.c encoder_h264.c
ifneq ($(ENCODER_API_VERSION), 8270)
ifneq ($(ENCODER_API_VERSION), 8290)
ifneq ($(ENCODER_API_VERSION), h1)
libhantrovideoenc_SRCS += encoder_mpeg4.c encoder_h263.c
endif
endif
endif

libhantrovideoenc_OBJS = $(base_SRCS:.c=.o) $(libhantrovideoenc_SRCS:.c=.o) encoder_video.o library_entry_point_video.o
libhantroimageenc_SRCS = encoder_constructor_image.c encoder_jpeg.c
libhantroimageenc_OBJS = $(base_SRCS:.c=.o) $(libhantroimageenc_SRCS:.c=.o) encoder_image.o library_entry_point_image.o

ENCODER_SW = $(ENCODER_RELEASE)/software/linux_reference/lib$(ENCODER_API_VERSION)enc.a
ENCODER_SYS = $(ENCODER_RELEASE)/system/models/enc$(ENCODER_API_VERSION)_asic_model.a

HANTRO_LIBS = $(ENCODER_SW)
HANTRO_LIBS += $(ENCODER_SYS)


# for HW
#CC=arm-none-linux-gnueabi-gcc
#CFLAGS =-mtune=arm926ej-s -fpic -DMEMALLOCHW
#ENCODER_API_VERSION = 8290
#INSTALL_DIR=/misc/export/work/sapi/omx
#INCDIR=software/source/inc

ENCODER_RELEASE  = ../../../$(ENCODER_API_VERSION)_encoder
MEMALLOC_RELEASE = ../../../$(ENCODER_API_VERSION)_encoder/software/linux_reference/memalloc

CFLAGS +=-DENCODER_TRACE
#CFLAGS +=-DOMX_ENCODER_TUNNELING_SUPPORT

## Enable only for OMX IL conformance testing ##
#CFLAGS += -DCONFORMANCE

#CFLAGS +=-O3 -fno-inline -Wall -pthread -DOMX_ENCODER_$(ENCODER_API_VERSION) $(M32)
CFLAGS += -I.. \
    -I../../headers \
    -I$(BELLAGIO_ROOT)/src \
    -I$(ENCODER_RELEASE)/$(INCDIR) \
    -I$(MEMALLOC_RELEASE)


all:
	@echo ""
	@echo "MAKE for OpenMAX IL Encoder Component"
	@echo ""
	@echo " Available targets:"
	@echo ""
	@echo "  pclinux          build image AND video encoder omxil component"
	@echo "  install          installs omxil component with Bellagio"
	@echo "  clean            deletes generated output"
	@echo ""
	@echo " Current settings:"
	@echo ""
	@echo "  Encoder API Version:           $(ENCODER_API_VERSION)"
	@echo "  Encoder Include Directory:     $(ENCODER_RELEASE)/$(INCDIR)"
	@echo "  Encoder Libraries:             $(HANTRO_LIBS)"
	@echo "  Bellagio Root:                 $(BELLAGIO_ROOT)"
	@echo ""

pclinux: libhantrovideoenc.so libhantroimageenc.so $(ENCODER_SW)

install: pclinux $(BELLAGIO_ROOT)/build/bin/omxregister-bellagio
	cp -vf libhantrovideoenc.so $(INSTALL_DIR)/
	cp -vf libhantroimageenc.so $(INSTALL_DIR)/
	$(BELLAGIO_ROOT)/build/bin/omxregister-bellagio $(INSTALL_DIR)/

clean:
	rm -f $(libhantrovideoenc_OBJS) libhantrovideoenc.so $(INSTALL_DIR)/libhantrovideoenc.so
	rm -f $(libhantroimageenc_OBJS) libhantroimageenc.so $(INSTALL_DIR)/libhantroimageenc.so
#	make -C $(ENCODER_RELEASE)/system $@

#$(ENCODER_SW):
#	make -C $(ENCODER_RELEASE)/system system

#$(ENCODER_SYS):
#	make -C $(ENCODER_RELEASE)/system system

libhantrovideoenc.so: $(libhantrovideoenc_OBJS) $(ENCODER_SW) $(ENCODER_SYS)
	@echo "[L] $@"
	@$(CC) $(LDFLAGS) -o libhantrovideoenc.so -Wl,-h -Wl,libhantrovideoenc.so -shared -Wl,--start-group $(libhantrovideoenc_OBJS) $(HANTRO_LIBS) -lrt -Wl,--end-group -pthread

libhantroimageenc.so: $(libhantroimageenc_OBJS) $(ENCODER_SW) $(ENCODER_SYS)
	@echo "[L] $@"
	@$(CC) $(LDFLAGS) -o libhantroimageenc.so -Wl,-h -Wl,libhantroimageenc.so -shared -Wl,--start-group $(libhantroimageenc_OBJS) $(HANTRO_LIBS) -lrt -Wl,--end-group -pthread

encoder_video.o : encoder.c
	@echo "[C] $<"
	@$(CC) $(CFLAGS) -DOMX_ENCODER_VIDEO_DOMAIN -c $< -o $@

encoder_image.o : encoder.c
	@echo "[C] $<"
	@$(CC) $(CFLAGS) -DOMX_ENCODER_IMAGE_DOMAIN -c $< -o $@

library_entry_point_video.o : library_entry_point.c
	@echo "[C] $<"
	$(CC) $(CFLAGS) -DOMX_ENCODER_VIDEO_DOMAIN -c $< -o $@

library_entry_point_image.o : library_entry_point.c
	@echo "[C] $<"
	$(CC) $(CFLAGS) -DOMX_ENCODER_IMAGE_DOMAIN -c $< -o $@

%.o : %.c
	@echo "[C] $<"
	@$(CC) $(CFLAGS) -c $< -o $@
