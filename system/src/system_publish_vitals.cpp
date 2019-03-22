
#include "system_publish_vitals.h"

#include <limits>

#include "logging.h"
#include "protocol_defs.h"

using namespace particle::system;

template <
    typename publish_fn_t,
    class Timer
>
VitalsPublisher<publish_fn_t, Timer>::VitalsPublisher (
    ProtocolFacade * protocol_,
    publish_fn_t fn_,
    const Timer & timer_
) :
    _period_s(std::numeric_limits<system_tick_t>::max()),
    _protocol(protocol_),
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
system_tick_t
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
    system_tick_t period_s_
) {
    const system_tick_t period_ms = period_s_ * 1000;
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
int
VitalsPublisher<publish_fn_t, Timer>::publish (
    void
) {
    return _publishVitals(_protocol, particle::protocol::DESCRIBE_METRICS, nullptr);
}

#include <functional>

#if PLATFORM_ID == PLATFORM_SPARK_CORE
  template class VitalsPublisher<std::function<int(ProtocolFacade *, int, void *)>, particle::NullTimer>;
#else // not PLATFORM_SPARK_CORE
  #include "spark_wiring_timer.h"
  template class VitalsPublisher<std::function<int(ProtocolFacade *, int, void *)>, Timer>;
#endif // PLATFORM_SPARK_CORE

