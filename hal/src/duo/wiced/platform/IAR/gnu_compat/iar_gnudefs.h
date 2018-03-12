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
 * This header include misc platform independent GNU
 * definitions for IAR.
 */

/* Note similar effort has been done in "bcmtypes.h"
 * for specific platform. This file, on the other hand,
 * is intended to be shared across platforms.
 * */
#if !defined(_IAR_GNUDEFS_H) && !defined(BCMTYPES_H)
#define _IAR_GNUDEFS_H


#ifdef    __cplusplus
extern "C" {
#endif /* __cplusplus */

/******************************************************
 *           Defs from <sys/type.h>
 ******************************************************/
typedef unsigned long useconds_t;
typedef long ssize_t;
typedef unsigned long    ulong;


/******************************************************
 *           Defs from <errno.h>
 ******************************************************/
#define EINTR 4     /* Interrupted system call */


#ifdef    __cplusplus
} /* end extern "C" */
#endif /* __cplusplus */

#endif /* _IAR_GNUDEFS_H */

