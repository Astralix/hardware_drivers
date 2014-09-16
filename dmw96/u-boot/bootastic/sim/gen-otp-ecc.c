#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

int ecc(const uint32_t *words, size_t count)
{
	int bit, ecc;
	uint32_t word, i;

	bit = 0xff00;
	ecc = 0;
	while (count--) {
		word = *words++;
		for (i = 0; i < 32; i++) {
			if (word & 0x80000000) {
				ecc ^= bit;
			}
			word = word * 2;
			bit += 0xff01;
		}
	}
	return ecc & 0xffff;
}

static int ascii2hex(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';

	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;

	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;

	return -1;
}

static int ascii2byte(char *str)
{
	int ret;

	ret  = ascii2hex(*str++) << 4;
	ret |= ascii2hex(*str);

	return ret;
}

union {
	char b[32];
	uint32_t l[8];
} buf;

int main(int argc, char *argv[])
{
	int fd, i;
	char *input, *p;
	uint16_t the_ecc;

	if (argc != 2) {
		fprintf(stderr, "need file as argument\n");
		return EXIT_FAILURE;
	}

	fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		perror("open");
		return EXIT_FAILURE;
	}

	input = mmap(NULL, 64, PROT_READ, MAP_PRIVATE, fd, 0);
	if (input == NULL) {
		perror("mmap");
		return EXIT_FAILURE;
	}

	p = input;

	for (i = 0; i < 32; i++) {
		buf.b[i] = ascii2byte(p);
		p += 2;
	}

	the_ecc = ecc(buf.l, sizeof(buf) / sizeof(uint32_t));

	printf("0x%08x\n", buf.l[1]);
	printf("%04x\n", the_ecc);

	return EXIT_SUCCESS;
}
