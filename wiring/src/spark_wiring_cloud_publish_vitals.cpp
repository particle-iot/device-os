
#include "spark_wiring_cloud_publish_vitals.h"

#include "logging.h"

using namespace particle::wiring::cloud;

template <
    typename publish_fn_t,
    class Timer
>
VitalsPublisher<publish_fn_t, Timer>::VitalsPublisher (
    publish_fn_t fn_,
    const Timer & timer_
) :
    _publishVitals(fn_),
    _timer(timer_)
{

}

template <
    typename publish_fn_t,
    class Timer
>
VitalsPublisher<publish_fn_t, Timer>::~VitalsPublisher (
    void
) {
    _timer.dispose();
}

template <
    typename publish_fn_t,
    class Timer
>
void
VitalsPublisher<publish_fn_t, Timer>::disablePeriodicPublish (
    void
) {
    _timer.stop();
}

template <
    typename publish_fn_t,
    class Timer
>
void
VitalsPublisher<publish_fn_t, Timer>::enablePeriodicPublish (
    void
) {
    _timer.start();
}

template <
    typename publish_fn_t,
    class Timer
>
size_t
VitalsPublisher<publish_fn_t, Timer>::period (
    void
) const {
    return _period_s;
}

template <
    typename publish_fn_t,
    class Timer
>
void
VitalsPublisher<publish_fn_t, Timer>::period (
    size_t period_s_
) {
    const size_t period_ms = period_s_ * 1000;
    const bool was_active = _timer.isActive();

    if ( _timer.changePeriod(period_ms) ) {
        LOG(ERROR, "Unable to update timer period!");
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

template <
    typename publish_fn_t,
    class Timer
>
bool
VitalsPublisher<publish_fn_t, Timer>::publish (
    void
) {
    return _publishVitals();
}

#include <functional>
#include "spark_wiring_timer.h"

template class VitalsPublisher<std::function<bool(void)>, Timer>;
