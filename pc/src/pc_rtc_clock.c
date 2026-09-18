#include "pc_rtc_clock.h"

int64_t pc_rtc_host_ticks(int64_t unix_seconds,
                          int64_t utc_offset_seconds,
                          int hour_override,
                          int minute_override,
                          int second_override,
                          int local_hour,
                          int local_minute,
                          int local_second) {
    int64_t local_seconds = unix_seconds - PC_RTC_UNIX_EPOCH_DIFF + utc_offset_seconds;
    if (hour_override >= 0) {
        local_seconds += (int64_t)(hour_override - local_hour) * 60 * 60;
        if (minute_override >= 0) {
            local_seconds += (int64_t)(minute_override - local_minute) * 60;
        }
        if (second_override >= 0) {
            local_seconds += second_override - local_second;
        }
    }
    return local_seconds * PC_RTC_TIMER_CLOCK;
}

int64_t pc_rtc_counter_ticks(uint64_t counter, uint64_t frequency) {
    if (frequency == 0) {
        return 0;
    }
    return (int64_t)(counter / frequency) * PC_RTC_TIMER_CLOCK +
           (int64_t)((counter % frequency) * PC_RTC_TIMER_CLOCK / frequency);
}

int64_t pc_rtc_subsecond_ticks(int64_t nanoseconds) {
    return nanoseconds * PC_RTC_TIMER_CLOCK / 1000000000LL;
}
