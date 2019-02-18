/*
 ******************************************************************************
 *  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.
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
 ******************************************************************************
 */

#pragma once

#include "dcd.h"
#include "flash_storage_impl.h"

template <typename Store, unsigned sectorSize, unsigned DCD1, unsigned DCD2, uint32_t(*calculateCRC)(const void* data, size_t len)>
class UpdateDCD : public DCD<Store, sectorSize, DCD1, DCD2, calculateCRC>
{
public:
    using base = DCD<Store, sectorSize, DCD1, DCD2, calculateCRC>;
    using Sector = typename base::Sector;

    UpdateDCD()
    {
    }

    inline bool isCurrent(const uint8_t* sector)
    {
        return sector[8]==0 && sector[9]==1;
    }
};
