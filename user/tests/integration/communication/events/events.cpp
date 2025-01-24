#include "application.h"
#include "test.h"

namespace {

String eventName;
EventData eventData;
ContentType eventContentType = ContentType();
bool eventReceived = false;

void clearReceivedEvent() {
    eventName = String();
    eventData = EventData();
    eventContentType = ContentType();
    eventReceived = false;
}

void eventHandler1(const char* name, const char* data, size_t size, ContentType type) {
    clearReceivedEvent();
    eventName = name;
    eventData = Buffer(data, size);
    eventContentType = type;
    eventReceived = true;
}

void eventHandler2(const char* name, EventData data) {
    clearReceivedEvent();
    eventName = name;
    eventData = std::move(data);
    eventReceived = true;
}

} // namespace

test(connect) {
    Particle.subscribe(System.deviceID() + "/my_event1", eventHandler1);
    Particle.subscribe(System.deviceID() + "/my_event2", eventHandler2);
    Particle.connect();
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
}

test(particle_publish_publishes_an_event) {
    assertTrue(Particle.connected());
    assertTrue((bool)Particle.publish("my_event", "event data", PRIVATE | WITH_ACK));
}

test(get_max_event_data_size) {
    char str[16] = {};
    snprintf(str, sizeof(str), "%d", Particle.maxEventDataSize());
    assertTrue(Particle.connected());
    assertTrue((bool)Particle.publish("max_event_data_size", str, WITH_ACK));
}

test(verify_max_event_data_size) {
    // 1500-character string
    auto data = "eI568Df9nXQmUyaDeNE7A4pZnrcdaAxetam6QYQe3lXFwzN3A6ZO2VGutxVBbIWc8EyrqFMtzKByspno2vL1bGB9H6btc5GWysJZ3XLa3paAmAG4P3UZcbg4NuSRTSEr2YsDMTIEF2lSdd51YR0BPsbcEiQN29ufOpfEHXqK7LfJ3lfEMySnl0iX3ajaQ9rlLsKF4vhSoLFQDp3SRAmzfHhLCDHqVFDT9o8I4Ac5ER6cPl5k8wucWJqxQVWCHB2jdrtSX3WNX8Uq14mAuS4L4s2SeP6UlCcWXrzV9AAuBeTON9Jw7Lbe09F7Ijz0KxIPlwnVZDqXV09GbxKXIOA41E1ZeR9Cg23vozKZZzn2cWeeYtJmRi5Evmwmjus72XQM1W7KGZABrQbzSZawK0pRk9Cp7kl2uy39IjxL6ev3nlC8EA2DE7zi1DJHW7bJceUvFevQcHjWHU5FNKx7m48SG2046PDxxl0vnkXQ6hompl04RFmjUnIgEfIT9XZCkes5lPa8T2V8Ueo7aDfPBYSZOX35XBCczj6nXZ9oxVqn9zxH5NrLcmeDsLop77PVmdJles0CWEAAr5zNVOxIETN2jJcksLXRfQ1pESo9YLaBTyjSuDRQqMenYwuv2qFFnEbaZCMqBQRvE4ql0Oo6K9rXKdfO5G8b9c9jSI4g56f1DAiv7iWU99NdMUMVFt2LmYZsT0azi6MztjRsbtVRG2thZUqAhaPuhvZd0Efbd5H01oUN2CIsh9NiMdEkG5ouSMVaLGjIuvfDeFnlKjL7wSvmNauWYQY021dCKfpJCx0Q7XRB9kFDWZLcew61CmCHsEctM4JldvVhKLdWcnKFDttz3CfbFgtkGBVPWSW0hOwA2e5SLNwHyyJyJXNsicFxMpelYlVAhFjSR8nXe0cJqylvmKYUQ85H2Qet4kehs4boQLIqTHeDoDy1ITDbNVnv3PWzbna5kmEiBhyRw4kn6Di1a6r7uamd5fgFAGURi9LYCp3wAuw6PbYpq8rFXFFzkOUniI3q5c1bLDFxRS4zxNOuH31W819DZGM57zimuZ8YeEfAljxmSOeUWQQdlJjZbjgvERF1Dlexe4nROXyDOadc4qlznOKL0u2ttG0hCVPHMXG4s4uP8YLXJMhyNZod6mkdW9R42aWAsJgDMZZnuU7J7HJL9OpOZXPDCl1l2wOlPCyUtVQzG7PD1Db0dIaTMe9YnFtNAPPxAD4JQXNKMkmWRrhVE2VuJlNvokoCZp9pBDYBFJEPHOYWZI93gsR2tdSIa7YQslZRykJRAF90xlBfNvljN9yR64g7Q1IKCbGwr59H2I5WFEHruiIFJpPs9QQOYxlgq9juAJ9GyfmpEwuvF6n49Bi34v9dQGwt5ZMRFB6HgoRTb9PaLCp4e0Ns7zYYY2rWwESeZnPsqADsFFG3pxsisIn8pjLdAlJrAdMiyUGaIvi7Vj6uFmClZMI8i39pnWXfJbUSJtofdeCthZD2awxZJMjC";
    assertTrue(Particle.connected());
    assertTrue((bool)Particle.publish("my_event", data, PRIVATE | WITH_ACK));
}

test(publish_device_to_cloud_event_with_content_type) {
    auto data = Buffer::fromHex("92b2d100238c490bd6e7a89050dbc5bbe53ff329b24cefadadd810d9c3a4f1b0e7748e7e2ecf48be4dd3ae08368f76a8d550ec139d5bca624e3c6b3cbc758565356c00afee12f2bd3ff22700dc4cc6fa02168ee5a1e6e9404a71d17da5a6b78d7d475ff2");
    assertTrue(Particle.connected());
    assertTrue((bool)Particle.publish("my_event", data.data(), data.size(), ContentType::BINARY, WITH_ACK));
}

test(publish_device_to_cloud_event_as_variant) {
    EventData v;
    v["a"] = 123;
    v["b"] = Buffer::fromHex("6e7200463f1bb774472f");
    assertTrue(Particle.connected());
    assertTrue((bool)Particle.publish("my_event", v, WITH_ACK));
}

test(publish_cloud_to_device_event_with_content_type) {
    clearReceivedEvent();
}

test(validate_cloud_to_device_event_with_content_type) {
    assertTrue(waitFor([]() {
        return eventReceived;
    }, 60000));
    auto expectedData = Buffer::fromHex("cbdf4383008631728baf35c2aaae5d2e777691b131a5f1051d7fc147a81f2a90e575309ddc2890688bb86e6e85140d95c064fdf3ce3dfb45a3a7fe3dcf94d869cb21392c9ac9bb5bcb2dd943b5be21dd3be18c876468004d981e6ae02a42e805b989efcd");
    assertTrue(eventName == System.deviceID() + "/my_event1");
    assertTrue(eventData.asBuffer() == expectedData);
    assertTrue(eventContentType == ContentType::BINARY);
}

test(publish_cloud_to_device_event_with_cbor_data) {
    clearReceivedEvent();
}

test(validate_cloud_to_device_event_with_cbor_data) {
    assertTrue(waitFor([]() {
        return eventReceived;
    }, 60000));
    assertTrue(eventName == System.deviceID() + "/my_event2");
    EventData v;
    v["c"] = 456;
    v["d"] = Buffer::fromHex("921bff008d91814e789b");
    assertTrue(eventData == v);
}
