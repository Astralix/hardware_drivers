# for SW
CFLAGS = -finline-functions -Wno-inline -Wall -fPIC -Wunused-variable -DNDEBUG -g -O0
LDFLAGS=-Wl#,--strip-all
INSTALL_DIR=.
CC=gcc
# for HW
#CC=arm-none-linux-gnueabi-gcc
#CFLAGS =-mtune=arm926ej-s -fpic
#INSTALL_DIR=/tmp
#CFLAGS += -DVIDEO_ONLY

DEC_DIR=../../../../g1_decoder
REAL_PARSER_DIR=$(DEC_DIR)/software/test/rv/rm_parser


CFLAGS +=-I"../.." -I"$(DEC_DIR)/software/source/inc" -I"$(REAL_PARSER_DIR)/include" -I.  -I"../../../headers" -I"$(DEC_DIR)/software/test/vp8" -pthread
LDFLAGS +=-Wl,-R -Wl,".." -Wl,-rpath-link -Wl,".." -pthread
CFLAGS += -m32
LDFLAGS += -m32

all: video_decoder

clean:
	rm -f *.o video_decoder



vid.o: video_decoder.c
	$(CC) $(CFLAGS) -c $< -o $@

rv_ff_read.o: rv_ff_read.c
	$(CC) $(CFLAGS) -c $< -o $@

ifneq (,$(findstring -DVIDEO_ONLY, $(CFLAGS)))
video_decoder: rv_ff_read.o vid.o ../libhantrovideodec.so $(REAL_PARSER_DIR)/librvparser.a
else
video_decoder: rv_ff_read.o vid.o ../libhantrovideodec.so ../libhantroimagedec.so $(REAL_PARSER_DIR)/librvparser.a
endif
	$(CC) $(LDFLAGS) -Wl,--start-group $^ -ldl -lm -lrt -Wl,--end-group -pthread -o $@

install:
	cp video_decoder $(INSTALL_DIR)

