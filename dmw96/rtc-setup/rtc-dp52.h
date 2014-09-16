#ifndef __LINUX_RTC_DP52_H
#define __LINUX_RTC_DP52_H

#include <linux/rtc.h>

struct rtc_dp52_reftime {
	struct rtc_time rtc_tm;
	unsigned long reference;
};

#define RTC_DP52_REFTIME_SET _IOW('p', 0xf0, struct rtc_dp52_reftime)
#define RTC_DP52_REFTIME_GET _IOR('p', 0xf1, struct rtc_dp52_reftime)

#endif /* __LINUX_RTC_DP52_H */
