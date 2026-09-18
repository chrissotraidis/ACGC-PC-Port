#ifndef PC_RTC_CLOCK_H
#define PC_RTC_CLOCK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PC_RTC_TIMER_CLOCK 40500000LL
#define PC_RTC_UNIX_EPOCH_DIFF 946684800LL

int64_t pc_rtc_host_ticks(int64_t unix_seconds,
                          int64_t utc_offset_seconds,
                          int hour_override,
                          int minute_override,
                          int second_override,
                          int local_hour,
                          int local_minute,
                          int local_second);
int64_t pc_rtc_counter_ticks(uint64_t counter, uint64_t frequency);
int64_t pc_rtc_subsecond_ticks(int64_t nanoseconds);

#ifdef __cplusplus
}
#endif

#endif
