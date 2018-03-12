/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 * Declares platform USB Host/Device functions
 */
#pragma once

#include "wiced_result.h"

#include "platform_peripheral.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/
/* USB20 Host HCI (host controller interface) type */
typedef enum
{
    USB_HOST_CONTROLLER_INTERFACE_OHCI,         /* Generic 1.1 USB host controller */
    USB_HOST_CONTROLLER_INTERFACE_EHCI,         /* Generic 2.0 USB host controller */
    USB_HOST_CONTROLLER_INTERFACE_ISP1161,      /* NXP 1.1 embedded discrete host controller */
    USB_HOST_CONTROLLER_INTERFACE_ISP1362,      /* NXP 2.0 embedded discrete host controller */
    USB_HOST_CONTROLLER_INTERFACE_PIC32,        /* Microchip embedded 1.1 host controller */
    USB_HOST_CONTROLLER_INTERFACE_RX,           /* Renesas RX62 embedded host controller */
    USB_HOST_CONTROLLER_INTERFACE_RZ,           /* Renesas RZ embedded host controller */
    USB_HOST_CONTROLLER_INTERFACE_SH2A,         /* Renesas SH2A based embedded host controller */
    USB_HOST_CONTROLLER_INTERFACE_STM32,        /* ST Micro STM32 F1/F2/F4 embedded host controller */
    USB_HOST_CONTROLLER_INTERFACE_MUSB,         /* Mentor generic host controller */
    USB_HOST_CONTROLLER_INTERFACE_MAX
} platform_usb_host_controller_interface_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/
/* USB20 Host interrupt handler */
typedef void (*platform_usb_host_irq_handler_t)( void );

/******************************************************
 *                    Structures
 ******************************************************/
/* USB20 Host HCI (host controller interface) resource */
typedef struct
{
    platform_usb_host_controller_interface_t    usb_host_hci_type;
    uint32_t                                    usb_host_hci_ioaddress;
    uint32_t                                    usb_host_hci_irq_number;
} platform_usb_host_hci_resource_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/
/* USB20 Host init/deinit */
platform_result_t   platform_usb_host_init( void );
platform_result_t   platform_usb_host_post_init( void );
void                platform_usb_host_deinit( void );

/* USB20 Host init IRQ */
platform_result_t   platform_usb_host_init_irq( platform_usb_host_irq_handler_t irq_handler );

platform_result_t   platform_usb_host_enable_irq( void );
platform_result_t   platform_usb_host_disable_irq( void );

/* USB20 Host get HCI (host controller interface) resource */
platform_result_t   platform_usb_host_get_hci_resource( platform_usb_host_hci_resource_t *resource_list_buf, uint32_t buf_size, uint32_t *resource_total_num );


#ifdef __cplusplus
} /*extern "C" */
#endif
