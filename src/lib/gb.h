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


Result GameboyInit(Gameboy* gb, const char* const romfile);

void GameboyDestroy(Gameboy* gb);

void GameboyRun(Gameboy* gb);

#endif
