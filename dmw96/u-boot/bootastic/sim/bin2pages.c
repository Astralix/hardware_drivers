#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

static char oob[28] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
};

static void die(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);

	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
	int pagesize, oobsize;
	int in_fd, out_fd;
	char *buf;
	ssize_t to_write, written;
	unsigned long cnt;
	
	if (argc != 4)
		die("usage: bin2pages <pagesize> <oobsize> <infile>\n");

	pagesize = strtoul(argv[1], NULL, 0);
	if (!pagesize || pagesize < 0 || pagesize % 512)
		die("invalid pagesize\n");

	oobsize = strtoul(argv[2], NULL, 0);
	if (!oobsize || oobsize < 0)
		die("invalid oobsize\n");

	oobsize /= pagesize / 512;
	if (oobsize != 16 && oobsize != 28)
		die("invalid oobsize\n");

	buf = malloc(512 + oobsize);
	if (!buf)
		die("buf: no memory\n");

	out_fd = creat("bin2pages.out", 0644);
	if (out_fd < 0)
		die("creat: %s\n", strerror(errno));

	in_fd = open(argv[3], O_RDONLY);
	if (in_fd < 0)
		die("open: %s\n", strerror(errno));

	cnt = 0;
	do {
		to_write = read(in_fd, buf, 512);
		if (to_write < 0)
			die("read: %s\n", strerror(errno));

		if (to_write == 0)
			break;

		if (to_write < 512)
			memset(&buf[to_write], 0xff, 512 - to_write);

		memcpy(&buf[512], oob, oobsize);

		/* dirty marker */
		if (cnt % pagesize == 0)
			buf[512 + 2] = 0x00;

		written = write(out_fd, buf, 512 + oobsize);
		cnt += written - oobsize;
	} while (to_write == 512 && written == 512 + oobsize);

	return EXIT_SUCCESS;
}
