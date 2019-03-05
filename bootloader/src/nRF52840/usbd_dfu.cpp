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

#include "usbd_dfu.h"
#include <cstring>
#include "hal_irq_flag.h"

using namespace particle::usbd;
using namespace particle::usbd::dfu;

constexpr const uint8_t DfuClassDriver::msftExtCompatIdOsDescr_[];
constexpr const uint8_t DfuClassDriver::msftExtPropOsDescr_[];

DfuClassDriver::DfuClassDriver(Device* dev)
    : dev_(dev) {
  dev_->registerClass(this);
}

DfuClassDriver::~DfuClassDriver() {
}

int DfuClassDriver::init(unsigned cfgIdx) {
  setState(detail::dfuIDLE);
  setStatus(detail::OK);

  reset();
  address_ = 0;

  for (auto m : mal_) {
    if (m) {
      m->init();
    }
  }
  return 0;
}

int DfuClassDriver::deInit(unsigned cfgIdx) {

  for (auto m : mal_) {
    if (m) {
      m->deInit();
    }
  }
  return 0;
}

int DfuClassDriver::handleMsftRequest(SetupRequest* req) {
  if (req->wIndex == 0x0004) {
    dev_->setupReply(req, msftExtCompatIdOsDescr_, req->wLength);
  } else if (req->wIndex == 0x0005) {
    if ((req->wValue & 0xff) == 0x00) {
      dev_->setupReply(req, msftExtPropOsDescr_, req->wLength);
    } else {
      // Send dummy
      const uint8_t dummy[10] = {0};
      dev_->setupReply(req, dummy, req->wLength);
    }
  } else {
    dev_->setupError(req);
  }

  return 0;
}

int DfuClassDriver::handleStandardRequest(SetupRequest* req) {
  switch (req->bRequest) {
    case SetupRequest::REQUEST_GET_DESCRIPTOR: {
      if ((req->wValue >> 8) == detail::DFU_FUNCTIONAL_DESCRIPTOR_TYPE) {
        dev_->setupReply(req, cfgDesc_ + sizeof(cfgDesc_) - 0x09,
                         0x09);
      }
      break;
    }
    case SetupRequest::REQUEST_GET_INTERFACE: {
      dev_->setupReply(req, &alternativeSetting_, sizeof(alternativeSetting_));
      break;
    }
    case SetupRequest::REQUEST_SET_INTERFACE: {
      if (req->wValue < USBD_DFU_MAX_CONFIGURATIONS) {
        alternativeSetting_ = req->wValue;
        dev_->setupReply(req, nullptr, 0);
        break;
      }
    }
    default: {
      dev_->setupError(req);
      return 1;
    }
  }

  return 0;
}

int DfuClassDriver::handleDfuRequest(SetupRequest* req) {
  switch (req->bRequest) {
    case detail::DFU_DETACH: {
      return handleDfuDetach(req);
    }
    case detail::DFU_DNLOAD: {
      return handleDfuDnload(req);
    }
    case detail::DFU_UPLOAD: {
      return handleDfuUpload(req);
    }
    case detail::DFU_GETSTATUS: {
      return handleDfuGetStatus(req);
    }
    case detail::DFU_CLRSTATUS: {
      return handleDfuClrStatus(req);
    }
    case detail::DFU_GETSTATE: {
      return handleDfuGetState(req);
    }
    case detail::DFU_ABORT: {
      return handleDfuAbort(req);
    }
    default: {
      dev_->setupError(req);
      break;
    }
  }
  return 1;
}

int DfuClassDriver::handleDfuDetach(SetupRequest* req) {
  /* Should we check state here?
   * In theory this request is only used to switch from app to DFU mode
   *
   * Let's just in case use the same states as in DFU_ABORT request
   */
  switch (state_) {
    case detail::dfuIDLE:
    case detail::dfuDNLOAD_SYNC:
    case detail::dfuDNLOAD_IDLE:
    case detail::dfuMANIFEST_SYNC:
    case detail::dfuUPLOAD_IDLE: {
      setState(detail::dfuIDLE);
      /* When bit 3 in bmAttributes (bitWillDetach) is set the device
       * will generate a detach-attach sequence on
       * the bus when it sees this request.
       */

      dev_->setupReply(req, nullptr, 0);

      /* We always set this bit to 1 */
      dev_->detach();
      dev_->attach();
      break;
    }
    default: {
      /* Transition not defined */
      setError(detail::errUNKNOWN);
      break;
    }
  }

  return 0;
}

int DfuClassDriver::handleDfuDnload(SetupRequest* req) {
  switch (state_) {
    case detail::dfuIDLE:
    case detail::dfuDNLOAD_IDLE: {
      req_ = *req;
      if (req_.wLength > 0) {
        /* Normal request */
        setState(detail::dfuDNLOAD_SYNC);
        /* Data stage */
        dev_->setupReceive(req, transferBuf_, req_.wLength);
      } else {
        /* Leave request */
        setState(detail::dfuMANIFEST_SYNC);
        dev_->setupReply(req, nullptr, 0);
      }
      break;
    }

    default: {
      setError(detail::errUNKNOWN);
      break;
    }
  }
  return 0;
}

int DfuClassDriver::handleDfuUpload(SetupRequest* req) {
  switch (state_) {
    case detail::dfuIDLE:
    case detail::dfuUPLOAD_IDLE: {
      req_ = *req;
      if (req_.wValue == 0) {
        /* DfuSe command */
        transferBuf_[0] = detail::DFUSE_COMMAND_GET_COMMAND;
        transferBuf_[1] = detail::DFUSE_COMMAND_SET_ADDRESS_POINTER;
        transferBuf_[2] = detail::DFUSE_COMMAND_ERASE;
        setState(detail::dfuIDLE);
        dev_->setupReply(req, transferBuf_, 3);
      } else if (req_.wValue > 1) {
        /* Normal request */
        setState(detail::dfuUPLOAD_IDLE);
        /* Address = ((wBlockNum – 2) × wTransferSize) + Addres_Pointer, where:
         * - wTransferSize: length of the data buffer sent by the host
         * - wBlockNumber: value of the wValue parameter
         */
        uintptr_t addr = ((req_.wValue - 2) * USBD_DFU_TRANSFER_SIZE) + address_;
        auto ret = currentMal()->read(transferBuf_, addr, req_.wLength);
        if (ret != detail::OK) {
          setError(detail::errUNKNOWN);
        } else {
          setState(detail::dfuUPLOAD_IDLE);
          dev_->setupReply(req, transferBuf_, req_.wLength);
        }
      } else {
        /* Unknown */
        setError(detail::errUNKNOWN);
      }
      break;
    }

    default: {
      setError(detail::errUNKNOWN);
      break;
    }
  }
  return 0;
}

int DfuClassDriver::handleDfuGetStatus(SetupRequest* req) {
  const auto state = state_;

  /* Special handling for these states */
  switch (state) {
    case detail::dfuDNLOAD_SYNC: {
      if (req_.wLength > 0) {
        /* There is either a block pending to be written, or
         * a pending erase request
         */
        setState(detail::dfuDNBUSY);
        /* Ask MAL to update bwPollTimeout depending on the current DfuSe command */
        currentMal()->getStatus(&status_, dfuseCmd_);
      } else {
        /* Block complete */
        setState(detail::dfuDNLOAD_IDLE);
      }
      break;
    }

    case detail::dfuMANIFEST_SYNC: {
      setState(detail::dfuMANIFEST);
      setStatus(detail::OK);
      break;
    }

    case detail::dfuMANIFEST: {
      /* We want to report dfuMANIFEST state */
      setState(detail::dfuMANIFEST_WAIT_RESET);
      break;
    }
  }

  switch (state) {
    /* Imitate fall-through from the previous switch-case */
    case detail::dfuDNLOAD_SYNC:
    case detail::dfuMANIFEST_SYNC:
    case detail::dfuMANIFEST:

    case detail::appIDLE:
    case detail::appDETACH:
    case detail::dfuIDLE:
    case detail::dfuDNLOAD_IDLE:
    case detail::dfuUPLOAD_IDLE:
    case detail::dfuERROR: {
      dev_->setupReply(req, (const uint8_t*)&status_, sizeof(status_));
      break;
    }

    default: {
      /* Transition not defined */
      setError(detail::errUNKNOWN);
      break;
    }
  }

  return 0;
}

int DfuClassDriver::handleDfuClrStatus(SetupRequest* req) {
  if (state_ == detail::dfuERROR) {
    /* Clear error */
    setState(detail::dfuIDLE);
    setStatus(detail::OK);
    dev_->setupReply(req, nullptr, 0);
  } else {
    /* If the device receives a request, and there is no transition defined for
     * that request (for whatever state the device happens to be in when the request arrives),
     * then the device stalls the control pipe and enters the dfuERROR state
     */
    setError(detail::errUNKNOWN);
  }

  return 0;
}

int DfuClassDriver::handleDfuGetState(SetupRequest* req) {
  switch (state_) {
    case detail::dfuDNBUSY:
    case detail::dfuMANIFEST:
    case detail::dfuMANIFEST_WAIT_RESET: {
      /* Transition not defined */
      setError(detail::errUNKNOWN);
      break;
    }
    default: {
      /* Return current state */
      dev_->setupReply(req, &status_.bState, sizeof(status_.bState));
      break;
    }
  }
  return 0;
}

int DfuClassDriver::handleDfuAbort(SetupRequest* req) {
  switch (state_) {
    case detail::dfuIDLE:
    case detail::dfuDNLOAD_SYNC:
    case detail::dfuDNLOAD_IDLE:
    case detail::dfuMANIFEST_SYNC:
    case detail::dfuUPLOAD_IDLE: {
      setState(detail::dfuIDLE);
      setStatus(detail::OK);
      reset();
      dev_->setupReply(req, nullptr, 0);
      break;
    }
    default: {
      /* Transition not defined */
      setError(detail::errUNKNOWN);
      break;
    }
  }
  return 0;
}

void DfuClassDriver::setState(detail::DfuDeviceState st) {
  state_ = st;
  status_.bState = state_;
}

void DfuClassDriver::setStatus(detail::DfuDeviceStatus st) {
  status_.bStatus = st;
}

void DfuClassDriver::setError(detail::DfuDeviceStatus st, bool stall) {
  setStatus(st);
  setState(detail::dfuERROR);
  if (stall) {
    dev_->setupError(nullptr);
  }
}

DfuMal* DfuClassDriver::currentMal() {
  return mal_[alternativeSetting_];
}

int DfuClassDriver::setup(SetupRequest* req) {
  if ((req->bRequest == 0xee && req->bmRequestType == 0xc1 && req->wIndex == 0x0005) ||
      (req->bRequest == 0xee && req->bmRequestType == 0xc0 && req->wIndex == 0x0004)) {
    return handleMsftRequest(req);
  }

  switch (req->bmRequestTypeType) {
    case SetupRequest::TYPE_STANDARD: {
      return handleStandardRequest(req);
      break;
    }
    case SetupRequest::TYPE_CLASS: {
      return handleDfuRequest(req);
      break;
    }
  }

  dev_->setupError(req);
  return 0;
}

void DfuClassDriver::reset() {
  dfuseCmd_ = detail::DFUSE_COMMAND_NONE;
  memset(&req_, 0, sizeof(req_));
  /* address_ = 0; */
  resetCounter_ = 0;
}

int DfuClassDriver::dataIn(uint8_t ep, void* ex) {
  return -1;
}

int DfuClassDriver::dataOut(uint8_t ep, void* ex) {
  return -1;
}

int DfuClassDriver::startOfFrame() {
  return -1;
}

int DfuClassDriver::outDone(uint8_t ep, unsigned status) {
  return -1;
}

int DfuClassDriver::inDone(uint8_t ep, unsigned status) {
  switch (state_) {
    case detail::dfuDNBUSY: {
      /* We need to perform either a DfuSe command or a write operation */
      if (req_.wValue == 0 && req_.wLength > 0) {
        /* Special command */
        dfuseCmd_ = (detail::DfuseCommand)transferBuf_[0];
        switch (dfuseCmd_) {
          case detail::DFUSE_COMMAND_GET_COMMAND: {
            break;
          }
          case detail::DFUSE_COMMAND_SET_ADDRESS_POINTER: {
            if (req_.wLength == sizeof(uint32_t) + 1) {
              address_ = *((uint32_t*)(transferBuf_ + 1));
              setState(detail::dfuDNLOAD_IDLE);
              setStatus(detail::OK);
            } else{
              setError(detail::errUNKNOWN);
            }
            break;
          }
          case detail::DFUSE_COMMAND_ERASE: {
            if (req_.wLength == sizeof(uint32_t) + 1) {
              uintptr_t addr = *((uint32_t*)(transferBuf_ + 1));
              auto ret = currentMal()->erase(addr, 0);
              if (ret != detail::OK) {
                setError(detail::errUNKNOWN);
              } else {
                setState(detail::dfuDNLOAD_IDLE);
                setStatus(detail::OK);
              }
            } else if (req_.wLength == 1) {
              /* Mass-erase unsupported */
              setError(detail::errTARGET);
            } else {
              setError(detail::errUNKNOWN);
            }
            break;
          }
          case detail::DFUSE_COMMAND_READ_UNPROTECT: {
            /* Unsupported */
            setError(detail::errUNKNOWN);
            break;
          }
          default: {
            /* Unsupported command */
            setError(detail::errSTALLEDPKT);
            break;
          }
        }
      } else if (req_.wValue > 1) {
        /* Normal write */
        /* Address = ((wBlockNum – 2) × wTransferSize) + Addres_Pointer, where:
         * - wTransferSize: length of the data buffer sent by the host
         * - wBlockNumber: value of the wValue parameter
         */
        uintptr_t addr = ((req_.wValue - 2) * USBD_DFU_TRANSFER_SIZE) + address_;
        auto ret = currentMal()->write(transferBuf_, addr, req_.wLength);
        if (ret != detail::OK) {
          setError(detail::errUNKNOWN);
        } else {
          setState(detail::dfuDNLOAD_IDLE);
          setStatus(detail::OK);
        }
      } else {
        /* Error */
        setError(detail::errTARGET);
        reset();
      }
      break;
    }
    case detail::dfuMANIFEST: {
      /* Just reported dfuMANIFEST state */
      /* Transition to dfuMANIFEST_WAIT_RESET */
      setState(detail::dfuMANIFEST_WAIT_RESET);
      resetCounter_ = USBD_DFU_POLL_TIMEOUT;
      break;
    }
  }
  return 0;
}

const uint8_t* DfuClassDriver::getConfigurationDescriptor(uint16_t* length) {
  *length = *((uint16_t*)(cfgDesc_ + 2));
  return cfgDesc_;
}

const uint8_t* DfuClassDriver::getStringDescriptor(unsigned index, uint16_t* length, bool* conv) {
  if (index >= (particle::usbd::Device::STRING_IDX_INTERFACE + 1) &&
      index <= (particle::usbd::Device::STRING_IDX_INTERFACE + 1 + USBD_DFU_MAX_CONFIGURATIONS)) {
    const unsigned idx = index - (particle::usbd::Device::STRING_IDX_INTERFACE + 1);
    auto m = mal_[idx];
    if (m) {
      *conv = true;
      auto s = m->getString();
      if (s != nullptr) {
        *length = strlen(s);
      }
      return (const uint8_t*)s;
    }
  }
  return nullptr;
}

bool DfuClassDriver::registerMal(unsigned index, DfuMal* mal) {
  if (index < USBD_DFU_MAX_CONFIGURATIONS) {
    mal_[index] = mal;

    return true;
  }

  return false;
}

bool DfuClassDriver::checkReset() {
  bool res = false;
  int st = HAL_disable_irq();
  if (state_ == detail::dfuMANIFEST_WAIT_RESET) {
    if (resetCounter_ && !--resetCounter_) {
      res = true;
    }
  }
  HAL_enable_irq(st);
  return res;
}
