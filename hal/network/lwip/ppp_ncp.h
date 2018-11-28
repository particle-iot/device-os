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

#ifndef HAL_NETWORK_LIP_PPP_NCP_H
#define HAL_NETWORK_LIP_PPP_NCP_H

#include "concurrent_hal.h"
#include <cstdint>
extern "C" {
#include <netif/ppp/ppp.h>
#ifndef PPPDEBUG_H
/* we do not want this header, as it redefines our LOG_XXX macros */
#define PPPDEBUG_H
#endif /* PPPDEBUG_H */
#include <netif/ppp/ppp_impl.h>
#include <netif/ppp/fsm.h>
}

#if defined(PPP_SUPPORT) && PPP_SUPPORT

#include "ppp_configuration_option.h"

#ifdef __cplusplus

namespace particle { namespace net { namespace ppp {

template <uint16_t protocol, void* ppp_pcb::*ContextVariable, const char* protoName, const char* dataProtoName>
class Ncp {
public:
  Ncp(ppp_pcb* pcb)
      : pcb_(pcb) {
    if (pcb_) {
      (*pcb_).*ContextVariable = this;
    }

    fsm_.pcb = pcb_;
    fsm_.protocol = protocol;
    fsm_.callbacks = &fsmCallbacks_;
    fsm_.flags |= OPT_RESTART;
    fsm_.callbacks = &fsmCallbacks_;
  }

  virtual ~Ncp() {
    if (pcb_) {
      (*pcb_).*ContextVariable = nullptr;
    }
  };

  /* Allow IPCP to come up */
  virtual void enable() = 0;

  /* IPCP is not allowed to come up.
   * Current IPCP session will be terminated
   */
  virtual void disable() = 0;

  /* Configuration options */
  virtual void registerOption(ConfigurationOption* option) {
    if (option == nullptr) {
      return;
    }

    option->next = nullptr;

    if (options_ == nullptr) {
      options_ = option;
    } else {
      for(auto opt = options_; opt != nullptr; opt = opt->next) {
        if (opt->next == nullptr) {
          opt->next = option;
          break;
        }
      }
    }
  }

  virtual void requestOption(int id) {
    auto opt = findOption(id);
    if (opt != nullptr) {
      opt->flagsLocal |= CONFIGURATION_OPTION_FLAG_REQUEST;
    }
  }

  template <typename F>
  void forEachOption(F&& lambda) {
    for (auto opt = options_; opt != nullptr; opt = opt->next) {
      lambda(opt);
    }
  }

  /* Protocol callbacks */
  /* Initialization procedure */
  virtual void init() = 0;
  /* Process a received packet */
  virtual void input(uint8_t* pkt, int len) = 0;
  /* Process a received protocol-reject */
  virtual void protocolReject() = 0;
  /* Lower layer has come up */
  virtual void lowerUp() = 0;
  /* Lower layer has gone down */
  virtual void lowerDown() = 0;
  /* Open the protocol */
  virtual void open() = 0;
  /* Close the protocol */
  virtual void close(const char* reason) = 0;

  /* State machine callbacks */
  /* Reset our Configuration Information */
  virtual void resetConfigurationInformation() = 0;
  /* Length of our Configuration Information */
  virtual int getConfigurationInformationLength() = 0;
  /* Add our Configuration Information */
  virtual void addConfigurationInformation(uint8_t* buf, int* len) = 0;
  /* ACK our Configuration Information */
  virtual int ackConfigurationInformation(uint8_t* buf, int len) = 0;
  /* NAK our Configuration Information */
  virtual int nakConfigurationInformation(uint8_t* buf, int len, int treatAsReject) = 0;
  /* Reject our Configuration Information */
  virtual int rejectConfigurationInformation(uint8_t* buf, int len) = 0;
  /* Request peer's Configuration Information */
  virtual int requestConfigurationInformation(uint8_t* buf, int* len, int rejectIfDisagree) = 0;
  /* Called when fsm reaches PPP_FSM_OPENED state */
  virtual void up() = 0;
  /* Called when fsm leaves PPP_FSM_OPENED state */
  virtual void down() = 0;
  /* Called when we want the lower layer */
  virtual void starting() = 0;
  /* Called when we don't want the lower layer */
  virtual void finished() = 0;
  /* Called when Protocol-Reject received */
  // virtual void protocolRejectCb(int) = 0;
  /* Retransmission is necessary */
  // virtual void retransmit() = 0;
  /* Called when unknown code received */
  virtual int extCode(int code, int id, uint8_t* buf, int len) = 0;

  static constexpr struct protent generateProtent() {
    constexpr struct protent p = {
      protocol,
      initCb,
      inputCb,
      protocolRejectCb,
      lowerUpCb,
      lowerDownCb,
      openCb,
      closeCb,
#if PRINTPKT_SUPPORT
      printPacketCb,
#endif /* PRINTPKT_SUPPORT */
#if PPP_DATAINPUT
      NULL,
#endif /* PPP_DATAINPUT */
#if PRINTPKT_SUPPORT
      protoName,
      dataProtoName,
#endif /* PRINTPKT_SUPPORT */
#if PPP_OPTIONS
      nullptr,
      nullptr,
#endif /* PPP_OPTIONS */
#if DEMAND_SUPPORT
      nullptr,
      nullptr
#endif /* DEMAND_SUPPORT */
    };

    return p;
  }

  /* State machine callbacks */

protected:
  ConfigurationOption* findOption(int id) {
    for (auto opt = options_; opt != nullptr; opt = opt->next) {
      if (opt->id == id) {
        return opt;
      }
    }

    return nullptr;
  }

protected:
  ppp_pcb* pcb_;
  fsm fsm_ = {};
  ConfigurationOption* options_ = nullptr;

private:

  using PacketPrinter = void (*)(void *, const char *, ...);

  static Ncp* getInstance(ppp_pcb* pcb) {
    if (pcb != nullptr) {
      Ncp* self = static_cast<Ncp*>((*pcb).*ContextVariable);
      return self;
    }

    return nullptr;
  }

  static Ncp* getInstance(fsm* fsm) {
    if (fsm != nullptr) {
      return getInstance(fsm->pcb);
    }

    return nullptr;
  }

  /* Protocol callbacks */
  /* Initialization procedure */
  static void initCb(ppp_pcb* pcb) {
    auto instance = getInstance(pcb);
    if (instance != nullptr) {
      instance->init();
    }
  }

  /* Process a received packet */
  static void inputCb(ppp_pcb* pcb, uint8_t* pkt, int len) {
    auto instance = getInstance(pcb);
    if (instance != nullptr) {
      instance->input(pkt, len);
    }
  }

  /* Process a received protocol-reject */
  static void protocolRejectCb(ppp_pcb* pcb) {
    auto instance = getInstance(pcb);
    if (instance != nullptr) {
      instance->protocolReject();
    }
  }

  /* Lower layer has come up */
  static void lowerUpCb(ppp_pcb* pcb) {
    auto instance = getInstance(pcb);
    if (instance != nullptr) {
      instance->lowerUp();
    }
  }

  /* Lower layer has gone down */
  static void lowerDownCb(ppp_pcb* pcb) {
    auto instance = getInstance(pcb);
    if (instance != nullptr) {
      instance->lowerDown();
    }
  }

  /* Open the protocol */
  static void openCb(ppp_pcb* pcb) {
    auto instance = getInstance(pcb);
    if (instance != nullptr) {
      instance->open();
    }
  }

  /* Close the protocol */
  static void closeCb(ppp_pcb* pcb, const char* reason) {
    auto instance = getInstance(pcb);
    if (instance != nullptr) {
      instance->close(reason);
    }
  }

  static int printPacketCb(const uint8_t *p, int plen, PacketPrinter printer, void *arg) {
    return 0;
  }

  /* State machine callbacks */
  /* Reset our Configuration Information */
  static void resetConfigurationInformationCb(fsm* fsm) {
    auto instance = getInstance(fsm);
    if (instance != nullptr) {
      instance->resetConfigurationInformation();
    }
  }

  /* Length of our Configuration Information */
  static int getConfigurationInformationLengthCb(fsm* fsm) {
    auto instance = getInstance(fsm);
    if (instance != nullptr) {
      return instance->getConfigurationInformationLength();
    }

    return 0;
  }

  /* Add our Configuration Information */
  static void addConfigurationInformationCb(fsm* fsm, uint8_t* buf, int* len) {
    auto instance = getInstance(fsm);
    if (instance != nullptr) {
      instance->addConfigurationInformation(buf, len);
    }
  }

  /* ACK our Configuration Information */
  static int ackConfigurationInformationCb(fsm* fsm, uint8_t* buf, int len) {
    auto instance = getInstance(fsm);
    if (instance != nullptr) {
      return instance->ackConfigurationInformation(buf, len);
    }

    /* generic error */
    return 0;
  }

  /* NAK our Configuration Information */
  static int nakConfigurationInformationCb(fsm* fsm, uint8_t* buf, int len, int treatAsReject) {
    auto instance = getInstance(fsm);
    if (instance != nullptr) {
      return instance->nakConfigurationInformation(buf, len, treatAsReject);
    }

    /* generic error */
    return 0;
  }

  /* Reject our Configuration Information */
  static int rejectConfigurationInformationCb(fsm* fsm, uint8_t* buf, int len) {
    auto instance = getInstance(fsm);
    if (instance != nullptr) {
      return instance->rejectConfigurationInformation(buf, len);
    }

    /* generic error */
    return 0;
  }

  /* Request peer's Configuration Information */
  static int requestConfigurationInformationCb(fsm* fsm, uint8_t* buf, int* len, int rejectIfDisagree) {
    auto instance = getInstance(fsm);
    if (instance != nullptr) {
      return instance->requestConfigurationInformation(buf, len, rejectIfDisagree);
    }

    /* generic error */
    return 0;
  }

  /* Called when fsm reaches PPP_FSM_OPENED state */
  static void upCb(fsm* fsm) {
    auto instance = getInstance(fsm);
    if (instance != nullptr) {
      instance->up();
    }
  }

  /* Called when fsm leaves PPP_FSM_OPENED state */
  static void downCb(fsm* fsm) {
    auto instance = getInstance(fsm);
    if (instance != nullptr) {
      instance->down();
    }
  }

  /* Called when we want the lower layer */
  static void startingCb(fsm* fsm) {
    auto instance = getInstance(fsm);
    if (instance != nullptr) {
      instance->starting();
    }
  }

  /* Called when we don't want the lower layer */
  static void finishedCb(fsm* fsm) {
    auto instance = getInstance(fsm);
    if (instance != nullptr) {
      instance->finished();
    }
  }

  /* Called when Protocol-Reject received */
  // static void protocolRejectCb(int);
  /* Retransmission is necessary */
  // static void retransmitCb(fsm* fsm);
  /* Called when unknown code received */

  static int extCodeCb(fsm* fsm, int code, int id, uint8_t* buf, int len) {
    auto instance = getInstance(fsm);
    if (instance != nullptr) {
      return instance->extCode(code, id, buf, len);
    }

    /* generic error */
    return 1;
  }

private:
  static fsm_callbacks fsmCallbacks_;
};

template <uint16_t protocol, void* ppp_pcb::*ContextVariable, const char* protoName, const char* dataProtoName>
fsm_callbacks Ncp<protocol, ContextVariable, protoName, dataProtoName>::fsmCallbacks_ = {
  resetConfigurationInformationCb,
  getConfigurationInformationLengthCb,
  addConfigurationInformationCb,
  ackConfigurationInformationCb,
  nakConfigurationInformationCb,
  rejectConfigurationInformationCb,
  requestConfigurationInformationCb,
  upCb,
  downCb,
  startingCb,
  finishedCb,
  nullptr,
  nullptr,
  extCodeCb,
  protoName
};


} } } /* namespace particle::net::ppp */

#endif /* __cplusplus */

#endif // defined(PPP_SUPPORT) && PPP_SUPPORT

#endif /* HAL_NETWORK_LIP_PPP_NCP_H */
