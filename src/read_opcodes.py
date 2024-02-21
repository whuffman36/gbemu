import json

opcodes = open('opcodes.json', 'r')
json_data = json.load(opcodes)
opcodes.close()

conditions = ['NZ', 'Z', 'NC']
registers = ['A', 'B', 'C', 'D', 'E', 'H', 'L', 'AF', 'BC', 'DE', 'HL']
bit_idx = ['1', '2', '3', '4', '5', '6', '7', '0']
imm_8 = ['n8', 'a8', 'e8']
imm_16 = ['n16', 'a16']

cb_prefix = True

if cb_prefix is True:
  filename = 'lib/cb_instruction.c'
  json_tag = 'cbprefixed'
  instr_map = 'const Instruction _CB_INSTRUCTION_MAP[0x100] = {\n\n'
else:
  filename = 'lib/instruction.c'
  json_tag = 'unprefixed'
  instr_map = 'const Instruction _INSTRUCTION_MAP[0x100] = {\n\n'

with open(filename, 'a') as instr_file:
  instr_file.write('#include "instruction.h"\n\n\n')
  instr_file.write(instr_map)

  for raw_instr in json_data[json_tag]:
    instr_dict = json_data[json_tag][raw_instr]
    opcode = 'OP_' + instr_dict['mnemonic']
    cycles = instr_dict['cycles'][0]
    param1 = 'PARA_NONE'
    param2 = 'PARA_NONE'
    condition = 'COND_NONE'

    if len(instr_dict['operands']) > 0:
      param_name = instr_dict['operands'][0]['name']
      param_is_imm = instr_dict['operands'][0]['immediate']

      if param_name in conditions:
        condition = 'COND_' + param_name

      elif param_name in registers:
        param1 = 'PARA_'
        if not param_is_imm:
          param1 += 'MEM_'
        param1 += 'REG_' + param_name
        if 'increment' in instr_dict['operands'][0]:
          param1 += '_INC'
        elif 'decrement' in instr_dict['operands'][0]:
          param1 += '_DEC'

      elif param_name in imm_8:
        param1 = 'PARA_IMM_8'
      elif param_name in imm_16:
        param1 = 'PARA_IMM_16'
      elif param_name in bit_idx:
        param1 = 'PARA_BIT_IDX'
      elif '$' in param_name:
        param1 = 'PARA_TGT'
    # if len == 1

    if len(instr_dict['operands']) == 2:
      param_name = instr_dict['operands'][1]['name']
      param_is_imm = instr_dict['operands'][1]['immediate']

      if param_name in conditions:
        condition = 'COND_' + param_name

      elif param_name in registers:
        param2 = 'PARA_'
        if not param_is_imm:
          param2 += 'MEM_'
        param2 += 'REG_' + param_name
        if 'increment' in instr_dict['operands'][1]:
          param2 += '_INC'
        elif 'decrement' in instr_dict['operands'][1]:
          param2 += '_DEC'

      elif param_name in imm_8:
        param2 = 'PARA_IMM_8'
      elif param_name in imm_16:
        param2 = 'PARA_IMM_16'
      elif param_name in bit_idx:
        param2 = 'PARA_BIT_IDX'
      elif '$' in param_name:
        param2 = 'PARA_TGT'
    #if len == 2

    if len(instr_dict['operands']) == 3:
      param1 = 'PARA_REG_HL'
      param2 = 'PARA_SP_IMM_8'
    # if len == 3
    
    instr_file.write('\t[%s] = {%s, %s, %s, %s, %s, %s},\n' % (raw_instr, opcode, param1, param2, condition, cycles, raw_instr))
  #for

  instr_file.write('\n};\n\n')
#with
