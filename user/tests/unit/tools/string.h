#ifndef TEST_TOOLS_STRING_H
#define TEST_TOOLS_STRING_H

#include <boost/regex.hpp>

#include <string>

namespace test {

class RegExpMatcher {
public:
    RegExpMatcher() = default;
    explicit RegExpMatcher(const std::string &expr);
    RegExpMatcher(const std::string &str, const std::string &expr);

    bool match(const std::string &str);

    std::string at(size_t i) const;

    size_t size() const;
    bool isEmpty() const;

    bool isValid() const;

private:
    boost::regex r_;
    boost::smatch m_;
    std::string s_;
};

std::string trim(const std::string &str);

std::string toLowerCase(const std::string &str);
std::string toUpperCase(const std::string &str);

std::string toHex(const std::string &str);
std::string fromHex(const std::string &str);

} // namespace test

// test::RegExpMatcher
inline bool test::RegExpMatcher::isEmpty() const {
    return m_.size() <= 1;
}

inline bool test::RegExpMatcher::isValid() const {
    return !r_.empty();
}

#endif // TEST_TOOLS_STRING_H
