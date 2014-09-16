#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/i2c.h>
#include "include/video/dmw96osdm_common.h"
#include "include/video/dwfb_common.h"

static char *on = "on";
static char *off = "off";
static unsigned char buf[10];

int main(int argc, char **argv)
{
	int i, fd, hdmi_fd, dummy, val, sum, sink = 0, mode = 0, state;

	if (argc < 2) {
		fprintf(stderr, "usage: %s 480p60[a]|720p50[a]|720p60[a]|1080p24[a]|1080p25[a]|1080p30[a]|off\n" \
		                "       'a', e.g. 480p60a - preserve Android resolution\n" \
		                "or:    %s poll - to check if sink is connected\n", argv[0], argv[0]);
		exit(1);
	}

	if (!strcmp(argv[1], "480p60a"))
		mode = 4;
	else if (!strcmp(argv[1], "720p50a"))
		mode = 5;
	else if (!strcmp(argv[1], "720p60a"))
		mode = 6;
	else if (!strcmp(argv[1], "1080p24a"))
		mode = 7;
	else if (!strcmp(argv[1], "1080p25a"))
		mode = 8;
	else if (!strcmp(argv[1], "1080p30a"))
		mode = 9;
	else if (!strcmp(argv[1], "480p60"))
		mode = 10;
	else if (!strcmp(argv[1], "720p50"))
		mode = 11;
	else if (!strcmp(argv[1], "720p60"))
		mode = 12;
	else if (!strcmp(argv[1], "1080p24"))
		mode = 13;
	else if (!strcmp(argv[1], "1080p25"))
		mode = 14;
	else if (!strcmp(argv[1], "1080p30"))
		mode = 15;
	else if (!strcmp(argv[1], "off"))
		mode = 1;
	else if (!strcmp(argv[1], "poll"))
		mode = 0;
	else {
		fprintf(stderr, "invalid mode. Currently supported: " \
		                "480p60[a],720p50[a],720p60[a],1080p24[a],1080p25[a],1080p30[a],off\n");
		exit(1);
	}

	hdmi_fd = open("/sys/class/switch/hdmi/mode", O_RDWR);
	if (hdmi_fd < 0) {
		printf("Error: HDMI output cannot be powered up\n");
		exit(1);
	}

	read(hdmi_fd, buf, 10);
	state = (buf[1] == 'n'); /* on or off */

	if (mode > 0) {
		fd = open("/dev/graphics/fb0", O_RDWR);
		if (fd < 0) {
			fprintf(stderr, "cannot open framebuffer device\n");
			exit(1);
		}

		if (ioctl(fd, 0x4004570a /*FBIOSET_DISPLAY_MODE*/, &mode) < 0) {
			fprintf(stderr, "ioctl failed\n");
			exit(1);
		}

		close(fd);

		fd = open("/dev/graphics/main_osd", O_RDWR);
		if (fd < 0) {
			fprintf(stderr, "cannot open main_osd device\n");
			exit(1);
		}

		if (ioctl(fd, 0x4004571b /*FBIOSET_OSDM_DISPLAY_CONFIG*/, &dummy) < 0) {
			fprintf(stderr, "ioctl failed\n");
			exit(1);
		}

		close(fd);
	} else {
		if (!state)
			write(hdmi_fd, on, sizeof(on));

		fd = open("/sys/class/switch/hdmi/state", O_RDONLY);
		if (fd < 0) {
			printf("No sink connected\n");
			exit(1);
		}
		read(fd, buf, 10);
		close(fd);

		if (!state)
			write(hdmi_fd, off, sizeof(off));

		if (buf[0] == '1') {
			printf("Sink connected\n");
			exit(0);
		} else {
			printf("No sink connected\n");
			exit(1);
		}
	}

	if (mode == 1)
		write(hdmi_fd, off, sizeof(off));
	else
		write(hdmi_fd, on, sizeof(on));

	close(hdmi_fd);

	printf("HDMI output active\n");

	return 0;
}
