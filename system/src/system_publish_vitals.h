#ifndef VITALS_PUBLISHER_H
#define VITALS_PUBLISHER_H

#include <cstdarg>
#include <cstddef>
#include <functional>

#include "spark_protocol_functions.h"
#include "system_tick_hal.h"

namespace particle
{
namespace system
{

/**
 * @class VitalsPublisher system_publish_vitals.h
 * @brief Publish vitals information
 *
 * Combines a protocol, function and timer, to schedule and periodically
 * publish vitals information to the cloud. This information is then consumed
 * by the fleet health metrics dashboard in the console.
 *
 * @tparam Timer An API compatible timer class with \p spark_wiring_timer.h:Timer
 *
 * @sa Timer
 */
template <class Timer>
class VitalsPublisher
{
public:
    /**
     * @brief Constructor
     *
     * The constructor initializes all member variables to known values.
     *
     * @param[in] timer The timer used to schedule the period
     */
    VitalsPublisher(Timer* timer = nullptr);

    /**
     * @brief Destructor
     *
     * The destructor ensures the resources allocated during the
     * scheduling of events are returned to the system.
     */
    virtual ~VitalsPublisher(void);

    /**
     * @brief Disable periodic publishing
     */
    void disablePeriodicPublish(void);

    /**
     * @brief Enable periodic publishing
     */
    void enablePeriodicPublish(void);

    /**
     * @brief Fetch the period value
     *
     * @return The period value in seconds
     */
    system_tick_t period(void) const;

    /**
     * @brief Update the period value
     *
     * @param[in] period_s The period value in seconds
     */
    void period(system_tick_t period_s);

    /**
     * @brief Publish vitals information to the cloud (immediately)
     *
     * @returns \p ProtocolError result code
     * @retval \p ProtocolError::NO_ERROR
     * @retval \p ProtocolError::IO_ERROR_GENERIC_SEND
     */
    int publish(void);

private:
    system_tick_t _period_s;
    Timer* const _timer;
    const bool _timer_owner;

    /**
     * @brief Publish vitals from Timer callback
     *
     * This function is required to work around Timer thread execution bug.
     * https://app.clubhouse.io/particle/story/30935/fix-spark-wiring-timer-timer-object-callback-to-ensure-execution-on-the-system-thread
     */
    void publishFromTimer(void);
};

} // namespace system
} // namespace particle

#endif
