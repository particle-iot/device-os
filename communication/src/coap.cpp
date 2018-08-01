/**
  ******************************************************************************
  * @file    coap.cpp
  * @authors  Zachary Crockett
  * @version V1.0.0
  * @date    15-Nov-2013
  * @brief   COAP
  ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program; if not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
  */
#include "coap.h"

namespace particle {
namespace protocol {

CoAPCode::Enum CoAP::code(const unsigned char *message) {
    CoAPCode::Enum code = (CoAPCode::Enum) message[1];
    switch (code) {
        case 0x00:
            return CoAPCode::EMPTY;
        case 0x01:
            return CoAPCode::GET;
        case 0x02:
            return CoAPCode::POST;
        case 0x03:
            return CoAPCode::PUT;

        case CoAPCode::OK: return CoAPCode::OK;
        case CoAPCode::CREATED: return CoAPCode::CREATED;
        case CoAPCode::DELETED: return CoAPCode::DELETED;
        case CoAPCode::CHANGED: return CoAPCode::CHANGED;
        case CoAPCode::NOT_MODIFIED: return CoAPCode::NOT_MODIFIED;
        case CoAPCode::CONTENT: return CoAPCode::CONTENT;
        default:
            // todo - add all recognised codes. Via a smart macro to void manually repeating them.
            if (CoAPCode::is_success(code)) {    // should have been handled above.
                return CoAPCode::ERROR;
            }
            return code;                        // allow any error code
    }
}

CoAPType::Enum CoAP::type(const unsigned char *message) {
    switch (message[0] & 0x30) {
        case 0x00:
            return CoAPType::CON;
        case 0x10:
            return CoAPType::NON;
        default:
        case 0x20:
            return CoAPType::ACK;
        case 0x30:
            return CoAPType::RESET;
    }
}

size_t CoAP::option_decode(unsigned char **option) {
    unsigned char nibble = **option & 0x0f;
    size_t option_length;
    if (13 > nibble) {
        option_length = nibble;
        (*option)++;
    } else if (13 == nibble) {
        (*option)++;
        option_length = **option + 13;
        (*option)++;
    } else if (14 == nibble) {
        option_length = ((*(*option + 1) << 8) | *(*option + 2)) + 269;
        (*option) += 3;
    } else {
        // 15 == nibble, reserved value in CoAP spec
        option_length = 0;
    }
    return option_length;
}

}
}
