/**
 ******************************************************************************
 * @file    usbd_composite.c
 * @author  Andrey Tolstoy
 * @version V1.0.0
 * @date    28-Feb-2016
 * @brief
 ******************************************************************************
  Copyright (c) 2013-2016 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#include <string.h>
#include "usbd_composite.h"
#include "usbd_desc.h"
#include "usbd_req.h"
#include "debug.h"
#include "interrupts_hal.h"
#include "logging.h"
#include "usbd_wcid.h"
#include "usbd_desc_device.h"

LOG_SOURCE_CATEGORY("usb.composite")

extern void HAL_USB_Vendor_Interface_SOF(void* pdev);

#define USBD_COMPOSITE_USRSTR_BASE 0x09

static USBD_Composite_Class_Data s_Class_Entries[USBD_COMPOSITE_MAX_CLASSES] = { {0} };
static uint32_t s_Classes_Count = 0;
static USBD_Composite_Class_Data* s_Classes = NULL;
static uint8_t s_Initialized = 0;
static USBD_Composite_Configuration_Callback s_Configuration_Callback = NULL;

static uint16_t USBD_Build_CfgDesc(uint8_t* buf, uint8_t speed, uint8_t other);

static uint8_t USBD_Composite_Init(void* pdev, uint8_t cfgidx);
static uint8_t USBD_Composite_DeInit(void* pdev, uint8_t cfgidx);
static uint8_t USBD_Composite_Setup(void* pdev, USB_SETUP_REQ* req);
static uint8_t USBD_Composite_EP0_TxSent(void* pdev);
static uint8_t USBD_Composite_EP0_RxReady(void* pdev);
static uint8_t USBD_Composite_DataIn(void* pdev, uint8_t epnum);
static uint8_t USBD_Composite_DataOut(void* pdev, uint8_t epnum);
static uint8_t USBD_Composite_SOF(void *pdev);
// static uint8_t USBD_Composite_IsoINIncomplete(void *pdev);
// static uint8_t USBD_Composite_IsoOUTIncomplete(void *pdev);
static uint8_t* USBD_Composite_GetConfigDescriptor(uint8_t speed, uint16_t *length);

#ifdef USE_USB_OTG_HS
static uint8_t* USBD_Composite_GetOtherConfigDescriptor(uint8_t speed, uint16_t *length);
#endif

static uint8_t* USBD_Composite_GetUsrStrDescriptor(uint8_t speed, uint8_t index, uint16_t *length);

static USBD_Class_cb_TypeDef USBD_Composite_cb = {
  USBD_Composite_Init,
  USBD_Composite_DeInit,
  USBD_Composite_Setup,
  USBD_Composite_EP0_TxSent,
  USBD_Composite_EP0_RxReady,
  USBD_Composite_DataIn,
  USBD_Composite_DataOut,
  USBD_Composite_SOF,
  NULL, // USBD_Composite_IsoINIncomplete,
  NULL, // USBD_Composite_IsoOUTIncomplete,
  USBD_Composite_GetConfigDescriptor,
#ifdef USB_OTG_HS_CORE
  USBD_Composite_GetOtherConfigDescriptor,
#endif
#ifdef USB_SUPPORT_USER_STRING_DESC
  USBD_Composite_GetUsrStrDescriptor
#endif
};

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
__ALIGN_BEGIN static uint8_t USBD_Composite_CfgDesc[USBD_COMPOSITE_CFGDESC_MAX_LENGTH] __ALIGN_END = {0};

static const uint8_t USBD_Composite_CfgDescHeaderTemplate[USBD_COMPOSITE_CFGDESC_HEADER_LENGTH] = {
  0x09,                                          /* bLength */
  USB_CONFIGURATION_DESCRIPTOR_TYPE,             /* bDescriptorType */
  0x00, 0x00,                                    /* wTotalLength TEMPLATE */
  0x00,                                          /* bNumInterfaces TEMPLATE */
  0x01,                                          /* bConfigurationValue */
  USBD_IDX_CONFIG_STR,                           /* iConfiguration */
  0x80,                                          /* bmAttirbutes (Bus powered) */
  // 0xFA                                           /* bMaxPower (500mA) */
  0x32                                           /* bMaxPower (100mA) */
};

static const uint8_t USBD_Composite_VendorInterface[] = {
  /* Vendor-specific interface #2 */
  0x09,                                          /* bLength: Interface Descriptor size */
  USB_INTERFACE_DESCRIPTOR_TYPE,                 /* bDescriptorType: Interface descriptor type */
  0x02,                                          /* bInterfaceNumber: Number of Interface */
  0x00,                                          /* bAlternateSetting: Alternate setting */
  0x00,                                          /* bNumEndpoints */
  0xff,                                          /* bInterfaceClass: Vendor */
  0xff,                                          /* bInterfaceSubClass */
  0xff,                                          /* nInterfaceProtocol */
  USBD_COMPOSITE_USRSTR_BASE                     /* iInterface: Index of string descriptor */
};

/* MS OS String Descriptor */
static const uint8_t USBD_Composite_MsftStrDesc[] = {
  USB_WCID_MS_OS_STRING_DESCRIPTOR(
    // "MSFT100"
    USB_WCID_DATA('M', '\0', 'S', '\0', 'F', '\0', 'T', '\0', '1', '\0', '0', '\0', '0', '\0'),
    0xee
  )
};

/* Extended Compat ID OS Descriptor */
static const uint8_t USBD_Composite_MsftExtCompatIdOsDescr[] = {
  USB_WCID_EXT_COMPAT_ID_OS_DESCRIPTOR(
    0x02,
    USB_WCID_DATA('W', 'I', 'N', 'U', 'S', 'B', '\0', '\0'),
    USB_WCID_DATA(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00)
  )
};

/* Extended Properties OS Descriptor */
static const uint8_t USBD_Composite_MsftExtPropOsDescr[] = {
  USB_WCID_EXT_PROP_OS_DESCRIPTOR(
    USB_WCID_DATA(
        /* bPropertyData "{20b6cfa4-6dc7-468a-a8db-faa7c23ddea5}" */
        '{', 0x00, '2', 0x00, '0', 0x00, 'b', 0x00,
        '6', 0x00, 'c', 0x00, 'f', 0x00, 'a', 0x00,
        '4', 0x00, '-', 0x00, '6', 0x00, 'd', 0x00,
        'c', 0x00, '7', 0x00, '-', 0x00, '4', 0x00,
        '6', 0x00, '8', 0x00, 'a', 0x00, '-', 0x00,
        'a', 0x00, '8', 0x00, 'd', 0x00, 'b', 0x00,
        '-', 0x00, 'f', 0x00, 'a', 0x00, 'a', 0x00,
        '7', 0x00, 'c', 0x00, '2', 0x00, '3', 0x00,
        'd', 0x00, 'd', 0x00, 'e', 0x00, 'a', 0x00,
        '5', 0x00, '}'
    )
  )
};

USBD_Class_cb_TypeDef* USBD_Composite_Instance(USBD_Composite_Configuration_Callback cb) {
  s_Configuration_Callback = cb;
  return &USBD_Composite_cb;
}


static uint8_t USBD_Composite_Init(void* pdev, uint8_t cfgidx) {
  uint8_t status = USBD_OK;

  //LOG_DEBUG(TRACE, "Initializing (0x%x)", cfgidx);

  for(USBD_Composite_Class_Data* c = s_Classes; c != NULL; c = c->next) {
    if(c->active && c->cb->Init) {
      status += c->cb->Init(pdev, c, cfgidx);
    }
  }

  s_Initialized = 1;

  if (status == USBD_OK) {
    if (s_Configuration_Callback)
      s_Configuration_Callback(cfgidx);
  }

  return status;
}

static uint8_t USBD_Composite_DeInit(void* pdev, uint8_t cfgidx) {
  uint8_t status = USBD_OK;

  s_Initialized = 0;

  //LOG_DEBUG(TRACE, "DeInitializing (0x%x)", cfgidx);

  for(USBD_Composite_Class_Data* c = s_Classes; c != NULL; c = c->next) {
    if(c->active && c->cb->DeInit) {
      status += c->cb->DeInit(pdev, c, cfgidx);
    }
  }

  if (s_Configuration_Callback)
    s_Configuration_Callback(0);

  return status;
}

static uint8_t USBD_Composite_Setup(void* pdev, USB_SETUP_REQ* req) {
  if (req->bRequest == 0xee && req->bmRequest == 0b11000001 && req->wIndex == 0x0005) {
    return USBD_Composite_Handle_Msft_Request(pdev, req);
  }

  for(USBD_Composite_Class_Data* c = s_Classes; c != NULL; c = c->next) {
    if (c->active && req->wIndex >= c->firstInterface && req->wIndex < (c->firstInterface + c->interfaces)) {
      if(c->cb->Setup) {
        return c->cb->Setup(pdev, c, req);
      }
    }
  }

  return USBD_OK;
}

static uint8_t USBD_Composite_EP0_TxSent(void* pdev) {
  uint8_t status = USBD_OK;
  for(USBD_Composite_Class_Data* c = s_Classes; c != NULL; c = c->next) {
    if(c->active && c->cb->EP0_TxSent) {
      status += c->cb->EP0_TxSent(pdev, c);
    }
  }

  return status;
}

static uint8_t USBD_Composite_EP0_RxReady(void* pdev) {
  uint8_t status = USBD_OK;
  for(USBD_Composite_Class_Data* c = s_Classes; c != NULL; c = c->next) {
    if(c->active && c->cb->EP0_RxReady) {
      status += c->cb->EP0_RxReady(pdev, c);
    }
  }

  return status;
}

static uint8_t USBD_Composite_DataIn(void* pdev, uint8_t epnum) {
  uint32_t msk = 1 << (epnum & 0x7f);
  for(USBD_Composite_Class_Data* c = s_Classes; c != NULL; c = c->next) {
    if(c->active && c->cb->DataIn) {
      if (c->epMask & msk) {
        // Class handled this endpoint?
        if (c->cb->DataIn(pdev, c, epnum) == USBD_OK)
          return USBD_OK;
        //LOG_DEBUG(ERROR, "FAIL %x %d", (void*)c, epnum);
      }
    }
  }
  // No class handled this endpoint number, return USBD_FAIL
  return USBD_FAIL;
}

static uint8_t USBD_Composite_DataOut(void* pdev , uint8_t epnum) {
  uint32_t msk = 1 << (epnum & 0x7f);
  msk <<= 16;
  for(USBD_Composite_Class_Data* c = s_Classes; c != NULL; c = c->next) {
    if(c->active && c->cb->DataOut) {
      if (c->epMask & msk) {
        // Class handled this endpoint?
        if (c->cb->DataOut(pdev, c, epnum) == USBD_OK)
          return USBD_OK;
        //LOG_DEBUG(ERROR, "FAIL %x %d", (void*)c, epnum);
      }
    }
  }
  // No class handled this endpoint number, return USBD_FAIL
  return USBD_FAIL;
}

static uint8_t USBD_Composite_SOF(void *pdev) {
  for(USBD_Composite_Class_Data* c = s_Classes; s_Initialized && c != NULL; c = c->next) {
    if(c->active && c->cb->SOF) {
      c->cb->SOF(pdev, c);
    }
  }

  HAL_USB_Vendor_Interface_SOF(pdev);

  return USBD_OK;
}

// static uint8_t USBD_Composite_IsoINIncomplete(void *pdev) {

// }

// static uint8_t USBD_Composite_IsoOUTIncomplete(void *pdev) {

// }

static uint8_t* USBD_Composite_GetConfigDescriptor(uint8_t speed, uint16_t *length) {
  *length = USBD_Build_CfgDesc(USBD_Composite_CfgDesc, speed, 0);
  return USBD_Composite_CfgDesc;
}

#ifdef USE_USB_OTG_HS
static uint8_t* USBD_Composite_GetOtherConfigDescriptor(uint8_t speed, uint16_t *length) {
  *length = USBD_Build_CfgDesc(USBD_Composite_CfgDesc, speed, 1);
  return USBD_Composite_CfgDesc;
}
#endif

static uint8_t* USBD_Composite_GetMsftStrDescriptor(uint16_t* length) {
  *length = sizeof(USBD_Composite_MsftStrDesc);
  return (uint8_t*)USBD_Composite_MsftStrDesc;
}

static uint8_t* USBD_Composite_GetUsrStrDescriptor(uint8_t speed, uint8_t index, uint16_t *length) {
  // MSFT-specific
  if (index == USBD_IDX_MSFT_STR) {
    return USBD_Composite_GetMsftStrDescriptor(length);
  } else if (index == USBD_COMPOSITE_USRSTR_BASE) {
    USBD_GetString(USBD_PRODUCT_STRING " " "Control Interface", USBD_StrDesc, length);
    return USBD_StrDesc;
  }

  for(USBD_Composite_Class_Data* c = s_Classes; s_Initialized && c != NULL; c = c->next) {
    if(c->active && c->cb->GetUsrStrDescriptor) {
      uint8_t* ret = c->cb->GetUsrStrDescriptor(speed, c, index, length);
      if (ret) {
        return ret;
      }
    }
  }

  *length = 0;
  return NULL;
}

static uint16_t USBD_Build_CfgDesc(uint8_t* buf, uint8_t speed, uint8_t other) {
  uint8_t totalInterfaces = 0;
  uint8_t activeInterfaces = 0;
  uint16_t totalLength = USBD_COMPOSITE_CFGDESC_HEADER_LENGTH;
  uint16_t clsCfgLength;

  uint32_t epMask = 0;

  uint8_t *pbuf = buf;
  uint8_t *vbuf = NULL;
  // usbd_req.c was patched to provide LOBYTE(wValue) here, which is a bConfigurationValue
  uint8_t configValue = speed + 1;

  memcpy(pbuf, USBD_Composite_CfgDescHeaderTemplate, USBD_COMPOSITE_CFGDESC_HEADER_LENGTH);

  if (configValue == USBD_CONFIGURATION_100MA) {
    // Set bMaxPower to 100mA
    *(pbuf + USBD_COMPOSITE_CFGDESC_HEADER_LENGTH - 1) = 0x32;
    *(pbuf + 5) = USBD_CONFIGURATION_100MA;
  }

  pbuf += USBD_COMPOSITE_CFGDESC_HEADER_LENGTH;

  // Append all class Interface and Class Specific descriptors
  for(USBD_Composite_Class_Data* c = s_Classes; c != NULL; c = c->next) {
    c->firstInterface = totalInterfaces;
    clsCfgLength = USBD_COMPOSITE_CFGDESC_MAX_LENGTH - (pbuf - buf);
    
    if (!other && c->cb->GetConfigDescriptor) {
      c->cb->GetConfigDescriptor(speed, c, pbuf, &clsCfgLength);
    }
#ifdef USE_USB_OTG_HS
    else if (other && c->cb->GetOtherConfigDescriptor) {
      c->cb->GetOtherConfigDescriptor(speed, c, pbuf, &clsCfgLength);
    }
#endif

    if (clsCfgLength) {
      totalInterfaces += c->interfaces;
      if (c->active && (epMask & c->epMask) == 0) {
        epMask |= c->epMask;
        activeInterfaces += c->interfaces;
        c->cfg = pbuf;
        pbuf += clsCfgLength;
        totalLength += clsCfgLength;
      }
    }

    // Vendor-specific interface #2
    if (totalInterfaces == 2) {
      clsCfgLength = USBD_COMPOSITE_CFGDESC_MAX_LENGTH - (pbuf - buf);
      if (clsCfgLength >= sizeof(USBD_Composite_VendorInterface)) {
        vbuf = pbuf;
        pbuf += sizeof(USBD_Composite_VendorInterface);
        activeInterfaces++;
        totalInterfaces++;
      }
    }
  }

  if (vbuf == NULL) {
    vbuf = pbuf;
    activeInterfaces++;
    totalInterfaces++;
  }

  memcpy(vbuf, USBD_Composite_VendorInterface, sizeof(USBD_Composite_VendorInterface));
  totalLength += sizeof(USBD_Composite_VendorInterface);

  // Update wTotalLength and bNumInterfaces
  *(buf + USBD_COMPOSITE_CFGDESC_HEADER_OFFSET_NUM_INTERFACES) = activeInterfaces;
  *((uint16_t *)(buf + USBD_COMPOSITE_CFGDESC_HEADER_OFFSET_TOTAL_LENGTH)) = totalLength;

  // LOG_DEBUG(TRACE, "Built configuration (0x%x): %d bytes, %d total interfaces, %d active",
  //           configValue, totalLength, totalInterfaces, activeInterfaces);

  return totalLength;
}


void* USBD_Composite_Register(USBD_Multi_Instance_cb_Typedef* cb, void* priv, uint8_t front) {
  if (s_Classes_Count >= USBD_COMPOSITE_MAX_CLASSES || cb == NULL)
    return NULL;

  int32_t irq = HAL_disable_irq();
  USBD_Composite_Class_Data* cls = NULL;
  for (int i = 0; i < USBD_COMPOSITE_MAX_CLASSES; i++) {
    if (s_Class_Entries[i].inuse == 0) {
      cls = s_Class_Entries + i;
      break;
    }
  }

  if (cls) {
    cls->inuse = 1;
    cls->active = 1;
    cls->cb = cb;
    cls->priv = priv;
    cls->epMask = 0xffffffff;
    cls->interfaces = 0;
    cls->firstInterface = 0;
    cls->next = NULL;
    cls->cfg = NULL;

    if (s_Classes == NULL) {
      s_Classes = cls;
    } else {
      USBD_Composite_Class_Data* c = s_Classes;
      if (!front) {
        while(c->next) {
          c = c->next;
        }
        c->next = cls;
      } else {
        s_Classes = cls;
        cls->next = c;
      }
    }
    s_Classes_Count++;
  }
  HAL_enable_irq(irq);
  return (void*)cls;
}

void USBD_Composite_Unregister(void* cls, void* priv) {
  int32_t irq = HAL_disable_irq();
  USBD_Composite_Class_Data* prev = NULL;
  for(USBD_Composite_Class_Data* c = s_Classes; c != NULL; prev = c, c = c->next) {
    if ((cls && c == cls) || (priv && c->priv == priv)) {
      c->inuse = 0;
      if (prev) {
        prev->next = c->next;
      } else {
        s_Classes = c->next;
      }
      s_Classes_Count--;
      break;
    }
  }
  HAL_enable_irq(irq);
}

void USBD_Composite_Unregister_All() {
  int32_t irq = HAL_disable_irq();
  s_Classes = NULL;
  s_Classes_Count = 0;
  memset(s_Class_Entries, 0, sizeof(s_Class_Entries));
  HAL_enable_irq(irq);
}

void USBD_Composite_Set_State(void* cls, bool state) {
  int32_t irq = HAL_disable_irq();
  for(USBD_Composite_Class_Data* c = s_Classes; c != NULL; c = c->next) {
    if (cls && c == cls) {
      c->active = state;
      break;
    }
  }
  HAL_enable_irq(irq);
}

bool USBD_Composite_Get_State(void* cls) {
  bool state = false;
  int32_t irq = HAL_disable_irq();
  for(USBD_Composite_Class_Data* c = s_Classes; c != NULL; c = c->next) {
    if (cls && c == cls) {
      state = c->active;
      break;
    }
  }
  HAL_enable_irq(irq);

  return state;
}

uint8_t USBD_Composite_Registered_Count(bool onlyActive) {
  uint8_t registered = (uint8_t)s_Classes_Count;

  if (onlyActive) {
    registered = 0;
    int32_t irq = HAL_disable_irq();
    for(USBD_Composite_Class_Data* c = s_Classes; c != NULL; c = c->next) {
      if (c->active)
        registered++;
    }
    HAL_enable_irq(irq);
  }

  return registered;
}

uint8_t USBD_Composite_Handle_Msft_Request(void* pdev, USB_SETUP_REQ* req) {
  if (req->wIndex == 0x0004) {
    USBD_CtlSendData(pdev, USBD_Composite_MsftExtCompatIdOsDescr, req->wLength);
  } else if (req->wIndex == 0x0005) {
    if ((req->wValue & 0xff) == 0x02) {
      USBD_CtlSendData(pdev, USBD_Composite_MsftExtPropOsDescr, req->wLength);
    } else {
      // Send dummy
      uint8_t dummy[10] = {0};
      USBD_CtlSendData(pdev, dummy, req->wLength);
    }
  } else {
    return USBD_FAIL;
  }

  return USBD_OK;
}
