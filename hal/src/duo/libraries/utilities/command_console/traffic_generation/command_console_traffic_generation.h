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

extern int traffic_stream_ipv4( int argc, char *argv[] );

#define TRAFFIC_GENERATION_COMMANDS \
    { (char*) "traffic_stream_ipv4", traffic_stream_ipv4,  0, NULL, NULL, (char*) "[-c <destination ip>] [-p <port>] [-u (for udp)] [-l <length>] [-d <duration in seconds>] [-r <rate (pps)>] [-S <type of service>] [-i <interface>]", (char*) "Start a traffic stream."},


#ifdef __cplusplus
} /*extern "C" */
#endif
