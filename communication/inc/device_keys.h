/**
 ******************************************************************************
 * @file    device_keys.h
 * @authors Matthew McGowan
 * @date    02 March 2015
 ******************************************************************************
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

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

#ifndef DEVICE_KEYS_H
#define	DEVICE_KEYS_H

/**
 * The maximum size of the 1024-bit device private key in DER format.
 */
const int MAX_DEVICE_PRIVATE_KEY_LENGTH = 612;

/**
 * The maximum size of the 1024-bit device public key in DER format.
 */
const int MAX_DEVICE_PUBLIC_KEY_LENGTH = 162;

/**
 * The maximum size of the 2048-bit server public key in DER format.
 */
const int MAX_SERVER_PUBLIC_KEY_LENGTH = 296;


#endif	/* DEVICE_KEYS_H */
