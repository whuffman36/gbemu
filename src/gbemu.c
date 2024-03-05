#include "gb.h"
#include "global.h"

#include <stdio.h>


static GlobalCtx global_ctx;


// gbemu <romfile>
int main(int argc, char** argv) {
  if (argc < 2) {
    printf("Usage: gbemu <romfile>\n");
    return 1;
  }
  const char* romfile = argv[1];

  Gameboy gb = {
    .cpu = {0},
    .cartridge = NULL,
    .bus = NULL,
    .screen = NULL,
    .global_ctx = &global_ctx
  };

  printf("Starting up gameboy...\n\n");

  Result result = GameboyInit(&gb, romfile);
  if (result == RESULT_NOTOK) {
    printf("Fatal Error: %s\n", _ERROR_CODE_STRINGS[gb.global_ctx->error]);
    return 1;
  }

  printf("Gameboy running!\n\n");

  if (GameboyRun(&gb) == RESULT_NOTOK) {
    printf("Fatal Error: %s\n", _ERROR_CODE_STRINGS[gb.global_ctx->error]);
  }

  printf("Gameboy powering off...\n");
  GameboyDestroy(&gb);
  return 0;
}
