#include "gb.h"

#include "bus.h"
#include "cartridge.h"
#include "cpu.h"
#include "global.h"

#include "stdio.h"

Result GameboyInit(Gameboy *gb, const char* const romfile) {
  gb->cartridge_ = CartridgeCreate(gb->global_ctx_, romfile);
  if (gb->cartridge_ == NULL) {
    return RESULT_NOTOK;
  }
  gb->bus_ = BusCreate(gb->global_ctx_, gb->cartridge_);
  if (gb->bus_ == NULL) {
    return RESULT_NOTOK;
  }
  CpuInit(&gb->cpu_);
  gb->cpu_.global_ctx_ = gb->global_ctx_;
  gb->cpu_.bus_ = gb->bus_;
  // TODO: Run boot ROM...
  return RESULT_OK;
}


void GameboyDestroy(Gameboy *gb) {
  if (gb == NULL) {
    return;
  }
  BusDestroy(gb->bus_);
  CartridgeDestroy(gb->cartridge_);
  gb->global_ctx_ = NULL;
}


void GameboyRun(Gameboy *gb) {
  char input;

  while(1) {
    scanf("%c", &input);
  
    if (input == 'q') {
      break;
    }

    CpuStep(&gb->cpu_);

    if (gb->global_ctx_->status != STATUS_RUNNING) {
      break;
    }
    if (gb->global_ctx_->error != NO_ERROR) {
      break;
    }
  }
}
