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
static const uint16_t _SERIAL_TRANSFER_DATA = 0xFF01;
static const uint16_t _SERIAL_TRANSFER_CONTROL = 0xFF02;
static const uint16_t _DIV_TIMER_REG = 0xFF04;
static const uint16_t _TIMA_TIMER_REG = 0xFF05;
static const uint16_t _TMA_TIMER_REG = 0xFF06;
static const uint16_t _TAC_TIMER_REG = 0xFF07;
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
  bus->global_ctx = global_ctx;
  bus->cartridge = cartridge;
  bus->interrupts_enable_reg = 0;
  bus->interrupts_flag = 0;
  bus->wram_bank = 0;
  bus->vram_bank = 0;
  TimerInit(&bus->timer);
  bus->timer.div = 0xABCC;
  return bus;
}


void BusDestroy(Bus* bus) {
  if (bus == NULL) {
    return;
  }
  bus->global_ctx = NULL;
  bus->cartridge = NULL;
  free(bus);
  bus = NULL;
}


uint8_t BusRead(const Bus* const bus, uint16_t addr) {
  if (addr < _ROM_END) {
    // Read from cartridge ROM.
    return CartridgeRead(bus->cartridge, addr);
  }
  if (addr < _VRAM_END) {
    // Read from VRAM.
    // VRAM consists of two switchable 0x2000 byte banks.
    return bus->vram [bus->vram_bank * _VRAM_BANK_SIZE + (addr - _VRAM_BEGIN)];
  }
  if (addr < _CRAM_END) {
    // Read from Cartridge RAM.
    return CartridgeRead(bus->cartridge, addr);
  }
  if (addr < _WRAM_END) {
    // Read from WRAM.
    // WRAM consists of eight switchable 0x1000 byte banks.
    return bus->wram [bus->wram_bank * _WRAM_BANK_SIZE + (addr - _WRAM_BEGIN)];
  }
  if (addr < _MIRROR_END) {
    // Mirror of 0xC000 - 0xDDFF.
    return bus->wram [bus->wram_bank * _WRAM_BANK_SIZE + (addr - _MIRROR_BEGIN)];
  }
  if (addr < _OAM_END) {
    // Read from OAM.
    return bus->oam [addr - _OAM_BEGIN];
  }
  if (addr < _UNUSED_END) {
    // This memory is supposed to be unusable. In this case we return junk
    // data.
    return 0xFF;
  }
  if (addr == _SERIAL_TRANSFER_DATA) {
    return bus->serial_data[0];
  }
  if (addr == _SERIAL_TRANSFER_CONTROL) {
    return bus->serial_data[1];
  }
  if (addr == _DIV_TIMER_REG) {
    return (uint8_t)bus->timer.div;
  }
  if (addr == _TIMA_TIMER_REG) {
    return bus->timer.tima;
  }
  if (addr == _TMA_TIMER_REG) {
    return bus->timer.tma;
  }
  if (addr == _TAC_TIMER_REG) {
    return bus->timer.tac;
  }
  if (addr == _INTERRUPTS_FLAG) {
    return bus->interrupts_flag;
  }
  if (addr < _IO_REGISTERS_END) {
    // Read from IO registers.
    return bus->io_regs [addr - _IO_REGISTERS_BEGIN];
  }
  if (addr < _HRAM_END) {
    // Read from HRAM.
    return bus->hram [addr - _HRAM_BEGIN];
  }
  // Read from interrupts register.
  return bus->interrupts_enable_reg;
}


Result BusWrite(Bus* const bus, uint16_t addr, uint8_t data) {
  if (addr < _ROM_END) {
    // Write to cartridge ROM.
    return CartridgeWrite(bus->cartridge, addr, data);
  }
  if (addr < _VRAM_END) {
    // Write to VRAM.
    // VRAM consists of two switchable 0x2000 byte banks.
    bus->vram [bus->vram_bank * _VRAM_BANK_SIZE + (addr - _VRAM_BEGIN)] = data;
    return RESULT_OK;
  }
  if (addr < _CRAM_END) {
    // Write to cartridge RAM.
    return CartridgeWrite(bus->cartridge, addr, data);
  }
  if (addr < _WRAM_END) {
    // Write to WRAM.
    // WRAM consists of eight switchable 0x1000 byte banks.
    bus->wram [bus->wram_bank * _WRAM_BANK_SIZE + (addr - _WRAM_BEGIN)] = data;
    return RESULT_OK;
  }
  if (addr < _MIRROR_END) {
    // Mirror of 0xC000 - 0xDDFF.
    bus->wram [bus->wram_bank * _WRAM_BANK_SIZE + (addr - _MIRROR_BEGIN)] = data;
    return RESULT_OK;
  }
  if (addr < _OAM_END) {
    // Write to OAM.
    bus->oam [addr - _OAM_BEGIN] = data;
    return RESULT_OK;
  }
  if (addr < _UNUSED_END) {
    // This memory is supposed to be unusable. Writing should fail.
    bus->global_ctx->error = ILLEGAL_WRITE_TO_MEMORY;
    return RESULT_NOTOK;
  }
  if (addr == _SERIAL_TRANSFER_DATA) {
    bus->serial_data[0] = data;
    return RESULT_OK;
  }
  if (addr == _SERIAL_TRANSFER_CONTROL) {
    bus->serial_data[1] = data;
    return RESULT_OK;
  }
  if (addr == _DIV_TIMER_REG) {
    bus->timer.div = 0;
    return RESULT_OK;
  }
  if (addr == _TIMA_TIMER_REG) {
    bus->timer.tima = data;
    return RESULT_OK;
  }
  if (addr == _TMA_TIMER_REG) {
    bus->timer.tma = data;
    return RESULT_OK;
  }
  if (addr == _TAC_TIMER_REG) {
    bus->timer.tac = data;
    return RESULT_OK;
  }
  if (addr == _INTERRUPTS_FLAG) {
    bus->interrupts_flag = data;
  }
  if (addr == _VRAM_BANK_SELECT && bus->global_ctx->mode == GB_MODE_GBC) {
    // Selects VRAM memory bank 0-1 in VRAM 0x8000-0x9FFF. GBC Only.
    // Only the least significant bit is used.
    bus->vram_bank = data & 0x01;
    return RESULT_OK;
  }
  if (addr == _WRAM_BANK_SELECT && bus->global_ctx->mode == GB_MODE_GBC) {
    // Selects WRAM memory bank 1-7 in WRAM 0xD000-0xDFFF. GBC Only.
    // Only the bottom three bits are needed for the range 1-7.
    bus->wram_bank = data & 0x07;
    return RESULT_OK;
  } 
  if (addr < _IO_REGISTERS_END) {
    // Write to IO registers.
    bus->io_regs [addr - _IO_REGISTERS_BEGIN] = data;
    return RESULT_OK;
  }
  if (addr < _HRAM_END) {
    // Write to HRAM.
    bus->hram [addr - _HRAM_BEGIN] = data;
    return RESULT_OK;
  }
  // Write to interrupts register.
  bus->interrupts_enable_reg = data;
  return RESULT_OK;
}
