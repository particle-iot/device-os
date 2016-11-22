#include "random.h"

#include "catch.h"

namespace {

unsigned seed() {
    std::random_device rd;
    return rd();
}

} // namespace

int test::randomInt(int min, int max) {
    std::uniform_int_distribution<int> dist(min, max);
    return dist(randomGenerator());
}

double test::randomDouble(double min, double max) {
    std::uniform_real_distribution<double> dist(min, max);
    return dist(randomGenerator());
}

std::string test::randomString(size_t size) {
    static const std::string chars("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!@#$%^&*()`~-_=+[{]}\\|;:'\",<.>/? ");
    auto &gen = randomGenerator();
    std::uniform_int_distribution<unsigned> dist(0, chars.size() - 1);
    std::string s;
    s.reserve(size);
    for (size_t i = 0; i < size; ++i) {
        s += chars.at(dist(gen));
    }
    return s;
}

std::string test::randomBytes(size_t size) {
    auto &gen = randomGenerator();
    std::uniform_int_distribution<unsigned> dist(0, 255);
    std::string s;
    s.reserve(size);
    for (size_t i = 0; i < size; ++i) {
        s += (char)dist(gen);
    }
    return s;
}

std::default_random_engine& test::randomGenerator() {
    static /* thread_local */ std::default_random_engine gen(seed());
    return gen;
}
