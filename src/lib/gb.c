#include "gb.h"

#include "bus.h"
#include "cartridge.h"
#include "cpu.h"
#include "global.h"

#include <stdlib.h>

#include <pthread.h>
#include <SDL2/SDL.h>


Result GameboyInit(Gameboy* const gb, const char* const romfile) {
  *gb->global_ctx = (GlobalCtx){
    .mode = GB_MODE_GBC,
    .error = NO_ERROR,
    .status = STATUS_RUNNING,
    .clock = 0
  };
  pthread_mutex_init(&gb->global_ctx->interrupt_mtx, NULL);
  pthread_cond_init(&gb->global_ctx->interrupt_write, NULL);

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    gb->global_ctx->error = SDL_VIDEO_INIT_ERROR;
    return RESULT_NOTOK;
  }

  gb->screen = SDL_CreateWindow(
    /*title=*/"gbemu",
    /*x=*/0,
    /*y=*/0,
    /*width=*/640,
    /*height=*/480,
    /*flags=*/SDL_WINDOW_SHOWN
  );
  if (gb->screen == NULL) {
    gb->global_ctx->error = SDL_WINDOW_CREATION_FAILED;
    return RESULT_NOTOK;
  }

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
  SDL_DestroyWindow(gb->screen);
  SDL_Quit();
  BusDestroy(gb->bus);
  CartridgeDestroy(gb->cartridge);
  pthread_mutex_destroy(&gb->global_ctx->interrupt_mtx);
  pthread_cond_destroy(&gb->global_ctx->interrupt_write);
  gb->global_ctx = NULL;
}


static void* GameboyRunCpu(void* const gb_arg) {
  Gameboy* const gb = (Gameboy* const)gb_arg;

  while(gb->global_ctx->error == NO_ERROR &&
        gb->global_ctx->status != STATUS_STOP) {
    CpuStep(&gb->cpu);
  }
  pthread_exit(NULL);
}


static void* GameboyRunPpu(void* const gb_arg) {
  Gameboy* const gb = (Gameboy* const)gb_arg;

  pthread_exit(NULL);
}


Result GameboyRun(Gameboy* const gb) {
  pthread_t cpu;
  pthread_t ppu;
  Result result = RESULT_OK;
  int window_open = 1;

  if (pthread_create(&cpu, NULL, GameboyRunCpu, gb) != 0) {
    gb->global_ctx->error = CPU_THREAD_CREATION_FAILED;
    return RESULT_NOTOK;
  }
  if (pthread_create(&ppu, NULL, GameboyRunPpu, gb) != 0) {
    gb->global_ctx->error = PPU_THREAD_CREATION_FAILED;
    return RESULT_NOTOK;
  }

  SDL_Event event;
  while (window_open) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        window_open = 0;
        gb->global_ctx->status = STATUS_STOP;
      }
    }

    if (gb->global_ctx->error != NO_ERROR) {
      result = RESULT_NOTOK;
      break;
    }
  }

  if (pthread_join(cpu, NULL) != 0) {
    gb->global_ctx->error = CPU_THREAD_JOIN_FAILED;
    return RESULT_NOTOK;
  }
  if (pthread_join(ppu, NULL) != 0) {
    gb->global_ctx->error = PPU_THREAD_JOIN_FAILED;
    return RESULT_NOTOK;
  }
  return result;
}
