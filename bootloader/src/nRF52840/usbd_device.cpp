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

/* nRF5 SDK always defines STATIC_ASSERT in app_util.h :(
 * This ensures that we do not redefine it
 */
#define NO_STATIC_ASSERT
#include "usbd_device.h"
#include "usbd_dfu.h"
#include "usbd_dfu_mal.h"
/* Important! Needs to be included before other headers */
#include "logging.h"
#include "hw_config.h"
#include "dfu_hal.h"
#include "usbd_wcid.h"

#include "nrf_drv_usbd.h"

#include "hal_irq_flag.h"

#include "nrf_drv_power.h"

#include <algorithm>
#include "nrf_delay.h"
#include "deviceid_hal.h"
#include "bytes2hexbuf.h"

using namespace particle::usbd;

namespace {

class NrfDevice : public Device {
public:
  static NrfDevice& instance() {
    static NrfDevice dev;
    return dev;
  }

  void init();
  void deInit();

  void setDeviceDescriptor(const uint8_t* desc, uint16_t len) {
    desc_ = desc;
    descLen_ = len;
  }

  virtual int registerClass(ClassDriver* drv);
  virtual int openEndpoint(unsigned ep, size_t pakSize, unsigned type) override;
  virtual int closeEndpoint(unsigned ep) override;
  virtual int flushEndpoint(unsigned ep) override;
  virtual int transferIn(unsigned ep, const uint8_t* ptr, size_t size) override;
  virtual int transferOut(unsigned ep, uint8_t* ptr, size_t size) override;

  virtual void setupReply(SetupRequest* r, const uint8_t* data, size_t size) override;
  virtual void setupReceive(SetupRequest* r, uint8_t* data, size_t size) override;
  virtual void setupError(SetupRequest* r) override;

  virtual void attach() override;
  virtual void detach() override;

protected:
  NrfDevice();
  ~NrfDevice();

private:
  static void eventHandlerCb(nrf_drv_usbd_evt_t const* const ev);
  void eventHandler(nrf_drv_usbd_evt_t const* const ev);

  static void powerUsbEventCb(nrf_drv_power_usb_evt_t event);
  void powerUsbEvent(nrf_drv_power_usb_evt_t event);

  void reset();
  void suspend();
  void resume();
  void wakeup();
  void setup();
  void setupDevice(SetupRequest* r);
  void setupInterface(SetupRequest* r);
  void setupEndpoint(SetupRequest* r);

  void getStatus(SetupRequest* r);
  void clearFeature(SetupRequest* r);
  void setFeature(SetupRequest* r);
  void setAddress(SetupRequest* r);
  void getDescriptor(SetupRequest* r);
  void setDescriptor(SetupRequest* r);
  void getConfiguration(SetupRequest* r);
  void setConfiguration(SetupRequest* r);

  int clearConfig(unsigned idx);
  int setConfig(unsigned idx);

  const uint8_t* getString(SetupRequest* r, uint16_t* len);

  const uint8_t* getDeviceDescriptor(uint16_t* len) {
    *len = descLen_;
    return desc_;
  }

  const uint8_t* getUnicodeString(const uint8_t* ascii, size_t len, uint16_t* outLen, bool noconv = false);
  const uint8_t* getUnicodeString(const char* ascii, size_t len, uint16_t* outLen, bool noconv = false) {
    return getUnicodeString((const uint8_t*)ascii, len, outLen, noconv);
  }

private:

  ClassDriver* drv_ = nullptr;
  uint16_t cfgStatus_ = 0;
  /* Default configuration = 0 */
  uint8_t cfgIdx_ = 0;
  const uint8_t defaultCfgIdx_ = 1;

  const uint8_t* desc_ = nullptr;
  uint16_t descLen_ = 0;

  uint8_t tmpDescriptorBuf_[256];
};

#define LOBYTE(x)  ((uint8_t)(x & 0x00FF))
#define HIBYTE(x)  ((uint8_t)((x & 0xFF00) >>8))

#define USBD_LANGID_STRING              "\x09\x04"  //U.S. English
#define USBD_MANUFACTURER_STRING        "Particle"

#if PLATFORM_ID == 12
# define USBD_PRODUCT_NAME              "Argon"
#elif PLATFORM_ID == 13
# define USBD_PRODUCT_NAME              "Boron"
#elif PLATFORM_ID == 14
# define USBD_PRODUCT_NAME              "Xenon"
#elif PLATFORM_ID == 22
# define USBD_PRODUCT_NAME              "A SoM"
#elif PLATFORM_ID == 23
# define USBD_PRODUCT_NAME              "B SoM"
#else
# define USBD_PRODUCT_NAME              "X SoM"
#endif
#define USBD_PRODUCT_STRING             USBD_PRODUCT_NAME " " "DFU Mode"
#define USBD_CONFIGURATION_STRING       USBD_PRODUCT_STRING
#define USBD_INTERFACE_STRING           USBD_PRODUCT_STRING

const uint8_t s_deviceDescriptor[] = {
  0x12,                       /*bLength */
  Device::DESCRIPTOR_DEVICE,  /*bDescriptorType*/
  0x00,                       /*bcdUSB */
  0x02,
  0x00,                       /*bDeviceClass*/
  0x00,                       /*bDeviceSubClass*/
  0x00,                       /*bDeviceProtocol*/
  NRF_DRV_USBD_EPSIZE,        /*bMaxPacketSize*/
  LOBYTE(USBD_VID_SPARK),     /*idVendor*/
  HIBYTE(USBD_VID_SPARK),     /*idVendor*/
  LOBYTE(USBD_PID_DFU),       /*idProduct*/
  HIBYTE(USBD_PID_DFU),       /*idProduct*/
  LOBYTE(0x0251),             /*bcdDevice (2.51) */
  HIBYTE(0x0251),             /*bcdDevice (2.51) */
  Device::STRING_IDX_MANUFACTURER, /*Index of manufacturer  string*/
  Device::STRING_IDX_PRODUCT,      /*Index of product string*/
  Device::STRING_IDX_SERIAL,       /*Index of serial number string*/
  0x01                        /*bNumConfigurations*/
};

/* MS OS String Descriptor */
const uint8_t USBD_MsftStrDesc[] = {
  USB_WCID_MS_OS_STRING_DESCRIPTOR(
    // "MSFT100"
    USB_WCID_DATA('M', '\0', 'S', '\0', 'F', '\0', 'T', '\0', '1', '\0', '0', '\0', '0', '\0'),
    0xee
  )
};


char* device_id_as_string(char* buf) {
  uint8_t deviceId[HAL_DEVICE_ID_SIZE] = {};
  unsigned deviceIdLen = HAL_device_ID(deviceId, sizeof(deviceId));
  bytes2hexbuf_lower_case(deviceId, deviceIdLen, buf);
  return buf;
}

} /* anonymous */

dfu::DfuClassDriver g_dfuInstance(&NrfDevice::instance());

/* FIXME: move somewhere else */
dfu::mal::InternalFlashMal g_internalFlashMal;
dfu::mal::DcdMal g_dcdMal;
dfu::mal::ExternalFlashMal g_externalFlashMal;

void HAL_DFU_USB_Init(void) {
  auto& dev = NrfDevice::instance();
  dev.setDeviceDescriptor(s_deviceDescriptor, sizeof(s_deviceDescriptor));

  /* Register DFU MALs */
  g_dfuInstance.registerMal(0, &g_internalFlashMal);
  g_dfuInstance.registerMal(1, &g_dcdMal);
  g_dfuInstance.registerMal(2, &g_externalFlashMal);
  dev.init();
}

void DFU_Check_Reset(void) {
  if (g_dfuInstance.checkReset()) {
    auto& dev = NrfDevice::instance();
    dev.deInit();
    Finish_Update();
  }
}

NrfDevice::NrfDevice()
    : Device() {
}

NrfDevice::~NrfDevice() {
}

void NrfDevice::init() {
  auto ret = nrf_drv_usbd_init(&eventHandlerCb);
  SPARK_ASSERT(ret == NRF_SUCCESS);

  nrf_drv_usbd_ep_max_packet_size_set(NRF_DRV_USBD_EPOUT0, NRF_DRV_USBD_EPSIZE);
  nrf_drv_usbd_ep_max_packet_size_set(NRF_DRV_USBD_EPIN0, NRF_DRV_USBD_EPSIZE);

  reset();

  const nrf_drv_power_usbevt_config_t config = {
      .handler = &NrfDevice::powerUsbEventCb
  };
  ret = nrf_drv_power_usbevt_init(&config);
  SPARK_ASSERT(ret == NRF_SUCCESS);

#if 0
  nrf_delay_us(10000);
  if (!nrf_drv_usbd_is_enabled()) {
    nrf_drv_usbd_enable();
    reset();
  }

  while (nrf_drv_power_usbstatus_get() == NRF_DRV_POWER_USB_STATE_CONNECTED) {
    /* Wait for regulator power up */
  }

  if (NRF_DRV_POWER_USB_STATE_READY == nrf_drv_power_usbstatus_get()) {
    if (!nrf_drv_usbd_is_started()) {
      attach();
    }
  } else {
    deInit();
  }
#endif /* 0 */
}

void NrfDevice::deInit() {
  /* FIXME */
  detach();
}

int NrfDevice::registerClass(ClassDriver* drv) {
  drv_ = drv;
  return 0;
}

int NrfDevice::openEndpoint(unsigned ep, size_t pakSize, unsigned type) {
  return -1;
}
int NrfDevice::closeEndpoint(unsigned ep) {
  return -1;
}
int NrfDevice::flushEndpoint(unsigned ep) {
  return -1;
}
int NrfDevice::transferIn(unsigned ep, const uint8_t* ptr, size_t size) {
  return -1;
}
int NrfDevice::transferOut(unsigned ep, uint8_t* ptr, size_t size) {
  return -1;
}

void NrfDevice::powerUsbEventCb(nrf_drv_power_usb_evt_t event) {
  auto& self = instance();
  self.powerUsbEvent(event);
}

void NrfDevice::powerUsbEvent(nrf_drv_power_usb_evt_t event) {
  switch (event) {
    case NRF_DRV_POWER_USB_EVT_DETECTED: {
      if (!nrf_drv_usbd_is_enabled()) {
          nrf_drv_usbd_enable();
      }
      break;
    }
    case NRF_DRV_POWER_USB_EVT_REMOVED: {
      reset();
      if (nrf_drv_usbd_is_started()) {
          nrf_drv_usbd_stop();
      }

      if (nrf_drv_usbd_is_enabled()) {
          nrf_drv_usbd_disable();
      }
      break;
    }
    case NRF_DRV_POWER_USB_EVT_READY: {
      if (!nrf_drv_usbd_is_started()) {
          nrf_drv_usbd_start(true);
      }
      break;
    }
  }
}

void NrfDevice::eventHandlerCb(nrf_drv_usbd_evt_t const* const ev) {
  auto& self = instance();
  self.eventHandler(ev);
}

void NrfDevice::eventHandler(nrf_drv_usbd_evt_t const* const ev) {
  switch (ev->type) {
    case NRF_DRV_USBD_EVT_SOF: {        /**< Start Of Frame event on USB bus detected */
      if (drv_) {
        drv_->startOfFrame();
      }
      break;
    }
    case NRF_DRV_USBD_EVT_RESET: {      /**< Reset condition on USB bus detected */
      reset();
      break;
    }
    case NRF_DRV_USBD_EVT_SUSPEND: {    /**< This device should go to suspend mode now */
      suspend();
      break;
    }
    case NRF_DRV_USBD_EVT_RESUME: {     /**< This device should resume from suspend now */
      resume();
      break;
    }
    case NRF_DRV_USBD_EVT_WUREQ: {      /**< Wakeup request - the USBD peripheral is ready to generate WAKEUP signal after exiting low power mode. */
      wakeup();
      break;
    }
    case NRF_DRV_USBD_EVT_SETUP: {      /**< Setup frame received and decoded */
      setup();
      break;
    }
    case NRF_DRV_USBD_EVT_EPTRANSFER: { /**<
                                         * For Rx (OUT: Host->Device):
                                         * 1. The packet has been received but there is no buffer prepared for transfer already.
                                         * 2. Whole transfer has been finished
                                         *
                                         * For Tx (IN: Device->Host):
                                         * The last packet from requested transfer has been transfered over USB bus and acknowledged
                                         */
      if (ev->data.eptransfer.ep & 0x80) {
        /* Direction - IN */
        if (ev->data.eptransfer.ep == NRF_DRV_USBD_EPIN0) {
          /* Special handling for Endpoint 0 */
          if (ev->data.eptransfer.status == NRF_USBD_EP_OK) {
            if (!nrf_drv_usbd_errata_154()) {
              /* Transfer ok - allow status stage */
              nrf_drv_usbd_setup_clear();
            }

            if (drv_) {
              drv_->inDone((uint8_t)ev->data.eptransfer.ep, 0);
            }
          } else if (NRF_USBD_EP_ABORTED == ev->data.eptransfer.status) {
            /* Ignore */
          } else {
            /* Transfer failed */
            nrf_drv_usbd_setup_stall();
          }
        } else {
          /* Other endpoints */
          if (drv_) {
            drv_->inDone((uint8_t)ev->data.eptransfer.ep, 0);
          }
        }
      } else {
        /* Direction - OUT */
        if (ev->data.eptransfer.ep == NRF_DRV_USBD_EPOUT0) {
          /* Special handling for Endpoint 0 */
          if (ev->data.eptransfer.status == NRF_USBD_EP_OK) {
            /* NOTE: Data values or size may be tested here to decide if clear or stall.
             * If errata 154 is present the data transfer is acknowledged by the hardware. */
            if (!nrf_drv_usbd_errata_154()) {
                /* Transfer ok - allow status stage */
                nrf_drv_usbd_setup_clear();
            }

            if (drv_) {
              drv_->outDone((uint8_t)ev->data.eptransfer.ep, 0);
            }
          } else if (ev->data.eptransfer.status == NRF_USBD_EP_ABORTED) {
            /* Ignore */
          } else {
            /* Transfer failed */
            nrf_drv_usbd_setup_stall();
          }
        } else {
          /* Other endpoints */
          if (drv_) {
            drv_->outDone((uint8_t)ev->data.eptransfer.ep, 0);
          }
        }
      }
      break;
    }
    case NRF_DRV_USBD_EVT_CNT: {        /**< Number of defined events */
      break;
    }
  }
}

void NrfDevice::reset() {
  nrf_drv_usbd_setup_clear();
  /* SetAddress for some reason is not forwarded to us, so default to ADDRESSED */
  setDeviceStatus(STATUS_ADDRESSED);
}

void NrfDevice::suspend() {
  setDeviceStatus(STATUS_SUSPENDED);
}

void NrfDevice::resume() {
  setDeviceStatus(STATUS_CONFIGURED);
}

void NrfDevice::wakeup() {
  setDeviceStatus(STATUS_CONFIGURED);
}

void NrfDevice::setup() {
  nrf_drv_usbd_setup_t setup;
  nrf_drv_usbd_setup_get(&setup);

  /* Convert to SetupRequest */
  SetupRequest r;
  static_assert(sizeof(SetupRequest) == sizeof(nrf_drv_usbd_setup_t), "SetupRequest and nrf_drv_usbd_setup_t are expected to be of the same size");
  memcpy(&r, &setup, sizeof(r));


  switch (r.bmRequestTypeRecipient) {
    case SetupRequest::RECIPIENT_DEVICE: {
      setupDevice(&r);
      break;
    }
    case SetupRequest::RECIPIENT_INTERFACE: {
      setupInterface(&r);
      break;
    }
    case SetupRequest::RECIPIENT_ENDPOINT: {
      setupEndpoint(&r);
      break;
    }
    default: {
      nrf_drv_usbd_setup_stall();
      break;
    }
  }
}

void NrfDevice::setupDevice(SetupRequest* r) {
  switch (r->bmRequestTypeType) {
    case SetupRequest::TYPE_STANDARD: {
      switch (r->bRequest) {
        case SetupRequest::REQUEST_GET_STATUS: {
          getStatus(r);
          break;
        }
        case SetupRequest::REQUEST_CLEAR_FEATURE: {
          clearFeature(r);
          break;
        }
        case SetupRequest::REQUEST_SET_FEATURE: {
          setFeature(r);
          break;
        }
        case SetupRequest::REQUEST_SET_ADDRESS: {
          setAddress(r);
          break;
        }
        case SetupRequest::REQUEST_GET_DESCRIPTOR: {
          getDescriptor(r);
          break;
        }
        case SetupRequest::REQUEST_SET_DESCRIPTOR: {
          setDescriptor(r);
          break;
        }
        case SetupRequest::REQUEST_GET_CONFIGURATION: {
          getConfiguration(r);
          break;
        }
        case SetupRequest::REQUEST_SET_CONFIGURATION: {
          setConfiguration(r);
          break;
        }
        default: {
          nrf_drv_usbd_setup_stall();
          break;
        }
      }
      break;
    }
    case SetupRequest::TYPE_VENDOR:
    case SetupRequest::TYPE_CLASS: {
      if (drv_) {
        drv_->setup(r);
      }
      break;
    }
    default: {
      nrf_drv_usbd_setup_stall();
      break;
    }
  }
}

void NrfDevice::setupInterface(SetupRequest* r) {
  switch (getDeviceStatus()) {
    case STATUS_CONFIGURED: {
      if (drv_) {
        drv_->setup(r);
      }
      break;
    }
    default: {
      nrf_drv_usbd_setup_stall();
      break;
    }
  }
}

void NrfDevice::setupEndpoint(SetupRequest* r) {
  uint8_t ep = (uint8_t)r->wIndex;
  switch (r->bRequest) {
    case SetupRequest::REQUEST_GET_STATUS: {
      switch (getDeviceStatus()) {
        case STATUS_ADDRESSED: {
          if ((ep & 0x7f) != 0x00) {
            nrf_drv_usbd_ep_stall((nrf_drv_usbd_ep_t)ep);
            setupError(r);
            break;
          }
        }
        /* Fall through */
        case STATUS_CONFIGURED: {
          uint16_t* tmp = (uint16_t*)tmpDescriptorBuf_;
          if (nrf_drv_usbd_ep_stall_check((nrf_drv_usbd_ep_t)ep)) {
            /* Halted */
            *tmp = 0x0001;
          } else {
            /* Active */
            *tmp = 0x0000;
          }
          setupReply(r, (const uint8_t*)tmpDescriptorBuf_, sizeof(uint16_t));
          break;
        }
        default: {
          setupError(r);
          break;
        }
      }
      break;
    }
    case SetupRequest::REQUEST_CLEAR_FEATURE: {
      switch (getDeviceStatus()) {
        case STATUS_ADDRESSED: {
          if ((ep & 0x7f) != 0x00) {
            nrf_drv_usbd_ep_stall((nrf_drv_usbd_ep_t)ep);
          }
          nrf_drv_usbd_setup_clear();
          break;
        }
        case STATUS_CONFIGURED: {
          if (r->wValue == FEATURE_EP_HALT) {
            if ((ep & 0x7f) != 0x00) {
              if (drv_) {
                nrf_drv_usbd_ep_stall_clear((nrf_drv_usbd_ep_t)ep);
                drv_->setup(r);
              }
            }
          }
          nrf_drv_usbd_setup_clear();
          break;
        }
        default: {
          setupError(r);
          break;
        }
      }
      break;
    }
    case SetupRequest::REQUEST_SET_FEATURE: {
      switch (getDeviceStatus()) {
        case STATUS_ADDRESSED: {
          if ((ep & 0x7f) != 0x00) {
            nrf_drv_usbd_ep_stall((nrf_drv_usbd_ep_t)ep);
          }
          nrf_drv_usbd_setup_clear();
          break;
        }
        case STATUS_CONFIGURED: {
          if (r->wValue == FEATURE_EP_HALT) {
            if ((ep & 0x7f) != 0x00) {
              nrf_drv_usbd_ep_stall((nrf_drv_usbd_ep_t)ep);
            }
          }
          nrf_drv_usbd_setup_clear();
          break;
        }
        default: {
          setupError(r);
          break;
        }
      }
      break;
    }
    default: {
      setupError(r);
      break;
    }
  }
}

void NrfDevice::getStatus(SetupRequest* r) {
  switch (getDeviceStatus()) {
    case STATUS_ADDRESSED:
    case STATUS_CONFIGURED: {
      /* FIXME: self-powered, remote wakeup */
      setupReply(r, (const uint8_t*)&cfgStatus_, sizeof(uint16_t));
      break;
    }
    default: {
      nrf_drv_usbd_setup_stall();
      break;
    }
  }
}

void NrfDevice::clearFeature(SetupRequest* r) {
  switch (getDeviceStatus()) {
    case STATUS_ADDRESSED:
    case STATUS_CONFIGURED: {
      if (r->wValue == FEATURE_DEVICE_REMOTE_WAKEUP) {
        /* FIXME */
        nrf_drv_usbd_setup_clear();
      } else {
        nrf_drv_usbd_setup_stall();
      }
      break;
    }
    default: {
      nrf_drv_usbd_setup_stall();
      break;
    }
  }
}

void NrfDevice::setFeature(SetupRequest* r) {
  switch (getDeviceStatus()) {
    case STATUS_ADDRESSED:
    case STATUS_CONFIGURED: {
      if (r->wValue == FEATURE_DEVICE_REMOTE_WAKEUP) {
        /* FIXME */
        nrf_drv_usbd_setup_clear();
      } else {
        nrf_drv_usbd_setup_stall();
      }
      break;
    }
    default: {
      nrf_drv_usbd_setup_stall();
      break;
    }
  }
}

void NrfDevice::setAddress(SetupRequest* r) {
  /* Should not be handled, handled by hardware. No stalling though! */
  /* Assumed we've been addressed */
  setDeviceStatus(STATUS_ADDRESSED);
}

void NrfDevice::getDescriptor(SetupRequest* r) {
  const uint8_t* desc = nullptr;
  uint16_t len = 0;

  switch (r->wValue >> 8) {
    case DESCRIPTOR_DEVICE: {
      desc = getDeviceDescriptor(&len);
      break;
    }

    case DESCRIPTOR_CONFIGURATION: {
      if (drv_) {
        desc = drv_->getConfigurationDescriptor(&len);
      }
      break;
    }

    case DESCRIPTOR_STRING: {
      desc = getString(r, &len);
      break;
    }
  }

  if (desc && len && r->wLength) {
    setupReply(r, desc, len);
  } else {
    nrf_drv_usbd_setup_stall();
  }
}

const uint8_t* NrfDevice::getString(SetupRequest* r, uint16_t* len) {
  /* FIXME */
  switch ((uint8_t)r->wValue) {
    case STRING_IDX_LANGID: {
      return getUnicodeString(USBD_LANGID_STRING, sizeof(USBD_LANGID_STRING) - 1, len, true);
    }
    case STRING_IDX_MANUFACTURER: {
      return getUnicodeString(USBD_MANUFACTURER_STRING, sizeof(USBD_MANUFACTURER_STRING) - 1, len);
    }
    case STRING_IDX_PRODUCT: {
      return getUnicodeString(USBD_PRODUCT_STRING, sizeof(USBD_PRODUCT_STRING) - 1, len);
    }
    case STRING_IDX_SERIAL: {
      char deviceid[HAL_DEVICE_ID_SIZE * 2 + 1] = {};
      return getUnicodeString(device_id_as_string(deviceid), sizeof(deviceid) - 1, len);
    }
    case STRING_IDX_CONFIG: {
      return getUnicodeString(USBD_CONFIGURATION_STRING, sizeof(USBD_CONFIGURATION_STRING) - 1, len);
    }
    case STRING_IDX_INTERFACE: {
      return getUnicodeString(USBD_INTERFACE_STRING, sizeof(USBD_INTERFACE_STRING) - 1, len);
    }
    case STRING_IDX_MSFT: {
      /* No conversion to UTF-16 */
      return getUnicodeString(USBD_MsftStrDesc, sizeof(USBD_MsftStrDesc), len, true);
    }
    default: {
      if (drv_) {
        bool conv = true;
        uint16_t l = 0;
        auto s = drv_->getStringDescriptor((uint8_t)r->wValue, &l, &conv);
        return getUnicodeString(s, l, len, !conv);
      }
      break;
    }
  }

  return nullptr;
}

const uint8_t* NrfDevice::getUnicodeString(const uint8_t* str, size_t len, uint16_t* outLen, bool noconv) {
  if (str == nullptr || len == 0) {
    *outLen = 0;
    return nullptr;
  }

  if (!noconv) {
    if ((len * 2) + 2 > sizeof(tmpDescriptorBuf_)) {
      len = (sizeof(tmpDescriptorBuf_) - 2) / 2;
    }

    tmpDescriptorBuf_[0] = (uint8_t)(len * 2 + 2);
    tmpDescriptorBuf_[1] = DESCRIPTOR_STRING;

    for (size_t i = 0; i < len; i++) {
      tmpDescriptorBuf_[2 + (i * 2)] = str[i];
      tmpDescriptorBuf_[2 + (i * 2) + 1] = 0x00;
    }
    *outLen = len * 2 + 2;
  } else {
    if (len + 2 > sizeof(tmpDescriptorBuf_)) {
      len = sizeof(tmpDescriptorBuf_) - 2;
    }

    tmpDescriptorBuf_[0] = (uint8_t)(len + 2);
    tmpDescriptorBuf_[1] = DESCRIPTOR_STRING;
    for (size_t i = 0; i < len; i++) {
      tmpDescriptorBuf_[2 + i] = str[i];
    }

    *outLen = len + 2;
  }
  return tmpDescriptorBuf_;
}

void NrfDevice::setDescriptor(SetupRequest* r) {
  /* UNUSED */
  nrf_drv_usbd_setup_stall();
}

void NrfDevice::getConfiguration(SetupRequest* r) {
  switch (getDeviceStatus()) {
    case STATUS_ADDRESSED: {
      setupReply(r, (const uint8_t*)&defaultCfgIdx_, sizeof(uint8_t));
      break;
    }
    case STATUS_CONFIGURED: {
      setupReply(r, (const uint8_t*)&cfgIdx_, sizeof(uint8_t));
      break;
    }
    default: {
      nrf_drv_usbd_setup_stall();
      break;
    }
  }
}

void NrfDevice::setConfiguration(SetupRequest* r) {
  switch (getDeviceStatus()) {
    case STATUS_ADDRESSED: {
      if (r->wValue) {
        setDeviceStatus(STATUS_CONFIGURED);
        if (setConfig(r->wValue)) {
          /* Failed */
          nrf_drv_usbd_setup_stall();
          break;
        }
      }
      nrf_drv_usbd_setup_clear();
      break;
    }
    case STATUS_CONFIGURED: {
      if (r->wValue == 0) {
        /* Clear configuration */
        setDeviceStatus(STATUS_ADDRESSED);
        clearConfig(r->wValue);
      } else if (r->wValue != cfgIdx_) {
        if (setConfig(r->wValue)) {
          /* Failed */
          nrf_drv_usbd_setup_stall();
          break;
        }
      } else {
        /* Same configuration */
      }
      nrf_drv_usbd_setup_clear();
      break;
    }

    default: {
      nrf_drv_usbd_setup_stall();
      break;
    }
  }
}

int NrfDevice::setConfig(unsigned idx) {
  if (drv_) {
    return drv_->init(idx);
  }

  return 1;
}

int NrfDevice::clearConfig(unsigned idx) {
  if (drv_) {
    return drv_->deInit(idx);
  }

  return 1;
}

void NrfDevice::setupReply(SetupRequest* r, const uint8_t* data, size_t size) {
  if (r->wLength > 0) {
    size_t txSize = std::min((uint16_t)size, r->wLength);
    bool zlpRequired = (size < r->wLength) &&
                       (0 == (size % nrf_drv_usbd_ep_max_packet_size_get(NRF_DRV_USBD_EPIN0)));
    nrf_drv_usbd_transfer_t transfer = {
        .p_data = {
          .tx = data
        },
        .size = txSize,
        .flags = (uint32_t)(zlpRequired ? NRF_DRV_USBD_TRANSFER_ZLP_FLAG : 0)
    };
    ret_code_t ret = nrf_drv_usbd_ep_transfer(NRF_DRV_USBD_EPIN0, &transfer);
    ASSERT(ret == NRF_SUCCESS);
  } else {
    nrf_drv_usbd_setup_clear();
  }
}

void NrfDevice::setupReceive(SetupRequest* r, uint8_t* data, size_t size) {
  if (r->wLength > 0) {
    nrf_drv_usbd_setup_data_clear();
    size_t rxSize = std::min((uint16_t)size, r->wLength);
    nrf_drv_usbd_transfer_t transfer = {
      .p_data = {.rx = data},
      .size = rxSize
    };
    ret_code_t ret = nrf_drv_usbd_ep_transfer(NRF_DRV_USBD_EPOUT0, &transfer);
    ASSERT(ret == NRF_SUCCESS);
  } else {
    nrf_drv_usbd_setup_clear();
  }
}

void NrfDevice::setupError(SetupRequest* r) {
  nrf_drv_usbd_setup_stall();
}

void NrfDevice::attach() {
  nrf_drv_usbd_start(true);
}

void NrfDevice::detach() {
  nrf_drv_usbd_stop();
}
