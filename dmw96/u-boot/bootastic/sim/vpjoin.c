#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

/* ecc codes for an erased vp */
static char *erased_ecc[] = {
	"0000", /* hamming */
	"2adfe9762d080e", /* bch4 */
	"7ac5126a6fa724b149bcb19e0e", /* bch8 */
	"e4f6ef4f69bc63067580163a0bef7d6147f5af0a", /* bch12 */
};

static void die(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);

	exit(EXIT_FAILURE);
}

static int get_oobsize(int vpfile_fd)
{
	struct stat stat;

	if (fstat(vpfile_fd, &stat) < 0)
		die("stat: %s\n", strerror(errno));

	/* 5 characters per byte, 512 byte per virtual page */
	return (stat.st_size / 5) - 512;
}

static int eat_vpfile(int vpfile_fd, char *data, char *oob)
{
	char buf[5];
	ssize_t ret;

	while (1) {
		ret = read(vpfile_fd, buf, 5);
		if (ret < 0)
			die("read: %s\n", strerror(errno));
		if (ret == 0)
			return 0;
		if (ret < 4)
			return -1;

		if (buf[3] == 'D') {
			*data++ = buf[0];
			*data++ = buf[1];
		} else {
			*oob++ = buf[0];
			*oob++ = buf[1];
		}
	}
}

static void
set_erased_ecc(char *oob, int oobsize, int vp_per_page, int ecc_mode)
{
	char *ecc = erased_ecc[ecc_mode];
	char *p = oob + 8; /* oob + n1 */
	int i;

	for (i = 0; i < vp_per_page; i++) {
		memcpy(p, ecc, strlen(ecc));
		p += oobsize;
	}
}

int main(int argc, char *argv[])
{
	int pagesize, oobsize, vp_per_page;
	int in_fd, out_fd;
	char *buf = NULL;
	char *data, *oob;
	int ecc_mode;
	int i;

	if (argc < 5)
		die("usage: vpjoin <pagesize> <ecc_mode> <outfile> <infiles...>\n");

	pagesize = strtoul(argv[1], NULL, 0);
	if (!pagesize || pagesize < 0 || pagesize % 512)
		die("invalid pagesize\n");

	vp_per_page = pagesize / 512;
	pagesize *= 2; /* in ascii */

	ecc_mode = strtoul(argv[2], NULL, 0) >> 2;
	if (ecc_mode < 0 || ecc_mode > 3)
		die("invalid ecc_mode\n");

	out_fd = creat(argv[3], 0666);
	if (out_fd < 0)
		die("creat: %s\n", strerror(errno));

	in_fd = open(argv[4], O_RDONLY);
	if (in_fd < 0)
		die("open: %s\n", strerror(errno));

	oobsize = get_oobsize(in_fd);
	if (oobsize != 16 && oobsize != 28)
		die("bad input file: invalid oobsize\n");

	oobsize *= 2; /* in ascii */

	close(in_fd);

	buf = malloc(pagesize + oobsize * vp_per_page);
	if (!buf)
		die("no mem\n");

	/* FF the whole physical page */
	memset(buf, 'F', pagesize + oobsize * vp_per_page);
	set_erased_ecc(buf + pagesize, oobsize, vp_per_page, ecc_mode);

	for (i = 0; i < argc - 4; i++) {
		int vp_in_page = i % vp_per_page;

		data = &buf[512 * 2 * vp_in_page];
		oob = &buf[pagesize + oobsize * vp_in_page];

		in_fd = open(argv[i + 4], O_RDONLY);
		if (in_fd < 0)
			die("open: %s\n", strerror(errno));

		if (get_oobsize(in_fd) * 2 != oobsize)
			die("bad input file: %s\n", argv[i + 3]);

		if (eat_vpfile(in_fd, data, oob) != 0)
			die("bad input file: %s\n", argv[i + 3]);

		/*
		 * if we have a full page write it out and set the whole
		 * buffer to FF again
		 */
		if (vp_in_page == vp_per_page - 1) {
			write(out_fd, buf, pagesize + oobsize * vp_per_page);
			write(out_fd, "\n", 1);

			memset(buf, 'F', pagesize + oobsize * vp_per_page);
			set_erased_ecc(buf + pagesize, oobsize, vp_per_page, ecc_mode);
		}

		close(in_fd);
	}

	/* if we dont have a remainder we can exit*/
	if (i % vp_per_page == 0)
		return EXIT_SUCCESS;

	i--;

#if 0
	/* move oob data closer to page data */
	memmove(data + 1024, buf + pagesize, oobsize * ((i % vp_per_page) + 1));

	write(out_fd, buf, (512 * 2 + oobsize) * ((i % vp_per_page) + 1));
#else
	/* write out the remaining page */
	write(out_fd, buf, pagesize + oobsize * vp_per_page);
	write(out_fd, "\n", 1);
#endif

	close(out_fd);

	free(buf);

	return EXIT_SUCCESS;
}
