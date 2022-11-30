/*
 * Copyright (c) 2022 Particle Industries, Inc.  All rights reserved.
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

#include <cstddef>
#include <cstdint>

namespace particle {

/**
 * Server configuration manager.
 */
class ServerConfig {
public:
    /**
     * Server settings.
     */
    struct ServerSettings {
        const char* address; ///< Server address (an IP address or domain name).
        const uint8_t* publicKey; ///< Server public key in DER format.
        size_t publicKeySize; ///< Size of the server public key.
        uint16_t port; ///< Server port. If 0, the default port will be used.

        ServerSettings() :
                address(""),
                publicKey(nullptr),
                publicKeySize(0),
                port(0) {
        }
    };

    /**
     * A request to change the server settings.
     */
    struct ServerMovedRequest: ServerSettings {
        const uint8_t* signature; ///< Request signature.
        size_t signatureSize; ///< Size of the request signature.

        ServerMovedRequest() :
                signature(nullptr),
                signatureSize(0) {
        }
    };

    /**
     * Update the current server settings.
     *
     * @param conf Server settings.
     * @return 0 if the settings were updated successfully, otherwise an error code defined by
     *         `system_error_t`.
     */
    int updateSettings(const ServerSettings& conf);

    /**
     * Validate the current server settings.
     *
     * @return 0 if the settings are valid, otherwise an error code defined by `system_error_t`.
     */
    int validateSettings() const;

    /**
     * Validate the signature of a ServerMoved request.
     *
     * @param req Request data.
     * @return 0 if the signature is valid, otherwise an error code defined by `system_error_t`.
     */
    int validateServerMovedRequest(const ServerMovedRequest& req) const;

    /**
     * Restore the default server settings.
     *
     * @return 0 if the settings were restored successfully, otherwise an error code defined by
     *         `system_error_t`.
     */
    int restoreDefaultSettings();

    /**
     * Get the global instance of this class.
     *
     * @return Instance.
     */
    static ServerConfig* instance();
};

} // namespace particle
