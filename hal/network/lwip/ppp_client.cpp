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

#undef LOG_COMPILE_TIME_LEVEL
#define LOG_COMPILE_TIME_LEVEL LOG_LEVEL_ALL

#include "ppp_client.h"

#if defined(PPP_SUPPORT) && PPP_SUPPORT

#include "service_debug.h"
extern "C" {
#include <netif/ppp/pppos.h>
}
#include <lwip/netifapi.h>
#include <netif/ppp/pppapi.h>
#include <mutex>
#include "socket_hal.h"
#include "inet_hal.h"
#include "system_error.h"
#include "lwiplock.h"
#include <lwip/stats.h>
#include <algorithm>
#include "delay_hal.h"
#include "platform_ncp.h"
#include "resolvapi.h"

LOG_SOURCE_CATEGORY("net.ppp.client");

/* Public Google DNS Servers */
const uint32_t GOOGLE_DNS_PRIMARY = 0x08080808UL;
const uint32_t GOOGLE_DNS_SECONDARY = 0x08080404UL;

using namespace particle::net::ppp;

std::once_flag Client::once_;
netif_ext_callback_t Client::netifCb_ = {};
int Client::netifClientDataIdx_ = -1;
constexpr const char* Client::eventNames_[];
constexpr const char* Client::stateNames_[];
const auto NCP_CLIENT_LCP_ECHO_INTERVAL_SECONDS_DEFAULT = 5;
const auto NCP_CLIENT_LCP_ECHO_INTERVAL_SECONDS_R510 = 240; // 4 minutes (4.25 minutes max)
const auto NCP_CLIENT_LCP_ECHO_MAX_FAILS_DEFAULT = 10;
const auto NCP_CLIENT_LCP_ECHO_MAX_FAILS_R510 = 1;

namespace {

#if !PPP_DEBUG
const char* pppPhaseToString(uint8_t phase) {
  const char* phases[] = {
    "Dead", "Master", "Holdoff", "Initialize", "SerialConn",
    "Dormant", "Establish", "Authenticate", "Callback", "Network",
    "Running", "Terminate", "Disconnect", "Unknown"
  };
  return phases[std::min<int>(phase, sizeof(phases) / sizeof(phases[0]) - 1)];
}
#endif // !PPP_DEBUG

} // anonymous

Client::Client() {
  std::call_once(once_, []() {
    LOCK_TCPIP_CORE();
    netifClientDataIdx_ = netif_alloc_client_data_id();
    SPARK_ASSERT(netifClientDataIdx_ > 0);
#if defined(LWIP_NETIF_EXT_STATUS_CALLBACK) && LWIP_NETIF_EXT_STATUS_CALLBACK == 1
    netif_add_ext_callback(&netifCb_, &Client::notifyNetifCb);
#endif /* defined(LWIP_NETIF_EXT_STATUS_CALLBACK) && LWIP_NETIF_EXT_STATUS_CALLBACK == 1 */
    UNLOCK_TCPIP_CORE();
  });

  exit_ = false;
  running_ = false;
}

Client::~Client() {
  deinit();
}

void Client::init() {
  std::lock_guard<std::mutex> lk(mutex_);
  if (!inited_) {
    LOG(TRACE, "PPP client initializing");
    inited_ = true;
    pcb_ = pppapi_pppos_create(&if_, &Client::outputCb, &Client::notifyStatusCb, this);
    SPARK_ASSERT(pcb_);
    if_.flags &= ~NETIF_FLAG_UP;

    LOCK_TCPIP_CORE();
    ipcp_ = std::make_unique<Ipcp>(pcb_);
    SPARK_ASSERT(ipcp_);
    int dnsIndex = resolv_get_dns_server_priority_for_iface((if_t)&if_, 0);
    if (dnsIndex < 0) {
      dnsIndex = 0;
    }
    ipcp_->setDnsEntryIndex(resolv_get_dns_server_priority_for_iface((if_t)&if_, 0));
    netif_set_client_data(&if_, netifClientDataIdx_, this);
    UNLOCK_TCPIP_CORE();

    // FIXME: do we need this workaround for R510?
    // XXX:
    if (platform_primary_ncp_identifier() == PLATFORM_NCP_SARA_R410 ||
        platform_primary_ncp_identifier() == PLATFORM_NCP_SARA_R510)
        {
      // SARA R4 PPP implementation is broken and often times we receive
      // an empty or non-conflicting Configure-Request in an already opened state
      // which, if we follow the state machine to the T, should restart the negotiation
      // bringing the protocol down. This causes unnecessary delays and affects
      // connection times.
      // This option will allow PPP state mchina to ignore non-conflicting Configure-Requests
      // and simply ACK them without restarting the negotiation.
      // We have necessary safeguards in place, so even if this doesn't go well, we should
      // be able to recover.
      // To repeat, we are slighly going off standard, but this should be an acceptable
      // behavior in this particular case.
      pcb_->settings.fsm_ignore_conf_req_opened = 1;
    }

    if (state_ == STATE_CONNECTED || state_ == STATE_DISCONNECTED) {
      // Allows the R510 to drop into low power mode (USPV=1) automatically after ~9s when idle
      pcb_->settings.lcp_echo_interval = NCP_CLIENT_LCP_ECHO_INTERVAL_SECONDS_R510;
      pcb_->settings.lcp_echo_fails = NCP_CLIENT_LCP_ECHO_MAX_FAILS_R510;
      pcb_->lcp_echos_pending = 0; // reset echo count
    }

    pppapi_set_notify_phase_callback(pcb_, &Client::notifyPhaseCb);

    os_queue_create(&queue_, sizeof(QueueEvent), 5, nullptr);
    SPARK_ASSERT(queue_);

    os_thread_create(&thread_, "ppp", OS_THREAD_PRIORITY_NETWORK, &Client::loopCb, this, OS_THREAD_STACK_SIZE_DEFAULT_HIGH);
    SPARK_ASSERT(thread_);
  }
}

void Client::deinit() {
  std::unique_lock<std::mutex> lk(mutex_);
  if (inited_) {
    if (thread_) {
      if (running_) {
        exit_ = true;
      }
      lk.unlock();
      os_thread_join(thread_);
      lk.lock();
      os_thread_cleanup(thread_);
      thread_ = nullptr;
    }
    if (queue_) {
      os_queue_destroy(queue_, nullptr);
      queue_ = nullptr;
    }
    if (pcb_) {
      pppapi_free(pcb_);
      pcb_ = nullptr;
    }
    inited_ = false;
  }
}

int Client::prepareConnect() {
  ipcp_->init();
  ipcp_->disable();
  ipcp_->enable();
  ipcp_->requestOption(ipcp::CONFIGURATION_OPTION_IP_ADDRESS, CONFIGURATION_OPTION_FLAG_ACCEPT_REMOTE_ALWAYS);
  ipcp_->requestOption(ipcp::CONFIGURATION_OPTION_PRIMARY_DNS_SERVER, CONFIGURATION_OPTION_FLAG_ACCEPT_REMOTE_ALWAYS);
  ipcp_->requestOption(ipcp::CONFIGURATION_OPTION_SECONDARY_DNS_SERVER, CONFIGURATION_OPTION_FLAG_ACCEPT_REMOTE_ALWAYS);

  ip4_addr_t pdns, sdns;
  ip4_addr_set_u32(&pdns, lwip_htonl(GOOGLE_DNS_PRIMARY));
  ip4_addr_set_u32(&sdns, lwip_htonl(GOOGLE_DNS_SECONDARY));
  ipcp_->setPrimaryDns(pdns);
  ipcp_->setSecondaryDns(sdns);

#if PPP_IPV6_SUPPORT
  LOCK_TCPIP_CORE();
  if_.ip6_autoconfig_enabled = 1;
  if_.flags |= NETIF_FLAG_MLD6;
  UNLOCK_TCPIP_CORE();
#endif // PPP_IPV6_SUPPORT

  std::unique_lock<std::mutex> lk(mutex_);
  auto cb = enterDataModeCb_;
  auto ctx = enterDataModeCbCtx_;
  lk.unlock();
  if (cb) {
    return cb(ctx);
  }
  return SYSTEM_ERROR_INVALID_STATE;
}

bool Client::start() {
  init();
  return true;
}

bool Client::connect() {
  return notifyEvent(EVENT_ADM_UP);

  // std::lock_guard<std::mutex> lk(mutex_);
  // /* TODO: configurable parameters */
  // LOCK_TCPIP_CORE();
  // ipcp_->requestOption(ipcp::CONFIGURATION_OPTION_IP_ADDRESS);
  // ipcp_->requestOption(ipcp::CONFIGURATION_OPTION_PRIMARY_DNS_SERVER);
  // ipcp_->requestOption(ipcp::CONFIGURATION_OPTION_SECONDARY_DNS_SERVER);
  // ipcp_->requestOption(ipcp::CONFIGURATION_OPTION_IP_NETMASK);
  // ipcp_->init();
}

bool Client::disconnect() {
  return notifyEvent(EVENT_ADM_DOWN);
}

bool Client::configure(void* config) {
  return true;
}

bool Client::notifyEvent(uint64_t ev, int data) {
  if (!running_) {
    return false;
  }
  QueueEvent event = {};
  event.ev = ev;
  event.data = data;
  return os_queue_put(queue_, &event, CONCURRENT_WAIT_FOREVER, nullptr) == 0;
}

int Client::input(const uint8_t* data, size_t size) {
  if (running_) {
    switch (state_) {
      case STATE_CONNECTING:
      case STATE_DISCONNECTING:
      case STATE_CONNECTED: {
        LOG_DEBUG(TRACE, "RX: %lu", size);
        // LOG_DUMP(TRACE, data, size);

        if (platform_primary_ncp_identifier() == PLATFORM_NCP_SARA_R410) {
          auto pppos = (pppos_pcb*)pcb_->link_ctx_cb;
          const char NO_CARRIER[] = "\r\nNO CARRIER\r\n";
          if (pppos && pppos->in_state == PDADDRESS && pcb_->phase == PPP_PHASE_NETWORK && data[0] != PPP_FLAG && size >= sizeof(NO_CARRIER) - 1 && !strncmp((const char*)data, NO_CARRIER, size)) {
            LOG(ERROR, "NO CARRIER in network PPP phase");
            pppapi_close(pcb_, 1);
            notifyEvent(EVENT_ERROR, ERROR_NO_CARRIER_IN_NETWORK_PHASE);
            break;
          }
        }

#if !PPP_INPROC_IRQ_SAFE
        err_t err = pppos_input_tcpip(pcb_, (u8_t*)data, size);
#else
        // We can safely pass the data directly to PPPoS without going
        // through TCPIP thread mailbox and wasting a buffer for each tiny chunk of data
#ifdef DEBUG_BUILD
        auto linkDropBefore = lwip_stats.link.drop;
#endif // DEBUG_BUILD

        pppos_input(pcb_, (u8_t*)data, size);

#ifdef DEBUG_BUILD
        auto linkDropAfter = lwip_stats.link.drop;
        if (linkDropAfter > linkDropBefore) {
          LOG(WARN, "May have dropped %u bytes/packets (received %u bytes)", linkDropAfter - linkDropBefore, size);
        }
#endif // DEBUG_BUILD
        // FIXME
        err_t err = ERR_OK;
        int poolAvail = MEMP_STATS_GET(avail, MEMP_PBUF_POOL) - MEMP_STATS_GET(used, MEMP_PBUF_POOL);
        if (poolAvail <= HAL_PLATFORM_PACKET_BUFFER_FLOW_CONTROL_THRESHOLD) {
          LOG_DEBUG(WARN, "Almost out of pbufs");
          return SYSTEM_ERROR_NO_MEMORY;
        }
#endif // PPP_INPROC_IRQ_SAFE
        if (err) {
          return SYSTEM_ERROR_INTERNAL;
        }
        return 0;
      }
    }
  }
  return SYSTEM_ERROR_INVALID_STATE;
}

void Client::setNotifyCallback(NotifyCallback cb, void* ctx) {
  std::lock_guard<std::mutex> lk(mutex_);
  cb_ = cb;
  cbCtx_ = ctx;
}

void Client::setOutputCallback(OutputCallback cb, void* ctx) {
  std::lock_guard<std::mutex> lk(mutex_);
  oCb_ = cb;
  oCbCtx_ = ctx;
}

void Client::setEnterDataModeCallback(EnterDataModeCallback cb, void* ctx) {
  std::lock_guard<std::mutex> lk(mutex_);
  enterDataModeCb_ = cb;
  enterDataModeCbCtx_ = ctx;
}

void Client::setAuth(const char* user, const char* password) {
  {
    std::lock_guard<std::mutex> lk(mutex_);
    user_.reset();
    pass_.reset();
    if (user) {
      user_.reset(strdup(user));
    }
    if (password) {
      pass_.reset(strdup(password));
    }
  }

  {
    LwipTcpIpCoreLock lk;
    auto authtype = PPPAUTHTYPE_CHAP | PPPAUTHTYPE_PAP;
    // FIXME: CHAP authentication is broken on Quectel modems when
    // resuming into an ongoing PPP session. We are not getting a CHAP
    // challenge request and just sit in AUTH stage indefinitely.
    // As a workaround enable only PAP
    if (PLATFORM_NCP_MANUFACTURER(platform_primary_ncp_identifier()) == PLATFORM_NCP_MANUFACTURER_QUECTEL) {
      authtype = PPPAUTHTYPE_PAP;
    }
    ppp_set_auth(pcb_, authtype, user_.get(), pass_.get());
  }
}

netif* Client::getIf() {
  return &if_;
}

void Client::loopCb(void* arg) {
  Client* self = static_cast<Client*>(arg);
  if (self) {
    self->loop();
  }
  os_thread_exit(nullptr);
}

void Client::loop() {
  running_ = true;
  LOG(TRACE, "PPP thread started");
  while(!exit_) {
    unsigned qWait = 100;

    switch (state_) {
      case STATE_CONNECT: {
        transition(STATE_CONNECTING);
        if (prepareConnect()) {
          LOG(ERROR, "Failed to dial");
          transition(STATE_CONNECT);
          break;
        }
        err_t err = pppapi_connect(pcb_, 0);
        if (err != ERR_OK) {
          LOG(TRACE, "PPP error connecting: %d", err);
          transition(STATE_CONNECT);
        }
        break;
      }

      case STATE_CONNECTING:
      case STATE_DISCONNECTING:
      case STATE_CONNECTED: {
        // Nothing to do
        break;
      }

      case STATE_DISCONNECT: {
        transition(STATE_DISCONNECTING);
        // FIXME: graceful disconnect
        pppapi_close(pcb_, 1);
        break;
      }

      case STATE_DISCONNECTED: {
        if (admState_) {
          if (lowerState_) {
            transition(STATE_CONNECT);
          } else {
            transition(STATE_READY);
          }
        } else {
          transition(STATE_NONE);
        }
        break;
      }

      case STATE_NONE:
      case STATE_READY:
      default: {
        break;
      }
    }

    QueueEvent ev = {};
    if (os_queue_take(queue_, &ev, qWait, nullptr) == 0) {
      LOG(TRACE, "PPP thread event %s data=%d", eventNames_[ev.ev], ev.data);
      /* Incoming event */
      switch (ev.ev) {
        case EVENT_LOWER_UP: {
          if (!lowerState_) {
            lowerState_ = true;
            if (state_ == STATE_READY) {
              transition(STATE_CONNECT);
            }
          }
          break;
        }
        case EVENT_LOWER_DOWN: {
          if (lowerState_) {
            lowerState_ = false;
            if (state_ == STATE_CONNECTED || state_ == STATE_CONNECTING || state_ == STATE_CONNECT) {
              // FIXME: Allow the PPP layer to lose carrier on its own
              transition(STATE_DISCONNECT);
            }
          }
          break;
        }
        case EVENT_ADM_UP: {
          if (!admState_) {
            admState_ = true;
            if (state_ == STATE_NONE) {
              if (!lowerState_) {
                transition(STATE_READY);
              } else {
                transition(STATE_CONNECT);
              }
            }
          }
          break;
        }
        case EVENT_ADM_DOWN: {
          if (admState_) {
            admState_ = false;
            if (state_ == STATE_CONNECTED || state_ == STATE_CONNECTING || state_ == STATE_CONNECT) {
              transition(STATE_DISCONNECT);
            } else if (state_ == STATE_READY) {
              transition(STATE_NONE);
            }
          }
          break;
        }
        case EVENT_UP: {
          if (!linkState_) {
            linkState_ = true;
            if (state_ == STATE_CONNECTING) {
              transition(STATE_CONNECTED);
            }
          }
          break;
        }
        case EVENT_DOWN: {
          linkState_ = false;
          if (state_ == STATE_CONNECTED || state_ == STATE_CONNECTING || state_ == STATE_CONNECT) {
            transition(STATE_DISCONNECT);
          } else if (state_ == STATE_DISCONNECTING) {
            transition(STATE_DISCONNECTED);
          }
          break;
        }
        case EVENT_ERROR: {
          std::unique_lock<std::mutex> lk(mutex_);
          auto cb = cb_;
          auto ctx = cbCtx_;
          lk.unlock();
          if (cb) {
            cb(this, EVENT_ERROR, ev.data, ctx);
          }
          break;
        }
      }
    }
  }
  LOG(TRACE, "PPP thread exited");
  running_ = false;
}

uint32_t Client::outputCb(ppp_pcb* pcb, uint8_t* data, uint32_t len, void* ctx) {
  Client* self = static_cast<Client*>(ctx);
  if (self) {
    return self->output(data, len);
  }

  return 0;
}

uint32_t Client::output(const uint8_t* data, size_t len) {
  LOG_DEBUG(TRACE, "TX: %lu", len);
  // LOG_DUMP(TRACE, data, len);

  if (oCb_) {
    auto r = oCb_(data, len, oCbCtx_);
    if (r >= 0) {
      return r;
    }
  }

  return 0;
}

void Client::notifyPhaseCb(ppp_pcb* pcb, uint8_t phase, void* ctx) {
  Client* self = static_cast<Client*>(ctx);
  if (self) {
    self->notifyPhase(phase);
  }
}

void Client::notifyPhase(uint8_t phase) {
#if !PPP_DEBUG
  LOG(TRACE, "PPP phase -> %s", pppPhaseToString(phase));
#endif // !PPP_DEBUG
}

void Client::notifyStatusCb(ppp_pcb* pcb, int err, void* ctx) {
  Client* self = static_cast<Client*>(ctx);
  if (self) {
    self->notifyStatus(err);
  }
}

void Client::notifyStatus(int err) {
  LOG_DEBUG(TRACE, "PPP status -> %d", err);
  switch (err) {
    case PPPERR_NONE: {
      /* Connected */
      notifyEvent(EVENT_UP);
      break;
    }
    default: {
      /* Error connecting */
      notifyEvent(EVENT_ERROR, err);
      notifyEvent(EVENT_DOWN);
      break;
    }
  }
}

#if defined(LWIP_NETIF_EXT_STATUS_CALLBACK) && LWIP_NETIF_EXT_STATUS_CALLBACK == 1
void Client::notifyNetifCb(netif* netif, netif_nsc_reason_t reason, const netif_ext_callback_args_t* args) {
  if (netif) {
    auto d = netif_get_client_data(netif, netifClientDataIdx_);
    if (d) {
      Client* self = static_cast<Client*>(d);
      self->notifyNetif(reason, args);
    }
  }
}

void Client::notifyNetif(netif_nsc_reason_t reason, const netif_ext_callback_args_t* args) {
  LOG_DEBUG(TRACE, "PPP netif -> %x", reason);
}
#endif /* defined(LWIP_NETIF_EXT_STATUS_CALLBACK) && LWIP_NETIF_EXT_STATUS_CALLBACK == 1 */

void Client::transition(State newState) {
  LOG(TRACE, "State %s -> %s", stateNames_[state_], stateNames_[newState]);
  if (newState != state_) {
    if (platform_primary_ncp_identifier() == PLATFORM_NCP_SARA_R510) {
      if (newState == STATE_CONNECTED || newState == STATE_DISCONNECTED) {
        // Allows the R510 to drop into low power mode (USPV=1) automatically after ~9s when idle
        pcb_->settings.lcp_echo_interval = NCP_CLIENT_LCP_ECHO_INTERVAL_SECONDS_R510;
        pcb_->settings.lcp_echo_fails = NCP_CLIENT_LCP_ECHO_MAX_FAILS_R510;
        pcb_->lcp_echos_pending = 0; // reset echo count
      } else {
        // Resume default keep alive when not CONNECTED/DISCONNECTED for R510
        pcb_->settings.lcp_echo_interval = NCP_CLIENT_LCP_ECHO_INTERVAL_SECONDS_DEFAULT;
        pcb_->settings.lcp_echo_fails = NCP_CLIENT_LCP_ECHO_MAX_FAILS_DEFAULT;
      }
    }
  }
  state_ = newState;

  {
    if (state_ == STATE_CONNECTED || state_ == STATE_DISCONNECTED || state_ == STATE_CONNECTING) {
      std::unique_lock<std::mutex> lk(mutex_);
      auto cb = cb_;
      auto ctx = cbCtx_;
      lk.unlock();
      if (cb) {
        if (state_ == STATE_CONNECTED) {
          cb(this, EVENT_UP, ERROR_NONE, ctx);
        } else if (state_ == STATE_DISCONNECTED) {
          cb(this, EVENT_DOWN, ERROR_NONE, ctx);
        } else if (state_ == STATE_CONNECTING) {
          cb(this, EVENT_CONNECTING, ERROR_NONE, ctx);
        }
      }
    }
  }
}

#endif // defined(PPP_SUPPORT) && PPP_SUPPORT
