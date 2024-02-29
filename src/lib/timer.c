#include "timer.h"

#include "global.h"


void TimerTick(Timer* const timer, uint8_t* const interrupts_flag) {
  uint16_t prev_div = timer->div++;
  int timer_update = 0;

  switch(timer->tac & 0x03) {
    case 0x00:
      // Divide the clock by 1024, update every time the 10th bit is reset.
      timer_update = (prev_div & (1 << 9)) && !(timer->div & (1 << 9));
      break;
    case 0x01:
      // Divide the clock by 16, update every time the 4th bit is reset.
      timer_update = (prev_div & (1 << 3)) && !(timer->div & (1 << 3));
      break;
    case 0x02:
      // Divide the clock by 64, update every time the 6th bit is reset.
      timer_update = (prev_div & (1 << 5)) && !(timer->div & (1 << 5));
      break;
    case 0x03:
      // Divide the clock by 256, update every time the 8th bit is reset.
      timer_update = (prev_div & (1 << 7)) && !(timer->div & (1 << 7));
      break;
  }

  if (timer_update && (timer->tac & (1 << 2))) {
    uint8_t prev_tima = timer->tima++;

    if (prev_tima == 0xFF) {
      timer->tima = timer->tma;
      RequestInterrupt(INTERRUPT_TIMER, interrupts_flag);
    }
  }
}
