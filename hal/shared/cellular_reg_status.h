/*
 ******************************************************************************
 *  Copyright (c) 2019 Particle Industries, Inc.  All rights reserved.
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

#include "timer_hal.h"

/** Cellular Registration Status
 * To run intervention commands and help improve/speed-up registration process
 */

class CellularRegistrationStatus {
public:
    enum Status {
        NONE = -1,
        NOT_REGISTERING = 0,
        HOME = 1,
        SEARCHING = 2,
        DENIED = 3,
        UNKNOWN = 4,
        ROAMING = 5
    };

    CellularRegistrationStatus() = default;
    CellularRegistrationStatus(Status stat, system_tick_t started, system_tick_t updated);

    /** Updates the latest registration status as reported by the module, and saves into Status stat_
    */
    void status(Status stat, system_tick_t ts);
    void status(Status stat);

    /** Resets the status, reg started time, and reg updated time
     */
    void reset();

    /** Gets current registration status
     */
    Status status() const;

    /** Gets the latest updated registration time
     */
    system_tick_t updated() const;

    /** Gets the start time of the registration for a given status
     *  For example, if registration status is changed from Searching to Denied, started_ time is updated
     */
    system_tick_t started() const;

    /** Checks for home/roaming registered
     */
    bool registered() const;

    /** Duration from which the reg state has changed last time (governed by started_)
     */
    system_tick_t duration() const;

    /** Checks if a given registration state has been sticky
     */
    bool sticky() const;

    /** Decodes reg status from raw AT value into Status enum (see above)
     */
    static CellularRegistrationStatus::Status decodeAtStatus(int status);

private:
    Status stat_ = NONE;
    system_tick_t updated_ = 0;
    system_tick_t started_ = 0;
};

// CellularRegistrationStatus

inline CellularRegistrationStatus::CellularRegistrationStatus(Status stat, system_tick_t started, system_tick_t updated)
        : stat_{stat},
          updated_{updated},
          started_{started} {
}

inline void CellularRegistrationStatus::status(Status stat, system_tick_t ts) {
    if (stat_ != stat) {
        stat_ = stat;
        started_ = ts;
    }
    updated_ = ts;
}

inline void CellularRegistrationStatus::status(Status stat) {
    status(stat, HAL_Timer_Get_Milli_Seconds());
}

inline void CellularRegistrationStatus::reset() {
    stat_ = NONE;
    updated_ = 0;
    started_ = 0;
}

inline CellularRegistrationStatus::Status CellularRegistrationStatus::status() const {
    return stat_;
}

inline system_tick_t CellularRegistrationStatus::updated() const {
    return updated_;
}

inline system_tick_t CellularRegistrationStatus::started() const {
    return started_;
}

inline system_tick_t CellularRegistrationStatus::duration() const {
    return HAL_Timer_Get_Milli_Seconds() - started_;
}

inline bool CellularRegistrationStatus::registered() const {
    return stat_ == HOME || stat_ == ROAMING;
}

inline bool CellularRegistrationStatus::sticky() const {
    return updated_ > 0 && updated_ != started_;
}

inline CellularRegistrationStatus::Status CellularRegistrationStatus::decodeAtStatus(int status) {
    switch (status) {
        case CellularRegistrationStatus::NOT_REGISTERING:
        case CellularRegistrationStatus::HOME:
        case CellularRegistrationStatus::SEARCHING:
        case CellularRegistrationStatus::DENIED:
        case CellularRegistrationStatus::UNKNOWN:
        case CellularRegistrationStatus::ROAMING: {
            return static_cast<CellularRegistrationStatus::Status>(status);
        }
    }

    return CellularRegistrationStatus::NONE;
}
