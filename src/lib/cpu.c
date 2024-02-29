#include "cpu.h"

#include "instruction.h"
#include "timer.h"

#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>
#ifdef GB_DEBUG_MODE
  #include <stdio.h>
#endif


#define MostSigByte_(bits16) (uint8_t)((bits16 & 0xFF00) >> 8)
#define LeastSigByte_(bits16) (uint8_t)(bits16 & 0x00FF)
#define CombineBytes_(hi, lo) (uint16_t)(((uint16_t)hi << 8) | (uint16_t)lo)


static const uint16_t _INTERRUPT_VBANK_ADDR = 0x0040;
static const uint16_t _INTERRUPT_STAT_ADDR = 0x0048;
static const uint16_t _INTERRUPT_TIMER_ADDR = 0x0050;
static const uint16_t _INTERRUPT_SERIAL_ADDR = 0x0058;
static const uint16_t _INTERRUPT_JOYPAD_ADDR = 0x0060;


typedef enum ShiftDirectionDef {
  RIGHT,
  LEFT
} ShiftDirection;


void CpuInit(Cpu* const cpu) {
  cpu->bus = NULL;
  cpu->global_ctx = NULL;

  cpu->regs.a = 0x01;
  cpu->regs.f = 0xB0;
  cpu->regs.b = 0x00;
  cpu->regs.c = 0x13;
  cpu->regs.d = 0x00;
  cpu->regs.e = 0xD8;
  cpu->regs.h = 0x01;
  cpu->regs.l = 0x4D;
  cpu->flags[FLAG_CARRY] = 0;
  cpu->flags[FLAG_HALF_CARRY] = 0;
  cpu->flags[FLAG_ADD_SUB] = 0;
  cpu->flags[FLAG_ZERO] = 1;
  cpu->pc = 0x0100;
  cpu->sp = 0xFFFE;
  cpu->interrupt_master_enable = 0;
}


#ifdef GB_DEBUG_MODE
  static void PrintInstruction(const Instruction* const instr) {
    printf("Raw binary: 0x%02x\n", instr->raw_instr);
    printf("%s %s %s %s\n\n", _INSTRUCTION_STR_MAP[instr->opcode],
                              _INSTRUCTION_PARAM_STR_MAP[instr->param1],
                              _INSTRUCTION_PARAM_STR_MAP[instr->param2],
                              _INSTRUCTION_COND_STR_MAP[instr->cond]);
    printf("-------------------------------------------------\n\n");
  }


  static void PrintCpuState(const Cpu* const cpu) {
    printf("Registers:\n");
    printf("\tA: 0x%02x | %d\n", cpu->regs.a, cpu->regs.a);
    printf("\tB: 0x%02x | %d\n", cpu->regs.b, cpu->regs.b);
    printf("\tC: 0x%02x | %d\n", cpu->regs.c, cpu->regs.c);
    printf("\tD: 0x%02x | %d\n", cpu->regs.d, cpu->regs.d);
    printf("\tE: 0x%02x | %d\n", cpu->regs.e, cpu->regs.e);
    printf("\tH: 0x%02x | %d\n", cpu->regs.h, cpu->regs.h);
    printf("\tL: 0x%02x | %d\n", cpu->regs.l, cpu->regs.l);
    printf("\tAF: 0x%04x | %d\n", CombineBytes_(cpu->regs.a, cpu->regs.f),
                                  CombineBytes_(cpu->regs.a, cpu->regs.f));
    printf("\tBC: 0x%04x | %d\n", CombineBytes_(cpu->regs.b, cpu->regs.c),
                                  CombineBytes_(cpu->regs.b, cpu->regs.c));
    printf("\tDE: 0x%04x | %d\n", CombineBytes_(cpu->regs.d, cpu->regs.e),
                                  CombineBytes_(cpu->regs.d, cpu->regs.e));
    printf("\tHL: 0x%04x | %d\n", CombineBytes_(cpu->regs.h, cpu->regs.l),
                                  CombineBytes_(cpu->regs.h, cpu->regs.l));
    printf("Flags:\n");
    printf("\tZ: %d N: %d H: %d C: %d\n", cpu->flags[FLAG_ZERO],
                                          cpu->flags[FLAG_ADD_SUB],
                                          cpu->flags[FLAG_HALF_CARRY],
                                          cpu->flags[FLAG_CARRY]);
    printf("Program Counter:\n");
    printf("\t0x%04x | %d\n", cpu->pc, cpu->pc);
    printf("Stack Pointer:\n");
    printf("\t0x%04x | %d\n", cpu->sp, cpu->sp);
    printf("Master Interrupt Enable:\n");
    printf("\t%s\n\n", cpu->interrupt_master_enable ? "Enabled" : "Disabled");
  }


  static void PrintSerialDebug(Cpu* const cpu) {
    static char dbg_msg[1024] = {0};
    static int msg_size = 0;
    if (cpu->bus->serial_data[1] == 0x81) {
      printf("DBG: Byte received\n");
      char c = (char)cpu->bus->serial_data[0];
      printf("DBG: %c\n", c);
      dbg_msg[msg_size++] = c;
      cpu->bus->serial_data[1] = 0;
    }

    if (dbg_msg[0]) {
      printf("DBG: Message size: %d\n", msg_size);
      printf("DBG: %s\n", dbg_msg);
    }
  }
#endif


void RequestInterrupt(InterruptType it, uint8_t *const interrupts_flag) {
  switch(it) {
    case INTERRUPT_VBANK:
      *interrupts_flag |= 0x01;
      break;
    case INTERRUPT_STAT:
      *interrupts_flag |= 0x02;
      break;
    case INTERRUPT_TIMER:
      *interrupts_flag |= 0x04;
      break;
    case INTERRUPT_SERIAL:
      *interrupts_flag |= 0x08;
      break;
    case INTERRUPT_JOYPAD:
      *interrupts_flag |= 0x10;
      break;
  }
}


static void HandleInterrupt(Cpu* const cpu) {
  cpu->interrupt_master_enable = 0;
  uint16_t interrupt = 0;

  if ((cpu->bus->interrupts_flag & 0x01) == 1 &&
      (cpu->bus->interrupts_enable_reg & 0x01) == 1) {
    interrupt = _INTERRUPT_VBANK_ADDR;
    cpu->bus->interrupts_flag &= 0xFE;
  }
  else if ((cpu->bus->interrupts_flag & 0x02) == 2 &&
           (cpu->bus->interrupts_enable_reg & 0x02) == 2) {
    interrupt = _INTERRUPT_STAT_ADDR;
    cpu->bus->interrupts_flag &= 0xFD;
  }
  else if ((cpu->bus->interrupts_flag & 0x04) == 4 &&
           (cpu->bus->interrupts_enable_reg & 0x04) == 4) {
    interrupt = _INTERRUPT_TIMER_ADDR;
    cpu->bus->interrupts_flag &= 0xFB;
  }
  else if ((cpu->bus->interrupts_flag & 0x08) == 8 &&
           (cpu->bus->interrupts_enable_reg & 0x08) == 8) {
    interrupt = _INTERRUPT_SERIAL_ADDR;
    cpu->bus->interrupts_flag &= 0xF7;
  }
  else if ((cpu->bus->interrupts_flag & 0x10) == 16 &&
           (cpu->bus->interrupts_enable_reg & 0x10) == 16) {
    interrupt = _INTERRUPT_JOYPAD_ADDR;
    cpu->bus->interrupts_flag &= 0xEF;
  }
  else {
    cpu->global_ctx->error = UNKNOWN_INTERRUPT_REQUESTED;
    return;
  }

  uint8_t hi = MostSigByte_(cpu->pc);
  uint8_t lo = LeastSigByte_(cpu->pc);
  BusWrite(cpu->bus, --cpu->sp, hi);
  BusWrite(cpu->bus, --cpu->sp, lo);
  cpu->pc = interrupt;

  cpu->global_ctx->clock += 20;
  for (int i = 0; i < 5; ++i) {
    TimerTick(&cpu->bus->timer, &cpu->bus->interrupts_flag);
  }
}


static uint8_t* ReadReg(Cpu* const cpu, const InstructionParameter reg) {
  switch(reg) {
    case PARA_REG_A:
      return &cpu->regs.a ;
    case PARA_REG_B:
      return &cpu->regs.b ;
    case PARA_REG_C:
      return &cpu->regs.c ;
    case PARA_REG_D:
      return &cpu->regs.d ;
    case PARA_REG_E:
      return &cpu->regs.e ;
    case PARA_REG_H:
      return &cpu->regs.h ;
    case PARA_REG_L:
      return &cpu->regs.l ;
    default:
      __builtin_unreachable();
  }
}


static uint16_t ReadReg16(const Cpu* const cpu, InstructionParameter reg) {
  switch(reg) {
    case PARA_REG_AF:
      return CombineBytes_(cpu->regs.a, cpu->regs.f);
    case PARA_REG_BC:
      return CombineBytes_(cpu->regs.b, cpu->regs.c);
    case PARA_REG_DE:
      return CombineBytes_(cpu->regs.d, cpu->regs.e);
    case PARA_REG_HL:
      return CombineBytes_(cpu->regs.h, cpu->regs.l);
    case PARA_SP:
      return cpu->sp ;
    default:
      __builtin_unreachable();
  }
}


static void WriteReg16(Cpu* const cpu, InstructionParameter reg,
                       uint16_t data) {
  switch(reg) {
    case PARA_REG_AF:
      cpu->regs.a = data >> 8;
      cpu->regs.f = data;
      break;
    case PARA_REG_BC:
      cpu->regs.b = data >> 8;
      cpu->regs.c = data;
      break;
    case PARA_REG_DE:
      cpu->regs.d = data >> 8;
      cpu->regs.e = data;
      break;
    case PARA_REG_HL:
      cpu->regs.h = data >> 8;
      cpu->regs.l = data;
      break;
    default:
      break;
  } 
}


static uint16_t ReadImm16(Cpu* const cpu) {
  // Memory is little endian.
  uint8_t lo = BusRead(cpu->bus, cpu->pc++);
  uint8_t hi = BusRead(cpu->bus, cpu->pc++);
  return CombineBytes_(hi, lo);
}


static void WriteImm16(const Cpu* const cpu, uint16_t addr, uint16_t data) {
  // Memory is little endian.
  uint8_t hi = MostSigByte_(data);
  uint8_t lo = LeastSigByte_(data);
  BusWrite(cpu->bus, addr, lo);
  BusWrite(cpu->bus, addr + 1, hi);
}


static void UpdateFlagsRegister(Cpu* const cpu) {
  cpu->regs.f |= cpu->flags[FLAG_ZERO] << 7;
  cpu->regs.f |= cpu->flags[FLAG_ADD_SUB] << 6;
  cpu->regs.f |= cpu->flags[FLAG_HALF_CARRY] << 5;
  cpu->regs.f |= cpu->flags[FLAG_CARRY] << 4;
}


static void Halt(Cpu* const cpu) {
  pthread_mutex_lock(cpu->global_ctx->interrupt_mtx);
  while (!(cpu->interrupt_master_enable &&
          (cpu->bus->interrupts_enable_reg &
           cpu->bus->interrupts_flag) != 0)) {
    pthread_cond_wait(cpu->global_ctx->interrupt_write, cpu->global_ctx->interrupt_mtx);
  }
  HandleInterrupt(cpu);
  pthread_mutex_unlock(cpu->global_ctx->interrupt_mtx);
}


static void LoadReg(Cpu* const cpu, const Instruction* const instr) {
  uint8_t source = 0;
  int8_t offset = 0;
  uint16_t source_16 = 0;
  uint16_t reg_hl = 0;

  // Source is from an 8 bit register.
  if (instr->param2 >= PARA_REG_A && instr->param2 <= PARA_REG_L) {
    source = *ReadReg(cpu, instr->param2);
  }
  // Source is from memory[16 bit register].
  else if (instr->param2 >= PARA_MEM_REG_BC &&
           instr->param2 <= PARA_MEM_REG_HL) {
    source = BusRead(cpu->bus,
                     ReadReg16(cpu, instr->param2 -
                              (PARA_MEM_REG_BC - PARA_REG_BC)));
  }
  // Other sources.
  else {
    switch(instr->param2) {
      case PARA_SP_IMM_8:
        offset = (int8_t)BusRead(cpu->bus, cpu->pc++);
        source_16 = (uint16_t)((int16_t)cpu->sp + offset);
        break;
      case PARA_MEM_REG_HL_INC:
        reg_hl = ReadReg16(cpu, PARA_REG_HL);
        source = BusRead(cpu->bus, reg_hl);
        WriteReg16(cpu, PARA_REG_HL, reg_hl + 1);
        break;
      case PARA_MEM_REG_HL_DEC:
        reg_hl = ReadReg16(cpu, PARA_REG_HL);
        source = BusRead(cpu->bus, reg_hl);
        WriteReg16(cpu, PARA_REG_HL, reg_hl - 1);
        break;
      case PARA_IMM_8:
        source = BusRead(cpu->bus, cpu->pc);
        cpu->pc++;
        break;
      case PARA_IMM_16:
        source_16 = ReadImm16(cpu);
        break;
      case PARA_ADDR:
        source_16 = ReadImm16(cpu);
        source = BusRead(cpu->bus, source_16);
        break;
      default:
        cpu->global_ctx->error = ILLEGAL_INSTRUCTION_PARAMETER;
        return;
    }
  }

  // Destination is an 8 bit register.
  if (instr->param1 >= PARA_REG_A && instr->param1 <= PARA_REG_L) {
    *ReadReg(cpu, instr->param1) = source;
  }
  // Destination is a 16 bit register.
  if (instr->param1 >= PARA_REG_BC && instr->param1 <= PARA_REG_HL) {
    WriteReg16(cpu, instr->param1, source_16);
  }
}


static void LoadSP(Cpu* const cpu, const Instruction* const instr) {
  switch(instr->param2) {
    case PARA_IMM_16:
      cpu->sp = ReadImm16(cpu);
      break;
    case PARA_REG_HL:
      cpu->sp = ReadReg16(cpu, PARA_REG_HL);
      break;
    default:
      cpu->global_ctx->error = ILLEGAL_INSTRUCTION_PARAMETER;
      return;
  }
}


static void LoadMem(Cpu* const cpu, const Instruction* const instr) {
  uint8_t source = 0;
  uint16_t source_16 = 0;
  uint16_t reg_hl = 0;

  if (instr->param2 >= PARA_REG_A && instr->param2 <= PARA_REG_L) {
    source = *ReadReg(cpu, instr->param2);
  }
  else {
    switch(instr->param2) {
      case PARA_SP:
        source_16 = cpu->sp ;
        break;
      case PARA_IMM_8:
        source = BusRead(cpu->bus, cpu->pc++);
        break;
      default:
        cpu->global_ctx->error = ILLEGAL_INSTRUCTION_PARAMETER;
        return;
    }
  }

  if (instr->param1 >= PARA_MEM_REG_BC && instr->param1 <= PARA_MEM_REG_HL) {
    BusWrite(cpu->bus,
             ReadReg16(cpu, instr->param1 - (PARA_MEM_REG_BC - PARA_REG_BC)),
                       source);
  }
  else {
    switch (instr->param1) {
      case PARA_MEM_REG_HL_INC:
        reg_hl = ReadReg16(cpu, PARA_REG_HL);
        BusWrite(cpu->bus, reg_hl, source);
        WriteReg16(cpu, PARA_REG_HL, reg_hl + 1);
        break;
      case PARA_MEM_REG_HL_DEC:
        reg_hl = ReadReg16(cpu, PARA_REG_HL);
        BusWrite(cpu->bus, reg_hl, source);
        WriteReg16(cpu, PARA_REG_HL, reg_hl - 1);
        break;
      case PARA_ADDR:
        WriteImm16(cpu, ReadImm16(cpu), source_16);
        break;
      default:
        cpu->global_ctx->error = ILLEGAL_INSTRUCTION_PARAMETER;
        return;
    }
  }
}


static void Load(Cpu* const cpu, const Instruction* const instr) {
  InstructionParameter destination = instr->param1 ;
  if (destination < 12) {
    // destination is a register.
    LoadReg(cpu, instr);
  }
  else if (destination == PARA_SP) {
    // destination is stack pointer.
    LoadSP(cpu, instr);
  }
  else {
    // destination is Memory address.
    LoadMem(cpu, instr);
  }
}


static void LoadH(Cpu* const cpu, const Instruction* const instr) {
  uint8_t source = 0;
  uint8_t dest = 0;

  switch(instr->param2) {
    case PARA_REG_A:
      source = cpu->regs.a ;
      break;
    case PARA_MEM_REG_C:
      source = BusRead(cpu->bus, 0xFF00 + (uint16_t)cpu->regs.c);
      break;
    case PARA_IMM_8:
      source = BusRead(cpu->bus, cpu->pc++);
      break;
    default:
      cpu->global_ctx->error = ILLEGAL_INSTRUCTION_PARAMETER;
      return;
  }

  switch(instr->param1) {
    case PARA_REG_A:
      cpu->regs.a = source;
      break;
    case PARA_MEM_REG_C:
      BusWrite(cpu->bus, 0xFF00 + (uint16_t)cpu->regs.c, source);
      break;
    case PARA_IMM_8:
      dest = BusRead(cpu->bus, cpu->pc++);
      BusWrite(cpu->bus, 0xFF00 + (uint16_t)dest, source);
      break;
    default:
      cpu->global_ctx->error = ILLEGAL_INSTRUCTION_PARAMETER;
      return;
  }
}


static void Increment(Cpu* const cpu, const Instruction* const instr) {
  uint8_t result = 0;
  int is_8_bit = 0;
  uint16_t reg_hl = 0;

  if (instr->param1 >= PARA_REG_A && instr->param1 <= PARA_REG_L) {
    result = ++(*ReadReg(cpu, instr->param1));
    is_8_bit = 1;
  }
  else if (instr->param1 >= PARA_REG_BC && instr->param1 <= PARA_REG_HL) {
    WriteReg16(cpu, PARA_REG_BC, ReadReg16(cpu, PARA_REG_BC) + 1);
  }
  else {
    switch(instr->param1) {
      case PARA_SP:
        ++cpu->sp ;
        break;
      case PARA_MEM_REG_HL:
        reg_hl = ReadReg16(cpu, PARA_REG_HL);
        result = BusRead(cpu->bus, reg_hl);
        BusWrite(cpu->bus, reg_hl, ++result);
        is_8_bit = 1;
        break;
      default:
        cpu->global_ctx->error = ILLEGAL_INSTRUCTION_PARAMETER;
        return;
    }
  }

  if (is_8_bit) {
    cpu->flags[FLAG_ADD_SUB] = 0;
    cpu->flags[FLAG_ZERO] = (result == 0);
    cpu->flags[FLAG_HALF_CARRY] = (result & 0x0F) == 0x00;
    UpdateFlagsRegister(cpu);
  }
}


static void Decrement(Cpu* const cpu, const Instruction* const instr) {
  uint8_t result = 0;
  int is_8_bit = 0;
  uint16_t reg_hl = 0;

  if (instr->param1 >= PARA_REG_A && instr->param1 <= PARA_REG_L) {
    result = --(*ReadReg(cpu, instr->param1));
    is_8_bit = 1;
  }
  else if (instr->param1 >= PARA_REG_BC && instr->param1 <= PARA_REG_HL) {
    WriteReg16(cpu, PARA_REG_BC, ReadReg16(cpu, PARA_REG_BC) - 1);
  }
  else {
    switch(instr->param1) {
      case PARA_SP:
        --cpu->sp ;
        break;
      case PARA_MEM_REG_HL:
        reg_hl = ReadReg16(cpu, PARA_REG_HL);
        result = BusRead(cpu->bus, reg_hl);
        BusWrite(cpu->bus, reg_hl, --result);
        is_8_bit = 1;
        break;
      default:
        cpu->global_ctx->error = ILLEGAL_INSTRUCTION_PARAMETER;
        return;
    }
  }

  if (is_8_bit) {
    cpu->flags[FLAG_ADD_SUB] = 1;
    cpu->flags[FLAG_ZERO] = (result == 0);
    cpu->flags[FLAG_HALF_CARRY] = (result & 0x0F) == 0x0F;
    UpdateFlagsRegister(cpu);
  }
}


static void StackPush(Cpu* const cpu, const Instruction* const instr) {
  uint16_t source = ReadReg16(cpu, instr->param1);
  uint8_t hi = MostSigByte_(source);
  uint8_t lo = LeastSigByte_(source);

  // The stack grows up, as in the address decreases as things are added
  // to the stack. This is why WriteImm16() cannot be used.
  BusWrite(cpu->bus, --cpu->sp, hi);
  BusWrite(cpu->bus, --cpu->sp, lo);
}


static void StackPop(Cpu* const cpu, const Instruction* const instr) {
  uint16_t lo = (uint16_t)BusRead(cpu->bus, cpu->sp++);
  uint16_t hi = (uint16_t)BusRead(cpu->bus, cpu->sp++);
  uint16_t source = CombineBytes_(hi, lo);

  WriteReg16(cpu, instr->param1, source);
}


static void Jump(Cpu* const cpu, Instruction* const instr) {
  if (instr->param1 == PARA_REG_HL) {
    cpu->pc = ReadReg16(cpu, PARA_REG_HL);
    return;
  }

  // Even though it's less efficient, the next bytes need to be read to
  // keep the Pragram Coounter accurate.
  uint16_t jmp_addr = ReadImm16(cpu);
  int condition = 0;

  switch(instr->cond) {
    case COND_NONE:
      condition = 1;
      break;
    case COND_NZ:
      condition = !cpu->flags[FLAG_ZERO];
      break;
    case COND_Z:
      condition = cpu->flags[FLAG_ZERO];
      break;
    case COND_NC:
      condition = !cpu->flags[FLAG_CARRY];
      break;
    case COND_C:
      condition = cpu->flags[FLAG_CARRY];
      break;
    default:
      cpu->global_ctx->error = ILLEGAL_INSTRUCTION_PARAMETER;
      return;
  }

  if (condition) {
    cpu->pc = jmp_addr;
    if (instr->cond != COND_NONE) {
      instr->cycles += 4;
    }
  }
}


static void RelativeJump(Cpu* const cpu, Instruction* const instr) {
  int8_t offset = (int8_t)BusRead(cpu->bus, cpu->pc++);
  uint16_t jmp_addr = (uint16_t)((int16_t)cpu->pc + (int16_t)offset);
  int condition = 0;

  switch(instr->cond) {
    case COND_NONE:
      condition = 1;
      break;
    case COND_NZ:
      condition = !cpu->flags[FLAG_ZERO];
      break;
    case COND_Z:
      condition = cpu->flags[FLAG_ZERO];
      break;
    case COND_NC:
      condition = !cpu->flags[FLAG_CARRY];
      break;
    case COND_C:
      condition = cpu->flags[FLAG_CARRY];
      break;
  }

  if (condition) {
    cpu->pc = jmp_addr;
    if (instr->cond != COND_NONE) {
      instr->cycles += 4;
    }
  }
}


static void Call(Cpu* const cpu, Instruction* const instr) {
  uint16_t call_addr = ReadImm16(cpu);
  int condition = 0;

  switch(instr->cond) {
    case COND_NONE:
      condition = 1;
      break;
    case COND_NZ:
      condition = !cpu->flags[FLAG_ZERO];
      break;
    case COND_Z:
      condition = cpu->flags[FLAG_ZERO];
      break;
    case COND_NC:
      condition = !cpu->flags[FLAG_CARRY];
      break;
    case COND_C:
      condition = cpu->flags[FLAG_CARRY];
      break;
  }

  if (condition) {
    uint8_t hi = MostSigByte_(cpu->pc);
    uint8_t lo = LeastSigByte_(cpu->pc);
    BusWrite(cpu->bus, --cpu->sp, hi);
    BusWrite(cpu->bus, --cpu->sp, lo);
    cpu->pc = call_addr;
    if (instr->cond != COND_NONE) {
      instr->cycles += 12;
    }
  }
}


static void Return(Cpu* const cpu, Instruction* const instr) {
  int condition = 0;

  switch(instr->cond) {
    case COND_NONE:
      condition = 1;
      break;
    case COND_NZ:
      condition = !cpu->flags[FLAG_ZERO];
      break;
    case COND_Z:
      condition = cpu->flags[FLAG_ZERO];
      break;
    case COND_NC:
      condition = !cpu->flags[FLAG_CARRY];
      break;
    case COND_C:
      condition = cpu->flags[FLAG_CARRY];
      break;
  }

  if (condition) {
    uint16_t lo = (uint16_t)BusRead(cpu->bus, cpu->sp++);
    uint16_t hi = (uint16_t)BusRead(cpu->bus, cpu->sp++);
    cpu->pc = CombineBytes_(hi, lo);
    if (instr->cond != COND_NONE) {
      instr->cycles += 12;
    }
  }
}


static void Restart(Cpu* const cpu, const Instruction* const instr) {
  uint8_t target = (instr->raw_instr & 0x38) >> 3;
  target *= 8;

  uint8_t hi = MostSigByte_(cpu->pc);
  uint8_t lo = LeastSigByte_(cpu->pc);
  BusWrite(cpu->bus, --cpu->sp, hi);
  BusWrite(cpu->bus, --cpu->sp, lo);
  cpu->pc = (uint16_t)target;
}


static void Add(Cpu* const cpu, const Instruction* const instr, int carry) {
  uint32_t initial = 0;

  switch(instr->param1) {
    case PARA_REG_A:
      initial = (uint32_t)cpu->regs.a ;
      if (instr->param2 == PARA_MEM_REG_HL) {
        cpu->regs.a += BusRead(cpu->bus, ReadReg16(cpu, PARA_REG_HL));
      }
      else if (instr->param2 == PARA_IMM_8) {
        cpu->regs.a += BusRead(cpu->bus, cpu->pc++);
      }
      else {
        cpu->regs.a += *ReadReg(cpu, instr->param2);
      }
      if (carry) {
        cpu->regs.a += cpu->flags[FLAG_CARRY];
      }
      cpu->flags[FLAG_ZERO] = cpu->regs.a == 0;
      cpu->flags[FLAG_CARRY] = cpu->regs.a < initial;
      cpu->flags[FLAG_HALF_CARRY] = (cpu->regs.a & 0x0F) > 0x0F;
      break;

    case PARA_REG_HL:
      initial = (uint32_t)ReadReg16(cpu, PARA_REG_HL) +
                (uint32_t)ReadReg16(cpu, instr->param2);
      WriteReg16(cpu, PARA_REG_HL, (uint16_t)initial);

      cpu->flags[FLAG_CARRY] = initial > 0xFFFF;
      cpu->flags[FLAG_HALF_CARRY] = (initial & 0x0F) > 0x0F;
      break;

    case PARA_SP:
      initial = (uint32_t)((int16_t)cpu->sp + (int8_t)BusRead(cpu->bus,
                                                                cpu->pc++));
      cpu->sp = (uint16_t)initial;
      cpu->flags[FLAG_ZERO] = 0;
      cpu->flags[FLAG_CARRY] = initial > 0xFFFF;
      cpu->flags[FLAG_HALF_CARRY] = (initial & 0x0F) > 0x0F;
      break;

    default:
      cpu->global_ctx->error = ILLEGAL_INSTRUCTION_PARAMETER;
      return;
  }
  cpu->flags[FLAG_ADD_SUB] = 0;
  UpdateFlagsRegister(cpu);
}


static void Subtract(Cpu* const cpu, const Instruction* const instr,
                     int carry) {
  uint8_t initial = cpu->regs.a ;

  if (instr->param2 >= PARA_REG_A && instr->param2 <= PARA_REG_L) {
    cpu->regs.a -= *ReadReg(cpu, instr->param2);
  }
  else if (instr->param2 == PARA_MEM_REG_HL) {
    cpu->regs.a -= BusRead(cpu->bus, ReadReg16(cpu, PARA_REG_HL));
  }
  else if (instr->param2 == PARA_IMM_8) {
    cpu->regs.a -= BusRead(cpu->bus, cpu->pc++);
  }
  if (carry) {
    cpu->regs.a -= cpu->flags[FLAG_CARRY];
  }

  cpu->flags[FLAG_ZERO] = cpu->regs.a == 0;
  cpu->flags[FLAG_ADD_SUB] = 1;
  cpu->flags[FLAG_CARRY] = (instr->param2 == PARA_REG_A) && carry
                            ? cpu->flags[FLAG_CARRY]
                            : (initial < cpu->regs.a);
  cpu->flags[FLAG_HALF_CARRY] = (cpu->regs.a & 0x0F) == 0x0F;
  UpdateFlagsRegister(cpu);
}


// I do not really understand  this instruction. Commented out is my
// implementation, essentially just converting the A reg to be in Binary
// Coded Decimal (BCD) format. Below is the implementation from youtube:
// "Gameboy Emulator Development - Part 09" by Low Level Devel.
static void DecimalAdjustAccumulator(Cpu* const cpu) {
  /*
  uint8_t hi = (cpu->regs.a & 0xF0) >> 4;
  uint8_t lo = (cpu->regs.a & 0x0F);

  if (hi > 9) {
    hi = 9;
  }
  if (lo > 9) {
    lo = 9;
  }
  cpu->regs.a = (hi * 10) + lo;

  cpu->flags[FLAG_ZERO] = cpu->regs.a == 0;
  cpu->flags[FLAG_HALF_CARRY] = 0;
  cpu->flags[FLAG_CARRY] = (cpu->regs.a & 0x0F) > 0x0F;
  */
  uint8_t u = 0;
  uint8_t carry_flag = 0;

  if (cpu->flags[FLAG_HALF_CARRY] || (!cpu->flags[FLAG_ADD_SUB] &&
                                      ((cpu->regs.a & 0x0F) > 9))) {
    u = 6;
  }
  if (cpu->flags[FLAG_CARRY] || (!cpu->flags[FLAG_ADD_SUB] &&
                                  (cpu->regs.a > 0x99))) {
    u |= 0x60;
    carry_flag = 1;
  }
  cpu->regs.a += cpu->flags[FLAG_ADD_SUB] ? -u : u;

  cpu->flags[FLAG_ZERO] = cpu->regs.a == 0;
  cpu->flags[FLAG_HALF_CARRY] = 0;
  cpu->flags[FLAG_CARRY] = carry_flag;
  UpdateFlagsRegister(cpu);
}


static void BitwiseAnd(Cpu* const cpu, const Instruction* const instr) {
  if (instr->param2 >= PARA_REG_A && instr->param2 <= PARA_REG_L) {
    cpu->regs.a &= *ReadReg(cpu, instr->param2);
  }
  else if (instr->param2 == PARA_MEM_REG_HL) {
    cpu->regs.a &= BusRead(cpu->bus, ReadReg16(cpu, PARA_REG_HL));
  }
  else if (instr->param2 == PARA_IMM_8) {
    cpu->regs.a &= BusRead(cpu->bus, cpu->pc++);
  }

  cpu->flags[FLAG_ZERO] = cpu->regs.a == 0;
  cpu->flags[FLAG_ADD_SUB] = 0;
  cpu->flags[FLAG_CARRY] = 0;
  cpu->flags[FLAG_HALF_CARRY] = 1;
  UpdateFlagsRegister(cpu);
}


static void BitwiseOr(Cpu* const cpu, const Instruction* const instr) {
  if (instr->param2 >= PARA_REG_A && instr->param2 <= PARA_REG_L) {
    cpu->regs.a |= *ReadReg(cpu, instr->param2);
  }
  else if (instr->param2 == PARA_MEM_REG_HL) {
    cpu->regs.a |= BusRead(cpu->bus, ReadReg16(cpu, PARA_REG_HL));
  }
  else if (instr->param2 == PARA_IMM_8) {
    cpu->regs.a |= BusRead(cpu->bus, cpu->pc++);
  }

  cpu->flags[FLAG_ZERO] = cpu->regs.a == 0;
  cpu->flags[FLAG_ADD_SUB] = 0;
  cpu->flags[FLAG_CARRY] = 0;
  cpu->flags[FLAG_HALF_CARRY] = 0;
  UpdateFlagsRegister(cpu);
}


static void BitwisXor(Cpu* const cpu, const Instruction* const instr) {
  if (instr->param2 >= PARA_REG_A && instr->param2 <= PARA_REG_L) {
    cpu->regs.a ^= *ReadReg(cpu, instr->param2);
  }
  else if (instr->param2 == PARA_MEM_REG_HL) {
    cpu->regs.a ^= BusRead(cpu->bus, ReadReg16(cpu, PARA_REG_HL));
  }
  else if (instr->param2 == PARA_IMM_8) {
    cpu->regs.a ^= BusRead(cpu->bus, cpu->pc++);
  }

  cpu->flags[FLAG_ZERO] = cpu->regs.a == 0;
  cpu->flags[FLAG_ADD_SUB] = 0;
  cpu->flags[FLAG_CARRY] = 0;
  cpu->flags[FLAG_HALF_CARRY] = 0;
  UpdateFlagsRegister(cpu);
}


static void Rotate(Cpu* const cpu, const Instruction* const instr,
                   ShiftDirection direction, int carry) {
  if (direction == LEFT) {
    if (instr->param1 == PARA_NONE) {
      // RL(C)A
      uint8_t msb = cpu->regs.a & 0x80;
      uint8_t lsb = carry ? cpu->flags[FLAG_CARRY] : msb;
      cpu->regs.a <<= 1;
      cpu->regs.a |= lsb;
      cpu->flags[FLAG_CARRY] = msb;
      cpu->flags[FLAG_ZERO] = 0;
    }
    else if (instr->param1 >= PARA_REG_A && instr->param1 <= PARA_REG_L) {
      // RL(C) r8
      uint8_t* reg = ReadReg(cpu, instr->param1);
      uint8_t msb = *reg & 0x80;
      uint8_t lsb = carry ? cpu->flags[FLAG_CARRY] : msb;
      *reg <<= 1;
      *reg |= lsb;
      cpu->flags[FLAG_CARRY] = msb;
      cpu->flags[FLAG_ZERO] = *reg == 0;
    }
    else if (instr->param1 == PARA_MEM_REG_HL) {
      // RL(C) [HL]
      uint16_t reg_hl = ReadReg16(cpu, PARA_REG_HL);
      uint8_t reg = BusRead(cpu->bus, reg_hl);
      uint8_t msb = reg & 0x80;
      uint8_t lsb = carry ? cpu->flags[FLAG_CARRY] : msb;
      reg <<= 1;
      reg |= lsb;
      cpu->flags[FLAG_CARRY] = msb;
      cpu->flags[FLAG_ZERO] = reg == 0;
      BusWrite(cpu->bus, reg_hl, reg);
    }
  }
  else {
    if (instr->param1 == PARA_NONE) {
      // RR(C)A
      uint8_t lsb = cpu->regs.a & 0x01;
      uint8_t msb = carry ? cpu->flags[FLAG_CARRY] : lsb;
      cpu->regs.a >>= 1;
      cpu->regs.a |= (msb << 7);
      cpu->flags[FLAG_CARRY] = lsb;
      cpu->flags[FLAG_ZERO] = 0;
    }
    else if (instr->param1 >= PARA_REG_A && instr->param1 <= PARA_REG_L) {
      // RR(C) r8
      uint8_t* reg = ReadReg(cpu, instr->param1);
      uint8_t lsb = *reg & 0x01;
      uint8_t msb = carry ? cpu->flags[FLAG_CARRY] : lsb;
      *reg >>= 1;
      *reg |= (msb << 7);
      cpu->flags[FLAG_CARRY] = lsb;
      cpu->flags[FLAG_ZERO] = *reg == 0;
    }
    else if (instr->param1 == PARA_MEM_REG_HL) {
      // RR(C) [HL]
      uint16_t reg_hl = ReadReg16(cpu, PARA_REG_HL);
      uint8_t reg = BusRead(cpu->bus, reg_hl);
      uint8_t lsb = reg & 0x01;
      uint8_t msb = carry ? cpu->flags[FLAG_CARRY] : lsb;
      reg >>= 1;
      reg |= (msb << 7);
      cpu->flags[FLAG_CARRY] = lsb;
      cpu->flags[FLAG_ZERO] = reg == 0;
      BusWrite(cpu->bus, reg_hl, reg);
    }
  }
  cpu->flags[FLAG_HALF_CARRY] = 0;
  cpu->flags[FLAG_ADD_SUB] = 0;
  UpdateFlagsRegister(cpu);
}


static void Shift(Cpu* const cpu, const Instruction* const instr,
                  ShiftDirection direction, int logically) {
  if (direction == LEFT) {
    if (instr->param1 >= PARA_REG_A && instr->param1 <= PARA_REG_L) {
      // SLA r8
      uint8_t* reg = ReadReg(cpu, instr->param1);
      uint8_t msb = *reg & 0x80;
      *reg <<= 1;
      cpu->flags[FLAG_CARRY] = msb;
      cpu->flags[FLAG_ZERO] = *reg == 0;
    }
    else if (instr->param1 == PARA_MEM_REG_HL) {
      // SLA [HL]
      uint16_t reg_hl = ReadReg16(cpu, PARA_REG_HL);
      uint8_t reg = BusRead(cpu->bus, reg_hl);
      uint8_t msb = reg & 0x80;
      reg <<= 1;
      cpu->flags[FLAG_CARRY] = msb;
      cpu->flags[FLAG_ZERO] = reg == 0;
      BusWrite(cpu->bus, reg_hl, reg);
    }
  }
  else {
    if (instr->param1 >= PARA_REG_A && instr->param1 <= PARA_REG_L) {
      // SR(A/L) r8
      uint8_t* reg = ReadReg(cpu, instr->param1);
      uint8_t msb = logically ? 0 : (*reg & 0x80);
      uint8_t lsb = *reg & 0x01;
      *reg >>= 1;
      *reg |= (msb << 7);
      cpu->flags[FLAG_CARRY] = lsb;
      cpu->flags[FLAG_ZERO] = *reg == 0;
    }
    else if (instr->param1 == PARA_MEM_REG_HL) {
      // SR(A/L) [HL]
      uint16_t reg_hl = ReadReg16(cpu, PARA_REG_HL);
      uint8_t reg = BusRead(cpu->bus, reg_hl);
      uint8_t msb = logically ? 0 : (reg & 0x80);
      uint8_t lsb = reg & 0x01;
      reg >>= 1;
      reg |= (msb << 7);
      cpu->flags[FLAG_CARRY] = lsb;
      cpu->flags[FLAG_ZERO] = reg == 0;
      BusWrite(cpu->bus, reg_hl, reg);
    }
  }
  cpu->flags[FLAG_HALF_CARRY] = 0;
  cpu->flags[FLAG_ADD_SUB] = 0;
  UpdateFlagsRegister(cpu);
}


static void Swap(Cpu* const cpu, const Instruction* const instr) {
  if (instr->param1 >= PARA_REG_A && instr->param1 <= PARA_REG_L) {
    uint8_t* reg = ReadReg(cpu, instr->param1);
    uint8_t hi = (*reg & 0xF0) >> 4;
    *reg <<= 4;
    *reg |= hi;

    cpu->flags[FLAG_ZERO] = *reg == 0;
  }
  else if (instr->param1 == PARA_MEM_REG_HL) {
    uint16_t reg_hl = ReadReg16(cpu, PARA_REG_HL);
    uint8_t reg = BusRead(cpu->bus, reg_hl);
    uint8_t hi = (reg & 0xF0) >> 4;
    reg <<= 4;
    reg |= hi;
    BusWrite(cpu->bus, reg_hl, reg);

    cpu->flags[FLAG_ZERO] = reg == 0;
  }

  cpu->flags[FLAG_ADD_SUB] = 0;
  cpu->flags[FLAG_CARRY] = 0;
  cpu->flags[FLAG_HALF_CARRY] = 0;
  UpdateFlagsRegister(cpu);
}


static void Bit(Cpu* const cpu, const Instruction* const instr) {
  uint8_t bit_idx = (instr->raw_instr & 0x38) >> 3;

  if (instr->param2 >= PARA_REG_A && instr->param2 <= PARA_REG_L) {
    uint8_t* reg = ReadReg(cpu, instr->param2);

    cpu->flags[FLAG_ZERO] = (*reg & (1 << bit_idx)) >> bit_idx;
  }
  else if (instr->param2 == PARA_MEM_REG_HL) {
    uint8_t reg = BusRead(cpu->bus, ReadReg16(cpu, PARA_REG_HL));

    cpu->flags[FLAG_ZERO] = (reg & (1 << bit_idx)) >> bit_idx;
  }

  cpu->flags[FLAG_ADD_SUB] = 0;
  cpu->flags[FLAG_HALF_CARRY] = 0;
  UpdateFlagsRegister(cpu);
}


static void Set(Cpu* const cpu, const Instruction* const instr) {
  uint8_t bit_idx = (instr->raw_instr & 0x38) >> 3;

  if (instr->param2 >= PARA_REG_A && instr->param2 <= PARA_REG_L) {
    uint8_t* reg = ReadReg(cpu, instr->param2);

    *reg |= (1 << bit_idx);
  }
  else if (instr->param2 == PARA_MEM_REG_HL) {
    uint16_t reg_hl = ReadReg16(cpu, PARA_REG_HL);
    uint8_t reg = BusRead(cpu->bus, reg_hl);

    reg |= (1 << bit_idx);
    BusWrite(cpu->bus, reg_hl, reg);
  }
}


static void Reset(Cpu* const cpu, const Instruction* const instr) {
  uint8_t bit_idx = (instr->raw_instr & 0x38) >> 3;

  if (instr->param2 >= PARA_REG_A && instr->param2 <= PARA_REG_L) {
    uint8_t* reg = ReadReg(cpu, instr->param2);

    *reg &= ~(1 << bit_idx);
  }
  else if (instr->param2 == PARA_MEM_REG_HL) {
    uint16_t reg_hl = ReadReg16(cpu, PARA_REG_HL);
    uint8_t reg = BusRead(cpu->bus, reg_hl);

    reg &= ~(1 << bit_idx);
    BusWrite(cpu->bus, reg_hl, reg);
  }
}


void CpuStep(Cpu* const cpu) {
  static uint8_t cb_prefix = 0;
  static uint8_t cpu_ime_enable = 0;
  uint8_t tmp = 0;

  // Check Interrupts.
  if (cpu_ime_enable) {
    cpu->interrupt_master_enable = 1;
    cpu_ime_enable = 0;
  }
  pthread_mutex_lock(cpu->global_ctx->interrupt_mtx);
  if (cpu->interrupt_master_enable &&
     (cpu->bus->interrupts_enable_reg & cpu->bus->interrupts_flag) != 0) {
    HandleInterrupt(cpu);
  }
  pthread_mutex_unlock(cpu->global_ctx->interrupt_mtx);

  // Fetch.
  uint8_t opcode = BusRead(cpu->bus, cpu->pc++);

  // Decode.
  Instruction instr;
  if (!cb_prefix) {
    instr = _INSTRUCTION_MAP[opcode];
  }
  else {
    instr = _CB_INSTRUCTION_MAP[opcode];
    cb_prefix = 0;
  }

  #ifdef GB_DEBUG_MODE
    //PrintCpuState(cpu);
    //PrintInstruction(&instr);
    //PrintSerialDebug(cpu);
  #endif

  // Execute.
  switch(instr.opcode) {
    case OP_NOOP:
      break;
    case OP_STOP:
      // Stop system and main clocks.
      cpu->global_ctx->status = STATUS_STOP;
      break;
    case OP_HALT:
      cpu->global_ctx->status = STATUS_HALT;
      Halt(cpu);
      cpu->global_ctx->status = STATUS_RUNNING;
      break;
    case OP_LD:
      Load(cpu, &instr);
      break;
    case OP_LDH:
      LoadH(cpu, &instr);
      break;
    case OP_INC:
      Increment(cpu, &instr);
      break;
    case OP_DEC:
      Decrement(cpu, &instr);
      break;
    case OP_PUSH:
      StackPush(cpu, &instr);
      break;
    case OP_POP:
      StackPop(cpu, &instr);
      break;
    case OP_JMP:
      Jump(cpu, &instr);
      break;
    case OP_JMPR:
      RelativeJump(cpu, &instr);
      break;
    case OP_CALL:
      Call(cpu, &instr);
      break;
    case OP_RET:
      Return(cpu, &instr);
      break;
    case OP_RETI:
      cpu_ime_enable = 1;
      Return(cpu, &instr);
      break;
    case OP_RST:
      Restart(cpu, &instr);
      break;
    case OP_DI:
      cpu_ime_enable = 0;
      cpu->interrupt_master_enable = 0;
      break;
    case OP_EI:
      cpu_ime_enable = 1;
      break;
    case OP_ADD:
      Add(cpu, &instr, /*carry=*/0);
      break;
    case OP_ADDC:
      Add(cpu, &instr, /*carry=*/1);
      break;
    case OP_SUB:
      Subtract(cpu, &instr, /*carry=*/0);
      break;
    case OP_SUBC:
      Subtract(cpu, &instr, /*carry=*/1);
      break;
    case OP_AND:
      BitwiseAnd(cpu, &instr);
      break;
    case OP_OR:
      BitwiseOr(cpu, &instr);
      break;
    case OP_XOR:
      BitwisXor(cpu, &instr);
      break;
    case OP_CMP:
      tmp = cpu->regs.a ;
      Subtract(cpu, &instr, /*carry=*/0);
      cpu->regs.a = tmp;
      break;
    case OP_CCF:
      cpu->flags[FLAG_CARRY] = !cpu->flags[FLAG_CARRY];
      cpu->flags[FLAG_ADD_SUB] = 0;
      cpu->flags[FLAG_HALF_CARRY] = 0;
      break;
    case OP_SCF:
      cpu->flags[FLAG_CARRY] = 1;
      cpu->flags[FLAG_ADD_SUB] = 0;
      cpu->flags[FLAG_HALF_CARRY] = 0;
      break;
    case OP_DAA:
      DecimalAdjustAccumulator(cpu);
      break;
    case OP_CPL:
      cpu->regs.a = ~cpu->regs.a ;
      cpu->flags[FLAG_ADD_SUB] = 1;
      cpu->flags[FLAG_HALF_CARRY] = 1;
      break;
    case OP_RLCA:
      Rotate(cpu, &instr, LEFT, /*carry=*/0);
      break;
    case OP_RLA:
      Rotate(cpu, &instr, LEFT, /*carry=*/1);
      break;
    case OP_RRCA:
      Rotate(cpu, &instr, RIGHT, /*carry=*/0);
      break;
    case OP_RRA:
      Rotate(cpu, &instr, RIGHT, /*carry=*/1);
      break;
    case OP_CB:
      cb_prefix = 1;
      break;
    case OP_RLC:
      Rotate(cpu, &instr, LEFT, /*carry=*/0);
      break;
    case OP_RRC:
      Rotate(cpu, &instr, RIGHT, /*carry=*/0);
      break;
    case OP_RL:
      Rotate(cpu, &instr, LEFT, /*carry=*/1);
      break;
    case OP_RR:
      Rotate(cpu, &instr, RIGHT, /*carry=*/1);
      break;
    case OP_SLA:
      Shift(cpu, &instr, LEFT, /*logically=*/0);
      break;
    case OP_SRA:
      Shift(cpu, &instr, RIGHT, /*logically=*/0);
      break;
    case OP_SWAP:
      Swap(cpu, &instr);
      break;
    case OP_SRL:
      Shift(cpu, &instr, RIGHT, /*logically=*/1);
      break;
    case OP_BIT:
      Bit(cpu, &instr);
      break;
    case OP_SET:
      Set(cpu, &instr);
      break;
    case OP_RES:
      Reset(cpu, &instr);
      break;
    case OP_ILLEGAL:
      cpu->global_ctx->error = ILLEGAL_INSTRUCTION;
      break;
    default:
      __builtin_unreachable();
  }
  cpu->global_ctx->clock += instr.cycles;
  int machine_cycles = instr.cycles / 4;
  pthread_mutex_lock(cpu->global_ctx->interrupt_mtx);
  for (int i = 0; i < machine_cycles; ++i) {
    TimerTick(&cpu->bus->timer, &cpu->bus->interrupts_flag);
  }
  pthread_mutex_unlock(cpu->global_ctx->interrupt_mtx);
}
