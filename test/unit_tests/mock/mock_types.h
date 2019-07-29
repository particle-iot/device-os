#ifndef MOCK_TYPES_H
#define MOCK_TYPES_H

#include <cstddef>

#include "spark_wiring_timer.h"

namespace particle
{
namespace mock_type
{

class Timer
{
public:
    Timer(unsigned, ::Timer::timer_callback_fn, bool) {
    }

    template <typename T>
    Timer(unsigned, void (T::*)(), T&, bool)
    {
    }

    virtual ~Timer(void) = default;
    virtual bool changePeriod(const size_t)
    {
        return false;
    }
    virtual void dispose(void)
    {
    }
    virtual bool isActive(void)
    {
        return false;
    }
    virtual void reset(void)
    {
    }
    virtual void start(void)
    {
    }
    virtual void stop(void)
    {
    }
};

} // namespace mock_type
} // namespace particle

#endif // MOCK_TYPES_H
