/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#ifndef INCLUDED_IPERF_H_
#define INCLUDED_IPERF_H_

#ifdef CONSOLE_ENABLE_IPERF

#ifdef __cplusplus
extern "C"
{
#endif


extern int iperf( int argc, char *argv[] );

#define IPERF_COMMANDS \
    { "iperf", iperf, 0, NULL, NULL, NULL, "Run iperf --help for usage."}, \

#else /* ifdef CONSOLE_ENABLE_IPERF */
#define IPERF_COMMANDS
#endif /* ifdef CONSOLE_ENABLE_IPERF */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ifndef INCLUDED_IPERF_H_ */
