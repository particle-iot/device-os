/**
 * \file
 *
 * \brief USBC Device Driver header file.
 *
 * Copyright (c) 2012 - 2013 Atmel Corporation. All rights reserved.
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

#ifndef _USBC_DEVICE_H_
#define _USBC_DEVICE_H_

#include "compiler.h"
#include "preprocessor.h"
#include "usbc_otg.h"

//! \ingroup udd_group
//! \defgroup udd_usbc_group USBC Device Driver
//! USBC low-level driver for USB Device mode
//!
//! @{

//! @name USBC Device IP properties
//! These macros give access to IP properties
//! @{
  //! Get maximal number of endpoints
#define  udd_get_endpoint_max_nbr() otg_get_max_nbr_endpoints()
#define  UDD_MAX_PEP_NB             (USBC_EPT_NBR-1)
#define  UDD_PEP_NB                 (USBC_EPT_NBR)
//! @}

//! @name USBC Device speeds management
//! @{
  //! Enable/disable device low-speed mode
#define  udd_low_speed_enable()              USBC_SET_BITS(UDCON,LS)
#define  udd_low_speed_disable()             USBC_CLR_BITS(UDCON,LS)
  //! Test if device low-speed mode is forced
#define  Is_udd_low_speed_enable()           USBC_TST_BITS(UDCON,LS)

#ifdef USBC_UDCON_SPDCONF
  //! Enable high speed mode
#define  udd_high_speed_enable()            USBC_WR_BITFIELD(UDCON,SPDCONF,0)
  //! Disable high speed mode
#define  udd_high_speed_disable()           USBC_WR_BITFIELD(UDCON,SPDCONF,3)
  //! Test if controller is in full speed mode
#define  Is_udd_full_speed_mode() \
		(USBC_RD_BITFIELD(USBSTA,SPEED) == (USBC_USBSTA_SPEED_FULL >> USBC_USBSTA_SPEED_Pos))
#else
#define  udd_high_speed_enable()             do { } while (0)
#define  udd_high_speed_disable()            do { } while (0)
#define  Is_udd_full_speed_mode()            true
#endif
//! @}

//! @name USBC Device HS test mode management
//! @{
#ifdef USBC_UDCON_SPDCONF
  //! Enable high speed test mode
#define   udd_enable_hs_test_mode()          USBC_WR_BITFIELD(UDCON,SPDCONF,2)
#define   udd_enable_hs_test_mode_j()        USBC_SET_BITS(UDCON,TSTJ)
#define   udd_enable_hs_test_mode_k()        USBC_SET_BITS(UDCON,TSTK)
#define   udd_enable_hs_test_mode_packet()   USBC_SET_BITS(UDCON,TSTPCKT)
#else
#define  udd_enable_hs_test_mode()           do { } while (0)
#define  udd_enable_hs_test_mode_j()         do { } while (0)
#define  udd_enable_hs_test_mode_k()         do { } while (0)
#define  udd_enable_hs_test_mode_packet()    do { } while (0)
#endif
//! @}



//! @name USBC device attach control
//! These macros manage the USBC Device attach.
//! @{
  //! detaches from USB bus
#define  udd_detach_device()                 USBC_SET_BITS(UDCON,DETACH)
  //! attaches to USB bus
#define  udd_attach_device()                 USBC_CLR_BITS(UDCON,DETACH)
  //! test if the device is detached
#define  Is_udd_detached()                   USBC_TST_BITS(UDCON,DETACH)
//! @}


//! @name USBC device bus events control
//! These macros manage the USBC Device bus events.
//! @{

//! Initiates a remote wake-up event
//! @{
#define  udd_initiate_remote_wake_up()       USBC_SET_BITS(UDCON,RMWKUP)
#define  Is_udd_pending_remote_wake_up()     USBC_TST_BITS(UDCON,RMWKUP)
//! @}

//! Manage upstream resume event (=remote wakeup from device)
//! The USB driver sends a resume signal called "Upstream Resume"
//! @{
#define  udd_enable_remote_wake_up_interrupt()     USBC_REG_SET(UDINTE,UPRSME)
#define  udd_disable_remote_wake_up_interrupt()    USBC_REG_CLR(UDINTE,UPRSME)
#define  Is_udd_remote_wake_up_interrupt_enabled() USBC_TST_BITS(UDINTE,UPRSME)
#define  udd_ack_remote_wake_up_start()            USBC_REG_CLR(UDINT,UPRSM)
#define  udd_raise_remote_wake_up_start()          USBC_REG_SET(UDINT,UPRSM)
#define  Is_udd_remote_wake_up_start()             USBC_TST_BITS(UDINT,UPRSM)
//! @}

//! Manage end of resume event (=remote wakeup from host)
//! The USB controller detects a valid "End of Resume" signal initiated by the host
//! @{
#define  udd_enable_resume_interrupt()             USBC_REG_SET(UDINTE,EORSME)
#define  udd_disable_resume_interrupt()            USBC_REG_CLR(UDINTE,EORSME)
#define  Is_udd_resume_interrupt_enabled()         USBC_TST_BITS(UDINTE,EORSME)
#define  udd_ack_resume()                          USBC_REG_CLR(UDINT,EORSM)
#define  udd_raise_resume()                        USBC_REG_SET(UDINT,EORSM)
#define  Is_udd_resume()                           USBC_TST_BITS(UDINT,EORSM)
//! @}

//! Manage wake-up event (=usb line activity)
//! The USB controller is reactivated by a filtered non-idle signal from the lines
//! @{
#define  udd_enable_wake_up_interrupt()            USBC_REG_SET(UDINTE,WAKEUPE)
#define  udd_disable_wake_up_interrupt()           USBC_REG_CLR(UDINTE,WAKEUPE)
#define  Is_udd_wake_up_interrupt_enabled()        USBC_TST_BITS(UDINTE,WAKEUPE)
#define  udd_ack_wake_up()                         USBC_REG_CLR(UDINT,WAKEUP)
#define  udd_raise_wake_up()                       USBC_REG_SET(UDINT,WAKEUP)
#define  Is_udd_wake_up()                          USBC_TST_BITS(UDINT,WAKEUP)
//! @}

//! Manage reset event
//! Set when a USB "End of Reset" has been detected
//! @{
#define  udd_enable_reset_interrupt()              USBC_REG_SET(UDINTE,EORSTE)
#define  udd_disable_reset_interrupt()             USBC_REG_CLR(UDINTE,EORSTE)
#define  Is_udd_reset_interrupt_enabled()          USBC_TST_BITS(UDINTE,EORSTE)
#define  udd_ack_reset()                           USBC_REG_CLR(UDINT,EORST)
#define  udd_raise_reset()                         USBC_REG_SET(UDINT,EORST)
#define  Is_udd_reset()                            USBC_TST_BITS(UDINT,EORST)
//! @}

//! Manage start of frame event
//! @{
#define  udd_enable_sof_interrupt()                USBC_REG_SET(UDINTE,SOFE)
#define  udd_disable_sof_interrupt()               USBC_REG_CLR(UDINTE,SOFE)
#define  Is_udd_sof_interrupt_enabled()            USBC_TST_BITS(UDINTE,SOFE)
#define  udd_ack_sof()                             USBC_REG_CLR(UDINT,SOF)
#define  udd_raise_sof()                           USBC_REG_SET(UDINT,SOF)
#define  Is_udd_sof()                              USBC_TST_BITS(UDINT,SOF)
#define  udd_frame_number()                        USBC_RD_BITFIELD(UDFNUM,FNUM)
#define  Is_udd_frame_number_crc_error()           USBC_TST_BITS(UDFNUM,FNCERR)
//! @}

//! Manage Micro start of frame event (High Speed Only)
//! @{
#ifdef USBC_UDINT_MSOF
#define  udd_enable_msof_interrupt()               USBC_REG_SET(UDINTE,MSOFE)
#define  udd_disable_msof_interrupt()              USBC_REG_CLR(UDINTE,MSOFE)
#define  Is_udd_msof_interrupt_enabled()           USBC_TST_BITS(UDINTE,MSOFE)
#define  udd_ack_msof()                            USBC_REG_CLR(UDINT,MSOF)
#define  udd_raise_msof()                          USBC_REG_SET(UDINT,MSOF)
#define  Is_udd_msof()                             USBC_TST_BITS(UDINT,MSOF)
#else
#define  udd_enable_msof_interrupt()               do { } while(0)
#define  udd_disable_msof_interrupt()              do { } while(0)
#define  Is_udd_msof_interrupt_enabled()           false
#define  udd_ack_msof()                            do { } while(0)
#define  udd_raise_msof()                          do { } while(0)
#define  Is_udd_msof()                             false
#endif
#ifdef USBC_UDFNUM_MFNUM_Msk
#define  udd_micro_frame_number() \
	(Rd_bitfield(USBC->USBC_UDFNUM,\
		(USBC_UDFNUM_FNUM_Msk|USBC_UDFNUM_MFNUM_Msk))
#else
#define  udd_micro_frame_number() \
	(Rd_bits(USBC->USBC_UDFNUM, USBC_UDFNUM_FNUM_Msk))
#endif
//! @}

//! Manage suspend event
//! @{
#define  udd_enable_suspend_interrupt()            USBC_REG_SET(UDINTE,SUSPE)
#define  udd_disable_suspend_interrupt()           USBC_REG_CLR(UDINTE,SUSPE)
#define  Is_udd_suspend_interrupt_enabled()        USBC_TST_BITS(UDINTE,SUSPE)
#define  udd_ack_suspend()                         USBC_REG_CLR(UDINT,SUSP)
#define  udd_raise_suspend()                       USBC_REG_SET(UDINT,SUSP)
#define  Is_udd_suspend()                          USBC_TST_BITS(UDINT,SUSP)
//! @}

//! @}

//! @name USBC device address control
//! These macros manage the USBC Device address.
//! @{
#define  udd_enable_address()                      USBC_SET_BITS(UDCON,ADDEN)
#define  udd_disable_address()                     USBC_CLR_BITS(UDCON,ADDEN)
#define  Is_udd_address_enabled()                  USBC_TST_BITS(UDCON,ADDEN)
#define  udd_configure_address(addr)               USBC_WR_BITFIELD(UDCON,UADD,addr)
#define  udd_get_configured_address()              USBC_RD_BITFIELD(UDCON,UADD)
//! @}


//! @name USBC Device endpoint drivers
//! These macros manage the common features of the endpoints.
//! @{

//! Generic macro for USBC registers that can be arrayed
//! @{
#define USBC_ARRAY(reg,index)   (((volatile uint32_t*)(&USBC->TPASTE2(USBC_,reg)))[index])
#define USBC_EP_CLR_BITS(reg, bit, ep) \
		(Clr_bits(USBC_ARRAY(TPASTE2(reg,0),ep),\
		TPASTE5(USBC_,reg,0_,bit,C)))
#define USBC_EP_SET_BITS(reg, bit, ep) \
		(Set_bits(USBC_ARRAY(TPASTE2(reg,0),ep),\
		TPASTE5(USBC_,reg,0_,bit,S)))
#define USBC_EP_TST_BITS(reg, bit, ep) \
		(Tst_bits(USBC_ARRAY(TPASTE2(reg,0),ep),\
		TPASTE4(USBC_,reg,0_,bit)))
#define USBC_EP_RD_BITS(reg, bit, ep) \
		(Rd_bits(USBC_ARRAY(TPASTE2(reg,0),ep),\
		TPASTE5(USBC_,reg,0_,bit,_Msk)))
#define USBC_EP_WR_BITS(reg, bit, ep, value) \
		(Wr_bits(USBC_ARRAY(TPASTE2(reg,0),ep),\
		TPASTE5(USBC_,reg,0_,bit,_Msk), value))
#define USBC_EP_RD_BITFIELD(reg, bit, ep) \
		(Rd_bitfield(USBC_ARRAY(TPASTE2(reg,0),ep),\
		TPASTE5(USBC_,reg,0_,bit,_Msk)))
#define USBC_EP_WR_BITFIELD(reg, bit, ep, value) \
		(Wr_bitfield(USBC_ARRAY(TPASTE2(reg,0),ep),\
		TPASTE5(USBC_,reg,0_,bit,_Msk), value))
#define USBC_EP_REG_CLR(reg, bit, ep) \
		(USBC_ARRAY(TPASTE2(reg,0CLR),ep) \
		 = TPASTE5(USBC_,reg,0CLR_,bit,C))
#define USBC_EP_REG_SET(reg, bit, ep) \
		(USBC_ARRAY(TPASTE2(reg,0SET),ep) \
		 = TPASTE5(USBC_,reg,0SET_,bit,S))
//! @}

//! @name USBC Device endpoint configuration
//! @{
#define  udd_disable_endpoints() \
		(Clr_bits(USBC->USBC_UERST, (1 << USBC_EPT_NBR) - 1))
#define  udd_enable_endpoint(ep) \
		(Set_bits(USBC->USBC_UERST, USBC_UERST_EPEN0 << (ep)))
#define  udd_disable_endpoint(ep) \
		(Clr_bits(USBC->USBC_UERST, USBC_UERST_EPEN0 << (ep)))
#define  Is_udd_endpoint_enabled(ep) \
		(Tst_bits(USBC->USBC_UERST, USBC_UERST_EPEN0 << (ep)))
#define  udd_reset_endpoint(ep) \
		(Clr_bits(USBC->USBC_UERST, USBC_UERST_EPEN0 << (ep)),\
		Set_bits(USBC->USBC_UERST, USBC_UERST_EPEN0 << (ep)))
#define  Is_udd_resetting_endpoint(ep) \
		(!Is_udd_endpoint_enabled())

  // type: USBC_UECFGn_EPTYPE_XXX defined in chip header
#define  udd_configure_endpoint_type(ep, type)     USBC_EP_WR_BITS(UECFG,EPTYPE,ep,type)
  // return USBC_UECFGn_EPTYPE_XXX
#define  udd_get_endpoint_type(ep)                 USBC_EP_RD_BITS(UECFG,EPTYPE,ep)
#define  Is_udd_endpoint_type_control(ep)          (USBC_EP_RD_BITS(UECFG,EPTYPE,ep) == USBC_UECFG0_EPTYPE_CONTROL)
#define  Is_udd_endpoint_type_bulk(ep)             (USBC_EP_RD_BITS(UECFG,EPTYPE,ep) == USBC_UECFG0_EPTYPE_BULK)
#define  Is_udd_endpoint_type_iso(ep)              (USBC_EP_RD_BITS(UECFG,EPTYPE,ep) == USBC_UECFG0_EPTYPE_ISOCHRONOUS)
#define  Is_udd_endpoint_type_int(ep)              (USBC_EP_RD_BITS(UECFG,EPTYPE,ep) == USBC_UECFG0_EPTYPE_INTERRUPT)
  // dir: USBC_UECFGn_EPDIR_XXX defined in chip header
#define  udd_configure_endpoint_direction(ep, dir) USBC_EP_WR_BITS(UECFG,EPDIR,ep,dir)
  // return USBC_UECFGn_EPDIR_XXX
#define  udd_get_endpoint_direction(ep)            USBC_EP_RD_BITS(UECFG,EPDIR,ep)
#define  Is_udd_endpoint_in(ep)                    USBC_EP_TST_BITS(UECFG,EPDIR,ep)
  //! Bounds given integer size to allowed range and rounds it up to the nearest
  //! available greater size, then applies register format of USBC controller
  //! for endpoint size bit-field.
#define  udd_format_endpoint_size(size)            (32 - clz(((uint32_t)Min(Max(size, 8), 1024) << 1) - 1) - 1 - 3)
  // size: 8, 16, 32, 64, 128, 256, 512, 1024
#define  udd_configure_endpoint_size(ep, size)     (USBC_EP_WR_BITFIELD(UECFG,EPSIZE,ep,udd_format_endpoint_size(size)))
#define  udd_get_endpoint_size(ep)                 (8 << USBC_EP_RD_BITFIELD(UECFG,EPSIZE,ep))
  // bank: USBC_UECFGn_EPBK_XXX defined in chip header
#define  udd_configure_endpoint_bank(ep, bank)     USBC_EP_WR_BITS(UECFG,EPBK,ep,bank)
  // return USBC_UECFGn_EPBK_XXX
#define  udd_get_endpoint_bank(ep)                 USBC_EP_RD_BITS(UECFG,EPBK,ep)

  //! configures selected endpoint in one step
  //! \param ep endpoint number
  //! \param type USB_EP_TYPE_XXXX
  //! \param dir USBC_UECFGn_EPDIR_XX
  //! \param size 8 ~ 1024
  //! \param bank USBC_UECFGn_EPBK_XXXXX
#define  udd_configure_endpoint(ep, type, dir, size, bank) \
(\
  Wr_bits(USBC_ARRAY(UECFG0,ep), (uint32_t)USBC_UECFG0_EPTYPE_Msk |\
                                  USBC_UECFG0_EPDIR  |\
                                  USBC_UECFG0_EPSIZE_Msk |\
                                  USBC_UECFG0_EPBK,   \
            USBC_UECFG0_EPTYPE(type) |\
            dir |\
            ( (uint32_t)udd_format_endpoint_size(size) << USBC_UECFG0_EPSIZE_Pos) |\
            bank)\
)

#define  udd_reset_data_toggle(ep)                 USBC_EP_REG_SET(UECON,RSTDT,ep)
#define  Is_udd_data_toggle_reset(ep)              USBC_EP_TST_BITS(UECON,RSTDT,ep)
#define  udd_data_toggle(ep)                       USBC_EP_RD_BITFIELD(UESTA,DTSEQ,ep)
//! @}


//! @name USBC Device control endpoint
//! These macros control the endpoints.
//! @{

//! @name Global NAK
//! @{
#define  udd_enable_global_nak()             USBC_SET_BITS(UDCON,GNAK)
#define  udd_disable_global_nak()            USBC_CLR_BITS(UDCON,GNAK)
#define  Is_udd_global_nak_enabled()         USBC_TST_BITS(UDCON,GNAK)
//! @}

//! @name USBC Device control endpoint interrupts
//! These macros control the endpoints interrupts.
//! @{
#define  udd_enable_endpoint_interrupt(ep) \
		(USBC->USBC_UDINTESET = USBC_UDINTESET_EP0INTES << (ep))
#define  udd_disable_endpoint_interrupt(ep) \
		(USBC->USBC_UDINTECLR = USBC_UDINTECLR_EP0INTEC << (ep))
#define  Is_udd_endpoint_interrupt_enabled(ep) \
		(Tst_bits(USBC->USBC_UDINTE, USBC_UDINTE_EP0INTE << (ep)))
#define  Is_udd_endpoint_interrupt(ep) \
		(Tst_bits(USBC->USBC_UDINT, USBC_UDINT_EP0INT << (ep)))
#define  udd_get_interrupt_endpoint_number() \
		(ctz(((USBC->USBC_UDINT >> USBC_UDINT_EP0INT_Pos) &\
		(USBC->USBC_UDINTE >> USBC_UDINT_EP0INT_Pos)) |\
		(1 << USBC_EPT_NBR)))
#define USBC_UDINT_EP0INT_Pos 12
//! @}

//! @name USBC Device control endpoint errors
//! These macros control the endpoint errors.
//! @{
  //! enables the STALL handshake
#define  udd_enable_stall_handshake(ep)            USBC_EP_REG_SET(UECON,STALLRQ,ep)
  //! disables the STALL handshake
#define  udd_disable_stall_handshake(ep)           USBC_EP_REG_CLR(UECON,STALLRQ,ep)
  //! tests if STALL handshake request is running
#define  Is_udd_endpoint_stall_requested(ep)       USBC_EP_TST_BITS(UECON,STALLRQ,ep)
  //! tests if STALL sent
#define  Is_udd_stall(ep)                          USBC_EP_TST_BITS(UESTA,STALLEDI,ep)
  //! acks STALL sent
#define  udd_ack_stall(ep)                         USBC_EP_REG_CLR(UESTA,STALLEDI,ep)
  //! raises STALL sent
#define  udd_raise_stall(ep)                       USBC_EP_REG_SET(UESTA,STALLEDI,ep)
  //! enables STALL sent interrupt
#define  udd_enable_stall_interrupt(ep)            USBC_EP_REG_SET(UECON,STALLEDE,ep)
  //! disables STALL sent interrupt
#define  udd_disable_stall_interrupt(ep)           USBC_EP_REG_CLR(UECON,STALLEDE,ep)
  //! tests if STALL sent interrupt is enabled
#define  Is_udd_stall_interrupt_enabled(ep)        USBC_EP_TST_BITS(UECON,STALLEDE,ep)

  //! tests if a RAM access error occur
#define  Is_udd_ram_access_error(ep)               USBC_EP_TST_BITS(UESTA,RAMACCERI,ep)

  //! tests if NAK OUT received
#define  Is_udd_nak_out(ep)                        USBC_EP_TST_BITS(UESTA,NAKOUTI,ep)
  //! acks NAK OUT received
#define  udd_ack_nak_out(ep)                       USBC_EP_REG_CLR(UESTA,NAKOUTI,ep)
  //! raises NAK OUT received
#define  udd_raise_nak_out(ep)                     USBC_EP_REG_SET(UESTA,NAKOUTI,ep)
  //! enables NAK OUT interrupt
#define  udd_enable_nak_out_interrupt(ep)          USBC_EP_REG_SET(UECON,NAKOUTE,ep)
  //! disables NAK OUT interrupt
#define  udd_disable_nak_out_interrupt(ep)         USBC_EP_REG_CLR(UECON,NAKOUTE,ep)
  //! tests if NAK OUT interrupt is enabled
#define  Is_udd_nak_out_interrupt_enabled(ep)      USBC_EP_TST_BITS(UECON,NAKOUTE,ep)

  //! tests if NAK IN received
#define  Is_udd_nak_in(ep)                         USBC_EP_TST_BITS(UESTA,NAKINI,ep)
  //! acks NAK IN received
#define  udd_ack_nak_in(ep)                        USBC_EP_REG_CLR(UESTA,NAKINI,ep)
  //! raises NAK IN received
#define  udd_raise_nak_in(ep)                      USBC_EP_REG_SET(UESTA,NAKINI,ep)
  //! enables NAK IN interrupt
#define  udd_enable_nak_in_interrupt(ep)           USBC_EP_REG_SET(UECON,NAKINE,ep)
  //! disables NAK IN interrupt
#define  udd_disable_nak_in_interrupt(ep)          USBC_EP_REG_CLR(UECON,NAKINE,ep)
  //! tests if NAK IN interrupt is enabled
#define  Is_udd_nak_in_interrupt_enabled(ep)       USBC_EP_TST_BITS(UECON,NAKINE,ep)

  //! acks endpoint isochronous overflow interrupt
#define  udd_ack_overflow_interrupt(ep)            USBC_EP_REG_CLR(UESTA,OVERFI,ep)
  //! raises endpoint isochronous overflow interrupt
#define  udd_raise_overflow_interrupt(ep)          USBC_EP_REG_SET(UESTA,OVERFI,ep)
  //! tests if an overflow occurs
#define  Is_udd_overflow(ep)                       USBC_EP_TST_BITS(UESTA,OVERFI,ep)
  //! enables overflow interrupt
#define  udd_enable_overflow_interrupt(ep)         USBC_EP_REG_SET(UECON,OVERFE,ep)
  //! disables overflow interrupt
#define  udd_disable_overflow_interrupt(ep)        USBC_EP_REG_CLR(UECON,OVERFE,ep)
  //! tests if overflow interrupt is enabled
#define  Is_udd_overflow_interrupt_enabled(ep)     USBC_EP_TST_BITS(UECON,OVERFE,ep)

  //! acks endpoint isochronous underflow interrupt
#define  udd_ack_underflow_interrupt(ep)           USBC_EP_REG_CLR(UESTA,UNDERFI,ep)
  //! raises endpoint isochronous underflow interrupt
#define  udd_raise_underflow_interrupt(ep)         USBC_EP_REG_SET(UESTA,UNDERFI,ep)
  //! tests if an underflow occurs
#define  Is_udd_underflow(ep)                      USBC_EP_TST_BITS(UESTA,UNDERFI,ep)
  //! enables underflow interrupt
#define  udd_enable_underflow_interrupt(ep)        USBC_EP_REG_SET(UECON,RXSTPE,ep)
  //! disables underflow interrupt
#define  udd_disable_underflow_interrupt(ep)       USBC_EP_REG_CLR(UECON,RXSTPE,ep)
  //! tests if underflow interrupt is enabled
#define  Is_udd_underflow_interrupt_enabled(ep)    USBC_EP_TST_BITS(UECON,RXSTPE,ep)

  //! tests if CRC ERROR ISO OUT detected
#define  Is_udd_crc_error(ep)                      USBC_EP_TST_BITS(UESTA,STALLEDI,ep)
  //! acks CRC ERROR ISO OUT detected
#define  udd_ack_crc_error(ep)                     USBC_EP_REG_CLR(UESTA,STALLEDI,ep)
  //! raises CRC ERROR ISO OUT detected
#define  udd_raise_crc_error(ep)                   USBC_EP_REG_SET(UESTA,STALLEDI,ep)
  //! enables CRC ERROR ISO OUT detected interrupt
#define  udd_enable_crc_error_interrupt(ep)        USBC_EP_REG_SET(UECON,STALLEDE,ep)
  //! disables CRC ERROR ISO OUT detected interrupt
#define  udd_disable_crc_error_interrupt(ep)       USBC_EP_REG_CLR(UECON,STALLEDE,ep)
  //! tests if CRC ERROR ISO OUT detected interrupt is enabled
#define  Is_udd_crc_error_interrupt_enabled(ep)    USBC_EP_TST_BITS(UECON,STALLEDE,ep)
//! @}

//! @name USBC Device control endpoint banks
//! These macros control the endpoint banks.
//! @{
#define  udd_ack_fifocon(ep)                       USBC_EP_REG_CLR(UECON,FIFOCON,ep)
#define  Is_udd_fifocon(ep)                        USBC_EP_TST_BITS(UECON,FIFOCON,ep)

#define  udd_disable_nyet(ep)                      USBC_EP_REG_SET(UECON,NYETDIS,ep)
#define  udd_enable_nyet(ep)                       USBC_EP_REG_CLR(UECON,NYETDIS,ep)

#define  udd_enable_busy_bank0(ep)                 USBC_EP_REG_SET(UECON,BUSY0,ep)
#define  udd_disable_busy_bank0(ep)                USBC_EP_REG_CLR(UECON,BUSY0,ep)
#define  udd_enable_busy_bank1(ep)                 USBC_EP_REG_SET(UECON,BUSY1,ep)
#define  udd_disable_busy_bank1(ep)                USBC_EP_REG_CLR(UECON,BUSY1,ep)
#define  udd_nb_busy_bank(ep)                      USBC_EP_RD_BITFIELD(UESTA,NBUSYBK,ep)
#define  udd_current_bank(ep)                      USBC_EP_RD_BITFIELD(UESTA,CURRBK,ep)

#define  udd_kill_last_in_bank(ep)                 USBC_EP_REG_SET(UECON,KILLBK,ep)
#define  Is_udd_last_in_bank_killed(ep)            USBC_EP_TST_BITS(UECON,KILLBK,ep)
#define  udd_force_bank_interrupt(ep)              USBC_EP_REG_SET(UESTA,NBUSYBK,ep)
#define  udd_unforce_bank_interrupt(ep)            USBC_EP_REG_SET(UESTA,NBUSYBK,ep)
#define  udd_enable_bank_interrupt(ep)             USBC_EP_REG_SET(UECON,NBUSYBKE,ep)
#define  udd_disable_bank_interrupt(ep)            USBC_EP_REG_CLR(UECON,NBUSYBKE,ep)
#define  Is_udd_bank_interrupt_enabled(ep)         USBC_EP_TST_BITS(UECON,NBUSYBKE,ep)

#define  Is_udd_short_packet(ep)                   USBC_EP_TST_BITS(UESTA,SHORTPACKETI,ep)
#define  udd_ack_short_packet(ep)                  USBC_EP_REG_CLR(UESTA,SHORTPACKETI,ep)
#define  udd_raise_short_packet(ep)                USBC_EP_REG_SET(UESTA,SHORTPACKETI,ep)
#define  udd_enable_short_packet_interrupt(ep)     USBC_EP_REG_SET(UECON,SHORTPACKETE,ep)
#define  udd_disable_short_packet_interrupt(ep)    USBC_EP_REG_CLR(UECON,SHORTPACKETE,ep)
#define  Is_udd_short_packet_interrupt_enabled(ep) USBC_EP_TST_BITS(UECON,SHORTPACKETE,ep)
//! @}

//! @name USBC Device control endpoint transfer
//! These macros control the endpoint transfers.
//! @{
#define  Is_udd_setup_received(ep)                 USBC_EP_TST_BITS(UESTA,RXSTPI,ep)
#define  udd_ack_setup_received(ep)                USBC_EP_REG_CLR(UESTA,RXSTPI,ep)
#define  udd_raise_setup_received(ep)              USBC_EP_REG_SET(UESTA,RXSTPI,ep)
#define  udd_enable_setup_received_interrupt(ep)   USBC_EP_REG_SET(UECON,RXSTPE,ep)
#define  udd_disable_setup_received_interrupt(ep)  USBC_EP_REG_CLR(UECON,RXSTPE,ep)
#define  Is_udd_setup_received_interrupt_enabled(ep) USBC_EP_TST_BITS(UECON,RXSTPE,ep)

#define  Is_udd_out_received(ep)                   USBC_EP_TST_BITS(UESTA,RXOUTI,ep)
#define  udd_ack_out_received(ep)                  USBC_EP_REG_CLR(UESTA,RXOUTI,ep)
#define  udd_raise_out_received(ep)                USBC_EP_REG_SET(UESTA,RXOUTI,ep)
#define  udd_enable_out_received_interrupt(ep)     USBC_EP_REG_SET(UECON,RXOUTE,ep)
#define  udd_disable_out_received_interrupt(ep)    USBC_EP_REG_CLR(UECON,RXOUTE,ep)
#define  Is_udd_out_received_interrupt_enabled(ep) USBC_EP_TST_BITS(UECON,RXOUTE,ep)

#define  Is_udd_in_send(ep)                        USBC_EP_TST_BITS(UESTA,TXINI,ep)
#define  udd_ack_in_send(ep)                       USBC_EP_REG_CLR(UESTA,TXINI,ep)
#define  udd_raise_in_send(ep)                     USBC_EP_REG_SET(UESTA,TXINI,ep)
#define  udd_enable_in_send_interrupt(ep)          USBC_EP_REG_SET(UECON,TXINE,ep)
#define  udd_disable_in_send_interrupt(ep)         USBC_EP_REG_CLR(UECON,TXINE,ep)
#define  Is_udd_in_send_interrupt_enabled(ep)      USBC_EP_TST_BITS(UECON,TXINE,ep)
//! @}

//! @name USB Device endpoints descriptor table management
//! @{
#define udd_udesc_set_buf0_addr(ep,buf)                \
	udd_g_ep_table[ep*2].endpoint_pipe_address = buf
#define udd_udesc_rst_buf0_size(ep)                    \
	udd_g_ep_table[ep*2].SIZES.multi_packet_size = 0
#define udd_udesc_get_buf0_size(ep)                    \
	udd_g_ep_table[ep*2].SIZES.multi_packet_size
#define udd_udesc_set_buf0_size(ep,size)               \
	udd_g_ep_table[ep*2].SIZES.multi_packet_size = size
#define udd_udesc_rst_buf0_ctn(ep)                     \
	udd_g_ep_table[ep*2].SIZES.byte_count = 0
#define udd_udesc_get_buf0_ctn(ep)                     \
	udd_g_ep_table[ep*2].SIZES.byte_count
#define udd_udesc_set_buf0_ctn(ep,size)                \
	udd_g_ep_table[ep*2].SIZES.byte_count = size
#define udd_udesc_set_buf0_autozlp(ep,val)             \
	udd_g_ep_table[ep*2].SIZES.auto_zlp = val

// Maximum size of a transfer in multipacket mode
#define UDD_ENDPOINT_MAX_TRANS ((32*1024)-1)

struct sam_usbc_udesc_sizes_t {
	uint32_t byte_count:15;
	uint32_t reserved:1;
	uint32_t multi_packet_size:15;
	uint32_t auto_zlp:1;
};

struct sam_usbc_udesc_bk_ctrl_stat_t {
	uint32_t stallrq:1;
	uint32_t reserved1:15;
	uint32_t crcerri:1;
	uint32_t overfi:1;
	uint32_t underfi:1;
	uint32_t reserved2:13;
};

struct sam_usbc_udesc_ep_ctrl_stat_t {
	uint32_t pipe_dev_addr:7;
	uint32_t reserved1:1;
	uint32_t pipe_num:4;
	uint32_t pipe_error_cnt_max:4;
	uint32_t pipe_error_status:8;
	uint32_t reserved2:8;
};

typedef struct {
	uint8_t *endpoint_pipe_address;
	union {
		uint32_t sizes;
		struct sam_usbc_udesc_sizes_t SIZES;
	};
	union {
		uint32_t bk_ctrl_stat;
		struct sam_usbc_udesc_bk_ctrl_stat_t BK_CTRL_STAT;
	};
	union {
		uint32_t ep_ctrl_stat;
		struct sam_usbc_udesc_ep_ctrl_stat_t EP_CTRL_STAT;
	};
} usb_desc_table_t;
//! @}

//! @}
//! @}
//! @}

#endif // _USBC_DEVICE_H_
