#include "gb.h"

#include "bus.h"
#include "cartridge.h"
#include "cpu.h"
#include "global.h"


Result GameboyInit(Gameboy* const gb, const char* const romfile) {
  gb->cartridge = CartridgeCreate(gb->global_ctx, romfile);
  if (gb->cartridge == NULL) {
    return RESULT_NOTOK;
  }
  gb->bus = BusCreate(gb->global_ctx, gb->cartridge);
  if (gb->bus == NULL) {
    return RESULT_NOTOK;
  }
  CpuInit(&gb->cpu);
  gb->cpu.global_ctx = gb->global_ctx;
  gb->cpu.bus = gb->bus;
  // TODO: Run boot ROM...
  return RESULT_OK;
}


void GameboyDestroy(Gameboy* const gb) {
  if (gb == NULL) {
    return;
  }
  BusDestroy(gb->bus);
  CartridgeDestroy(gb->cartridge);
  gb->global_ctx = NULL;
}


void* GameboyRunCpu(void* const gb_arg) {
  Gameboy* const gb = (Gameboy* const)gb_arg;

  while(gb->global_ctx->error == NO_ERROR &&
        gb->global_ctx->status != STATUS_STOP) {
    CpuStep(&gb->cpu);
  }
  pthread_exit(NULL);
}


void* GameboyRunPpu(void* const gb_arg) {
  Gameboy* const gb = (Gameboy* const)gb_arg;

  pthread_exit(NULL);
}
