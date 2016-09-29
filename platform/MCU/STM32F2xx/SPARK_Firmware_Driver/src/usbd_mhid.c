#include <string.h>
#include "usbd_mhid.h"
#include "usbd_desc.h"
#include "usbd_req.h"
#include "usbd_desc_device.h"
#include "debug.h"

#define USBD_MHID_USRSTR_BASE 15
#define USBD_MHID_INTERFACE_NAME USBD_PRODUCT_STRING " " "HID Mouse / Keyboard"

static uint8_t USBD_MHID_Init(void* pdev, USBD_Composite_Class_Data* cls, uint8_t cfgidx);
static uint8_t USBD_MHID_DeInit(void* pdev, USBD_Composite_Class_Data* cls, uint8_t cfgidx);
static uint8_t USBD_MHID_Setup(void* pdev, USBD_Composite_Class_Data* cls, USB_SETUP_REQ* req);
static uint8_t USBD_MHID_DataIn(void* pdev, USBD_Composite_Class_Data* cls, uint8_t epnum);
static uint8_t* USBD_MHID_GetConfigDescriptor(uint8_t speed, USBD_Composite_Class_Data* cls, uint8_t* buf, uint16_t* length);
static uint8_t* USBD_MHID_GetUsrStrDescriptor(uint8_t speed, USBD_Composite_Class_Data* cls, uint8_t index, uint16_t* length);

__ALIGN_BEGIN static const uint8_t USBD_MHID_DefaultReportDesc[USBD_MHID_REPORT_DESC_SIZE] __ALIGN_END;

// Declare instance data struct here for now
static USBD_MHID_Instance_Data USBD_MHID_Instance = {
  .alt_set = 0,
  .protocol = 0,
  .idle_state = 0,
  .ep_in = HID_IN_EP,
  .ep_out = HID_OUT_EP,
  .report_descriptor = (uint8_t*)USBD_MHID_DefaultReportDesc,
  .report_descriptor_size = USBD_MHID_REPORT_DESC_SIZE
};

USBD_Multi_Instance_cb_Typedef USBD_MHID_cb =
{
  USBD_MHID_Init,
  USBD_MHID_DeInit,
  USBD_MHID_Setup,
  NULL, /*EP0_TxSent*/
  NULL, /*EP0_RxReady*/
  USBD_MHID_DataIn, /*DataIn*/
  NULL, /*DataOut*/
  NULL, /*SOF */
  NULL,
  NULL,
  USBD_MHID_GetConfigDescriptor,
#ifdef USB_OTG_HS_CORE
  USBD_MHID_GetConfigDescriptor, /* use same config as per FS */
#endif
  USBD_MHID_GetUsrStrDescriptor
};

/* USB HID device Configuration Descriptor */
static const uint8_t USBD_MHID_CfgDesc[USBD_MHID_CONFIG_DESC_SIZE] =
{
  /************** Descriptor of Joystick Mouse interface ****************/
  /* 00 */
  0x09,         /*bLength: Interface Descriptor size*/
  USB_INTERFACE_DESCRIPTOR_TYPE,/*bDescriptorType: Interface descriptor type*/
  0x00,         /*bInterfaceNumber: Number of Interface*/
  0x00,         /*bAlternateSetting: Alternate setting*/
  0x02,         /*bNumEndpoints*/
  0x03,         /*bInterfaceClass: HID*/
  0x00,         /*bInterfaceSubClass : 1=BOOT, 0=no boot*/
  0x00,         /*nInterfaceProtocol : 0=none, 1=keyboard, 2=mouse*/
  USBD_MHID_USRSTR_BASE, /*iInterface: Index of string descriptor*/
  /******************** Descriptor of Joystick Mouse HID ********************/
  /* 09 */
  0x09,         /*bLength: HID Descriptor size*/
  HID_DESCRIPTOR_TYPE, /*bDescriptorType: HID*/
  0x11,         /*bcdHID: HID Class Spec release number*/
  0x01,
  0x00,         /*bCountryCode: Hardware target country*/
  0x01,         /*bNumDescriptors: Number of HID class descriptors to follow*/
  0x22,         /*bDescriptorType*/
  USBD_MHID_REPORT_DESC_SIZE,/*wItemLength: Total length of Report descriptor*/
  0x00,
  /******************** Descriptor of Mouse endpoint ********************/
  /* 18 */
  0x07,          /*bLength: Endpoint Descriptor size*/
  USB_ENDPOINT_DESCRIPTOR_TYPE, /*bDescriptorType:*/

  HID_IN_EP,     /*bEndpointAddress: Endpoint Address (IN)*/
  0x03,          /*bmAttributes: Interrupt endpoint*/
  HID_IN_PACKET, /*wMaxPacketSize: 5 Byte max */
  0x00,
  0x01,          /*bInterval: Polling Interval (10 ms)*/


  0x07,          /*bLength: Endpoint Descriptor size*/
  USB_ENDPOINT_DESCRIPTOR_TYPE, /*bDescriptorType:*/

  HID_OUT_EP,     /*bEndpointAddress: Endpoint Address (IN)*/
  0x03,           /*bmAttributes: Interrupt endpoint*/
  HID_OUT_PACKET, /*wMaxPacketSize: 5 Byte max */
  0x00,
  0x01,          /*bInterval: Polling Interval (10 ms)*/
  /* 27 */
};

__ALIGN_BEGIN static const uint8_t USBD_MHID_DefaultReportDesc[USBD_MHID_REPORT_DESC_SIZE] __ALIGN_END =
{
  0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
  0x09, 0x02,                    // USAGE (Mouse)
  0xa1, 0x01,                    // COLLECTION (Application)
  0x85, 0x01,                    //   REPORT_ID (1)
  0x09, 0x01,                    //   USAGE (Pointer)
  0xa1, 0x00,                    //   COLLECTION (Physical)
  0x05, 0x09,                    //     USAGE_PAGE (Button)
  0x19, 0x01,                    //     USAGE_MINIMUM (Button 1)
  0x29, 0x03,                    //     USAGE_MAXIMUM (Button 3)
  0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
  0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
  0x95, 0x03,                    //     REPORT_COUNT (3)
  0x75, 0x01,                    //     REPORT_SIZE (1)
  0x81, 0x02,                    //     INPUT (Data,Var,Abs)
  0x95, 0x01,                    //     REPORT_COUNT (1)
  0x75, 0x05,                    //     REPORT_SIZE (5)
  0x81, 0x03,                    //     INPUT (Cnst,Var,Abs)
  0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
  0x09, 0x30,                    //     USAGE (X)
  0x09, 0x31,                    //     USAGE (Y)
  // 0x36, 0x01, 0x80,              //     PHYSICAL_MINIMUM (-32767)
  // 0x46, 0xff, 0x7f,              //     PHYSICAL_MAXIMUM (32767)
  0x16, 0x01, 0x80,              //     LOGICAL_MINIMUM (-32767)
  0x26, 0xff, 0x7f,              //     LOGICAL_MAXIMUM (32767)
  // 0x15, 0x81,                    //     LOGICAL_MINIMUM (-127)
  // 0x25, 0x7f,                    //     LOGICAL_MAXIMUM (127)
  0x75, 0x10,                    //     REPORT_SIZE (16)
  0x95, 0x02,                    //     REPORT_COUNT (2)
  0x81, 0x06,                    //     INPUT (Data,Var,Rel)
  0x09, 0x38,                    //     USAGE (WHEEL)
  0x15, 0x81,                    //     LOGICAL_MINIMUM (-127)
  0x25, 0x7f,                    //     LOGICAL_MAXIMUM (127)
  0x75, 0x08,                    //     REPORT_SIZE (8)
  0x95, 0x01,                    //     REPORT_COUNT (1)
  0x81, 0x06,                    //     INPUT (Data,Var,Rel)
  0xc0,                          //   END_COLLECTION
  0xc0,                          // END_COLLECTION
  0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
  0x09, 0x06,                    // USAGE (Keyboard)
  0xa1, 0x01,                    // COLLECTION (Application)
  0x85, 0x02,                    //   REPORT_ID (2)
  0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
  0x19, 0xe0,                    //   USAGE_MINIMUM (Keyboard LeftControl)
  0x29, 0xe7,                    //   USAGE_MAXIMUM (Keyboard Right GUI)
  0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
  0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
  0x75, 0x01,                    //   REPORT_SIZE (1)
  0x95, 0x08,                    //   REPORT_COUNT (8)
  0x81, 0x02,                    //   INPUT (Data,Var,Abs)
  0x95, 0x01,                    //   REPORT_COUNT (1)
  0x75, 0x08,                    //   REPORT_SIZE (8)
  0x81, 0x01,                    //   INPUT (Cnst,Ary,Abs)
  // We don't use OUT endpoint to receive LED reports from host
  // 0x95, 0x05,                    //   REPORT_COUNT (5)
  // 0x75, 0x01,                    //   REPORT_SIZE (1)
  // 0x05, 0x08,                    //   USAGE_PAGE (LEDs)
  // 0x19, 0x01,                    //   USAGE_MINIMUM (Num Lock)
  // 0x29, 0x05,                    //   USAGE_MAXIMUM (Kana)
  // 0x91, 0x02,                    //   OUTPUT (Data,Var,Abs)
  // 0x95, 0x01,                    //   REPORT_COUNT (1)
  // 0x75, 0x03,                    //   REPORT_SIZE (3)
  // 0x91, 0x01,                    //   OUTPUT (Cnst,Ary,Abs)
  0x95, 0x06,                    //   REPORT_COUNT (6)
  0x75, 0x08,                    //   REPORT_SIZE (8)
  0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
  0x25, 0xdd,                    //   LOGICAL_MAXIMUM (221)
  0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
  0x19, 0x00,                    //   USAGE_MINIMUM (Reserved (no event indicated))
  0x29, 0xdd,                    //   USAGE_MAXIMUM (Keypad Hexadecimal)
  0x81, 0x00,                    //   INPUT (Data,Ary,Abs)
  0xc0,                          // END_COLLECTION
  0x05, 0x0d,                    // USAGE_PAGE (Digitizer)
  0x09, 0x02,                    // USAGE (Pen)
  0xa1, 0x01,                    // COLLECTION (Application)
  0x85, 0x03,                    //   REPORT_ID (3)
  0x09, 0x20,                    //   USAGE (Stylus)
  0xa1, 0x00,                    //   COLLECTION (Physical)
  0x09, 0x42,                    //     USAGE (Tip Switch)
  0x09, 0x32,                    //     USAGE (In Rnage)
  0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
  0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
  0x75, 0x01,                    //     REPORT_SIZE (1)
  0x95, 0x02,                    //     REPORT_COUNT (2)
  0x81, 0x02,                    //     INPUT (Data,Var,Abs)
  0x75, 0x01,                    //     REPORT_SIZE (1)
  0x95, 0x06,                    //     REPORT_COUNT (6)
  0x81, 0x01,                    //     INPUT (Cnst,Ary,Abs)
  0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
  0x09, 0x01,                    //     USAGE (Pointer)
  0xa1, 0x00,                    //     COLLECTION (Physical)
  0x09, 0x30,                    //       USAGE (X)
  0x09, 0x31,                    //       USAGE (Y)
  0x16, 0x00, 0x00,              //       LOGICAL_MINIMUM (0)
  0x26, 0xff, 0x7f,              //       LOGICAL_MAXIMUM (32767)
  0x36, 0x00, 0x00,              //       PHYSICAL_MINIMUM (0)
  0x46, 0xff, 0x7f,              //       PHYSICAL_MAXIMUM (32767)
  0x65, 0x00,                    //       UNIT (None)
  0x75, 0x10,                    //       REPORT_SIZE (16)
  0x95, 0x02,                    //       REPORT_COUNT (2)
  0x81, 0x02,                    //       INPUT (Data,Var,Abs)
  0xc0,                          //     END_COLLECTION
  0xc0,                          //   END_COLLECTION
  0xc0                           // END_COLLECTION
};

static uint8_t USBD_MHID_Init(void* pdev, USBD_Composite_Class_Data* cls, uint8_t cfgidx)
{
  USBD_MHID_Instance_Data* priv = &USBD_MHID_Instance; /* (USBD_MHID_Instance_Data*)cls->priv; */
#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  memcpy(priv->descriptor, cls->cfg + 0x09, USBD_MHID_DESC_SIZE);
#endif

  /* Open EP IN */
  DCD_EP_Open(pdev,
              priv->ep_in,
              HID_IN_PACKET,
              USB_OTG_EP_INT);

  // /* Open EP OUT */
  // DCD_EP_Open(pdev,
  //             priv->ep_out,
  //             HID_OUT_PACKET,
  //             USB_OTG_EP_INT);

  priv->intransfer = 0;
  priv->configured = 1;

  return USBD_OK;
}

static uint8_t USBD_MHID_DeInit(void* pdev, USBD_Composite_Class_Data* cls, uint8_t cfgidx)
{
  USBD_MHID_Instance_Data* priv = &USBD_MHID_Instance; /* (USBD_MHID_Instance_Data*)cls->priv; */
  /* Close HID EPs */
  DCD_EP_Close (pdev, priv->ep_in);
  // DCD_EP_Close (pdev, priv->ep_out);

  priv->configured = 0;

  return USBD_OK;
}

static uint8_t USBD_MHID_Setup(void* pdev, USBD_Composite_Class_Data* cls, USB_SETUP_REQ* req)
{
  USBD_MHID_Instance_Data* priv = &USBD_MHID_Instance; /* (USBD_MHID_Instance_Data*)cls->priv; */
  uint16_t len = 0;
  uint8_t* pbuf = NULL;

  switch (req->bmRequest & USB_REQ_TYPE_MASK)
  {
  case USB_REQ_TYPE_CLASS :
    switch (req->bRequest)
    {
    case HID_REQ_SET_PROTOCOL:
      priv->protocol = (uint8_t)(req->wValue);
      break;

    case HID_REQ_GET_PROTOCOL:
      USBD_CtlSendData (pdev,
                        (uint8_t *)&priv->protocol,
                        1);
      break;

    case HID_REQ_SET_IDLE:
      priv->idle_state = (uint8_t)(req->wValue >> 8);
      break;

    case HID_REQ_GET_IDLE:
      USBD_CtlSendData (pdev,
                        (uint8_t *)&priv->idle_state,
                        1);
      break;

    default:
      USBD_CtlError (pdev, req);
      return USBD_FAIL;
    }
    break;

  case USB_REQ_TYPE_STANDARD:
    switch (req->bRequest)
    {
    case USB_REQ_GET_DESCRIPTOR:
      if( req->wValue >> 8 == HID_REPORT_DESC)
      {
        len = MIN(priv->report_descriptor_size, req->wLength);
        pbuf = priv->report_descriptor;
      }
      else if( req->wValue >> 8 == HID_DESCRIPTOR_TYPE)
      {

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
        pbuf = priv->descriptor;
#else
        pbuf = cls->cfg + 0x09;
#endif
        len = MIN(USBD_MHID_DESC_SIZE, req->wLength);
      }

      USBD_CtlSendData (pdev,
                        pbuf,
                        len);

      break;

    case USB_REQ_GET_INTERFACE :
      USBD_CtlSendData (pdev,
                        (uint8_t *)&priv->alt_set,
                        1);
      break;

    case USB_REQ_SET_INTERFACE :
      priv->alt_set = (uint8_t)(req->wValue);
      break;
    }
  }
  return USBD_OK;
}

uint8_t USBD_MHID_SendReport (USB_OTG_CORE_HANDLE* pdev, USBD_MHID_Instance_Data* priv, uint8_t* report, uint16_t len)
{
  priv = &USBD_MHID_Instance;
  
  if (pdev->dev.device_status == USB_OTG_CONFIGURED && priv->configured)
  {
    priv->intransfer = 1;
    DCD_EP_Tx (pdev, priv->ep_in, report, len);
  }
  return USBD_OK;
}

static uint8_t* USBD_MHID_GetConfigDescriptor(uint8_t speed, USBD_Composite_Class_Data* cls, uint8_t* buf, uint16_t* length)
{
  USBD_MHID_Instance_Data* priv = &USBD_MHID_Instance;
  *length = USBD_MHID_CONFIG_DESC_SIZE;
  // Only one interface, HID doesn't need to use IAD
  cls->interfaces = 1;

  memcpy(buf, USBD_MHID_CfgDesc, *length);
  // Update bInterfaceNumber
  *(buf + 2) = cls->firstInterface;
  // Update endpoint number
  *(buf + 20) = priv->ep_in;

  cls->epMask = (1 << (priv->ep_in & 0x7f));

  return buf;
}

static uint8_t USBD_MHID_DataIn(void* pdev, USBD_Composite_Class_Data* cls, uint8_t epnum)
{
  USBD_MHID_Instance_Data* priv = &USBD_MHID_Instance; /* (USBD_MHID_Instance_Data*)cls->priv; */

  /* Ensure that the FIFO is empty before a new transfer, this condition could
  be caused by  a new transfer before the end of the previous transfer */
  if ((epnum | 0x80) != priv->ep_in)
    return USBD_FAIL;

  DCD_EP_Flush(pdev, priv->ep_in);
  priv->intransfer = 0;
  return USBD_OK;
}

uint8_t* USBD_MHID_GetUsrStrDescriptor(uint8_t speed, USBD_Composite_Class_Data* cls, uint8_t index, uint16_t* length) {
  if (index == (USBD_MHID_USRSTR_BASE)) {
    USBD_GetString((uint8_t*)USBD_MHID_INTERFACE_NAME, USBD_StrDesc, length);
    return USBD_StrDesc;
  }

  *length = 0;
  return NULL;
}

int32_t USBD_MHID_Transfer_Status(void* pdev, USBD_Composite_Class_Data* cls)
{
  USBD_MHID_Instance_Data* priv = &USBD_MHID_Instance; /* (USBD_MHID_Instance_Data*)cls->priv; */
  return priv->intransfer && (((USB_OTG_CORE_HANDLE*)pdev)->dev.device_status == USB_OTG_CONFIGURED && priv->configured);
}

uint8_t USBD_MHID_SetDigitizerState(void* pdev, USBD_Composite_Class_Data* cls, uint8_t idx, uint8_t state)
{
  USBD_MHID_Instance_Data* priv = &USBD_MHID_Instance; /* (USBD_MHID_Instance_Data*)cls->priv; */
  if (state) {
    priv->report_descriptor_size = USBD_MHID_REPORT_DESC_SIZE;
  } else {
    priv->report_descriptor_size = USBD_MHID_REPORT_DESC_SIZE - USBD_MHID_DIGITIZER_REPORT_DESC_SIZE;
  }

  return 1;
}