/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include "sha4.h"

#define ed25519_hash_init(context)        sha4_starts(context,0)
#define ed25519_hash_update               sha4_update
#define ed25519_hash_final                sha4_finish
#define ed25519_hash(output,input,ilen)   sha4((input),(ilen),(output),0)
#define ed25519_hash_context              sha4_context

#ifdef __cplusplus
} /*extern "C" */
#endif
