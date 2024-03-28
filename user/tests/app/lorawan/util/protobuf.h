#pragma once

#include <pb.h>

#include "buffer.h"

namespace particle::util {

int encodeProtobuf(Buffer& buf, const void* msg, const pb_msgdesc_t* desc);
int decodeProtobuf(const Buffer& buf, void* msg, const pb_msgdesc_t* desc);

} // namespace particle::util
