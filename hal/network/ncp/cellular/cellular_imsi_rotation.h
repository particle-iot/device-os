/*
 * Copyright (c) 2026 Particle Industries, Inc.  All rights reserved.
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

#include <cstring>

#include "system_tick_hal.h"

#define CELLULAR_IMSI_CHECK_INTERVAL (60 * 1000)
#define CELLULAR_IMSI_ROTATION_ATTEMPTS (3)

namespace particle {

/**
 * IMSI rotation for modems whose own rotation triggers never fire in Limited Service, so a
 * multi-IMSI SIM sits on one IMSI forever. Decides when to force the applet on to the next IMSI
 * and when to clear EF_FPLMN.
 *
 * EF_FPLMN filling up while the SIM is deactivated is expected, but once it is activated the full
 * table blocks every suitable network. It gets cleared at boot and at each registration timeout. Not
 * on an IMSI change, the applet's REFRESH already reloads it, and when the applet switched because
 * every network was forbidden it cleared the table itself.
 *
 * The applet does not always act on the rotation command. When it says so, the command is sent
 * again on the next IMSI check that finds the IMSI unchanged, up to CELLULAR_IMSI_ROTATION_ATTEMPTS
 * per window.
 *
 * millis() is passed in so this can be accelerated in tests.
 */
class CellularImsiRotation {
public:
    static const size_t IMSI_SIZE = 16; // 15 digits

    // "+QUSIM: 1", the SIM was (re)initialised. Brings the next IMSI check forward.
    void simRefreshed() {
        refreshed_ = true;
    }

    bool imsiCheckDue(system_tick_t now) const {
        return refreshed_ || now - lastCheck_ >= CELLULAR_IMSI_CHECK_INTERVAL;
    }

    // Result of an IMSI check, nullptr if it could not be read. Returns true if the IMSI changed.
    bool imsiChecked(const char* imsi, system_tick_t now) {
        lastCheck_ = now;
        refreshed_ = false;
        if (!imsi || !imsi[0]) {
            return false;
        }
        if (!imsi_[0]) {
            // Nothing to compare the first one against
            setImsi(imsi);
            return false;
        }
        if (!strcmp(imsi, imsi_)) {
            if (rotationRetry_) {
                rotationRetry_ = false;
                rotationDue_ = true;
            }
            return false;
        }
        setImsi(imsi);
        changed_ = true;
        rotationRetry_ = false;
        return true;
    }

    const char* imsi() const {
        return imsi_;
    }

    void windowStarted() {
        imsi_[0] = '\0';
        changed_ = false;
        rotationRetry_ = false;
        rotationAttempts_ = 0;
    }

    void registrationTimedOut() {
        fplmnClearDue_ = true;
        rotationDue_ = !changed_;
        rotationRetry_ = false;
    }

    void registrationRestarted() {
        fplmnClearDue_ = true;
    }

    bool fplmnClearDue() const {
        return fplmnClearDue_;
    }

    void fplmnCleared() {
        fplmnClearDue_ = false;
    }

    bool rotationDue() const {
        return rotationDue_;
    }

    void rotationForced(bool started) {
        rotationDue_ = false;
        rotationAttempts_++;
        rotationRetry_ = !started && rotationAttempts_ < CELLULAR_IMSI_ROTATION_ATTEMPTS;
    }

    void rotationAbandoned() {
        rotationDue_ = false;
        rotationRetry_ = false;
    }

    unsigned rotationAttempts() const {
        return rotationAttempts_;
    }

private:
    void setImsi(const char* imsi) {
        strncpy(imsi_, imsi, sizeof(imsi_) - 1);
        imsi_[sizeof(imsi_) - 1] = '\0';
    }

    char imsi_[IMSI_SIZE] = {};
    system_tick_t lastCheck_ = 0;
    bool refreshed_ = false;
    bool changed_ = false;
    bool fplmnClearDue_ = true; // Boot
    bool rotationDue_ = false;
    bool rotationRetry_ = false;
    unsigned rotationAttempts_ = 0;
};

} // namespace particle
