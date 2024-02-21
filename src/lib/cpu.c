#include "cpu.h"

#include "instruction.h"

#include "stdlib.h"
#include "stdint.h"
#include "stdio.h"


#define MostSigByte_(bits16) (uint8_t)((bits16 & 0xFF00) >> 8)
#define LeastSigByte_(bits16) (uint8_t)(bits16 & 0x00FF)
#define CombineBytes_(hi, lo) (uint16_t)(((uint16_t)hi << 8) | (uint16_t)lo)


void CpuInit(Cpu* const cpu) {
  cpu->bus_ = NULL;
  cpu->global_ctx_ = NULL;

  cpu->regs_.a_ = 0x11;
  cpu->regs_.b_ = 0x00;
  cpu->regs_.c_ = 0x00;
  cpu->regs_.d_ = 0xFF;
  cpu->regs_.e_ = 0x56;
  cpu->regs_.h_ = 0x00;
  cpu->regs_.l_ = 0x0D;
  cpu->flags_[FLAG_CARRY] = 0;
  cpu->flags_[FLAG_HALF_CARRY] = 0;
  cpu->flags_[FLAG_ADD_SUB] = 0;
  cpu->flags_[FLAG_ZERO] = 1;
  cpu->pc_ = 0x0100;
  cpu->sp_ = 0xFFFE;
  cpu->interrupt_master_enable_ = 0;
}


static void PrintInstruction(const Instruction* const instr) {
  printf("Raw binary: 0x%02x\n", instr->raw_instr_);
  printf("%s %s %s %s\n\n", _INSTRUCTION_STR_MAP[instr->opcode_],
                            _INSTRUCTION_PARAM_STR_MAP[instr->param1_],
                            _INSTRUCTION_PARAM_STR_MAP[instr->param2_],
                            _INSTRUCTION_COND_STR_MAP[instr->cond_]);
  printf("-------------------------------------------------\n\n");
}


static void PrintCpuState(const Cpu* const cpu) {
  printf("Registers:\n");
  printf("\tA: 0x%02x | %d\n", cpu->regs_.a_, cpu->regs_.a_);
  printf("\tB: 0x%02x | %d\n", cpu->regs_.b_, cpu->regs_.b_);
  printf("\tC: 0x%02x | %d\n", cpu->regs_.c_, cpu->regs_.c_);
  printf("\tD: 0x%02x | %d\n", cpu->regs_.d_, cpu->regs_.d_);
  printf("\tE: 0x%02x | %d\n", cpu->regs_.e_, cpu->regs_.e_);
  printf("\tH: 0x%02x | %d\n", cpu->regs_.h_, cpu->regs_.h_);
  printf("\tL: 0x%02x | %d\n", cpu->regs_.l_, cpu->regs_.l_);
  printf("\tAF: 0x%04x | %d\n", CombineBytes_(cpu->regs_.a_, cpu->regs_.f_),
                              CombineBytes_(cpu->regs_.a_, cpu->regs_.f_));
  printf("\tBC: 0x%04x | %d\n", CombineBytes_(cpu->regs_.b_, cpu->regs_.c_),
                              CombineBytes_(cpu->regs_.b_, cpu->regs_.c_));
  printf("\tDE: 0x%04x | %d\n", CombineBytes_(cpu->regs_.d_, cpu->regs_.e_),
                              CombineBytes_(cpu->regs_.d_, cpu->regs_.e_));
  printf("\tHL: 0x%04x | %d\n", CombineBytes_(cpu->regs_.h_, cpu->regs_.l_),
                              CombineBytes_(cpu->regs_.h_, cpu->regs_.l_));
  printf("Flags:\n");
  printf("\tZ: %d N: %d H: %d C: %d\n", cpu->flags_[FLAG_ZERO],
                                      cpu->flags_[FLAG_ADD_SUB],
                                      cpu->flags_[FLAG_HALF_CARRY],
                                      cpu->flags_[FLAG_CARRY]);
  printf("Program Counter:\n");
  printf("\t0x%04x | %d\n", cpu->pc_, cpu->pc_);
  printf("Stack Pointer:\n");
  printf("\t0x%04x | %d\n", cpu->sp_, cpu->sp_);
  printf("Master Interrupt Enable:\n");
  printf("\t%s\n\n", cpu->interrupt_master_enable_ ? "Enabled" : "Disabled");
}


typedef enum ShiftDirectionDef {
  RIGHT,
  LEFT
} ShiftDirection;


static uint8_t* ReadReg(Cpu* const cpu, InstructionParameter reg) {
  switch(reg) {
    case PARA_REG_A:
      return &cpu->regs_.a_;
    case PARA_REG_B:
      return &cpu->regs_.b_;
    case PARA_REG_C:
      return &cpu->regs_.c_;
    case PARA_REG_D:
      return &cpu->regs_.d_;
    case PARA_REG_E:
      return &cpu->regs_.e_;
    case PARA_REG_H:
      return &cpu->regs_.h_;
    case PARA_REG_L:
      return &cpu->regs_.l_;
    default:
      __builtin_unreachable();
  }
}


static uint16_t ReadReg16(const Cpu* const cpu, InstructionParameter reg) {
  switch(reg) {
    case PARA_REG_AF:
      return CombineBytes_(cpu->regs_.a_, cpu->regs_.f_);
    case PARA_REG_BC:
      return CombineBytes_(cpu->regs_.b_, cpu->regs_.c_);
    case PARA_REG_DE:
      return CombineBytes_(cpu->regs_.d_, cpu->regs_.e_);
    case PARA_REG_HL:
      return CombineBytes_(cpu->regs_.h_, cpu->regs_.l_);
    case PARA_SP:
      return cpu->sp_;
    default:
      __builtin_unreachable();
  }
}


static void WriteReg16(Cpu* const cpu, InstructionParameter reg,
                       uint16_t data) {
  switch(reg) {
    case PARA_REG_AF:
      cpu->regs_.a_ = data >> 8;
      cpu->regs_.f_ = data;
      break;
    case PARA_REG_BC:
      cpu->regs_.b_ = data >> 8;
      cpu->regs_.c_ = data;
      break;
    case PARA_REG_DE:
      cpu->regs_.d_ = data >> 8;
      cpu->regs_.e_ = data;
      break;
    case PARA_REG_HL:
      cpu->regs_.h_ = data >> 8;
      cpu->regs_.l_ = data;
      break;
    default:
      break;
  } 
}


static uint16_t ReadImm16(Cpu* const cpu) {
  // Memory is little endian.
  uint16_t lo = (uint16_t)BusRead(cpu->bus_, cpu->pc_++);
  uint16_t hi = (uint16_t)BusRead(cpu->bus_, cpu->pc_++);
  return CombineBytes_(hi, lo);
}


static void WriteImm16(const Cpu* const cpu, uint16_t addr, uint16_t data) {
  // Memory is little endian.
  uint8_t hi = MostSigByte_(data);
  uint8_t lo = LeastSigByte_(data);
  BusWrite(cpu->bus_, addr, lo);
  BusWrite(cpu->bus_, addr + 1, hi);
}


static void LoadReg(Cpu* const cpu, const Instruction* const instr) {
  uint8_t source = 0;
  int8_t offset = 0;
  uint16_t source_16 = 0;
  uint16_t reg_hl = 0;

  // Source is from an 8 bit register.
  if (instr->param2_ >= PARA_REG_A && instr->param2_ <= PARA_REG_L) {
    source = *ReadReg(cpu, instr->param2_);
  }
  // Source is from memory[16 bit register].
  else if (instr->param2_ >= PARA_MEM_REG_BC && instr->param2_ <= PARA_MEM_REG_HL) {
    source = BusRead(cpu->bus_, ReadReg16(cpu, instr->param2_));
  }
  // Other sources.
  else {
    switch(instr->param2_) {
      case PARA_SP_IMM_8:
        offset = (int8_t)BusRead(cpu->bus_, cpu->pc_++);
        source_16 = (uint16_t)((int16_t)cpu->sp_ + offset);
        break;
      case PARA_MEM_REG_HL_INC:
        reg_hl = ReadReg16(cpu, PARA_REG_HL);
        source = BusRead(cpu->bus_, reg_hl);
        WriteReg16(cpu, PARA_REG_HL, reg_hl + 1);
        break;
      case PARA_MEM_REG_HL_DEC:
        reg_hl = ReadReg16(cpu, PARA_REG_HL);
        source = BusRead(cpu->bus_, reg_hl);
        WriteReg16(cpu, PARA_REG_HL, reg_hl - 1);
        break;
      case PARA_IMM_8:
        source = BusRead(cpu->bus_, cpu->pc_);
        cpu->pc_++;
        break;
      case PARA_IMM_16:
        source_16 = ReadImm16(cpu);
        break;
      case PARA_ADDR:
        source_16 = ReadImm16(cpu);
        source = BusRead(cpu->bus_, source_16);
        break;
      default:
        cpu->global_ctx_->error = ILLEGAL_INSTRUCTION_PARAMETER;
        return;
    }
  }

  // Destination is an 8 bit register.
  if (instr->param1_ >= PARA_REG_A && instr->param1_ <= PARA_REG_L) {
    *ReadReg(cpu, instr->param1_) = source;
  }
  // Destination is a 16 bit register.
  if (instr->param1_ >= PARA_REG_BC && instr->param1_ <= PARA_REG_HL) {
    WriteReg16(cpu, instr->param1_, source_16);
  }
}


static void LoadSP(Cpu* const cpu, const Instruction* const instr) {
  switch(instr->param2_) {
    case PARA_IMM_16:
      cpu->sp_ = ReadImm16(cpu);
      break;
    case PARA_REG_HL:
      cpu->sp_ = ReadReg16(cpu, PARA_REG_HL);
      break;
    default:
      cpu->global_ctx_->error = ILLEGAL_INSTRUCTION_PARAMETER;
      return;
  }
}


static void LoadMem(Cpu* const cpu, const Instruction* const instr) {
  uint8_t source = 0;
  uint16_t source_16 = 0;
  uint16_t reg_hl = 0;

  if (instr->param2_ >= PARA_REG_A && instr->param2_ <= PARA_REG_L) {
    source = *ReadReg(cpu, instr->param2_);
  }
  else {
    switch(instr->param2_) {
      case PARA_SP:
        source_16 = cpu->sp_;
        break;
      case PARA_IMM_8:
        source = BusRead(cpu->bus_, cpu->pc_++);
        break;
      default:
        cpu->global_ctx_->error = ILLEGAL_INSTRUCTION_PARAMETER;
        return;
    }
  }

  if (instr->param1_ >= PARA_MEM_REG_BC && instr->param1_ <= PARA_MEM_REG_HL) {
    BusWrite(cpu->bus_, ReadReg16(cpu, instr->param1_), source);
  }
  else {
    switch (instr->param1_) {
      case PARA_MEM_REG_HL_INC:
        reg_hl = ReadReg16(cpu, PARA_REG_HL);
        BusWrite(cpu->bus_, reg_hl, source);
        WriteReg16(cpu, PARA_REG_HL, reg_hl + 1);
        break;
      case PARA_MEM_REG_HL_DEC:
        reg_hl = ReadReg16(cpu, PARA_REG_HL);
        BusWrite(cpu->bus_, reg_hl, source);
        WriteReg16(cpu, PARA_REG_HL, reg_hl - 1);
        break;
      case PARA_ADDR:
        WriteImm16(cpu, ReadImm16(cpu), source_16);
        break;
      default:
        cpu->global_ctx_->error = ILLEGAL_INSTRUCTION_PARAMETER;
        return;
    }
  }
}


static void Load(Cpu* const cpu, const Instruction* const instr) {
  InstructionParameter destination = instr->param1_;
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

  switch(instr->param2_) {
    case PARA_REG_A:
      source = cpu->regs_.a_;
      break;
    case PARA_MEM_REG_C:
      source = BusRead(cpu->bus_, 0xFF00 + (uint16_t)cpu->regs_.c_);
      break;
    case PARA_IMM_8:
      source = BusRead(cpu->bus_, cpu->pc_++);
      break;
    default:
      cpu->global_ctx_->error = ILLEGAL_INSTRUCTION_PARAMETER;
      return;
  }

  switch(instr->param1_) {
    case PARA_REG_A:
      cpu->regs_.a_ = source;
      break;
    case PARA_MEM_REG_C:
      BusWrite(cpu->bus_, 0xFF00 + (uint16_t)cpu->regs_.c_, source);
      break;
    case PARA_IMM_8:
      dest = BusRead(cpu->bus_, cpu->pc_++);
      BusWrite(cpu->bus_, 0xFF00 + (uint16_t)dest, source);
      break;
    default:
      cpu->global_ctx_->error = ILLEGAL_INSTRUCTION_PARAMETER;
      return;
  }
}


static void Increment(Cpu* const cpu, const Instruction* const instr) {
  uint8_t result = 0;
  int is_8_bit = 0;
  uint16_t reg_hl = 0;

  if (instr->param1_ >= PARA_REG_A && instr->param1_ <= PARA_REG_L) {
    result = ++(*ReadReg(cpu, instr->param2_));
    is_8_bit = 1;
  }
  else if (instr->param1_ >= PARA_REG_BC && instr->param1_ <= PARA_REG_HL) {
    WriteReg16(cpu, PARA_REG_BC, ReadReg16(cpu, PARA_REG_BC) + 1);
  }
  else {
    switch(instr->param1_) {
      case PARA_SP:
        ++cpu->sp_;
        break;
      case PARA_MEM_REG_HL:
        reg_hl = ReadReg16(cpu, PARA_REG_HL);
        result = BusRead(cpu->bus_, reg_hl);
        BusWrite(cpu->bus_, reg_hl, ++result);
        is_8_bit = 1;
        break;
      default:
        cpu->global_ctx_->error = ILLEGAL_INSTRUCTION_PARAMETER;
        return;
    }
  }

  if (is_8_bit) {
    cpu->flags_[FLAG_ADD_SUB] = 0;
    cpu->flags_[FLAG_ZERO] = (result == 0);
    cpu->flags_[FLAG_HALF_CARRY] = (result & 0x0F) == 0x00;
  }
}


static void Decrement(Cpu* const cpu, const Instruction* const instr) {
  uint8_t result = 0;
  int is_8_bit = 0;
  uint16_t reg_hl = 0;

  if (instr->param1_ >= PARA_REG_A && instr->param1_ <= PARA_REG_L) {
    result = --(*ReadReg(cpu, instr->param2_));
    is_8_bit = 1;
  }
  else if (instr->param1_ >= PARA_REG_BC && instr->param1_ <= PARA_REG_HL) {
    WriteReg16(cpu, PARA_REG_BC, ReadReg16(cpu, PARA_REG_BC) - 1);
  }
  else {
    switch(instr->param1_) {
      case PARA_SP:
        --cpu->sp_;
        break;
      case PARA_MEM_REG_HL:
        reg_hl = ReadReg16(cpu, PARA_REG_HL);
        result = BusRead(cpu->bus_, reg_hl);
        BusWrite(cpu->bus_, reg_hl, --result);
        is_8_bit = 1;
        break;
      default:
        cpu->global_ctx_->error = ILLEGAL_INSTRUCTION_PARAMETER;
        return;
    }
  }

  if (is_8_bit) {
    cpu->flags_[FLAG_ADD_SUB] = 1;
    cpu->flags_[FLAG_ZERO] = (result == 0);
    cpu->flags_[FLAG_HALF_CARRY] = (result & 0x0F) == 0x0F;
  }
}


static void StackPush(Cpu* const cpu, const Instruction* const instr) {
  uint16_t source = ReadReg16(cpu, instr->param1_);
  uint8_t hi = MostSigByte_(source);
  uint8_t lo = LeastSigByte_(source);

  // The stack grows up, as in the address decreases as things are added
  // to the stack. This is why WriteImm16() cannot be used.
  BusWrite(cpu->bus_, --cpu->sp_, hi);
  BusWrite(cpu->bus_, --cpu->sp_, lo);
}


static void StackPop(Cpu* const cpu, const Instruction* const instr) {
  uint16_t lo = (uint16_t)BusRead(cpu->bus_, cpu->sp_++);
  uint16_t hi = (uint16_t)BusRead(cpu->bus_, cpu->sp_++);
  uint16_t source = CombineBytes_(hi, lo);

  WriteReg16(cpu, instr->param1_, source);
}


static void Jump(Cpu* const cpu, const Instruction* const instr) {
  if (instr->param1_ == PARA_REG_HL) {
    cpu->pc_ = ReadReg16(cpu, PARA_REG_HL);
    return;
  }

  // Even though it's less efficient, the next bytes need to be read to
  // keep the Pragram Coounter accurate.
  uint16_t jmp_addr = ReadImm16(cpu);
  int condition = 0;

  switch(instr->cond_) {
    case COND_NONE:
      condition = 1;
      break;
    case COND_NZ:
      condition = !cpu->flags_[FLAG_ZERO];
      break;
    case COND_Z:
      condition = cpu->flags_[FLAG_ZERO];
      break;
    case COND_NC:
      condition = !cpu->flags_[FLAG_CARRY];
      break;
    case COND_C:
      condition = cpu->flags_[FLAG_CARRY];
      break;
    default:
      cpu->global_ctx_->error = ILLEGAL_INSTRUCTION_PARAMETER;
      return;
  }

  if (condition) {
    cpu->pc_ = jmp_addr;
    if (instr->cond_ != COND_NONE) {
      cpu->global_ctx_->clock += 4;
    }
  }
}


static void RelativeJump(Cpu* const cpu, const Instruction* const instr) {
  int8_t offset = (int8_t)BusRead(cpu->bus_, cpu->pc_++);
  uint16_t jmp_addr = (uint16_t)((int16_t)cpu->pc_ + (int16_t)offset);
  int condition = 0;

  switch(instr->cond_) {
    case COND_NONE:
      condition = 1;
      break;
    case COND_NZ:
      condition = !cpu->flags_[FLAG_ZERO];
      break;
    case COND_Z:
      condition = cpu->flags_[FLAG_ZERO];
      break;
    case COND_NC:
      condition = !cpu->flags_[FLAG_CARRY];
      break;
    case COND_C:
      condition = cpu->flags_[FLAG_CARRY];
      break;
  }

  if (condition) {
    cpu->pc_ = jmp_addr;
    if (instr->cond_ != COND_NONE) {
      cpu->global_ctx_->clock += 4;
    }
  }
}


static void Call(Cpu* const cpu, const Instruction* const instr) {
  uint16_t call_addr = ReadImm16(cpu);
  int condition = 0;

  switch(instr->cond_) {
    case COND_NONE:
      condition = 1;
      break;
    case COND_NZ:
      condition = !cpu->flags_[FLAG_ZERO];
      break;
    case COND_Z:
      condition = cpu->flags_[FLAG_ZERO];
      break;
    case COND_NC:
      condition = !cpu->flags_[FLAG_CARRY];
      break;
    case COND_C:
      condition = cpu->flags_[FLAG_CARRY];
      break;
  }

  if (condition) {
    uint8_t hi = MostSigByte_(cpu->pc_);
    uint8_t lo = LeastSigByte_(cpu->pc_);
    BusWrite(cpu->bus_, --cpu->sp_, hi);
    BusWrite(cpu->bus_, --cpu->sp_, lo);
    cpu->pc_ = call_addr;
    if (instr->cond_ != COND_NONE) {
      cpu->global_ctx_->clock += 12;
    }
  }
}


static void Return(Cpu* const cpu, const Instruction* const instr) {
  int condition = 0;

  switch(instr->cond_) {
    case COND_NONE:
      condition = 1;
      break;
    case COND_NZ:
      condition = !cpu->flags_[FLAG_ZERO];
      break;
    case COND_Z:
      condition = cpu->flags_[FLAG_ZERO];
      break;
    case COND_NC:
      condition = !cpu->flags_[FLAG_CARRY];
      break;
    case COND_C:
      condition = cpu->flags_[FLAG_CARRY];
      break;
  }

  if (condition) {
    uint16_t lo = (uint16_t)BusRead(cpu->bus_, cpu->sp_++);
    uint16_t hi = (uint16_t)BusRead(cpu->bus_, cpu->sp_++);
    cpu->pc_ = CombineBytes_(hi, lo);
    if (instr->cond_ != COND_NONE) {
      cpu->global_ctx_->clock += 12;
    }
  }
}


static void Restart(Cpu* const cpu, const Instruction* const instr) {
  uint8_t target = (instr->raw_instr_ & 0x38) >> 3;
  target *= 8;

  uint8_t hi = MostSigByte_(cpu->pc_);
  uint8_t lo = LeastSigByte_(cpu->pc_);
  BusWrite(cpu->bus_, --cpu->sp_, hi);
  BusWrite(cpu->bus_, --cpu->sp_, lo);
  cpu->pc_ = (uint16_t)target;
}


static void Add(Cpu* const cpu, const Instruction* const instr, int carry) {
  uint32_t initial = 0;

  switch(instr->param1_) {
    case PARA_REG_A:
      initial = (uint32_t)cpu->regs_.a_;
      if (instr->param2_ == PARA_MEM_REG_HL) {
        cpu->regs_.a_ += BusRead(cpu->bus_, ReadReg16(cpu, PARA_REG_HL));
      }
      else if (instr->param2_ == PARA_IMM_8) {
        cpu->regs_.a_ += BusRead(cpu->bus_, cpu->pc_++);
      }
      else {
        cpu->regs_.a_ += *ReadReg(cpu, instr->param2_);
      }
      if (carry) {
        cpu->regs_.a_ += cpu->flags_[FLAG_CARRY];
      }
      cpu->flags_[FLAG_ZERO] = cpu->regs_.a_ == 0;
      cpu->flags_[FLAG_CARRY] = cpu->regs_.a_ < initial;
      cpu->flags_[FLAG_HALF_CARRY] = (cpu->regs_.a_ & 0x0F) > 0x0F;
      break;

    case PARA_REG_HL:
      initial = (uint32_t)ReadReg16(cpu, PARA_REG_HL) +
                (uint32_t)ReadReg16(cpu, instr->param2_);
      WriteReg16(cpu, PARA_REG_HL, (uint16_t)initial);

      cpu->flags_[FLAG_CARRY] = initial > 0xFFFF;
      cpu->flags_[FLAG_HALF_CARRY] = (initial & 0x0F) > 0x0F;
      break;

    case PARA_SP:
      initial = (uint32_t)((int16_t)cpu->sp_ + (int8_t)BusRead(cpu->bus_,
                                                                cpu->pc_++));
      cpu->sp_ = (uint16_t)initial;
      cpu->flags_[FLAG_ZERO] = 0;
      cpu->flags_[FLAG_CARRY] = initial > 0xFFFF;
      cpu->flags_[FLAG_HALF_CARRY] = (initial & 0x0F) > 0x0F;
      break;

    default:
      cpu->global_ctx_->error = ILLEGAL_INSTRUCTION_PARAMETER;
      return;
  }
  cpu->flags_[FLAG_ADD_SUB] = 0;
}


static void Subtract(Cpu* const cpu, const Instruction* const instr,
                     int carry) {
  uint8_t initial = cpu->regs_.a_;

  if (instr->param2_ >= PARA_REG_A && instr->param2_ <= PARA_REG_L) {
    cpu->regs_.a_ -= *ReadReg(cpu, instr->param2_);
  }
  else if (instr->param2_ == PARA_MEM_REG_HL) {
    cpu->regs_.a_ -= BusRead(cpu->bus_, ReadReg16(cpu, PARA_MEM_REG_HL));
  }
  else if (instr->param2_ == PARA_IMM_8) {
    cpu->regs_.a_ -= BusRead(cpu->bus_, cpu->pc_++);
  }
  if (carry) {
    cpu->regs_.a_ -= cpu->flags_[FLAG_CARRY];
  }

  cpu->flags_[FLAG_ZERO] = cpu->regs_.a_ == 0;
  cpu->flags_[FLAG_ADD_SUB] = 1;
  cpu->flags_[FLAG_CARRY] = (instr->param2_ == PARA_REG_A) && carry
                            ? cpu->flags_[FLAG_CARRY]
                            : (initial < cpu->regs_.a_);
  cpu->flags_[FLAG_HALF_CARRY] = (cpu->regs_.a_ & 0x0F) == 0x0F;
}


static void DecimalAdjustAccumulator(Cpu* const cpu) {
  uint8_t hi = (cpu->regs_.a_ & 0xF0) >> 4;
  uint8_t lo = (cpu->regs_.a_ & 0x0F);

  if (hi > 9) {
    hi = 9;
  }
  if (lo > 9) {
    lo = 9;
  }
  cpu->regs_.a_ = (hi * 10) + lo;

  cpu->flags_[FLAG_ZERO] = cpu->regs_.a_ == 0;
  cpu->flags_[FLAG_HALF_CARRY] = 0;
  cpu->flags_[FLAG_CARRY] = (cpu->regs_.a_ & 0x0F) > 0x0F;
}


static void BitwiseAnd(Cpu* const cpu, const Instruction* const instr) {
  if (instr->param2_ >= PARA_REG_A && instr->param2_ <= PARA_REG_L) {
    cpu->regs_.a_ &= *ReadReg(cpu, instr->param2_);
  }
  else if (instr->param2_ == PARA_MEM_REG_HL) {
    cpu->regs_.a_ &= BusRead(cpu->bus_, ReadReg16(cpu, PARA_MEM_REG_HL));
  }
  else if (instr->param2_ == PARA_IMM_8) {
    cpu->regs_.a_ &= BusRead(cpu->bus_, cpu->pc_++);
  }

  cpu->flags_[FLAG_ZERO] = cpu->regs_.a_ == 0;
  cpu->flags_[FLAG_ADD_SUB] = 0;
  cpu->flags_[FLAG_CARRY] = 0;
  cpu->flags_[FLAG_HALF_CARRY] = 1;
}


static void BitwiseOr(Cpu* const cpu, const Instruction* const instr) {
  if (instr->param2_ >= PARA_REG_A && instr->param2_ <= PARA_REG_L) {
    cpu->regs_.a_ |= *ReadReg(cpu, instr->param2_);
  }
  else if (instr->param2_ == PARA_MEM_REG_HL) {
    cpu->regs_.a_ |= BusRead(cpu->bus_, ReadReg16(cpu, PARA_MEM_REG_HL));
  }
  else if (instr->param2_ == PARA_IMM_8) {
    cpu->regs_.a_ |= BusRead(cpu->bus_, cpu->pc_++);
  }

  cpu->flags_[FLAG_ZERO] = cpu->regs_.a_ == 0;
  cpu->flags_[FLAG_ADD_SUB] = 0;
  cpu->flags_[FLAG_CARRY] = 0;
  cpu->flags_[FLAG_HALF_CARRY] = 0;
}


static void BitwisXor(Cpu* const cpu, const Instruction* const instr) {
  if (instr->param2_ >= PARA_REG_A && instr->param2_ <= PARA_REG_L) {
    cpu->regs_.a_ ^= *ReadReg(cpu, instr->param2_);
  }
  else if (instr->param2_ == PARA_MEM_REG_HL) {
    cpu->regs_.a_ ^= BusRead(cpu->bus_, ReadReg16(cpu, PARA_MEM_REG_HL));
  }
  else if (instr->param2_ == PARA_IMM_8) {
    cpu->regs_.a_ ^= BusRead(cpu->bus_, cpu->pc_++);
  }

  cpu->flags_[FLAG_ZERO] = cpu->regs_.a_ == 0;
  cpu->flags_[FLAG_ADD_SUB] = 0;
  cpu->flags_[FLAG_CARRY] = 0;
  cpu->flags_[FLAG_HALF_CARRY] = 0;
}


static void Rotate(Cpu* const cpu, const Instruction* const instr,
                   ShiftDirection direction, int carry) {
  if (direction == LEFT) {
    if (instr->param1_ == PARA_NONE) {
      // RL(C)A
      uint8_t msb = cpu->regs_.a_ & 0x80;
      uint8_t lsb = carry ? cpu->flags_[FLAG_CARRY] : msb;
      cpu->regs_.a_ <<= 1;
      cpu->regs_.a_ |= lsb;
      cpu->flags_[FLAG_CARRY] = msb;
      cpu->flags_[FLAG_ZERO] = 0;
    }
    else if (instr->param1_ >= PARA_REG_A && instr->param1_ <= PARA_REG_L) {
      // RL(C) r8
      uint8_t* reg = ReadReg(cpu, instr->param1_);
      uint8_t msb = *reg & 0x80;
      uint8_t lsb = carry ? cpu->flags_[FLAG_CARRY] : msb;
      *reg <<= 1;
      *reg |= lsb;
      cpu->flags_[FLAG_CARRY] = msb;
      cpu->flags_[FLAG_ZERO] = *reg == 0;
    }
    else if (instr->param1_ == PARA_MEM_REG_HL) {
      // RL(C) [HL]
      uint16_t reg_hl = ReadReg16(cpu, PARA_REG_HL);
      uint8_t reg = BusRead(cpu->bus_, reg_hl);
      uint8_t msb = reg & 0x80;
      uint8_t lsb = carry ? cpu->flags_[FLAG_CARRY] : msb;
      reg <<= 1;
      reg |= lsb;
      cpu->flags_[FLAG_CARRY] = msb;
      cpu->flags_[FLAG_ZERO] = reg == 0;
      BusWrite(cpu->bus_, reg_hl, reg);
    }
  }
  else {
    if (instr->param1_ == PARA_NONE) {
      // RR(C)A
      uint8_t lsb = cpu->regs_.a_ & 0x01;
      uint8_t msb = carry ? cpu->flags_[FLAG_CARRY] : lsb;
      cpu->regs_.a_ >>= 1;
      cpu->regs_.a_ |= (msb << 7);
      cpu->flags_[FLAG_CARRY] = lsb;
      cpu->flags_[FLAG_ZERO] = 0;
    }
    else if (instr->param1_ >= PARA_REG_A && instr->param1_ <= PARA_REG_L) {
      // RR(C) r8
      uint8_t* reg = ReadReg(cpu, instr->param1_);
      uint8_t lsb = *reg & 0x01;
      uint8_t msb = carry ? cpu->flags_[FLAG_CARRY] : lsb;
      *reg >>= 1;
      *reg |= (msb << 7);
      cpu->flags_[FLAG_CARRY] = lsb;
      cpu->flags_[FLAG_ZERO] = *reg == 0;
    }
    else if (instr->param1_ == PARA_MEM_REG_HL) {
      // RR(C) [HL]
      uint16_t reg_hl = ReadReg16(cpu, PARA_REG_HL);
      uint8_t reg = BusRead(cpu->bus_, reg_hl);
      uint8_t lsb = reg & 0x01;
      uint8_t msb = carry ? cpu->flags_[FLAG_CARRY] : lsb;
      reg >>= 1;
      reg |= (msb << 7);
      cpu->flags_[FLAG_CARRY] = lsb;
      cpu->flags_[FLAG_ZERO] = reg == 0;
      BusWrite(cpu->bus_, reg_hl, reg);
    }
  }
  cpu->flags_[FLAG_HALF_CARRY] = 0;
  cpu->flags_[FLAG_ADD_SUB] = 0;
}


static void Shift(Cpu* const cpu, const Instruction* const instr,
                  ShiftDirection direction, int logically) {
  if (direction == LEFT) {
    if (instr->param1_ >= PARA_REG_A && instr->param1_ <= PARA_REG_L) {
      // SLA r8
      uint8_t* reg = ReadReg(cpu, instr->param1_);
      uint8_t msb = *reg & 0x80;
      *reg <<= 1;
      cpu->flags_[FLAG_CARRY] = msb;
      cpu->flags_[FLAG_ZERO] = *reg == 0;
    }
    else if (instr->param1_ == PARA_MEM_REG_HL) {
      // SLA [HL]
      uint16_t reg_hl = ReadReg16(cpu, PARA_REG_HL);
      uint8_t reg = BusRead(cpu->bus_, reg_hl);
      uint8_t msb = reg & 0x80;
      reg <<= 1;
      cpu->flags_[FLAG_CARRY] = msb;
      cpu->flags_[FLAG_ZERO] = reg == 0;
      BusWrite(cpu->bus_, reg_hl, reg);
    }
  }
  else {
    if (instr->param1_ >= PARA_REG_A && instr->param1_ <= PARA_REG_L) {
      // SR(A/L) r8
      uint8_t* reg = ReadReg(cpu, instr->param1_);
      uint8_t msb = logically ? 0 : (*reg & 0x80);
      uint8_t lsb = *reg & 0x01;
      *reg >>= 1;
      *reg |= (msb << 7);
      cpu->flags_[FLAG_CARRY] = lsb;
      cpu->flags_[FLAG_ZERO] = *reg == 0;
    }
    else if (instr->param1_ == PARA_MEM_REG_HL) {
      // SR(A/L) [HL]
      uint16_t reg_hl = ReadReg16(cpu, PARA_REG_HL);
      uint8_t reg = BusRead(cpu->bus_, reg_hl);
      uint8_t msb = logically ? 0 : (reg & 0x80);
      uint8_t lsb = reg & 0x01;
      reg >>= 1;
      reg |= (msb << 7);
      cpu->flags_[FLAG_CARRY] = lsb;
      cpu->flags_[FLAG_ZERO] = reg == 0;
      BusWrite(cpu->bus_, reg_hl, reg);
    }
  }
  cpu->flags_[FLAG_HALF_CARRY] = 0;
  cpu->flags_[FLAG_ADD_SUB] = 0;
}


static void Swap(Cpu* const cpu, const Instruction* const instr) {
  if (instr->param1_ >= PARA_REG_A && instr->param1_ <= PARA_REG_L) {
    uint8_t* reg = ReadReg(cpu, instr->param1_);
    uint8_t hi = (*reg & 0xF0) >> 4;
    *reg <<= 4;
    *reg |= hi;

    cpu->flags_[FLAG_ZERO] = *reg == 0;
  }
  else if (instr->param1_ == PARA_MEM_REG_HL) {
    uint16_t reg_hl = ReadReg16(cpu, PARA_REG_HL);
    uint8_t reg = BusRead(cpu->bus_, reg_hl);
    uint8_t hi = (reg & 0xF0) >> 4;
    reg <<= 4;
    reg |= hi;
    BusWrite(cpu->bus_, reg_hl, reg);

    cpu->flags_[FLAG_ZERO] = reg == 0;
  }

  cpu->flags_[FLAG_ADD_SUB] = 0;
  cpu->flags_[FLAG_CARRY] = 0;
  cpu->flags_[FLAG_HALF_CARRY] = 0;
}


static void Bit(Cpu* const cpu, const Instruction* const instr) {
  uint8_t bit_idx = (instr->raw_instr_ & 0x38) >> 3;

  if (instr->param2_ >= PARA_REG_A && instr->param2_ <= PARA_REG_L) {
    uint8_t* reg = ReadReg(cpu, instr->param2_);

    cpu->flags_[FLAG_ZERO] = (*reg & (1 << bit_idx)) >> bit_idx;
  }
  else if (instr->param2_ == PARA_MEM_REG_HL) {
    uint8_t reg = BusRead(cpu->bus_, ReadReg16(cpu, PARA_REG_HL));

    cpu->flags_[FLAG_ZERO] = (reg & (1 << bit_idx)) >> bit_idx;
  }

  cpu->flags_[FLAG_ADD_SUB] = 0;
  cpu->flags_[FLAG_HALF_CARRY] = 0;
}


static void Set(Cpu* const cpu, const Instruction* const instr) {
  uint8_t bit_idx = (instr->raw_instr_ & 0x38) >> 3;

  if (instr->param2_ >= PARA_REG_A && instr->param2_ <= PARA_REG_L) {
    uint8_t* reg = ReadReg(cpu, instr->param2_);

    *reg |= (1 << bit_idx);
  }
  else if (instr->param2_ == PARA_MEM_REG_HL) {
    uint16_t reg_hl = ReadReg16(cpu, PARA_REG_HL);
    uint8_t reg = BusRead(cpu->bus_, reg_hl);

    reg |= (1 << bit_idx);
    BusWrite(cpu->bus_, reg_hl, reg);
  }
}


static void Reset(Cpu* const cpu, const Instruction* const instr) {
  uint8_t bit_idx = (instr->raw_instr_ & 0x38) >> 3;

  if (instr->param2_ >= PARA_REG_A && instr->param2_ <= PARA_REG_L) {
    uint8_t* reg = ReadReg(cpu, instr->param2_);

    *reg &= ~(1 << bit_idx);
  }
  else if (instr->param2_ == PARA_MEM_REG_HL) {
    uint16_t reg_hl = ReadReg16(cpu, PARA_REG_HL);
    uint8_t reg = BusRead(cpu->bus_, reg_hl);

    reg &= ~(1 << bit_idx);
    BusWrite(cpu->bus_, reg_hl, reg);
  }
}


void CpuStep(Cpu* const cpu) {
  static uint8_t cb_prefix = 0;
  uint8_t tmp = 0;

  // Fetch.
  uint8_t opcode = BusRead(cpu->bus_, cpu->pc_++);

  // Decode.
  Instruction instr;
  if (!cb_prefix) {
    instr = _INSTRUCTION_MAP[opcode];
  }
  else {
    instr = _CB_INSTRUCTION_MAP[opcode];
    cb_prefix = 0;
  }

  PrintCpuState(cpu);
  PrintInstruction(&instr);

  // Execute.
  switch(instr.opcode_) {
    case OP_NOOP:
      break;
    case OP_STOP:
      // Stop system and main clocks.
      cpu->global_ctx_->status = STATUS_STOP;
      break;
    case OP_HALT:
      // Stop system clock.
      cpu->global_ctx_->status = STATUS_STOP;
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
      cpu->interrupt_master_enable_ = 1;
      Return(cpu, &instr);
      break;
    case OP_RST:
      Restart(cpu, &instr);
      break;
    case OP_DI:
      cpu->interrupt_master_enable_ = 0;
      break;
    case OP_EI:
      cpu->interrupt_master_enable_ = 1;
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
      tmp = cpu->regs_.a_;
      Subtract(cpu, &instr, /*carry=*/0);
      cpu->regs_.a_ = tmp;
      break;
    case OP_CCF:
      cpu->flags_[FLAG_CARRY] = !cpu->flags_[FLAG_CARRY];
      cpu->flags_[FLAG_ADD_SUB] = 0;
      cpu->flags_[FLAG_HALF_CARRY] = 0;
      break;
    case OP_SCF:
      cpu->flags_[FLAG_CARRY] = 1;
      cpu->flags_[FLAG_ADD_SUB] = 0;
      cpu->flags_[FLAG_HALF_CARRY] = 0;
      break;
    case OP_DAA:
      DecimalAdjustAccumulator(cpu);
      break;
    case OP_CPL:
      cpu->regs_.a_ = ~cpu->regs_.a_;
      cpu->flags_[FLAG_ADD_SUB] = 1;
      cpu->flags_[FLAG_HALF_CARRY] = 1;
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
      cpu->global_ctx_->error = ILLEGAL_INSTRUCTION;
      break;
    default:
      __builtin_unreachable();
  }
  cpu->global_ctx_->clock += instr.cycles_;
}
