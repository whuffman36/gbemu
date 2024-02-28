#include "gb.h"

#include "bus.h"
#include "cartridge.h"
#include "cpu.h"
#include "global.h"

#include <stdio.h>


Result GameboyInit(Gameboy* const gb, const char* const romfile) {
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


void GameboyDestroy(Gameboy* const gb) {
  if (gb == NULL) {
    return;
  }
  BusDestroy(gb->bus_);
  CartridgeDestroy(gb->cartridge_);
  gb->global_ctx_ = NULL;
}


void* GameboyRunCpu(void* const gb_arg) {
  Gameboy* const gb = (Gameboy* const)gb_arg;
  //char input;

  while(gb->global_ctx_->error == NO_ERROR &&
        gb->global_ctx_->status != STATUS_STOP) {
    CpuStep(&gb->cpu_);
  }
  pthread_exit(NULL);
}


void* GameboyRunPpu(void* const gb_arg) {
  Gameboy* const gb = (Gameboy* const)gb_arg;

  pthread_exit(NULL);
}
