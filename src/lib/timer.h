#ifndef TIMER_H
#define TIMER_H

#include "global.h"

#include <stdint.h>
#include <string.h>


typedef struct TimerDef {
  uint16_t div;
  uint8_t tima;
  uint8_t tma;
  uint8_t tac;
} Timer;


static inline void TimerInit(Timer* const timer) {
  memset(timer, 0, sizeof(Timer));
}

void TimerTick(Timer* const timer, uint8_t* const interrupts_flag);

#endif
