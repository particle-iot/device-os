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

#include "application.h"
#include "unit-test/unit-test.h"

#if Wiring_Cellular == 1

// Global variable to indicate a connection attempt
volatile int g_state_conn_attempt = 0;

static void nwstatus_callback_handler(system_event_t ev, int param) {
    if (param == network_status_connecting){
        g_state_conn_attempt = 1;
    }
}

test(CELLULAR_CONN_01_conn_after_off)
{
    // Connect to Particle cloud
    Particle.connect();
    delay(30000);
    // Power off the cell radio
    Cellular.off();
    delay(60000);
    // Callback handler for network_status
    System.on(network_status, nwstatus_callback_handler);
    // clear g_state_conn_attempt just-in-case
    g_state_conn_attempt = 0;
    // Check that Particle.connect() attempts to work after the delay
    Particle.connect();
    // Wait sometime for Particle.connect() to try
    delay(30000);
    // Verify that a connection attempt has been made
    assertEqual(g_state_conn_attempt, 1);
}

#endif // Wiring_Cellular == 1