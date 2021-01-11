/*
 * Copyright (c) 2021 Particle Industries, Inc.  All rights reserved.
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

#pragma once

/** \def MBEDTLS_SSL_MAX_CONTENT_LEN
 *
 * Maximum length (in bytes) of incoming and outgoing plaintext fragments.
 *
 * This determines the size of both the incoming and outgoing TLS I/O buffers
 * in such a way that both are capable of holding the specified amount of
 * plaintext data, regardless of the protection mechanism used.
 *
 * To configure incoming and outgoing I/O buffers separately, use
 * #MBEDTLS_SSL_IN_CONTENT_LEN and #MBEDTLS_SSL_OUT_CONTENT_LEN,
 * which overwrite the value set by this option.
 *
 * \note When using a value less than the default of 16KB on the client, it is
 *       recommended to use the Maximum Fragment Length (MFL) extension to
 *       inform the server about this limitation. On the server, there
 *       is no supported, standardized way of informing the client about
 *       restriction on the maximum size of incoming messages, and unless
 *       the limitation has been communicated by other means, it is recommended
 *       to only change the outgoing buffer size #MBEDTLS_SSL_OUT_CONTENT_LEN
 *       while keeping the default value of 16KB for the incoming buffer.
 *
 * Uncomment to set the maximum plaintext size of both
 * incoming and outgoing I/O buffers.
 */
// The current value of 1152 bytes is chosen so as to maximize the amount of
// data that can be fit into a 1280-byte IPv4 or IPv6 packet while still having
// some space left for DTLS and CoAP extensions that we may employ in the future.
// IPv6 mandates a minimum MTU of 1280 bytes, and we consider the chance of
// fragmentation of a 1280-byte packet to be minimal in IPv4 networks.
//
// Application data: 1152 bytes;
// Nonce and authentication tag: 16 bytes for AEAD_AES_128_CCM_8;
// DTLS header: 13 bytes;
// UDP header: 8 bytes;
// IP header: 20 (IPv4) or 40 (IPv6) bytes without options/extensions.
#define MBEDTLS_SSL_MAX_CONTENT_LEN 1152
