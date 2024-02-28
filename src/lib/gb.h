#ifndef GAMEBOY_H
#define GAMEBOY_H

#include "bus.h"
#include "cartridge.h"
#include "cpu.h"
#include "global.h"


typedef struct GameboyDef {
  Cpu cpu;
  Cartridge* cartridge;
  Bus* bus;
  GlobalCtx* global_ctx;
} Gameboy;


Result GameboyInit(Gameboy* const gb, const char* const romfile);

void GameboyDestroy(Gameboy* const gb);

void* GameboyRunCpu(void* const gb);

void* GameboyRunPpu(void* const gb);

#endif
