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

#pragma once

#include "ncp_client.h"
#include "cellular_network_manager.h"
#include "cellular_hal_cellular_global_identity.h"

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

    CellularNcpClientConfig& ncpIdentifier(MeshNCPIdentifier ident);
    MeshNCPIdentifier ncpIdentifier() const;

private:
    SimType simType_;
    MeshNCPIdentifier ident_;
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
    EC_GSM_IOT = 8,
    E_UTRAN = 9
};

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
};

inline CellularNcpClientConfig::CellularNcpClientConfig() :
        simType_(SimType::INTERNAL),
        ident_(MESH_NCP_UNKNOWN) {
}

inline CellularNcpClientConfig& CellularNcpClientConfig::simType(SimType type) {
    simType_ = type;
    return *this;
}

inline SimType CellularNcpClientConfig::simType() const {
    return simType_;
}


inline CellularNcpClientConfig& CellularNcpClientConfig::ncpIdentifier(MeshNCPIdentifier ident) {
    ident_ = ident;
    return *this;
}

inline MeshNCPIdentifier CellularNcpClientConfig::ncpIdentifier() const {
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
        case CellularAccessTechnology::EC_GSM_IOT:
        case CellularAccessTechnology::E_UTRAN: {
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
        case CellularAccessTechnology::EC_GSM_IOT:
        case CellularAccessTechnology::E_UTRAN: {
            return CellularQualityUnits::RSRQ;
        }
        default: {
            return CellularQualityUnits::NONE;
        }
    }
}

} // particle
