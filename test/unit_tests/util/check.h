#ifndef TEST_TOOLS_CHECK_H
#define TEST_TOOLS_CHECK_H

#include "catch.h"
#include "string.h"

#include <type_traits>

namespace test {

namespace detail {

class CheckableBase {
};

template<typename CheckerT, typename WrapperT>
class CheckerBase {
public:
    typedef WrapperT WrapperType;

    explicit CheckerBase(const WrapperT &wrapper) :
            w_(wrapper) {
    }

    const WrapperT& end() const {
        return w_;
    }

private:
    const WrapperT& w_;
};

template<typename CheckerT>
class CheckerBase<CheckerT, void> {
public:
    typedef CheckerT WrapperType;

    const CheckerT& end() const {
        return static_cast<const CheckerT&>(*this);
    }
};

template<typename T, typename CheckerT, typename WrapperT>
class BasicChecker: public CheckerBase<CheckerT, WrapperT> {
public:
    typedef BasicChecker<T, CheckerT, WrapperT> CheckerType;
    typedef CheckerBase<CheckerT, WrapperT> CheckerBaseType;
    typedef typename CheckerBaseType::WrapperType WrapperType;

    template<typename... ArgsT>
    explicit BasicChecker(T value, ArgsT&&... args) :
            CheckerBaseType(std::forward<ArgsT>(args)...),
            v_(std::move(value)) {
    }

    const WrapperType& equals(const T& value) const {
        CHECK(v_ == value);
        return this->end();
    }

    const WrapperType& lessThan(const T& value) const {
        CHECK(v_ < value);
        return this->end();
    }

    const WrapperType& lessThanOrEquals(const T& value) const {
        CHECK(v_ <= value);
        return this->end();
    }

    const WrapperType& greaterThan(const T& value) const {
        CHECK(v_ > value);
        return this->end();
    }

    const WrapperType& greaterThanOrEquals(const T& value) const {
        CHECK(v_ >= value);
        return this->end();
    }

    const T& value() const {
        return v_;
    }

    operator T() const {
        return v_;
    }

private:
    T v_;
};

} // namespace test::detail

template<typename T, typename WrapperT = void>
class Checker: public detail::BasicChecker<T, Checker<T, WrapperT>, WrapperT> {
public:
    typedef Checker<T, WrapperT> CheckerType;
    typedef detail::BasicChecker<T, CheckerType, WrapperT> CheckerBaseType;

    using CheckerBaseType::CheckerBaseType;
};

template<typename WrapperT>
class RegExpChecker;

template<typename WrapperT>
class Checker<std::string, WrapperT>: public detail::BasicChecker<std::string, Checker<std::string, WrapperT>, WrapperT> {
public:
    typedef Checker<std::string, WrapperT> CheckerType;
    typedef detail::BasicChecker<std::string, CheckerType, WrapperT> CheckerBaseType;
    typedef typename CheckerBaseType::WrapperType WrapperType;

    using CheckerBaseType::CheckerBaseType;

    const WrapperType& startsWith(const std::string& str) const {
        const std::string &s = this->value();
        if (s.size() < str.size() || s.substr(0, str.size()) != str) {
            FAIL('"' << s << "\" doesn't start with \"" << str << '"');
        }
        return this->end();
    }

    const WrapperType& endsWith(const std::string& str) const {
        const std::string &s = this->value();
        if (s.size() < str.size() || s.substr(s.size() - str.size(), str.size()) != str) {
            FAIL('"' << s << "\" doesn't end with \"" << str << '"');
        }
        return this->end();
    }

    const WrapperType& contains(const std::string& str) const {
        const std::string &s = this->value();
        if (s.find(str) == std::string::npos) {
            FAIL('"' << s << "\" doesn't contain \"" << str << '"');
        }
        return this->end();
    }

    const WrapperType& isEmpty() const {
        const std::string &s = this->value();
        CHECK(s == "");
        return this->end();
    }

    CheckerType lowerCase() const {
        return CheckerType(toLowerCase(this->value()));
    }

    CheckerType upperCase() const {
        return CheckerType(toUpperCase(this->value()));
    }

    CheckerType hex() const {
        return CheckerType(toHex(this->value()));
    }

    CheckerType unhex() const {
        return CheckerType(fromHex(this->value()));
    }

    CheckerType trim() const {
        return CheckerType(trim(this->value()));
    }

    Checker<size_t, CheckerType> length() const {
        const std::string &s = this->value();
        return Checker<size_t, CheckerType>(s.size(), *this);
    }

    RegExpChecker<CheckerType> matches(const std::string &expr) const;
};

template<typename WrapperT>
class Checker<const char*, WrapperT>: public Checker<std::string, WrapperT> {
public:
    typedef Checker<std::string, WrapperT> CheckerBaseType;

    using CheckerBaseType::CheckerBaseType;
};

template<typename WrapperT>
class RegExpChecker: public detail::CheckerBase<RegExpChecker<WrapperT>, WrapperT> {
public:
    typedef RegExpChecker<WrapperT> CheckerType;
    typedef detail::CheckerBase<CheckerType, WrapperT> CheckerBaseType;
    typedef typename CheckerBaseType::WrapperType WrapperType;

    template<typename... ArgsT>
    RegExpChecker(const std::string &str, const std::string &expr, ArgsT&&... args) :
            CheckerBaseType(std::forward<ArgsT>(args)...),
            m_(expr) {
        if (!m_.isValid()) {
            FAIL("Invalid regular expression: \"" << expr << '"');
        }
        if (!m_.match(str)) {
            FAIL('"' << str << "\" doesn't match \"" << expr << '"');
        }
    }

    Checker<std::string, CheckerType> at(size_t i) const {
        if (i >= m_.size()) {
            FAIL("Invalid subexpression index: " << i);
        }
        return Checker<std::string, CheckerType>(m_.at(i), *this);
    }

private:
    RegExpMatcher m_;
};

template<typename WrapperT>
inline RegExpChecker<Checker<std::string, WrapperT>> Checker<std::string, WrapperT>::matches(const std::string &expr) const {
    return RegExpChecker<CheckerType>(this->value(), expr, *this);
}

template<typename T, typename CheckableT>
class Checkable: public detail::CheckableBase {
public:
    Checker<T> check() const {
        return Checker<T>((T)static_cast<const CheckableT&>(*this));
    }
};

template<typename T, typename CheckableT>
inline Checker<T> check(const Checkable<T, CheckableT> &c) {
    return c.check();
}

template<typename T, typename std::enable_if<!std::is_base_of<detail::CheckableBase, T>::value>::type* = nullptr>
inline Checker<T> check(const T &value) {
    return Checker<T>(value);
}

} // namespace test

using test::check;

#endif // TEST_TOOLS_CHECK_H
