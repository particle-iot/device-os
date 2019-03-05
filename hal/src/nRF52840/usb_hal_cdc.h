#ifndef  _USB_HAL_CDC_H
#define  _USB_HAL_CDC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int usb_hal_init(void);
bool usb_hal_is_enabled(void);
int usb_uart_init(uint8_t *rx_buf, uint16_t rx_buf_size, uint8_t *tx_buf, uint16_t tx_buf_size);
int usb_uart_send(uint8_t data[], uint16_t size);
int usb_uart_available_data(void);

void usb_uart_set_baudrate(uint32_t baudrate);
uint32_t usb_uart_get_baudrate(void);
void usb_hal_attach(void);
void usb_hal_detach(void);

int usb_uart_available_rx_data(void);
uint8_t usb_uart_get_rx_data(void);
uint8_t usb_uart_peek_rx_data(uint8_t index);
void usb_uart_flush_rx_data(void);
void usb_uart_flush_tx_data(void);
int usb_uart_available_tx_data(void);
bool usb_hal_is_connected(void);
void usb_hal_set_bit_rate_changed_handler(void (*handler)(uint32_t bitRate));

#ifdef __cplusplus
}
#endif

#endif

