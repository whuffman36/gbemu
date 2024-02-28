#include "mbc.h"

#include "time.h"


void LatchCurrentTimeIntoRTC(RealTimeClock* const rtc) {
  time_t raw_time;
  time(&raw_time);
  struct tm* time_info = localtime(&raw_time);
  rtc->seconds = time_info->tm_sec;
  rtc->minutes = time_info->tm_min;
  rtc->hours = time_info->tm_hour;
}
