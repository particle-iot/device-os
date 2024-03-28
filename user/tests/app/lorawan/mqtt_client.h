#pragma once

#include <functional>
#include <string_view>
#include <memory>

namespace particle {

class MqttClient {
public:
    typedef std::function<void()> ConnectedFn;
    typedef std::function<void(int error)> DisconnectedFn;
    typedef std::function<void()> MessageSentFn;
    typedef std::function<void(std::string_view topic, std::string_view data)> MessageReceivedFn;

    MqttClient();
    ~MqttClient();

    void connect();
    void disconnect();
    bool isConnected();
    
    void publish(std::string_view topic, std::string_view data, MessageSentFn fn = MessageSentFn());
    void subscribe(std::string_view topic, MessageReceivedFn fn);

    void run();

    MqttClient& host(std::string_view host);
    MqttClient& port(int port);
    MqttClient& user(std::string_view user);
    MqttClient& password(std::string_view password);
    MqttClient& connected(ConnectedFn fn);
    MqttClient& disconnected(DisconnectedFn fn);

private:
    struct Data;

    std::unique_ptr<Data> d_;
};

} // namespace particle
