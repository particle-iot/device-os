/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

#include "dtls_types.h"
#include "wiced_result.h"

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

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/*****************************************************************************/
/** Initialises a simple DTLS context handle
 *
 * @param[out] context     : A pointer to a wiced_dtls_context_t context object that will be initialised
 * @param[out] identity    : A pointer to a wiced_tls_identity_t object that will be initialised
 *
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_dtls_init_context( wiced_dtls_context_t* context, wiced_dtls_identity_t* identity, const char* peer_cn );


/** Initialises a DTLS identity using a supplied certificate and private key
 *
 * @param[out] identity          : A pointer to a wiced_tls_identity_t object that will be initialised
 * @param[in] private_key        : The server private key in binary format
 * @param[in] certificate_data   : The server x509 certificate in PEM or DER format
 * @param[in] certificate_length : The length of the certificate
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_dtls_init_identity( wiced_dtls_identity_t* identity, const char* private_key, const uint32_t key_length, const uint8_t* certificate_data, uint32_t certificate_length );

/** DeiInitialises a DTLS identity
 *
 * @param[in] identity    : A pointer to a wiced_dtls_identity_t object that will be de-initialised
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_dtls_deinit_identity( wiced_dtls_identity_t* dtls_identity);

/** De-initialise a previously inited DTLS context
 *
 * @param[in,out] context : a pointer to a wiced_dtls_context_t object
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_dtls_deinit_context( wiced_dtls_context_t* context );

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif
