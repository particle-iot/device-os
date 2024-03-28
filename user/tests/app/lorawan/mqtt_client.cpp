#include <type_traits>
#include <stdexcept>
#include <map>

#include <mqtt_client_cpp.hpp>
#include <mqtt/setup_log.hpp>

#include <spark_wiring_error.h>
#include <spark_wiring_logging.h>

#include "mqtt_client.h"

namespace particle {

struct MqttClient::Data {
    using Client = decltype(mqtt::make_tls_sync_client(std::declval<boost::asio::io_context&>(), std::string(), 0, mqtt::protocol_version::v3_1_1));

    Client client;
    boost::asio::io_context ioCtx;
    std::map<uint16_t, MessageSentFn> pubComp;
    std::map<std::string, MessageReceivedFn> subTopics;
    ConnectedFn connFn;
    DisconnectedFn disconnFn;
    std::string host;
    std::string user;
    std::string password;
    int port;
    int connError;
    bool connected;
    bool connecting;

    Data() :
            port(1883),
            connError(0),
            connected(false),
            connecting(false) {
    }
};

MqttClient::MqttClient() {
    mqtt::setup_log(mqtt::severity_level::trace);
    d_ = std::make_unique<Data>();
}

MqttClient::~MqttClient() {
}

void MqttClient::connect() {
    if (d_->connected || d_->connecting) {
        return;
    }
    d_->client = mqtt::make_tls_sync_client(d_->ioCtx, d_->host, d_->port);
    d_->client->get_ssl_context().set_verify_mode(boost::asio::ssl::verify_none);
    d_->client->set_user_name(d_->user);
    d_->client->set_password(d_->password);
    d_->client->set_connack_handler([this](bool sessPresent, mqtt::connect_return_code retCode) {
        Log.info("mqtt: connack");
        Log.info("mqtt: return code: %s", mqtt::connect_return_code_to_str(retCode));
        d_->connecting = false;
        if (retCode != mqtt::connect_return_code::accepted) {
            return false;
        }
        d_->connected = true;
        if (d_->connFn) {
            d_->connFn();
        }
        return true;
    });
    d_->client->set_close_handler([this]() {
        Log.info("mqtt: disconnected");
        int error = d_->connError;
        d_->connError = 0;
        d_->connecting = false;
        if (d_->connected) {
            d_->connected = false;
            if (d_->disconnFn) {
                d_->disconnFn(error);
            }
        }
    });
    d_->client->set_error_handler([this](mqtt::error_code err) {
        Log.error("mqtt: error: %s", err.message().c_str());
        d_->connError = Error::NETWORK;
        return false;
    });
    d_->client->set_pubcomp_handler([this](uint16_t packetId) {
        Log.info("mqtt: pubcomp: %d", (int)packetId);
        auto it = d_->pubComp.find(packetId);
        if (it != d_->pubComp.end()) {
            auto fn = std::move(it->second);
            d_->pubComp.erase(it);
            fn();
        }
        return true;
    });
    d_->client->set_publish_handler([this](std::optional<uint16_t> /* packetId */, mqtt::publish_options /* opts */,
            mqtt::buffer topic, mqtt::buffer data) {
        auto it = d_->subTopics.find(std::string(topic));
        if (it != d_->subTopics.end()) {
            it->second(topic, data);
        }
        return true;
    });
    d_->client->set_clean_session(true);
    d_->client->connect(); // Establishes a TLS connection
    d_->connecting = true;
}

bool MqttClient::isConnected() {
    return d_->connected;
}

void MqttClient::publish(std::string_view topic, std::string_view data, MessageSentFn fn) {
    if (!d_->connected) {
        throw std::runtime_error("Not connected");
    }
    auto packetId = d_->client->publish(std::string(topic), std::string(data), mqtt::qos::exactly_once);
    if (fn) {
        if (!packetId) {
            throw std::runtime_error("Internal error: Invalid QoS level");
        }
        d_->pubComp[packetId] = fn;
    }
}

void MqttClient::subscribe(std::string_view topic, MessageReceivedFn fn) {
    if (!fn) {
        throw std::runtime_error("Invalid arguments");
    }
    if (!d_->connected) {
        throw std::runtime_error("Not connected");
    }
    std::string t(topic);
    if (d_->subTopics.count(t)) {
        return;
    }
    d_->client->subscribe(t, mqtt::qos::exactly_once);
    d_->subTopics[t] = fn; // XXX: Wildcards in topics are not supported
}

void MqttClient::run() {
    d_->ioCtx.run_one_for(std::chrono::milliseconds(1));
}

MqttClient& MqttClient::host(std::string_view host) {
    d_->host = host;
    return *this;
}

MqttClient& MqttClient::port(int port) {
    d_->port = port;
    return *this;
}

MqttClient& MqttClient::user(std::string_view user) {
    d_->user = user;
    return *this;
}

MqttClient& MqttClient::password(std::string_view password) {
    d_->password = password;
    return *this;
}

MqttClient& MqttClient::connected(ConnectedFn fn) {
    d_->connFn = std::move(fn);
    return *this;
}

MqttClient& MqttClient::disconnected(DisconnectedFn fn) {
    d_->disconnFn = std::move(fn);
    return *this;
}

} // namespace particle
