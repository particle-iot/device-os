/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
/**
 * This file defines trace functions accessible by the console.
 */
#pragma once

#include "trace.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define TRACE_COMMANDS \
    { (char*) "trace",         console_trace,         0, NULL, NULL, (char*) "trace_type trace_process_type <cmds>", (char*) "Traces the execution of the specified commands, storing the information in a buffer."}, \
    { (char*) "start_trace",   console_start_trace,   0, NULL, NULL, (char*) "trace_type",                           (char*) "Starts tracing scheduler actions."}, \
    { (char*) "stop_trace",    console_end_trace,     0, NULL, NULL, NULL,                                           (char*) "Pauses tracing scheduler actions."}, \

#ifdef __cplusplus
} /*extern "C" */
#endif
