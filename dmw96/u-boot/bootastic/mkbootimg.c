/*
 * (c) 2010 DSPG Technologies GmbH, Dirk Hoerner <dirk.hoerner@dspg.com>
 *
 * Boot Image Builder
 *
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>

#include "bootrom.h"

/*
 * CRC calculation from the Linux kernel, lib/crc32.c:
 */
#include "crc32table.h"
#define DO_CRC(x) crc = tab[ (crc ^ (x)) & 255 ] ^ (crc>>8)

/**
 * crc32_le() - Calculate bitwise little-endian Ethernet AUTODIN II CRC32
 * @crc: seed value for computation.  ~0 for Ethernet, sometimes 0 for
 *	other uses, or the previous crc32 value if computing incrementally.
 * @p: pointer to buffer over which CRC is run
 * @len: length of buffer @p
 */
static uint32_t crc32_le(uint32_t crc, uint32_t const *b, uint32_t len)
{
	const uint8_t *p = (uint8_t *)b;
	const unsigned long *tab = crc32table_le;

	/* Align it */
	if(((long)b)&3 && len){
		do {
			uint8_t *p = (uint8_t *)b;
			DO_CRC(*p++);
			b = (void *)p;
		} while ((--len) && ((long)b)&3 );
	}
	if(len >= 4){
		/* load data 32 bits wide, xor data 32 bits wide. */
		uint32_t save_len = len & 3;
	        len = len >> 2;
		--b; /* use pre increment below(*++b) for speed */
		do {
			crc ^= *++b;
			DO_CRC(0);
			DO_CRC(0);
			DO_CRC(0);
			DO_CRC(0);
		} while (--len);
		b++; /* point to next byte(s) */
		len = save_len;
	}
	/* And the last few bytes */
	if(len){
		do {
			uint8_t *p = (uint8_t *)b;
			DO_CRC(*p++);
			b = (void *)p;
		} while (--len);
	}

	return crc;
}

/* standard crc32 checksum */
static unsigned int calculate_crc32(void *buffer, unsigned int size)
{
	return crc32_le(0, (uint32_t const *)buffer, size);
}

/*
 * mkbootimg implementation
 */
#define align4(a) (((a) + 3) & ~3)

char *cmd;
char *fname_key;
char *fname_pre;
char *fname_pre_vrl;
char *fname_load;
char *fname_load_vrl;
char *fname_out = "nand.image";
uint32_t load_addr = 0x41000000;
uint32_t flags;

enum options {
	OPTION_KEY,
	OPTION_PRELOADER,
	OPTION_PRE_VRL,
	OPTION_LOADER,
	OPTION_LOAD_ADDR,
	OPTION_LOAD_VRL,
	OPTION_CRC32,
	OPTION_OUTPUT,
};

struct option long_opts[] = {
	{ "help",          0, 0, 'h' },
	{ "key",           1, 0, OPTION_KEY },
	{ "preloader",     1, 0, OPTION_PRELOADER },
	{ "preloader-vrl", 1, 0, OPTION_PRE_VRL },
	{ "loader",        1, 0, OPTION_LOADER },
	{ "loader-addr",   1, 0, OPTION_LOAD_ADDR },
	{ "loader-vrl",    1, 0, OPTION_LOAD_VRL },
	{ "crc32",         0, 0, OPTION_CRC32 },
	{ "output",        1, 0, OPTION_OUTPUT },
	{ NULL,            0, 0, 0 },
};

static void die(const char *fmt, ...)
{
	va_list params;

	va_start(params, fmt);
	vfprintf(stderr, fmt, params);
	va_end(params);
	exit(EXIT_FAILURE);
}

static void die_file(const char *fname)
{
	die("'%s': %s\n", fname, strerror(errno));
}

static void usage(void)
{
	fprintf(stderr, "Usage: %s <opts>\n"
			"  -h, --help\n"
			"      --key=<file>\n"
			"      --preloader=<file>\n"
			"      --preloader-vrl=<file>\n"
			"      --loader=<file>\n"
			"      --loader-addr=<addr>\n"
			"      --loader-vrl=<file>\n"
			"      --crc32\n"
			"      --output=<file>\n",
	        cmd);
	exit(EXIT_FAILURE);
}

static void parse_cmdline(int argc, char *argv[])
{
	int ret;
	int optnum;
	cmd = argv[0];

	while (1) {
		ret = getopt_long(argc, argv, "h", long_opts, &optnum);
		if (ret < 0)
			break;

		/*
		printf("%s(): got param '%s' with argument '%s'\n", __func__,
		       long_opts[optnum].name, optarg);
		      */

		switch (ret) {
		case OPTION_KEY:
			fname_key = strdup(optarg);
			break;
		case OPTION_PRELOADER:
			fname_pre = strdup(optarg);
			break;
		case OPTION_PRE_VRL:
			fname_pre_vrl = strdup(optarg);
			break;
		case OPTION_LOADER:
			fname_load = strdup(optarg);
			break;
		case OPTION_LOAD_ADDR:
			load_addr = strtoul(optarg, NULL, 0);
			break;
		case OPTION_LOAD_VRL:
			fname_load = strdup(optarg);
			break;
		case OPTION_CRC32:
			flags |= HDRFLAG_CRC32;
			break;
		case OPTION_OUTPUT:
			fname_out = strdup(optarg);
			break;
		case 'h':
		default:
			usage();
		}
	}

	if (!fname_pre)
		die("need preloader\n");

	if ((fname_pre_vrl || fname_load_vrl) && !fname_key)
		die("cannot specify vrl's without key\n");

	if (fname_load_vrl && !fname_load) {
		fprintf(stderr, "cannot specify loader vrl without loader\n\n");
		usage();
	}
}

off_t get_filesize(int fd)
{
	struct stat s;
	int ret;

	ret = fstat(fd, &s);
	if (ret < 0)
		die("%s(): stat: %s\n", __func__, strerror(errno));

	return s.st_size;
}

int copy_file(int fd_in, int fd_out)
{
	char buf[4096];
	int ret;

	do {
		ret = read(fd_in, buf, sizeof(buf));
		if (ret < 0)
			goto out;

		ret = write(fd_out, buf, ret);
	} while (ret > 0);

out:
	if (ret < 0)
		die("%s: %s\n", __func__, strerror(errno));
}

void file_align4(int fd)
{
	off_t pos = lseek(fd, 0, SEEK_CUR);
	int pad = pos % 4;
	char buf[3] = { 0xff, 0xff, 0xff };
	int ret = 0;

	if (pad)
		ret = write(fd, buf, pad);

	if (ret < 0)
		die("%s: error while padding\n", __func__);
}

static struct image_header h;

void cleanup(int status, void *arg)
{
	if (status == EXIT_SUCCESS)
		return;

	/* delete output file */
	unlink(fname_out);
}

int main(int argc, char *argv[])
{
	int i, c;
	int fd_out, fd_in;
	unsigned int totalsize;
	int ret;
	char *image, *p;

	parse_cmdline(argc, argv);

	on_exit(cleanup, NULL);

	fd_out = open(fname_out, O_CREAT|O_TRUNC|O_RDWR, 0666);
	if (fd_out < 0)
		die("'%s': %s\n", strerror(errno));

	/* header is written last */
	ret = lseek(fd_out, sizeof(h), SEEK_SET);
	if (ret < 0)
		die_file(fname_out);

	/*
	 * write contents from external files
	 */

	if (fname_key) {
		fd_in = open(fname_key, O_RDONLY);
		if (fd_in < 0)
			die_file(fname_key);

		h.key_offset = lseek(fd_out, 0, SEEK_CUR);
		h.key_size = get_filesize(fd_in);

		copy_file(fd_in, fd_out);
		close(fd_in);
	}

	if (fname_pre_vrl) {
		fd_in = open(fname_pre_vrl, O_RDONLY);
		if (fd_in < 0)
			die_file(fname_pre_vrl);

		h.pre_vrl_offset = lseek(fd_out, 0, SEEK_CUR);

		copy_file(fd_in, fd_out);
		close(fd_in);
	}

	file_align4(fd_out);

	if (fname_pre) {
		fd_in = open(fname_pre, O_RDONLY);
		if (fd_in < 0)
			die_file(fname_pre);

		h.pre_offset = lseek(fd_out, 0, SEEK_CUR);
		h.pre_size = get_filesize(fd_in);

		if (fname_pre_vrl && (h.pre_size % 64))
			die("'%s': needs to be aligned to 64 byte boundary\n",
			    fname_pre);

		h.pre_size = align4(h.pre_size);

		copy_file(fd_in, fd_out);
		close(fd_in);
	}

	file_align4(fd_out);

	if (fname_load_vrl) {
		fd_in = open(fname_load_vrl, O_RDONLY);
		if (fd_in < 0)
			die_file(fname_load_vrl);

		h.load_vrl_offset = lseek(fd_out, 0, SEEK_CUR);

		copy_file(fd_in, fd_out);
		close(fd_in);
	}

	file_align4(fd_out);

	if (fname_load) {
		fd_in = open(fname_load, O_RDONLY);
		if (fd_in < 0)
			die_file(fname_load);

		h.load_offset = lseek(fd_out, 0, SEEK_CUR);
		h.load_size = get_filesize(fd_in);
		h.load_addr = load_addr;

		if (fname_load_vrl && (h.load_size % 64))
			die("'%s': needs to be aligned to 64 byte boundary\n",
			    fname_load);

		h.load_size = align4(h.load_size);

		copy_file(fd_in, fd_out);
		close(fd_in);
	}

	file_align4(fd_out);

	/*
	 * fill in remaining header fields
	 */

	h.signature[0] = 'D';
	h.signature[1] = 'S';
	h.signature[2] = 'P';
	h.signature[3] = 'G';
	h.flags = flags;

	ret = lseek(fd_out, 0, SEEK_SET);
	if (ret < 0)
		die_file(fname_out);

	if (h.flags & HDRFLAG_CRC32) {
		image = mmap(NULL, 10 << 20, PROT_READ, MAP_PRIVATE, fd_out, 0);
		if (image == MAP_FAILED)
			die_file(fname_out);

		if (fname_pre) {
			p = image + h.pre_offset;
			h.pre_crc32 = calculate_crc32(p, h.pre_size);
		}

		if (fname_load) {
			p = image + h.load_offset;
			h.load_crc32 = calculate_crc32(p, h.load_size);
		}
	}

	h.crc32 = calculate_crc32(&h, sizeof(h) - sizeof(h.crc32));

	/*
	 * write the header
	 */

	ret = write(fd_out, &h, sizeof(h));
	if (ret < 0)
		die_file(fname_out);
	if (ret < sizeof(h))
		die("'%s': could not write full header\n", fname_out);

	return 0;
}
