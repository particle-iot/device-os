#ifndef VITALS_PUBLISHER_H
#define VITALS_PUBLISHER_H

#include <cstddef>

namespace particle {
namespace wiring {
namespace cloud {

template <
    typename publish_fn_t,
    class Timer
>
class VitalsPublisher {
  public:
    VitalsPublisher (
        publish_fn_t fn_,
        const Timer & timer_
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

    size_t
    period (
        void
    ) const;

    void
    period (
        size_t period_s
    );

    bool
    publish (
        void
    );

  private:
    publish_fn_t _publishVitals;
    Timer _timer;
    size_t _period_s;
};

} // namespace cloud
} // namespace wiring
} // namespace particle

#endif