/**
 *
 * \file
 *
 * \brief AES software driver for SAM.
 *
 * This file defines a useful set of functions for the AES on SAM devices.
 *
 * Copyright (c) 2013 Atmel Corporation. All rights reserved.
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

#ifndef AES_H_INCLUDED
#define AES_H_INCLUDED

/**
 * \defgroup group_sam_drivers_aesa AES - Advanced Encryption Standard module
 *
 * Driver for the AES (Advanced Encryption Standard) module.
 * Provides functions for configuring and initiating ciphering/deciphering
 * with AES algorithms.
 * Several modes(ECB, CBC, OFB, CFB or CTR) and
 * key sizes(128-, 192- or 256-bit) are supported,
 * in addition to manual or DMA-based data input to the AES module.
 *
 * \{
 */

#include "compiler.h"


/** AES Processing mode */
enum aes_encrypt_mode {
	AES_DECRYPTION = 0,
	AES_ENCRYPTION,
};
/** AES Cryptographic key size */
enum aes_key_size {
	AES_KEY_SIZE_128 = 0,
	AES_KEY_SIZE_192,
	AES_KEY_SIZE_256,
};

/** AES DMA mode */
enum aes_dma_mode {
	AES_MANUAL_MODE = 0,
	AES_DMA_MODE,
};

/** AES Confidentiality mode */
enum aes_opmode {
	AES_ECB_MODE = 0,
	AES_CBC_MODE,
	AES_OFB_MODE,
	AES_CFB_MODE,
	AES_CTR_MODE,
};

/** AES CFB size */
enum aes_cfb_size {
	AES_CFB_SIZE_128 = 0,
	AES_CFB_SIZE_64,
	AES_CFB_SIZE_32,
	AES_CFB_SIZE_16,
	AES_CFB_SIZE_8,
};

/** AES CounterMeasure type */
enum aes_countermeature_type {
	AES_COUNTERMEASURE_TYPE_1 = 0x01,
	AES_COUNTERMEASURE_TYPE_2 = 0x02,
	AES_COUNTERMEASURE_TYPE_3 = 0x04,
	AES_COUNTERMEASURE_TYPE_4 = 0x08,
	AES_COUNTERMEASURE_TYPE_ALL = 0x0F,
};

/** AES interrupt source type */
typedef enum aes_interrupt_source {
	AES_INTERRUPT_INPUT_BUFFER_READY = AESA_IER_ODATARDY,
	AES_INTERRUPT_OUTPUT_DATA_READY = AESA_IER_IBUFRDY,
} aes_interrupt_source_t;

/**
 * \brief Interrupt callback function type for AES.
 *
 * The interrupt handler can be configured to do a function callback,
 * the callback function must match the aes_callback_t type.
 *
 */
typedef void (*aes_callback_t)(void);

/** AES Configuration structure. */
struct aes_config {
	/* 0=decryption data, 1=encryption data */
	enum aes_encrypt_mode encrypt_mode;
	/* 0 = 128bits, 1 = 192bits, 2 = 256bits */
	enum aes_key_size key_size;
	/* 0=Non-DMA mode, 1=DMA mode */
	enum aes_dma_mode dma_mode;
	/* 0 = ECB, 1 = CBC, 2 = OFB, 3 = CFB, 4 = CTR */
	enum aes_opmode opmode;
	/* 0 = 128bits, 1 = 64bits, 2 = 32bits, 3 = 16bits, 4 = 8bits */
	enum aes_cfb_size cfb_size;
	/* [0,15], bit=0 means CounterMeasure is disabled. */
	enum aes_countermeature_type countermeasure_mask;
};

/**
 * \brief AES driver software instance structure.
 *
 * Device instance structure for a AES driver instance. This
 * structure should be initialized by the \ref aes_init() function to
 * associate the instance with a particular hardware module of the device.
 */
struct aes_dev_inst {
	/** Base address of the AES module. */
	Aesa *hw_dev;
	/** Pointer to AES configuration structure. */
	struct aes_config  *aes_cfg;
};

void aes_get_config_defaults(struct aes_config *const cfg);

bool aes_init(struct aes_dev_inst *const dev_inst, Aesa *const aes,
		struct aes_config *const cfg);

/**
 * \brief Perform a software reset of the AES.
 *
 * \param dev_inst Device structure pointer.
 *
 */
static inline void aes_reset(struct aes_dev_inst *const dev_inst)
{
	dev_inst->hw_dev->AESA_CTRL |= AESA_CTRL_SWRST;
}

/**
 * \brief Notifies the module that the next input data block
 * is the beginning of a new message.
 *
 * \param dev_inst Device structure pointer.
 *
 */
static inline void aes_set_new_message(struct aes_dev_inst *const dev_inst)
{
	dev_inst->hw_dev->AESA_CTRL |= AESA_CTRL_NEWMSG;
}

/**
 * \brief Starts the computation of the last Nk words of the expanded key.
 *
 * \param dev_inst Device structure pointer.
 *
 */
static inline void aes_decryption_key_generate(
		struct aes_dev_inst *const dev_inst)
{
	dev_inst->hw_dev->AESA_CTRL |= AESA_CTRL_DKEYGEN;
}

void aes_enable(struct aes_dev_inst *const dev_inst);

void aes_disable(struct aes_dev_inst *const dev_inst);

/**
 * \brief Configure the AES.
 *
 * \param  dev_inst Device structure pointer.
 *
 */
static inline void aes_set_config(struct aes_dev_inst *const dev_inst)
{
	dev_inst->hw_dev->AESA_MODE = dev_inst->aes_cfg->encrypt_mode |
			AESA_MODE_KEYSIZE(dev_inst->aes_cfg->key_size) |
			(dev_inst->aes_cfg->dma_mode ? AESA_MODE_DMA : 0) |
			AESA_MODE_OPMODE(dev_inst->aes_cfg->opmode) |
			AESA_MODE_CFBS(dev_inst->aes_cfg->cfb_size) |
			AESA_MODE_CTYPE(dev_inst->aes_cfg->countermeasure_mask);
}

/**
 * \brief Write the input buffer pointer position.
 *
 * \param  dev_inst Device structure pointer.
 * \param  ul_in_position Input buffer pointer position.
 *
 */
static inline void aes_write_input_buffer_pointer(
		struct aes_dev_inst *const dev_inst, uint32_t ul_in_position)
{
	dev_inst->hw_dev->AESA_DATABUFPTR |=
			AESA_DATABUFPTR_IDATAW(ul_in_position);
}

/**
 * \brief Write the output buffer pointer position.
 *
 * \param  dev_inst Device structure pointer.
 * \param  ul_out_position Output buffer pointer position.
 *
 */
static inline void aes_write_output_buffer_pointer(
		struct aes_dev_inst *const dev_inst, uint32_t ul_out_position)
{
	dev_inst->hw_dev->AESA_DATABUFPTR |=
			AESA_DATABUFPTR_ODATAW(ul_out_position);
}

/**
 * \brief Read the input buffer pointer position.
 *
 * \param  dev_inst Device structure pointer.
 *
 * \return the input buffer pointer position.
 *
 */
static inline uint32_t aes_read_input_buffer_pointer(
		struct aes_dev_inst *const dev_inst)
{
	return ((dev_inst->hw_dev->AESA_DATABUFPTR & AESA_DATABUFPTR_IDATAW_Msk)
			>> AESA_DATABUFPTR_IDATAW_Pos);
}

/**
 * \brief Read the output buffer pointer position.
 *
 * \param  dev_inst Device structure pointer.
 *
 * \return the output buffer pointer position.
 *
 */
static inline uint32_t aes_read_output_buffer_pointer(
		struct aes_dev_inst *const dev_inst)
{
	return ((dev_inst->hw_dev->AESA_DATABUFPTR & AESA_DATABUFPTR_ODATAW_Msk)
			>> AESA_DATABUFPTR_ODATAW_Pos);
}

/**
 * \brief Get the AES status.
 *
 * \param  dev_inst Device structure pointer.
 *
 * \return the content of the status register.
 *
 */
static inline uint32_t aes_read_status(struct aes_dev_inst *const dev_inst)
{
	return dev_inst->hw_dev->AESA_SR;
}

void aes_set_callback(struct aes_dev_inst *const dev_inst,
		aes_interrupt_source_t source, aes_callback_t callback,
		uint8_t irq_level);

/**
 * \brief Enable the AES interrupt.
 *
 * \param  dev_inst Device structure pointer.
 * \param  source Interrupt source.
 *
 */
static inline void aes_enable_interrupt(struct aes_dev_inst *const dev_inst,
		aes_interrupt_source_t source)
{
	dev_inst->hw_dev->AESA_IER = (uint32_t)source;
}

/**
 * \brief Disable the AES interrupt.
 *
 * \param  dev_inst Device structure pointer.
 * \param  source Interrupt source.
 *
 */
static inline void aes_disable_interrupt(
		struct aes_dev_inst *const dev_inst, aes_interrupt_source_t source)
{
	dev_inst->hw_dev->AESA_IDR = (uint32_t)source;
}

/**
 * \brief Get the AES interrupt mask status.
 *
 * \param  dev_inst Device structure pointer.
 *
 * \return the content of the interrupt mask register.
 *
 */
static inline uint32_t aes_read_interrupt_mask(
		struct aes_dev_inst *const dev_inst)
{
	return dev_inst->hw_dev->AESA_IMR;
}

void aes_write_key(struct aes_dev_inst *const dev_inst,
		const uint32_t *p_key);

void aes_write_initvector(struct aes_dev_inst *const dev_inst,
		const uint32_t *p_vector);

/**
 * \brief Write the input data.
 *
 * \param  dev_inst Device structure pointer.
 * \param  ul_data Input data.
 *
 */
static inline void aes_write_input_data(
		struct aes_dev_inst *const dev_inst, uint32_t ul_data)
{
	dev_inst->hw_dev->AESA_IDATA = ul_data;
}

/**
 * \brief Read the output data.
 *
 * \param  dev_inst Device structure pointer.
 *
 * \return  the output data.
 *
 */
static inline uint32_t aes_read_output_data(
		struct aes_dev_inst *const dev_inst)
{
	return dev_inst->hw_dev->AESA_ODATA;
}

/**
 * \brief Write the DRNG seed.
 *
 * \param  dev_inst Device structure pointer.
 * \param  ul_drng_seed DRNG seed.
 *
 */
static inline void aes_write_drng_seed(struct aes_dev_inst *const dev_inst,
		uint32_t ul_drng_seed)
{
	dev_inst->hw_dev->AESA_DRNGSEED = ul_drng_seed;
}

/**
 * \}
 */

/**
 * \page sam_aes_quick_start Quick Start Guide for the AES driver
 *
 * This is the quick start guide for the \ref group_sam_drivers_aes, with
 * step-by-step instructions on how to configure and use the driver for
 * a specific use case.The code examples can be copied into e.g the main
 * application loop or any other function that will need to control the
 * AES module.
 *
 * \section aes_qs_use_cases Use cases
 * - \ref aes_basic
 *
 * \section aes_basic AES basic usage
 *
 * This use case will demonstrate how to initialize the AES module to
 * encryption or decryption data.
 *
 *
 * \section aes_basic_setup Setup steps
 *
 * \subsection aes_basic_prereq Prerequisites
 *
 * This module requires the following service
 * - \ref clk_group
 *
 * \subsection aes_basic_setup_code
 *
 * Add this to the main loop or a setup function:
 * \code
 * struct aes_dev_inst g_aes_inst;
 * struct aes_config   g_aes_cfg;
 * aes_get_config_defaults(&g_aes_cfg);
 * aes_init(&g_aes_inst, AESA, &g_aes_cfg);
 * aes_enable(&g_aes_inst);
 * \endcode
 *
 * \subsection aes_basic_setup_workflow
 *
 * -# Enable the AES module
 *  - \code aes_enable(&g_aes_inst); \endcode
 *
 * -# Set the AES interrupt and callback
 * \code
 * aes_set_callback(&g_aes_inst, AES_INTERRUPT_INPUT_BUFFER_READY,
 *		aes_callback, 1);
 * \endcode
 *
 * -# Initialize the AES to ECB cipher mode
 * \code
 * g_aes_inst.aes_cfg->encrypt_mode = AES_ENCRYPTION;
 * g_aes_inst.aes_cfg->key_size = AES_KEY_SIZE_128;
 * g_aes_inst.aes_cfg->dma_mode = AES_MANUAL_MODE;
 * g_aes_inst.aes_cfg->opmode = AES_ECB_MODE;
 * g_aes_inst.aes_cfg->cfb_size = AES_CFB_SIZE_128;
 * g_aes_inst.aes_cfg->countermeasure_mask = 0xF;
 * aes_set_config(&g_aes_inst);
 * \endcode
 *
 * \section aes_basic_usage Usage steps
 *
 * \subsection aes_basic_usage_code
 *
 * We can then encrypte the plain text by
 * \code
 * aes_set_new_message(&g_aes_inst);
 * aes_write_key(&g_aes_inst, key128);
 * aes_write_input_data(&g_aes_inst, ref_plain_text[0]);
 * aes_write_input_data(&g_aes_inst, ref_plain_text[1]);
 * aes_write_input_data(&g_aes_inst, ref_plain_text[2]);
 * aes_write_input_data(&g_aes_inst, ref_plain_text[3]);
 * \endcode
 *
 * We can get the cipher text after it's ready by
 * \code
 * output_data[0] = aes_read_output_data(&g_aes_inst);
 * output_data[1] = aes_read_output_data(&g_aes_inst);
 * output_data[2] = aes_read_output_data(&g_aes_inst);
 * output_data[3] = aes_read_output_data(&g_aes_inst);
 * \endcode
 */

#endif  /* AES_H_INCLUDED */
