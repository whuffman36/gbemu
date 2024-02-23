#ifndef BUS_H
#define BUS_H

#include "cartridge.h"
#include "global.h"

#include "stdint.h"


typedef struct BusDef {
  // 0xC000 - 0xDFFF
  uint8_t wram_[0x8000];
  // 0x8000 - 0x9FFF
  uint8_t vram_[0x4000];
  // 0xFE00 - 0xFE9F
  uint8_t oam_[0x9F];
  // 0xFF00 - 0xFF70
  uint8_t io_regs_[0x7F];
  // 0xFF80 - 0xFFFE
  uint8_t hram_[0x7E];
  // 0xFFFF
  uint8_t interrupts_enable_reg_;
  // 0xFF0F
  uint8_t interrupts_flag_;
  // Offset for bank switching wram
  uint8_t wram_bank_;
  // Offset for bank switching vram
  uint8_t vram_bank_;

  Cartridge* cartridge_;
  GlobalCtx* global_ctx_;
} Bus;


Bus* BusCreate(GlobalCtx* const global_ctx, Cartridge* const cartridge);

void BusDestroy(Bus* bus);

uint8_t BusRead(const Bus* const bus, uint16_t addr);

Result BusWrite(Bus* const bus, uint16_t addr, uint8_t data);

#endif
