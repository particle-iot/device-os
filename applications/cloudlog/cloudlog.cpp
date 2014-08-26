
#include "application.h"


char buf[601];
int count = 0;

int next(String arg)
{
    if (count==3)
        return -1;
    memset(buf, 'A'+count, 600);
    buf[600] = 0;
    count++;
    return 600;
}

void setup() 
{
    Spark.variable("var", &buf, STRING);
    Spark.function("next", next);
}


void loop()
{    
}