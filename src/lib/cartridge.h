#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include "global.h"
#include "mbc.h"

#include "stdint.h"


typedef struct CartridgeHeaderDef {
  uint8_t entry_point[0x04];
  uint8_t nintendo_logo[0x30];
  char title[0x0B];
  char manufacturer_code[0x04];
  uint8_t gbc_flag;
  char new_licensee_code[0x02];
  uint8_t sgb_flag;
  uint8_t cartridge_type;
  uint8_t rom_size;
  uint8_t ram_size;
  uint8_t destination_code;
  uint8_t old_licensee_code;
  uint8_t rom_version;
  uint8_t header_checksum;
  uint16_t global_checksum;
} CartridgeHeader;

typedef struct CartridgeDef {
  MemBankController mbc;
  const char* filename;
  uint8_t* data;
  uint8_t* ram;
  GlobalCtx* global_ctx;
} Cartridge;


Cartridge* CartridgeCreate(GlobalCtx* const global_ctx,
                           const char* const filename);

void CartridgeDestroy(Cartridge* cart);

uint8_t CartridgeRead(const Cartridge* const cart, uint16_t addr);

Result CartridgeWrite(Cartridge* const cart, uint16_t addr,
                      uint8_t data);

#endif
