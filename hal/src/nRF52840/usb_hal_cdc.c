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

#include "logging.h"
LOG_SOURCE_CATEGORY("hal.usb.cdc");

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "nrf.h"
#include "nrf_drv_usbd.h"
#include "nrf_drv_clock.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_drv_power.h"

#include "app_error.h"
#include "app_util.h"
#include "app_usbd_core.h"
#include "app_usbd.h"
#include "app_usbd_string_desc.h"
#include "app_usbd_cdc_acm.h"
#include "app_usbd_serial_num.h"
#include "app_fifo.h"
#include "usb_hal_cdc.h"
#include "deviceid_hal.h"
#include "bytes2hexbuf.h"
#include "hal_platform.h"
#include "system_error.h"
#include "interrupts_hal.h"

/**
 * @brief Enable power USB detection
 *
 * Configure if example supports USB port connection
 */
#ifndef USBD_POWER_DETECTION
#define USBD_POWER_DETECTION true
#endif

#define CDC_ACM_COMM_INTERFACE  0
#define CDC_ACM_COMM_EPIN       NRF_DRV_USBD_EPIN2

#define CDC_ACM_DATA_INTERFACE  1
#define CDC_ACM_DATA_EPIN       NRF_DRV_USBD_EPIN1
#define CDC_ACM_DATA_EPOUT      NRF_DRV_USBD_EPOUT1

#define MAX_USB_STATE_CB_NUM    1

extern uint8_t g_extern_serial_number[SERIAL_NUMBER_STRING_SIZE + 1];

typedef enum {
     USB_MODE_NONE,
     USB_MODE_CDC_UART,
     USB_MODE_HID
} usb_mode_t;

typedef struct {
    bool                    initialized;
    volatile HAL_USB_State  state;
    usb_mode_t              mode;

    app_fifo_t              rx_fifo;
    app_fifo_t              tx_fifo;

    volatile bool           com_opened;
    volatile bool           transmitting;
    volatile uint32_t       tx_failed;
    volatile uint32_t       rx_data_size;
    volatile bool           rx_done;
    volatile bool           rx_state;

    void (*bit_rate_changed_handler)(uint32_t bitRate);
    HAL_USB_State_Callback  state_callback[MAX_USB_STATE_CB_NUM];
    void*                   state_callback_context[MAX_USB_STATE_CB_NUM];

    volatile bool enabled;
} usb_instance_t;

static usb_instance_t m_usb_instance = {0};

// Rx buffer length must by multiple of NRF_DRV_USBD_EPSIZE.
#define READ_SIZE       (NRF_DRV_USBD_EPSIZE * 2)
static char m_rx_buffer[READ_SIZE];
#define SEND_SIZE       NRF_DRV_USBD_EPSIZE
static char             m_tx_buffer[SEND_SIZE];

static void cdc_acm_user_ev_handler(app_usbd_class_inst_t const * p_inst,
                                    app_usbd_cdc_acm_user_event_t event);
/**
 * @brief CDC_ACM class instance
 * */
APP_USBD_CDC_ACM_GLOBAL_DEF(m_app_cdc_acm,
                            cdc_acm_user_ev_handler,
                            CDC_ACM_COMM_INTERFACE,
                            CDC_ACM_DATA_INTERFACE,
                            CDC_ACM_COMM_EPIN,
                            CDC_ACM_DATA_EPIN,
                            CDC_ACM_DATA_EPOUT,
                            APP_USBD_CDC_COMM_PROTOCOL_AT_V250
);

#define FIFO_LENGTH(p_fifo)     fifo_length(p_fifo)  /**< Macro for calculating the FIFO length. */
#define IS_FIFO_FULL(p_fifo)    fifo_full(p_fifo)
static __INLINE uint32_t fifo_length(app_fifo_t * p_fifo) {
    uint32_t tmp = p_fifo->read_pos;
    return p_fifo->write_pos - tmp;
}
static __INLINE bool fifo_full(app_fifo_t * p_fifo) {
    return (FIFO_LENGTH(p_fifo) > p_fifo->buf_size_mask);
}

static void reset_rx_tx_state(void) {
    m_usb_instance.rx_done = false;
    m_usb_instance.rx_data_size = 0;
    m_usb_instance.transmitting = false;
    m_usb_instance.tx_failed = 0;
    m_usb_instance.rx_state = false;
    app_fifo_flush(&m_usb_instance.tx_fifo);
    app_fifo_flush(&m_usb_instance.rx_fifo);
}

static void set_usb_state(HAL_USB_State state) {
    if (m_usb_instance.state != state) {
        m_usb_instance.state = state;
        for (int i = 0; i < MAX_USB_STATE_CB_NUM; i++) {
            if (m_usb_instance.state_callback[i]) {
                (*m_usb_instance.state_callback[i])(state, m_usb_instance.state_callback_context[i]);
            } else {
                break;
            }
        }
    }
}

static bool usb_cdc_copy_from_rx_buffer() {
    if (m_usb_instance.rx_done && (FIFO_LENGTH(&m_usb_instance.rx_fifo) + m_usb_instance.rx_data_size) <= m_usb_instance.rx_fifo.buf_size_mask) {
        // Receive data into buffer
        for (uint32_t i = 0; i < m_usb_instance.rx_data_size; i++) {
            SPARK_ASSERT(app_fifo_put(&m_usb_instance.rx_fifo, m_rx_buffer[i]) == NRF_SUCCESS);
        }
        return true;
    }

    return false;
}

static bool usb_cdc_handle_rx() {
    m_usb_instance.rx_data_size = app_usbd_cdc_acm_rx_size(&m_app_cdc_acm);
    m_usb_instance.rx_done = true;
    m_usb_instance.rx_state = false;

    if (m_usb_instance.rx_data_size == 0 || usb_cdc_copy_from_rx_buffer()) {
        // Reset receive status
        m_usb_instance.rx_done = false;
        m_usb_instance.rx_data_size = 0;

        return true;
    }

    return false;
}

static void usb_cdc_schedule_rx() {
    if (m_usb_instance.com_opened && !m_usb_instance.rx_state && !m_usb_instance.rx_done) {
        // Enforce DTR state, otherwise the Nordic SDK
        // will disallow us to schedule any transfers in the closed state
        uint32_t cache = m_app_cdc_acm.specific.p_data->ctx.line_state;
        m_app_cdc_acm.specific.p_data->ctx.line_state |= APP_USBD_CDC_ACM_LINE_STATE_DTR;
        ret_code_t r = app_usbd_cdc_acm_read_any(&m_app_cdc_acm, m_rx_buffer, READ_SIZE);
        if (r == NRF_ERROR_IO_PENDING) {
            m_usb_instance.rx_data_size = 0;
            m_usb_instance.rx_done = false;
            m_usb_instance.rx_state = true;
        } else if (r == NRF_SUCCESS) {
            // Data has already been stored into the RX buffer
            usb_cdc_handle_rx();
        }
        m_app_cdc_acm.specific.p_data->ctx.line_state = cache;
    }
}

static void usb_cdc_change_port_state(bool state)
{
    if (state != m_usb_instance.com_opened) {
        m_usb_instance.com_opened = state;

        if (state) {
            app_fifo_flush(&m_usb_instance.tx_fifo);
            m_app_cdc_acm.specific.p_data->ctx.line_state |= APP_USBD_CDC_ACM_LINE_STATE_DTR;
            usb_cdc_schedule_rx();
        } else {
            // When going into closed state Nordic SDK
            // aborts transfers!
            m_usb_instance.tx_failed = 0;
            m_usb_instance.transmitting = false;
        }
    }
}

static void usb_cdc_schedule_tx() {
    uint32_t cache = m_app_cdc_acm.specific.p_data->ctx.line_state;
    if (m_usb_instance.com_opened && m_usb_instance.transmitting &&
            m_usb_instance.tx_failed++ >= HAL_PLATFORM_USB_CDC_TX_FAIL_TIMEOUT_MS) {
        // Failed to transmit (probably lost host)
        // Make sure to cancel ongoing transfer
        nrf_drv_usbd_ep_abort(CDC_ACM_DATA_EPIN);
        m_usb_instance.transmitting = false;
        usb_cdc_change_port_state(false);
        // Send ZLP, otherwise the Nordic SDK
        // will disallow us to schedule any transfers in the closed state
        m_app_cdc_acm.specific.p_data->ctx.line_state |= APP_USBD_CDC_ACM_LINE_STATE_DTR;
        app_usbd_cdc_acm_write(&m_app_cdc_acm, m_tx_buffer, 0);
        m_app_cdc_acm.specific.p_data->ctx.line_state = cache;
    }

    if (m_usb_instance.com_opened && !m_usb_instance.transmitting && FIFO_LENGTH(&m_usb_instance.tx_fifo) > 0) {
        size_t to_send = 0;
        uint8_t data;
        while ((to_send < SEND_SIZE) && app_fifo_get(&m_usb_instance.tx_fifo, &data) == NRF_SUCCESS) {
            m_tx_buffer[to_send++] = data;
        }

        m_usb_instance.tx_failed = 0;
        m_app_cdc_acm.specific.p_data->ctx.line_state |= APP_USBD_CDC_ACM_LINE_STATE_DTR;
        if (app_usbd_cdc_acm_write(&m_app_cdc_acm, m_tx_buffer, to_send) == NRF_SUCCESS) {
            m_usb_instance.transmitting = true;
        }
        m_app_cdc_acm.specific.p_data->ctx.line_state = cache;
    }
}

/**
 * @brief User event handler @ref app_usbd_cdc_acm_user_ev_handler_t (headphones)
 * */
static void cdc_acm_user_ev_handler(app_usbd_class_inst_t const * p_inst,
                                    app_usbd_cdc_acm_user_event_t event)
{
    switch (event) {
        case APP_USBD_CDC_ACM_USER_EVT_SET_LINE_CODING: {
            if (m_usb_instance.bit_rate_changed_handler) {
                (*m_usb_instance.bit_rate_changed_handler)(usb_uart_get_baudrate());
            }
            break;
        }
        case APP_USBD_CDC_ACM_USER_EVT_PORT_OPEN: {
            // Remote side asserted DTR
            usb_cdc_change_port_state(true);
            break;
        }
        case APP_USBD_CDC_ACM_USER_EVT_PORT_CLOSE: {
            // Remote side deasserted DTR
            // NOTE: Nordic SDK will abort any outstanding RX/TX transfers.
            m_usb_instance.rx_state = false;
            usb_cdc_change_port_state(false);
            break;
        }
        case APP_USBD_CDC_ACM_USER_EVT_TX_DONE: {
            m_usb_instance.transmitting = false;
            m_usb_instance.tx_failed = 0;

            // We are definitely open
            usb_cdc_change_port_state(true);

            usb_cdc_schedule_tx();
            break;
        }
        case APP_USBD_CDC_ACM_USER_EVT_RX_DONE: {
            // We are definitely open
            usb_cdc_change_port_state(true);

            if (usb_cdc_handle_rx()) {
                // Setup next transfer.
                usb_cdc_schedule_rx();
            }
            break;
        }
        default:
            break;
    }
}

static HAL_USB_State nrf_usb_state_to_hal_usb_state(app_usbd_state_t state) {
    switch (state) {
        case APP_USBD_STATE_Disabled: {
            return HAL_USB_STATE_DISABLED;
        }
        case APP_USBD_STATE_Unattached: {
            return HAL_USB_STATE_DETACHED;
        }
        case APP_USBD_STATE_Powered: {
            return HAL_USB_STATE_POWERED;
        }
        case APP_USBD_STATE_Default: {
            return HAL_USB_STATE_DEFAULT;
        }
        case APP_USBD_STATE_Addressed: {
            return HAL_USB_STATE_ADDRESSED;
        }
        case APP_USBD_STATE_Configured: {
            return HAL_USB_STATE_CONFIGURED;
        }
        default: {
            return HAL_USB_STATE_NONE;
        }
    }
}

static void usbd_user_ev_handler(app_usbd_event_type_t event)
{
    switch (event) {
        case APP_USBD_EVT_DRV_SOF: {
            usb_cdc_schedule_rx();
            usb_cdc_schedule_tx();
            break;
        }
        case APP_USBD_EVT_DRV_SUSPEND: {
            set_usb_state(HAL_USB_STATE_SUSPENDED);
            usb_cdc_change_port_state(false);
            nrf_drv_usbd_ep_abort(CDC_ACM_DATA_EPIN);
            nrf_drv_usbd_ep_abort(CDC_ACM_DATA_EPOUT);
            m_usb_instance.transmitting = false;
            m_usb_instance.rx_state = false;
            // Required so that we can restart rx transfers after waking up
            m_app_cdc_acm.specific.p_data->ctx.rx_transfer[0].p_buf = NULL;
            m_app_cdc_acm.specific.p_data->ctx.rx_transfer[1].p_buf = NULL;
            m_app_cdc_acm.specific.p_data->ctx.bytes_left = 0;
            m_app_cdc_acm.specific.p_data->ctx.bytes_read = 0;
            m_app_cdc_acm.specific.p_data->ctx.last_read = 0;
            m_app_cdc_acm.specific.p_data->ctx.cur_read = 0;
            m_app_cdc_acm.specific.p_data->ctx.p_copy_pos = m_app_cdc_acm.specific.p_data->ctx.internal_rx_buf;
            // IMPORTANT! Otherwise the state machine will fail
            // to come out of the suspended state
            app_usbd_suspend_req();
            break;
        }
        case APP_USBD_EVT_DRV_RESUME: {
            set_usb_state(nrf_usb_state_to_hal_usb_state(app_usbd_core_state_get()));
            if (m_app_cdc_acm.specific.p_data->ctx.line_state & APP_USBD_CDC_ACM_LINE_STATE_DTR) {
                usb_cdc_change_port_state(true);
            }
            break;
        }
        case APP_USBD_EVT_START_REQ: {
            set_usb_state(HAL_USB_STATE_DETACHED);
            break;
        }
        case APP_USBD_EVT_STARTED: {
            // triggered by app_usbd_start()

            reset_rx_tx_state();
            set_usb_state(nrf_usb_state_to_hal_usb_state(app_usbd_core_state_get()));
            break;
        }
        case APP_USBD_EVT_STOPPED: {
            // triggered by app_usbd_stop()
            app_usbd_disable();
            if (m_usb_instance.enabled) {
                set_usb_state(HAL_USB_STATE_DETACHED);
            } else {
                set_usb_state(HAL_USB_STATE_DISABLED);
            }
            break;
        }
        case APP_USBD_EVT_POWER_DETECTED: {
            if (!nrf_drv_usbd_is_enabled()) {
                app_usbd_enable();
            }
            break;
        }
        case APP_USBD_EVT_POWER_REMOVED: {
            // We need to check for nrfx_usbd_is_enabled() in order not to accidentally
            // trigger an assertion
            if (nrfx_usbd_is_enabled()) {
                app_usbd_stop();
            } else if (m_usb_instance.enabled) {
                // Just in case go into detached state
                set_usb_state(HAL_USB_STATE_DETACHED);
            }
            m_usb_instance.rx_state = false;
            usb_cdc_change_port_state(false);
            break;
        }
        case APP_USBD_EVT_DRV_RESET: {
            // Nordic CDC driver willl silently abort transfers
            m_usb_instance.rx_state = false;
            usb_cdc_change_port_state(false);
            break;
        }
        case APP_USBD_EVT_POWER_READY: {
            if (m_usb_instance.enabled) {
                app_usbd_start();
            }
            break;
        }
        case APP_USBD_EVT_STATE_CHANGED: {
            set_usb_state(nrf_usb_state_to_hal_usb_state(app_usbd_core_state_get()));
            break;
        }
        default:
            break;
    }
}

static bool usb_will_preempt(void)
{
    // Ain't no one is preempting us if interrupts are currently disabled or basepri masked
    if (hal_interrupt_is_irq_masked(USBD_IRQn) || nrf_nvic_state.__cr_flag) {
        return false;
    }

    if (hal_interrupt_is_isr()) {
        if (!hal_interrupt_will_preempt(USBD_IRQn, hal_interrupt_serviced_irqn())) {
            return false;
        }
    }

    return true;
}

int usb_hal_init(void) {
    if (m_usb_instance.initialized) {
        return 0;
    }

    ret_code_t ret;
    static const app_usbd_config_t usbd_config = {
        .ev_state_proc = usbd_user_ev_handler
    };

    ret = nrf_drv_clock_init();
    if (ret != NRF_ERROR_MODULE_ALREADY_INITIALIZED) {
        SPARK_ASSERT(ret == NRF_SUCCESS);
    }

    nrf_drv_clock_lfclk_request(NULL);
    while(!nrf_drv_clock_lfclk_is_running()) {
        /* Just waiting */
    }

    // Create USB Serial string by Device ID
    uint8_t device_id[HAL_DEVICE_ID_SIZE] = {};
    uint8_t device_id_len = hal_get_device_id(device_id, sizeof(device_id));
    memset(g_extern_serial_number, 0, sizeof(g_extern_serial_number));
    bytes2hexbuf_lower_case(device_id, MIN(device_id_len, sizeof(g_extern_serial_number) - 1), g_extern_serial_number);

    ret = app_usbd_init(&usbd_config);
    SPARK_ASSERT(ret == NRF_SUCCESS);

    m_usb_instance.initialized = true;

    return 0;
}

int usb_uart_init(uint8_t *rx_buf, uint16_t rx_buf_size, uint8_t *tx_buf, uint16_t tx_buf_size) {
    uint32_t ret;

    if (m_usb_instance.mode == USB_MODE_CDC_UART) {
        return 0;
    }

    if (app_fifo_init(&m_usb_instance.rx_fifo, rx_buf, rx_buf_size)) {
        return -1;
    }

    if (app_fifo_init(&m_usb_instance.tx_fifo, tx_buf, tx_buf_size)) {
        return -2;
    }

    app_usbd_class_inst_t const * class_cdc_acm = app_usbd_cdc_acm_class_inst_get(&m_app_cdc_acm);
    ret = app_usbd_class_append(class_cdc_acm);
    SPARK_ASSERT(ret == NRF_SUCCESS);

    // We'd like to receive SOF events
    app_usbd_class_sof_register(class_cdc_acm);

    // FIXME: this should not be handled in here, but for now in order to ensure that
    // the control interface is always at interface #2, we do it here
    extern int hal_usb_control_interface_init(void* reserved);
#if HAL_PLATFORM_USB_CONTROL_INTERFACE
    hal_usb_control_interface_init(NULL);
#endif // HAL_PLATFORM_USB_CONTROL_INTERFACE

    if (USBD_POWER_DETECTION) {
        ret = app_usbd_power_events_enable();
        SPARK_ASSERT(ret == NRF_SUCCESS);
    } else {
        app_usbd_enable();
        app_usbd_start();
    }

    m_usb_instance.mode = USB_MODE_CDC_UART;

    return 0;
}

int usb_uart_send(uint8_t data[], uint16_t size) {
    if (m_usb_instance.state != HAL_USB_STATE_CONFIGURED || !m_usb_instance.com_opened) {
        return -1;
    }

    for (int i = 0; i < size; i++) {
        // wait until tx fifo is available
        while (IS_FIFO_FULL(&m_usb_instance.tx_fifo)) {
            if (!usb_hal_is_connected() || !usb_will_preempt()) {
                return -1;
            }
        }
        SPARK_ASSERT(app_fifo_put(&m_usb_instance.tx_fifo, data[i]) == NRF_SUCCESS);
    }

    // NOTE: we only care and report about how many bytes were actually put into the transmit buffer
    return size;
}

void usb_uart_set_baudrate(uint32_t baudrate) {
    app_usbd_class_inst_t const * cdc_inst_class = app_usbd_cdc_acm_class_inst_get(&m_app_cdc_acm);
    app_usbd_cdc_acm_t const *cdc_acm_class= app_usbd_cdc_acm_class_get(cdc_inst_class);

    if ((cdc_acm_class != NULL) && (cdc_acm_class->specific.p_data != NULL)) {
        *(uint32_t *)cdc_acm_class->specific.p_data->ctx.line_coding.dwDTERate = baudrate;
    }
}

uint32_t usb_uart_get_baudrate(void) {
    app_usbd_class_inst_t const * cdc_inst_class = app_usbd_cdc_acm_class_inst_get(&m_app_cdc_acm);
    app_usbd_cdc_acm_t const *cdc_acm_class= app_usbd_cdc_acm_class_get(cdc_inst_class);

    if ((cdc_acm_class != NULL) && (cdc_acm_class->specific.p_data != NULL)) {
        return uint32_decode(cdc_acm_class->specific.p_data->ctx.line_coding.dwDTERate);
    }

    return 0;
}

void usb_hal_attach(void) {
    if (m_usb_instance.enabled) {
        return;
    }

    m_usb_instance.enabled = true;

    set_usb_state(HAL_USB_STATE_DETACHED);

    reset_rx_tx_state();
    usb_cdc_change_port_state(false);

    if (!nrf_drv_usbd_is_enabled()) {
        app_usbd_enable();
    }
    app_usbd_start();
}

void usb_hal_detach(void) {
    if (!m_usb_instance.enabled) {
        return;
    }

    m_usb_instance.enabled = false;

    // We need to check for nrfx_usbd_is_enabled() in order not to accidentally
    // trigger an assertion
    if (nrfx_usbd_is_enabled()) {
        app_usbd_stop();
    }
    if (nrf_drv_usbd_is_enabled()) {
        app_usbd_disable();
    }

    // Go to disabled state just in case
    set_usb_state(HAL_USB_STATE_DISABLED);

    reset_rx_tx_state();
    usb_cdc_change_port_state(false);
}

int usb_uart_available_rx_data(void) {
    return FIFO_LENGTH(&m_usb_instance.rx_fifo) + m_usb_instance.rx_data_size;
}

uint8_t usb_uart_get_rx_data(void) {
    if (usb_cdc_copy_from_rx_buffer()) {
        m_usb_instance.rx_data_size = 0;
        m_usb_instance.rx_done = false;
    }

    uint8_t data = 0;
    if (FIFO_LENGTH(&m_usb_instance.rx_fifo)) {
        SPARK_ASSERT(app_fifo_get(&m_usb_instance.rx_fifo, &data) == NRF_SUCCESS);
    }

    return data;
}

uint8_t usb_uart_peek_rx_data(uint8_t index) {
    uint8_t data = 0;
    if (app_fifo_peek(&m_usb_instance.rx_fifo, index, &data)) {
        return 0;
    }
    return data;
}

void usb_uart_flush_rx_data(void) {
    app_fifo_flush(&m_usb_instance.rx_fifo);
}

void usb_uart_flush_tx_data(void) {
    if (!usb_will_preempt()) {
        return;
    }
    while(usb_hal_is_connected() && FIFO_LENGTH(&m_usb_instance.tx_fifo) != 0 && m_usb_instance.transmitting) {
        // Wait
    }
}

int usb_uart_available_tx_data(void) {
    if (usb_hal_is_connected()) {
        return m_usb_instance.tx_fifo.buf_size_mask - FIFO_LENGTH(&m_usb_instance.tx_fifo) + 1;
    }
    return -1;
}

bool usb_hal_is_enabled(void) {
    return m_usb_instance.enabled;
}

bool usb_hal_is_connected(void) {
    return m_usb_instance.com_opened;
}

void usb_hal_set_bit_rate_changed_handler(void (*handler)(uint32_t bitRate)) {
    m_usb_instance.bit_rate_changed_handler = handler;
}

HAL_USB_State usb_hal_get_state() {
    return m_usb_instance.state;
}

static void dummy_state_change_handler(HAL_USB_State state, void* context) {
    (void)state;
    (void)context;
}

int usb_hal_set_state_change_callback(HAL_USB_State_Callback cb, void* context, void* reserved) {
    for (int i = 0; i < MAX_USB_STATE_CB_NUM; i++) {
        if (m_usb_instance.state_callback[i] == NULL) {
            // FIXME: a weird way to resolve a race condition between cb and context setting
            m_usb_instance.state_callback[i] = dummy_state_change_handler;
            m_usb_instance.state_callback_context[i] = context;
            m_usb_instance.state_callback[i] = cb;
            return 0;
        }
    }
    return SYSTEM_ERROR_NO_MEMORY;
}
