#include "cpu.h"

#include "testing.h"

/*
static void TestReadReg(void);
static void TestWriteReg(void);


void TestCpu(void) {
  tmodbegin_

  TestReadReg();
  TestWriteReg();

  tmodend_
}

static void TestReadReg(void) {
  tbegin_

  Cpu cpu;
  CpuInit(&cpu);
  cpu.regs_.a_ = 1;
  tassert_(CpuReadReg(&cpu, REG_AF) == 0x0100);

  cpu.regs_.b_ = 3;
  cpu.regs_.c_ = 7;
  tassert_(CpuReadReg(&cpu, REG_BC) == 0x0307);

  cpu.regs_.d_ = 0xFF;
  cpu.regs_.e_ = 0x0F;
  tassert_(CpuReadReg(&cpu, REG_DE) == 0xFF0F);

  cpu.regs_.h_ = 0xAC;
  cpu.regs_.l_ = 0x06;
  tassert_(CpuReadReg(&cpu, REG_HL) == 0xAC06);

  tend_
}

static void TestWriteReg(void) {
  tbegin_

  Cpu cpu;
  CpuInit(&cpu);

  CpuWriteReg(&cpu, REG_AF, 0x43FF);
  tassert_(CpuReadReg(&cpu, REG_AF) == 0x43FF);

  CpuWriteReg(&cpu, REG_BC, 0x0101);
  tassert_(CpuReadReg(&cpu, REG_BC) == 0x0101);

  CpuWriteReg(&cpu, REG_DE, 0x1001);
  tassert_(CpuReadReg(&cpu, REG_DE) == 0x1001);

  CpuWriteReg(&cpu, REG_HL, 0xFFFF);
  tassert_(CpuReadReg(&cpu, REG_HL) == 0xFFFF);

  tend_
}
*/
