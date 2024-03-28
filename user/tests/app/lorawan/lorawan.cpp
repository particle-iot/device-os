#include <stdexcept>

#include <boost/json.hpp>
#include <boost/beast/core/detail/base64.ipp> // FIXME

#include <spark_wiring_error.h>
#include <spark_wiring_logging.h>

#include "lorawan.h"

using namespace std::literals;

namespace particle {

namespace {

const auto MQTT_HOST = "htmqtt.subspace-767871877483.sandbox.spark.li";
const auto MQTT_PORT = 8883;
const auto MQTT_USER = "sergey";
const auto MQTT_PASSWORD =                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    "Tr9^pjT6aYIvRhm&Pm5T";

const auto HELIUM_DEVICE_ID = "8af63987-9ccf-4e39-9777-826dbe5ee975";
const auto DEV_EUI = "2BF7F1503230A158";
const auto APP_EUI = "526973696E674847";
const auto DEV_ADDR = "41000148";

std::string toBase64(const util::Buffer& buf) {
    auto size = boost::beast::detail::base64::encoded_size(buf.size());
    std::string s;
    s.resize(size);
    size = boost::beast::detail::base64::encode(s.data(), buf.data(), buf.size());
    s.resize(size);
    return s;
}

util::Buffer fromBase64(std::string_view str) {
    auto size = boost::beast::detail::base64::decoded_size(str.size());
    util::Buffer buf;
    if (buf.resize(size) < 0) {
        throw std::bad_alloc();
    }
    auto r = boost::beast::detail::base64::decode(buf.data(), str.data(), str.size());
    buf.resize(r.first);
    return buf;
}

std::string formatMessage(const util::Buffer& payload, int port, int fcnt) {
    boost::json::object msg;
    msg["id"] = HELIUM_DEVICE_ID;
    msg["dev_eui"] = DEV_EUI;
    msg["devaddr"] = DEV_ADDR;
    msg["app_eui"] = APP_EUI;
    msg["type"] = "uplink";
    msg["port"] = port;
    msg["fcnt"] = fcnt;
    msg["payload"] = toBase64(payload);
    msg["payload_size"] = payload.size();
    return boost::json::serialize(msg);
}

} // namespace

int Lorawan::init(LorawanConfig conf) {
    try {
        if (inited_) {
            return 0;
        }
        mqtt_.host(MQTT_HOST);
        mqtt_.port(MQTT_PORT);
        mqtt_.user(MQTT_USER);
        mqtt_.password(MQTT_PASSWORD);
        mqtt_.connected([this]() {
            if (conf_.connected()) {
                conf_.connected()();
            }
        });
        mqtt_.disconnected([this](int error) {
            if (conf_.disconnected()) {
                conf_.disconnected()(error);
            }
        });
        conf_ = std::move(conf);
        inited_ = true;
        return 0;
    } catch (const std::exception& e) {
        Log.error("Lorawan::init() failed: %s", e.what());
        return Error::NETWORK;
    }
}

int Lorawan::connect() {
    try {
        if (!inited_) {
            throw std::runtime_error("Not initialized");
        }
        mqtt_.connect();
#if 0
        mqtt_.subscribe("helium/"s + HELIUM_DEVICE_ID + "/tx", [](std::string_view /* topic */, std::string_view data) {
            //auto d = boost::algorithm::unhex(data);
            //util::Buffer buf(d.data(), d.size());
            //conf_.messageReceived()(std::move(buf), 1 /* port */);
        });
#endif
        return 0;
    } catch (const std::exception& e) {
        Log.error("Lorawan::connect() failed: %s", e.what());
        return Error::NETWORK;
    }
}

int Lorawan::send(util::Buffer data, int port, AckReceivedFn ackFn) {
    try {
        if (!inited_) {
            throw std::runtime_error("Not initialized");
        }
        auto msg = formatMessage(data, port, (uint16_t)(fcntUp_++));
        mqtt_.publish("helium/"s + HELIUM_DEVICE_ID + "/rx", msg);
        return 0;
    } catch (const std::exception& e) {
        Log.error("Lorawan::send() failed: %s", e.what());
        return Error::NETWORK;
    }
}

int Lorawan::run() {
    try {
        if (!inited_) {
            return 0;
        }
        mqtt_.run();
        return 0;
    } catch (const std::exception& e) {
        Log.error("Lorawan::run() failed: %s", e.what());
        return Error::NETWORK;
    }
}

} // namespace particle
