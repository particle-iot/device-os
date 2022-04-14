/**
 ******************************************************************************
 Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation, either
 version 3 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#ifndef MBEDTLS_COMPAT_H
#define MBEDTLS_COMPAT_H

// Compatibility aliases for mbedTLS / TropicSSL APIs

#define rsa_context mbedtls_rsa_context
#define rsa_free mbedtls_rsa_free

#define aes_context mbedtls_aes_context

#define mpi mbedtls_mpi
#define mpi_init mbedtls_mpi_init
#define mpi_free mbedtls_mpi_free
#define mpi_lset mbedtls_mpi_lset
#define mpi_size mbedtls_mpi_size
#define mpi_read_binary mbedtls_mpi_read_binary
#define mpi_read_string mbedtls_mpi_read_string
#define mpi_write_binary mbedtls_mpi_write_binary

#endif // MBEDTLS_COMPAT_H
