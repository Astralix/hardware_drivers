#
# image should be loaded at 0x61000000
#

U_BOOT_RELEASE = dw 5.20
TEXT_BASE = 0x61000000

PLATFORM_CPPFLAGS += -fPIC -fPIE -fno-jump-tables # -msingle-pic-base

LDSCRIPT := $(OBJTREE)/board/$(BOARDDIR)/u-boot.lds
