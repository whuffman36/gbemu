#ifndef GLOBAL_H
#define GLOBAL_H


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
  "ILLEGAL INSTRUCTION PARAMETER"
};

typedef enum GBModeDef {
  GB_MODE_GBC = 0,
  GB_MODE_DMG = 1,
} GBMode;

typedef enum GBStatusDef {
  STATUS_RUNNING = 0,
  STATUS_PAUSED = 1,
  STATUS_STOP = 2,
} GBStatus;

typedef struct GlobalCtxDef {
  GBMode mode;
  ErrorCode error;
  GBStatus status;
  unsigned int clock;
} GlobalCtx;

#endif