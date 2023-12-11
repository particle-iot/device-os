/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#undef LOG_COMPILE_TIME_LEVEL
#define LOG_COMPILE_TIME_LEVEL LOG_LEVEL_ALL

#include "lwip_logging.h"
#include <cstring>
#include "logging.h"
#include "lwipopts.h"

#if defined(PPP_SUPPORT) && PPP_SUPPORT
#ifndef PPPDEBUG_H
/* we do not want this header, as it redefines our LOG_XXX macros */
#define PPPDEBUG_H
#endif /* PPPDEBUG_H */
extern "C" {
#include <netif/ppp/ppp_impl.h>
}
#endif // defined(PPP_SUPPORT) && PPP_SUPPORT

uint32_t g_lwip_debug_flags = 0xffffffff;

void lwip_log_message(const char* fmt, ...) {
    LogAttributes attr = {};
    va_list args;
    va_start(args, fmt);

    char tmp[LOG_MAX_STRING_LENGTH] = {};
    strncpy(tmp, fmt, sizeof(tmp));
    tmp[strcspn(tmp, "\r\n")] = 0;

    log_message_v(1, "lwip", &attr, nullptr /* reserved */, tmp, args);
    va_end(args);
}

#if defined(PPP_SUPPORT) && PPP_SUPPORT && defined(PRINTPKT_SUPPORT) && PRINTPKT_SUPPORT

void ppp_dbglog(const char *fmt, ...) {
    // Only enable PPP negotiation packet logs
    if (strstr(fmt, "%P") == nullptr) {
        return;
    }

    va_list args;

    va_start(args, fmt);
    char tmp[LOG_MAX_STRING_LENGTH] = {};
    ppp_vslprintf(tmp, sizeof(tmp) - 1, fmt, args);
    va_end(args);
    tmp[strcspn(tmp, "\r\n")] = 0;

    LogAttributes attr = {};
    log_message(LOG_LEVEL_TRACE, "lwip.ppp", &attr, nullptr /* reserved */, tmp);
}
#else

void ppp_dbglog(const char* fmt, ...) {
}

#endif // defined(PPP_SUPPORT) && PPP_SUPPORT && defined(PRINTPKT_SUPPORT) && PRINTPKT_SUPPORT

void ppp_fatal(const char *fmt, ...) {
}

void ppp_error(const char *fmt, ...) {
}

void ppp_warn(const char *fmt, ...) {
}

void ppp_notice(const char *fmt, ...) {
}

void ppp_info(const char *fmt, ...) {
}
