/**
 ******************************************************************************
 * @file    module_info_hal.h
 * @authors mat
 * @date    24 February 2015
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

#ifndef MODULE_INFO_HAL_H
#define	MODULE_INFO_HAL_H

#define MODULE_VERIFY_CRC                               (1<<1)
#define MODULE_VERIFY_DESTINATION_IS_START_ADDRESS      (1<<2)
// FIXME: duplicate value. We're leaving this as-is for now in order to keep
// the compatibility and keep the same behavior of checking both the function and length.
// This should probably be changed to a single flag called MODULE_VERIFY_FUNCTION_AND_LENGTH
#define MODULE_VERIFY_FUNCTION                          (1<<3)
#define MODULE_VERIFY_LENGTH                            (1<<3)
#define MODULE_DROP_MODULE_INFO                         (1<<4)
#define MODULE_COMPRESSED                               (1<<5)
#define MODULE_ENCRYPTED                                (1<<6)

#define MODULE_VERIFY_MASK \
            (MODULE_VERIFY_CRC | \
            MODULE_VERIFY_DESTINATION_IS_START_ADDRESS | \
            MODULE_VERIFY_FUNCTION | \
            MODULE_VERIFY_LENGTH)


#endif	/* MODULE_INFO_HAL_H */

