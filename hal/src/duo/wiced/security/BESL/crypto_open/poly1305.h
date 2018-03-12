/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/*
 * From https://github.com/floodyberry/poly1305-donna
 * License: "MIT or PUBLIC DOMAIN"
 */

#ifndef POLY1305_DONNA_H
#define POLY1305_DONNA_H

#include "crypto_structures.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define POLY1305_KEYLEN 32
#define POLY1305_TAGLEN 16

/**
 *  Get the poly1305 Message-Authentication code for a buffer of message data
 *  The Key MUST ONLY BE USED ONCE ONLY
 *  The sender MUST NOT use poly1305_auth to authenticate
 *  more than one message under the same key. Authenticators
 *  for two messages under the same key should be expected
 *  to reveal enough information to allow forgeries of
 *  authenticators on other messages.
 *
 *  @param mac           the output message-authentication code
 *  @param message_data  the message data to be processed through the authenticator
 *  @param bytes         number of bytes of message data to read
 *  @param key           the UNIQUE 32 byte key to be used for this session
 */
void poly1305_auth   ( unsigned char mac[16], const unsigned char *message_data, size_t bytes, const unsigned char key[32]);



/*  Use these functions for processing non-contiguous data */

/**
 *  Initialise a poly1305 session with a key.
 *  The Key MUST ONLY BE USED ONCE ONLY
 *  The sender MUST NOT use poly1305_auth to authenticate
 *  more than one message under the same key. Authenticators
 *  for two messages under the same key should be expected
 *  to reveal enough information to allow forgeries of
 *  authenticators on other messages.
 *
 *  @param context a poly1305_context which will be used for scratch space
 *                 until poly1305_finish is called
 *  @param key     the UNIQUE 32 byte key to be used for this session
 */
void poly1305_init   ( poly1305_context *context, const unsigned char key[32]);

/**
 *  Process message data through poly1305 authenticator
 *
 *  @param context       a poly1305_context which has been initialised with poly1305_init
 *                       and will be used for scratch space until poly1305_finish is called
 *  @param message_data  the message data to be processed through the authenticator
 *  @param bytes         number of bytes of message data to read
 */
void poly1305_update ( poly1305_context *context, const unsigned char *message_data, size_t bytes);

/**
 *  Finish processing a poly1305 authenticator session
 *
 *  @param context       a poly1305_context which has been filled via poly1305_init and
 *                       poly1305_update. Will not be be used subsequently.
 *  @param mac           the output message-authentication code
 */
void poly1305_finish ( poly1305_context *context, unsigned char mac[16]);


/**
 *  Verify that two Message-Authentication Codes match
 *
 *  @param mac1  the second message-authentication code
 *  @param mac2  the second message-authentication code
 *
 *  @return  1 if mac1 matches mac2,    0 otherwise
 */
int poly1305_verify(const unsigned char mac1[16], const unsigned char mac2[16]);


/**
 *  Run tests on the poly1305 algorithm
 *
 *  @return  1 if successful,    0 otherwise
 */
int poly1305_power_on_self_test(void);

/**
 *  Run TLS tests on the poly1305 algorithm
 *
 *  @return  0 if successful
 */
int test_poly1305_tls( void );


#ifdef __cplusplus
} /*extern "C" */
#endif

#endif /* POLY1305_DONNA_H */

