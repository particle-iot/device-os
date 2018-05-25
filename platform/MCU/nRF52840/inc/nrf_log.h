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
#include "logging.h"

#ifndef NRF_LOG_MODULE_REGISTER
#define NRF_LOG_MODULE_REGISTER()
#endif /* NRF_LOG_MODULE_REGISTER */

#ifndef NRF_LOG_ERROR_STRING_GET
#define NRF_LOG_ERROR_STRING_GET(code) ""
#endif


#define NRF_LOG_INIT()
#define NRF_LOG_DEBUG(...)            LOG_DEBUG(TRACE, __VA_ARGS__)
#define NRF_LOG_INFO(...)             LOG_DEBUG(INFO, __VA_ARGS__)
#define NRF_LOG_WARNING(...)          LOG_DEBUG(WARN, __VA_ARGS__)
#define NRF_LOG_ERROR(...)            LOG_DEBUG(ERROR, __VA_ARGS__)

#define NRF_LOG_INST_ERROR(p_inst, ...)         NRF_LOG_ERROR(__VA_ARGS__)
#define NRF_LOG_INST_WARNING(p_inst, ...)       NRF_LOG_WARNING(__VA_ARGS__)
#define NRF_LOG_INST_INFO(p_inst, ...)          NRF_LOG_INFO(__VA_ARGS__)
#define NRF_LOG_INST_DEBUG(p_inst, ...)         NRF_LOG_DEBUG(__VA_ARGS__)


#endif /* NRF_LOG_H */
