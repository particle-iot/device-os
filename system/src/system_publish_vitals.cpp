
#include "system_publish_vitals.h"

#include <limits>

#include "logging.h"
#include "spark_protocol_functions.h"
#include "system_threading.h"

using namespace particle::system;

template <class Timer>
const typename VitalsPublisher<Timer>::publish_fn_t VitalsPublisher<Timer>::_publishVitals =
    std::bind(spark_protocol_post_description, spark_protocol_instance(),
              particle::protocol::DESCRIBE_METRICS, nullptr);

template <class Timer>
VitalsPublisher<Timer>::VitalsPublisher(Timer* timer_)
    : _period_s(std::numeric_limits<system_tick_t>::max()),
      _timer(timer_ ? timer_ : onDemandTimerInstance())
{
}

template <class Timer>
VitalsPublisher<Timer>::~VitalsPublisher(void)
{
    _timer->dispose();
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
Timer* VitalsPublisher<Timer>::onDemandTimerInstance(void)
{
    static Timer* timer = new Timer(_period_s, &VitalsPublisher::publishFromTimer, *this, false);
    return timer;
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
    return _publishVitals();
}

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
        _publishVitals();
    };
    SystemISRTaskQueue.enqueue(task);
}

#include "spark_wiring_timer.h"
#if PLATFORM_THREADING
template class VitalsPublisher<Timer>;
#else  // not PLATFORM_THREADING
template class VitalsPublisher<particle::NullTimer>;
#endif // PLATFORM_THREADING

#ifdef UNIT_TEST
#include "../test/unit_tests/mock_types.h"
template class VitalsPublisher<particle::mock_type::Timer>;
#endif // UNIT_TEST
