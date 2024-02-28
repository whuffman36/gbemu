#ifndef MBC_H
#define MBC_H

#include "stdint.h"
#include "string.h"

#ifdef memset
  #undef memset
#endif

typedef enum MBCTypeDef {
  MBC_NONE = 0,
  MBC_1 = 1,
  MBC_2 = 2,
  MBC_3 = 3,
  MBC_5 = 5,
} MBCType;

typedef enum RAMEnableDef {
  RAM_DISABLED = 0,
  RAM_ENABLED = 1,
} RAMEnable;

typedef enum MemoryBankingModeDef {
  ROM_BANKING = 0,
  RAM_BANKING = 1,
  RTC_BANKING = 2,
} MemoryBankingMode;

typedef struct RealTimeClockDef {
  uint8_t seconds;
  uint8_t minutes;
  uint8_t hours;
  uint8_t l_day_counter;
  uint8_t h_day_counter;
  uint8_t latch;
} RealTimeClock;

typedef struct MemBankControllerDef {
  // Used only for MBC3.
  RealTimeClock rtc;
  uint8_t rom_bank;
  uint8_t ram_bank;
  MBCType type;
  RAMEnable ram_enable;
  MemoryBankingMode banking_mode;
} MemBankController;


static inline void MemBankControllerInit(MemBankController* mbc) {
  memset(mbc, 0, sizeof(MemBankController));
}

void LatchCurrentTimeIntoRTC(RealTimeClock* const rtc);

#endif
