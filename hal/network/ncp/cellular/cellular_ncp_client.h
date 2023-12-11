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

#pragma once

#include "ncp_client.h"
#include "cellular_network_manager.h"
#include "cellular_hal_cellular_global_identity.h"
#include "timer_hal.h"
#include "underlying_type.h"

namespace particle {

struct CellularNcpEvent: NcpEvent {
    enum Type {
        AUTH = CUSTOM_EVENT_TYPE_BASE
    };
};

struct CellularNcpAuthEvent: CellularNcpEvent {
    const char* user;
    const char* password;
};

class CellularNcpClientConfig: public NcpClientConfig {
public:
    CellularNcpClientConfig();

    CellularNcpClientConfig& simType(SimType type);
    SimType simType() const;

    CellularNcpClientConfig& ncpIdentifier(PlatformNCPIdentifier ident);
    PlatformNCPIdentifier ncpIdentifier() const;

private:
    SimType simType_;
    PlatformNCPIdentifier ident_;
};

enum class UbloxSaraUmnoprof {
    NONE             = -1,
    SW_DEFAULT       = 0,
    SIM_SELECT       = 1,
    ATT              = 2,
    VERIZON          = 3,
    TELSTRA          = 4,
    TMOBILE          = 5,
    CHINA_TELECOM    = 6,
    SPRINT           = 8,
    VODAFONE         = 19,
    TELUS            = 21,
    DEUTSCHE_TELEKOM = 31,
    STANDARD_EUROPE  = 100,
};

enum class CellularAccessTechnology {
    NONE = -1,
    GSM = 0,
    GSM_COMPACT = 1,
    UTRAN = 2,
    GSM_EDGE = 3,
    UTRAN_HSDPA = 4,
    UTRAN_HSUPA = 5,
    UTRAN_HSDPA_HSUPA = 6,
    LTE = 7,
    LTE_CAT_M1 = 8,
    LTE_NB_IOT = 9
};

enum class CellularPowerSavingValue {
    NONE = -1,
    UPSV_DISABLED = 0,
    UPSV_ENABLED_TIMER = 1,
    UPSV_ENABLED_RTS = 2,
    UPSV_ENABLED_DTR = 3,
};

enum class CellularOperationMode {
    NONE = -1,
    PS_ONLY = 0,
    CS_PS_MODE = 2,
};

PARTICLE_DEFINE_ENUM_COMPARISON_OPERATORS(CellularOperationMode);

enum class CellularFunctionality {
    NONE = -1,
    MINIMUM = 0,
    FULL = 1,
    AIRPLANE = 4,
    FAST_SHUTDOWN = 10,
    RESET_NO_SIM = 15,
    RESET_WITH_SIM = 16,
    MINIMUM_NO_SIM = 19,
};

PARTICLE_DEFINE_ENUM_COMPARISON_OPERATORS(CellularFunctionality);

enum class CellularStrengthUnits {
    NONE = 0,
    RXLEV = 1,
    RSCP = 2,
    RSRP = 3
};

enum class CellularQualityUnits {
    NONE = 0,
    RXQUAL = 1,
    ECN0 = 2,
    RSRQ = 3,
    MEAN_BEP = 4
};

class CellularSignalQuality {
public:
    CellularSignalQuality() = default;

    CellularSignalQuality& accessTechnology(const CellularAccessTechnology& act);
    CellularAccessTechnology accessTechnology() const;

    CellularSignalQuality& strength(int val);
    int strength() const;

    CellularStrengthUnits strengthUnits() const;

    CellularSignalQuality& quality(int val);
    int quality() const;

    CellularSignalQuality& qualityUnits(const CellularQualityUnits& units);
    CellularQualityUnits qualityUnits() const;

private:
    CellularAccessTechnology act_ = CellularAccessTechnology::NONE;
    int strength_ = -1;
    int quality_ = -1;
    CellularQualityUnits qunits_ = CellularQualityUnits::NONE;
};

class CellularNcpClient: public NcpClient {
public:
    virtual int connect(const CellularNetworkConfig& conf) = 0;
    virtual int getCellularGlobalIdentity(CellularGlobalIdentity* cgi) = 0;
    virtual int getIccid(char* buf, size_t size) = 0;
    virtual int getImei(char* buf, size_t size) = 0;
    virtual int getSignalQuality(CellularSignalQuality* qual) = 0;
    virtual int setRegistrationTimeout(unsigned timeout) = 0;
    virtual int getTxDelayInDataChannel() = 0;
    virtual int enterDataMode() = 0;
    virtual int getMtu() = 0;
    virtual int urcs(bool enable) = 0;
    virtual int startNcpFwUpdate(bool update) = 0;
    virtual int dataModeError(int error) = 0;
};

inline CellularNcpClientConfig::CellularNcpClientConfig() :
        simType_(SimType::INTERNAL),
        ident_(PLATFORM_NCP_UNKNOWN) {
}

inline CellularNcpClientConfig& CellularNcpClientConfig::simType(SimType type) {
    simType_ = type;
    return *this;
}

inline SimType CellularNcpClientConfig::simType() const {
    return simType_;
}


inline CellularNcpClientConfig& CellularNcpClientConfig::ncpIdentifier(PlatformNCPIdentifier ident) {
    ident_ = ident;
    return *this;
}

inline PlatformNCPIdentifier CellularNcpClientConfig::ncpIdentifier() const {
    return ident_;
}

// CellularSignalQuality

inline CellularSignalQuality& CellularSignalQuality::accessTechnology(const CellularAccessTechnology& act) {
    act_ = act;
    return *this;
}

inline CellularAccessTechnology CellularSignalQuality::accessTechnology() const {
    return act_;
}

inline CellularSignalQuality& CellularSignalQuality::strength(int val) {
    strength_ = val;
    return *this;
}

inline int CellularSignalQuality::strength() const {
    return strength_;
}

inline CellularStrengthUnits CellularSignalQuality::strengthUnits() const {
    switch (act_) {
        case CellularAccessTechnology::GSM:
        case CellularAccessTechnology::GSM_COMPACT:
        case CellularAccessTechnology::GSM_EDGE: {
            return CellularStrengthUnits::RXLEV;
        }
        case CellularAccessTechnology::UTRAN:
        case CellularAccessTechnology::UTRAN_HSDPA:
        case CellularAccessTechnology::UTRAN_HSUPA:
        case CellularAccessTechnology::UTRAN_HSDPA_HSUPA: {
            return CellularStrengthUnits::RSCP;
        }
        case CellularAccessTechnology::LTE:
        case CellularAccessTechnology::LTE_CAT_M1:
        case CellularAccessTechnology::LTE_NB_IOT: {
            return CellularStrengthUnits::RSRP;
        }
        default: {
            return CellularStrengthUnits::NONE;
        }
    }
}

inline CellularSignalQuality& CellularSignalQuality::quality(int val) {
    quality_ = val;
    return *this;
}

inline int CellularSignalQuality::quality() const {
    return quality_;
}

inline CellularSignalQuality& CellularSignalQuality::qualityUnits(const CellularQualityUnits& units) {
    qunits_ = units;
    return *this;
}

inline CellularQualityUnits CellularSignalQuality::qualityUnits() const {
    if (qunits_ != CellularQualityUnits::NONE) {
        return qunits_;
    }
    switch (act_) {
        case CellularAccessTechnology::GSM:
        case CellularAccessTechnology::GSM_COMPACT:
        case CellularAccessTechnology::GSM_EDGE: {
            return CellularQualityUnits::RXQUAL;
        }
        case CellularAccessTechnology::UTRAN:
        case CellularAccessTechnology::UTRAN_HSDPA:
        case CellularAccessTechnology::UTRAN_HSUPA:
        case CellularAccessTechnology::UTRAN_HSDPA_HSUPA: {
            return CellularQualityUnits::ECN0;
        }
        case CellularAccessTechnology::LTE:
        case CellularAccessTechnology::LTE_CAT_M1:
        case CellularAccessTechnology::LTE_NB_IOT: {
            return CellularQualityUnits::RSRQ;
        }
        default: {
            return CellularQualityUnits::NONE;
        }
    }
}

} // particle
