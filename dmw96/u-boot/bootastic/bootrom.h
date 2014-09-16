#ifndef __BOOTROM_H
#define __BOOTROM_H

struct image_header {
	uint8_t  signature[4];		/* "DSPG" */
	uint32_t flags;			/* optional flags */
	uint32_t key_offset;		/* key: starts here */
	uint32_t key_size;		/* key: size */
	uint32_t pre_vrl_offset;	/* preloader: vrl offset */
	uint32_t pre_offset;		/* preloader: starts at offset */
	uint32_t pre_size;		/* preloader: size of preloader */
	uint32_t pre_crc32;		/* preloader: crc32 checksum */
	uint32_t load_vrl_offset;	/* loader:    vrl offset */
	uint32_t load_offset;		/* loader:    starts at offset */
	uint32_t load_size;		/* loader:    size of loader */
	uint32_t load_addr;		/* loader:    destination address */
	uint32_t load_crc32;		/* loader:    crc32 checksum */
	uint32_t crc32;			/* crc32 of all above fields */
} __attribute__ ((packed));

#define HDRFLAG_CRC32 0x00000001

enum bootrom_errors {
	ERR_CC_INIT      = 0x0100,
	ERR_UART_TIMEOUT = 0x0200,
	ERR_NAND_TIMEOUT = 0x0300,
	ERR_NAND_ECC     = 0x0301,
	ERR_NAND_IMAGE   = 0x0302,
	ERR_PRE_VERIFY   = 0x0400,
	ERR_PRE_CRC32    = 0x0401,
	ERR_LOAD_VERIFY  = 0x0500,
	ERR_LOAD_CRC32   = 0x0501,
	ERR_DIVBYZERO    = 0xfffe,
	ERR_EXCEPTION    = 0xffff,
};

#ifdef BOOTROM

#define __weak     __attribute__((weak))
#define __noinline __weak __attribute__((noinline))
#define __noreturn __attribute__((noreturn))

#define DECLARE_BOOTROM_ERROR \
	enum bootrom_errors bootrom_error __attribute__((section(".error")))

void loop(void) __noreturn __noinline;
void die(enum bootrom_errors error) __noreturn;
void nand_init(void);
void next_bootloader(void *addr);

#endif /* BOOTROM */

#endif
