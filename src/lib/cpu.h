#ifndef CPU_H
#define CPU_H

#include "bus.h"
#include "global.h"
#include "instruction.h"

#include <stdint.h>


typedef struct CpuRegistersDef {
  uint8_t a;
  uint8_t f;
  uint8_t b;
  uint8_t c;
  uint8_t d;
  uint8_t e;
  uint8_t h;
  uint8_t l;
} CpuRegisters;

typedef enum CpuFlagsDef {
  FLAG_CARRY = 0,
  FLAG_HALF_CARRY = 1,
  FLAG_ADD_SUB = 2,
  FLAG_ZERO = 3
} CpuFlags;

typedef struct CpuDef {
  CpuRegisters regs;
  Bus* bus;
  GlobalCtx* global_ctx;
  uint8_t flags[4];
  uint16_t pc;
  uint16_t sp;
  int interrupt_master_enable;
} Cpu;


void CpuInit(Cpu* const cpu);

void CpuStep(Cpu* const cpu);

#endif
