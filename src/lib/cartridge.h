#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include "global.h"
#include "mbc.h"

#include "stdint.h"


typedef struct CartridgeHeaderDef {
  uint8_t entry_point_[0x04];
  uint8_t nintendo_logo_[0x30];
  char title_[0x0B];
  char manufacturer_code_[0x04];
  uint8_t gbc_flag_;
  char new_licensee_code_[0x02];
  uint8_t sgb_flag_;
  uint8_t cartridge_type_;
  uint8_t rom_size_;
  uint8_t ram_size_;
  uint8_t destination_code_;
  uint8_t old_licensee_code_;
  uint8_t rom_version_;
  uint8_t header_checksum_;
  uint16_t global_checksum_;
} CartridgeHeader;

typedef struct CartridgeDef {
  MemBankController mbc_;
  const char* filename_;
  uint8_t* data_;
  uint8_t* ram_;
  GlobalCtx* global_ctx_;
} Cartridge;


Cartridge* CartridgeCreate(GlobalCtx* const global_ctx,
                           const char* const filename);

void CartridgeDestroy(Cartridge* cart);

uint8_t CartridgeRead(const Cartridge* const cart, uint16_t addr);

Result CartridgeWrite(Cartridge* const cart, uint16_t addr,
                      uint8_t data);

#endif
