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

/*-
 * Copyright 2003-2005 Colin Percival
 * Copyright 2012 Matthew Endsley
 * All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted providing that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "bspatch.h"

#include "endian_util.h"
#include "check.h"

#include <algorithm>
#include <limits>
#include <cstdint>

namespace {

using namespace particle;

int readCtrlVal(ssize_t* val, bspatch_read read, void* userData) {
    int64_t v = 0;
    const size_t n = CHECK(read((char*)&v, 8, userData));
    if (n != 8) {
        return SYSTEM_ERROR_NOT_ENOUGH_DATA;
    }
    v = littleEndianToNative(v);
    if (v < 0) {
        // bsdiff uses the sign-magnitude representation for negative offsets
        v = -(v & 0x7fffffffffffffffull);
    }
    if (v < std::numeric_limits<ssize_t>::min() || v > std::numeric_limits<ssize_t>::max()) {
        return SYSTEM_ERROR_BAD_DATA;
    }
    *val = v;
    return n;
}

} // namespace

// The implementation is based on the mendsley's variant of bsdiff (https://github.com/mendsley/bsdiff)
// which uses a different format of patch files and thus is not compatible with the original bsdiff.
// Most importantly, the control, diff and extra data blocks are interleaved in patch data produced
// by the mendsley's bsdiff, which allows compressing and decompressing it as a single stream
int bspatch(bspatch_read patchRead, bspatch_read srcRead, bspatch_seek srcSeek, bspatch_write destWrite,
        size_t destSize, void* userData) {
    // Size of each of the two intermediate buffers used for copying data to the destination stream
    const size_t BUFFER_SIZE = 128;
    uint8_t srcBuf[BUFFER_SIZE] = {};
    uint8_t diffBuf[BUFFER_SIZE] = {};
    size_t patchOffs = 0;
    size_t destOffs = 0;
    while (destOffs < destSize) {
        // Read control data
        ssize_t ctrl[3] = {};
        for (unsigned i = 0; i < 3; ++i) {
            const size_t n = CHECK(readCtrlVal(&ctrl[i], patchRead, userData));
            patchOffs += n;
        }
        if (ctrl[0] < 0 || destOffs + ctrl[0] > destSize ||
                ctrl[1] < 0 || destOffs + ctrl[1] > destSize) {
            return SYSTEM_ERROR_BAD_DATA;
        }
        // Read and apply diff data
        size_t offs = 0;
        while (offs < (size_t)ctrl[0]) {
            const size_t size = std::min(ctrl[0] - offs, BUFFER_SIZE);
            size_t n = CHECK(srcRead((char*)srcBuf, size, userData));
            if (n != size) {
                return SYSTEM_ERROR_NOT_ENOUGH_DATA;
            }
            n = CHECK(patchRead((char*)diffBuf, size, userData));
            if (n != size) {
                return SYSTEM_ERROR_NOT_ENOUGH_DATA;
            }
            for (size_t i = 0; i < size; ++i) {
                srcBuf[i] += diffBuf[i];
            }
            n = CHECK(destWrite((const char*)srcBuf, size, userData));
            if (n != size) {
                return SYSTEM_ERROR_TOO_LARGE;
            }
            offs += size;
            patchOffs += size;
            destOffs += size;
        }
        // Copy extra data
        offs = 0;
        while (offs < (size_t)ctrl[1]) {
            const size_t size = std::min(ctrl[1] - offs, BUFFER_SIZE);
            size_t n = CHECK(patchRead((char*)diffBuf, size, userData));
            if (n != size) {
                return SYSTEM_ERROR_NOT_ENOUGH_DATA;
            }
            n = CHECK(destWrite((const char*)diffBuf, size, userData));
            if (n != size) {
                return SYSTEM_ERROR_TOO_LARGE;
            }
            offs += size;
            patchOffs += size;
            destOffs += size;
        }
        // Change the position in the source data
        if (ctrl[2] != 0) {
            CHECK(srcSeek(ctrl[2], userData));
        }
    };
    return patchOffs;
}
