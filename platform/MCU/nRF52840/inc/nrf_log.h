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

#ifndef NRF_LOG_H
#define NRF_LOG_H

#include "nrfx_log.h"

#ifndef NRF_LOG_MODULE_REGISTER
#define NRF_LOG_MODULE_REGISTER()
#endif /* NRF_LOG_MODULE_REGISTER */

#ifndef NRF_LOG_ERROR_STRING_GET
#define NRF_LOG_ERROR_STRING_GET(code) ""
#endif

// DBG_DEV_SRC option: 
// 0 => disable debug log
// 1 => use USB Virtual UART
// 2 => use RTT
#define DBG_DEV_SRC_NONE        0
#define DBG_DEV_SRC_UART        1
#define DBG_DEV_SRC_USB         2
#define DBG_DEV_SRC_RTT         3

#ifndef DBG_DEV_SRC
#define DBG_DEV_SRC             DBG_DEV_SRC_NONE
#endif


#if DBG_DEV_SRC == DBG_DEV_SRC_NONE
#define NRF_LOG_INIT()
#define NRF_LOG_DEBUG(...)
#define NRF_LOG_INFO(...)
#define NRF_LOG_WARNING(...)
#define NRF_LOG_ERROR(...)
#define NRF_LOG_PRINT(...)
#else
#define NRF_LOG_INIT()                  dbg_log_init()
#define NRF_LOG_DEBUG(F, ...)           dbg_log_print(F "\r\n",  ##__VA_ARGS__)
#define NRF_LOG_INFO(F, ...)            dbg_log_print("[info]" F "\r\n",  ##__VA_ARGS__)
#define NRF_LOG_WARNING(F, ...)         dbg_log_print("[WARN!!!]" F "\r\n",  ##__VA_ARGS__)
#define NRF_LOG_ERROR(F, ...)           dbg_log_print("[ERROR!!!]" F "\r\n",  ##__VA_ARGS__)
#define NRF_LOG_PRINT(F, ...)           dbg_log_print( F ,  ##__VA_ARGS__)
#endif

void dbg_log_init(void);
void dbg_log_print(char * format_msg, ...);

#endif /* NRF_LOG_H */
