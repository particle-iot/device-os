#include "application.h"

bool variableBool = false;
char variableString[1024];
double variableDouble;
int variableInt;


int update(String data)
{
    variableBool = !variableBool;
    variableDouble += 0.25;
    variableInt += 1;
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

}
