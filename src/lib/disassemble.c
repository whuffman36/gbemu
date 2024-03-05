#include "disassemble.h"

#include "instruction.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void Disassemble(const char* filename) {
  FILE* read_file = fopen(filename, "rb");
  if (read_file == NULL) {
    printf("Failed to open file %s\n", filename);
    exit(1);
  }

  FILE* write_file = fopen("translate_output.txt", "w");
  if (write_file == NULL) {
    printf("Failed to open file output.txt\n");
    exit(1);
  }

  int byte;
  int lo, hi;
  int byte_count = 0;
  char instr_str[16] = {0};
  char param1[16] = {0};
  char param2[16] = {0};
  char cond[16] = {0};
  char opcode[16] = {0};
  char total_str[256] = {0};
  while((byte = fgetc(read_file)) != EOF) {
      Instruction instr = _INSTRUCTION_MAP[byte];
      strncpy(instr_str, _INSTRUCTION_STR_MAP[instr.opcode], 16);
      int curr_byte_count = byte_count;
      switch(instr.param1) {
        case PARA_IMM_8:
          byte = fgetc(read_file);
          byte_count++;
          snprintf(param1, 16, "0x%02x", byte);
          break;
        case PARA_IMM_16:
          lo = fgetc(read_file);
          hi = fgetc(read_file);
          byte_count += 2;
          byte = (hi << 8) | lo;
          snprintf(param1, 16, "0x%04x", byte);
          break;
        case PARA_ADDR:
          lo = fgetc(read_file);
          hi = fgetc(read_file);
          byte_count += 2;
          byte = (hi << 8) | lo;
          snprintf(param1, 16, "0x%04x", byte);
          break;
        default:
          strncpy(param1, _INSTRUCTION_PARAM_STR_MAP[instr.param1], 16);
          break;
      }

      switch(instr.param2) {
        case PARA_IMM_8:
          byte = fgetc(read_file);
          byte_count++;
          snprintf(param2, 16, "0x%02x", byte);
          break;
        case PARA_IMM_16:
          lo = fgetc(read_file);
          hi = fgetc(read_file);
          byte_count += 2;
          byte = (hi << 8) | lo;
          snprintf(param2, 16, "0x%04x", byte);
          break;
        case PARA_ADDR:
          lo = fgetc(read_file);
          hi = fgetc(read_file);
          byte_count += 2;
          byte = (hi << 8) | lo;
          snprintf(param2, 16, "0x%04x", byte);
          break;
        default:
          strncpy(param2, _INSTRUCTION_PARAM_STR_MAP[instr.param2], 16);
          break;
      }

      switch(instr.cond) {
        case COND_NONE:
          strncpy(cond, "", 16);
          break;
        default:
          strncpy(cond, _INSTRUCTION_COND_STR_MAP[instr.cond], 16);
          break;
      }

      snprintf(opcode, 16, "0x%02x ", instr.raw_instr);

      snprintf(total_str, 256, "0x%04x: %s %s %s %s %s\n", curr_byte_count, opcode, instr_str, param1, param2, cond);

      fputs(total_str, write_file);

      byte_count++;
  }
  fclose(read_file);
  fclose(write_file);
}
