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

#include "ppp_ipcp.h"

#if defined(PPP_SUPPORT) && PPP_SUPPORT

#include <lwip/dns.h>
#include "logging.h"

LOG_SOURCE_CATEGORY("net.ppp.ipcp");

using namespace particle::net::ppp;

#ifndef IP4_ADDR_ANY4
#define IP4_ADDR_ANY4 (ip_2_ip4(&ip_addr_any))
#endif /* IP4_ADDR_ANY4 */

const char detail::ipcpProtocolName[] = "IPCP";
const char detail::ipProtocolName[] = "IP";

const struct protent ipcp_protent = Ipcp::generateProtent();

Ipcp::Ipcp(ppp_pcb* pcb)
    : IpcpBase(pcb) {

  /* Register supported options */
  registerOption(new ipcp::IpAddressConfigurationOption());
  registerOption(new ipcp::IpNetmaskConfigurationOption());
  registerOption(new ipcp::PrimaryDnsServerConfigurationOption());
  registerOption(new ipcp::SecondaryDnsServerConfigurationOption());
}

Ipcp::~Ipcp() {

}

void Ipcp::enable() {
  LOG(TRACE, "IPCP: enable %d %d %d", admState_, lowerState_, state_);
  if (!admState_) {
    admState_ = true;
    if (lowerState_ && !state_) {
      open();
    }
  }
}

void Ipcp::disable() {
  LOG(TRACE, "IPCP: disable %d %d %d", admState_, lowerState_, state_);
  if (admState_) {
    admState_ = false;
    if (lowerState_ && state_) {
      close("Administrative down");
    }
  }
}

void Ipcp::setDnsEntryIndex(int idx) {
  dnsIndex_ = idx;
}

void Ipcp::init() {
  LOG(TRACE, "IPCP: init");

  fsm_init(&fsm_);
}

void Ipcp::input(uint8_t* pkt, int len) {
  LOG(TRACE, "IPCP: input %d", len);
  fsm_input(&fsm_, pkt, len);
}

void Ipcp::protocolReject() {
  LOG(TRACE, "IPCP: Protocol-Reject");
  /* RFC 1661:
   * Upon reception of a Protocol-Reject, the implementation MUST stop
   * sending packets of the indicated protocol at the earliest
   * opportunity.
   */
  fsm_protreject(&fsm_);
}

void Ipcp::lowerUp() {
  LOG(TRACE, "IPCP: lowerUp");
  lowerState_= true;
  fsm_lowerup(&fsm_);
}

void Ipcp::lowerDown() {
  LOG(TRACE, "IPCP: lowerDown");
  lowerState_ = false;
  fsm_lowerdown(&fsm_);
}

void Ipcp::open() {
  LOG(TRACE, "IPCP: open");
  if (admState_) {
    fsm_open(&fsm_);
  } else {
    LOG(TRACE, "IPCP is not allowed to come up");
  }
}

void Ipcp::close(const char* reason) {
  LOG(TRACE, "ipcp:close");
  fsm_close(&fsm_, reason);
}

/* State machine callbacks */
/* Reset our Configuration Information */
void Ipcp::resetConfigurationInformation() {
  LOG(TRACE, "IPCP: reset ci");

  const auto& conf = config_;

  forEachOption([&conf](auto opt) {
    opt->reset();

    switch (opt->id) {
      case ipcp::CONFIGURATION_OPTION_IP_COMPRESSION_PROTOCOL: {
        // TODO:
        break;
      }
      case ipcp::CONFIGURATION_OPTION_IP_ADDRESS: {
        auto o = static_cast<ipcp::CommonConfigurationOptionIpAddress*>(opt);
        o->setLocalAddress(conf.localAddress);
        o->setPeerAddress(conf.peerAddress);
        break;
      }
      case ipcp::CONFIGURATION_OPTION_PRIMARY_DNS_SERVER: {
        auto o = static_cast<ipcp::CommonConfigurationOptionIpAddress*>(opt);
        o->setPeerAddress(conf.primaryDns);
        break;
      }
      case ipcp::CONFIGURATION_OPTION_SECONDARY_DNS_SERVER: {
        auto o = static_cast<ipcp::CommonConfigurationOptionIpAddress*>(opt);
        o->setPeerAddress(conf.secondaryDns);
        break;
      }
      case ipcp::CONFIGURATION_OPTION_IP_NETMASK: {
        auto o = static_cast<ipcp::CommonConfigurationOptionIpAddress*>(opt);
        o->setPeerAddress(conf.peerNetmask);
        break;
      }
    }
  });
}

/* Length of our Configuration Information */
int Ipcp::getConfigurationInformationLength() {
  LOG(TRACE, "IPCP: get ci length");
  size_t len = 0;

  forEachOption([&len](auto opt) {
    if (opt->flagsLocal & CONFIGURATION_OPTION_FLAG_REQUEST) {
      if (opt->stateLocal == CONFIGURATION_OPTION_STATE_NONE || opt->stateLocal == CONFIGURATION_OPTION_STATE_NAK) {
        len += opt->length;
      }
    }
  });

  LOG(TRACE, "IPCP: Our CI length: %lu", len);
  return len;
}

/* Add our Configuration Information */
void Ipcp::addConfigurationInformation(uint8_t* buf, int* len) {
  int available = *len;
  forEachOption([buf, &available](auto opt) mutable {
    if (opt->flagsLocal & CONFIGURATION_OPTION_FLAG_REQUEST) {
      if (opt->stateLocal == CONFIGURATION_OPTION_STATE_NONE || opt->stateLocal == CONFIGURATION_OPTION_STATE_NAK) {
        int l = opt->sendConfigureReq(buf, available);
        buf += l;
        available -= l;
      }
    }
  });

  *len -= available;
}

/* ACK our Configuration Information */
int Ipcp::ackConfigurationInformation(uint8_t* buf, int len) {
  LOG(TRACE, "IPCP: ack ci");

  while (len > 0) {
    uint8_t id = *buf;
    auto opt = findOption(id);
    if (opt != nullptr) {
      int l = opt->recvConfigureAck(buf, len);
      if (l == 0) {
        break;
      }
      buf += l;
      len -= l;
    } else {
      LOG(ERROR, "Peer ACK'd an option unknown to us");
      break;
    }
  }

  return len == 0;
}

/* NAK our Configuration Information */
int Ipcp::nakConfigurationInformation(uint8_t* buf, int len, int treatAsReject) {
  LOG(TRACE, "IPCP: nak ci");

  while (len > 0) {
    uint8_t id = *buf;
    auto opt = findOption(id);
    if (opt != nullptr) {
      int l = opt->recvConfigureNak(buf, len);
      if (l == 0) {
        break;
      }
      buf += l;
      len -= l;
    } else {
      LOG(ERROR, "Peer NAK'd an option unknown to us");
      break;
    }
  }

  return len == 0;
}

/* Reject our Configuration Information */
int Ipcp::rejectConfigurationInformation(uint8_t* buf, int len) {
  LOG(TRACE, "IPCP: reject ci");

  while (len > 0) {
    uint8_t id = *buf;
    auto opt = findOption(id);
    if (opt != nullptr) {
      int l = opt->recvConfigureRej(buf, len);
      if (l == 0) {
        break;
      }
      buf += l;
      len -= l;
    } else {
      LOG(ERROR, "Peer REJECTED an option unknown to us");
      break;
    }
  }

  return len == 0;
}

/* Request peer's Configuration Information */
int Ipcp::requestConfigurationInformation(uint8_t* buf, int* len, int rejectIfDisagree) {
  LOG(TRACE, "IPCP: request ci");
  ConfigurationOptionState resultingState = CONFIGURATION_OPTION_STATE_ACK;

  int available = *len;
  uint8_t* orig = buf;
  uint8_t* writePtr = buf;
  size_t written = 0;

  ipcp::UnknownConfigurationOption unknownOpt;
  ConfigurationOption* lastOpt = nullptr;

  while (available > 0) {
    uint8_t id = *buf;
    auto opt = findOption(id);
    if (opt != nullptr) {
      int l = opt->recvConfigureReq(buf, available);
      if (l == 0) {
        break;
      }
      buf += l;
      available -= l;

      lastOpt = opt;
    } else {
      int l = unknownOpt.recvConfigureReq(buf, available);
      if (l == 0) {
        break;
      }
      buf += l;
      available -= l;

      lastOpt = &unknownOpt;
    }

    if (lastOpt->statePeer > resultingState) {
      resultingState = lastOpt->statePeer;

      /* Start writing from the beginning */
      writePtr = orig;
      written = 0;
    }

    if (lastOpt->statePeer == resultingState) {
      switch (resultingState) {
        case CONFIGURATION_OPTION_STATE_ACK: {
          writePtr += lastOpt->sendConfigureAck(writePtr, (*len) - written);
          break;
        }
        case CONFIGURATION_OPTION_STATE_NAK: {
          writePtr += lastOpt->sendConfigureNak(writePtr, (*len) - written);
          break;
        }
        case CONFIGURATION_OPTION_STATE_REJ: {
          writePtr += lastOpt->sendConfigureRej(writePtr, (*len) - written);
          break;
        }
        default: {
          break;
        }
      }
      written = writePtr - orig;
    }
  }

  *len = written;

  return resultingState;
}

/* Called when fsm reaches PPP_FSM_OPENED state */
void Ipcp::up() {
  LOG(TRACE, "IPCP: up");

  auto ip = getNegotiatedLocalAddress();
  auto peer = getNegotiatedPeerAddress();

#ifndef PPP_IPCP_NO_DEFAULT_PEER_IP_ADDRESS
  if (ip4_addr_isany_val(peer)) {
    /* Default to 10.64.64.64 */
    ip4_addr_set_u32(&peer, lwip_htonl(0x0a404040));
  }
#endif

  if (!ip4_addr_isany_val(ip) && !ip4_addr_isany_val(peer)) {
    sifup(pcb_);

    auto mask = getNegotiatedNetmask();
    netif_set_addr(pcb_->netif, &ip, &mask, &peer);

    auto pdns = getNegotiatedPrimaryDns();
    auto sdns = getNegotiatedSecondaryDns();
    if (!ip4_addr_isany_val(pdns)) {
      ip_addr_t tmp;
      ip_addr_copy_from_ip4(tmp, pdns);
      dns_setserver(dnsIndex_, &tmp);
    }

    if (!ip4_addr_isany_val(sdns)) {
      ip_addr_t tmp;
      ip_addr_copy_from_ip4(tmp, sdns);
      dns_setserver(dnsIndex_ + 1, &tmp);
    }

    if (!state_) {
      state_ = true;
      np_up(pcb_, fsm_.protocol);
    }
  } else {
    // Teardown
    close("Failed to negotiate local or peer IP");
  }
}

/* Called when fsm leaves PPP_FSM_OPENED state */
void Ipcp::down() {
  LOG(TRACE, "IPCP: down");

  sifdown(pcb_);

  netif_set_addr(pcb_->netif, IP4_ADDR_ANY4, IP4_ADDR_BROADCAST, IP4_ADDR_ANY4);

  if (state_) {
    state_ = false;
    np_down(pcb_, fsm_.protocol);
  }
}

/* Called when we want the lower layer */
void Ipcp::starting() {
  LOG(TRACE, "IPCP: starting");
}

/* Called when we don't want the lower layer */
void Ipcp::finished() {
  LOG(TRACE, "IPCP: finished");
}

/* Called when unknown code received */
int Ipcp::extCode(int code, int id, uint8_t* buf, int len) {
  LOG(TRACE, "IPCP: ext code %d %d", code, id);
  return 0;
}

void Ipcp::setLocalAddress(const ip4_addr_t& addr) {
  ip4_addr_copy(config_.localAddress, addr);
}

void Ipcp::setPeerAddress(const ip4_addr_t& addr) {
  ip4_addr_copy(config_.peerAddress, addr);
}

void Ipcp::setNetmask(const ip4_addr_t& netmask) {
  ip4_addr_copy(config_.peerNetmask, netmask);
}

void Ipcp::setPrimaryDns(const ip4_addr_t& dns) {
  ip4_addr_copy(config_.primaryDns, dns);
}

void Ipcp::setSecondaryDns(const ip4_addr_t& dns) {
  ip4_addr_copy(config_.secondaryDns, dns);
}

ip4_addr_t Ipcp::getLocalAddress() {
  return config_.localAddress;
}

ip4_addr_t Ipcp::getPeerAddress() {
  return config_.peerAddress;
}

ip4_addr_t Ipcp::getNetmask() {
  return config_.peerNetmask;
}

ip4_addr_t Ipcp::getPrimaryDns() {
  return config_.primaryDns;
}

ip4_addr_t Ipcp::getSecondaryDns() {
  return config_.secondaryDns;
}

ip4_addr_t Ipcp::getNegotiatedLocalAddress() {
  using namespace ipcp;
  auto opt = static_cast<CommonConfigurationOptionIpAddress*>(findOption(CONFIGURATION_OPTION_IP_ADDRESS));
  ip4_addr_t ret = *IP4_ADDR_ANY4;
  if (opt->stateLocal == CONFIGURATION_OPTION_STATE_ACK) {
    ret = opt->getLocalAddress();
  }
  return ret;
}

ip4_addr_t Ipcp::getNegotiatedPeerAddress() {
  using namespace ipcp;
  auto opt = static_cast<CommonConfigurationOptionIpAddress*>(findOption(CONFIGURATION_OPTION_IP_ADDRESS));
  ip4_addr_t ret = *IP4_ADDR_ANY4;
  if (opt->statePeer == CONFIGURATION_OPTION_STATE_ACK) {
    ret = opt->getPeerAddress();
  }
  return ret;
}

ip4_addr_t Ipcp::getNegotiatedNetmask() {
  using namespace ipcp;
  auto opt = static_cast<CommonConfigurationOptionIpAddress*>(findOption(CONFIGURATION_OPTION_IP_NETMASK));
  ip4_addr_t ret = *IP4_ADDR_BROADCAST;
  if (opt->stateLocal == CONFIGURATION_OPTION_STATE_ACK) {
    ret = opt->getLocalAddress();
  }
  return ret;
}

ip4_addr_t Ipcp::getNegotiatedPrimaryDns() {
  using namespace ipcp;
  auto opt = static_cast<CommonConfigurationOptionIpAddress*>(findOption(CONFIGURATION_OPTION_PRIMARY_DNS_SERVER));
  ip4_addr_t ret = *IP4_ADDR_ANY4;
  if (opt->stateLocal == CONFIGURATION_OPTION_STATE_ACK) {
    ret = opt->getLocalAddress();
  }
  return ret;
}

ip4_addr_t Ipcp::getNegotiatedSecondaryDns() {
  using namespace ipcp;
  auto opt = static_cast<CommonConfigurationOptionIpAddress*>(findOption(CONFIGURATION_OPTION_SECONDARY_DNS_SERVER));
  ip4_addr_t ret = *IP4_ADDR_ANY4;
  if (opt->stateLocal == CONFIGURATION_OPTION_STATE_ACK) {
    ret = opt->getLocalAddress();
  }
  return ret;
}

#endif // defined(PPP_SUPPORT) && PPP_SUPPORT
