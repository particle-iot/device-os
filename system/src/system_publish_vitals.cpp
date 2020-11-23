
#include "system_publish_vitals.h"

#include <limits>

#include "logging.h"
#include "system_cloud.h"
#include "system_threading.h"
#include "spark_wiring_timer.h"

#ifdef UNIT_TEST
#include "../../test/unit_tests/mock/mock_types.h"
#endif // UNIT_TEST

namespace
{

/**
 * @brief Post description message via the CoAP protocol
 *
 * This function is shared internally by both the `publish` and `publishFromTimer` methods. It is
 * static to ensure it can be called on the system thread via the `ISRTaskQueue`.
 *
 * @sa template <class Timer> particle::cloud::VitalsPublisher<Timer>::publish
 * @sa template <class Timer> particle::cloud::VitalsPublisher<Timer>::publishFromTimer
 */
inline int postDescription()
{
    int error;

    if (spark_cloud_flag_connected())
    {
        // Transmit CoAP message via communication layer
        error = spark_protocol_post_description(spark_protocol_instance(),
                                                particle::protocol::DESCRIBE_METRICS, nullptr);

        // Convert `protocol` error to `system` error
        error = spark_protocol_to_system_error(error);
    }
    else
    {
        error = SYSTEM_ERROR_INVALID_STATE;
    }

    return error;
}

} // namespace

namespace particle { namespace system {

template <class Timer>
VitalsPublisher<Timer>::VitalsPublisher(Timer* timer_)
    : _period_s(std::numeric_limits<system_tick_t>::max()),
      _timer(timer_ ? timer_
                    : new Timer(_period_s, &VitalsPublisher::publishFromTimer, *this, false)),
      _timer_owner(!timer_)
{
}

template <class Timer>
VitalsPublisher<Timer>::~VitalsPublisher(void)
{
    _timer->dispose();
    if (_timer_owner)
    {
        delete _timer;
    }
}

template <class Timer>
void VitalsPublisher<Timer>::disablePeriodicPublish(void)
{
    _timer->stop();
}

template <class Timer>
void VitalsPublisher<Timer>::enablePeriodicPublish(void)
{
    _timer->start();
}

template <class Timer>
system_tick_t VitalsPublisher<Timer>::period(void) const
{
    return _period_s;
}

template <class Timer>
void VitalsPublisher<Timer>::period(system_tick_t period_s_)
{
    const system_tick_t period_ms = period_s_ * 1000;
    const bool was_active = _timer->isActive();

    if (!_timer->changePeriod(period_ms))
    {
        LOG(ERROR, "Unable to update vitals timer period!");
    }
    else
    {
        _period_s = period_s_;

        // Maintain pre-existing state
        if (was_active)
        {
            _timer->reset();
        }
        else
        {
            _timer->stop();
        }
    }
}

template <class Timer>
int VitalsPublisher<Timer>::publish(void)
{
    return postDescription();
}

// Functionality covered by E2E tests
#define LCOV_EXCL_START
/*
 * TODO: CH38588 - Abstract ISRTaskQueue boilerplate
 * https://app.clubhouse.io/particle/story/38588/abstract-isrtaskqueue-boilerplate
 */
template <class Timer>
void VitalsPublisher<Timer>::publishFromTimer(void)
{
    const auto task = new (std::nothrow) ISRTaskQueue::Task;
    if (!task)
    {
        return;
    }
    task->func = [](ISRTaskQueue::Task* task) {
        delete task;
        postDescription();
    };
    SystemISRTaskQueue.enqueue(task);
}
#define LCOV_EXCL_STOP

#if PLATFORM_THREADING
template class VitalsPublisher<Timer>;
#else  // not PLATFORM_THREADING
template class VitalsPublisher<particle::NullTimer>;
#endif // PLATFORM_THREADING

#ifdef UNIT_TEST

template class VitalsPublisher<particle::mock_type::Timer>;

#endif // UNIT_TEST

}
}// namespace particle { namespace system {