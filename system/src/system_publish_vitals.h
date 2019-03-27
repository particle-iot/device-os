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

/**
 * @brief Publish vitals information
 *
 * Combines a protocol, function and timer, to schedule and periodically
 * publish vitals information to the cloud. This information is then consumed
 * by the fleet health metrics dashboard in the console.
 */
template <
    typename publish_fn_t,
    class Timer
>
class VitalsPublisher {
  public:
    /**
     * @brief Constructor
     *
     * @param[in] protocol The protocol used to send cloud messages
     * @param[in] fn The function used to send cloud messages
     * @param[in] timer The timer used to schedule the period
     */
    VitalsPublisher (
        ProtocolFacade * protocol,
        publish_fn_t fn,
        const Timer & timer
    );

    /**
     * @brief Destructor
     *
     * The destructor ensures the resources allocated during the
     * scheduling of events are returned to the system.
     */
    virtual
    ~VitalsPublisher (
        void
    );

    /**
     * @brief Disable periodic publishing
     */
    void
    disablePeriodicPublish (
        void
    );

    /**
     * @brief Enable periodic publishing
     */
    void
    enablePeriodicPublish (
        void
    );

    /**
     * @brief Fetch the period value
     *
     * @return The period value in seconds
     */
    system_tick_t
    period (
        void
    ) const;

    /**
     * @brief Update the period value
     *
     * @param[in] period_s The period value in seconds
     */
    void
    period (
        system_tick_t period_s
    );

    /**
     * @brief Publish vitals information to the cloud (immediately).
     *
     * @returns \p system_error_t result code
     * @retval \p system_error_t::SYSTEM_ERROR_NONE
     * @retval \p system_error_t::SYSTEM_ERROR_IO
     */
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