/*
 * Copyright (c) 2024 Particle Industries, Inc.  All rights reserved.
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

#ifdef ENABLE_MUON_DETECTION

#include "Particle.h"

namespace particle {

class BoardConfig {
public:
    BoardConfig();
    ~BoardConfig();

    static BoardConfig* instance();

    bool process(JSONValue config);
    char* reply();
    size_t replySize();
    
private:
    JSONBufferWriter replyWriter_;
    char replyBuffer_[64];
    bool isMuon_;
    bool isMuonDetected_;

    void detectBaseBoard();
    void configureBaseBoard(JSONValue value);
    void detectI2cSlaves(bool force = true);
    int configure(bool muon);
};

} // particle

#endif // ENABLE_MUON_DETECTION
