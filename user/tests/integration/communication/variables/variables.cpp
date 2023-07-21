#include "application.h"
#include "test.h"

#include <type_traits>
#include <limits>

namespace {

template<typename T>
typename std::enable_if<!std::is_integral<T>::value && !std::is_floating_point<T>::value, T>::type varVal() {
    return T();
}

template<typename T>
typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value, T>::type varVal() {
    return std::numeric_limits<T>::min();
}

template<typename T>
typename std::enable_if<std::is_integral<T>::value && std::is_unsigned<T>::value, T>::type varVal() {
    return std::numeric_limits<T>::max();
}

template<typename T>
typename std::enable_if<std::is_floating_point<T>::value, T>::type varVal() {
    return std::numeric_limits<T>::max();
}

template<>
bool varVal<bool>() {
    return true;
}

template<>
const char* varVal<const char*>() {
    return "LlGqWwi6MhknxMW5Cbr62qBzYjrdXYxJiPKsnNOJ9dZli4WPOu22sr3Xc9r7yoni8Mkmb1KXrCyQI5J1V4JmWbhbrd5wJEOgUrsn";
}

template<>
String varVal<String>() {
    return "1n4m9ISRdZf7VlMP4RxrYiq0iVTwPKe1tD3maxwOCu3WnYJmngZmQM1wSChiP6twUyqozIs73lA7XGZBwgbGjGifa2cvAJ1Dodyw";
}

auto boolVar = varVal<bool>();
auto intVar = varVal<int>();
auto doubleVar = varVal<double>();
auto strVar = varVal<const char*>();
auto strObjVar = varVal<String>();

bool appThread = true;

template<typename T>
T varFn() {
    if (!application_thread_current(nullptr)) {
        appThread = false;
    }
    return varVal<T>();
}

template<typename T>
std::function<T()> makeVarStdFn() {
    return std::function<T()>(varFn<T>);
}

} // namespace

test(01_register_variables) {
    // Register variables
    Particle.variable("var_b", boolVar);
    Particle.variable("var_i", intVar);
    Particle.variable("var_d", doubleVar);
    Particle.variable("var_c", strVar);
    Particle.variable("var_s", strObjVar);
    // Register variable functions
    Particle.variable("fn_b", varFn<bool>);
    Particle.variable("fn_i8", varFn<int8_t>);
    Particle.variable("fn_u8", varFn<uint8_t>);
    Particle.variable("fn_i16", varFn<int16_t>);
    Particle.variable("fn_u16", varFn<uint16_t>);
    Particle.variable("fn_i32", varFn<int32_t>);
    Particle.variable("fn_u32", varFn<uint32_t>);
    Particle.variable("fn_f", varFn<float>);
    Particle.variable("fn_d", varFn<double>);
    Particle.variable("fn_c", varFn<const char*>);
    Particle.variable("fn_s", varFn<String>);
    Particle.variable("std_fn_i", makeVarStdFn<int>()); // std::function
    Particle.variable("std_fn_d", makeVarStdFn<double>()); // std::function
    Particle.variable("std_fn_c", makeVarStdFn<const char*>()); // std::function
    Particle.variable("std_fn_s", makeVarStdFn<String>()); // std::function
    // Connect to the cloud
    Particle.connect();
    waitUntil(Particle.connected);

    // Loop a bit before we check this (JS processes after this test case) to make sure the device app
    // thread has received this message. 10s is kind of long, but we are not validating how fast it can
    // process this data, just that it does.  We want this test to be reliable.
    for (auto start = millis(); millis() - start < 10000;) {
        Particle.process();   // pump application events
    }
}

test(02_check_variable_values) {
    // See variables.spec.js
}

test(03_publish_variable_limits) {
    char buf[128] = {};
    JSONBufferWriter json(buf, sizeof(buf));
    json.beginObject();
    json.name("max_num").value(USER_VAR_MAX_COUNT);
    json.name("max_name_len").value(USER_VAR_KEY_LENGTH);
    json.name("max_val_len").value(Particle.maxVariableValueSize());
    json.endObject();
    Particle.publish("limits", buf);
}

test(04_verify_max_variable_value_size) {
    // 1500-character string
    strObjVar = "XvFclXWVOG6n99rUYpsLzrp8VyPWdpKfm4z4SdX2GwxLwoJOSPpHL5jF6ajMaJhJdWUuDPSfmqoDmb5DQZRWZFM2f6tSsqmDzVPojUr5qZJQKEgb8WPndRRnD6y9AA5RPfkoqNZKfTgmDCWSGDHygLaFvYOUsM6ggZD8pBLnyrfs5c1fMrM6qZsRglUfaEit4hrDKfsdHoD2SUmdckgU6vqmYHpeVEwW6xitwwFRtyHSvCUb4XbZIWBHJypHEHS17wUpDbTPHcaowsod9Ogp1UjD2ybAUaNd1ul0yPvPigNAqdBsOQ8viVEnOyADAnf0TPQjaXEQ5LWgLJNIheO2qmniPFL9WSnQFPZSY7lwjANoK07ys62nRGoAgwS1sNL0LOvweWwklUxhVDw7foEWBDSXoLaaHieQ7sUvcxAH05S0LMd4m3QbFbkxwFnZPjqvdS98dtAIcvAZqGwbHtnGIInWT5LArXrsyAmiGouezRbgMwS6IFn6ObkGyvEmGqyIGmTdhGlDUSMVMzRXXKoXDn36yqKimGwLhiBKEc4oq7TpwfQ8P17DjO3rVC8hA9cf0UFNHSIhrK4bHtOKSoXEIDv4O4p86xG9oJ84yuUxz4psJHolfwFFlZ6m5csmSOk6urU3kpxh9FyuBnwGrICGIfTxMNfOU0EiV1ajMudqz9G2L2IBgxsqjKaOmeGjja4tgg9cW1UMFnEK9QaXs88kdUmXiJRnIHuZlCg1rOUvQgxmoUlPR7lZ9R6ZfWOivmX2gs7kxiSxK84JmVirVjqgE1gHASoSXjUj3YhJ5h0c6yR5QN6QrHc4zPN2jnI1Tukt8mS7WXbRmGPz31dZSUC9LYVqifY9bw77QYiqenXFbtX4vEeOKFxCvXbzZv3QKKCReobPu0eTM0iLNcrVXUocZXjOnfU7e42UrV8HBGrkB0ozu0mgmVcDlW0M5wp8gcx4ekXLlmvfYH0WO3YamV1ioraHwXJ0MmRSjaFuau7CqOZyUPfhspnM7Yo8yz8J58oVs7oxTzdkgINbr0zBclRyNY6Box9p1MMOtR5t5oNiRYs7g8WxIN4KCKY5CWnlUNUByCwNHhnEGRIIi5guZNt22FHsBPtoztLDwJ7YUY26GTJUypdXm3QOho3vw4IP68w651rcJU7SWX9aEw7pkTS7FqHYT0vsyt2H2Jzx5QQsBcbVei2RL9lgnNRB2UvxNSOyiifjeIECvapmMLiTdTYgq2ZVBDjoTJyZ5DPRdCsJlpKzlNvoomiXnIPyVfhMWhGk5IKieNvkYtTqQZEVhwysndg3MkQLHqlSpU061PPrEoPUtJSvX4c5JtBnISKDT3sFpIHnUayITBUjzKUpJABiPr8E2zBJP1WFJd5yWEBf1JsRBFmrnP7qq6b6zNraRw1NrBvxva04kxIcW8wiTuOkrvlGwChxy5vG4AtVVDga2TSDotdzu5W2mLW3QI9r05zNY0MWVPwpZVmbGZjcYBqE2As7Gl67";

    // Loop a bit before we check this (JS processes after this test case) to make sure the device app
    // thread has received this message. 10s is kind of long, but we are not validating how fast it can
    // process this data, just that it does.  We want this test to be reliable.
    for (auto start = millis(); millis() - start < 10000;) {
        Particle.process();   // pump application events
    }
}

test(05_empty_string_variable) {
    strObjVar = "";

    // Loop a bit before we check this (JS processes after this test case) to make sure the device app
    // thread has received this message. 10s is kind of long, but we are not validating how fast it can
    // process this data, just that it does.  We want this test to be reliable.
    for (auto start = millis(); millis() - start < 10000;) {
        Particle.process();   // pump application events
    }
}

test(06_check_current_thread) {
    // Verify that all variable requests have been processed in the application thread
    assertTrue(appThread);
}

test(07_register_many_variables) {
    const unsigned varCount = USER_VAR_MAX_COUNT;
    char name[USER_VAR_KEY_LENGTH + 1] = {};
    for (unsigned i = 1; i <= varCount; ++i) {
        const int n = snprintf(name, sizeof(name), "var_%03u_", i);
        assertMore(n, 0);
        memset(name + n, 'x', sizeof(name) - n - 1);
        Particle.variable(name, intVar);
    }
    Particle.connect();
    waitUntil(Particle.connected);

    // Loop a bit before we check this (JS processes after this test case) to make sure the device app
    // thread has received this message. 10s is kind of long, but we are not validating how fast it can
    // process this data, just that it does.  We want this test to be reliable.
    for (auto start = millis(); millis() - start < 10000;) {
        Particle.process();   // pump application events
    }
}
