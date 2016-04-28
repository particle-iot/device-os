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

static USBD_Composite_Class_Data s_Class_Entries[USBD_COMPOSITE_MAX_CLASSES] = { {0} };
static uint32_t s_Classes_Count = 0;
static USBD_Composite_Class_Data* s_Classes = NULL;
static uint8_t s_Initialized = 0;

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

USBD_Class_cb_TypeDef USBD_Composite_cb = {
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
  0xc0,                                          /* bmAttirbutes (Bus powered) */
  0x32                                           /* bMaxPower (100mA) */
};

static uint8_t USBD_Composite_Init(void* pdev, uint8_t cfgidx) {
  uint8_t status = USBD_OK;
  for(USBD_Composite_Class_Data* c = s_Classes; c != NULL; c = c->next) {
    if(c->enabled && c->cb->Init) {
      status += c->cb->Init(pdev, c, cfgidx);
    }
  }

  s_Initialized = 1;

  return status;
}

static uint8_t USBD_Composite_DeInit(void* pdev, uint8_t cfgidx) {
  uint8_t status = USBD_OK;

  s_Initialized = 0;

  for(USBD_Composite_Class_Data* c = s_Classes; c != NULL; c = c->next) {
    if(c->enabled && c->cb->DeInit) {
      status += c->cb->DeInit(pdev, c, cfgidx);
    }
  }

  return status;
}

static uint8_t USBD_Composite_Setup(void* pdev, USB_SETUP_REQ* req) {
  for(USBD_Composite_Class_Data* c = s_Classes; c != NULL; c = c->next) {
    if (c->enabled && req->wIndex >= c->firstInterface && req->wIndex < (c->firstInterface + c->interfaces)) {
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
    if(c->enabled && c->cb->EP0_TxSent) {
      status += c->cb->EP0_TxSent(pdev, c);
    }
  }

  return status;
}

static uint8_t USBD_Composite_EP0_RxReady(void* pdev) {
  uint8_t status = USBD_OK;
  for(USBD_Composite_Class_Data* c = s_Classes; c != NULL; c = c->next) {
    if(c->enabled && c->cb->EP0_RxReady) {
      status += c->cb->EP0_RxReady(pdev, c);
    }
  }

  return status;
}

static uint8_t USBD_Composite_DataIn(void* pdev, uint8_t epnum) {
  uint32_t msk = 1 << (epnum & 0x7f);
  for(USBD_Composite_Class_Data* c = s_Classes; c != NULL; c = c->next) {
    if(c->enabled && c->cb->DataIn) {
      if (c->epMask & msk) {
        // Class handled this endpoint?
        if (c->cb->DataIn(pdev, c, epnum) == USBD_OK)
          return USBD_OK;
        // DEBUG("FAIL %x %d", (void*)c, epnum);
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
    if(c->enabled && c->cb->DataOut) {
      if (c->epMask & msk) {
        // Class handled this endpoint?
        if (c->cb->DataOut(pdev, c, epnum) == USBD_OK)
          return USBD_OK;
        // DEBUG("FAIL %x %d", (void*)c, epnum);
      }
    }
  }
  // No class handled this endpoint number, return USBD_FAIL
  return USBD_FAIL;
}

static uint8_t USBD_Composite_SOF(void *pdev) {
  for(USBD_Composite_Class_Data* c = s_Classes; s_Initialized && c != NULL; c = c->next) {
    if(c->enabled && c->cb->SOF) {
      c->cb->SOF(pdev, c);
    }
  }

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

static uint8_t* USBD_Composite_GetUsrStrDescriptor(uint8_t speed, uint8_t index, uint16_t *length) {
  for(USBD_Composite_Class_Data* c = s_Classes; s_Initialized && c != NULL; c = c->next) {
    if(c->enabled && c->cb->GetUsrStrDescriptor) {
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
  memcpy(pbuf, USBD_Composite_CfgDescHeaderTemplate, USBD_COMPOSITE_CFGDESC_HEADER_LENGTH);
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
      if (c->enabled && (epMask & c->epMask) == 0) {
        epMask |= c->epMask;
        activeInterfaces += c->interfaces;
        c->cfg = pbuf;
        pbuf += clsCfgLength;
        totalLength += clsCfgLength;
      }
    }
  }

  // Update wTotalLength and bNumInterfaces
  *(buf + USBD_COMPOSITE_CFGDESC_HEADER_OFFSET_NUM_INTERFACES) = activeInterfaces;
  *((uint16_t *)(buf + USBD_COMPOSITE_CFGDESC_HEADER_OFFSET_TOTAL_LENGTH)) = totalLength;

  // DEBUG("Built USB descriptors: %d bytes, %d total interfaces, %d active", totalLength, totalInterfaces, activeInterfaces);

  return totalLength;
}


void* USBD_Composite_Register(USBD_Multi_Instance_cb_Typedef* cb, void* priv, uint8_t front) {
  if (s_Classes_Count >= USBD_COMPOSITE_MAX_CLASSES || cb == NULL)
    return NULL;

  int32_t irq = HAL_disable_irq();
  USBD_Composite_Class_Data* cls = NULL;
  for (int i = 0; i < USBD_COMPOSITE_MAX_CLASSES; i++) {
    if (s_Class_Entries[i].active == 0) {
      cls = s_Class_Entries + i;
      break;
    }
  }

  if (cls) {
    cls->active = 1;
    cls->enabled = 1;
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
      c->active = 0;
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
      c->enabled = state;
      break;
    }
  }
  HAL_enable_irq(irq);
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
