/**
 * \file
 *
 * \brief SAM Liquid Crystal Display driver (LCDCA).
 *
 * Copyright (c) 2012 Atmel Corporation. All rights reserved.
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
#ifndef LCDCA_H_INCLUDED
#define LCDCA_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "compiler.h"
#include "sysclk.h"
#include "conf_lcdca.h"
#include "pdca.h"

/**
 * \defgroup group_sam_drivers_lcdca Liquid Crystal Display (LCDCA)
 *
 * This is a driver for configuring, enabling/disabling and use of the on-chip
 * LCDCA controller.
 *
 * \section dependencies Dependencies
 *
 * The LCDCA module depends on the following modules:
 * - \ref sysclk_group for LCDCA clock control.
 * - \ref interrupt_group for enabling or disabling interrupts.
 * - \ref sleepmgr_group to unlock LCDCA controller
 * @{
 */

/** \name LCDCA addressing limits */
/* @{ */
/** Maximum number of common lines.*/
#define LCDCA_MAX_NR_OF_COM    4
/** Maximum number of segment lines. */
# define LCDCA_MAX_NBR_OF_SEG  40
/* @} */

/** \name LCDCA Type of digit */
/* @{ */
/** 7-segment with 3 common terminals. */
#define LCDCA_TDG_7SEG3COM        0
/** 7-segment with 4 common terminals. */
#define LCDCA_TDG_7SEG4COM        1
/** 14-segment with 4 common terminals. */
#define LCDCA_TDG_14SEG4COM       2
/** 16-segment with 3 common terminals. */
#define LCDCA_TDG_16SEG3COM       3
/* @} */

/** \name LCDCA Timer Resource */
/* @{ */
/** Timer FCO ressource. */
#define LCDCA_TIMER_FC0        0
/** Timer FC1 ressource. */
#define LCDCA_TIMER_FC1        1
/** Timer FC2 ressource. */
#define LCDCA_TIMER_FC2        2
/* @} */

/** \name LCDCA CMCFG Digit Reverse Mode */
/* @{ */
#define LCDCA_CMCFG_DREV_LEFT   1
#define LCDCA_CMCFG_DREV_RIGHT  0
/* @} */

/** \name LCDCA Automated Char DMA Channel (Sequential or Scrolling) */
/* @{ */
#ifndef LCDCA_AUTOMATED_CHAR_DMA_CH
#define LCDCA_AUTOMATED_CHAR_DMA_CH  3
#endif
/* @} */

/**
 * Basic configuration for LCDCA controller.
 */
typedef struct lcdca_config {
	/** Number of SEG used. */
	uint8_t port_mask;
	/** External bias (false: internal gen, true: external gen). */
	bool x_bias;
	/** Wave mode (false: lowpower waveform, true: standard waveform). */
	bool lp_wave;
	/** Duty type selection. */
	uint8_t duty_type;
/** \name LCDCA duty selection */
/* @{ */
#define LCDCA_DUTY_1_4     0   /**< Duty=1/4, Bias=1/3, COM0:3 */
#define LCDCA_DUTY_STATIC  1   /**< Duty=Static, Bias=Static, COM0 */
#define LCDCA_DUTY_1_2     2   /**< Duty=1/2, Bias=1/3, COM0:1 */
#define LCDCA_DUTY_1_3     3   /**< Duty=1/3, Bias=1/3, COM0:2 */
/* @} */
	/** prescaler of the clock source. */
	bool lcd_pres;
	/** Divider of the prescaled clock source. */
	uint8_t lcd_clkdiv;
	/** Frame Counter 0. */
	uint8_t fc0;
	/** Frame Counter 1. */
	uint8_t fc1;
	/** Frame Counter 2. */
	uint8_t fc2;
	/** -32 <= signed contrast value <= 31. */
	int8_t contrast;
} lcdca_config_t;

/**
 *  Blink configuration.
 */
typedef struct lcdca_blink_config {
	/** LCD Blink Timer Selected. */
	uint8_t lcd_blink_timer;
	/** Blink Mode selected */
	uint8_t lcd_blink_mode;
/** \name LCDCA Blink Mode */
/* @{ */
/** Blink mode for LCD: blink all lcd. */
#define LCDCA_BLINK_FULL       0
/** Blink mode for LCD: only selected segment will blink. */
#define LCDCA_BLINK_SELECTED   LCDCA_BCFG_MODE
/* @} */
} lcdca_blink_config_t;

/**
 * Circular shift configuration.
 */
typedef struct lcdca_circular_shift_config {
	/** LCD Shift register Timer Selected. */
	uint8_t lcd_csr_timer;
	/** Shift direction. */
	uint8_t lcd_csr_dir;
/** \name LCDCA Shift Register Direction */
/* @{ */
/** Right Direction. */
#define LCDCA_CSR_RIGHT       LCDCA_CSRCFG_DIR
/** Left Direction. */
#define LCDCA_CSR_LEFT        0
/* @} */
	/** Defines the size of the circular shift register. */
	uint8_t size;
	/** Circular Shift Register Value. */
	uint8_t data;
} lcdca_circular_shift_config_t;

/**
 * Automated char display configuration.
 */
typedef struct lcdca_automated_char_config {
	/** Automated display mode selection: sequential or scrolling */
	uint8_t automated_mode;
/** \name Automated Mode */
/* @{ */
/** Sequential character string display mode */
#define LCDCA_AUTOMATED_MODE_SEQUENTIAL    0
/** Scrolling of character string display mode */
#define LCDCA_AUTOMATED_MODE_SCROLLING     1
/* @} */

	/** Timer Selected for automated mode */
	uint8_t automated_timer;
	/** Type of digit selected */
	uint8_t lcd_tdg;
	/** Defines the start segment */
	uint8_t stseg;
	/**
	 * Defines the number of steps in scrolling mode.
	 * STEPS = string length - DIGN + 1
	 */
	uint8_t steps;
	/** Defines the the number of digit used */
	uint8_t dign;

	/** Define digit display direction */
	uint8_t dir_reverse;
/** \name LCDCA automated direction */
/* @{ */
/** Digit direction reversed. */
#define LCDCA_AUTOMATED_DIR_REVERSE        1
/** Digit direction not reversed. */
#define LCDCA_AUTOMATED_DIR_NOT_REVERSE    0
/* @} */
} lcdca_automated_char_config_t;

/**
 * \brief Interrupt event callback function type
 *
 * The interrupt handler can be configured to do a function callback,
 * the callback function must match the lcdca_callback_t type.
 *
 */
typedef void (*lcdca_callback_t)(void);

/**
 * \brief LCDCA clock initialization.
 *
 * This function enables the specified LCDCA clock (RC32K or OSC32K) for
 * LCDCA input clock.
 *
 */
void lcdca_clk_init(void);

/**
 * \brief Set basic configuration for LCDCA controller.
 *
 * The function enables LCDCA port mask, waveform mode, duty type, contrast
 * and the using (or no) of an External Resistor Bias Generation.
 * It'll also set up timer for LCDCA controller, include clock prescaler,
 * dividor and frame counter.
 *
 * \param  lcdca_cfg Configuration for LCDCA controller.
 */
void lcdca_set_config(struct lcdca_config *lcdca_cfg);

/**
 * \brief Enable LCDCA.
 *
 * This function enables the LCDCA module.
 */
void lcdca_enable(void);

/**
 * \brief Disable LCDCA.
 *
 * This function disables the LCDCA module.
 *
 */
void lcdca_disable(void);

/**
 * \brief Set the LCDCA contrast.
 *
 * This function finely sets the LCDCA contrast.
 *
 * Transfer function: VLCD = 3.0 V + (fcont[5:0] * 0.016 V)
 *
 * \param  contrast  -32 <= signed contrast value <= 31.
 */
void lcdca_set_contrast(int8_t contrast);

/**
 * \brief Set all LCDCA display memory.
 * This will set all content in display memory.
 */
void lcdca_set_display_memory(void);

/**
 * \brief Clear all LCDCA display memory.
 * This will clear all content in display memory.
 */
static inline void lcdca_clear_display_memory(void)
{
	LCDCA->LCDCA_CR = LCDCA_CR_CDM;
}

/**
 * \brief Lock LCDCA shadow display memory.
 */
static inline void lcdca_lock_shadow_dislay(void)
{
	LCDCA->LCDCA_CFG |= LCDCA_CFG_LOCK;
}

/**
 * \brief Unlock LCDCA shadow display memory.
 */
static inline void lcdca_unlock_shadow_dislay(void)
{
	LCDCA->LCDCA_CFG &= ~LCDCA_CFG_LOCK;
}

/**
 * \brief LCDCA timer enable.
 *
 * This function enables the LCDCA timer and wait until it is correctly enabled.
 *
 * \note lcdca_enable() must be called before this function.
 *
 * \param  lcd_timer   Timer number to be enabled.
 */
void lcdca_enable_timer(uint8_t lcd_timer);

/**
 * \brief LCDCA timer disable.
 *
 * This function disables the LCDCA timer.
 *
 * \param  lcd_timer   Timer number to be disabled.
 */
void lcdca_disable_timer(uint8_t lcd_timer);

/**
 * \brief Set configuration for LCDCA blinking.
 *
 * The function defines the blinking rate. In the same time, the hardware
 * display blinking is disabled.
 *
 * \param  blink_cfg Configuration for blinking.
 */
void lcdca_blink_set_config(struct lcdca_blink_config *blink_cfg);

/**
 * \brief Set the blinking of pixels.
 *
 * This function sets the blinking of pixels in LCD module on SEG0 & SEG1.
 * Each bit position corresponds to one pixel.
 *
 * \param  pix_com  Pixel coordinate - COMx - of the pixel (icon).
 * \param  pix_seg  Pixel coordinate - SEGy - of the pixel (icon).
 *
 */
void lcdca_set_blink_pixel(uint8_t pix_com, uint8_t pix_seg);

/**
 * \brief Clear the blinking of pixels.
 *
 * This function clears the blinking of pixels in LCD module on SEG0 & SEG1.
 * Each bit position corresponds to one pixel.
 *
 * \param  pix_com  Pixel coordinate - COMx - of the pixel (icon).
 * \param  pix_seg  Pixel coordinate - SEGy - of the pixel (icon).
 *
 */
void lcdca_clear_blink_pixel(uint8_t pix_com, uint8_t pix_seg);

/**
 * \brief Clear the blinking of all pixels.
 *
 * This function clears the blinking of all pixels in LCD module on SEG0 & SEG1.
 *
 */
void lcdca_clear_blink_all_pixel(void);

/**
 * \brief LCDCA blink enable.
 *
 * This function enables the blinking mode in LCD module.
 */
static inline void lcdca_blink_enable(void)
{
	/* Blinking "on" */
	LCDCA->LCDCA_CR  = LCDCA_CR_BSTART;
}

/**
 * \brief LCDCA blink disable.
 *
 * This function disables the blinking mode in LCD module.
 */
static inline void lcdca_blink_disable(void)
{
	/* Blinking "off" */
	LCDCA->LCDCA_CR  = LCDCA_CR_BSTOP;
}

/**
 * \brief Set configuration for LCDCA automated display.
 *
 * This function initializes the hardware sequential or scrolling.
 * At the same time, the hardware automated display is disabled.
 *
 * \param  ac_cfg Pointer to automated display configuration.
 */
void lcdca_automated_char_set_config(
		struct lcdca_automated_char_config *ac_cfg);

/**
 * \brief LCDCA automated display start.
 *
 * This function start the hardware automated char display.
 *
 * \param  data          Data string buffer.
 * \param  width         Data string length.
 */
void lcdca_automated_char_start(const uint8_t *data, size_t width);

/**
 * \brief LCDCA automated display reload.
 *
 * \param  data          Data string buffer.
 * \param  width         Data string length.
 */
void lcdca_automated_char_reload(const uint8_t *data, size_t width);

/**
 * \brief LCDCA automated display stop.
 *
 * This function stop the hardware automated display.
 *
 */
static inline void lcdca_automated_char_stop(void)
{
	/* Disable PDCA channel */
	pdca_channel_disable(LCDCA_AUTOMATED_CHAR_DMA_CH);

	/* Disable automated display */
	LCDCA->LCDCA_ACMCFG &= ~LCDCA_ACMCFG_EN;
}

/**
 * \brief Set configuration for LCDCA circular shift.
 *
 * The function defines the shift register rate. In the same time, the hardware
 * display shift register is disabled.
 *
 * \param  cs_cfg Configuration of circular shift.
 */
void lcdca_circular_shift_set_config(struct lcdca_circular_shift_config *cs_cfg);

/**
 * \brief LCDCA circular shift enable.
 *
 * This function enables the CSR mode in LCD module.
 */
static inline void lcdca_circular_shift_enable(void)
{
	/* CSR "on" */
	LCDCA->LCDCA_CR  = LCDCA_CR_CSTART;
}

/**
 * \brief LCDCA circular shift disable.
 *
 * This function disables the CSR mode in LCD module.
 */
static inline void lcdca_circular_shift_disable(void)
{
	/* CSR "off" */
	LCDCA->LCDCA_CR  = LCDCA_CR_CSTOP;
}

/**
 * \brief Send a sequence of ASCII characters to LCD device.
 *
 * This function enables LCD segments via the digit decoder.
 * The function will write the maximum number of byte passed as argument,
 * and will stop writing if a NULL character is found.
 *
 * \param  lcd_tdg    Type of digit decoder.
 * \param  first_seg  First SEG where the first data will be writen.
 * \param  data       Data buffer.
 * \param  width      Maximum Number of data.
 * \param  dir        Direction (==0: Left->Right, !=0: Left<-Right).
 */
void lcdca_write_packet(uint8_t lcd_tdg, uint8_t first_seg,
		const uint8_t *data, size_t width, uint8_t dir);

/**
 * \brief Set pixel (icon) in LCDCA display memory.
 *
 * This function sets a pixel in LCD (icon) display memory. If a parameter
 * is out of range, then the function doesn't set any bit.
 *
 * \param  pix_com  Pixel coordinate - COMx - of the pixel (icon).
 * \param  pix_seg  Pixel coordinate - SEGy - of the pixel (icon).
 */
void lcdca_set_pixel(uint8_t pix_com, uint8_t pix_seg);

/**
 * \brief Clear pixel (icon) in LCDCA display memory.
 *
 * This function clears a pixel in LCD (icon) display memory. If a parameter
 * is out of range, then the function doesn't clear any bit.
 *
 * \param  pix_com  Pixel coordinate - COMx - of the pixel (icon).
 * \param  pix_seg  Pixel coordinate - SEGy - of the pixel (icon).
 */
void lcdca_clear_pixel(uint8_t pix_com, uint8_t pix_seg);

/**
 * \brief Toggle pixel (icon) in LCDCA display memory.
 *
 * This function toggles a pixel in LCD (icon) display memory. If a parameter
 * is out of range, then the function doesn't toggle any bit.
 *
 * \param  pix_com  Pixel coordinate - COMx - of the pixel (icon).
 * \param  pix_seg  Pixel coordinate - SEGy - of the pixel (icon).
 */
void lcdca_toggle_pixel(uint8_t pix_com, uint8_t pix_seg);

/**
 * \brief Get pixel value (icon) in LCDCA display memory.
 *
 * \param  pix_com  Pixel coordinate - COMx - of the pixel (icon).
 * \param  pix_seg  Pixel coordinate - SEGy - of the pixel (icon).
 *
 * \return return the pixel value in LCD (icon) display memory.
 */
bool lcdca_get_pixel(uint8_t pix_com, uint8_t pix_seg);

/**
 * \brief Enable LCDCA to wake up CPU.
 */
static inline void lcdca_enable_wakeup(void)
{
	PM->PM_AWEN |= ((0x1u << PM_AWEN_LCDCA) << PM_AWEN_AWEN_Pos);
	LCDCA->LCDCA_CR = LCDCA_CR_WEN;
}

/**
 * \brief Disable LCDCA to wake up CPU.
 */
static inline void lcdca_disable_wakeup(void)
{
	LCDCA->LCDCA_CR = LCDCA_CR_WDIS;
	PM->PM_AWEN &= ~((0x1u << PM_AWEN_LCDCA) << PM_AWEN_AWEN_Pos);
}

/**
 * \brief Get the LCDCA status.
 *
 * \return return bit OR of the LCDCA status register.
 */
static inline uint32_t lcdca_get_status(void)
{
	return LCDCA->LCDCA_SR;
}

/**
 * \brief Clear the LCDCA status.
 *
 * This function clears "FC0R" bit of the LCDCA status.
 */
static inline void lcdca_clear_status(void)
{
	LCDCA->LCDCA_SCR = LCDCA_SCR_FC0R;
}

/**
 * \brief LCDCA frame interrupt callback function
 *
 * This function allows the caller to set and change the interrupt callback
 * function. Without setting a callback function the interrupt handler in the
 * driver will only clear the interrupt flags.
 *
 * \param callback Reference to a callback function
 * \param irq_line  interrupt line.
 * \param irq_level interrupt level.
 */
void lcdca_set_callback(lcdca_callback_t callback,
		uint8_t irq_line, uint8_t irq_level);

/**
 * \brief LCDCA enable interrupt.
 *
 * This function enables the interrupt for LCD module.
 */
static inline void lcdca_enable_interrupt(void)
{
	LCDCA->LCDCA_IER = LCDCA_IER_FC0R;
}

/**
 * \brief LCDCA disable interrupt.
 *
 * This function disables the interrupt for LCD module.
 */
static inline void lcdca_disable_interrupt(void)
{
	LCDCA->LCDCA_IDR = LCDCA_IDR_FC0R;
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

/**
 * \page sam_drivers_lcdca_quick_start Quick Start Guide for the LCDCA driver
 *
 * This is the quick start guide for the \ref group_sam_drivers_lcdca, with
 * step-by-step instructions on how to configure and use the driver for
 * a specific use case.
 *
 * The use cases contain several code fragments. The code fragments in the
 * steps for setup can be copied into a custom initialization function, while
 * the steps for usage can be copied into, e.g., the main application function.
 *
 * \section lcdca_qs_use_cases Use cases
 * - \ref lcdca_basic
 *
 * \section lcdca_basic LCDCA basic usage
 *
 * This use case will demonstrate how to configure and use of the on-chip
 * LCDCA controller to address an external LCD segment (C42048A).
 *
 * \section lcdca_basic_setup Setup steps
 *
 * \subsection lcdca_basic_prereq Prerequisites
 *
 * This module requires the following service
 * - \ref clk_group
 * - \ref sleepmgr_group
 *
 * \subsection lcdca_basic_setup_code Setup Code Example
 *
 * Add this to the main loop or a setup function:
 * \code
 *   #define PORT_MASK  40
 *   #define LCD_DUTY   LCDCA_DUTY_1_4
 *   #define LCD_CONTRAST_LEVEL 30
 *
 *   struct lcdca_config lcdca_cfg;
 *
 *   // LCDCA Controller initialization
 *   // - Clock,
 *   // - Connect to C42364A glass LCD component,
 *   // - Timing:  64 Hz frame rate & low power waveform, FC0, FC1, FC2
 *   // - Interrupt: off
 *   lcdca_clk_init();
 *   lcdca_cfg.port_mask = PORT_MASK;
 *   lcdca_cfg.x_bias = false;
 *   lcdca_cfg.lp_wave = true;
 *   lcdca_cfg.duty_type = LCD_DUTY;
 *   lcdca_cfg.lcd_pres = false;
 *   lcdca_cfg.lcd_clkdiv = 3;
 *   lcdca_cfg.fc0 = 16;
 *   lcdca_cfg.fc1 = 2;
 *   lcdca_cfg.fc2 = 6;
 *   lcdca_cfg.contrast = LCD_CONTRAST_LEVEL;
 *   lcdca_set_config(&lcdca_cfg);
 *   lcdca_enable();
 *   lcdca_enable_timer(LCDCA_TIMER_FC0);
 *   lcdca_enable_timer(LCDCA_TIMER_FC1);
 *   lcdca_enable_timer(LCDCA_TIMER_FC2);
 *
 *   // Turn on LCD back light
 *   ioport_set_pin_level(LCD_BL_GPIO, IOPORT_PIN_LEVEL_HIGH);
 * \endcode
 *
 * \subsection lcdca_basic_setup_workflow Basic Setup Workflow
 *
 * -# Initialize LCDCA clock
 *  - \code lcdca_clk_init(); \endcode
 * -# Set basic LCDCA configuration
 *  - \code lcdca_set_config(&lcdca_cfg); \endcode
 * -# Enable LCDCA module
 *  - \code lcdca_enable(); \endcode
 * -# Enable frame counter timer according to your application
 *  - \code
 *    lcdca_enable_timer(LCDCA_TIMER_FC0);
 *    lcdca_enable_timer(LCDCA_TIMER_FC1);
 *    lcdca_enable_timer(LCDCA_TIMER_FC2);
 *    \endcode
 * -# Turn on LCD back light
 *  - \code ioport_set_pin_level(LCD_BL_GPIO, IOPORT_PIN_LEVEL_HIGH); \endcode
 *
 * \section lcdca_basic_usage Usage steps
 *
 * \subsection lcdca_basic_usage_normal Normal Usage
 *
 * We can use below functions for set/clear/toggle one pixel:
 * \code
 * lcdca_set_pixel(ICON_ARM);
 * lcdca_clear_pixel(ICON_ARM);
 * lcdca_toggle_pixel(ICON_ARM);
 * \endcode
 *
 * We can use lcdca_write_packet() to display ASCII characters:
 * \code
 * // Display in alphanumeric field
 * lcdca_write_packet(LCDCA_TDG_14SEG4COM, FIRST_14SEG_4C, data, \
 *         WIDTH_14SEG_4C, DIR_14SEG_4C);
 *
 * // Display in numeric field
 * lcdca_write_packet(LCDCA_TDG_7SEG4COM, FIRST_7SEG_4C, data, \
 *         WIDTH_7SEG_4C, DIR_7SEG_4C);
 * \endcode
 *
 * We can change LCD contrast:
 * \code
 * lcdca_set_contrast(contrast_value);
 * \endcode
 *
 * \subsection lcdca_basic_usage_blink Using Hardware Blinking
 * We can use hardware blinking:
 * \code
 * struct lcdca_blink_config blink_cfg;
 *
 * blink_cfg.lcd_blink_timer = LCDCA_TIMER_FC1;
 * blink_cfg.lcd_blink_mode = LCDCA_BLINK_SELECTED;
 * lcdca_blink_set_config(&blink_cfg);
 * lcdca_set_pixel(ICON_ERROR);
 * lcdca_set_blink_pixel(ICON_ERROR);
 * lcdca_blink_enable();
 * \endcode
 *
 * \subsection lcdca_basic_usage_autonomous Using Hardware Autonomous Animation
 * We can use hardware autonomous segment animation:
 * \code
 * struct lcdca_circular_shift_config cs_cfg;
 *
 * cs_cfg.lcd_csr_timer = LCDCA_TIMER_FC1;
 * cs_cfg.lcd_csr_dir = LCDCA_CSR_RIGHT;
 * cs_cfg.size = 7;    // Total 7-pixels
 * cs_cfg.data = 0x03; // Display 2 pixel at one time
 * lcdca_circular_shift_set_config(&cs_cfg);
 * lcdca_circular_shift_enable();
 * \endcode
 *
 * \subsection lcdca_basic_usage_automated_char Using Hardware Automated Character
 * We can use hardware automated character (e.g., scrolling here):
 * \code
 * struct lcdca_automated_char_config automated_char_cfg;
 * uint8_t const scrolling_str[] = "Scrolling string display";
 *
 * automated_char_cfg.automated_mode = LCDCA_AUTOMATED_MODE_SCROLLING;
 * automated_char_cfg.automated_timer = LCDCA_TIMER_FC2;
 * automated_char_cfg.lcd_tdg = LCDCA_TDG_14SEG4COM;
 * automated_char_cfg.stseg = FIRST_14SEG_4C;
 * automated_char_cfg.dign = WIDTH_14SEG_4C;
 * // STEPS = string length - DIGN + 1
 * automated_char_cfg.steps = sizeof(scrolling_str) - WIDTH_14SEG_4C + 1;
 * automated_char_cfg.dir_reverse = LCDCA_AUTOMATED_DIR_REVERSE;
 * lcdca_automated_char_set_config(&automated_char_cfg);
 * lcdca_automated_char_start(scrolling_str, strlen((char const *)scrolling_str));
 * \endcode
 *
 */

#endif /* LCDCA_H_INCLUDED */
