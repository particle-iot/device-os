#include "application.h"

// ALL_LEVEL, TRACE_LEVEL, DEBUG_LEVEL, WARN_LEVEL, ERROR_LEVEL, PANIC_LEVEL, NO_LOG_LEVEL
//SerialDebugOutput debugOutput(9600, ALL_LEVEL);


bool variableBool = false;
char variableString[1024];
double variableDouble;
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
    variableDouble += 0.25;
    variableInt += 1;
    return 0;
}

int updateString(String data)
{
    size_t len = data.length();
    if (len>sizeof(variableString)-1)
        len = sizeof(variableString)-1;
    strncpy(variableString, data.c_str(), len);
    variableString[len] = 0;
    return data.length();
}

int setString(String data)
{
    int len = atoi(data.c_str());
    for (int i=0; i<len; i++)
    {
        variableString[i] = 'A'+(i%26);
    }
    variableString[len] = 0;
    return 0;
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
	Serial.print("subscription received: ");
	Serial.print(name);
	Serial.print(" ");
	Serial.print(data);
	Serial.println();

	updateString(data);
}

bool disconnect = false;

void setup()
{
    Particle.variable("bool", variableBool);
    Particle.variable("int", variableInt);
    Particle.variable("double", variableDouble);
    Particle.variable("string", variableString);

    Particle.function("updateString", updateString);
    Particle.function("update", update);
    Particle.function("setString", setString);
    Particle.function("checkString", checkString);

    Particle.function("cmd", cmd);

    Particle.subscribe("cloudtest", subscription);

}

void do_cmd(String arg)
{
	if (arg.equals("bounce_pdp"))
	{
#if Wiring_Cellular
		 Cellular.command(40*1000, "AT+UPSDA=0,4\r\n");
		 Cellular.command(40*1000, "AT+UPSDA=0,3\r\n");
		 Serial.println("bounced pdp context");
		 disconnect = true;
 #endif
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

