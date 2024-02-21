#include "cartridge.h"

#include "global.h"
#include "mbc.h"

#include "assert.h"
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"


static const uint16_t _ROM_BANK_0_END = 0x4000;
static const uint16_t _ROM_BANK_N_END = 0x8000;
static const uint16_t _ROM_BANK_SIZE = 0x4000;

static const uint16_t _RAM_BEGIN = 0xA000;
static const uint16_t _RAM_END = 0xC000;
static const uint16_t _RAM_BANK_SIZE = 0x2000;

static const uint16_t _RAM_ENABLE_END = 0x2000;
static const uint16_t _ROM_BANK_SELECT_END = 0x4000;
static const uint16_t _RAM_ROM_RTC_SELECT_END = 0x6000;
static const uint16_t _BANKING_MODE_SELECT_LATCH_END = 0x8000;

static const char* _CARTRIDGE_TYPES[] = {
  "ROM ONLY",
  "MBC_1",
  "MBC_1+RAM",
  "MBC_1+RAM+BATTERY",
  "0x04 UNKNOWN",
  "MBC_2",
  "MBC_2+BATTERY",
  "0x07 UNKNOWN",
  "ROM+RAM 1",
  "ROM+RAM+BATTERY 1",
  "0x0A UNKNOWN",
  "MMM01",
  "MMM01+RAM",
  "MMM01+RAM+BATTERY",
  "0x0E UNKNOWN",
  "MBC_3+TIMER+BATTERY",
  "MBC_3+TIMER+RAM+BATTERY 2",
  "MBC_3",
  "MBC_3+RAM 2",
  "MBC_3+RAM+BATTERY 2",
  "0x14 UNKNOWN",
  "0x15 UNKNOWN",
  "0x16 UNKNOWN",
  "0x17 UNKNOWN",
  "0x18 UNKNOWN",
  "MBC_5",
  "MBC_5+RAM",
  "MBC_5+RAM+BATTERY",
  "MBC_5+RUMBLE",
  "MBC_5+RUMBLE+RAM",
  "MBC_5+RUMBLE+RAM+BATTERY",
  "0x1F UNKNOWN",
  "MBC6",
  "0x21 UNKNOWN",
  "MBC7+SENSOR+RUMBLE+RAM+BATTERY"
};

static const uint8_t _RAM_SIZES[] = {
  0,
  0,
  8,
  32,
  128,
  64
};

static Result ReadRomFile(const char* const filename,
                          Cartridge* const cartridge) {
  FILE* fp;
  fp = fopen(filename, "rb");
  if (fp == NULL) {
    cartridge->global_ctx_->error = FILE_NOT_FOUND;
    return RESULT_NOTOK;
  }

  fseek(fp, 0, SEEK_END);
  uint32_t rom_size = ftell(fp);
  rewind(fp);
  cartridge->data_ = (uint8_t*)malloc(rom_size);
  if (cartridge->data_ == NULL) {
    cartridge->global_ctx_->error = MEMORY_ALLOCATION_FAILURE;
    return RESULT_NOTOK;
  }

  size_t rom_read = fread(cartridge->data_, rom_size, 1, fp);
  if (rom_read != 1) {
    cartridge->global_ctx_->error = FAILED_TO_READ_ROM;
    return RESULT_NOTOK;
  }
  fclose(fp);

  uint8_t checksum = 0;
  for (uint16_t i = 0x0134; i < 0x014D; ++i) {
    checksum = checksum - cartridge->data_[i] - 1;
  }

  // Cartridge header begins at memory address 0x100.
  CartridgeHeader* header = (CartridgeHeader*)(cartridge->data_ + 0x100);
  if (header->header_checksum_ != checksum) {
    cartridge->global_ctx_->error = HEADER_CHECKSUM_FAILED;
    return RESULT_NOTOK;
  }

  // Done for brevity's sake.
  uint8_t ct = header->cartridge_type_;
  if (ct == 0 || ct == 0x08 || ct == 0x09) {
    cartridge->mbc_.type_ = MBC_NONE;
  }
  else if (ct < 0x04) {
    cartridge->mbc_.type_ = MBC_1;
  }
  else if (ct < 0x07) {
    cartridge->mbc_.type_ = MBC_2;
  }
  else if (ct < 0x14 && ct > 0x0E) {
    cartridge->mbc_.type_ = MBC_3;
  }
  else if (ct < 0x1F && ct > 0x18) {
    cartridge->mbc_.type_ = MBC_5;
    cartridge->global_ctx_->error = MBC_TYPE_NOT_SUPPORTED;
    return RESULT_NOTOK;
  }
  else {
    cartridge->global_ctx_->error = MBC_TYPE_NOT_SUPPORTED;
    return RESULT_NOTOK;
  }

  // The MBC_2 is marked as having no external RAM, though it does have 512
  // half bytes (4 bit memory slots) built into the chip.
  if (header->ram_size_ > 1) {
    cartridge->ram_ = (uint8_t*)malloc(_RAM_SIZES[header->ram_size_] * 1024);
    if (cartridge->ram_ == NULL) {
      cartridge->global_ctx_->error = MEMORY_ALLOCATION_FAILURE;
      return RESULT_NOTOK;
    }
  }
  else if (cartridge->mbc_.type_ == MBC_2) {
    cartridge->ram_ = (uint8_t*)malloc(512);
    if (cartridge->ram_ == NULL) {
      cartridge->global_ctx_->error = MEMORY_ALLOCATION_FAILURE;
      return RESULT_NOTOK;
    }
  }

  printf("Game Title: %s\n", header->title_);
  printf("Manufacterer Code: %s\n", header->manufacturer_code_);
  printf("Gameboy Color Mode: %s\n", header->gbc_flag_ > 0x7F ? "YES" : "NO");
  printf("New Licensee Code: %c%c\n", header->new_licensee_code_[0], header->new_licensee_code_[1]);
  printf("Super Gameboy Mode: %s\n", header->sgb_flag_ == 0x03 ? "YES" : "NO");
  printf("Cartridge Type: %s\n", _CARTRIDGE_TYPES[header->cartridge_type_]);
  printf("ROM Size: %d KiB\n", 32 * (1 << header->rom_size_));
  printf("Measured ROM Size: %d KiB\n", rom_size / 1024);
  printf("RAM Size: %d KiB\n", _RAM_SIZES[header->ram_size_]);
  printf("Destination Code: %s\n", header->destination_code_ == 0 ? "Japan" : "Overseas");
  printf("Old Licensee Code: %02x\n", header->old_licensee_code_);
  printf("ROM Version: %d\n\n", header->rom_version_);
  return RESULT_OK;
}


Cartridge* CartridgeCreate(GlobalCtx* const global_ctx,
                          const char* const filename) {
  Cartridge* cartridge = (Cartridge*)malloc(sizeof(Cartridge));
  if (cartridge == NULL) {
    global_ctx->error = MEMORY_ALLOCATION_FAILURE;
    return NULL;
  }

  cartridge->global_ctx_ = global_ctx;
  cartridge->filename_ = filename;
  MemBankControllerInit(&cartridge->mbc_);

  Result result = ReadRomFile(cartridge->filename_, cartridge);
  if (result == RESULT_NOTOK) {
    CartridgeDestroy(cartridge);
    return NULL;
  }
  return cartridge;
}


void CartridgeDestroy(Cartridge* cart) {
  if (cart == NULL) {
    return;
  }
  cart->global_ctx_ = NULL;
  cart->filename_ = NULL;
  free(cart->data_);
  cart->data_ = NULL;
  free(cart->ram_);
  cart->ram_ = NULL;
  free(cart);
  cart = NULL;
}


uint8_t CartridgeRead(const Cartridge* const cart, uint16_t addr) {
  if (addr < _ROM_BANK_0_END) {
    // Read from ROM bank 0
    return cart->data_[addr];
  }
  if (addr < _ROM_BANK_N_END) {
    // Read from ROM bank <mbc_.rom_bank_>.
    return cart->data_[cart->mbc_.rom_bank_ * _ROM_BANK_SIZE + addr];
  }
  if (addr < _RAM_END) {
    // Read from RAM, which must be enabled.
    assert(cart->mbc_.ram_enable_ == RAM_ENABLED);

    switch (cart->mbc_.type_) {
      case MBC_NONE:
        // Not a valid option, as there are no known cartridges with RAM and
        // no MBC.
        return cart->ram_[addr - _RAM_BEGIN];

      case MBC_1:
        // Read from RAM bank <mbc_.ram_abnk_>.
        return cart->ram_[cart->mbc_.ram_bank_ * _RAM_BANK_SIZE + (addr - _RAM_BEGIN)];

      case MBC_2:
        return cart->ram_[addr - _RAM_BEGIN];

      case MBC_3:
        if (cart->mbc_.banking_mode_ == RAM_BANKING) {
          return cart->ram_[cart->mbc_.ram_bank_ * _RAM_BANK_SIZE + (addr - _RAM_BEGIN)];
        }
        else if (cart->mbc_.banking_mode_ == RTC_BANKING) {
          // In RTC Register select mode (RTC_BANKING), reading from this
          // address range takes whatever RTC register value was written.
          // In this implementation, the selected RTC register value will
          // always be written in ram_[0].
          return cart->ram_[0];
        }
        break;

      case MBC_5:
        // TODO
        break;
    }
  }
  // Code should never reach this point. Return junk value.
  // return 0xFF;
  __builtin_unreachable();
}


Result CartridgeWrite(Cartridge* const cart, uint16_t addr,
                      uint8_t data) {
  if (addr < _RAM_ENABLE_END) {
    // RAM enable. 0xA in the bottom 4 bits enables, any other value disables.
    if (cart->mbc_.type_ == MBC_2) {
      // Top byte of data must be even to enable/disable RAM.
      uint8_t h_data = data & 0xF0;
      if (h_data % 2 != 0) {
        return RESULT_OK;
      }
    }
    if ((data & 0x0F) == 0x0A) {
      cart->mbc_.ram_enable_ = RAM_ENABLED;
      return RESULT_OK;
    }
    cart->mbc_.ram_enable_ = RAM_DISABLED;
    return RESULT_OK;
  }
  if (addr < _ROM_BANK_SELECT_END) {
    // ROM Bank select.
    switch (cart->mbc_.type_) {
      case MBC_NONE:
        cart->global_ctx_->error = ILLEGAL_WRITE_TO_MEMORY;
        return RESULT_NOTOK;

      case MBC_1:
        // Only the bottom five bits are used.
        cart->mbc_.rom_bank_ = data & 0x1F;
        // Due to a quirk in MBC_1 hardware, 0x00, 0x20, 0x40, and 0x60 all short
        // circuit to one value higher. Each of these values have all 0s in the
        // lower 5 bits.
        if (cart->mbc_.rom_bank_ == 0) {
          cart->mbc_.rom_bank_++;
        }
        return RESULT_OK;
      case MBC_2:
        // Only the bottom four bits are used. The top four bits must be odd
        // to select the ROM bank.
        ; // Why the fuck is the semicolon needed ?
        uint8_t h_data = data & 0xF0;
        if (h_data % 2 == 0) {
          return RESULT_OK;
        }
        uint8_t l_data = data & 0x0F;
        cart->mbc_.rom_bank_ = l_data;
        return RESULT_OK;

      case MBC_3:
        // Only the seven bottom bits are used.
        cart->mbc_.rom_bank_ = data & 0x7F;
        // Selecting bank 0 aautomatically short circuits to selecting bank 1.
        if (cart->mbc_.rom_bank_ == 0) {
          cart->mbc_.rom_bank_++;
        }
      case MBC_5:
        // TODO
        break;
    } // switch
  }
  if (addr < _RAM_ROM_RTC_SELECT_END) {
    // RAM bank select, ROM bank select, or RTC register select, depending on
    // banking mode and data.
    switch (cart->mbc_.type_) {
      case MBC_NONE:
        cart->global_ctx_->error = ILLEGAL_WRITE_TO_MEMORY;
        return RESULT_NOTOK;

      case MBC_1:
        // Only the bottom two bits are used. In ROM select, these two bits are
        // the upper bits for the ROM bank.
        if (cart->mbc_.banking_mode_ == RAM_BANKING) {
          cart->mbc_.ram_bank_ = data & 0x03;
        }
        else if (cart->mbc_.banking_mode_ == ROM_BANKING) {
          uint8_t l_rom_bank = cart->mbc_.rom_bank_;
          uint8_t h_rom_bank = (data & 0x03) << 5;
          cart->mbc_.rom_bank_ = h_rom_bank | l_rom_bank;
        }
        return RESULT_OK;

      case MBC_2:
        cart->global_ctx_->error = ILLEGAL_WRITE_TO_MEMORY;
        return RESULT_NOTOK;

      case MBC_3:
        if (data < 0x08) {
          cart->mbc_.ram_bank_ = data;
        }
        else if (data < 0x0D) {
          cart->mbc_.banking_mode_ = RTC_BANKING;
          switch (data) {
            case 0x08:
              cart->ram_[0] = cart->mbc_.rtc_.seconds_;
              break;
            case 0x09:
              cart->ram_[0] = cart->mbc_.rtc_.minutes_;
              break;
            case 0x0A:
              cart->ram_[0] = cart->mbc_.rtc_.hours_;
              break;
            case 0x0B:
              cart->ram_[0] = cart->mbc_.rtc_.l_day_counter_;
              break;
            case 0x0C:
              cart->ram_[0] = cart->mbc_.rtc_.h_day_counter_;
              break;
          } // switch
        } // if
        return RESULT_OK;

      case MBC_5:
        // TODO
        break;
    } // switch
  }
  if (addr < _BANKING_MODE_SELECT_LATCH_END) {
    switch (cart->mbc_.type_) {
      case MBC_NONE:
        cart->global_ctx_->error = ILLEGAL_WRITE_TO_MEMORY;
        return RESULT_NOTOK;

      case MBC_1:
        // Banking mode select.
        if (data == 0x00) {
          cart->mbc_.banking_mode_ = ROM_BANKING;
        }
        if (data == 0x01) {
          cart->mbc_.banking_mode_ = RAM_BANKING;
        }
        return RESULT_OK;

      case MBC_2:
        cart->global_ctx_->error = ILLEGAL_WRITE_TO_MEMORY;
        return RESULT_NOTOK;

      case MBC_3:
        if (cart->mbc_.rtc_.latch_ == 0 && data == 1) {
          // Capture current time into RTC registers.
          LatchCurrentTimeIntoRTC(&cart->mbc_.rtc_);
        }
        cart->mbc_.rtc_.latch_ = data;
        return RESULT_OK;

      case MBC_5:
        // TODO
        break;
    } // switch
  }
  if (addr < _RAM_END && addr >= _RAM_BEGIN) {
    // Write to RAM, which must be enabled.
    assert(cart->mbc_.ram_enable_ == RAM_ENABLED);

    if (cart->mbc_.type_ == MBC_2) {
      // MBC_2 has less RAM than other MBCs, ending at address 0xA1FF.
      if (addr > 0xA1FF) {
        cart->global_ctx_->error = ILLEGAL_WRITE_TO_MEMORY;
        return RESULT_NOTOK;
      }
      cart->ram_[addr - _RAM_BEGIN] = data;
      return RESULT_OK;
    }

    // Not entirely sure why you would want to write to these addresses in
    // RTC register select mode, as this value cannot be latched back into
    // the RTC registers, only read from again at this address. However, I
    // didn't find any information saying this area couldn't be written to.
    if (cart->mbc_.type_ == MBC_3 && cart->mbc_.banking_mode_ == RTC_BANKING) {
      cart->ram_[0] = data;
      return RESULT_OK;
    }

    cart->ram_[cart->mbc_.ram_bank_ * _RAM_BANK_SIZE + (addr - _RAM_BEGIN)] = data;
    return RESULT_OK;
  }
  // Code should never reach this point.
  return RESULT_NOTOK;
}
