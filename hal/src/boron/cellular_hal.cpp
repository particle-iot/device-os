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

#include "cellular_hal.h"

#include "network/cellular_network_manager.h"
#include "network/cellular_ncp_client.h"
#include "network/ncp.h"
#include "ifapi.h"

#include "system_network.h" // FIXME: For network_interface_index

#include "str_util.h"
#include "endian_util.h"
#include "scope_guard.h"
#include "check.h"

#include "at_parser.h"
#include "at_command.h"
#include "at_response.h"
#include "modem/enums_hal.h"

#include <limits>

namespace {

using namespace particle;

const size_t MAX_RESP_SIZE = 1024;

int parseMdmType(const char* buf, size_t size) {
    static const struct {
        const char* prefix;
        int type;
    } types[] = {
        { "RING", TYPE_RING },
        { "CONNECT", TYPE_CONNECT },
        { "+", TYPE_PLUS },
        { "@", TYPE_PROMPT }, // Sockets
        { ">", TYPE_PROMPT }, // SMS
        { "ABORTED", TYPE_ABORTED } // Current command aborted
    };
    for (size_t i = 0; i < sizeof(types) / sizeof(types[0]); ++i) {
        const auto& t = types[i];
        if (startsWith(buf, size, t.prefix, strlen(t.prefix))) {
            return t.type;
        }
    }
    return TYPE_UNKNOWN;
}

int mdmTypeToResult(int type) {
    switch (type) {
    case TYPE_OK:
        return RESP_OK;
    case TYPE_ERROR:
    case TYPE_BUSY:
    case TYPE_NODIALTONE:
    case TYPE_NOANSWER:
    case TYPE_NOCARRIER:
        return RESP_ERROR;
    case TYPE_PROMPT:
        return RESP_PROMPT;
    case TYPE_ABORTED:
        return RESP_ABORTED;
    default:
        return NOT_FOUND;
    }
}

hal_net_access_tech_t fromCellularAccessTechnology(CellularAccessTechnology rat) {
    switch (rat) {
    case CellularAccessTechnology::GSM:
    case CellularAccessTechnology::GSM_COMPACT:
        return NET_ACCESS_TECHNOLOGY_GSM;
    case CellularAccessTechnology::GSM_EDGE:
        return NET_ACCESS_TECHNOLOGY_EDGE;
    case CellularAccessTechnology::UTRAN:
    case CellularAccessTechnology::UTRAN_HSDPA:
    case CellularAccessTechnology::UTRAN_HSUPA:
    case CellularAccessTechnology::UTRAN_HSDPA_HSUPA:
        return NET_ACCESS_TECHNOLOGY_UTRAN;
    case CellularAccessTechnology::LTE:
        return NET_ACCESS_TECHNOLOGY_LTE;
    case CellularAccessTechnology::EC_GSM_IOT:
        return NET_ACCESS_TECHNOLOGY_LTE_CAT_M1;
    case CellularAccessTechnology::E_UTRAN:
        return NET_ACCESS_TECHNOLOGY_LTE_CAT_NB1;
    default:
        return NET_ACCESS_TECHNOLOGY_UNKNOWN;
    }
}

} // unnamed

int cellular_on(void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int cellular_init(void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int cellular_off(void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int cellular_register(void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int cellular_pdp_activate(CellularCredentials* connect, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int cellular_pdp_deactivate(void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int cellular_gprs_attach(CellularCredentials* connect, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int cellular_gprs_detach(void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int cellular_fetch_ipconfig(CellularConfig* conf, void* reserved) {
    if_t iface = nullptr;
    CHECK(if_get_by_index(NETWORK_INTERFACE_CELLULAR, &iface));
    CHECK_TRUE(iface, SYSTEM_ERROR_INVALID_STATE);
    unsigned flags = 0;
    CHECK(if_get_flags(iface, &flags));
    CHECK_TRUE((flags & IFF_UP) && (flags & IFF_LOWER_UP), SYSTEM_ERROR_INVALID_STATE);
    // IP address
    if_addrs* ifAddrList = nullptr;
    CHECK(if_get_addrs(iface, &ifAddrList));
    SCOPE_GUARD({
        if_free_if_addrs(ifAddrList);
    });
    if_addr* ifAddr = nullptr;
    for (if_addrs* i = ifAddrList; i; i = i->next) {
        if (i->if_addr->addr->sa_family == AF_INET) { // Skip non-IPv4 addresses
            ifAddr = i->if_addr;
            break;
        }
    }
    CHECK_TRUE(ifAddr, SYSTEM_ERROR_INVALID_STATE);
    auto sockAddr = (const sockaddr_in*)ifAddr->addr;
    CHECK_TRUE(sockAddr, SYSTEM_ERROR_INVALID_STATE);
    static_assert(sizeof(conf->nw.aucIP.ipv4) == sizeof(sockAddr->sin_addr), "");
    memcpy(&conf->nw.aucIP.ipv4, &sockAddr->sin_addr, sizeof(sockAddr->sin_addr));
    conf->nw.aucIP.ipv4 = reverseByteOrder(conf->nw.aucIP.ipv4);
    conf->nw.aucIP.v = 4;
    // Subnet mask
    sockAddr = (const sockaddr_in*)ifAddr->netmask;
    CHECK_TRUE(sockAddr, SYSTEM_ERROR_INVALID_STATE);
    memcpy(&conf->nw.aucSubnetMask.ipv4, &sockAddr->sin_addr, sizeof(sockAddr->sin_addr));
    conf->nw.aucSubnetMask.ipv4 = reverseByteOrder(conf->nw.aucSubnetMask.ipv4);
    conf->nw.aucSubnetMask.v = 4;
    // Peer address
    sockAddr = (const sockaddr_in*)ifAddr->peeraddr;
    CHECK_TRUE(sockAddr, SYSTEM_ERROR_INVALID_STATE);
    memcpy(&conf->nw.aucDefaultGateway.ipv4, &sockAddr->sin_addr, sizeof(sockAddr->sin_addr));
    conf->nw.aucDefaultGateway.ipv4 = reverseByteOrder(conf->nw.aucDefaultGateway.ipv4);
    conf->nw.aucDefaultGateway.v = 4;
    return 0;
}

int cellular_device_info(CellularDevice* info, void* reserved) {
    const auto mgr = cellularNetworkManager();
    CHECK_TRUE(mgr, SYSTEM_ERROR_UNKNOWN);
    const auto client = mgr->ncpClient();
    CHECK_TRUE(client, SYSTEM_ERROR_UNKNOWN);

    const NcpClientLock lock(client);
    // Ensure the modem is powered on
    CHECK(client->on());
    CHECK(client->getIccid(info->iccid, sizeof(info->iccid)));
    CHECK(client->getImei(info->imei, sizeof(info->imei)));
    if (info->size >= offsetof(CellularDevice, dev) + sizeof(CellularDevice::dev)) {
        switch (client->ncpId()) {
        case MESH_NCP_SARA_U201:
            info->dev = DEV_SARA_U201;
            break;
        case MESH_NCP_SARA_G350:
            info->dev = DEV_SARA_G350;
            break;
        case MESH_NCP_SARA_R410:
            info->dev = DEV_SARA_R410;
            break;
        default:
            info->dev = DEV_UNKNOWN;
            break;
        }
    }
    return 0;
}

int cellular_credentials_set(const char* apn, const char* user, const char* password, void* reserved) {
    auto sim = particle::SimType::INTERNAL;
    CHECK(CellularNetworkManager::getActiveSim(&sim));
    auto cred = CellularNetworkConfig().apn(apn).user(user).password(password);
    CHECK(CellularNetworkManager::setNetworkConfig(sim, std::move(cred)));
    return 0;
}

CellularCredentials* cellular_credentials_get(void* reserved) {
    // TODO: Copy the settings to a storage provided by the calling code
    static CellularCredentials cred;
    static CellularNetworkConfig conf;
    auto sim = particle::SimType::INTERNAL;
    CHECK_RETURN(CellularNetworkManager::getActiveSim(&sim), nullptr);
    CHECK_RETURN(CellularNetworkManager::getNetworkConfig(sim, &conf), nullptr);
    cred.apn = conf.hasApn() ? conf.apn() : "";
    cred.username = conf.hasUser() ? conf.user() : "";
    cred.password = conf.hasPassword() ? conf.password() : "";
    return &cred;
}

int cellular_credentials_clear(void* reserved) {
    auto sim = particle::SimType::INTERNAL;
    CHECK(CellularNetworkManager::getActiveSim(&sim));
    CHECK(CellularNetworkManager::clearNetworkConfig(sim));
    return 0;
}

cellular_result_t cellular_global_identity(CellularGlobalIdentity* cgi_, void* reserved_) {
    CellularGlobalIdentity cgi;  // Intentionally left uninitialized

    // Acquire Cellular NCP Client
    const auto mgr = cellularNetworkManager();
    CHECK_TRUE(mgr, SYSTEM_ERROR_UNKNOWN);
    const auto client = mgr->ncpClient();
    CHECK_TRUE(client, SYSTEM_ERROR_UNKNOWN);

    // Validate Argument(s)
    (void)reserved_;
    CHECK_TRUE((nullptr != cgi_), SYSTEM_ERROR_INVALID_ARGUMENT);

    // Load cached data into result struct
    CHECK(client->getCellularGlobalIdentity(&cgi));

    // Validate cache
    CHECK_TRUE(0 != cgi.mobile_country_code, SYSTEM_ERROR_BAD_DATA);
    CHECK_TRUE(0 != cgi.mobile_network_code, SYSTEM_ERROR_BAD_DATA);
    CHECK_TRUE(0xFFFF != cgi.location_area_code, SYSTEM_ERROR_BAD_DATA);
    CHECK_TRUE(0xFFFFFFFF != cgi.cell_id, SYSTEM_ERROR_BAD_DATA);

    // Update result
    *cgi_ = cgi;

    return SYSTEM_ERROR_NONE;
}

bool cellular_sim_ready(void* reserved) {
    return false;
}

void cellular_cancel(bool cancel, bool calledFromISR, void* reserved) {
}

int cellular_signal(CellularSignalHal* signal, cellular_signal_t* signalExt) {
    const auto mgr = cellularNetworkManager();
    CHECK_TRUE(mgr, SYSTEM_ERROR_UNKNOWN);
    const auto client = mgr->ncpClient();
    CHECK_TRUE(client, SYSTEM_ERROR_UNKNOWN);
    CellularSignalQuality s;
    CHECK(client->getSignalQuality(&s));
    const auto strn = s.strength();
    const auto qual = s.quality();
    if (signal) {
        signal->rssi = strn;
        signal->qual = qual;
    }
    if (signalExt) {
        signalExt->rat = fromCellularAccessTechnology(s.accessTechnology());
        // Signal strength
        switch (s.strengthUnits()) {
        case CellularStrengthUnits::RXLEV: {
            // Convert to dBm [-111, -48], see 3GPP TS 45.008 8.1.4
            // Reported multiplied by 100
            signalExt->rssi = (strn != 99) ? (strn - 111) * 100 : std::numeric_limits<int32_t>::min();
            // RSSI in % [0, 100] based on [-111, -48] range mapped to [0, 65535] integer range
            signalExt->strength = (strn != 99) ? strn * 65535 / 63 : std::numeric_limits<int32_t>::min();
            break;
        }
        case CellularStrengthUnits::RSCP: {
            // Convert to dBm [-121, -25], see 3GPP TS 25.133 9.1.1.3
            // Reported multiplied by 100
            signalExt->rscp = (strn != 255) ? (strn - 116) * 100 : std::numeric_limits<int32_t>::min();
            // RSCP in % [0, 100] based on [-121, -25] range mapped to [0, 65535] integer range
            signalExt->strength = (strn != 255) ? (strn + 5) * 65535 / 96 : std::numeric_limits<int32_t>::min();
            break;
        }
        case CellularStrengthUnits::RSRP: {
            // Convert to dBm [-141, -44], see 3GPP TS 36.133 subclause 9.1.4
            // Reported multiplied by 100
            signalExt->rsrp = (strn != 255) ? (strn - 141) * 100 : std::numeric_limits<int32_t>::min();
            // RSRP in % [0, 100] based on [-141, -44] range mapped to [0, 65535] integer range
            signalExt->strength = (strn != 255) ? strn * 65535 / 97 : std::numeric_limits<int32_t>::min();
            break;
        }
        default:
            signalExt->rssi = std::numeric_limits<int32_t>::min();
            signalExt->strength = std::numeric_limits<int32_t>::min();
            break;
        }
        // Signal quality
        switch (s.qualityUnits()) {
        case CellularQualityUnits::RXQUAL: {
            // % * 100, see 3GPP TS 45.008 8.2.4
            // 0.14%, 0.28%, 0.57%, 1.13%, 2.26%, 4.53%, 9.05%, 18.10%
            static const uint16_t berMapping[] = { 14, 28, 57, 113, 226, 453, 905, 1810 };
            static const size_t berMappingSize = sizeof(berMapping) / sizeof(berMapping[0]);
            // Convert to BER (% * 100), see 3GPP TS 45.008 8.2.4
            signalExt->ber = (qual >= 0 && (size_t)qual < berMappingSize) ? berMapping[qual] : std::numeric_limits<int32_t>::min();
            // Quality based on RXQUAL in % [0, 100] mapped to [0, 65535] integer range
            signalExt->quality = (qual != 99) ? (7 - qual) * 65535 / 7 : std::numeric_limits<int32_t>::min();
            break;
        }
        case CellularQualityUnits::MEAN_BEP: {
            // Convert to MEAN_BEP level first
            // See u-blox AT Reference Manual:
            // In 2G RAT EGPRS packet transfer mode indicates the Mean Bit Error Probability (BEP) of a radio
            // block. 3GPP TS 45.008 [148] specifies the range 0-31 for the Mean BEP which is mapped to
            // the range 0-7 of <qual>
            const int bepLevel = (qual != 99) ? (7 - qual) * 31 / 7 : std::numeric_limits<int32_t>::min();
            // Convert to log10(MEAN_BEP) multiplied by 100, see 3GPP TS 45.008 10.2.3.3
            // Uses QPSK table
            signalExt->bep = bepLevel >= 0 ? (-(bepLevel - 31) * 10 - 370) : bepLevel;
            // Quality based on RXQUAL in % [0, 100] mapped to [0, 65535] integer range
            signalExt->quality = (qual != 99) ? (7 - qual) * 65535 / 7 : std::numeric_limits<int32_t>::min();
            break;
        }
        case CellularQualityUnits::ECN0: {
            // Convert to Ec/Io (dB) [-24.5, 0], see 3GPP TS 25.133 9.1.2.3
            // Report multiplied by 100
            signalExt->ecno = (qual != 255) ? qual * 50 - 2450 : std::numeric_limits<int32_t>::min();
            // Quality based on Ec/Io in % [0, 100] mapped to [0,65535] integer range
            signalExt->quality = (qual != 255) ? qual * 65535 / 49 : std::numeric_limits<int32_t>::min();
            break;
        }
        case CellularQualityUnits::RSRQ: {
            // Convert to dB [-19.5, -3], see 3GPP TS 36.133 subclause 9.1.7
            // Report multiplied by 100
            signalExt->rsrq = (qual != 255) ? qual * 50 - 2000 : std::numeric_limits<int32_t>::min();
            // Quality based on RSRQ in % [0, 100] mapped to [0,65535] integer range
            signalExt->quality = (qual != 255) ? qual * 65535 / 34 : std::numeric_limits<int32_t>::min();
            break;
        }
        default:
            signalExt->qual = std::numeric_limits<int32_t>::min();
            signalExt->quality = std::numeric_limits<int32_t>::min();
            break;
        }
    }
    return 0;
}

int cellular_command(_CALLBACKPTR_MDM cb, void* param, system_tick_t timeout_ms, const char* format, ...) {
    auto mgr = cellularNetworkManager();
    CHECK_TRUE(mgr, SYSTEM_ERROR_UNKNOWN);
    auto client = mgr->ncpClient();
    CHECK_TRUE(client, SYSTEM_ERROR_UNKNOWN);
    auto parser = client->atParser();
    CHECK_TRUE(parser, SYSTEM_ERROR_UNKNOWN);

    NcpClientLock lock(client);

    // WORKAROUND: cut "\r\n", it has been resolved in AT Parser
    size_t n = strlen(format);
    while (n > 0 && (format[n - 1] == '\r' || format[n - 1] == '\n')) {
        --n;
    }
    std::unique_ptr<char[]> fmt(new(std::nothrow) char[n + 1]);
    CHECK_TRUE(fmt, SYSTEM_ERROR_NO_MEMORY);
    memcpy(fmt.get(), format, n);
    fmt[n] = '\0';

    va_list args;
    va_start(args, format);
    auto resp = parser->command().vprintf(fmt.get(), args).timeout(timeout_ms).send();
    va_end(args);
    if (!resp) {
        if (resp.error() == SYSTEM_ERROR_TIMEOUT) {
            return WAIT;
        }
        return resp.error();
    }

    fmt.reset();

    const size_t bufSize = MAX_RESP_SIZE + 64; // add some more space for framing
    std::unique_ptr<char[]> buf(new(std::nothrow) char[bufSize]);
    CHECK_TRUE(buf, SYSTEM_ERROR_NO_MEMORY);
    // WORKAROUND: Add "\r\n" for compatibility
    buf[0] = '\r';
    buf[1] = '\n';

    int mdmType = TYPE_OK;
    while (resp.hasNextLine()) {
        int n = resp.readLine(buf.get() + 2, bufSize - 5);
        if (n < 0) {
            if (n == SYSTEM_ERROR_TIMEOUT) {
                return WAIT;
            }
            return n;
        }
        if (cb) {
            mdmType = parseMdmType(buf.get() + 2, n);
            n += 2;
            // WORKAROUND: Add "\r\n" for compatibility
            buf[n++] = '\r';
            buf[n++] = '\n';
            buf[n] = '\0';
            const int r = cb(mdmType, buf.get(), n, param);
            if (r != WAIT) {
                return r;
            }
        }
    };

    buf.reset();

    const int result = resp.readResult();
    if (result < 0) {
        if (result == SYSTEM_ERROR_TIMEOUT) {
            return WAIT;
        }
        return result;
    }

    const char* mdmStr = nullptr;
    switch (result) {
    case AtResponse::OK:
        mdmType = TYPE_OK;
        mdmStr = "\r\nOK\r\n";
        break;
    case AtResponse::BUSY:
        mdmType = TYPE_BUSY;
        mdmStr = "\r\nBUSY\r\n";
        break;
    case AtResponse::ERROR:
        mdmType = TYPE_ERROR;
        mdmStr = "\r\nERROR\r\n";
        break;
    case AtResponse::CME_ERROR:
        mdmType = TYPE_ERROR;
        mdmStr = "\r\n+CME ERROR";
        break;
    case AtResponse::CMS_ERROR:
        mdmType = TYPE_ERROR;
        mdmStr = "\r\n+CMS ERROR";
        break;
    case AtResponse::NO_DIALTONE:
        mdmType = TYPE_NODIALTONE;
        mdmStr = "\r\nNO DIALTONE\r\n";
        break;
    case AtResponse::NO_ANSWER:
        mdmType = TYPE_NOANSWER;
        mdmStr = "\r\nNO ANSWER\r\n";
        break;
    case AtResponse::NO_CARRIER:
        mdmType = TYPE_NOCARRIER;
        mdmStr = "\r\nNO CARRIER\r\n";
        break;
    default:
        mdmType = TYPE_UNKNOWN;
        mdmStr = "";
        break;
    }

    if (cb) {
        if (result == AtResponse::CME_ERROR || result == AtResponse::CMS_ERROR) {
            char msg[32] = {};
            snprintf(msg, sizeof(msg), "%s: %d\r\n", mdmStr, resp.resultErrorCode());
            cb(mdmType, msg, strlen(msg), param);
        } else {
            cb(mdmType, mdmStr, strlen(mdmStr), param);
        }
    }

    return mdmTypeToResult(mdmType);
}

int cellular_data_usage_set(CellularDataHal* data, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int cellular_data_usage_get(CellularDataHal* data, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int cellular_sms_received_handler_set(_CELLULAR_SMS_CB_MDM cb, void* data, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

void HAL_USART3_Handler_Impl(void* reserved) {
}

int cellular_pause(void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int cellular_resume(void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int cellular_imsi_to_network_provider(void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

CellularNetProvData cellular_network_provider_data_get(void* reserved) {
    return CellularNetProvData();
}

int cellular_lock(void* reserved) {
    const auto mgr = cellularNetworkManager();
    CHECK_TRUE(mgr, SYSTEM_ERROR_UNKNOWN);
    const auto client = mgr->ncpClient();
    CHECK_TRUE(client, SYSTEM_ERROR_UNKNOWN);
    client->lock();
    return 0;
}

void cellular_unlock(void* reserved) {
    const auto mgr = cellularNetworkManager();
    if (mgr) {
        const auto client = mgr->ncpClient();
        if (client) {
            client->unlock();
        }
    }
}

void cellular_set_power_mode(int mode, void* reserved) {
}

int cellular_band_select_set(MDM_BandSelect* bands, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int cellular_band_select_get(MDM_BandSelect* bands, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int cellular_band_available_get(MDM_BandSelect* bands, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int cellular_set_active_sim(int simType, void* reserved) {
    auto sim = particle::SimType::INTERNAL;
    if (simType == EXTERNAL_SIM) {
        sim = particle::SimType::EXTERNAL;
    }
    CHECK(CellularNetworkManager::setActiveSim(sim));
    return 0;
}

int cellular_get_active_sim(int* simType, void* reserved) {
    auto sim = particle::SimType::INTERNAL;
    CHECK(CellularNetworkManager::getActiveSim(&sim));
    if (sim == particle::SimType::EXTERNAL) {
        *simType = EXTERNAL_SIM;
    } else {
        *simType = INTERNAL_SIM;
    }
    return 0;
}
