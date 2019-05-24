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

LOG_SOURCE_CATEGORY("hal.usb.control");

#include "usb_hal.h"
#include "app_usbd.h"
#include "app_usbd_string_desc.h"
#include "app_usbd_core.h"

#include "usbd_wcid.h"

namespace {

/* Extended Compat ID OS Descriptor */
static const uint8_t MSFT_EXTENDED_COMPAT_ID_DESCRIPTOR[] = {
    USB_WCID_EXT_COMPAT_ID_OS_DESCRIPTOR(
        0x02,
        USB_WCID_DATA('W', 'I', 'N', 'U', 'S', 'B', '\0', '\0'),
        USB_WCID_DATA(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00)
    )
};

/* Extended Properties OS Descriptor */
static const uint8_t MSFT_EXTENDED_PROPERTIES_OS_DESCRIPTOR[] = {
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

HAL_USB_Vendor_Request_Callback s_usb_vendor_request_callback = nullptr;
void* s_usb_vendor_request_callback_ctx = nullptr;
HAL_USB_Vendor_Request_State_Callback s_usb_vendor_request_state_callback = nullptr;
void* s_usb_vendor_request_state_callback_ctx = nullptr;

static ret_code_t usbd_control_event_handler(app_usbd_class_inst_t const* inst,
        app_usbd_complex_evt_t const* event);

static bool usbd_control_feed_descriptors(app_usbd_class_descriptor_ctx_t* ctx,
        app_usbd_class_inst_t const* inst, uint8_t* buf, size_t max_size);

APP_USBD_CLASS_FORWARD(usbd_control);

typedef struct {
    uint8_t none;
} usbd_control_inst_t;

typedef struct {
    uint8_t none;
} usbd_control_ctx_t;

#define USBD_CONTROL_CONFIG(iface) (iface)
#define USBD_CONTROL_INSTANCE_SPECIFIC_DEC usbd_control_inst_t inst;
#define USBD_CONTROL_DATA_SPECIFIC_DEC usbd_control_ctx_t ctx;

APP_USBD_CLASS_NO_EP_TYPEDEF(
    usbd_control,
    USBD_CONTROL_CONFIG(2),
    USBD_CONTROL_INSTANCE_SPECIFIC_DEC,
    USBD_CONTROL_DATA_SPECIFIC_DEC
);

const app_usbd_class_methods_t usbd_control_class_methods = {
    .event_handler    = usbd_control_event_handler,
    .feed_descriptors = usbd_control_feed_descriptors,
};

APP_USBD_CLASS_INST_NO_EP_GLOBAL_DEF(
    usbd_control_instance,
    usbd_control,
    &usbd_control_class_methods,
    USBD_CONTROL_CONFIG((2)),
    ()
);

inline app_usbd_class_inst_t const* usbd_control_class_inst_get(usbd_control_t const* control) {
    return &control->base;
}

inline usbd_control_t const* usbd_control_class_get(app_usbd_class_inst_t const* inst) {
    return (usbd_control_t const*)inst;
}

ret_code_t usbd_control_setup_handler(app_usbd_class_inst_t const* inst,
        app_usbd_setup_evt_t const* event);

ret_code_t usbd_control_in_data_handler(nrf_drv_usbd_ep_status_t status, void* ctx);
ret_code_t usbd_control_out_data_handler(nrf_drv_usbd_ep_status_t status, void* ctx);
ret_code_t usbd_control_reset();

ret_code_t usbd_control_event_handler(app_usbd_class_inst_t const* inst,
        app_usbd_complex_evt_t const* event) {

    ret_code_t ret = NRF_SUCCESS;
    switch (event->app_evt.type) {
        case APP_USBD_EVT_DRV_RESET:
        case APP_USBD_EVT_STARTED:
        case APP_USBD_EVT_STOPPED:
        case APP_USBD_EVT_DRV_SUSPEND:
        case APP_USBD_EVT_DRV_RESUME: {
            ret = usbd_control_reset();
            break;
        }
        case APP_USBD_EVT_DRV_SETUP: {
            ret = usbd_control_setup_handler(inst, (app_usbd_setup_evt_t const*)event);
            break;
        }
        case APP_USBD_EVT_DRV_SOF: {
            break;
        }
        case APP_USBD_EVT_DRV_EPTRANSFER: {
            break;
        }
        case APP_USBD_EVT_INST_APPEND: {
            break;
        }
        case APP_USBD_EVT_INST_REMOVE: {
            break;
        }
        default: {
            ret = NRF_ERROR_NOT_SUPPORTED;
            break;
        }
    }

    return ret;
}

bool usbd_control_feed_descriptors(app_usbd_class_descriptor_ctx_t* ctx,
        app_usbd_class_inst_t const* inst, uint8_t* buf, size_t max_size) {

    static app_usbd_class_iface_conf_t const* curIface = nullptr;
    curIface = app_usbd_class_iface_get(inst, 0);

    APP_USBD_CLASS_DESCRIPTOR_BEGIN(ctx, buf, max_size)

    /* Particle vendor-specific Control Interface #2 descriptor */
    APP_USBD_CLASS_DESCRIPTOR_WRITE(sizeof(app_usbd_descriptor_iface_t)); // bLength
    APP_USBD_CLASS_DESCRIPTOR_WRITE(APP_USBD_DESCRIPTOR_INTERFACE); // bDescriptorType
    APP_USBD_CLASS_DESCRIPTOR_WRITE(app_usbd_class_iface_number_get(curIface)); // bInterfaceNumber
    APP_USBD_CLASS_DESCRIPTOR_WRITE(0x00); // bAlternateSetting
    APP_USBD_CLASS_DESCRIPTOR_WRITE(0x00); // bNumEndpoints
    APP_USBD_CLASS_DESCRIPTOR_WRITE(0xff); // bInterfaceClass
    APP_USBD_CLASS_DESCRIPTOR_WRITE(0xff); // bInterfaceSubClass
    APP_USBD_CLASS_DESCRIPTOR_WRITE(0xff); // bInterfaceProtocol
    APP_USBD_CLASS_DESCRIPTOR_WRITE(USBD_CONTROL_STRING_IDX); // iInterface

    APP_USBD_CLASS_DESCRIPTOR_END();
}

ret_code_t usbd_control_handle_msft_request(app_usbd_class_inst_t const* inst,
        app_usbd_setup_evt_t const* event) {
    auto req = &event->setup;

    // FIXME: we should get rid of magick numbers
    if (req->wIndex.w == 0x0004) {
        return app_usbd_core_setup_rsp(req, MSFT_EXTENDED_COMPAT_ID_DESCRIPTOR, sizeof(MSFT_EXTENDED_COMPAT_ID_DESCRIPTOR));
    } else if (req->wIndex.w == 0x0005) {
        if ((req->wValue.w & 0xff) == 0x02) {
            return app_usbd_core_setup_rsp(req, MSFT_EXTENDED_PROPERTIES_OS_DESCRIPTOR, sizeof(MSFT_EXTENDED_PROPERTIES_OS_DESCRIPTOR));
        } else {
            // Send dummy
            uint8_t dummy[10] = {0};
            return app_usbd_core_setup_rsp(req, dummy, sizeof(dummy));
        }
    } else {
        return NRF_ERROR_INTERNAL;
    }

    return NRF_SUCCESS;
}

HAL_USB_SetupRequest s_usb_setup_request = {};
uint8_t s_usb_setup_request_data[NRF_DRV_USBD_EPSIZE] = {};

typedef enum {
    USB_IN_SETUP_REQUEST_STATE_NONE = 0,
    USB_IN_SETUP_REQUEST_STATE_TX = 1,
    USB_IN_SETUP_REQUEST_STATE_RX = 2
} usbd_control_in_setup_request_state_t;

static volatile uint32_t s_usb_vendor_in_setup_counter = 0;
static volatile usbd_control_in_setup_request_state_t s_usb_vendor_in_setup_request = USB_IN_SETUP_REQUEST_STATE_NONE;

ret_code_t usbd_control_setup_handler(app_usbd_class_inst_t const* inst,
        app_usbd_setup_evt_t const* event) {

    ret_code_t ret = NRF_SUCCESS;

    auto req = &event->setup;

    // Handle Microsoft vendor requests
    if ((req->bRequest == 0xee && req->bmRequestType == 0xc1 && req->wIndex.w == 0x0005) ||
            (req->bRequest == 0xee && req->bmRequestType == 0xc0 && req->wIndex.w == 0x0004)) {
        return usbd_control_handle_msft_request(inst, event);
    }

    if (s_usb_vendor_request_callback == nullptr) {
        return NRF_ERROR_INVALID_STATE;
    }

    s_usb_setup_request.bmRequestType = req->bmRequestType;
    s_usb_setup_request.bRequest = req->bRequest;
    s_usb_setup_request.wValue = req->wValue.w;
    s_usb_setup_request.wIndex = req->wIndex.w;
    s_usb_setup_request.wLength = req->wLength.w;

    if (req->wLength.w) {
        // Setup request with data stage
        if (req->bmRequestType & 0x80) {
            // Device -> Host (IN)
            s_usb_vendor_in_setup_request = USB_IN_SETUP_REQUEST_STATE_TX;
            if (req->wLength.w <= NRF_DRV_USBD_EPSIZE) {
                s_usb_setup_request.data = s_usb_setup_request_data;
            } else {
                s_usb_setup_request.data = nullptr;
            }
            ret = s_usb_vendor_request_callback(&s_usb_setup_request, s_usb_vendor_request_callback_ctx);

            if (ret == 0 && ((s_usb_setup_request.data != nullptr && s_usb_setup_request.wLength) || !s_usb_setup_request.wLength)) {
                if (s_usb_setup_request.data != s_usb_setup_request_data &&
                    s_usb_setup_request.wLength <= NRF_DRV_USBD_EPSIZE && s_usb_setup_request.wLength) {
                    // Don't use user buffer if wLength <= NRF_DRV_USBD_EPSIZE
                    // and copy into internal buffer
                    memcpy(s_usb_setup_request_data, s_usb_setup_request.data, s_usb_setup_request.wLength);
                    s_usb_setup_request.data = s_usb_setup_request_data;
                }
                CRITICAL_REGION_ENTER();
                ret = app_usbd_core_setup_rsp(req, s_usb_setup_request.data, s_usb_setup_request.wLength);
                if (ret == NRF_SUCCESS) {
                    app_usbd_core_setup_data_handler_desc_t desc = {
                        .handler   = usbd_control_in_data_handler,
                        .p_context = nullptr
                    };

                    ret = app_usbd_core_setup_data_handler_set(NRF_DRV_USBD_EPIN0, &desc);
                }
                CRITICAL_REGION_EXIT();
            } else {
                ret = NRF_ERROR_INTERNAL;
            }
        } else {
            // Host -> Device (OUT)
            s_usb_vendor_in_setup_request = USB_IN_SETUP_REQUEST_STATE_RX;
            if (req->wLength.w <= NRF_DRV_USBD_EPSIZE) {
                // Use internal buffer
                s_usb_setup_request.data = s_usb_setup_request_data;
            } else {
                // Try to request the buffer
                s_usb_setup_request.data = nullptr;
                s_usb_vendor_request_callback(&s_usb_setup_request, s_usb_vendor_request_callback_ctx);
                if (s_usb_setup_request.data != nullptr && s_usb_setup_request.wLength >= req->wLength.w) {
                    s_usb_setup_request.wLength = req->wLength.w;
                } else {
                    ret = NRF_ERROR_INTERNAL;
                }
            }

            if (ret == NRF_SUCCESS) {
                /* Request setup data */
                NRF_DRV_USBD_TRANSFER_OUT(transfer, s_usb_setup_request.data, s_usb_setup_request.wLength);
                CRITICAL_REGION_ENTER();
                ret = app_usbd_ep_transfer(NRF_DRV_USBD_EPOUT0, &transfer);
                if (ret == NRF_SUCCESS) {
                    app_usbd_core_setup_data_handler_desc_t desc = {
                        .handler   = usbd_control_out_data_handler,
                        .p_context = nullptr
                    };

                    ret = app_usbd_core_setup_data_handler_set(NRF_DRV_USBD_EPOUT0, &desc);
                }
                CRITICAL_REGION_EXIT();
            }
        }
    } else {
        // Setup request without data stage
        s_usb_setup_request.data = nullptr;
        ret = s_usb_vendor_request_callback(&s_usb_setup_request, s_usb_vendor_request_callback_ctx);
    }

    return ret;
}

ret_code_t usbd_control_in_data_handler(nrf_drv_usbd_ep_status_t status, void* ctx) {
    // Data stage completed
    if (s_usb_vendor_in_setup_request == USB_IN_SETUP_REQUEST_STATE_TX && s_usb_vendor_request_state_callback) {
        s_usb_vendor_request_state_callback(HAL_USB_VENDOR_REQUEST_STATE_TX_COMPLETED, s_usb_vendor_request_state_callback_ctx);
    }
    s_usb_vendor_in_setup_request = USB_IN_SETUP_REQUEST_STATE_NONE;
    s_usb_vendor_in_setup_counter = 0;

    return NRF_SUCCESS;
}

ret_code_t usbd_control_out_data_handler(nrf_drv_usbd_ep_status_t status, void* ctx) {
    // Data stage completed
    if (s_usb_vendor_in_setup_request == USB_IN_SETUP_REQUEST_STATE_RX) {
        s_usb_vendor_request_callback(&s_usb_setup_request, s_usb_vendor_request_callback_ctx);
    }
    s_usb_vendor_in_setup_request = USB_IN_SETUP_REQUEST_STATE_NONE;
    s_usb_vendor_in_setup_counter = 0;

    return NRF_SUCCESS;
}

ret_code_t usbd_control_reset() {
    if (s_usb_vendor_request_state_callback) {
        s_usb_vendor_request_state_callback(HAL_USB_VENDOR_REQUEST_STATE_RESET, s_usb_vendor_request_state_callback_ctx);
    }
    s_usb_vendor_in_setup_request = USB_IN_SETUP_REQUEST_STATE_NONE;
    s_usb_vendor_in_setup_counter = 0;
    return NRF_SUCCESS;
}

} // anonymous

extern "C" int hal_usb_control_interface_init(void* reserved) {
    return !(app_usbd_class_append(usbd_control_class_inst_get(&usbd_control_instance)) == NRF_SUCCESS);
}

void HAL_USB_Set_Vendor_Request_Callback(HAL_USB_Vendor_Request_Callback cb, void* p) {
    s_usb_vendor_request_callback = cb;
    s_usb_vendor_request_callback_ctx = p;
}

void HAL_USB_Set_Vendor_Request_State_Callback(HAL_USB_Vendor_Request_State_Callback cb, void* p) {
    s_usb_vendor_request_state_callback = cb;
    s_usb_vendor_request_state_callback_ctx = p;
}
