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

#ifdef CONSOLE_ENABLE_WL

#ifdef __cplusplus
extern "C"
{
#endif


extern int wlu_main_args( int argc, char *argv[] );

#define WL_COMMANDS \
        { (char*) "wl",     wlu_main_args, 0, NULL, NULL, (char *) "[<command> [option...]]", "wl commands"},

#else /* ifdef CONSOLE_ENABLE_WL */
#define WL_COMMANDS
#endif /* ifdef CONSOLE_ENABLE_WL */

#ifdef __cplusplus
} /* extern "C" */
#endif
