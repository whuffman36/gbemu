#include "gb.h"
#include "global.h"
#include "translate.h"

#include <pthread.h>
#include <stdio.h>

#include "SDL2/SDL.h"


static pthread_mutex_t interrupt_mtx;
static pthread_cond_t interrupt_write;

static const int _WINDOW_WIDTH = 640;
static const int _WINDOW_HEIGHT = 480;


// gbemu <romfile>
int main(int argc, char** argv) {
  if (argc < 2) {
    printf("Usage: gbemu <romfile>\n");
    return 1;
  }
  const char* romfile = argv[1];

  if (pthread_mutex_init(&interrupt_mtx, NULL) != 0) {
    printf("Failed to initialize mutex: interrupt_mtx\n");
    return 1;
  }
  if (pthread_cond_init(&interrupt_write, NULL) != 0) {
    printf("Failed to initialize condition variable: interrupt_write\n");
    return 1;
  }

  static GlobalCtx global_ctx = {
    .mode = GB_MODE_GBC,
    .error = NO_ERROR,
    .status = STATUS_RUNNING,
    .clock = 0,
    .interrupt_mtx = &interrupt_mtx,
    .interrupt_write = &interrupt_write
  };

  Gameboy gb = {
    .bus = NULL,
    .cartridge = NULL,
    .global_ctx = &global_ctx
  };

  Translate(romfile);

  printf("Starting up gameboy...\n\n");

  Result result = GameboyInit(&gb, romfile);
  if (result == RESULT_NOTOK) {
    printf("Fatal Error: %s\n", _ERROR_CODE_STRINGS[gb.global_ctx->error]);
    return 1;
  }

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("Fatal Error: Failed to initialize SDL: %s\n", SDL_GetError());
    return 1;
  }
  SDL_Window* window = SDL_CreateWindow(
                          argv[0],
                          0,
                          0,
                          _WINDOW_WIDTH,
                          _WINDOW_HEIGHT,
                          SDL_WINDOW_SHOWN
                       );
  if (window == NULL) {
    printf("Fata Error: Failed to create SDL window: %s", SDL_GetError());
  }
  int window_open = 1;

  printf("Gameboy running!\n\n");
/*
  pthread_t cpu;
  pthread_t ppu;

  if (pthread_create(&cpu, NULL, GameboyRunCpu, &gb) != 0) {
    printf("Fatal Error: could not create CPU thread\n");
  }
  if (pthread_create(&ppu, NULL, GameboyRunPpu, &gb) != 0) {
    printf("Fatal Error: could not create PPU thread\n");
  }
*/
  char c;
  SDL_Event event;
  while (window_open) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        window_open = 0;
        gb.global_ctx->status = STATUS_STOP;
      }
    }/*
    scanf("%c", &c);
    if (c == 'q') {
      break;
    }*/
    CpuStep(&gb.cpu);
    if (gb.global_ctx->error != NO_ERROR) {
      printf("Fatal Error: %s\n\n", _ERROR_CODE_STRINGS[gb.global_ctx->error]);
      break;
    }
  }
  SDL_DestroyWindow(window);
/*
  if (pthread_join(cpu, NULL) != 0) {
    printf("Fatal Error: CPU thread failed to join successfully\n");
  }
  if (pthread_join(ppu, NULL) != 0) {
    printf("Fatal Error: PPU thread failed to join successfully\n");
  }
*/
  pthread_mutex_destroy(&interrupt_mtx);
  pthread_cond_destroy(&interrupt_write);
  SDL_Quit();
  GameboyDestroy(&gb);
  return 0;
}
