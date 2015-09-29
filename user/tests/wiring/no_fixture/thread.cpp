
#include "application.h"
#include "unit-test/unit-test.h"

#if PLATFORM_THREADING
test(Thread_creation)
{
    volatile bool threadRan = false;
    Thread testThread = Thread("test", [&]() {
        threadRan = true;
        for(;;) {}
    });

    for(int tries = 5; !threadRan && tries >= 0; tries--) {
        delay(1);
    }

    testThread.dispose();

    assertTrue((bool)threadRan);
}

#endif
