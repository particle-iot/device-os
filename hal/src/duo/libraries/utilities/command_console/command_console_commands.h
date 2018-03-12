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

#include "command_console_ping.h"
#include "command_console_wps.h"
#include "command_console.h"
#include "console_wl.h"
#include "wiced_management.h"
#include "command_console_wifi.h"
#include "command_console_mallinfo.h"
#include "console_iperf.h"
#include "command_console_thread.h"
#include "command_console_platform.h"
#include "command_console_tracex.h"

#ifdef CONSOLE_INCLUDE_P2P
#include "command_console_p2p.h"
#endif

#ifdef CONSOLE_INCLUDE_ETHERNET
#include "command_console_ethernet.h"
#endif

#ifdef CONSOLE_ENABLE_WL
#include "console_wl.h"
#endif

#ifdef CONSOLE_INCLUDE_TRAFFIC_GENERATION
#include "command_console_traffic_generation.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

#define MAX_LINE_LENGTH    (128)
#define MAX_HISTORY_LENGTH (20)

#ifdef CONSOLE_INCLUDE_P2P
#define ALL_COMMANDS_P2P_COMMANDS                P2P_COMMANDS
#else
#define ALL_COMMANDS_P2P_COMMANDS
#endif

#ifdef CONSOLE_INCLUDE_ETHERNET
#define ALL_COMMANDS_ETHERNET_COMMANDS           ETHERNET_COMMANDS
#else
#define ALL_COMMANDS_ETHERNET_COMMANDS
#endif

#ifdef CONSOLE_ENABLE_WL
#define ALL_COMMANDS_WL_COMMANDS                 WL_COMMANDS
#else
#define ALL_COMMANDS_WL_COMMANDS
#endif

#ifdef CONSOLE_INCLUDE_TRAFFIC_GENERATION
#define ALL_COMMANDS_TRAFFIC_GENERATION_COMMANDS TRAFFIC_GENERATION_COMMANDS
#else
#define ALL_COMMANDS_TRAFFIC_GENERATION_COMMANDS
#endif


#define ALL_COMMANDS                         \
    WIFI_COMMANDS                            \
    IPERF_COMMANDS                           \
    MALLINFO_COMMANDS                        \
    PING_COMMANDS                            \
    PLATFORM_COMMANDS                        \
    THREAD_COMMANDS                          \
    WPS_COMMANDS                             \
    TRACEX_COMMANDS                          \
    ALL_COMMANDS_P2P_COMMANDS                \
    ALL_COMMANDS_ETHERNET_COMMANDS           \
    ALL_COMMANDS_WL_COMMANDS                 \
    ALL_COMMANDS_TRAFFIC_GENERATION_COMMANDS \

#ifdef __cplusplus
}
#endif
