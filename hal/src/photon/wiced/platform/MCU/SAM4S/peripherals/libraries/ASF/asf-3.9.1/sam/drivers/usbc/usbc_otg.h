/**
 * \file
 *
 * \brief USBC OTG Driver header file.
 *
 * Copyright (c) 2012-2013 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */

#ifndef _USBC_OTG_H_
#define _USBC_OTG_H_

#include "compiler.h"
#include "preprocessor.h"

/* Get USB pads pins configuration in board configuration */
#include "conf_board.h"
#include "board.h"
#include "ioport.h"
#include "gpio.h"
#include "eic.h"

// To simplify the macros definition of this file
#define USBC_CLR_BITS(reg, bit) \
		(Clr_bits(USBC->TPASTE2(USBC_,reg), TPASTE4(USBC_,reg,_,bit)))
#define USBC_SET_BITS(reg, bit) \
		(Set_bits(USBC->TPASTE2(USBC_,reg), TPASTE4(USBC_,reg,_,bit)))
#define USBC_RD_BITS(reg, bit) \
		(Rd_bits(USBC->TPASTE2(USBC_,reg), TPASTE4(USBC_,reg,_,bit)))
#define USBC_TST_BITS(reg, bit) \
		(Tst_bits(USBC->TPASTE2(USBC_,reg), TPASTE4(USBC_,reg,_,bit)))
#define USBC_RD_BITFIELD(reg, bit) \
		(Rd_bitfield(USBC->TPASTE2(USBC_,reg),\
		TPASTE5(USBC_,reg,_,bit,_Msk)))
#define USBC_WR_BITFIELD(reg, bit, value) \
		(Wr_bitfield(USBC->TPASTE2(USBC_,reg),\
		TPASTE5(USBC_,reg,_,bit,_Msk),value))
#define USBC_REG_CLR(reg, bit) \
		(USBC->TPASTE3(USBC_,reg,CLR) = TPASTE5(USBC_,reg,CLR_,bit,C))
#define USBC_REG_SET(reg, bit) \
		(USBC->TPASTE3(USBC_,reg,SET) = TPASTE5(USBC_,reg,SET_,bit,S))

/**
 * \name USB IO PADs management
 */
//@{
__always_inline static void eic_line_change_config(uint8_t line, bool b_high)
{
	struct eic_line_config eic_line_conf;
	eic_line_conf.eic_mode = EIC_MODE_LEVEL_TRIGGERED;
	eic_line_conf.eic_level = b_high ?
			EIC_LEVEL_HIGH_LEVEL : EIC_LEVEL_LOW_LEVEL;
	eic_line_conf.eic_filter = EIC_FILTER_DISABLED;
	eic_line_conf.eic_async = EIC_ASYNCH_MODE;
	eic_line_set_config(EIC, line, &eic_line_conf);
}

__always_inline static void eic_pad_init(uint8_t line, eic_callback_t callback,
		uint8_t irq_line, ioport_pin_t pin, uint8_t irq_level)
{
	eic_line_disable_interrupt(EIC, line);
	eic_line_disable(EIC, line);
	eic_line_clear_interrupt(EIC, line);
	eic_line_set_callback(EIC, line, callback, irq_line, irq_level);
	eic_line_change_config(line, !ioport_get_pin_level(pin));
	eic_line_enable(EIC, line);
	eic_line_enable_interrupt(EIC, line);
}
__always_inline static void io_pad_init(ioport_pin_t pin, ioport_mode_t mode,
		gpio_pin_callback_t callback, uint8_t irq_level)
{
	gpio_disable_pin_interrupt(pin);
	ioport_set_pin_mode(pin, mode);
	ioport_set_pin_sense_mode(pin, IOPORT_SENSE_BOTHEDGES);
	gpio_set_pin_callback(pin, callback, irq_level);
	gpio_enable_pin_interrupt(pin);
}
//@}

//! \ingroup usb_group
//! \defgroup otg_usbc_group USBC OTG Driver
//! USBC low-level driver for OTG features
//!
//! @{

/**
 * \brief Initialize the dual role
 * This function is implemented in usbc_host.c file.
 *
 * \return \c true if the ID pin management has been started, otherwise \c false.
 */
bool otg_dual_enable(void);

/**
 * \brief Uninitialize the dual role
 * This function is implemented in usbc_host.c file.
 */
void otg_dual_disable(void);

//! @name USBC IP properties
//! These macros give access to IP properties
//! @{

//! Get IP name part 1 or 2
#define  otg_get_ip_name() \
		(((uint64_t)USBC->USBC_UNAME1<<32)|(uint64_t)USBC->USBC_UNAME2)
#define  otg_data_memory_barrier()      do { barrier(); } while (0)
//! Get IP version
#define  otg_get_ip_version()           USBC_RD_BITFIELD(UVERS,VERSION)
//! Get maximal number of pipes/endpoints
#define  otg_get_max_nbr_endpoints()    USBC_RD_BITFIELD(UFEATURES, EPTNBRMAX)
#define  otg_get_max_nbr_pipes()        USBC_RD_BITFIELD(UFEATURES, EPTNBRMAX)

//! @}

//! @name USBC OTG ID pin management
//! The ID pin come from the USB OTG connector (A and B receptable) and
//! allows to select the USB mode host or device.
//! The ID pin can be managed through GPIO or EIC pin.
//! This feature is optional, and it is enabled if USB_ID_PIN or USB_ID_EIC
//! is defined in board.h and CONF_BOARD_USB_ID_DETECT defined in
//! conf_board.h.
//!
//! @{
#define OTG_ID_DETECT       (defined(CONF_BOARD_USB_ID_DETECT))
#define OTG_ID_IO           (defined(USB_ID_PIN) && OTG_ID_DETECT)
#define OTG_ID_EIC          (defined(USB_ID_EIC) && OTG_ID_DETECT)

#if OTG_ID_EIC
# define pad_id_init() \
	eic_pad_init(USB_ID_EIC_LINE, otg_id_handler, USB_ID_EIC_IRQn, USB_ID_EIC, UHD_USB_INT_LEVEL);
# define pad_id_interrupt_disable() eic_line_disable_interrupt(EIC, USB_ID_EIC_LINE)
# define pad_ack_id_interrupt() \
	(eic_line_change_config(USB_ID_EIC_LINE, !ioport_get_pin_level(USB_ID_EIC)), \
	eic_line_clear_interrupt(EIC, USB_ID_EIC_LINE))
# define Is_pad_id_device()         ioport_get_pin_level(USB_ID_EIC)
#elif OTG_ID_IO
# define pad_id_init() \
	io_pad_init(USB_ID_PIN, USB_ID_FLAGS, otg_id_handler, UHD_USB_INT_LEVEL);
# define pad_id_interrupt_disable() gpio_disable_pin_interrupt(USB_ID_PIN)
# define pad_ack_id_interrupt()     gpio_clear_pin_interrupt_flag(USB_ID_PIN)
# define Is_pad_id_device()         ioport_get_pin_level(USB_ID_PIN)
#endif
//! @}

//! @name USBC Vbus management
//!
//! The VBus line can be monitored through a GPIO pin and
//! a basic resitor voltage divider.
//! This feature is optional, and it is enabled if USB_VBUS_PIN or USB_VBUS_EIC
//! is defined in board.h and CONF_BOARD_USB_VBUS_DETECT defined in
//! conf_board.h.
//! @{
#define OTG_VBUS_DETECT     (defined(CONF_BOARD_USB_VBUS_DETECT))
#define OTG_VBUS_IO         (defined(USB_VBUS_PIN) && OTG_VBUS_DETECT)
#define OTG_VBUS_EIC        (defined(USB_VBUS_EIC) && OTG_VBUS_DETECT)

#if OTG_VBUS_EIC
# define pad_vbus_init(level) \
	eic_pad_init(USB_VBUS_EIC_LINE, uhd_vbus_handler, USB_VBUS_EIC_IRQn, USB_VBUS_EIC, level);
# define pad_vbus_interrupt_disable() eic_line_disable_interrupt(EIC, USB_VBUS_EIC_LINE)
# define pad_ack_vbus_interrupt() \
	(eic_line_change_config(USB_VBUS_EIC_LINE, !ioport_get_pin_level(USB_VBUS_EIC)), \
	eic_line_clear_interrupt(EIC, USB_VBUS_EIC_LINE))
# define Is_pad_vbus_high()           ioport_get_pin_level(USB_VBUS_EIC)
#elif OTG_VBUS_IO
# define pad_vbus_init(level) \
	io_pad_init(USB_VBUS_PIN, USB_VBUS_FLAGS, uhd_vbus_handler, level);
# define pad_vbus_interrupt_disable() gpio_disable_pin_interrupt(USB_VBUS_PIN)
# define pad_ack_vbus_interrupt()     gpio_clear_pin_interrupt_flag(USB_VBUS_PIN)
# define Is_pad_vbus_high()           ioport_get_pin_level(USB_VBUS_PIN)
#endif

//! Notify USBC that the VBUS on the usb line is powered
#define uhd_vbus_is_on()          USBC_REG_SET(USBSTA,VBUSRQ)
//! Notify USBC that the VBUS on the usb line is not powered
#define uhd_vbus_is_off()         USBC_REG_CLR(USBSTA,VBUSRQ)

//! @}

//! @name USBC OTG main management
//! These macros allows to enable/disable pad and USBC hardware
//! @{
#define  otg_enable()                        USBC_SET_BITS(USBCON,USBE)
#define  otg_disable()                       USBC_CLR_BITS(USBCON,USBE)

//! @name USBC mode management
//! The USBC mode device or host must be selected manualy by user
//! @{
#define  otg_enable_device_mode()             USBC_SET_BITS(USBCON,UIMOD)
#define  Is_otg_device_mode_enabled()         USBC_TST_BITS(USBCON,UIMOD)
#define  otg_enable_host_mode()               USBC_CLR_BITS(USBCON,UIMOD)
#define  Is_otg_host_mode_enabled()           (!Is_otg_device_mode_enabled())
//! @}

//! Get the dual-role device state of the internal USB finite state machine
#define  otg_get_fsm_drd_state()             USBC_RD_BITFIELD(USBFSM,DRDSTATE)

#define otg_register_desc_tab(addr) \
		(Wr_bitfield(USBC->USBC_UDESC, USBC_UDESC_UDESCA_Msk, addr))

//! Check transceiver state
#define Is_otg_suspend()                     USBC_TST_BITS(USBSTA,SUSPEND)
#define Is_otg_transceiver_on()              (!Is_otg_suspend())
#define Is_otg_transceiver_off()             (Is_otg_suspend())

//! Check USB interface clock usable
#define  Is_otg_clock_usable()               USBC_TST_BITS(USBSTA,CLKUSABLE)

#define  otg_freeze_clock()                  USBC_SET_BITS(USBCON,FRZCLK)
#define  otg_unfreeze_clock()                USBC_CLR_BITS(USBCON,FRZCLK)
#define  Is_otg_clock_frozen()               USBC_TST_BITS(USBCON,FRZCLK)
//! @}

//! @name USBC Power Manager wake-up feature
//! @{

/*! \brief Enable one or several asynchronous wake-up source.
 *
 * \param awen_mask Mask of asynchronous wake-up sources (use one of the defines
 *  PM_AWEN_xxxx in the part-specific header file)
 */
__always_inline static void usbc_async_wake_up_enable(void)
{
	PM->PM_AWEN |= (1U << PM_AWEN_USBC);
}

/*! \brief Disable one or several asynchronous wake-up source.
 *
 * \param awen_mask Mask of asynchronous wake-up sources (use one of the defines
 *  PM_AWEN_xxxx in the part-specific header file)
 */
__always_inline static void usbc_async_wake_up_disable(void)
{
	PM->PM_AWEN &= ~(1U << PM_AWEN_USBC);
}
//! @}

//! @}

#endif // _USBC_OTG_H_
