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

test(get_max_function_argument_size) {
    char str[16] = {};
    snprintf(str, sizeof(str), "%d", Particle.maxFunctionArgumentSize());
    Particle.publish("max_function_argument_size", str);
}

test(check_function_argument_value) {
    assertTrue(fn1Arg == "argument string");
    // Original 1500-character string used in the spec file
    String s = "WlbOWabfZl6J5H5vrB6KDnJhsI18avqx4RZHSJyDjQGOahaTyT52rHrE1cfUMLxwxZPFxjmDYeQuRUZKLEdwBynABXmQpRkJ09hdBbYZesdUiMqhCS5CcqXApZfidk5w8zb9LoWSPbI45vLod8cjSPsSykOhj64VUzH9FtamoU0a4Mq9pXr3Sz51kwNFpFpBG6a0dfWEzVBL5iYJn680MbUSK6RsMPUTSLrQLUbTlCt5loI4DKmppmg9yc4wsagDvd0AbU86dGeQTwLWHPL1i8k7EpEhjs3Ynf5mvcVwRv5Ik8HAQrtehSrAsWnvez1xqbMa7VxuLsgrBCeoTkiAyeIvXUUXy4DuwyTuM1VQ9kx9uksOLrQDPQEFfhchMe27SnWFwmPOD8vHQPR6P31YRgdnhL1dA91icnULzWD5qh4B9HyTshF0x4bRLRRQwgqJMkURXXqeYhn1vO1OxuSMl3yc0uAGiqBNrgD8DKVHsmnPMoQWN1igBYTvr6EGnuOFgmqhOVRPj9LJ4qfQFFJ7EkKPXSpddca2LoeWcxWNxBBMqISoeWV4GfynNQcLtPvPchj4mf9J1at4j0jiQsSF65Sw5Cy18RbYqazcl1pkchOl3YBk0jkJEF19KCniHD64uPeSeQT8HRtczK1bYCWjeQzZMyKMoRx8YdoUlt55gJfS8MSvDgvsdpMb39WWlCCnYJzQMOCFXmWlOEhWnABcML5ozmIWlB6LaDNcP3kC1Ou87VyslM5IbLSIBw8GNHBetNDnVsFhOntGldkVAapcIDakhunQHXcWB8QbECpy9ijZvSSDtMSGsYu7xFNznSR5QwXJQToITOsvpSffblarWmaXlCWnwfqTKxzAife4U7EmFdOrHQwq4RIHDgOUTtogzgkia9O3e4g1e7qEuweGEJm6YDZqXeGUdcxXvHmQxFmvKoGijur3jEGKVkv2SSanVQ5weOklb4se91mEcA6nUtQXZePx7T6aSf9UumuUeoAToIGTGweMViIxjI2wV5Eikqazfn7AEiXWbRLzKfC7XAZ6ZzqzWqFME4bPgxIbI2aHXe50sa3iA7NEkXZVWfLs8bCjNF66DyfZHahCMjevIpzclwUnlT2Q1Bg1LMyHXD4d6frplmlTvYAKAwVyGz10kZ38oxPtQoYGVF1wkNwXT8cBG1yT6vU6NUpkB22oHdXHzjL6fJ0jvPhIm2a4Sh5pO0Gg7NCktVRBj9jGqn3qo6uggAPTiqBPAoJM2yNJ3aQ5tJLdrN3EM3Brkdm27OaWvbbDQrFneBDjQy8aijwxjtYPz8jXxjGG9c3AW5hPTHxzDKuu64XvTjvbFLr8048on0V7RvvNg7PNmX4mRJr34PKYuEkoJ1KxRvttzziVVlQbvAiy72Inw6DqON0JBpDhg6RhHxXseqZkyrQtFJsOWB1A81cPu5eTzdD9izYpjo7kxpe9iUrb5AL36fhc5WJOBJIFwYEpsD7zh3GycAKc3yWb9BBv";
    assertTrue(fn2Arg == s.substring(0, Particle.maxFunctionArgumentSize()));
}

test(check_current_thread) {
    // Verify that all function calls have been performed in the application thread
    assertTrue(appThread);
}
