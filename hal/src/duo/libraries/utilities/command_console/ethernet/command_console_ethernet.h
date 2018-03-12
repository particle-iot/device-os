/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 *
 * Ethernet Testing for Console Application
 *
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif


/******************************************************
 *                     Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/
#define ETHERNET_COMMANDS \
    { (char*) "ethernet_up",         ethernet_up,       0, NULL, NULL, "[ip netmask gateway]", (char*) "Brings up the Ethernet. DHCP assumed if no IP address provided"}, \
    { (char*) "ethernet_down",       ethernet_down,     0, NULL, NULL, NULL, (char*) "Brings down the ethernet"}, \
    { (char*) "ethernet_ping",       ethernet_ping,     0, NULL, NULL, (char*) "<destination> [-i <interval in ms>] [-n <number>] [-l <length>]", (char*) "Pings the specified IP or Host via Ethernet."}, \
    { (char*) "network_suspend",     network_suspend,   0, NULL, NULL, NULL, (char*) "Will suspend network"}, \
    { (char*) "network_resume",      network_resume,    0, NULL, NULL, NULL, (char*) "Will resume network"},

#define PING_THREAD_STACK_SIZE (2500)
#define PING_DESCRIPTION_LEN    (200)
#define PING_HISTORY_LEN          (5)
#define PING_RESULT_LEN          (30)

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
int ethernet_up( int argc, char* argv[] );
int ethernet_down( int argc, char* argv[] );
int ethernet_ping( int argc, char* argv[] );
int network_suspend( int argc, char* argv[] );
int network_resume( int argc, char* argv[] );

#ifdef __cplusplus
}   /*extern "C"    */
#endif
