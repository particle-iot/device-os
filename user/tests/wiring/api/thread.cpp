#include "testapi.h"

#if PLATFORM_THREADING

test(api_thread) {

    void (*thread_fn)(void *) = NULL;
    void *param = (void *)0x123;
    int x = 0;

    API_COMPILE(Thread("test", thread_fn));
    API_COMPILE(Thread("test", thread_fn, param));
    API_COMPILE(Thread("test", thread_fn, param, OS_THREAD_PRIORITY_DEFAULT + 1));
    API_COMPILE(Thread("test", thread_fn, param, OS_THREAD_PRIORITY_DEFAULT + 1, 512));
    API_COMPILE(Thread("test", [&]() { x++; }));

    Thread t;
    API_COMPILE(t.dispose());
    API_COMPILE(t.join());
    API_COMPILE(t.isValid());
    API_COMPILE(t.isCurrent());
    API_COMPILE(t.is_valid()); // Deprecated
    API_COMPILE(t.is_current()); // Deprecated
    API_COMPILE(t = Thread("test", thread_fn));
}

test(api_single_threaded_section) {
    API_COMPILE(SingleThreadedSection sts);
}

test(api_atomic_section) {
    API_COMPILE(AtomicSection as);
}

test(api_application_watchdog)
{
	unsigned stack_size = 512;
	application_checkin();
	ApplicationWatchdog wd(30000, System.reset);
	ApplicationWatchdog wd2(30000, System.reset, stack_size);
}


#endif
