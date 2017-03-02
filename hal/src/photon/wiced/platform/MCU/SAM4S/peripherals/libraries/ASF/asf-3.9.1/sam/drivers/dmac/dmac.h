/**
 * \file
 *
 * \brief DMA Controller (DMAC) driver for SAM.
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

#ifndef DMAC_H_INCLUDED
#define DMAC_H_INCLUDED

#include  "compiler.h"

/** @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
extern "C" {
#endif
/**INDENT-ON**/
/** @endcond */

/**
 * \defgroup sam_driver_dmac_group DMA Controller (DMAC) Driver
 *
 * \par Purpose
 *
 * The DMA Controller (DMAC) is an AHB-central DMA controller core that
 * transfers data from a source peripheral to a destination peripheral
 * over one or more AMBA buses. This is a driver for configuration, enabling,
 * disabling and use of the DMAC peripheral.
 *
 * @{
 */

/** \brief DMAC priority mode */
typedef enum {
#if (SAM3U)
	DMAC_PRIORITY_FIXED       = 0,
	DMAC_PRIORITY_ROUND_ROBIN = DMAC_GCFG_ARB_CFG
#else
	DMAC_PRIORITY_FIXED       = DMAC_GCFG_ARB_CFG_FIXED,
	DMAC_PRIORITY_ROUND_ROBIN = DMAC_GCFG_ARB_CFG_ROUND_ROBIN
#endif
} dmac_priority_mode_t;

/** \brief DMA transfer descriptor, otherwise known as Linked List Item (LLI) */
typedef struct {
	uint32_t ul_source_addr;      /**< Source buffer address */
	uint32_t ul_destination_addr; /**< Destination buffer address */
	uint32_t ul_ctrlA;            /**< Control A register settings */
	uint32_t ul_ctrlB;            /**< Control B register settings */
	uint32_t ul_descriptor_addr;  /**< Next descriptor address */
} dma_transfer_descriptor_t;

#define DMA_MAX_LENGTH 0xFFFu

void dmac_init(Dmac *p_dmac);
void dmac_set_priority_mode(Dmac *p_dmac, dmac_priority_mode_t mode);
void dmac_enable(Dmac *p_dmac);
void dmac_disable(Dmac *p_dmac);
void dmac_enable_interrupt(Dmac *p_dmac, uint32_t ul_mask);
void dmac_disable_interrupt(Dmac *p_dmac, uint32_t ul_mask);
uint32_t dmac_get_interrupt_mask(Dmac *p_dmac);
uint32_t dmac_get_status(Dmac *p_dmac);

void dmac_channel_enable(Dmac *p_dmac, uint32_t ul_num);
void dmac_channel_disable(Dmac *p_dmac, uint32_t ul_num);
uint32_t dmac_channel_is_enable(Dmac *p_dmac, uint32_t ul_num);
void dmac_channel_suspend(Dmac *p_dmac, uint32_t ul_num);
void dmac_channel_resume(Dmac *p_dmac, uint32_t ul_num);
void dmac_channel_keep(Dmac *p_dmac, uint32_t ul_num);
uint32_t dmac_channel_get_status(Dmac *p_dmac);
void dmac_channel_set_source_addr(Dmac *p_dmac,
		uint32_t ul_num, uint32_t ul_addr);
void dmac_channel_set_destination_addr(Dmac *p_dmac,
		uint32_t ul_num, uint32_t ul_addr);
void dmac_channel_set_descriptor_addr(Dmac *p_dmac,
		uint32_t ul_num, uint32_t ul_desc);
void dmac_channel_set_ctrlA(Dmac *p_dmac, uint32_t ul_num, uint32_t ul_ctrlA);
void dmac_channel_set_ctrlB(Dmac *p_dmac, uint32_t ul_num, uint32_t ul_ctrlB);
void dmac_channel_set_configuration(Dmac *p_dmac, uint32_t ul_num,
		uint32_t ul_cfg);
void dmac_channel_single_buf_transfer_init(Dmac *p_dmac,
		uint32_t ul_num, dma_transfer_descriptor_t *p_desc);
void dmac_channel_multi_buf_transfer_init(Dmac *p_dmac,
		uint32_t ul_num, dma_transfer_descriptor_t *p_desc);
void dmac_channel_stop_transfer(Dmac *p_dmac, uint32_t ul_num);
uint32_t dmac_channel_is_transfer_done(Dmac *p_dmac, uint32_t ul_num);

void dmac_soft_single_transfer_request(Dmac *p_dmac,
		uint32_t ul_num, uint32_t ul_src_req, uint32_t ul_dst_req);
void dmac_soft_chunk_transfer_request(Dmac *p_dmac,
		uint32_t ul_num, uint32_t ul_src_req, uint32_t ul_dst_req);
void dmac_soft_set_last_transfer_flag(Dmac *p_dmac,
		uint32_t ul_num, uint32_t ul_src_flag, uint32_t ul_dst_flag);

#if (SAM3XA || SAM4E)
void dmac_set_writeprotect(Dmac *p_dmac, uint32_t ul_enable);
uint32_t dmac_get_writeprotect_status(Dmac *p_dmac);
#endif

/** @} */

/** @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/** @endcond */

#endif /* DMAC_H_INCLUDED */
