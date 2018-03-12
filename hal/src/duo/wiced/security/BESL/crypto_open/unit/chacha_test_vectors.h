/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#ifndef INCLUDED_CHACHA_TEST_VECTORS_H_
#define INCLUDED_CHACHA_TEST_VECTORS_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct
{
    char*    description;
    uint8_t  key[256];
    uint32_t key_len;
    uint8_t  iv[8];
    uint8_t  rounds;
    uint8_t  keystream_block[64];
    uint8_t  keystream_block_1[64];
} chacha_vector_t;

extern const chacha_vector_t chacha_test_vectors[];
extern const int chacha_test_vectors_count;

#ifdef __cplusplus
}
#endif

#endif /* ifndef INCLUDED_CHACHA_TEST_VECTORS_H_ */
