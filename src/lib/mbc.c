#include "mbc.h"

#include "time.h"


void LatchCurrentTimeIntoRTC(RealTimeClock* const rtc) {
  time_t raw_time;
  time(&raw_time);
  struct tm* time_info = localtime(&raw_time);
  rtc->seconds_ = time_info->tm_sec;
  rtc->minutes_ = time_info->tm_min;
  rtc->hours_ = time_info->tm_hour;
}
