#ifndef GAMEBOY_H
#define GAMEBOY_H

#include "bus.h"
#include "cartridge.h"
#include "cpu.h"
#include "global.h"

#include <SDL2/SDL.h>


typedef struct GameboyDef {
  Cpu cpu;
  Cartridge* cartridge;
  Bus* bus;
  SDL_Window* screen;
  GlobalCtx* global_ctx;
} Gameboy;


Result GameboyInit(Gameboy* const gb, const char* const romfile);

void GameboyDestroy(Gameboy* const gb);

Result GameboyRun(Gameboy* const gb);

#endif
