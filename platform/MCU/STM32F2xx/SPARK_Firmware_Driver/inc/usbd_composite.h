#ifndef USBD_COMPOSITE_H_
#define USBD_COMPOSITE_H_

#include <stdint.h>
#include <stdbool.h>
#include "usbd_conf.h"
#include "usb_core.h"
#include "usbd_ioreq.h"

#define USBD_COMPOSITE_CFGDESC_MAX_LENGTH     256
#define USBD_COMPOSITE_MAX_CLASSES            4
#define USBD_COMPOSITE_CFGDESC_HEADER_LENGTH  9
#define USBD_COMPOSITE_CFGDESC_HEADER_OFFSET_TOTAL_LENGTH  2
#define USBD_COMPOSITE_CFGDESC_HEADER_OFFSET_NUM_INTERFACES 4

typedef struct USBD_Composite_Class_Data USBD_Composite_Class_Data;
typedef struct USBD_Multi_Instance_cb_Typedef USBD_Multi_Instance_cb_Typedef;

struct USBD_Multi_Instance_cb_Typedef
{
  uint8_t  (*Init)                      (void* pdev, USBD_Composite_Class_Data* cls, uint8_t cfgidx);
  uint8_t  (*DeInit)                    (void* pdev, USBD_Composite_Class_Data* cls, uint8_t cfgidx);
  /* Control Endpoints*/
  uint8_t  (*Setup)                     (void* pdev, USBD_Composite_Class_Data* cls, USB_SETUP_REQ* req);
  uint8_t  (*EP0_TxSent)                (void* pdev, USBD_Composite_Class_Data* cls);
  uint8_t  (*EP0_RxReady)               (void* pdev, USBD_Composite_Class_Data* cls);
  /* Class Specific Endpoints*/
  uint8_t  (*DataIn)                    (void* pdev, USBD_Composite_Class_Data* cls, uint8_t epnum);
  uint8_t  (*DataOut)                   (void* pdev, USBD_Composite_Class_Data* cls, uint8_t epnum);
  uint8_t  (*SOF)                       (void* pdev, USBD_Composite_Class_Data* cls);
  uint8_t  (*IsoINIncomplete)           (void* pdev, USBD_Composite_Class_Data* cls);
  uint8_t  (*IsoOUTIncomplete)          (void* pdev, USBD_Composite_Class_Data* cls);

  uint8_t* (*GetConfigDescriptor)       (uint8_t speed, USBD_Composite_Class_Data* cls, uint8_t* buf, uint16_t* length);
#ifdef USB_OTG_HS_CORE
  uint8_t* (*GetOtherConfigDescriptor)  (uint8_t speed, USBD_Composite_Class_Data* cls, uint8_t* buf, uint16_t* length);
#endif

#ifdef USB_SUPPORT_USER_STRING_DESC
  uint8_t* (*GetUsrStrDescriptor)       (uint8_t speed, USBD_Composite_Class_Data* cls, uint8_t index, uint16_t* length);
#endif
};

struct USBD_Composite_Class_Data {
  // Linked-list
  struct USBD_Composite_Class_Data* next;
  uint8_t inuse;
  uint8_t active;

  USBD_Multi_Instance_cb_Typedef* cb;
  uint8_t* cfg;
  uint16_t cfgLen;
  uint8_t interfaces;
  uint8_t firstInterface;
  uint32_t epMask;

  // Class-specific instance data
  void* priv;
};

typedef void (*USBD_Composite_Configuration_Callback)(uint8_t cfgidx);

#define USBD_CONFIGURATION_NONE  0
#define USBD_CONFIGURATION_500MA 1
#define USBD_CONFIGURATION_100MA 2

USBD_Class_cb_TypeDef* USBD_Composite_Instance(USBD_Composite_Configuration_Callback cb);
void* USBD_Composite_Register(USBD_Multi_Instance_cb_Typedef* cb, void* priv, uint8_t front);
void USBD_Composite_Unregister(void* cls, void* priv);
void USBD_Composite_Set_State(void* cls, bool state);
bool USBD_Composite_Get_State(void* cls);
void USBD_Composite_Unregister_All();
uint8_t USBD_Composite_Registered_Count(bool onlyActive);
uint8_t USBD_Composite_Handle_Msft_Request(void* pdev, USB_SETUP_REQ* req);

#endif /* USBD_COMPOSITE_H_ */
