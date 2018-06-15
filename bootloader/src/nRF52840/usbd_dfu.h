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

#ifndef USBD_DFU_H
#define USBD_DFU_H

#include "usbd_device.h"

#include "usbd_wcid.h"

#ifndef USBD_DFU_MAX_CONFIGURATIONS
#define USBD_DFU_MAX_CONFIGURATIONS (3)
#endif /* USBD_DFU_MAX_CONFIGURATIONS */

#ifndef USBD_DFU_TRANSFER_SIZE
#define USBD_DFU_TRANSFER_SIZE (4096)
#endif /* USBD_DFU_TRANSFER_SIZE */

#ifndef USBD_DFU_POLL_TIMEOUT
#define USBD_DFU_POLL_TIMEOUT (500)
#endif /* USBD_DFU_POLL_TIMEOUT */

namespace particle { namespace usbd { namespace dfu {

namespace detail {

static const size_t DFU_CONFIGURATION_DESCRIPTOR_MAX_LENGTH = 9 + USBD_DFU_MAX_CONFIGURATIONS * 9 + 9;

/* Table 4.4 DFU Mode Interface Descriptor,
 * USB Device Firmware Upgrade Specification, Revision 1.1 */
#define USBD_DFU_IF_DESCRIPTOR(idx) \
  0x09,  /* bLength: Size of this descriptor, in bytes */        \
  0x04,  /* bDescriptorType: INTERFACE descriptor type */        \
  0x00,  /* bInterfaceNumber: Number of this interface */        \
  (idx), /* bAlternateSetting: Alternate setting */              \
  0x00,  /* bNumEndpoints: Only the control pipe is used */      \
  0xfe,  /* bInterfaceClass: Application Specific Class Code */  \
  0x01,  /* bInterfaceSubClass: Device Firmware Upgrade Code */  \
  0x02,  /* bInterfaceProtocol: DFU mode protocol */             \
  particle::usbd::Device::STRING_IDX_INTERFACE + (idx) + 1       \
                                     /* iInterface: Index of */  \
                                     /* string descriptor for */ \
                                     /* this interface */        \

static const uint8_t DFU_FUNCTIONAL_DESCRIPTOR_TYPE = 0x21;

#define USBD_DFU_FUNCTIONAL_DESCRIPTOR()                              \
  0x09,       /* bLength */                                           \
              /* bDescriptorType: DFU FUNCTIONAL descriptor type. */  \
  particle::usbd::dfu::detail::DFU_FUNCTIONAL_DESCRIPTOR_TYPE,        \
  0b00001011, /* bmAttributes: DFU attributes */                      \
              /* Bit 7..4: reserved */                                \
              /* Bit 3: device will perform a bus */                  \
              /* detach-attach sequence when it */                    \
              /* receives a DFU_DETACH request. */                    \
              /* The host must not issue a USB */                     \
              /* Reset. (bitWillDetach) */                            \
              /* 0 = no */                                            \
              /* 1 = yes */                                           \
              /* Bit 2: device is able to communicate */              \
              /* via USB after Manifestation phase. */                \
              /* (bitManifestationTolerant) */                        \
              /* 0 = no, must see bus reset */                        \
              /* 1 = yes */                                           \
              /* Bit 1: upload capable (bitCanUpload) */              \
              /* 0 = no */                                            \
              /* 1 = yes */                                           \
              /* Bit 0: download capable */                           \
              /* (bitCanDnload) */                                    \
              /* 0 = no */                                            \
              /* 1 = yes */                                           \
  0xff,       /* wDetachTimeout */                                    \
  0x00,                                                               \
  ((uint8_t)(USBD_DFU_TRANSFER_SIZE)),                                \
  ((uint8_t)(USBD_DFU_TRANSFER_SIZE >> 8)), /* wTransferSize */       \
  0x1a,       /* bcdDFUVersion 1.1a (DfuSe) */                        \
  0x01

/* 3. Requests, USB Device Firmware Upgrade Specification, Revision 1.1 */
enum DfuRequestType {
                     /* | wValue    | wIndex    | wLength | Data     | */
                     /* +-----------+-----------+---------+----------+ */
  DFU_DETACH    = 0, /* | wTimeout  | Interface | Zero    | None     | */
  DFU_DNLOAD    = 1, /* | wBlockNum | Interface | Length  | Firmware | */
  DFU_UPLOAD    = 2, /* | Zero      | Interface | Length  | Firmware | */
  DFU_GETSTATUS = 3, /* | Zero      | Interface | 6       | Status   | */
  DFU_CLRSTATUS = 4, /* | Zero      | Interface | Zero    | None     | */
  DFU_GETSTATE  = 5, /* | Zero      | Interface | 1       | State    | */
  DFU_ABORT     = 6  /* | Zero      | Interface | Zero    | None     | */
};

/* 6.1.2 DFU_GETSTATUS Request, USB Device Firmware Upgrade Specification, Revision 1.1 */
enum DfuDeviceStatus {
  OK              = 0x00, /* No error condition is present. */
  errTARGET       = 0x01, /* File is not targeted for use by this device. */
  errFILE         = 0x02, /* File is for this device but fails some vendor-specific
                           * verification test. */
  errWRITE        = 0x03, /* Device is unable to write memory. */
  errERASE        = 0x04, /* Memory erase function failed. */
  errCHECK_ERASED = 0x05, /* Memory erase check failed. */
  errPROG         = 0x06, /* Program memory function failed. */
  errVERIFY       = 0x07, /* Programmed memory failed verification. */
  errADDRESS      = 0x08, /* Cannot program memory due to received address that is
                           * out of range. */
  errNOTDONE      = 0x09, /* Received DFU_DNLOAD with wLength = 0, but device does not
                           * think it has all of the data yet. */
  errFIRMWARE     = 0x0A, /* Device’s firmware is corrupt. It cannot return to run-time
                           * (non-DFU) operations. */
  errVENDOR       = 0x0B, /* iString indicates a vendor-specific error. */
  errUSBR         = 0x0C, /* Device detected unexpected USB reset signaling. */
  errPOR          = 0x0D, /* Device detected unexpected power on reset. */
  errUNKNOWN      = 0x0E, /* Something went wrong, but the device does not know what
                           * it was */
  errSTALLEDPKT   = 0x0F, /* Device stalled an unexpected request. */
};

/* 6.1.2 DFU_GETSTATUS Request, USB Device Firmware Upgrade Specification, Revision 1.1 */
enum DfuDeviceState {
  appIDLE                 = 0, /* Device is running its normal application. */
  appDETACH               = 1, /* Device is running its normal application, has received the
                                * DFU_DETACH request, and is waiting for a USB reset. */
  dfuIDLE                 = 2, /* Device is operating in the DFU mode and is waiting for
                                * requests. */
  dfuDNLOAD_SYNC          = 3, /* Device has received a block and is waiting for the host to
                                * solicit the status via DFU_GETSTATUS. */
  dfuDNBUSY               = 4, /* Device is programming a control-write block into its
                                nonvolatile memories. */
  dfuDNLOAD_IDLE          = 5, /* Device is processing a download operation. Expecting
                                * DFU_DNLOAD requests. */
  dfuMANIFEST_SYNC        = 6, /* Device has received the final block of firmware from the host
                                * and is waiting for receipt of DFU_GETSTATUS to begin the
                                * Manifestation phase; or device has completed the
                                * Manifestation phase and is waiting for receipt of
                                * DFU_GETSTATUS. (Devices that can enter this state after
                                * the Manifestation phase set bmAttributes bit
                                * bitManifestationTolerant to 1.) */
  dfuMANIFEST             = 7, /* Device is in the Manifestation phase. (Not all devices will be
                                * able to respond to DFU_GETSTATUS when in this state.) */
  dfuMANIFEST_WAIT_RESET  = 8, /* Device has programmed its memories and is waiting for a
                                * USB reset or a power on reset. (Devices that must enter
                                * this state clear bitManifestationTolerant to 0.) */
  dfuUPLOAD_IDLE          = 9, /* The device is processing an upload operation. Expecting
                                * DFU_UPLOAD requests. */
  dfuERROR                = 10 /* An error has occurred. Awaiting the DFU_CLRSTATUS
                                * request. */
};

/* DFU with ST Microsystems extensions */
enum DfuseCommand {
  DFUSE_COMMAND_NONE                = 0xff,
  DFUSE_COMMAND_GET_COMMAND         = 0x00,
  DFUSE_COMMAND_SET_ADDRESS_POINTER = 0x21,
  DFUSE_COMMAND_ERASE               = 0x41,
  DFUSE_COMMAND_READ_UNPROTECT      = 0x92
};

#pragma pack(push, 1)
struct DfuGetStatus {
  uint8_t bStatus;
  uint8_t bwPollTimeout[3];
  uint8_t bState;
  uint8_t iString;
};
#pragma pack(pop)

} /* namespace detail */

class DfuMal {
public:
  DfuMal() = default;
  virtual ~DfuMal() = default;

  virtual int init() = 0;
  virtual int deInit() = 0;
  virtual bool validate(uintptr_t addr, size_t len) = 0;
  virtual int read(uint8_t* buf, uintptr_t addr, size_t len) = 0;
  virtual int write(const uint8_t* buf, uintptr_t addr, size_t len) = 0;
  virtual int erase(uintptr_t addr, size_t len) = 0;
  virtual int getStatus(detail::DfuGetStatus* status, dfu::detail::DfuseCommand cmd) = 0;
  virtual const char* getString() = 0;
};

class DfuClassDriver : public particle::usbd::ClassDriver {
public:
  DfuClassDriver(particle::usbd::Device* dev);
  ~DfuClassDriver();

  virtual int init(unsigned cfgIdx) override;
  virtual int deInit(unsigned cfgIdx) override;
  virtual int setup(particle::usbd::SetupRequest* req) override;
  virtual int dataIn(uint8_t ep, void* ex) override;
  virtual int dataOut(uint8_t ep, void* ex) override;
  virtual int startOfFrame() override;
  virtual int outDone(uint8_t ep, unsigned status) override;
  virtual int inDone(uint8_t ep, unsigned status) override;
  virtual const uint8_t* getConfigurationDescriptor(uint16_t* length) override;
  virtual const uint8_t* getStringDescriptor(unsigned index, uint16_t* length, bool* conv) override;

  bool registerMal(unsigned index, DfuMal* mal);

  bool checkReset();

private:
  int handleMsftRequest(particle::usbd::SetupRequest* req);
  int handleStandardRequest(particle::usbd::SetupRequest* req);
  int handleDfuRequest(particle::usbd::SetupRequest* req);
  int handleDfuDetach(particle::usbd::SetupRequest* req);
  int handleDfuDnload(particle::usbd::SetupRequest* req);
  int handleDfuUpload(particle::usbd::SetupRequest* req);
  int handleDfuGetStatus(particle::usbd::SetupRequest* req);
  int handleDfuClrStatus(particle::usbd::SetupRequest* req);
  int handleDfuGetState(particle::usbd::SetupRequest* req);
  int handleDfuAbort(particle::usbd::SetupRequest* req);

  void reset();
  void setState(detail::DfuDeviceState st);
  void setStatus(detail::DfuDeviceStatus st);
  void setError(detail::DfuDeviceStatus st, bool stall = true);
  DfuMal* currentMal();

private:
  Device* dev_;
  uint8_t alternativeSetting_ = 0;
  detail::DfuDeviceState state_;
  detail::DfuGetStatus status_ = {};
  uintptr_t address_ = 0;
  detail::DfuseCommand dfuseCmd_;
  particle::usbd::SetupRequest req_;
  uint8_t transferBuf_[USBD_DFU_TRANSFER_SIZE] __attribute__((aligned(4)));

  DfuMal* mal_[USBD_DFU_MAX_CONFIGURATIONS] = {};

  unsigned resetCounter_ = 0;

  /* Extended Compat ID OS Descriptor */
  static constexpr const uint8_t msftExtCompatIdOsDescr_[] = {
    USB_WCID_EXT_COMPAT_ID_OS_DESCRIPTOR(
      0x00,
      USB_WCID_DATA('W', 'I', 'N', 'U', 'S', 'B', '\0', '\0'),
      USB_WCID_DATA(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00)
    )
  };

  /* Extended Properties OS Descriptor */
  static constexpr const uint8_t msftExtPropOsDescr_[] = {
    USB_WCID_EXT_PROP_OS_DESCRIPTOR(
      USB_WCID_DATA(
        /* bPropertyData "{37fb5f90-1a34-4929-933b-8a27e1850033}" */
        '{', 0x00, '3', 0x00, '7', 0x00, 'f', 0x00, 'b', 0x00,
        '5', 0x00, 'f', 0x00, '9', 0x00, '0', 0x00, '-', 0x00,
        '1', 0x00, 'a', 0x00, '3', 0x00, '4', 0x00, '-', 0x00,
        '4', 0x00, '9', 0x00, '2', 0x00, '9', 0x00, '-', 0x00,
        '9', 0x00, '3', 0x00, '3', 0x00, 'b', 0x00, '-', 0x00,
        '8', 0x00, 'a', 0x00, '2', 0x00, '7', 0x00, 'e', 0x00,
        '1', 0x00, '8', 0x00, '5', 0x00, '0', 0x00, '0', 0x00,
        '3', 0x00, '3', 0x00, '}'
      )
    )
  };

  uint8_t cfgDesc_[detail::DFU_CONFIGURATION_DESCRIPTOR_MAX_LENGTH] = {
    0x09,         /* bLength: Configuation Descriptor size */
    Device::DESCRIPTOR_CONFIGURATION, /* bDescriptorType: Configuration */
    0x09 + 0x09 + USBD_DFU_MAX_CONFIGURATIONS * 0x09, /* wTotalLength: Bytes returned (provisional) */
    0x00,
    0x01,         /* bNumInterfaces: 1 interface*/
    0x01,         /* bConfigurationValue: Configuration value*/
    particle::usbd::Device::STRING_IDX_CONFIG,         /* iConfiguration: Index of string descriptor describing the configuration*/
    0xC0,         /* bmAttributes: bus powered and Supprts Remote Wakeup */
    0x32,         /* MaxPower 100 mA: this current is used for detecting Vbus*/
    USBD_DFU_IF_DESCRIPTOR(0),
    /* XXX: Ideally we should construct these during runtime */
#if USBD_DFU_MAX_CONFIGURATIONS >= 2
    USBD_DFU_IF_DESCRIPTOR(1),
#endif
#if USBD_DFU_MAX_CONFIGURATIONS >= 3
    USBD_DFU_IF_DESCRIPTOR(2),
#endif
#if USBD_DFU_MAX_CONFIGURATIONS >= 4
    USBD_DFU_IF_DESCRIPTOR(3),
#endif
#if USBD_DFU_MAX_CONFIGURATIONS >= 5
    USBD_DFU_IF_DESCRIPTOR(4),
#endif
    USBD_DFU_FUNCTIONAL_DESCRIPTOR()
  };
};

} } } /* namespace particle::usbd::dfu */

#endif /* USBD_DFU_H */
