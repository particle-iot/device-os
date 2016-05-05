
#include "application.h"

//STARTUP(WiFi.selectAntenna(ANT_EXTERNAL));


struct Stats {

    size_t size;
    size_t total;
    size_t success;
    size_t fail;
    time_t start_time;

    size_t printTo(Print& p) const
    {
        size_t result = p.print("total OTA: ");
        result += p.println(total);

        result += p.print("Success: ");
        result += p.println(success);

        result += p.print("Errors: ");
        result += p.println(fail);

        result += p.print("Uncounted: ");
        result += p.println(total-(success+fail));
        result += p.println();
        return result;
    }

    void load()
    {
        EEPROM.get(0, *this);
        if (size<sizeof(*this)) {
            memset(((uint8_t*)this)+size, 0, sizeof(*this)-size);
        }
    }

    void save()
    {
        EEPROM.put(0, *this);
    }

    int reset()
    {
        memset(this, 0, sizeof(*this));
        this->size = sizeof(*this);
        start_time = Time.now();
        save();
        return 0;
    }

} stats;

void update_begin()
{
    Serial.print("OTA update started: ");
    Serial.println(Time.timeStr());
    stats.printTo(Serial);
    stats.save();
}

void update_end(bool success)
{
    if (success)
        Serial.print("OTA complete: ");
    else
        Serial.print("OTA failed: ");
    Serial.println(Time.timeStr());
    stats.printTo(Serial);
    stats.save();
}

void system_event_handler(system_event_t events, int param, void* pointer)
{
    if (events&firmware_update)
    {
        switch (param)
        {
        case firmware_update_begin:
            stats.total++;
            update_begin();
            break;

        case firmware_update_complete:
            stats.success++;
            update_end(true);
            break;

        case firmware_update_failed:
            stats.fail++;
            update_end(false);
            break;
        }
    }
}

void setup()
{
    Serial.begin(9600);
    stats.load();
    System.on(all_events, system_event_handler);

    Particle.function("reset", [](String){ return stats.reset(); }, NULL);
    Particle.function("print", [](String){ return stats.printTo(Serial); }, NULL);
    Particle.variable("ready", "1", STRING);

    stats.printTo(Serial);
}

void loop()
{
}
