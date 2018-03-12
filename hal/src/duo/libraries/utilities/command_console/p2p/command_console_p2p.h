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

/******************************************************
 *                     Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define P2P_COMMANDS \
    { (char*) "p2p_connect",                 p2p_connect,                 0,  NULL, NULL, (char*) "<pin|pbc> [8 digit PIN]",                   (char*) "Connect to an existing Group Owner or negotiate to form a group"}, \
    { (char*) "p2p_client_test",             p2p_client_test,             0,  NULL, NULL, (char*) "<SSID> <8 digit PIN> [Iterations]",         (char*) "Test P2P client connection to existing group owner"}, \
    { (char*) "p2p_discovery_disable",       p2p_discovery_disable,       0,  NULL, NULL, (char*) "",                                          (char*) "Disable P2P discovery mode"}, \
    { (char*) "p2p_discovery_enable",        p2p_discovery_enable,        0,  NULL, NULL, (char*) "",                                          (char*) "Enable P2P discovery mode"}, \
    { (char*) "p2p_discovery_test",          p2p_discovery_test,          0,  NULL, NULL, (char*) "[iterations]",                              (char*) "Test enable/disable of P2P discovery mode"}, \
    { (char*) "p2p_go_start",                p2p_go_start,                0,  NULL, NULL, (char*) "<p|n> [<ssid_suffix> <key> <operating channel>]", (char*) "Start P2P (p)ersistent or (n)on-persistent group"}, \
    { (char*) "p2p_go_stop",                 p2p_go_stop,                 0,  NULL, NULL, (char*) "",                                          (char*) "Stop P2P group owner"}, \
    { (char*) "p2p_go_test",                 p2p_go_test,                 0,  NULL, NULL, (char*) "<p|n> [iterations]",                        (char*) "Test start/stop of (p)ersistent or (n)on-persistent group"}, \
    { (char*) "p2p_go_client_test_mode",     p2p_go_client_test_mode,     0,  NULL, NULL, (char*) "<enable=1|disable=0> <pin value>",          (char*) "Allow test client to join using PIN value"}, \
    { (char*) "p2p_leave",                   p2p_leave,                   0,  NULL, NULL, (char*) "",                                          (char*) "Disassociate from P2P group owner"}, \
    { (char*) "p2p_negotiation_test",        p2p_negotiation_test,        0,  NULL, NULL, (char*) "<8 digit PIN> [Iterations]",                (char*) "Test P2P negotiation handshake"}, \
    { (char*) "p2p_peer_list",               p2p_peer_list,               0,  NULL, NULL, (char*) "",                                          (char*) "Print P2P peer list"}, \
    { (char*) "p2p_registrar_start",         p2p_registrar_start,         0,  NULL, NULL, (char*) "<pin|pbc> [8 digit PIN]",                   (char*) "Start P2P group owner's WPS registrar"}, \
    { (char*) "p2p_registrar_stop",          p2p_registrar_stop,          0,  NULL, NULL, (char*) "",                                          (char*) "Stop P2P group owner's WPS registrar"}, \
    { (char*) "p2p_set_go_intent",           p2p_set_go_intent,           0,  NULL, NULL, (char*) "<intent value>",                            (char*) "Set P2P group owner intent (0..15 where 15 = must be GO)"}, \
    { (char*) "p2p_set_go_pbc_mode_support", p2p_set_go_pbc_mode_support, 0,  NULL, NULL, (char*) "<0|1>",                                     (char*) "Set P2P group owner support for PBC mode (1 = allow, 0 = disallow)"}, \
    { (char*) "p2p_set_listen_channel",      p2p_set_listen_channel,      0,  NULL, NULL, (char*) "<channel>",                                 (char*) "Set listen channel (1, 6 or 11)"}, \
    { (char*) "p2p_set_operating_channel",   p2p_set_operating_channel,   0,  NULL, NULL, (char*) "<channel>",                                 (char*) "Set operating channel (1..11)"}, \

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

extern int p2p_connect                ( int argc, char* argv[] );
extern int p2p_discovery_disable      ( int argc, char* argv[] );
extern int p2p_discovery_enable       ( int argc, char* argv[] );
extern int p2p_discovery_test         ( int argc, char* argv[] );
extern int p2p_go_start               ( int argc, char* argv[] );
extern int p2p_go_stop                ( int argc, char* argv[] );
extern int p2p_go_test                ( int argc, char* argv[] );
extern int p2p_peer_list              ( int argc, char* argv[] );
extern int p2p_registrar_start        ( int argc, char* argv[] );
extern int p2p_registrar_stop         ( int argc, char* argv[] );
extern int p2p_leave                  ( int argc, char* argv[] );
extern int p2p_client_test            ( int argc, char* argv[] );
extern int p2p_go_client_test_mode    ( int argc, char* argv[] );
extern int p2p_set_go_intent          ( int argc, char* argv[] );
extern int p2p_set_listen_channel     ( int argc, char* argv[] );
extern int p2p_set_operating_channel  ( int argc, char* argv[] );
extern int p2p_set_go_pbc_mode_support( int argc, char* argv[] );
extern int p2p_negotiation_test( int argc, char* argv[] );


#ifdef __cplusplus
} /*extern "C" */
#endif
