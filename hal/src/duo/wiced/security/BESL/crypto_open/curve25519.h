/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/**
 * @file
 *
 * Curve25519 elliptic curve key generation functions
 * See http://cr.yp.to/ecdh.html
 *
 */

#ifndef INCLUDED_CURVE25519_H_
#define INCLUDED_CURVE25519_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Calculate a public (shared) key given a basepoint and secret key
 *
 * @param mypublic_output Receives the 32 byte output shared key
 * @param secret          The 32 byte secret key. Must have been randomly
 *                        generated then have the following operations performed
 *                        on it:
 *                               secret[0]  &= 248;
 *                               secret[31] &= 127;
 *                               secret[31] |= 64;
 * @param basepoint       The starting point for the calculation - usually the
 *                        public key of the other party
 *
 * @return 0 when successful
 */
int curve25519( uint8_t *mypublic_output, const uint8_t *secret, const uint8_t *basepoint );

#ifdef __cplusplus
}
#endif

#endif /* ifndef INCLUDED_CURVE25519_H_ */
