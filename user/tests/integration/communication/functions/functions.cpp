#include "application.h"
#include "test.h"

namespace {

String fn1Arg;
String fn2Arg;
bool appThread = true;

int fn1(const String& arg) {
    if (!application_thread_current(nullptr)) {
        appThread = false;
    }
    fn1Arg = arg;
    return 1234;
}

int fn2(const String& arg) {
    if (!application_thread_current(nullptr)) {
        appThread = false;
    }
    fn2Arg = arg;
    return -1234;
}

} // namespace

test(register_functions) {
    Particle.function("fn_1", fn1);
    Particle.function("fn_2", fn2);
    // Connect to the cloud
    Particle.connect();
    waitUntil(Particle.connected);
    delay(3000);
}

test(check_function_argument_value) {
    assertTrue(fn1Arg == "argument string");
    // Original 1024-character string used in the spec file
    String s = "1eD5TLw9CLc7CNolLSIbhpTYuc8V3cDGXeEVGSjAEPCQqzzhBNO97UPuF5OInUHP4wbySaLRCAw682m8AkK3VqEPXTVP2aMrsUHJYZB9B1Euz2ZIA6xFU0TlRwq1MAXpn7njmyzlJLNJioYcNQ4B6Id1GhDFX0DZOueDRf9KCLyMa5JXXlOlcYsO7yrJP7RuHgpDztbJz8UHEnW5uxsRjt8qlt4789b4JieWHqRSaf8LwczHiEBGeZXnTnsP09qr5WGXnvpdo0zoDxUH4rg07TbLph6luGj4pDR0Tl1lEKRurP0suvq7lZhbLniI6fmbTMV58lAc6oWs3N6inIdOyIMrwOhfIPRa7714pReW1yZCy1lGh7E3HMChMx27tlwxCoa5qcyXuq3GxKwMJvrAgTd0ZXF05JRK5CWwapr4rrs1hNrkniM6q7x7k00tS5E9umjiaQWPwgfu2HBdrtId4Y7wESeS61Z8xM15oBFZ6LxD1EiBqRvnY314q5yy6LHEgLBGmq8MDzKPLemoFNy5ysWATf5XZIx6BfooajFW4WN1bNFF66PznhN2Cy5w9HDzO1lGhshYx3gbzoynW4yXSN1MBqtSfORRgnDdwpe8ADozXXHbvwS0ZMfPN27DreZD24cw9K7AarXCToKom4fbZAQzz8NgWiNGerJZ5vkc6Qsk1qrC9tQ6t1r79wmm4z9bIgy1IiEnlX7zPUBqegJRpQETIIY7oaWcwtf1FBPtiGIbrIcBbneV0rjsCdx7h343uoiKubciQz3ewqNEFJsIvg7caVsamIVCgBVClxax6dOut6fI0n29JdOrpVtYtxMtaY7fGPxwdjTRpACVRg3FAhN8JDzetsoOlEuuUQCSdTFMnqvVGhauQpbQPupn8N2cVkeQYbc1nT1cnY0JZq1PvLZXWyXmoONFK1rxwsBj2W54NoQ0L9pqe3DSV3PZNSnprJInoIwsp9br37QzihSA7doUDAbnAkVxqoJF3Ewmyb46tuzpf8D8JB1fS2idltIG";
    assertTrue(fn2Arg == s.substring(0, protocol::MAX_FUNCTION_ARG_LENGTH));
}

test(check_current_thread) {
    // Verify that all function calls have been performed in the application thread
    assertTrue(appThread);
}
