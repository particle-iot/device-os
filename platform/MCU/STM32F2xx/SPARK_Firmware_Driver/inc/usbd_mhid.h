#ifndef USBD_MHID_H_
#define USBD_MHID_H_

#include "usbd_ioreq.h"
#include "usbd_composite.h"

#define USBD_MHID_CONFIG_DESC_SIZE      32
#define USBD_MHID_DESC_SIZE              9
#define USBD_MHID_REPORT_DESC_SIZE     178

#define USBD_MHID_DIGITIZER_REPORT_DESC_SIZE 65

#define HID_DESCRIPTOR_TYPE           0x21
#define HID_REPORT_DESC               0x22

#define HID_REQ_SET_PROTOCOL          0x0B
#define HID_REQ_GET_PROTOCOL          0x03

#define HID_REQ_SET_IDLE              0x0A
#define HID_REQ_GET_IDLE              0x02

#define HID_REQ_SET_REPORT            0x09
#define HID_REQ_GET_REPORT            0x01

extern USBD_Multi_Instance_cb_Typedef  USBD_MHID_cb;

typedef struct USBD_MHID_Instance_Data {
  __ALIGN_BEGIN volatile uint32_t alt_set __ALIGN_END;
  __ALIGN_BEGIN volatile uint32_t protocol __ALIGN_END;
  __ALIGN_BEGIN volatile uint32_t idle_state __ALIGN_END;
  uint8_t ep_in;
  uint8_t ep_out;

  uint8_t* report_descriptor;
  uint16_t report_descriptor_size;

  uint8_t configured;

  #ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  // Temporary aligned buffer
  __ALIGN_BEGIN uint8_t descriptor[USBD_MHID_DESC_SIZE] __ALIGN_END;
  #endif

  volatile uint8_t intransfer;
} USBD_MHID_Instance_Data;

uint8_t USBD_MHID_SendReport (USB_OTG_CORE_HANDLE* pdev, USBD_MHID_Instance_Data* priv, uint8_t* report, uint16_t len);
int32_t USBD_MHID_Transfer_Status(void* pdev, USBD_Composite_Class_Data* cls);
uint8_t USBD_MHID_SetDigitizerState(void* pdev, USBD_Composite_Class_Data* cls, uint8_t idx, uint8_t state);

#endif /* USBD_MHID_H_ */
