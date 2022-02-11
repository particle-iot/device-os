#include "string.h"

#include "catch.h"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim_all.hpp>
#include <boost/algorithm/hex.hpp>

// test::RegExpMatcher
test::RegExpMatcher::RegExpMatcher(const std::string &expr) {
    try {
        r_.assign(expr);
    } catch (const boost::regex_error&) {
        CATCH_WARN("test::RegExpMatcher: Invalid expression: " << expr);
    }
}

test::RegExpMatcher::RegExpMatcher(const std::string &str, const std::string &expr) :
        RegExpMatcher(expr) {
    if (!match(str)) {
        r_ = boost::regex(); // Invalidate matcher
    }
}

bool test::RegExpMatcher::match(const std::string &str) {
    try {
        if (r_.empty()) {
            return false;
        }
        boost::smatch m;
        if (!boost::regex_match(str, m, r_)) {
            return false;
        }
        s_ = str;
        m_ = m;
        return true;
    } catch (boost::regex_error&) {
        return false;
    }
}

std::string test::RegExpMatcher::at(size_t i) const {
    i += 1; // Skip string matching whole expression
    if (i >= m_.size()) {
        return std::string();
    }
    return m_.str(i);
}

size_t test::RegExpMatcher::size() const {
    if (m_.empty()) {
        return 0;
    }
    return m_.size() - 1;
}

// test::
std::string test::trim(const std::string &str) {
    return boost::algorithm::trim_all_copy(str, std::locale::classic());
}

std::string test::toLowerCase(const std::string &str) {
    return boost::algorithm::to_lower_copy(str, std::locale::classic());
}

std::string test::toUpperCase(const std::string &str) {
    return boost::algorithm::to_upper_copy(str, std::locale::classic());
}

std::string test::toHex(const std::string &str) {
    std::string s = boost::algorithm::hex(str);
    boost::algorithm::to_lower(s, std::locale::classic());
    return s;
}

std::string test::fromHex(const std::string &str) {
    try {
        return boost::algorithm::unhex(str);
    } catch (const boost::algorithm::hex_decode_error&) {
        return std::string();
    }
}
