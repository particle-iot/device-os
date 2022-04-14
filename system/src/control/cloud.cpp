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

#include "cloud.h"

#if SYSTEM_CONTROL_ENABLED

#include "common.h"

#include "system_cloud.h"

#include "spark_wiring_diagnostics.h"

#include "cloud.pb.h"

#define PB(_name) particle_ctrl_cloud_##_name
#define PB_FIELDS(_name) particle_ctrl_cloud_##_name##_fields

namespace particle {

namespace ctrl {

namespace cloud {

using namespace particle::control::common;

int getConnectionStatus(ctrl_request* req) {
    AbstractIntegerDiagnosticData::IntType stat = 0;
    int ret = AbstractIntegerDiagnosticData::get(DIAG_ID_CLOUD_CONNECTION_STATUS, stat);
    if (ret != 0) {
        return ret;
    }
    PB(GetConnectionStatusReply) pbRep = {};
    pbRep.status = (PB(ConnectionStatus))stat;
    ret = encodeReplyMessage(req, PB_FIELDS(GetConnectionStatusReply), &pbRep);
    if (ret != 0) {
        return ret;
    }
    return 0;
}

int connect(ctrl_request* req) {
    spark_cloud_flag_connect();
    return 0;
}

int disconnect(ctrl_request* req) {
    spark_cloud_flag_disconnect();
    return 0;
}

} // particle::ctrl::cloud

} // particle::ctrl

} // particle

#endif // SYSTEM_CONTROL_ENABLED
