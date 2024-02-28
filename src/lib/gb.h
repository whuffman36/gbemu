#ifndef GAMEBOY_H
#define GAMEBOY_H

#include "bus.h"
#include "cartridge.h"
#include "cpu.h"
#include "global.h"


typedef struct GameboyDef {
  Cpu cpu_;
  Cartridge* cartridge_;
  Bus* bus_;
  GlobalCtx* global_ctx_;
} Gameboy;


Result GameboyInit(Gameboy* const gb, const char* const romfile);

void GameboyDestroy(Gameboy* const gb);

void* GameboyRunCpu(void* const gb);

void* GameboyRunPpu(void* const gb);

#endif
