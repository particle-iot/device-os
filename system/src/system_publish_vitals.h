#ifndef VITALS_PUBLISHER_H
#define VITALS_PUBLISHER_H

#include <cstddef>

#include "platforms.h"
#include "protocol_selector.h"
#include "system_tick_hal.h"

namespace particle {

#if PLATFORM_ID == PLATFORM_SPARK_CORE  // CORE
  class NullTimer {
    public:
      inline static bool changePeriod (const size_t) { return false; }
      inline static void dispose (void) {}
      inline static bool isActive (void) { return false; }
      inline static void reset (void) {}
      inline static void start (void) {}
      inline static void stop (void) {}
  };
#endif // PLATFORM_SPARK_CORE

namespace system {

template <
    typename publish_fn_t,
    class Timer
>
class VitalsPublisher {
  public:
    VitalsPublisher (
        ProtocolFacade * protocol,
        publish_fn_t fn,
        const Timer & timer
    );

    virtual
    ~VitalsPublisher (
        void
    );

    void
    disablePeriodicPublish (
        void
    );

    void
    enablePeriodicPublish (
        void
    );

    system_tick_t
    period (
        void
    ) const;

    void
    period (
        system_tick_t period_s
    );

    int
    publish (
        void
    );

  private:
    system_tick_t _period_s;
    ProtocolFacade * _protocol;
    publish_fn_t _publishVitals;
    Timer _timer;
};

} // namespace system
} // namespace particle

#endif