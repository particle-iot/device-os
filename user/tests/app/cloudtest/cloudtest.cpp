#include "Particle.h"

SerialLogHandler logHandler(LOG_LEVEL_ALL);

#define FUNC_KEY_MIN "F"
#define EVENT_NAME_MAX "1234567890123456789012345678901234567890123456789012345678901234"
#if PLATFORM_ID == 0
// 12
#define FUNC_KEY_MAX "FUN126789012"
#else
// 64
#define FUNC_KEY_MAX "FUN646789012345678901234567890123456789012345678901234567890ABCD"
#endif

bool variableBool = false;
String variableString;
double variableDouble = 0.11;
int variableInt;

String next_cmd;

int cmd(String arg)
{
	if (next_cmd.length())
		return 1;
	next_cmd = arg;
	return 0;
}

int update(String data)
{
    variableBool = !variableBool;
    variableDouble += 0.11;
    variableInt += 1;
    return 0;
}

int updateString(String data)
{
    Serial.printlnf("[%s]", data.c_str());
    // Let the String class provide protections here
    variableString = data;
    return variableString.length();
}

int setString(String data)
{
    Serial.printlnf("[%s]", data.c_str());
    variableString = "";
    int len = data.toInt();
    for (int i=0; i<len; i++)
    {
        variableString.concat(char('A'+(i%26)));
    }
    return variableString.length();
}

int checkString(String data)
{
    int len = data.length();
    const char* s = data.c_str();
    for (int i=0; i<len; i++)
    {
        if (s[i]!=('A'+(i%26)))
            return -i;
    }
    return len;
}

void subscription(const char* name, const char* data)
{
	// Serial.print("subscription received: ");
	// Serial.print(name);
	// Serial.print(" ");
	// Serial.print(data);
	// Serial.println();

	updateString(data);
}

void subscription2(const char* name, const char* data)
{
    // Serial.print("subscription received: ");
    // Serial.print(name);
    // Serial.print(" ");
    // Serial.print(data);
    // Serial.println();

    updateString(data);
}

bool disconnect = false;

void setup()
{
    Serial.begin();

    variableString.reserve(622);
    Particle.variable("bool", variableBool);
    Particle.variable("int", variableInt);
    Particle.variable("double", variableDouble);
    Particle.variable("string", variableString);

    Particle.function("updateString", updateString);
    Particle.function("update", update);
    Particle.function("setString", setString);
    Particle.function("checkString", checkString);
    Particle.function(FUNC_KEY_MAX, updateString);
    Particle.function(FUNC_KEY_MIN, updateString);

    Particle.function("cmd", cmd);

    Particle.subscribe("cloudtest", subscription, MY_DEVICES);
    Particle.subscribe(EVENT_NAME_MAX, subscription2, MY_DEVICES);
}

#if Wiring_Cellular
/**
 * Returns current modem type:
 * DEV_UNKNOWN, DEV_SARA_G350, DEV_SARA_U260, DEV_SARA_U270, DEV_SARA_U201, DEV_SARA_R410
 */
int cellular_modem_type() {
    CellularDevice device;
    memset(&device, 0, sizeof(device));
    device.size = sizeof(device);
    cellular_device_info(&device, NULL);

    return device.dev;
}
#endif // Wiring_Cellular

void do_cmd(String arg)
{
	if (arg.equals("bounce_pdp"))
	{
#if Wiring_Cellular
        if (cellular_modem_type() == DEV_SARA_R410) {
            return;
        }
        // Wait 10s for the function return value to be sent
        for (system_tick_t s = millis(); millis() - s <= 10000UL; ) {
            // Wish we had a waitFor(Particle.idle, 10000UL);
            Particle.process();
        }
		cellular_command(nullptr, nullptr, 40*1000, "AT+UPSDA=0,4\r\n");
		cellular_command(nullptr, nullptr, 40*1000, "AT+UPSDA=0,3\r\n");
		Serial.println("bounced pdp context");
		// disconnect = true;
        // only delay for 20s on Cellular devices
        for (system_tick_t s = millis(); millis() - s <= 20000UL; ) {
            Particle.process();
            if (Particle.connected()) {
                break;
            }
        }
 #endif // Wiring_Cellular
	}
	else if (arg.equals("bounce_connection"))
	{
		disconnect = true;
	}
	else if (arg.equals("drop_dtls_session"))
	{
		Particle.publish("spark/device/session/end","", PRIVATE);
	}
	else if (arg.equals("reset"))
	{
		System.reset();
	}
}

void loop()
{
	if (disconnect)
	{
		Particle.disconnect();
		Particle.process();
		disconnect = false;
	}

	if (Particle.disconnected())
	{
		Particle.connect();
		Particle.process();
	}

	if (next_cmd.length())
	{
		String cmd = next_cmd;
		next_cmd = "";
		do_cmd(cmd);
	}
}

