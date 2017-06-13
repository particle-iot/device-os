#include <string.h>
#include "usbd_mcdc.h"
#include "usbd_desc.h"
#include "usbd_req.h"
#include "debug.h"
#include "logging.h"
#include "ringbuf_helper.h"

LOG_SOURCE_CATEGORY("usb.mcdc")

#define USBD_MCDC_USRSTR_BASE 10

#ifndef MIN
#define MIN(a, b) (a) < (b) ? (a) : (b)
#endif
#ifndef MAX
#define MAX(a, b) (a) > (b) ? (a) : (b)
#endif

static uint8_t  USBD_MCDC_Init        (void* pdev, USBD_Composite_Class_Data* cls, uint8_t cfgidx);
static uint8_t  USBD_MCDC_DeInit      (void* pdev, USBD_Composite_Class_Data* cls, uint8_t cfgidx);
static uint8_t  USBD_MCDC_Setup       (void* pdev, USBD_Composite_Class_Data* cls, USB_SETUP_REQ *req);
static uint8_t  USBD_MCDC_EP0_RxReady (void* pdev, USBD_Composite_Class_Data* cls);
static uint8_t  USBD_MCDC_DataIn      (void* pdev, USBD_Composite_Class_Data* cls, uint8_t epnum);
static uint8_t  USBD_MCDC_DataOut     (void* pdev, USBD_Composite_Class_Data* cls, uint8_t epnum);
static uint8_t  USBD_MCDC_SOF         (void* pdev, USBD_Composite_Class_Data* cls);

static uint8_t* USBD_MCDC_GetCfgDesc (uint8_t speed, USBD_Composite_Class_Data* cls, uint8_t* buf, uint16_t *length);
static uint8_t* USBD_MCDC_GetUsrStrDescriptor(uint8_t speed, USBD_Composite_Class_Data* cls, uint8_t index, uint16_t* length);

static uint16_t USBD_MCDC_Request_Handler(void* pdev, USBD_Composite_Class_Data* cls, uint32_t cmd, uint8_t* buf, uint32_t len);
static int USBD_MCDC_Start_Rx(void *pdev, USBD_MCDC_Instance_Data* priv);
static void USBD_MCDC_Schedule_Out(void *pdev, USBD_MCDC_Instance_Data* priv);

static const uint8_t USBD_MCDC_CfgDesc[USBD_MCDC_CONFIG_DESC_SIZE] __ALIGN_END;

#ifdef CDC_CMD_EP_SHARED
static uint8_t USBD_MCDC_Cmd_Ep_Refcount = 0;
#endif

/* CDC interface class callbacks structure */
USBD_Multi_Instance_cb_Typedef USBD_MCDC_cb =
{
  USBD_MCDC_Init,
  USBD_MCDC_DeInit,
  USBD_MCDC_Setup,
  NULL,                 /* EP0_TxSent, */
  USBD_MCDC_EP0_RxReady,
  USBD_MCDC_DataIn,
  USBD_MCDC_DataOut,
  USBD_MCDC_SOF,
  NULL,
  NULL,
  USBD_MCDC_GetCfgDesc,
#ifdef USE_USB_OTG_HS
  USBD_MCDC_GetCfgDesc, /* use same cobfig as per FS */
#endif /* USE_USB_OTG_HS  */
  USBD_MCDC_GetUsrStrDescriptor,

};

static const uint8_t USBD_MCDC_CfgDesc[USBD_MCDC_CONFIG_DESC_SIZE] __ALIGN_END =
{
  /* Interface Association Descriptor */
  0x08,   /* bLength: Configuration Descriptor size */
  0x0b,   /* bDescriptorType: IAD */
  0x00,   /* bFirstInterface */
  0x02,   /* bInterfaceCount */
  0x02,   /* bFunctionClass */
  0x02,   /* bFunctionSubClass */
  0x01,   /* bFunctionProtocol */
  0x00,   /* iFunction */

  /*Interface Descriptor */
  0x09,   /* bLength: Interface Descriptor size */
  USB_INTERFACE_DESCRIPTOR_TYPE,  /* bDescriptorType: Interface */
  /* Interface descriptor type */
  0x00,   /* bInterfaceNumber: Number of Interface */
  0x00,   /* bAlternateSetting: Alternate setting */
  0x01,   /* bNumEndpoints: One endpoints used */
  0x02,   /* bInterfaceClass: Communication Interface Class */
  0x02,   /* bInterfaceSubClass: Abstract Control Model */
  0x01,   /* bInterfaceProtocol: Common AT commands */
  0x00,   /* iInterface: */

  /*Header Functional Descriptor*/
  0x05,   /* bLength: Endpoint Descriptor size */
  0x24,   /* bDescriptorType: CS_INTERFACE */
  0x00,   /* bDescriptorSubtype: Header Func Desc */
  0x10,   /* bcdCDC: spec release number */
  0x01,

  /*Call Management Functional Descriptor*/
  0x05,   /* bFunctionLength */
  0x24,   /* bDescriptorType: CS_INTERFACE */
  0x01,   /* bDescriptorSubtype: Call Management Func Desc */
  0x00,   /* bmCapabilities: D0+D1 */
  0x01,   /* bDataInterface: 1 */

  /*ACM Functional Descriptor*/
  0x04,   /* bFunctionLength */
  0x24,   /* bDescriptorType: CS_INTERFACE */
  0x02,   /* bDescriptorSubtype: Abstract Control Management desc */
  0x02,   /* bmCapabilities */

  /*Union Functional Descriptor*/
  0x05,   /* bFunctionLength */
  0x24,   /* bDescriptorType: CS_INTERFACE */
  0x06,   /* bDescriptorSubtype: Union func desc */
  0x00,   /* bMasterInterface: Communication class interface */
  0x01,   /* bSlaveInterface0: Data Class Interface */

  /*Endpoint 2 Descriptor*/
  0x07,                           /* bLength: Endpoint Descriptor size */
  USB_ENDPOINT_DESCRIPTOR_TYPE,   /* bDescriptorType: Endpoint */
  CDC0_CMD_EP,                     /* bEndpointAddress */
  0x03,                           /* bmAttributes: Interrupt */
  LOBYTE(CDC_CMD_PACKET_SZE),     /* wMaxPacketSize: */
  HIBYTE(CDC_CMD_PACKET_SZE),
#ifdef USE_USB_OTG_HS
  0x10,                          /* bInterval: */
#else
  0xFF,                           /* bInterval: */
#endif /* USE_USB_OTG_HS */

  /*---------------------------------------------------------------------------*/

  /*Data class interface descriptor*/
  0x09,   /* bLength: Endpoint Descriptor size */
  USB_INTERFACE_DESCRIPTOR_TYPE,  /* bDescriptorType: */
  0x01,   /* bInterfaceNumber: Number of Interface */
  0x00,   /* bAlternateSetting: Alternate setting */
  0x02,   /* bNumEndpoints: Two endpoints used */
  0x0A,   /* bInterfaceClass: CDC */
  0x00,   /* bInterfaceSubClass: */
  0x00,   /* bInterfaceProtocol: */
  0x00,   /* iInterface: */

  /*Endpoint OUT Descriptor*/
  0x07,   /* bLength: Endpoint Descriptor size */
  USB_ENDPOINT_DESCRIPTOR_TYPE,      /* bDescriptorType: Endpoint */
  CDC0_OUT_EP,                        /* bEndpointAddress */
  0x02,                              /* bmAttributes: Bulk */
  LOBYTE(CDC_DATA_MAX_PACKET_SIZE),  /* wMaxPacketSize: */
  HIBYTE(CDC_DATA_MAX_PACKET_SIZE),
#ifdef USE_USB_OTG_HS
  0x10,                          /* bInterval: */
#else
  0xFF,                          /* bInterval: */
#endif /* USE_USB_OTG_HS */

  /*Endpoint IN Descriptor*/
  0x07,   /* bLength: Endpoint Descriptor size */
  USB_ENDPOINT_DESCRIPTOR_TYPE,      /* bDescriptorType: Endpoint */
  CDC0_IN_EP,                         /* bEndpointAddress */
  0x02,                              /* bmAttributes: Bulk */
  LOBYTE(CDC_DATA_MAX_PACKET_SIZE),  /* wMaxPacketSize: */
  HIBYTE(CDC_DATA_MAX_PACKET_SIZE),
  0x00                               /* bInterval: ignore for Bulk transfer */
};

static void USBD_MCDC_Change_Open_State(void* pdev, USBD_MCDC_Instance_Data* priv, uint8_t state) {
  if (state != priv->serial_open) {
    USBD_Composite_Class_Data* cls = (USBD_Composite_Class_Data*)priv->cls;
    (void)cls;
    //LOG_DEBUG(TRACE, "[%s] USB Serial state: %d", priv->name, state);
    if (state) {
      priv->tx_failed_counter = 0;
      // Also flush everything in TX buffer
      priv->tx_buffer_head = priv->tx_buffer_tail = priv->tx_buffer_last = 0;

      priv->tx_state = 0;
      USBD_MCDC_Schedule_Out(pdev, priv);
    }
    priv->serial_open = state;
  }
}

static uint8_t USBD_MCDC_Init(void* pdev, USBD_Composite_Class_Data* cls, uint8_t cfgidx)
{
  USBD_MCDC_Instance_Data* priv = (USBD_MCDC_Instance_Data*)cls->priv;
  USBD_MCDC_DeInit(pdev, cls, cfgidx);

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  memcpy(priv->descriptor, cls->cfg, USBD_MCDC_DESC_SIZE);
#endif

  /* Open EP IN */
  DCD_EP_Open(pdev,
              priv->ep_in_data,
              CDC_DATA_IN_PACKET_SIZE,
              USB_OTG_EP_BULK);

  /* Open EP OUT */
  DCD_EP_Open(pdev,
              priv->ep_out_data,
              CDC_DATA_OUT_PACKET_SIZE,
              USB_OTG_EP_BULK);

#ifdef CDC_CMD_EP_SHARED
  if (USBD_MCDC_Cmd_Ep_Refcount++ == 0) {
#endif
  if (priv->ep_in_int != priv->ep_in_data) {
    /* Open Command IN EP */
    DCD_EP_Open(pdev,
                priv->ep_in_int,
                CDC_CMD_PACKET_SZE,
                USB_OTG_EP_INT);
  }
#ifdef CDC_CMD_EP_SHARED
  }
#endif
  priv->configured = 1;
  priv->rx_state = 0;

  USBD_MCDC_Start_Rx(pdev, priv);

  return USBD_OK;
}

static uint8_t USBD_MCDC_DeInit(void* pdev, USBD_Composite_Class_Data* cls, uint8_t cfgidx)
{
  USBD_MCDC_Instance_Data* priv = (USBD_MCDC_Instance_Data*)cls->priv;
  USBD_MCDC_Change_Open_State(pdev, priv, 0);

  if (priv->configured) {
    if (priv->tx_state) {
      DCD_EP_Flush(pdev, priv->ep_in_data);
    }

    /* Close EP IN */
    DCD_EP_Close(pdev,
                 priv->ep_in_data);

    /* Close EP OUT */
    DCD_EP_Close(pdev,
                 priv->ep_out_data);
#ifdef CDC_CMD_EP_SHARED
    if (--USBD_MCDC_Cmd_Ep_Refcount == 0) {
#endif
    if (priv->ep_in_int != priv->ep_in_data) {
      /* Close Command IN EP */
      DCD_EP_Close(pdev,
                   priv->ep_in_int);
    }
#ifdef CDC_CMD_EP_SHARED
    }
#endif
  }

  USBD_MCDC_Change_Open_State(pdev, priv, 0);

  priv->configured = 0;
  priv->tx_state = 0;
  priv->rx_state = 0;
  priv->rx_buffer_head = 0;
  priv->rx_buffer_tail = 0;
  priv->rx_buffer_length = priv->rx_buffer_size;
  priv->tx_buffer_head = 0;
  priv->tx_buffer_tail = 0;
  priv->tx_buffer_last = 0;
  priv->frame_count = 0;  
  priv->cmd = NO_CMD;
  priv->ctrl_line = 0x00;

  return USBD_OK;
}

static uint8_t USBD_MCDC_Setup(void* pdev, USBD_Composite_Class_Data* cls, USB_SETUP_REQ *req)
{
  USBD_MCDC_Instance_Data* priv = (USBD_MCDC_Instance_Data*)cls->priv;
  uint16_t len = 0;
  uint8_t* pbuf = NULL;

  switch (req->bmRequest & USB_REQ_TYPE_MASK)
  {
    /* CDC Class Requests -------------------------------*/
  case USB_REQ_TYPE_CLASS :
      /* Check if the request is a data setup packet */
      if (req->wLength)
      {
        /* Check if the request is Device-to-Host */
        if (req->bmRequest & 0x80)
        {
          /* Get the data to be sent to Host from interface layer */
          USBD_MCDC_Request_Handler(pdev, cls, req->bRequest, priv->cmd_buffer, req->wLength);

          /* Send the data to the host */
          USBD_CtlSendData (pdev,
                            priv->cmd_buffer,
                            req->wLength);
        }
        else /* Host-to-Device requeset */
        {
          /* Set the value of the current command to be processed */
          priv->cmd = req->bRequest;
          priv->cmd_len = req->wLength;

          /* Prepare the reception of the buffer over EP0
          Next step: the received data will be managed in usbd_cdc_EP0_TxSent()
          function. */
          USBD_CtlPrepareRx (pdev,
                             priv->cmd_buffer,
                             req->wLength);
        }
      }
      else /* No Data request */
      {
        /* Transfer the command to the interface layer */
        USBD_MCDC_Request_Handler(pdev, cls, req->bRequest, (uint8_t*)&req->wValue, sizeof(req->wValue));
      }

      return USBD_OK;

  /* Standard Requests -------------------------------*/
  case USB_REQ_TYPE_STANDARD: {
      switch (req->bRequest)
      {
      case USB_REQ_GET_DESCRIPTOR:
        if( (req->wValue >> 8) == CDC_DESCRIPTOR_TYPE)
        {
          // For CDC this request should never arrive probably
  #ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
          pbuf = priv->descriptor;
  #else
          pbuf = cls->cfg;
  #endif
          len = MIN(USBD_MCDC_CONFIG_DESC_SIZE , req->wLength);
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
        if ((uint8_t)(req->wValue) < USBD_ITF_MAX_NUM)
        {
          priv->alt_set = (uint8_t)(req->wValue);
        }
        else
        {
          /* Call the error management function (command will be nacked */
          USBD_CtlError (pdev, req);
        }
        break;
      default:
        USBD_CtlError(pdev, req);
        return USBD_OK;
      }
    break;
    }

  default:
    USBD_CtlError (pdev, req);
    return USBD_FAIL;
  }
  return USBD_OK;
}

static uint8_t USBD_MCDC_EP0_RxReady(void* pdev, USBD_Composite_Class_Data* cls)
{
  USBD_MCDC_Instance_Data* priv = (USBD_MCDC_Instance_Data*)cls->priv;

  if (priv->cmd != NO_CMD)
  {
    // USBD_MCDC_Change_Open_State(priv, 1);
    /* Process the data */
    USBD_MCDC_Request_Handler(pdev, cls, priv->cmd, priv->cmd_buffer, priv->cmd_len);

    /* Reset the command variable to default value */
    priv->cmd = NO_CMD;
  }

  return USBD_OK;
}

static inline uint32_t USBD_Last_Tx_Packet_size(void *pdev, uint8_t epnum)
{
  return ((USB_OTG_CORE_HANDLE*)pdev)->dev.in_ep[epnum].xfer_len;
}

static inline uint32_t USBD_Last_Rx_Packet_size(void *pdev, uint8_t epnum)
{
  return ((USB_OTG_CORE_HANDLE*)pdev)->dev.out_ep[epnum].xfer_count;
}

static uint8_t USBD_MCDC_DataIn(void* pdev, USBD_Composite_Class_Data* cls, uint8_t epnum)
{
  USBD_MCDC_Instance_Data* priv = (USBD_MCDC_Instance_Data*)cls->priv;
  uint32_t USB_Tx_length;

  if ((epnum | 0x80) != (priv->ep_in_data) && (epnum | 0x80) != (priv->ep_in_int)) {
    return USBD_FAIL;
  }

  USBD_MCDC_Change_Open_State(pdev, priv, 1);

  if (!priv->tx_state)
    return USBD_OK;

  priv->tx_failed_counter = 0;
  priv->tx_buffer_tail = ring_wrap(priv->tx_buffer_size, priv->tx_buffer_tail + priv->tx_buffer_last);
  priv->tx_buffer_last = 0;

  USB_Tx_length = ring_data_contig(priv->tx_buffer_size, priv->tx_buffer_head, priv->tx_buffer_tail);

  if (USB_Tx_length) {
    USB_Tx_length = MIN(USB_Tx_length, CDC_DATA_IN_PACKET_SIZE);
  } else if (USBD_Last_Tx_Packet_size(pdev, epnum) != CDC_DATA_IN_PACKET_SIZE) {
    priv->tx_state = 0;
    return USBD_OK;
  }

  priv->tx_buffer_last = USB_Tx_length;

  /* Prepare the available data buffer to be sent on IN endpoint */
  DCD_EP_Tx (pdev,
             priv->ep_in_data,
             (uint8_t*)&priv->tx_buffer[priv->tx_buffer_tail],
             USB_Tx_length);

  return USBD_OK;
}

int USBD_MCDC_Start_Rx(void *pdev, USBD_MCDC_Instance_Data* priv)
{

  /* USB_Rx_Buffer_length is used here to keep track of
   * available _contiguous_ buffer space in USB_Rx_Buffer.
   */
  uint32_t USB_Rx_length;
  if (priv->rx_buffer_head >= priv->rx_buffer_tail)
    priv->rx_buffer_length = priv->rx_buffer_size;

  USB_Rx_length = ring_space_contig(priv->rx_buffer_length, priv->rx_buffer_head, priv->rx_buffer_tail);

  if (USB_Rx_length < CDC_DATA_OUT_PACKET_SIZE) {
    USB_Rx_length = ring_space_wrapped(priv->rx_buffer_length, priv->rx_buffer_head, priv->rx_buffer_tail);
    if (USB_Rx_length < CDC_DATA_OUT_PACKET_SIZE) {
      if (priv->rx_state) {
        priv->rx_state = 0;
        DCD_SetEPStatus(pdev, priv->ep_out_data, USB_OTG_EP_RX_NAK);
      }
      return 0;
    }
    priv->rx_buffer_length = priv->rx_buffer_head;
    priv->rx_buffer_head = 0;
    if (priv->rx_buffer_tail == priv->rx_buffer_length)
      priv->rx_buffer_tail = 0;
  }
  if (!priv->rx_state) {
    priv->rx_state = 1;
  }

  DCD_EP_PrepareRx(pdev,
                   priv->ep_out_data,
                   priv->rx_buffer + priv->rx_buffer_head,
                   CDC_DATA_OUT_PACKET_SIZE);
  return 1;
}

static uint8_t  USBD_MCDC_DataOut(void* pdev, USBD_Composite_Class_Data* cls, uint8_t epnum)
{
  USBD_MCDC_Instance_Data* priv = (USBD_MCDC_Instance_Data*)cls->priv;
  if (epnum != (priv->ep_out_data)) {
    return USBD_FAIL;
  }

  if (priv->rx_state) {
    uint32_t USB_Rx_Count = USBD_Last_Rx_Packet_size(pdev, epnum);
    priv->rx_buffer_head = ring_wrap(priv->rx_buffer_length, priv->rx_buffer_head + USB_Rx_Count);
  }

  // Serial port is definitely open
  USBD_MCDC_Change_Open_State(pdev, priv, 1);

  USBD_MCDC_Start_Rx(pdev, priv);

  return USBD_OK;
}

void USBD_MCDC_Schedule_Out(void *pdev, USBD_MCDC_Instance_Data* priv)
{
  if (!priv->rx_state)
    USBD_MCDC_Start_Rx(pdev, priv);
}

static void USBD_MCDC_Schedule_In(void *pdev, USBD_MCDC_Instance_Data* priv)
{
  uint32_t USB_Tx_length;

  if (priv->tx_state) {
    if (priv->serial_open) {
      priv->tx_failed_counter++;
      if (priv->tx_failed_counter >= 1000) {
        USBD_MCDC_Change_Open_State(pdev, priv, 0);
      }
    }

    if (!priv->serial_open) {
        // Completely flush TX buffer
        DCD_EP_Flush(pdev, priv->ep_in_data);
        // Send ZLP
        DCD_EP_Tx(pdev, priv->ep_in_data, NULL, 0);
        priv->tx_buffer_head = 0;
        priv->tx_buffer_tail = 0;
        priv->tx_buffer_last = 0;
        USB_Tx_length = 0;

        priv->tx_state = 0;
    }
    return;
  }

  priv->tx_buffer_tail = ring_wrap(priv->tx_buffer_size, priv->tx_buffer_tail + priv->tx_buffer_last);
  priv->tx_buffer_last = 0;

  USB_Tx_length = ring_data_contig(priv->tx_buffer_size, priv->tx_buffer_head, priv->tx_buffer_tail);

  if (!USB_Tx_length)
    return;

  priv->tx_state = 1;
  priv->tx_failed_counter = 0;

  USB_Tx_length = MIN(USB_Tx_length, CDC_DATA_IN_PACKET_SIZE);
  priv->tx_buffer_last = USB_Tx_length;

  DCD_EP_Tx (pdev,
             priv->ep_in_data,
             (uint8_t*)&priv->tx_buffer[priv->tx_buffer_tail],
             USB_Tx_length);
}

static uint8_t USBD_MCDC_SOF(void* pdev, USBD_Composite_Class_Data* cls)
{
  USBD_MCDC_Instance_Data* priv = (USBD_MCDC_Instance_Data*)cls->priv;

  USBD_MCDC_Schedule_In(pdev, priv);
  USBD_MCDC_Schedule_Out(pdev, priv);

  return USBD_OK;
}

static uint8_t* USBD_MCDC_GetCfgDesc(uint8_t speed, USBD_Composite_Class_Data* cls, uint8_t* buf, uint16_t *length)
{
  USBD_MCDC_Instance_Data* priv = (USBD_MCDC_Instance_Data*)cls->priv;
  *length = USBD_MCDC_CONFIG_DESC_SIZE;
  
  cls->interfaces = 2;
  memcpy(buf, USBD_MCDC_CfgDesc, *length);
  // Update bNumInterfaces in IAD
  *(buf + 2) = cls->firstInterface;
  // Update bInterfaceNumber 0
  *(buf + 10) = cls->firstInterface;
  // Update iFunction in IAD
  *(buf + 7) = USBD_MCDC_USRSTR_BASE + cls->firstInterface;
  // Update iInterface
  *(buf + 10 + 6) = USBD_MCDC_USRSTR_BASE + cls->firstInterface;
  // Update bDataInterface
  *(buf + 26) = cls->firstInterface + 1;
  // Update bMasterInterface and bSlaveInterface0
  *(buf + 34) = cls->firstInterface;
  *(buf + 35) = cls->firstInterface + 1;

  // XXX: Compatibility for old Windows driver
#ifdef CDC0_CMD_EP_COMPAT
  // Serial
  if (cls->firstInterface == 0) {
    if (USBD_Composite_Registered_Count(true) == 1) {
      priv->ep_in_int = CDC0_CMD_EP_COMPAT;
    } else {
      priv->ep_in_int = CDC0_CMD_EP;
    }
  }
#endif

  // Update CMD endpoint
  *(buf + 38) = priv->ep_in_int;
  
  // Update bInterfaceNumber 1
  *(buf + 45) = cls->firstInterface + 1;
  // Update iInterface
  *(buf + 45 + 6) = USBD_MCDC_USRSTR_BASE + cls->firstInterface;

  // Update OUT endpoint
  *(buf + 54) = priv->ep_out_data;

  // Update IN endpoint
  *(buf + 61) = priv->ep_in_data;
#ifndef CDC_CMD_EP_SHARED
  cls->epMask = (1 << (priv->ep_in_data & 0x7f)) | (1 << (priv->ep_in_int & 0x7f)) |
                ((1 << (priv->ep_out_data & 0x7f)) << 16);
#else
  cls->epMask = (1 << (priv->ep_in_data & 0x7f)) | ((cls->firstInterface == 0 ? 1 : 0) << (priv->ep_in_int & 0x7f)) |
                ((1 << (priv->ep_out_data & 0x7f)) << 16);
#endif
  return buf;
}

static uint16_t USBD_MCDC_Request_Handler(void* pdev, USBD_Composite_Class_Data* cls, uint32_t cmd, uint8_t* buf, uint32_t len) {
  USBD_MCDC_Instance_Data* priv = (USBD_MCDC_Instance_Data*)cls->priv;
  switch (cmd)
  {
  case SEND_ENCAPSULATED_COMMAND:
      /* Not needed for this driver */
      break;

  case GET_ENCAPSULATED_RESPONSE:
      /* Not needed for this driver */
      break;

  case SET_COMM_FEATURE:
      /* Not needed for this driver */
      break;

  case GET_COMM_FEATURE:
      /* Not needed for this driver */
      break;

  case CLEAR_COMM_FEATURE:
      /* Not needed for this driver */
      break;

  case SET_LINE_CODING:
      priv->linecoding.bitrate = (uint32_t)(buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24));
      priv->linecoding.format = buf[4];
      priv->linecoding.paritytype = buf[5];
      priv->linecoding.datatype = buf[6];
      //LOG_DEBUG(TRACE, "[%s] SET_LINE_CODING %d", priv->name, priv->linecoding.bitrate);
      break;

  case GET_LINE_CODING:
      buf[0] = (uint8_t)(priv->linecoding.bitrate);
      buf[1] = (uint8_t)(priv->linecoding.bitrate >> 8);
      buf[2] = (uint8_t)(priv->linecoding.bitrate >> 16);
      buf[3] = (uint8_t)(priv->linecoding.bitrate >> 24);
      buf[4] = priv->linecoding.format;
      buf[5] = priv->linecoding.paritytype;
      buf[6] = priv->linecoding.datatype;
      break;

  case SET_CONTROL_LINE_STATE:
      priv->ctrl_line = buf[0];
      if (priv->ctrl_line & CDC_DTR) {
        USBD_MCDC_Change_Open_State(pdev, priv, 1);
      } else if ((priv->ctrl_line & CDC_DTR) == 0x00) {
        USBD_MCDC_Change_Open_State(pdev, priv, 0);
      }
      //LOG_DEBUG(TRACE, "[%s] SET_CONTROL_LINE_STATE DTR=%d RTS=%d", priv->name, priv->ctrl_line & CDC_DTR ? 1 : 0, priv->ctrl_line & CDC_RTS ? 1 : 0);
      break;

  case SEND_BREAK:
      /* Not needed for this driver */
      break;

  default:
      break;
  }

  if (priv->req_handler) {
    priv->req_handler(cls, cmd, buf, len);
  }

  return USBD_OK;
}

uint8_t* USBD_MCDC_GetUsrStrDescriptor(uint8_t speed, USBD_Composite_Class_Data* cls, uint8_t index, uint16_t* length) {
  USBD_MCDC_Instance_Data* priv = (USBD_MCDC_Instance_Data*)cls->priv;

  if (index == (USBD_MCDC_USRSTR_BASE + cls->firstInterface)) {
    USBD_GetString((uint8_t*)priv->name, USBD_StrDesc, length);
    return USBD_StrDesc;
  }

  *length = 0;
  return NULL;
}
