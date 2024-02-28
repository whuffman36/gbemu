#include "bus.h"

#include "cartridge.h"
#include "global.h"

#include <stdint.h>
#include <stdlib.h>


static const uint16_t _ROM_END = 0x8000;
static const uint16_t _VRAM_BEGIN = _ROM_END;
static const uint16_t _VRAM_END = 0xA000;
static const uint16_t _CRAM_END = 0xC000;
static const uint16_t _WRAM_BEGIN = _CRAM_END;
static const uint16_t _WRAM_END = 0xE000;
static const uint16_t _MIRROR_BEGIN = _WRAM_END;
static const uint16_t _MIRROR_END = 0xFE00;
static const uint16_t _OAM_BEGIN = _MIRROR_END;
static const uint16_t _OAM_END = 0xFEA0;
static const uint16_t _UNUSED_END = 0xFF00;
static const uint16_t _INTERRUPTS_FLAG = 0xFF0F;
static const uint16_t _VRAM_BANK_SELECT = 0xFF4F;
static const uint16_t _WRAM_BANK_SELECT = 0xFF70;
static const uint16_t _IO_REGISTERS_BEGIN = _UNUSED_END;
static const uint16_t _IO_REGISTERS_END = 0xFF80;
static const uint16_t _HRAM_BEGIN = _IO_REGISTERS_END;
static const uint16_t _HRAM_END = 0xFFFF;

static const uint16_t _VRAM_BANK_SIZE = 0x2000;
static const uint16_t _WRAM_BANK_SIZE = 0x1000;


Bus* BusCreate(GlobalCtx* const global_ctx, Cartridge* const cartridge) {
  Bus* bus = (Bus*)malloc(sizeof(Bus));
  if (bus == NULL) {
    global_ctx->error = MEMORY_ALLOCATION_FAILURE;
    return NULL;
  }
  bus->global_ctx_ = global_ctx;
  bus->cartridge_ = cartridge;
  bus->interrupts_enable_reg_ = 0;
  bus->interrupts_flag_ = 0;
  bus->wram_bank_ = 0;
  bus->vram_bank_ = 0;
  return bus;
}


void BusDestroy(Bus* bus) {
  if (bus == NULL) {
    return;
  }
  bus->global_ctx_ = NULL;
  bus->cartridge_ = NULL;
  free(bus);
  bus = NULL;
}


uint8_t BusRead(const Bus* const bus, uint16_t addr) {
  if (addr < _ROM_END) {
    // Read from cartridge ROM.
    return CartridgeRead(bus->cartridge_, addr);
  }
  if (addr < _VRAM_END) {
    // Read from VRAM.
    // VRAM consists of two switchable 0x2000 byte banks.
    return bus->vram_[bus->vram_bank_ * _VRAM_BANK_SIZE + (addr - _VRAM_BEGIN)];
  }
  if (addr < _CRAM_END) {
    // Read from Cartridge RAM.
    return CartridgeRead(bus->cartridge_, addr);
  }
  if (addr < _WRAM_END) {
    // Read from WRAM.
    // WRAM consists of eight switchable 0x1000 byte banks.
    return bus->wram_[bus->wram_bank_ * _WRAM_BANK_SIZE + (addr - _WRAM_BEGIN)];
  }
  if (addr < _MIRROR_END) {
    // Mirror of 0xC000 - 0xDDFF.
    return bus->wram_[bus->wram_bank_ * _WRAM_BANK_SIZE + (addr - _MIRROR_BEGIN)];
  }
  if (addr < _OAM_END) {
    // Read from OAM.
    return bus->oam_[addr - _OAM_BEGIN];
  }
  if (addr < _UNUSED_END) {
    // This memory is supposed to be unusable. In this case we return junk
    // data.
    return 0xFF;
  }
  if (addr == _INTERRUPTS_FLAG) {
    return bus->interrupts_flag_;
  }
  if (addr < _IO_REGISTERS_END) {
    // Read from IO registers.
    return bus->io_regs_[addr - _IO_REGISTERS_BEGIN];
  }
  if (addr < _HRAM_END) {
    // Read from HRAM.
    return bus->hram_[addr - _HRAM_BEGIN];
  }
  // Read from interrupts register.
  return bus->interrupts_enable_reg_;
}


Result BusWrite(Bus* const bus, uint16_t addr, uint8_t data) {
  if (addr < _ROM_END) {
    // Write to cartridge ROM.
    return CartridgeWrite(bus->cartridge_, addr, data);
  }
  if (addr < _VRAM_END) {
    // Write to VRAM.
    // VRAM consists of two switchable 0x2000 byte banks.
    bus->vram_[bus->vram_bank_ * _VRAM_BANK_SIZE + (addr - _VRAM_BEGIN)] = data;
    return RESULT_OK;
  }
  if (addr < _CRAM_END) {
    // Write to cartridge RAM.
    return CartridgeWrite(bus->cartridge_, addr, data);
  }
  if (addr < _WRAM_END) {
    // Write to WRAM.
    // WRAM consists of eight switchable 0x1000 byte banks.
    bus->wram_[bus->wram_bank_ * _WRAM_BANK_SIZE + (addr - _WRAM_BEGIN)] = data;
    return RESULT_OK;
  }
  if (addr < _MIRROR_END) {
    // Mirror of 0xC000 - 0xDDFF.
    bus->wram_[bus->wram_bank_ * _WRAM_BANK_SIZE + (addr - _MIRROR_BEGIN)] = data;
    return RESULT_OK;
  }
  if (addr < _OAM_END) {
    // Write to OAM.
    bus->oam_[addr - _OAM_BEGIN] = data;
    return RESULT_OK;
  }
  if (addr < _UNUSED_END) {
    // This memory is supposed to be unusable. Writing should fail.
    bus->global_ctx_->error = ILLEGAL_WRITE_TO_MEMORY;
    return RESULT_NOTOK;
  }
  if (addr == _INTERRUPTS_FLAG) {
    bus->interrupts_flag_ = data;
  }
  if (addr == _VRAM_BANK_SELECT && bus->global_ctx_->mode == GB_MODE_GBC) {
    // Selects VRAM memory bank 0-1 in VRAM 0x8000-0x9FFF. GBC Only.
    // Only the least significant bit is used.
    bus->vram_bank_ = data & 0x01;
  }
  if (addr == _WRAM_BANK_SELECT && bus->global_ctx_->mode == GB_MODE_GBC) {
    // Selects WRAM memory bank 1-7 in WRAM 0xD000-0xDFFF. GBC Only.
    // Only the bottom three bits are needed for the range 1-7.
    bus->wram_bank_ = data & 0x07;
    return RESULT_OK;
  } 
  if (addr < _IO_REGISTERS_END) {
    // Write to IO registers.
    bus->io_regs_[addr - _IO_REGISTERS_BEGIN] = data;
    return RESULT_OK;
  }
  if (addr < _HRAM_END) {
    // Write to HRAM.
    bus->hram_[addr - _HRAM_BEGIN] = data;
    return RESULT_OK;
  }
  // Write to interrupts register.
  bus->interrupts_enable_reg_ = data;
  return RESULT_OK;
}
