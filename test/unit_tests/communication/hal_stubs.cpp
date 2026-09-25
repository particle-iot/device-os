// these are HAL functions that are called directly by the DTLS
// implementation. Over time these should disappear as we move
// the DTLS functions into callbacks, so we get more control over
// the return value from
// the _gettimeofday() and similar functions that call HAL functions directly.

#include <stdint.h>
#include <stdlib.h>
#include "logging.h"
#include "diagnostics.h"

#include "buffer_message_channel.h"
#include "protocol.h"
#include "spark_protocol_functions.h"

namespace particle::protocol {

class Protocol;

} // namespace particle::protocol

extern "C" uint32_t HAL_RNG_GetRandomNumber()
{
	return rand();
}


extern "C" uint32_t HAL_Timer_Get_Milli_Seconds()
{
	static uint32_t millis = 0;
	return ++millis;
}


extern "C" uint32_t HAL_Timer_Get_Micro_Seconds()
{
	return HAL_Timer_Get_Milli_Seconds()*1000;
}

extern "C" uint32_t HAL_Core_Compute_CRC32(const uint8_t* buf, size_t length)
{
	return 0;
}

extern "C" void log_message(int level, const char *category, LogAttributes *attr, void *reserved, const char *fmt, ...)
{
}

extern "C" void log_write(int level, const char *category, const char *data, size_t size, void *reserved)
{
}

extern "C" int diag_register_source(const diag_source* src, void* reserved) {
	return 0;
}

extern "C" particle::protocol::Protocol* spark_protocol_instance(void);

// Allows tests to override the protocol instance returned by spark_protocol_instance()
particle::protocol::Protocol* g_protocolInstance = nullptr;

namespace particle::protocol {

namespace {

// Message sending and receiving is not supported, only buffer allocation
class DefaultMessageChannel: public MessageChannel {
public:
    // Reimplemented from MessageChannel
    ProtocolError receive(Message& msg) override {
        return ProtocolError::NO_ERROR;
    }
    ProtocolError send(Message& msg) override {
        return ProtocolError::NO_ERROR;
    }
    ProtocolError command(Command cmd, void* arg) override {
        return ProtocolError::NO_ERROR;
    }
    bool is_unreliable() override {
        return true;
    }
    ProtocolError establish() override {
        return ProtocolError::NO_ERROR;
    }
    ProtocolError create(Message& msg, size_t minimumSize) override {
        if (minimumSize > sizeof(queue_)) {
            return ProtocolError::INSUFFICIENT_STORAGE;
        }
        msg.clear();
        msg.set_buffer(queue_, sizeof(queue_));
        msg.set_length(0);
        return ProtocolError::NO_ERROR;
    }
    ProtocolError response(Message& original,
            Message& response, size_t required) override {
        return ProtocolError::NO_ERROR;
    }
    ProtocolError notify_established() override {
        return ProtocolError::NO_ERROR;
    }
    void notify_client_messages_processed() override {
    }
    AppStateDescriptor cached_app_state_descriptor() const override {
        return AppStateDescriptor();
    }
    void reset() override {
    }
    void set_debug_enabled(bool enabled) override {
    }

private:
    uint8_t queue_[1500];
};

// Default protocol instance for tests that don't install their own
class DefaultProtocol: public Protocol {
public:
    explicit DefaultProtocol() :
            Protocol(channel_) {
        SparkCallbacks callbacks = {};
        callbacks.size = sizeof(callbacks);
        callbacks.millis = []() -> system_tick_t {
            return ++s_millis;
        };
        SparkDescriptor descriptor = {};
        descriptor.size = sizeof(descriptor);
        Protocol::init(callbacks, descriptor);
    }

    // Reimplemented from Protocol
    void init(const char* id, const SparkKeys& keys, const SparkCallbacks& callbacks,
            const SparkDescriptor& descriptor) override {
    }
    int command(ProtocolCommands::Enum cmd, uint32_t val, const void* data) override {
        return 0;
    }
    size_t build_hello(Message& msg, uint16_t flags) override {
        return 0;
    }
    int get_status(protocol_status* status) const override {
        return 0;
    }

private:
    static system_tick_t s_millis;

    DefaultMessageChannel channel_;
};

system_tick_t DefaultProtocol::s_millis = 0;

Protocol* defaultProtocolInstance() {
    if (g_protocolInstance) {
        return g_protocolInstance;
    }
    static DefaultProtocol protocol; // Created on first use
    return &protocol;
}

} // namespace

} // namespace particle::protocol

extern "C" particle::protocol::Protocol* spark_protocol_instance(void) {
	return particle::protocol::defaultProtocolInstance();
}