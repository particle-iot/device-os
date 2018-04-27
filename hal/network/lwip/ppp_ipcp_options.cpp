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

#include "ppp_ipcp_options.h"
#include "logging.h"
#include <cstring>

LOG_SOURCE_CATEGORY("net.ppp.ipcp");

using namespace particle::net::ppp::ipcp;

void CommonConfigurationOptionIpAddress::setLocalAddress(const ip4_addr_t& addr) {
  ip4_addr_copy(local, addr);
}

void CommonConfigurationOptionIpAddress::setPeerAddress(const ip4_addr_t& addr) {
  ip4_addr_copy(peer, addr);
}

ip4_addr_t CommonConfigurationOptionIpAddress::getLocalAddress() {
  return local;
}

ip4_addr_t CommonConfigurationOptionIpAddress::getPeerAddress() {
  return peer;
}

void CommonConfigurationOptionIpAddress::reset() {
  ConfigurationOption::reset();
  ip4_addr_set_zero(&local);
  ip4_addr_set_zero(&peer);
  ip4_addr_set_zero(&peerIdea);
}

bool CommonConfigurationOptionIpAddress::validate(uint8_t* buf, size_t len) {
  if (len >= length) {
    uint8_t rid = buf[0];
    uint8_t rlength = buf[1];
    if (rid == id && rlength == length) {
      return true;
    }
  }

  return false;
}

bool CommonConfigurationOptionIpAddress::compareLocal(uint8_t* buf, size_t len) {
  ip4_addr_t addr;
  ip4_addr_set_u32(&addr, *(uint32_t*)(buf + 2));
  if (ip4_addr_cmp(&addr, &local)) {
    return true;
  }

  return false;
}

bool CommonConfigurationOptionIpAddress::comparePeer(uint8_t* buf, size_t len) {
  ip4_addr_t addr;
  ip4_addr_set_u32(&addr, *(uint32_t*)(buf + 2));
  if (ip4_addr_cmp(&addr, &peer)) {
    return true;
  }

  return false;
}

/* Local -> Remote */
int CommonConfigurationOptionIpAddress::sendConfigureReq(uint8_t* buf, size_t len) {
  if (len >= length) {
    buf[0] = id;
    buf[1] = length;
    *((uint32_t*)(buf + 2)) = ip4_addr_get_u32(&local);
    return length;
  }

  return 0;
}

int CommonConfigurationOptionIpAddress::recvConfigureRej(uint8_t* buf, size_t len) {
  if (validate(buf, len) && compareLocal(buf, len)) {
    stateLocal = CONFIGURATION_OPTION_STATE_REJ;
    return length;
  }
  /* error */
  return 0;
}

int CommonConfigurationOptionIpAddress::recvConfigureAck(uint8_t* buf, size_t len) {
  if (validate(buf, len) && compareLocal(buf, len)) {
    stateLocal = CONFIGURATION_OPTION_STATE_ACK;
    return length;
  }
  /* error */
  return 0;
}

int CommonConfigurationOptionIpAddress::recvConfigureNak(uint8_t* buf, size_t len) {
  if (validate(buf, len) && !compareLocal(buf, len)) {
    stateLocal = CONFIGURATION_OPTION_STATE_NAK;
    if (ip4_addr_isany_val(local)) {
      // Accept remote idea
      ip4_addr_set_u32(&local, *(uint32_t*)(buf + 2));
    }
    return length;
  }
  /* error */
  return 0;
}

/* Remote -> Local */
int CommonConfigurationOptionIpAddress::recvConfigureReq(uint8_t* buf, size_t len) {
  if (validate(buf, len)) {
    if (comparePeer(buf, len)) {
      if (!ip4_addr_isany_val(peer)) {
        statePeer = CONFIGURATION_OPTION_STATE_ACK;
      } else {
        statePeer = CONFIGURATION_OPTION_STATE_REJ;
        ip4_addr_set_u32(&peerIdea, *(uint32_t*)(buf + 2));
      }
    } else {
      if (!ip4_addr_isany_val(peer)) {
        statePeer = CONFIGURATION_OPTION_STATE_NAK;
      } else {
        ip4_addr_set_u32(&peerIdea, *(uint32_t*)(buf + 2));
        if (!ip4_addr_isany_val(peerIdea)) {
          statePeer = CONFIGURATION_OPTION_STATE_NAK;
          ip4_addr_copy(peer, peerIdea);
        } else {
          statePeer = CONFIGURATION_OPTION_STATE_REJ;
        }
      }
    }
    return length;
  }
  return 0;
}

int CommonConfigurationOptionIpAddress::sendConfigureRej(uint8_t* buf, size_t len) {
  if (len >= length) {
    buf[0] = id;
    buf[1] = length;
    *((uint32_t*)(buf + 2)) = ip4_addr_get_u32(&peerIdea);
    return length;
  }

  return 0;
}

int CommonConfigurationOptionIpAddress::sendConfigureAck(uint8_t* buf, size_t len) {
  if (len >= length) {
    buf[0] = id;
    buf[1] = length;
    *((uint32_t*)(buf + 2)) = ip4_addr_get_u32(&peer);
    return length;
  }

  return 0;
}

int CommonConfigurationOptionIpAddress::sendConfigureNak(uint8_t* buf, size_t len) {
  if (len >= length) {
    buf[0] = id;
    buf[1] = length;
    *((uint32_t*)(buf + 2)) = ip4_addr_get_u32(&peer);
    return length;
  }

  return 0;
}

bool UnknownConfigurationOption::validate(uint8_t* buf, size_t len) {
  if (len >= 2) {
    if (buf[1] <= len) {
      return true;
    }
  }
  return false;
}

void UnknownConfigurationOption::reset() {
  ConfigurationOption::reset();
  data = nullptr;
  length = 0;
}

int UnknownConfigurationOption::recvConfigureReq(uint8_t* buf, size_t len) {
  if (validate(buf, len)) {
    id = buf[0];
    length = buf[1];

    data = buf;
    statePeer = CONFIGURATION_OPTION_STATE_REJ;
    return length;
  }

  statePeer = CONFIGURATION_OPTION_STATE_ERR;

  return 0;
}

int UnknownConfigurationOption::sendConfigureRej(uint8_t* buf, size_t len) {
  if (len >= length && data != nullptr) {
    memmove(buf, data, length);
    return length;
  }

  return 0;
}