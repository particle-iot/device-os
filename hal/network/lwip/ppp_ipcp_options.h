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

#ifndef HAL_NETWORK_LWIP_PPP_IPCP_OPTIONS_H
#define HAL_NETWORK_LWIP_PPP_IPCP_OPTIONS_H

#include <cstddef>
#include <cstdint>
#include <lwip/ip4_addr.h>
#include "ppp_configuration_option.h"

#ifdef __cplusplus

namespace particle { namespace net { namespace ppp {

namespace ipcp {

enum ConfigurationOptionId {
  CONFIGURATION_OPTION_NONE                    = 0,
  CONFIGURATION_OPTION_IP_COMPRESSION_PROTOCOL = 2,
  CONFIGURATION_OPTION_IP_ADDRESS              = 3,
  CONFIGURATION_OPTION_PRIMARY_DNS_SERVER      = 129,
  CONFIGURATION_OPTION_SECONDARY_DNS_SERVER    = 131,
  CONFIGURATION_OPTION_IP_NETMASK              = 144
};

static const size_t OPTION_HEADER_SIZE = 2;

struct CommonConfigurationOptionIpAddress : public ConfigurationOption {
  CommonConfigurationOptionIpAddress(int id)
      : ConfigurationOption(id, OPTION_HEADER_SIZE + 4) {
  }

  virtual void setLocalAddress(const ip4_addr_t& addr);
  virtual void setPeerAddress(const ip4_addr_t& addr);

  virtual ip4_addr_t getLocalAddress();
  virtual ip4_addr_t getPeerAddress();

  virtual void reset() override;

  virtual bool validate(uint8_t* buf, size_t len) override;
  virtual bool compareLocal(uint8_t* buf, size_t len);
  virtual bool comparePeer(uint8_t* buf, size_t len);

  /* Local -> Remote */
  virtual int sendConfigureReq(uint8_t* buf, size_t len) override;
  virtual int recvConfigureRej(uint8_t* buf, size_t len) override;
  virtual int recvConfigureAck(uint8_t* buf, size_t len) override;
  virtual int recvConfigureNak(uint8_t* buf, size_t len) override;

  /* Remote -> Local */
  virtual int recvConfigureReq(uint8_t* buf, size_t len) override;
  virtual int sendConfigureRej(uint8_t* buf, size_t len) override;
  virtual int sendConfigureAck(uint8_t* buf, size_t len) override;
  virtual int sendConfigureNak(uint8_t* buf, size_t len) override;

  ip4_addr_t local = {};
  ip4_addr_t peer = {};
  ip4_addr_t peerIdea = {};
};

struct IpAddressConfigurationOption : public CommonConfigurationOptionIpAddress {
  IpAddressConfigurationOption()
      : CommonConfigurationOptionIpAddress(CONFIGURATION_OPTION_IP_ADDRESS) {
  }
};

struct IpNetmaskConfigurationOption : public CommonConfigurationOptionIpAddress {
  IpNetmaskConfigurationOption()
      : CommonConfigurationOptionIpAddress(CONFIGURATION_OPTION_IP_NETMASK) {
  }
};

struct PrimaryDnsServerConfigurationOption : public CommonConfigurationOptionIpAddress {
  PrimaryDnsServerConfigurationOption()
      : CommonConfigurationOptionIpAddress(CONFIGURATION_OPTION_PRIMARY_DNS_SERVER) {
  }
};

struct SecondaryDnsServerConfigurationOption : public CommonConfigurationOptionIpAddress {
  SecondaryDnsServerConfigurationOption()
      : CommonConfigurationOptionIpAddress(CONFIGURATION_OPTION_SECONDARY_DNS_SERVER) {
  }
};

struct UnknownConfigurationOption : public ConfigurationOption {
  UnknownConfigurationOption()
      : ConfigurationOption(0, 0) {
  }

  virtual bool validate(uint8_t* buf, size_t len) override;

  virtual void reset() override;

  /* Local -> Remote */
  virtual int sendConfigureReq(uint8_t* buf, size_t len) override {
    return 0;
  }
  virtual int recvConfigureRej(uint8_t* buf, size_t len) override {
    return 0;
  }
  virtual int recvConfigureAck(uint8_t* buf, size_t len) override {
    return 0;
  }
  virtual int recvConfigureNak(uint8_t* buf, size_t len) override {
    return 0;
  }

  /* Remote -> Local */
  virtual int recvConfigureReq(uint8_t* buf, size_t len) override;
  virtual int sendConfigureRej(uint8_t* buf, size_t len) override;
  virtual int sendConfigureAck(uint8_t* buf, size_t len) override {
    return sendConfigureRej(buf, len);
  }

  virtual int sendConfigureNak(uint8_t* buf, size_t len) override {
    return sendConfigureRej(buf, len);
  }

  uint8_t* data = nullptr;
};

} /* namespace ipcp */

} } } /* namespace particle::net::ppp */

#endif /* __cplusplus */

#endif /* HAL_NETWORK_LWIP_PPP_IPCP_OPTIONS_H */
