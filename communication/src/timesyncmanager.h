#pragma once

#include "protocol_defs.h"
#include "service_debug.h"

namespace particle { namespace protocol {

class TimeSyncManager
{
public:
    TimeSyncManager()
        : lastSyncMillis_{0},
          requestSentMillis_{0},
          lastSyncTime_{0},
          expectingResponse_{false}
    {
    }

    void reset()
    {
        expectingResponse_ = false;
        requestSentMillis_ = 0;
    }

    template <typename Callback>
    bool send_request(system_tick_t mil, Callback send_time_request) {
        if (expectingResponse_) {
            return true;
        }

        requestSentMillis_ = mil;
        expectingResponse_ = true;
        LOG(INFO, "Sending TIME request");
        return send_time_request();
    }

    template <typename Callback>
    bool handle_time_response(time_t tm, system_tick_t mil, Callback set_time) {
        LOG(INFO, "Received TIME response: %lu", (unsigned long)tm);
        set_time(tm, 0, NULL);
        expectingResponse_ = false;
        lastSyncTime_ = tm;
        lastSyncMillis_ = mil;
        return true;
    }

    bool is_request_pending() const {
        return expectingResponse_;
    }

    system_tick_t last_sync(time_t& tm) const {
        tm = lastSyncTime_;
        return lastSyncMillis_;
    }


private:
    system_tick_t lastSyncMillis_;
    system_tick_t requestSentMillis_;
    time_t lastSyncTime_;
    bool expectingResponse_;
};


}} // namespace particle::protocol
