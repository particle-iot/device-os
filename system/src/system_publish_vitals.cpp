
#include "system_publish_vitals.h"

#include <limits>

#include "logging.h"

using namespace particle::system;

template <class Timer>
VitalsPublisher<Timer>::VitalsPublisher (
    publish_fn_t publish_fn_,
    const Timer & timer_
) :
    _period_s(std::numeric_limits<system_tick_t>::max()),
    _publishVitals(publish_fn_),
    _timer(timer_)
{

}

template <class Timer>
VitalsPublisher<Timer>::~VitalsPublisher (
    void
) {
    _timer.dispose();
}

template <class Timer>
void
VitalsPublisher<Timer>::disablePeriodicPublish (
    void
) {
    _timer.stop();
}

template <class Timer>
void
VitalsPublisher<Timer>::enablePeriodicPublish (
    void
) {
    _timer.start();
}

template <class Timer>
system_tick_t
VitalsPublisher<Timer>::period (
    void
) const {
    return _period_s;
}

template <class Timer>
void
VitalsPublisher<Timer>::period (
    system_tick_t period_s_
) {
    const system_tick_t period_ms = period_s_ * 1000;
    const bool was_active = _timer.isActive();

    if ( _timer.changePeriod(period_ms) ) {
        LOG(ERROR, "Unable to update vitals timer period!");
    } else {
        _period_s = period_s_;

        // Maintain pre-existing state
        if ( was_active ) {
            _timer.reset();
        } else {
            _timer.stop();
        }
    }
}

template <class Timer>
int
VitalsPublisher<Timer>::publish (
    void
) {
    return _publishVitals();
}

#if PLATFORM_ID == PLATFORM_SPARK_CORE
  template class VitalsPublisher<particle::NullTimer>;
#else // not PLATFORM_SPARK_CORE
  #include "spark_wiring_timer.h"
  template class VitalsPublisher<Timer>;
#endif // PLATFORM_SPARK_CORE
