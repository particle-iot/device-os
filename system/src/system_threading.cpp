

#include "system_threading.h"

ActiveObject SystemThread;

bool ActiveObject::isCurrentThread()
{
    return true;
}
