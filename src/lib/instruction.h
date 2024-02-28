#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include "stdint.h"


typedef enum OpcodeDef {
  OP_NOOP,
  OP_STOP,
  OP_HALT,
  OP_LD,    // Load
  OP_LDH,   // Load Half Word
  OP_INC,   // Increment
  OP_DEC,   // Decrement
  OP_PUSH,  // Stack Push
  OP_POP,   // Stack Pop
  OP_JMP,   // Jump
  OP_JMPR,  // Relative Jump
  OP_CALL,  // Function Call
  OP_RET,   // Function Return
  OP_RETI,  // Interrupt Handler Return
  OP_RST,   // Restart/Function Call
  OP_DI,    // Disable Interrupts
  OP_EI,    // Enable Interrupts
  OP_ADD,   // Add
  OP_ADDC,  // Add With Carry
  OP_SUB,   // Subtract
  OP_SUBC,  // Subtract With Carry
  OP_AND,   // Bitwise AND
  OP_OR,    // Bitwise OR
  OP_XOR,   // Bitwise XOR
  OP_CMP,   // Compare
  OP_CCF,   // Complement Carry Flag
  OP_SCF,   // Set Carry Flag
  OP_DAA,   // Decimal Adjust Accumulator
  OP_CPL,   // Complement Accumulator
  OP_RLCA,  // Rotate Left Accumulator
  OP_RLA,   // Rotate Left Accumulator Through Carry
  OP_RRCA,  // Rotate Right Accumulator
  OP_RRA,   // Rotate Right Accumulator Through Carry
  OP_CB,    // CB Prefix
  OP_RLC,   // Rotate Left
  OP_RRC,   // Rotate Right
  OP_RL,    // Rotate Left Through Carry
  OP_RR,    // Rotate Right Through Carry
  OP_SLA,   // Arithmetic Shift Left
  OP_SRA,   // Arithmetic Shift Right
  OP_SWAP,  // Swap Low And High Nibble
  OP_SRL,   // Logical Shift Right
  OP_BIT,   // Test Bit
  OP_SET,   // Set Bit
  OP_RES,   // Reset Bit
  OP_ILLEGAL
} Opcode;

typedef enum InstructionParameterDef {
  PARA_NONE,
  PARA_REG_A,           // Register A
  PARA_REG_B,           // Register B
  PARA_REG_C,           // Register C
  PARA_REG_D,           // Register D
  PARA_REG_E,           // Register E
  PARA_REG_H,           // Register H
  PARA_REG_L,           // Register L
  PARA_REG_AF,          // Register AF
  PARA_REG_BC,          // Register BC
  PARA_REG_DE,          // Register DE
  PARA_REG_HL,          // Register HL
  PARA_SP,              // Stack Pointer
  PARA_SP_IMM_8,        // Stack Pointer + imm8
  PARA_MEM_REG_C,       // [C]
  PARA_MEM_REG_BC,      // [BC]
  PARA_MEM_REG_DE,      // [DE]
  PARA_MEM_REG_HL,      // [HL]
  PARA_MEM_REG_HL_INC,  // [HL++]
  PARA_MEM_REG_HL_DEC,  // [HL--]
  PARA_IMM_8,           // Immediate 8 bit value
  PARA_IMM_16,          // Immediate 16 bit value
  PARA_ADDR,            // 16 bit address
  PARA_BIT_IDX,         // Bit index
  PARA_TGT              // Jump Target
} InstructionParameter;

typedef enum ConditionDef {
  COND_NONE,
  COND_NZ,
  COND_Z,
  COND_NC,
  COND_C
} Condition;

typedef struct InstructionDef {
  Opcode opcode;
  InstructionParameter param1;
  InstructionParameter param2;
  Condition cond;
  uint8_t cycles;
  uint8_t raw_instr;
} Instruction;

extern const Instruction _INSTRUCTION_MAP[0x100];
extern const Instruction _CB_INSTRUCTION_MAP[0x100];

extern const char* _INSTRUCTION_STR_MAP[];
extern const char* _INSTRUCTION_PARAM_STR_MAP[];
extern const char* _INSTRUCTION_COND_STR_MAP[];

#endif
