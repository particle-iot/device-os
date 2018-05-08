/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "usbd_dfu_mal.h"

/* Important! Needs to be included before other headers */
#include "logging.h"
#include "nrf_nvmc.h"

using namespace particle::usbd::dfu::mal;

namespace {

} /* anonymous */

InternalFlashMal::InternalFlashMal()
    : dfu::DfuMal() {
}

InternalFlashMal::~InternalFlashMal() {
}

int InternalFlashMal::init() {
  return 0;
}

int InternalFlashMal::deInit() {
  return 0;
}

bool InternalFlashMal::validate(uintptr_t addr, size_t len) {
  uintptr_t end = addr + len;
  if (addr >= 0x100000 || end > 0x100000) {
    return false;
  }

  return true;
}

int InternalFlashMal::read(uint8_t* buf, uintptr_t addr, size_t len) {
  if (!validate(addr, len)) {
    return 1;
  }

  memcpy(buf, (void*)addr, len);
  return 0;
}

int InternalFlashMal::write(const uint8_t* buf, uintptr_t addr, size_t len) {
  if (!validate(addr, len)) {
    return 1;
  }

  size_t fullWords = len / sizeof(uintptr_t);
  size_t totalWords = (len + sizeof(uintptr_t) - 1) / sizeof(uintptr_t);
  nrf_nvmc_write_words(addr, (const uint32_t*)buf, fullWords);
  if (totalWords > fullWords) {
    uint32_t w = 0xffffffff;
    for (unsigned i = 0; i < (totalWords * sizeof(uintptr_t) - len); i++) {
      ((uint8_t*)&w)[i] = buf[fullWords * sizeof(uintptr_t) + i];
    }
    nrf_nvmc_write_word(addr + fullWords * sizeof(uintptr_t), w);
  }
  return 0;
}

int InternalFlashMal::erase(uintptr_t addr, size_t len) {
  if (!validate(addr, len)) {
    return 1;
  }

  nrf_nvmc_page_erase(addr);
  (void)len;
  return 0;
}

int InternalFlashMal::getStatus(detail::DfuGetStatus* status, dfu::detail::DfuseCommand cmd) {
  status->bwPollTimeout[0] = status->bwPollTimeout[1] = status->bwPollTimeout[2] = 0;
  return 0;
}

const char* InternalFlashMal::getString() {
  /* 12K MBR (read-only)
   * 192K softdevice area
   * 197 * 4K normal flash
   * 32K bootloader (read-only)
   */
  return "@Internal Flash   /0x00000000/3*004Ka,48*004Kg,197*004Kg,8*004Ka";
}

DcdMal::DcdMal()
    : dfu::DfuMal() {
}

DcdMal::~DcdMal() {
}

int DcdMal::init() {
  return dfu::detail::errUNKNOWN;
}

int DcdMal::deInit() {
  return dfu::detail::errUNKNOWN;
}

bool DcdMal::validate(uintptr_t addr, size_t len) {
  return true;
}

int DcdMal::read(uint8_t* buf, uintptr_t addr, size_t len) {
  return dfu::detail::errUNKNOWN;
}

int DcdMal::write(const uint8_t* buf, uintptr_t addr, size_t len) {
  return dfu::detail::errUNKNOWN;
}

int DcdMal::erase(uintptr_t addr, size_t len) {
  return dfu::detail::errUNKNOWN;
}

int DcdMal::getStatus(detail::DfuGetStatus* status, dfu::detail::DfuseCommand cmd) {
  status->bwPollTimeout[0] = status->bwPollTimeout[1] = status->bwPollTimeout[2] = 0;
  return dfu::detail::errUNKNOWN;
}

const char* DcdMal::getString() {
  return "@DCD Flash   /0x00000000/01*016Kg";
}

ExternalFlashMal::ExternalFlashMal()
    : dfu::DfuMal() {
}

ExternalFlashMal::~ExternalFlashMal() {
}

int ExternalFlashMal::init() {
  return dfu::detail::errUNKNOWN;
}

int ExternalFlashMal::deInit() {
  return dfu::detail::errUNKNOWN;
}

bool ExternalFlashMal::validate(uintptr_t addr, size_t len) {
  return true;
}

int ExternalFlashMal::read(uint8_t* buf, uintptr_t addr, size_t len) {
  return dfu::detail::errUNKNOWN;
}

int ExternalFlashMal::write(const uint8_t* buf, uintptr_t addr, size_t len) {
  return dfu::detail::errUNKNOWN;
}

int ExternalFlashMal::erase(uintptr_t addr, size_t len) {
  return dfu::detail::errUNKNOWN;
}

int ExternalFlashMal::getStatus(detail::DfuGetStatus* status, dfu::detail::DfuseCommand cmd) {
  status->bwPollTimeout[0] = status->bwPollTimeout[1] = status->bwPollTimeout[2] = 0;
  return dfu::detail::errUNKNOWN;
}

const char* ExternalFlashMal::getString() {
  /* 4M */
  return "@External Flash   /0x00000000/4096*001Kg";
}
