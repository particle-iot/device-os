/*
 * Copyright (c) 2019 Particle Industries, Inc.  All rights reserved.
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

#include "logging.h"

LOG_SOURCE_CATEGORY("ot.ext");

#include "openthread-core-config.h"

#include "openthread/particle_extension.h"

#include <common/instance.hpp>
#include <common/extension.hpp>
#include <coap/coap.hpp>
#include <common/tlvs.hpp>
#include <thread/thread_netif.hpp>
#include "ot_api.h"
#include "deviceid_hal.h"
#include "scope_guard.h"
#include <new>

namespace ot {
namespace Extension {
namespace Particle {

const constexpr auto PARTICLE_URI_PATH_DEVICE_ID_GET_REQUEST = "v/p/di";

OT_TOOL_PACKED_BEGIN
class ParticleTlv: public ot::Tlv {
public:
    enum Type {
        TLV_TYPE_DEVICE_ID = 0
    };

    Type GetType(void) const {
        return static_cast<Type>(ot::Tlv::GetType());
    }

    void SetType(Type aType) {
        ot::Tlv::SetType(static_cast<uint8_t>(aType));
    }
} OT_TOOL_PACKED_END;

OT_TOOL_PACKED_BEGIN
class DeviceIdTlv: public ParticleTlv {
public:
    void Init(void) {
        SetType(TLV_TYPE_DEVICE_ID);
        SetLength(sizeof(*this) - sizeof(ParticleTlv));
    }

    bool IsValid(void) const {
        return GetLength() == sizeof(*this) - sizeof(ParticleTlv);
    }

    const uint8_t* GetDeviceId(void) const {
        return deviceId_;
    }

    void SetDeviceId(const uint8_t* deviceId) {
        memcpy(deviceId_, deviceId, HAL_DEVICE_ID_SIZE);
    }

private:
    uint8_t deviceId_[HAL_DEVICE_ID_SIZE];
} OT_TOOL_PACKED_END;

class Extension: public ExtensionBase {
public:
    explicit Extension(Instance& instance)
            : ExtensionBase(instance),
              deviceIdGetRequest_(PARTICLE_URI_PATH_DEVICE_ID_GET_REQUEST, &handleDeviceIdGetRequest, this),
              receiveDeviceIdGetCallback_(nullptr),
              receiveDeviceIdGetCallbackCtx_(nullptr) {
        GetNetif().GetCoap().AddResource(deviceIdGetRequest_);
    }

    otError sendDeviceIdGetRequest(const Ip6::Address& dst) {
        auto msg = GetNetif().GetCoap().NewMessage();
        if (!msg) {
            return OT_ERROR_NO_BUFS;
        }

        NAMED_SCOPE_GUARD(guard, {
            msg->Free();
        });

        if (dst.IsMulticast()) {
            msg->Init(OT_COAP_TYPE_NON_CONFIRMABLE, OT_COAP_CODE_GET);
        } else {
            msg->Init(OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_GET);
        }
        msg->SetToken(Coap::Message::kDefaultTokenLength);
        CHECK_THREAD_OTERR(msg->AppendUriPathOptions(PARTICLE_URI_PATH_DEVICE_ID_GET_REQUEST));

        Ip6::MessageInfo msgInfo;

        msgInfo.SetSockAddr(GetNetif().GetMle().GetMeshLocal16());
        msgInfo.SetPeerAddr(dst);
        msgInfo.SetPeerPort(kCoapUdpPort);
        msgInfo.SetInterfaceId(GetNetif().GetInterfaceId());

        auto err = GetNetif().GetCoap().SendMessage(*msg, msgInfo, &handleDeviceIdGetResponse, this);

        if (err == OT_ERROR_NONE) {
            LOG_DEBUG(TRACE, "Sent GET Device Id request");
            guard.dismiss();
        }

        return err;
    }

    otError setReceiveDeviceIdGetCallback(otExtParticleReceiveDeviceIdGetCallback cb, void* ctx) {
        receiveDeviceIdGetCallback_ = cb;
        receiveDeviceIdGetCallbackCtx_ = ctx;
        return OT_ERROR_NONE;
    }

private:
    static void handleDeviceIdGetRequest(void* ctx, otMessage* msg, const otMessageInfo* msgInfo) {
        static_cast<Extension*>(ctx)->handleDeviceIdGetRequest(*static_cast<Coap::Message*>(msg),
                *static_cast<const Ip6::MessageInfo*>(msgInfo));
    }

    void handleDeviceIdGetRequest(Coap::Message& msg, const Ip6::MessageInfo& msgInfo) {
        if (msg.GetCode() != OT_COAP_CODE_GET) {
            return;
        }

        LOG_DEBUG(TRACE, "Received GET Device Id request");

        auto rep = GetNetif().GetCoap().NewMessage();
        if (!rep) {
            return;
        }

        NAMED_SCOPE_GUARD(guard, {
            rep->Free();
        });

        rep->SetDefaultResponseHeader(msg);
        rep->SetCode(OT_COAP_CODE_CONTENT);
        rep->SetPayloadMarker();

        uint8_t deviceId[HAL_DEVICE_ID_SIZE] = {};
        HAL_device_ID(deviceId, sizeof(deviceId));

        DeviceIdTlv tlv;
        tlv.Init();
        tlv.SetDeviceId(deviceId);

        auto err = rep->Append(&tlv, sizeof(tlv));
        if (err == OT_ERROR_NONE) {
            err = GetNetif().GetCoap().SendMessage(*rep, msgInfo);
        }

        if (err == OT_ERROR_NONE) {
            LOG_DEBUG(TRACE, "Replied to GET Device Id request");
            guard.dismiss();
        }
    }

    static void handleDeviceIdGetResponse(void *ctx, otMessage* msg, const otMessageInfo* msgInfo, otError result) {
        static_cast<Extension*>(ctx)->handleDeviceIdGetResponse(
                *static_cast<Coap::Message*>(msg), *static_cast<const Ip6::MessageInfo*>(msgInfo), result);
    }

    void handleDeviceIdGetResponse(Coap::Message& msg, const Ip6::MessageInfo& msgInfo, otError result) {
        LOG_DEBUG(TRACE, "Received response to GET Device Id request: %d %s", result,
                otCoapMessageCodeToString(&msg));
        if (receiveDeviceIdGetCallback_) {
            DeviceIdTlv tlv;
            const uint8_t* deviceId = nullptr;
            size_t deviceIdSize = 0;

            if (result == OT_ERROR_NONE && msg.GetCode() == OT_COAP_CODE_CONTENT) {
                result = ot::Tlv::Get(msg, ParticleTlv::TLV_TYPE_DEVICE_ID, sizeof(DeviceIdTlv), tlv);
                if (result == OT_ERROR_NONE && tlv.IsValid()) {
                    deviceId = tlv.GetDeviceId();
                    deviceIdSize = HAL_DEVICE_ID_SIZE;
                } else {
                    if (result == OT_ERROR_NONE) {
                        // !tlv.IsValid()
                        result = OT_ERROR_PARSE;
                    }
                }
            }
            receiveDeviceIdGetCallback_(result, msg.GetCode(), deviceId, deviceIdSize, &msgInfo,
                    receiveDeviceIdGetCallbackCtx_);
        }
    }

private:
    Coap::Resource deviceIdGetRequest_;
    otExtParticleReceiveDeviceIdGetCallback receiveDeviceIdGetCallback_;
    void* receiveDeviceIdGetCallbackCtx_;
};

} // namespace Particle

ExtensionBase& ExtensionBase::Init(Instance& instance) {
    static Particle::Extension extension(instance);
    return extension;
}

void ExtensionBase::SignalInstanceInit(void) {
}

void ExtensionBase::SignalNcpInit(Ncp::NcpBase& ncpBase) {
}

} // namespace Extension
} // namespace ot

otError otExtParticleSetReceiveDeviceIdGetCallback(otInstance* instance,
        otExtParticleReceiveDeviceIdGetCallback cb, void* ctx) {
    using namespace ot;
    Instance& inst = *static_cast<Instance*>(instance);

    auto& extension = static_cast<Extension::Particle::Extension&>(inst.GetExtension());
    return extension.setReceiveDeviceIdGetCallback(cb, ctx);
}

otError otExtParticleSendDeviceIdGet(otInstance* instance, const otIp6Address* dest) {
    using namespace ot;
    Instance& inst = *static_cast<Instance*>(instance);

    auto& extension = static_cast<Extension::Particle::Extension&>(inst.GetExtension());
    return extension.sendDeviceIdGetRequest(*static_cast<const Ip6::Address*>(dest));
}
