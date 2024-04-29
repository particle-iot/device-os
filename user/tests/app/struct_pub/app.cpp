#include <application.h>

namespace {

void onEventSentCallback(int error, Event event) {
    if (error < 0) {
        Log.error("Failed to send event: %s, error: %d", event.name(), error);
        return;
    }
    Log.info("Sent event: %s", event.name());
}

void publishingBufferWithBinaryData() {
    uint8_t buf[100] = {};
    // ...

    Event event = Particle.publish("my_event", buf, sizeof(buf));
    event.onSent(onEventSentCallback);
}

void publishingBinaryDataUsingStream() {
    uint8_t buf[100] = {};
    // ...

    Event event = Particle.beginPublish("my_event");
    event.write(buf, sizeof(buf));
    event.write(buf, sizeof(buf));
    event.onSent(onEventSentCallback);
    event.end();
}

void publishCborUsingVariant() {
    Variant v;
    v["key_1"] = 123;
    v["key_2"].append("nested_value_1");
    v["key_2"].append("nested_value_2");

    Event event = Particle.publish("my_event", v);
    event.onSent(onEventSentCallback);
}

void publishCborUsingStream() {
    int result = Particle.beginEvent("my_event") // beginEvent() returns an Event instance
            .beginMap()
                .key("key_1").value(123)
                .key("key_2").beginArray()
                        .value("nested_value_1")
                        .value("nested_value_2")
                        .endArray()
            .endMap()
            .onSent(onEventSentCallback)
            .end();

    if (result < 0) {
        Log.error("Failed to publish event: %d", result);
    }
}

void subscribeUsingVariant() {
    Particle.subscribe("my_event", [](const char* name, Variant data) {
        Log.info("Received event: %s", name);

        int value1 = data["key_1"].toInt();
        Log.info("key_1 = %d", value1);

        Variant value2 = data["key_2"];
        for (int i = 0; i < value2.size(); ++i) {
            Log.info("key_2[%d] = %s", i, value2[i].toString().c_str());
        }
    });
}

void subscribeUsingStream() {
    Particle.subscribe("my_event", [](Event event) {
        Log.info("Received event: %s", event.name());

        event.enterMap();
        while (event.hasNext()) {
            auto key = event.nextKey();
            Variant value = event.nextValue();
            if (key == "key_1") {
                Log.info("key_1 = %d", value.toInt());
            } else if (key == "key_2") {
                event.enterArray();
                int index = 0;
                while (event.hasNext()) {
                    auto value = event.nextValue();
                    Log.info("key_2[%d] = %s", index, value.toString().c_str());
                    ++index;
                }
                event.leaveArray();
            }
        }
        event.leaveMap();

        if (!event.ok()) {
            Log.error("Error while parsing event: %d", event.error());
        }
    });
}

void subscribeUsingStreamWithExplicitValidation() {
    Particle.subscribe("my_event", [](Event event) {
        Log.info("Received event: %s", event.name());

        if (event.nextType() != Variant::MAP) {
            Log.error("Expected event data to be a map");
            return;
        }

        event.enterMap();
        while (event.hasNext()) {
            if (event.nextType() != Variant::STRING) {
                Log.error("Expected map to have string keys");
                return;
            }

            auto key = event.key().toString();
            if (key == "key_1") {
                if (event.nextType() != Variant::INT) {
                    Log.error("Expected key_1 to have an integer value");
                    return;
                }
                int value = event.value().toInt();
                Log.info("key_1 = %d", value);

            } else if (key == "key_2") {
                if (event.nextType() != Variant::ARRAY) {
                    Log.error("Expected key_2 to have an array value");
                    return;
                }
                event.enterArray();
                int index = 0;
                while (event.hasNext()) {
                    if (event.nextType() != Variant::STRING) {
                        Log.error("Expected key_2 array to contain strings");
                        return;
                    }
                    auto value = event.nextValue().toString();
                    Log.info("key_2[%d] = %s", index, value.c_str());
                    ++index;
                }
                event.leaveArray();
            }
        }
        event.leaveMap();

        if (!event.ok()) {
            Log.error("Error while parsing event: %d", event.error());
        }
    });
}

} // namespace

void setup() {
}

void loop() {
}
