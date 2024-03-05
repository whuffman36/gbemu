#ifndef GLOBAL_H
#define GLOBAL_H

#include <pthread.h>
#include <stdint.h>

#define GB_DEBUG_MODE


typedef enum ResultDef {
  RESULT_OK =  0,
  RESULT_NOTOK = 1,
} Result;

typedef enum ErrorCodeDef {
  ILLEGAL_WRITE_TO_MEMORY = 0,
  MEMORY_ALLOCATION_FAILURE = 1,
  FILE_NOT_FOUND = 2,
  FAILED_TO_READ_ROM = 3,
  HEADER_CHECKSUM_FAILED = 4,
  MBC_TYPE_NOT_SUPPORTED = 5,
  ILLEGAL_INSTRUCTION = 6,
  ILLEGAL_INSTRUCTION_PARAMETER = 7,
  UNKNOWN_INTERRUPT_REQUESTED = 8,
  SDL_VIDEO_INIT_ERROR = 9,
  SDL_WINDOW_CREATION_FAILED = 10,
  CPU_THREAD_CREATION_FAILED = 11,
  PPU_THREAD_CREATION_FAILED = 12,
  CPU_THREAD_JOIN_FAILED = 13,
  PPU_THREAD_JOIN_FAILED = 14,
  NO_ERROR,
} ErrorCode;

static const char* _ERROR_CODE_STRINGS[] = {
  "ILLEGAL WRITE TO MEMORY",
  "MEMORY ALLOCATION FAILURE",
  "FILE NOT FOUND",
  "FAILED TO READ ROM",
  "HEADER CHECKSUM FAILED",
  "MBC TYPE NOT SUPPORTED",
  "ILLEGAL INSTRUCTION",
  "ILLEGAL INSTRUCTION PARAMETER",
  "UNKNOWN INTERRUPT REQUESTED",
  "SDL VIDEO INIT ERROR",
  "SDL WINDOW CREATION FAIL",
  "CPU THREAD CREATION FAILED",
  "PPU THREAD CREATION FAILED",
  "CPU THREAD JOIN FAILED",
  "PPU THREAD JOIN FAILED"
};

typedef enum GBModeDef {
  GB_MODE_GBC = 0,
  GB_MODE_DMG = 1,
} GBMode;

typedef enum GBStatusDef {
  STATUS_RUNNING = 0,
  STATUS_PAUSED = 1,
  STATUS_HALT = 2,
  STATUS_STOP = 3,
} GBStatus;

typedef enum InterruptTypeDef {
  INTERRUPT_VBANK = 0,
  INTERRUPT_STAT = 1,
  INTERRUPT_TIMER = 2,
  INTERRUPT_SERIAL = 3,
  INTERRUPT_JOYPAD = 4
} InterruptType;

typedef struct GlobalCtxDef {
  GBMode mode;
  ErrorCode error;
  GBStatus status;
  unsigned int clock;
  pthread_mutex_t interrupt_mtx;
  pthread_cond_t interrupt_write;
} GlobalCtx;


extern void RequestInterrupt(InterruptType it, uint8_t* const interrupts_flag);

#endif
