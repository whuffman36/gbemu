#include "gb.h"
#include "global.h"

#include "stdio.h"


// gbemu <romfile>
int main(int argc, char** argv) {
  if (argc < 2) {
    printf("Usage: gbemu <romfile>\n");
    return 1;
  }
  const char* romfile = argv[1];

  GlobalCtx global_ctx = {
    .mode = GB_MODE_GBC,
    .error = NO_ERROR,
    .clock = 0
  };

  Gameboy gb = {
    .bus_ = NULL,
    .cartridge_ = NULL,
    .global_ctx_ = &global_ctx
  };

  printf("Starting up gameboy...\n\n");
  Result result = GameboyInit(&gb, romfile);
  if (result == RESULT_NOTOK) {
    printf("Fatal Error: %s\n", _ERROR_CODE_STRINGS[gb.global_ctx_->error]);
    return 1;
  }

  printf("Gameboy running!\n\n");
  GameboyRun(&gb);

  if (gb.global_ctx_->error != NO_ERROR) {
    printf("Fatal Error: %s\n\n", _ERROR_CODE_STRINGS[gb.global_ctx_->error]);
  }

  GameboyDestroy(&gb);
  return 0;
}
