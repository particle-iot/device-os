/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#ifndef INCLUDED_WWD_LOGGING_H_
#define INCLUDED_WWD_LOGGING_H_

#ifdef __cplusplus
extern "C"
{
#endif

/*
#define WWD_LOGGING_STDOUT_ENABLE
*/
/*
#define WWD_LOGGING_BUFFER_ENABLE
*/

#if defined( WWD_LOGGING_STDOUT_ENABLE )

#include <stdio.h>
extern int wwd_logging_enabled;
#define WWD_LOG( x ) if (wwd_logging_enabled) {printf x; }

#elif defined( WWD_LOGGING_BUFFER_ENABLE )
extern int wwd_logging_enabled;

extern int wwd_logging_printf(const char *format, ...);

#define WWD_LOG( x ) if (wwd_logging_enabled) {wwd_logging_printf x; }


#else /* if defined( WWD_LOGGING_BUFFER_ENABLE ) */

#define WWD_LOG(x)

#endif /* if defined( WWD_LOGGING_BUFFER_ENABLE ) */


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ifndef INCLUDED_WWD_LOGGING_H_ */
