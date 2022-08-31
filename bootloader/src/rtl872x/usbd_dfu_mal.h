/*
 * Copyright (c) 2021 Particle Industries, Inc.  All rights reserved.
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

#ifndef USBD_DFU_MAL_H
#define USBD_DFU_MAL_H

#include "usbd_dfu.h"
#include "platforms.h"

// FIXME: these should be coming from platform config
#define INTERNAL_FLASH_START_ADD        0x08000000
#define INTERNAL_FLASH_END_ADDR         0x08800000
#define INTERNAL_FLASH_IF_STRING        "@Internal Flash   /0x08000000/2048*004Kg";

#define DCT_START_ADD                   0x00000000
#define DCT_END_ADDR                    0x00004000
#define DCT_IF_STRING                   "@DCT Flash   /0x00000000/1*016Ke";

namespace particle { namespace usbd { namespace dfu { namespace mal {

namespace dfu = ::particle::usbd::dfu;

class InternalFlashMal: public dfu::DfuMal {
public:
  InternalFlashMal();
  virtual ~InternalFlashMal();

  virtual int init() override;
  virtual int deInit() override;
  virtual bool validate(uintptr_t addr, size_t len) override;
  virtual int read(uint8_t* buf, uintptr_t addr, size_t len) override;
  virtual int write(const uint8_t* buf, uintptr_t addr, size_t len) override;
  virtual int erase(uintptr_t addr, size_t len) override;
  virtual int getStatus(detail::DfuGetStatus* status, dfu::detail::DfuseCommand cmd) override;
  virtual const char* getString() override;
};

class DctMal: public dfu::DfuMal {
public:
  DctMal();
  virtual ~DctMal();

  virtual int init() override;
  virtual int deInit() override;
  virtual bool validate(uintptr_t addr, size_t len) override;
  virtual int read(uint8_t* buf, uintptr_t addr, size_t len) override;
  virtual int write(const uint8_t* buf, uintptr_t addr, size_t len) override;
  virtual int erase(uintptr_t addr, size_t len) override;
  virtual int getStatus(detail::DfuGetStatus* status, dfu::detail::DfuseCommand cmd) override;
  virtual const char* getString() override;
};

} } } } /* namespace particle::usbd::dfu::mal */

#endif /* USBD_DFU_MAL_H */
