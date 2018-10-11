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

#include "cellular.h"

#include "hal_platform.h"

#if SYSTEM_CONTROL_ENABLED && HAL_PLATFORM_NCP && HAL_PLATFORM_CELLULAR

#include "common.h"

#include "network/cellular_network_manager.h"
#include "network/cellular_ncp_client.h"
#include "network/ncp.h"

#include "check.h"

#include "cellular.pb.h"

#define PB(_name) particle_ctrl_cellular_##_name

namespace particle {

namespace ctrl {

namespace cellular {

using namespace particle::control::common;

int setAccessPoint(ctrl_request* req) {
    PB(SetAccessPointRequest) pbReq = {};
    DecodedCString dApn(&pbReq.access_point.apn);
    DecodedCString dUser(&pbReq.access_point.user);
    DecodedCString dPwd(&pbReq.access_point.password);
    CHECK(decodeRequestMessage(req, PB(SetAccessPointRequest_fields), &pbReq));
    const auto sim = (SimType)pbReq.sim_type;
    CellularNetworkConfig conf;
    if (!pbReq.access_point.use_defaults) {
        conf.apn(dApn.data).user(dUser.data).password(dPwd.data);
    }
    const auto cellMgr = cellularNetworkManager();
    CHECK_TRUE(cellMgr, SYSTEM_ERROR_UNKNOWN);
    CHECK(cellMgr->setNetworkConfig(sim, std::move(conf)));
    return 0;
}

int getAccessPoint(ctrl_request* req) {
    PB(GetAccessPointRequest) pbReq = {};
    CHECK(decodeRequestMessage(req, PB(GetAccessPointRequest_fields), &pbReq));
    const auto sim = (SimType)pbReq.sim_type;
    const auto cellMgr = cellularNetworkManager();
    CHECK_TRUE(cellMgr, SYSTEM_ERROR_UNKNOWN);
    CellularNetworkConfig conf;
    CHECK(cellMgr->getNetworkConfig(sim, &conf));
    PB(GetAccessPointReply) pbRep = {};
    EncodedString eApn(&pbRep.access_point.apn);
    EncodedString eUser(&pbRep.access_point.user);
    EncodedString ePwd(&pbRep.access_point.password);
    if (conf.isValid()) {
        if (conf.apn()) {
            eApn.data = conf.apn();
            eApn.size = strlen(conf.apn());
        }
        if (conf.user()) {
            eUser.data = conf.user();
            eUser.size = strlen(conf.user());
        }
        if (conf.password()) {
            ePwd.data = conf.password();
            ePwd.size = strlen(conf.password());
        }
    } else {
        pbRep.access_point.use_defaults = true;
    }
    CHECK(encodeReplyMessage(req, PB(GetAccessPointReply_fields), &pbRep));
    return 0;
}

int setActiveSim(ctrl_request* req) {
    PB(SetActiveSimRequest) pbReq = {};
    CHECK(decodeRequestMessage(req, PB(SetActiveSimRequest_fields), &pbReq));
    const auto sim = (SimType)pbReq.sim_type;
    const auto cellMgr = cellularNetworkManager();
    CHECK_TRUE(cellMgr, SYSTEM_ERROR_UNKNOWN);
    CHECK(cellMgr->setActiveSim(sim));
    return 0;
}

int getActiveSim(ctrl_request* req) {
    const auto cellMgr = cellularNetworkManager();
    CHECK_TRUE(cellMgr, SYSTEM_ERROR_UNKNOWN);
    SimType sim = SimType::INVALID;
    CHECK(cellMgr->getActiveSim(&sim));
    PB(GetActiveSimReply) pbRep = {};
    pbRep.sim_type = (PB(SimType))sim;
    CHECK(encodeReplyMessage(req, PB(GetActiveSimReply_fields), &pbRep));
    return 0;
}

int getIccid(ctrl_request* req) {
    const auto cellMgr = cellularNetworkManager();
    CHECK_TRUE(cellMgr, SYSTEM_ERROR_UNKNOWN);
    const auto ncpClient = cellMgr->ncpClient();
    CHECK_TRUE(ncpClient, SYSTEM_ERROR_UNKNOWN);
    CHECK(ncpClient->on());
    char buf[32] = {};
    CHECK(ncpClient->getIccid(buf, sizeof(buf)));
    PB(GetIccidReply) pbRep = {};
    EncodedString eIccid(&pbRep.iccid, buf, strlen(buf));
    CHECK(encodeReplyMessage(req, PB(GetIccidReply_fields), &pbRep));
    return 0;
}

} // particle::ctrl::cellular

} // particle::ctrl

} // particle

#endif // SYSTEM_CONTROL_ENABLED && HAL_PLATFORM_NCP && HAL_PLATFORM_CELLULAR
