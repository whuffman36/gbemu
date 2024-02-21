#ifndef CPU_H
#define CPU_H

#include "bus.h"
#include "global.h"
#include "instruction.h"

#include "stdint.h"
#include "string.h"


typedef struct CpuRegistersDef {
  uint8_t a_;
  uint8_t f_;
  uint8_t b_;
  uint8_t c_;
  uint8_t d_;
  uint8_t e_;
  uint8_t h_;
  uint8_t l_;
} CpuRegisters;

typedef enum CpuFlagsDef {
  FLAG_CARRY = 0,
  FLAG_HALF_CARRY = 1,
  FLAG_ADD_SUB = 2,
  FLAG_ZERO = 3
} CpuFlags;

typedef struct CpuDef {
  CpuRegisters regs_;
  Bus* bus_;
  GlobalCtx* global_ctx_;
  uint8_t flags_[4];
  uint16_t pc_;
  uint16_t sp_;
  int interrupt_master_enable_;
} Cpu;


void CpuInit(Cpu* const cpu);

void CpuStep(Cpu* const cpu);

#endif
