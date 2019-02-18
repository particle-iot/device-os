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

#ifndef HAL_NETWORK_LWIP_PPP_CLIENT_H
#define HAL_NETWORK_LWIP_PPP_CLIENT_H

#include <memory>
#include <cstdint>
#include <lwip/netif.h>
extern "C" {
#include <netif/ppp/ppp.h>
}

#if defined(PPP_SUPPORT) && PPP_SUPPORT

#include "ppp_ipcp.h"
#include "concurrent_hal.h"
#include <mutex>
#include <atomic>
#include "stream.h"

#ifdef __cplusplus

namespace particle { namespace net { namespace ppp {

class Client {
public:
  Client();
  ~Client();

  bool start();

  /* Administrative up */
  bool connect();
  /* Administrative down */
  bool disconnect();

  /* FIXME */
  bool configure(void* config);

  enum Event {
    EVENT_NONE        = 0x00,
    EVENT_LOWER_UP    = 0x01,
    EVENT_LOWER_DOWN  = 0x02,
    EVENT_UP          = 0x03,
    EVENT_DOWN        = 0x04,
    EVENT_ADM_UP      = 0x05,
    EVENT_ADM_DOWN    = 0x06,
    EVENT_ERROR       = 0x07,
    EVENT_MAX         = 0x08
  };

  enum State {
    STATE_NONE         = 0,
    STATE_READY        = 1,
    STATE_CONNECT      = 2,
    STATE_CONNECTING   = 3,
    STATE_CONNECTED    = 4,
    STATE_DISCONNECT   = 5,
    STATE_DISCONNECTING= 6,
    STATE_DISCONNECTED = 7,
    STATE_MAX          = 8
  };

  bool notifyEvent(uint64_t ev);
  int input(const uint8_t* data, size_t size);

  typedef int (*OutputCallback)(const uint8_t* data, size_t size, void* ctx);
  void setOutputCallback(OutputCallback cb, void* ctx);

  typedef void (*NotifyCallback)(Client* c, uint64_t ev, void* ctx);

  void setNotifyCallback(NotifyCallback cb, void* ctx);

  void setAuth(const char* user, const char* password);

  netif* getIf();

private:

  static constexpr const char* eventNames_[] = {
    "NONE",
    "LOWER_UP",
    "LOWER_DOWN",
    "UP",
    "DOWN",
    "ADM_UP",
    "ADM_DOWN",
    "ERROR"
  };

  static constexpr const char* stateNames_[] = {
    "NONE",
    "READY",
    "CONNECT",
    "CONNECTING",
    "CONNECTED",
    "DISCONNECT",
    "DISCONNECTING",
    "DISCONNECTED"
  };

  void init();
  void deinit();

  bool prepareConnect();

  static void loopCb(void* arg);
  void loop();

  static uint32_t outputCb(ppp_pcb* pcb, uint8_t* data, uint32_t len, void* ctx);
  uint32_t output(const uint8_t* data, size_t len);

  static void notifyPhaseCb(ppp_pcb* pcb, uint8_t phase, void* ctx);
  void notifyPhase(uint8_t phase);

  static void notifyStatusCb(ppp_pcb* pcb, int err, void* ctx);
  void notifyStatus(int err);

#if defined(LWIP_NETIF_EXT_STATUS_CALLBACK) && LWIP_NETIF_EXT_STATUS_CALLBACK == 1
  static void notifyNetifCb(netif* netif, netif_nsc_reason_t reason, const netif_ext_callback_args_t* args);
  void notifyNetif(netif_nsc_reason_t reason, const netif_ext_callback_args_t* args);
#endif /* defined(LWIP_NETIF_EXT_STATUS_CALLBACK) && LWIP_NETIF_EXT_STATUS_CALLBACK == 1 */

  void transition(State newState);

private:
  netif if_ = {};
  ppp_pcb* pcb_ = nullptr;
#if PPP_IPCP_OVERRIDE
  std::unique_ptr<Ipcp> ipcp_;
#endif // PPP_IPCP_OVERRIDE

  std::unique_ptr<char> user_;
  std::unique_ptr<char> pass_;

  bool lowerState_ = false;
  bool admState_ = false;
  bool linkState_ = false;

  State state_ = STATE_NONE;

  os_thread_t thread_ = nullptr;
  std::mutex mutex_;
  os_queue_t queue_ = nullptr;

  NotifyCallback cb_ = nullptr;
  void* cbCtx_ = nullptr;

  OutputCallback oCb_ = nullptr;
  void* oCbCtx_ = nullptr;

  bool inited_ = false;
  std::atomic_bool running_;
  std::atomic_bool exit_;

  static std::once_flag once_;
  static netif_ext_callback_t netifCb_;
  static int netifClientDataIdx_;
};

} } } /* namespace particle::net::ppp */

#endif /* __cplusplus */

#endif // defined(PPP_SUPPORT) && PPP_SUPPORT

#endif /* HAL_NETWORK_LWIP_PPP_CLIENT_H */
