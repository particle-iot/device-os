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

#include "wiced_result.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                     Macros
 ******************************************************/

#define WPS_COMMANDS \
    { (char*) "force_alignment",    force_alignment,   0, NULL, NULL, (char*) "",                                           (char*) "Force aligned memory accesses"}, \
    { (char*) "join_wps",           join_wps,          1, NULL, NULL, (char*) "<pbc|pin> [pin] [<ip> <netmask> <gateway>]", (char*) "Join an AP using WPS"}, \
    { (char*) "join_wps_specific",  join_wps_specific, 0, NULL, NULL, (char*) "[pin] [<ip> <netmask> <gateway>]",           (char*) "Join a specific WLAN using WPS PIN mode"}, \
    { (char*) "scan_wps",           scan_wps,          0, NULL, NULL, (char*) "",                                           (char*) "Scan for APs supporting WPS"}, \
    { (char*) "start_registrar",    start_registrar,   1, NULL, NULL, (char*) "<pbc|pin> [pin]",                            (char*) "Start the WPS Registrar"}, \
    { (char*) "stop_registrar",     stop_registrar,    0, NULL, NULL, (char*) "",                                           (char*) "Stop the WPS Registrar"}, \

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

int join_wps( int argc, char* argv[] );
int start_registrar( int argc, char* argv[] );
int force_alignment( int argc, char* argv[] );
int stop_registrar( int argc, char* argv[] );
wiced_result_t enable_ap_registrar_events( void );
void disable_ap_registrar_events( void );
int scan_wps( int argc, char* argv[] );
int join_wps_specific( int argc, char* argv[] );

/* Function used by other modules */
void dehyphenate_pin(char* str );

#ifdef __cplusplus
} /*extern "C" */
#endif
