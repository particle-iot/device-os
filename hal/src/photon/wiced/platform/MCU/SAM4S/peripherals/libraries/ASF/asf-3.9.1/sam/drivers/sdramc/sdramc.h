/**
 * \file
 *
 * \brief SDRAM controller (SDRAMC) driver for SAM.
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

#ifndef SDRAMC_H_INCLUDED
#define SDRAMC_H_INCLUDED

#include  "compiler.h"
#include "sleepmgr.h"
/// @cond 0
/**INDENT-OFF**/
#ifdef __cplusplus
extern "C" {
#endif
/**INDENT-ON**/
/// @endcond

/* SDRAM memory type */
typedef struct sdramc_memory_dev {
	/** Block 1 address */
	uint32_t ul_bk1;
	/** SDRAM mode */
	uint32_t ul_mode;
	union sdramc_cr {
		/** SDRAMC configuration */
		uint32_t ul_cfg;
		struct sdramc_cr_bm {
			/** Number of Column Bits */
			uint32_t ul_column_bits : 2,
			/** Number of Row Bits */
			ul_row_bits : 2,
			/** Number of Banks */
			ul_banks : 1,
			/** CAS Latency */
			ul_cas : 2,
			/** Data Bus Width */
			ul_data_bus_width : 1,
			/** Write Recovery Delay */
			ul_write_recovery_delay : 4,
			/** Row Cycle Delay and Row Refresh Cycle */
			ul_row_cycle_delay_row_fefresh_cycle : 4,
			/** Row Precharge Delay */
			ul_row_precharge_delay : 4,
			/** Row to Column Delay */
			ul_row_column_delay : 4,
			/** Active to Precharge Delay */
			ul_active_precharge_delay : 4,
			/** Exit from Self Refresh to Active Delay */
			ul_exit_self_refresh_active_delay : 4;
		} sdramc_cr_bm_t;
	} cr;
} sdramc_memory_dev_t;

/**
 * \brief Initialize the SDRAM controller.
 *
 * \param p_sdram  Pointer to the sdram memory structure.
 * \param ul_clk  SDRAM clock frequency.
 */
void sdramc_init(sdramc_memory_dev_t *p_sdram, uint32_t ul_clk);

/**
 * \brief De-initialize the SDRAM controller.
 */
void sdramc_deinit( void );

/**
 * \brief Set the SDRAM in self-refresh mode.
 */
static inline void sdram_enter_self_refresh(void)
{
	sleepmgr_unlock_mode(SLEEPMGR_ACTIVE);
	SDRAMC->SDRAMC_LPR |= SDRAMC_LPR_LPCB_SELF_REFRESH;
}

/**
 * \brief Exit from the SDRAM self-refresh mode, and inhibit self-refresh mode.
 */
static inline void sdram_exit_self_refresh(void)
{
	sleepmgr_lock_mode(SLEEPMGR_ACTIVE);
	SDRAMC->SDRAMC_LPR &= ~SDRAMC_LPR_LPCB_SELF_REFRESH;
}

/**
 * \brief Set the SDRAM in power down mode.
 */
static inline void sdram_enter_power_down(void)
{
	sleepmgr_unlock_mode(SLEEPMGR_ACTIVE);
	SDRAMC->SDRAMC_LPR |= SDRAMC_LPR_LPCB_POWER_DOWN;
}

/**
 * \brief Exit from the SDRAM power down mode, and inhibit power down mode.
 */
static inline void sdram_exit_power_down(void)
{
	sleepmgr_lock_mode(SLEEPMGR_ACTIVE);
	SDRAMC->SDRAMC_LPR &= ~SDRAMC_LPR_LPCB_POWER_DOWN;
}

/**
 * \brief Enter SDRAM deep power down mode.
 *
 * \note This mode is unique to low-power SDRAM.
 */
static inline void sdram_enter_deep_power_down(void)
{
	sleepmgr_unlock_mode(SLEEPMGR_ACTIVE);
	SDRAMC->SDRAMC_LPR |= SDRAMC_LPR_LPCB_DEEP_POWER_DOWN;
}

/// @cond 0
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/// @endcond

/**
 * \page sam_sdramc_quickstart Quickstart guide for SDRAMC driver.
 *
 * This is the quickstart guide for the \ref sam_drivers_sdramc_group
 * "SDRAM Controller", with step-by-step instructions on how to configure
 * and use the driver in a selection of use cases.
 *
 * The use cases contain several code fragments. The code fragments in the
 * steps for setup can be copied into a custom initialization function, while
 * the steps for usage can be copied into, e.g., the main application function.
 *
 * \section sdramc_basic_use_case Basic use case
 * In the basic use case, the SDMRAC driver are configured for:
 * - SDRAM component MT48C16M16A2 is used
 * - Write/Read access to the MT48C16M16A2
 *
 * \section sdramc_basic_use_case_setup Setup steps
 *
 * \subsection sdramc_basic_use_case_setup_prereq Prerequisites
 * -# \ref sysclk_group "System Clock Management (sysclock)"
 * -# \ref sam_drivers_pio_group "Parallel Input/Output Controller (pio)" 
 * -# \ref sam_drivers_pmc_group "Power Management Controller (pmc)"
 *
 * \subsection sdramc_basic_use_case_setup_code Example code
 * The macro of CONF_BOARD_SDRAMC must be added to the project:
 * \code
 *   #define CONF_BOARD_SDRAMC
 * \endcode
 *
 * A speicific sdramc device must be defined.
 * \code
 *
 * const sdramc_memory_dev_t SDRAM_MICRON_MT48LC16M16A2 = {
 *   24,
 *   0,
 *   {
 *     SDRAMC_CR_NC_COL9      |
 *     SDRAMC_CR_NR_ROW13     |
 *     SDRAMC_CR_NB_BANK4     |
 *     SDRAMC_CR_CAS_LATENCY2 |
 *     SDRAMC_CR_DBW          |
 *     SDRAMC_CR_TWR(2)       |
 *     SDRAMC_CR_TRC_TRFC(9)  |
 *     SDRAMC_CR_TRP(3)       |
 *     SDRAMC_CR_TRCD(3)      |
 *     SDRAMC_CR_TRAS(6)      |
 *     SDRAMC_CR_TXSR(10)
 *   },
 * };
 * \endcode
 *
 * Add to application C-file:
 * \code
 * sysclk_init();
 * board_init();
 * sdramc_init((sdramc_memory_dev_t *)&SDRAM_MICRON_MT48LC16M16A2,
 *            sysclk_get_cpu_hz());
 *
 * \endcode
 *
 * \subsection sdramc_basic_use_case_setup_flow Workflow
 * -# Enable the PINs for SDRAMC:
 * \code
 *    #define CONF_BOARD_SDRAMC
 * \endcode
 * -# Initialize the SDRAM device:
 * - \code
 * const sdramc_memory_dev_t SDRAM_MICRON_MT48LC16M16A2 = {
 * \endcode
 *  -#  Block1 is at the bit 24, 1(M0)+9(Col)+13(Row)+1(BK1):
 *   - \code
 *   24,
 *    \endcode
 *  -#  Set SDRAMC to normal mode 0:
 *   - \code
 *   0,
 *    \endcode
 *  -#  MT48LC16M16A2 has 9 column bits:
 *   - \code
 *   SDRAMC_CR_NC_COL9      |
 *    \endcode
 *  -#  MT48LC16M16A2 has 13 row bits:
 *   - \code
 *   SDRAMC_CR_NR_ROW13     |
 *    \endcode
  *  -#  MT48LC16M16A2 has 4 banks:
 *   - \code
 *   SDRAMC_CR_NB_BANK4     |
 *    \endcode
 *  -#  Set CAS latency to 2 cycles:
 *   - \code
 *   SDRAMC_CR_CAS_LATENCY2 |
 *    \endcode
 *  -#  The data bus width of MT48LC16M16A2 is 16 bits:
 *   - \code
 *   SDRAMC_CR_DBW          |
 *    \endcode
 *  -#  Set Write Recovery Delay to 2 cycles:
 *   - \code
 *   SDRAMC_CR_TWR(2)       |
 *    \endcode
 *  -#  Set Row Cycle Delay and Row Refresh Cycle to 9 cycles:
 *   - \code
 *   SDRAMC_CR_TRC_TRFC(9)  |
 *    \endcode
 *  -#  Set Row Precharge Delay to 3 cycles:
 *   - \code
 *   SDRAMC_CR_TRP(3)       |
 *    \endcode
 *  -#  Set Row to Column Delay to 3 cycles:
 *   - \code
 *   SDRAMC_CR_TRCD(3)      |
 *    \endcode
 *  -#  Set Active to Precharge Delay to 6 cycles:
 *   - \code
 *   SDRAMC_CR_TRAS(6)      |
 *    \endcode
 *  -#  Set Exit from Self Refresh to Active Delay to 10 cycles:
 *   - \code
 *   SDRAMC_CR_TXSR(10)
 *    \endcode
 * -# Enable system clock:
 * - \code sysclk_init(); \endcode
 * -# Enable PIO configurations for SDRMAC:
 * - \code board_init(); \endcode
 * -# Initialize MT48LC16M16A2 device:
 * - \code
 *  sdramc_init((sdramc_memory_dev_t *)&SDRAM_MICRON_MT48LC16M16A2,
 *            sysclk_get_cpu_hz());
 * \endcode
 *
 * \section sdramc_basic_use_case_usage Usage steps
 * \subsection sdramc_basic_use_case_usage_code Example code
 * Add to, e.g., main loop in application C-file:
 * \code
 *    uint32_t *pul = (uint32_t *)BOARD_SDRAM_ADDR;
 *
 *    pul[0] = 0xdeadbeef;
 *    if (pul[0] == 0xdeadbeef) {
 *        LED_On(LED0_GPIO);
 *    }
 * \endcode
 *
 * \subsection sdramc_basic_use_case_usage_flow Workflow
 * -#  Set the pointer to the SDRAMC address:
 *   - \code
 *  uint32_t *pul = (uint32_t *)BOARD_SDRAM_ADDR;
 *    \endcode
 * -#  Write the specific data to the SDRAMC space:
 *   - \code
 *  pul[0] = 0xdeadbeef;
 *    \endcode
 * -#  Read the data, if matched the specific data, turn on the LED0:
 *   - \code
 *  if (pul[0] == 0xdeadbeef) {
 *      LED_On(LED0_GPIO);
 * 	}
 *    \endcode
 */

#endif /* SDRAMC_H_INCLUDED */
