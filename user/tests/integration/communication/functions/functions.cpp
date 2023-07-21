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

test(01_register_functions) {
    Particle.function("fn_1", fn1);
    Particle.function("fn_2", fn2);
    // Connect to the cloud
    Particle.connect();
    waitUntil(Particle.connected);
    delay(3000);
}

test(02_publish_function_limits) {
    char buf[128] = {};
    JSONBufferWriter json(buf, sizeof(buf));
    json.beginObject();
    json.name("max_num").value(USER_FUNC_MAX_COUNT);
    json.name("max_name_len").value(USER_FUNC_KEY_LENGTH);
    json.name("max_arg_len").value(Particle.maxFunctionArgumentSize());
    json.endObject();
    Particle.publish("limits", buf);
}

test(03_call_function_and_check_return_value) {
    // See functions.spec.js
}

test(04_check_function_argument_value) {
    // Loop a bit before we check this to make sure the device app thread has received this message.
    // 60s is kind of long, but we are not validating how fast it can receive this data, just that it
    // does.  We want this test to be reliable.
    for (auto start = millis(); fn1Arg != "argument string" && millis() - start < 60000;) {
        Particle.process();   // pump application events
    }

    assertTrue(fn1Arg == "argument string");
    // Original 1500-character string used in the spec file
    String s = "WlbOWabfZl6J5H5vrB6KDnJhsI18avqx4RZHSJyDjQGOahaTyT52rHrE1cfUMLxwxZPFxjmDYeQuRUZKLEdwBynABXmQpRkJ09hdBbYZesdUiMqhCS5CcqXApZfidk5w8zb9LoWSPbI45vLod8cjSPsSykOhj64VUzH9FtamoU0a4Mq9pXr3Sz51kwNFpFpBG6a0dfWEzVBL5iYJn680MbUSK6RsMPUTSLrQLUbTlCt5loI4DKmppmg9yc4wsagDvd0AbU86dGeQTwLWHPL1i8k7EpEhjs3Ynf5mvcVwRv5Ik8HAQrtehSrAsWnvez1xqbMa7VxuLsgrBCeoTkiAyeIvXUUXy4DuwyTuM1VQ9kx9uksOLrQDPQEFfhchMe27SnWFwmPOD8vHQPR6P31YRgdnhL1dA91icnULzWD5qh4B9HyTshF0x4bRLRRQwgqJMkURXXqeYhn1vO1OxuSMl3yc0uAGiqBNrgD8DKVHsmnPMoQWN1igBYTvr6EGnuOFgmqhOVRPj9LJ4qfQFFJ7EkKPXSpddca2LoeWcxWNxBBMqISoeWV4GfynNQcLtPvPchj4mf9J1at4j0jiQsSF65Sw5Cy18RbYqazcl1pkchOl3YBk0jkJEF19KCniHD64uPeSeQT8HRtczK1bYCWjeQzZMyKMoRx8YdoUlt55gJfS8MSvDgvsdpMb39WWlCCnYJzQMOCFXmWlOEhWnABcML5ozmIWlB6LaDNcP3kC1Ou87VyslM5IbLSIBw8GNHBetNDnVsFhOntGldkVAapcIDakhunQHXcWB8QbECpy9ijZvSSDtMSGsYu7xFNznSR5QwXJQToITOsvpSffblarWmaXlCWnwfqTKxzAife4U7EmFdOrHQwq4RIHDgOUTtogzgkia9O3e4g1e7qEuweGEJm6YDZqXeGUdcxXvHmQxFmvKoGijur3jEGKVkv2SSanVQ5weOklb4se91mEcA6nUtQXZePx7T6aSf9UumuUeoAToIGTGweMViIxjI2wV5Eikqazfn7AEiXWbRLzKfC7XAZ6ZzqzWqFME4bPgxIbI2aHXe50sa3iA7NEkXZVWfLs8bCjNF66DyfZHahCMjevIpzclwUnlT2Q1Bg1LMyHXD4d6frplmlTvYAKAwVyGz10kZ38oxPtQoYGVF1wkNwXT8cBG1yT6vU6NUpkB22oHdXHzjL6fJ0jvPhIm2a4Sh5pO0Gg7NCktVRBj9jGqn3qo6uggAPTiqBPAoJM2yNJ3aQ5tJLdrN3EM3Brkdm27OaWvbbDQrFneBDjQy8aijwxjtYPz8jXxjGG9c3AW5hPTHxzDKuu64XvTjvbFLr8048on0V7RvvNg7PNmX4mRJr34PKYuEkoJ1KxRvttzziVVlQbvAiy72Inw6DqON0JBpDhg6RhHxXseqZkyrQtFJsOWB1A81cPu5eTzdD9izYpjo7kxpe9iUrb5AL36fhc5WJOBJIFwYEpsD7zh3GycAKc3yWb9BBv";
    assertTrue(fn2Arg == s.substring(0, Particle.maxFunctionArgumentSize()));
}

test(05_check_current_thread) {
    // Verify that all function calls have been performed in the application thread
    assertTrue(appThread);
}

test(06_register_many_functions) {
    const unsigned funcCount = USER_FUNC_MAX_COUNT;
    char name[USER_FUNC_KEY_LENGTH + 1] = {};
    for (unsigned i = 1; i <= funcCount; ++i) {
        const int n = snprintf(name, sizeof(name), "fn_%03u_", i);
        assertMore(n, 0);
        memset(name + n, 'x', sizeof(name) - n - 1);
        Particle.function(name, fn1);
    }
    Particle.connect();
    waitUntil(Particle.connected);

    // Give the system some time to send a blockwise Describe message
    for (auto start = millis(); millis() - start < 10000;) {
        Particle.process();   // pump application events
    }
}
