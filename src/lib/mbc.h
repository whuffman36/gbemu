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
  uint8_t seconds_;
  uint8_t minutes_;
  uint8_t hours_;
  uint8_t l_day_counter_;
  uint8_t h_day_counter_;
  uint8_t latch_;
} RealTimeClock;

typedef struct MemBankControllerDef {
  // Used only for MBC3.
  RealTimeClock rtc_;
  uint8_t rom_bank_;
  uint8_t ram_bank_;
  MBCType type_;
  RAMEnable ram_enable_;
  MemoryBankingMode banking_mode_;
} MemBankController;


static inline void MemBankControllerInit(MemBankController* mbc) {
  memset(mbc, 0, sizeof(MemBankController));
}

void LatchCurrentTimeIntoRTC(RealTimeClock* const rtc);

#endif
