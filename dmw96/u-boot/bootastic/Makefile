VERSION = v1.10-rc2

CROSS_COMPILE ?= arm-eabi-

# configuration
USE_UART ?= 1

CFLAGS     += -DUSE_UART=$(USE_UART)
ifneq (,$(OSCILLATOR_CONFIG))
CFLAGS     += -DDW_OSCILLATOR_CONFIG=$(OSCILLATOR_CONFIG)
endif
ifneq (,$(CONFIG_SDRAM16BIT))
CFLAGS     += -DDW_SDRAM16BIT
endif

DRAM ?= lpddr2_100
ifeq (, $(wildcard ddrconf/$(DRAM).c))
  $(error invalid dram configuration)
else
  bootastic-obj += ddrconf/$(DRAM).o
endif

# check for legacy parameters
ifdef DDR
  $(error DDR parameter is not supported anymore, use DRAM)
endif
ifdef RAMSIZE
  $(error RAMSIZE parameter is not supported anymore, use DRAM)
endif
ifdef RAMTYPE
  $(error RAMTYPE parameter is not supported anymore, use DRAM)
endif

# do not change below

VERSION_MISMATCH = \
	"\n" \
	"Version mismatch between bootastic and u-boot! Please make sure \n" \
	"you use bootastic and u-boot from the same delivery.\n" \
	"\n" \
	"Build aborted.\n" \
	"\n"

CC = $(CROSS_COMPILE)gcc -g
LD = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
SIZE = $(CROSS_COMPILE)size

VERSION_FILE = version.h
CFLAGS_FILE  = .cflags
OBJECTS_FILE = .objects

bootastic-obj +=         \
	bootastic.o      \
	common.o         \
	romapi.o         \
	syscfg.o         \
	cmu.o            \
	gpio.o           \
	serial.o         \
	spi.o            \
	pmu.o            \
	ddr.o            \
	legacy_nand.o    \
	load.o

# Add GCC lib
LIB_GCC = -L $(shell dirname `$(CC) $(CFLAGS) -print-libgcc-file-name`) -lgcc
PLATFORM_LIBS += $(LIB_GCC)

gccincdir := $(shell $(CC) -print-file-name=include)

CFLAGS     += -fno-strict-aliasing -fno-common -ffixed-r8
CFLAGS     += -fno-builtin -ffreestanding -nostdinc -isystem $(gccincdir) -pipe
CFLAGS     += -mcpu=cortex-a8 -msoft-float

CFLAGS     += -Os -Wall -Wstrict-prototypes
CFLAGS     += -I./include

LDFLAGS += -Bstatic -T bootastic.lds

all: bootloader.img
	@echo
	@echo " bootastic.elf statistics:"
	@echo " -------------------------"
	@echo
	@$(SIZE) bootastic.elf
	@echo

help u-boot.bin:
	@cat README
	@exit 1

mkbootimg: mkbootimg.c Makefile
	@echo ' HOSTCC ' $@
	@gcc -O2 -o $@ $<

bootloader.img: bootastic.bin u-boot.bin mkbootimg Makefile
	@./scripts/versioncheck.sh || (echo -e $(VERSION_MISMATCH) && exit 1)
	@echo ' MKBIMG ' $@
	@./mkbootimg --preloader=bootastic.bin \
	             --loader=u-boot.bin \
	             --loader-addr=0x41000000 \
	             --crc32 \
	             --output=$@

bootastic.bin: bootastic.elf bootastic.lds Makefile
	@echo ' OBJCPY ' $@
	@$(OBJCOPY) --gap-fill=0xff -O binary bootastic.elf bootastic.bin

bootastic.elf: $(bootastic-obj) $(OBJECTS_FILE) Makefile
	@echo ' LD     ' $@
	@$(LD) $(LDFLAGS) --start-group $(bootastic-obj) --end-group \
		$(PLATFORM_LIBS) -o $@

%.o: %.c $(VERSION_FILE) $(CFLAGS_FILE)
	@echo ' CC     ' $<
	@$(CC) $(CFLAGS) -c $< -o $@


clean:
	rm -f mkbootimg *.image *.img bootastic.bin *.elf *.o

$(VERSION_FILE): FORCE
	@( printf '#define VERSION "%s%s"\n' \
	 "$(VERSION)" \
	 '$(shell scripts/setlocalversion .)' ) >> $@.tmp
	@cmp -s $@ $@.tmp && rm -f $@.tmp || mv -f $@.tmp $@

$(CFLAGS_FILE): FORCE
	@if test x"$(CFLAGS)" != x"`cat $@ 2>/dev/null`"; then \
		echo -e 1>&2 "\n * new build flags, rebuilding\n"; \
		echo "$(CFLAGS)" > $@; \
	 fi

$(OBJECTS_FILE): FORCE
	@if test x"$(bootastic-obj)" != x"`cat $@ 2>/dev/null`"; then \
		echo -e 1>&2 "\n * new object list, relinking\n"; \
		echo "$(bootastic-obj)" > $@; \
	 fi

.PHONY: FORCE
