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

#ifndef HAL_NETWORK_LWIP_PPP_IPCP_H
#define HAL_NETWORK_LWIP_PPP_IPCP_H

#include "ppp_ncp.h"

#if defined(PPP_SUPPORT) && PPP_SUPPORT

#include <netif/ppp/ipcp.h>
#include "ppp_ipcp_options.h"

#ifdef __cplusplus

namespace particle { namespace net { namespace ppp {

namespace detail {
extern const char ipcpProtocolName[];
extern const char ipProtocolName[];
} /* namespace detail */

using IpcpBase = Ncp<PPP_IPCP, &ppp_pcb::ipcp_ctx, detail::ipcpProtocolName, detail::ipProtocolName>;

class Ipcp : public IpcpBase {
public:
  Ipcp(ppp_pcb* pcb);
  virtual ~Ipcp();

  virtual void enable() override;
  virtual void disable() override;

  void setDnsEntryIndex(int idx);

  void setLocalAddress(const ip4_addr_t& addr);
  void setPeerAddress(const ip4_addr_t& addr);
  void setNetmask(const ip4_addr_t& netmask);
  void setPrimaryDns(const ip4_addr_t& dns);
  void setSecondaryDns(const ip4_addr_t& dns);

  ip4_addr_t getLocalAddress();
  ip4_addr_t getPeerAddress();
  ip4_addr_t getNetmask();
  ip4_addr_t getPrimaryDns();
  ip4_addr_t getSecondaryDns();

  /* Protocol callbacks */
  /* Initialization procedure */
  virtual void init() override;
  /* Process a received packet */
  virtual void input(uint8_t* pkt, int len) override;
  /* Process a received protocol-reject */
  virtual void protocolReject() override;
  /* Lower layer has come up */
  virtual void lowerUp() override;
  /* Lower layer has gone down */
  virtual void lowerDown() override;
  /* Open the protocol */
  virtual void open() override;
  /* Close the protocol */
  virtual void close(const char* reason) override;

  /* State machine callbacks */
  /* Reset our Configuration Information */
  virtual void resetConfigurationInformation() override;
  /* Length of our Configuration Information */
  virtual int getConfigurationInformationLength() override;
  /* Add our Configuration Information */
  virtual void addConfigurationInformation(uint8_t* buf, int* len) override;
  /* ACK our Configuration Information */
  virtual int ackConfigurationInformation(uint8_t* buf, int len) override;
  /* NAK our Configuration Information */
  virtual int nakConfigurationInformation(uint8_t* buf, int len, int treatAsReject) override;
  /* Reject our Configuration Information */
  virtual int rejectConfigurationInformation(uint8_t* buf, int len) override;
  /* Request peer's Configuration Information */
  virtual int requestConfigurationInformation(uint8_t* buf, int* len, int rejectIfDisagree) override;
  /* Called when fsm reaches PPP_FSM_OPENED state */
  virtual void up() override;
  /* Called when fsm leaves PPP_FSM_OPENED state */
  virtual void down() override;
  /* Called when we want the lower layer */
  virtual void starting() override;
  /* Called when we don't want the lower layer */
  virtual void finished() override;
  /* Called when unknown code received */
  virtual int extCode(int code, int id, uint8_t* buf, int len) override;

private:
  ip4_addr_t getNegotiatedLocalAddress();
  ip4_addr_t getNegotiatedPeerAddress();
  ip4_addr_t getNegotiatedNetmask();
  ip4_addr_t getNegotiatedPrimaryDns();
  ip4_addr_t getNegotiatedSecondaryDns();

private:
  bool lowerState_ = false;
  bool admState_ = false;
  bool state_ = false;
  int dnsIndex_ = 0;

  struct IpcpConfiguration {
    ip4_addr_t localAddress;
    ip4_addr_t peerAddress;

    ip4_addr_t peerNetmask;

    ip4_addr_t primaryDns;
    ip4_addr_t secondaryDns;

    uint32_t requestNetmask : 1;
    uint32_t requestDns : 1;
  };

  IpcpConfiguration config_ = {};
};

} } } /* namespace particle::net::ppp */

#endif /* __cplusplus */

#endif // defined(PPP_SUPPORT) && PPP_SUPPORT

#endif /* HAL_NETWORK_LWIP_PPP_IPCP_H */
