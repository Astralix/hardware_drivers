#include <stdio.h>
#include <stdlib.h>
#include <linux/rtc.h>
#ifdef ANDROID
#include <rtc-dp52.h>
#include <linux/android_alarm.h>
#else
#include <linux/rtc-dp52.h>
#endif
#include <sys/ioctl.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

#ifdef ANDROID
#include <linux/android_alarm.h>
#endif


/*
 * format of the rtc-setup file:
 * 	line 1: <long>   // reference tick
 * 	line 2: <long>   // seconds since epoch
 */

static char setup_default[] = "/etc/rtc-setup";
static char rtc_default[] = "/dev/rtc0";

static char *cmd;
static char *setup = setup_default;
static char *rtc = rtc_default;
static int daemonize = 0;
static unsigned long interval = 3600;

void usage()
{
	fprintf(stderr,
	        "%s [-h] [-d] [-i <interval>] [-r <rtc-device>] [-s <setup>]:\n"
	        "    Periodically synchronize RTC with system clock. Uses\n"
	        "    %s and %s as defaults for device and\n"
	        "    setup file.\n"
	        "    Options:\n"
		"        -h          help; shows this text\n"
	        "        -d          daemonize; forks into background\n"
		"        -i          update interval in seconds (def: 3600, min: 1)\n"
	        "        -r <device> use this rtc device\n"
	        "        -s <setup>  use this file as setup file\n",
	        cmd, rtc_default, setup_default);

	exit(EXIT_FAILURE);
}

void parse_args(int argc, char *argv[])
{
	char *options = "hdi:r:s:";
	int optchar;

	cmd = argv[0];

	while ((optchar = getopt(argc, argv, options)) != -1) {
		switch (optchar) {
		case 'd':
			daemonize = 1;
			break;
		case 'i':
			interval = atol(optarg);
			if (interval == 0)
				usage();
			break;
		case 'r':
			rtc = strdup(optarg);
			break;
		case 's':
			setup = strdup(optarg);
			break;			
		case 'h':
		default:
			usage();
		}
	}
}

void to_rtc_time(time_t sec, struct rtc_time *rtc_tm)
{
	struct tm tm;

	gmtime_r(&sec, &tm);

	memset(rtc_tm, 0, sizeof(*rtc_tm));
	rtc_tm->tm_sec  = tm.tm_sec;
	rtc_tm->tm_min  = tm.tm_min;
	rtc_tm->tm_hour = tm.tm_hour;
	rtc_tm->tm_mday = tm.tm_mday;
	rtc_tm->tm_mon  = tm.tm_mon;
	rtc_tm->tm_year = tm.tm_year;
}

time_t to_sec(struct rtc_time *rtc_tm)
{
	struct tm tm;

	memset(&tm, 0, sizeof(tm));
	tm.tm_sec  = rtc_tm->tm_sec;
	tm.tm_min  = rtc_tm->tm_min;
	tm.tm_hour = rtc_tm->tm_hour;
	tm.tm_mday = rtc_tm->tm_mday;
	tm.tm_mon  = rtc_tm->tm_mon;
	tm.tm_year = rtc_tm->tm_year;

	return mktime(&tm);
}

int reftime_save(struct rtc_dp52_reftime *reftime)
{
	int fd;
	ssize_t size;
	char buf[1024];
	int printed;

	fd = open(setup, O_WRONLY | O_CREAT | O_TRUNC | O_SYNC, 00666);
	if (fd < 0)
		return -1;

	printed = snprintf(buf, sizeof(buf), "%lu\n%lu\n", reftime->reference,
	                   to_sec(&reftime->rtc_tm));
	
	size = write(fd, buf, printed);
	if (size != printed)
		return -1;

	close(fd);
	return 0;
}

int reftime_load(struct rtc_dp52_reftime *reftime)
{
	int fd;
	ssize_t size;
	char buf[1024], *p;

	fd = open(setup, O_RDONLY);
	if (fd < 0)
		return -1;

	memset(buf, '0', sizeof(buf));
	size = read(fd, buf, sizeof(buf));
	if (size < 0 || size >= 1024)
		return -1;

	close(fd);

	/* find beginning of line 2 */
	p = strchr(buf, '\n');
	if (!p)
		return -1;
	p++;

	/* fill in reftime struct with values from file */
	reftime->reference = atol(buf);
	to_rtc_time(atol(p), &reftime->rtc_tm);

	return 0;
}

void update_rtc()
{
	struct rtc_dp52_reftime reftime;
	struct tm tm;
	time_t curtime;
	int ret, fd;
#ifdef ANDROID
        struct timespec ts;
	int fd_alarm;
#endif

	/* update rtc reftime from file once */
	if ((fd = open(rtc, O_RDONLY)) < 0) {
		perror(rtc);
		exit(errno);
	}
	ret = reftime_load(&reftime);
	if (ret == 0)
		ioctl(fd, RTC_DP52_REFTIME_SET, &reftime);
	
	/* Update the system time */

	// Read current hw clock
	ioctl(fd, RTC_RD_TIME, &tm);

	// Set the system time
	curtime = mktime(&tm);	
#ifdef ANDROID
	if(curtime != -1) {
	    fd_alarm = open("/dev/alarm", O_RDWR);
	    ts.tv_sec = curtime;
	    ts.tv_nsec = 0;
	    ioctl(fd_alarm, ANDROID_ALARM_SET_RTC, &ts);
	    close(fd_alarm);
	}
#else
	stime(&curtime);
#endif
	close(fd);


	while (1) {
		/* sync rtc with current system time */
		curtime = time(NULL);
		to_rtc_time(curtime, &reftime.rtc_tm);

		if ((fd = open(rtc, O_RDONLY)) < 0) {
			perror(rtc);
			exit(errno);
		}

		ret = ioctl(fd, RTC_SET_TIME, &reftime.rtc_tm);
		if (ret < 0) {
			perror(rtc);
			exit(errno);
		}

		/* save reftime from rtc to file */
		ret = ioctl(fd, RTC_DP52_REFTIME_GET, &reftime);
		if (ret < 0) {
			perror(rtc);
			exit(errno);
		}

		close(fd);

		reftime_save(&reftime);

		sleep(interval);
	}
}

int main(int argc, char *argv[])
{
	pid_t pid;
	int fd, ret;
	
	parse_args(argc, argv);

	if (daemonize) {
		pid = fork();
		if (pid != 0)
			exit(EXIT_SUCCESS);
		if (pid == -1)
			exit(EXIT_FAILURE);

		/* we're now in the forked process */
		pid = setsid();
	}


	while (1) {
		update_rtc();

		sleep(30);
	}

	return 0;
}
