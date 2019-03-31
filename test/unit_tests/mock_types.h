#ifndef MOCK_TYPES_H
#define MOCK_TYPES_H

#include <cstddef>

namespace particle {
namespace mock_type {

class Timer {
    public:
    virtual ~Timer (void) = default;
    virtual bool changePeriod (const size_t) { return false; }
    virtual void dispose (void) { }
    virtual bool isActive (void) { return false; }
    virtual void reset (void) { }
    virtual void start (void) { }
    virtual void stop (void) { }
};

} // namespace mock_type
} // namespace particle

#endif // MOCK_TYPES_H
