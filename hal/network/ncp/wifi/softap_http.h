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

#ifndef HAL_NRF52840_SOFTAP_HTTP_H
#define HAL_NRF52840_SOFTAP_HTTP_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

struct Header;
struct Writer;
struct Reader;

typedef int (ResponseCallback)(void* cbArg, uint16_t flags, uint16_t responseCode, const char* mimeType, Header* reserved);
typedef void (PageProvider)(const char* url, ResponseCallback* cb, void* cbArg, Reader* body, Writer* result, void* reserved);

int softap_set_application_page_handler(PageProvider* provider, void* reserved);

PageProvider* softap_get_application_page_handler(void);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // HAL_NRF52840_SOFTAP_HTTP_H
