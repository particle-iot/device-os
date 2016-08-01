#ifndef USBD_MCDC_H_
#define USBD_MCDC_H_

#include <stdint.h>
#include "usbd_ioreq.h"
#include "usbd_composite.h"

#define USBD_MCDC_CONFIG_DESC_SIZE              (58 + 8) // + IAD
#define USBD_MCDC_DESC_SIZE                     (USBD_MCDC_CONFIG_DESC_SIZE)

#define CDC_DESCRIPTOR_TYPE                     0x21

#define DEVICE_CLASS_CDC                        0x02
#define DEVICE_SUBCLASS_CDC                     0x00

#define USB_DEVICE_DESCRIPTOR_TYPE              0x01
#define USB_CONFIGURATION_DESCRIPTOR_TYPE       0x02
#define USB_STRING_DESCRIPTOR_TYPE              0x03
#define USB_INTERFACE_DESCRIPTOR_TYPE           0x04
#define USB_ENDPOINT_DESCRIPTOR_TYPE            0x05

#define STANDARD_ENDPOINT_DESC_SIZE             0x09

#define CDC_DATA_IN_PACKET_SIZE                 CDC_DATA_MAX_PACKET_SIZE

#define CDC_DATA_OUT_PACKET_SIZE                CDC_DATA_MAX_PACKET_SIZE

/*---------------------------------------------------------------------*/
/*  CDC definitions                                                    */
/*---------------------------------------------------------------------*/

/**************************************************/
/* CDC Requests                                   */
/**************************************************/
#define SEND_ENCAPSULATED_COMMAND               0x00
#define GET_ENCAPSULATED_RESPONSE               0x01
#define SET_COMM_FEATURE                        0x02
#define GET_COMM_FEATURE                        0x03
#define CLEAR_COMM_FEATURE                      0x04
#define SET_LINE_CODING                         0x20
#define GET_LINE_CODING                         0x21
#define SET_CONTROL_LINE_STATE                  0x22
#define SEND_BREAK                              0x23
#define NO_CMD                                  0xFF
#define ACM_SERIAL_STATE                        0x20

#define CDC_DTR                                 0x01
#define CDC_RTS                                 0x02

typedef struct USBD_MCDC_Instance_Data {
  // Back-reference to USBD_Composite_Class_Data
  void* cls;

  LINE_CODING linecoding;
  uint8_t ep_in_data;
  uint8_t ep_out_data;
  uint8_t ep_in_int;

  __ALIGN_BEGIN volatile uint32_t alt_set __ALIGN_END;
  
  uint8_t* rx_buffer;
  uint16_t rx_buffer_size;
  volatile uint32_t rx_buffer_head;
  volatile uint32_t rx_buffer_tail;
  volatile uint32_t rx_buffer_length;

  uint8_t* tx_buffer;
  uint16_t tx_buffer_size;
  volatile uint32_t tx_buffer_head;
  volatile uint32_t tx_buffer_tail;
  volatile uint32_t tx_failed_counter;
  volatile uint32_t tx_buffer_last;

  __ALIGN_BEGIN uint8_t cmd_buffer[CDC_CMD_PACKET_SZE] __ALIGN_END;

  volatile uint8_t rx_state;
  volatile uint8_t tx_state;
  volatile uint8_t serial_open;

  uint8_t configured;

  uint32_t cmd;
  uint32_t cmd_len;

  uint32_t frame_count;

  uint8_t ctrl_line;

  uint16_t (*req_handler) (USBD_Composite_Class_Data* cls, uint32_t cmd, uint8_t* buf, uint32_t len);

  const char* name;

  #ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  // Temporary aligned buffer
  __ALIGN_BEGIN uint8_t descriptor[USBD_MCDC_DESC_SIZE] __ALIGN_END;
  #endif
} USBD_MCDC_Instance_Data;

extern USBD_Multi_Instance_cb_Typedef USBD_MCDC_cb;

#endif /* USBD_MCDC_H_ */