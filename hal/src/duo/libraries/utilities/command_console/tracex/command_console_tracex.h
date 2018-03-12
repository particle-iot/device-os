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
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#if defined(TX_ENABLE_EVENT_TRACE)
#include "TraceX.h"
#endif

/******************************************************
 *                     Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#if defined(TX_ENABLE_EVENT_TRACE)
#define TRACEX_COMMANDS \
    { (char*) "tracex_enable",  command_console_tracex_enable,  0, NULL, NULL, (char*) "[-l] [-t <ip_addr>] [-p <port>] [-d <len>]",                                      (char*) "Enable TraceX; -l to enable loop-recording; -t, -p, and -d to specify TCP server IP address, port, and max data packet length for sending the TraceX buffer."}, \
    { (char*) "tracex_disable", command_console_tracex_disable, 0, NULL, NULL, NULL,                                                                                      (char*) "Disable TraceX and send current event buffer trace to the TCP server if specified."}, \
    { (char*) "tracex_restart", command_console_tracex_restart, 0, NULL, NULL, NULL,                                                                                      (char*) "Restart TraceX with same configuration."}, \
    { (char*) "tracex_status",  command_console_tracex_status,  0, NULL, NULL, NULL,                                                                                      (char*) "Print status of TraceX."}, \
    { (char*) "tracex_filter",  command_console_tracex_filter,  0, NULL, NULL, (char*) "[-l] [-a] [-c] [-d] [-f <filters...>] [-F <mask>] [-u <filters...>] [-U <mask>]", (char*) "Print/modify TraceX filters: no args to print current filters; -l to list all known filters; -a to filter out everything; -c to unfilter everything; -d to use default filter; -f/-u to filter/unfilter out by comma-delimited filter labels; -F/-U to filter/unfilter by filter masks"}, \
    { (char*) "tracex_send",    command_console_tracex_send,    0, NULL, NULL, (char*) "[-t <ip_addr>] [-p <port>] [-d <len>]",                                           (char*) "Send TraceX buffer to TCP server (TraceX must be disabled): no args to use current TCP server configuration; -t, -p, and -d to specify TCP server IP address, port, and max data packet length for sending the TraceX buffer."}, \
    { (char*) "tracex_test",    command_console_tracex_test,    1, NULL, NULL, (char*) "-n <num> [-i <id>]",                                                              (char*) "Test TraceX: -n to specify number of events to insert; -i to specify event ID (default is 4096)."},
#else
#define TRACEX_COMMANDS
#endif

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

#if defined(TX_ENABLE_EVENT_TRACE)
int command_console_tracex_enable(int argc, char* argv[]);
int command_console_tracex_disable(int argc, char* argv[]);
int command_console_tracex_restart(int argc, char* argv[]);
int command_console_tracex_status(int argc, char* argv[]);
int command_console_tracex_filter(int argc, char* argv[]);
int command_console_tracex_send(int argc, char* argv[]);
int command_console_tracex_test(int argc, char* argv[]);
#endif

#ifdef __cplusplus
} /*extern "C" */
#endif
