#include "spark_wiring_vector.h"

#include "tools/catch.h"
#include "tools/alloc.h"

#include <type_traits>

namespace {

template<typename VectorT>
class Checker {
public:
    using T = typename VectorT::ValueType;

    explicit Checker(const VectorT &vector) :
            v_(vector) {
    }

    template<typename... ArgsT>
    const Checker<VectorT>& values(ArgsT... args) const { // Implies size check
        checkValues(0, args...);
        return *this;
    }

    const Checker<VectorT>& size(int size) const {
        checkSize(size);
        return *this;
    }

    const Checker<VectorT>& capacity(int capacity) const {
        checkCapacity(capacity);
        return *this;
    }

private:
    const VectorT &v_;

    template<typename... ArgsT>
    void checkValues(int index, const T& value, ArgsT... args) const {
        REQUIRE(v_.size() > index);
        REQUIRE(v_.at(index) == value);
        checkValues(index + 1, args...);
    }

    void checkValues(int index, const T& value) const {
        REQUIRE(v_.size() > index);
        REQUIRE(v_.at(index) == value);
        checkSize(index + 1);
    }

    void checkValues(int index) const {
        checkSize(index);
    }

    void checkSize(int size) const {
        REQUIRE(v_.size() == size);
        REQUIRE(v_.size() <= v_.capacity());
        if (size > 0) {
            REQUIRE(v_.isEmpty() == false);
        } else {
            REQUIRE(v_.isEmpty() == true);
        }
    }

    void checkCapacity(int capacity) const {
        REQUIRE(v_.capacity() == capacity);
        REQUIRE(v_.capacity() >= v_.size());
        if (capacity > 0) {
            REQUIRE(v_.data() != nullptr);
        } else {
            REQUIRE(v_.data() == nullptr);
        }
    }
};

// Non-trivially copyable wrapper for 'int' type
class NonTrivialInt {
public:
    NonTrivialInt() :
            v_(0) {
        ++s_count;
    }

    NonTrivialInt(int value) :
            v_(value) {
        ++s_count;
    }

    NonTrivialInt(const NonTrivialInt &value) :
            v_(value.v_) {
        ++s_count;
    }

    NonTrivialInt(NonTrivialInt&& value) : NonTrivialInt() {
        swap(*this, value);
    }

    ~NonTrivialInt() {
        --s_count;
    }

    NonTrivialInt& operator=(NonTrivialInt value) {
        swap(*this, value);
        return *this;
    }

    NonTrivialInt& operator=(int value) {
        v_ = value;
        return *this;
    }

    bool operator==(const NonTrivialInt &value) const {
        return v_ == value.v_;
    }

    bool operator==(int value) const {
        return v_ == value;
    }

    bool operator!=(const NonTrivialInt &value) const {
        return !(*this == value);
    }

    bool operator!=(int value) const {
        return !(*this == value);
    }

    operator int() const {
        return v_;
    }

    static size_t instanceCount() {
        return s_count;
    }

private:
    int v_;

    static size_t s_count;

    friend void swap(NonTrivialInt& value1, NonTrivialInt& value2) {
        using std::swap;
        swap(value1.v_, value2.v_);
    }
};

size_t NonTrivialInt::s_count = 0;

static_assert(!PARTICLE_VECTOR_TRIVIALLY_COPYABLE_TRAIT<NonTrivialInt>::value, "NonTrivialInt is too trivial!");

template<typename T, typename AllocatorT>
inline Checker<spark::Vector<T, AllocatorT>> check(const spark::Vector<T, AllocatorT> &vector) {
    return Checker<spark::Vector<T, AllocatorT>>(vector);
}

template<typename VectorT>
void testVector() {
    using Vector = VectorT;
    using T = typename Vector::ValueType;

    SECTION("Vector()") {
        SECTION("Vector()") {
            const Vector a;
            check(a).size(0).capacity(0);
        }
        SECTION("Vector(int n)") {
            const Vector a(3); // n = 3
            check(a).values(0, 0, 0).capacity(3);
            const Vector b(0); // n = 0
            check(b).size(0).capacity(0);
        }
        SECTION("Vector(int n, const T& value)") {
            const Vector a(3, 1); // n = 3
            check(a).values(1, 1, 1).capacity(3);
            const Vector b(0, T(1)); // n = 0
            check(b).size(0).capacity(0);
        }
        SECTION("Vector(const T* values, int n)") {
            const T x[] = { 1, 2, 3 };
            const Vector a(x, 3); // n = 3
            check(a).values(1, 2, 3).capacity(3);
            const Vector b(x, 0); // n = 0
            check(b).size(0).capacity(0);
        }
        SECTION("Vector(std::initializer_list<T> values)") {
            const Vector a({ 1, 2, 3 });
            check(a).values(1, 2, 3).capacity(3);
            const Vector b({}); // copy from empty initializer list
            check(b).size(0).capacity(0);
        }
        SECTION("Vector(const Vector<T>& vector)") {
            const Vector a({ 1, 2, 3 });
            const Vector b(a);
            check(b).values(1, 2, 3).capacity(3);
            check(a).values(1, 2, 3).capacity(3); // source data is not affected
            const Vector c;
            const Vector d(c); // copy from empty vector
            check(d).size(0).capacity(0);
            check(c).size(0).capacity(0);
        }
        SECTION("Vector(Vector<T>&& vector)") {
            Vector a({ 1, 2, 3 });
            const Vector b(std::move(a));
            check(b).values(1, 2, 3).capacity(3);
            check(a).size(0).capacity(0); // source data has been moved
            Vector c;
            const Vector d(std::move(c)); // move from empty vector
            check(d).size(0).capacity(0);
            check(c).size(0).capacity(0);
        }
    }

    SECTION("append()") {
        SECTION("append(T value)") {
            Vector a({ 1, 2, 3 });
            REQUIRE(a.append(4));
            check(a).values(1, 2, 3, 4).capacity(4);
            Vector b;
            REQUIRE(b.append(1)); // append to empty vector
            check(b).values(1).capacity(1);
        }
        SECTION("append(int n, const T& value)") {
            Vector a({ 1, 2, 3 });
            REQUIRE(a.append(0, T(4))); // n = 0
            check(a).values(1, 2, 3).capacity(3);
            REQUIRE(a.append(1, 4)); // n = 1
            check(a).values(1, 2, 3, 4).capacity(4);
            REQUIRE(a.append(2, 5)); // n = 2
            check(a).values(1, 2, 3, 4, 5, 5).capacity(6);
            Vector b;
            REQUIRE(b.append(3, 1)); // append to empty vector
            check(b).values(1, 1, 1).capacity(3);
        }
        SECTION("append(const T* values, int n)") {
            const T x[] = { 4 };
            const T y[] = { 5, 6 };
            Vector a({ 1, 2, 3 });
            REQUIRE(a.append(x, 0)); // n = 0
            check(a).values(1, 2, 3).capacity(3);
            REQUIRE(a.append(x, 1)); // n = 1
            check(a).values(1, 2, 3, 4).capacity(4);
            REQUIRE(a.append(y, 2)); // n = 2
            check(a).values(1, 2, 3, 4, 5, 6).capacity(6);
            Vector b;
            REQUIRE(b.append(y, 2)); // append to empty vector
            check(b).values(5, 6).capacity(2);
        }
        SECTION("append(const Vector<T>& vector)") {
            const Vector x({ 1, 2, 3 });
            const Vector y({ 4, 5, 6 });
            const Vector z;
            Vector a;
            REQUIRE(a.append(x)); // append to empty vector
            check(a).values(1, 2, 3).capacity(3);
            REQUIRE(a.append(y));
            check(a).values(1, 2, 3, 4, 5, 6).capacity(6);
            REQUIRE(a.append(z)); // append from empty vector
            check(a).values(1, 2, 3, 4, 5, 6).capacity(6);
        }
    }

    SECTION("prepend()") {
        SECTION("prepend(T value)") {
            Vector a({ 2, 3, 4 });
            REQUIRE(a.prepend(1));
            check(a).values(1, 2, 3, 4).capacity(4);
            Vector b;
            REQUIRE(b.prepend(1)); // prepend to empty vector
            check(b).values(1).capacity(1);
        }
        SECTION("prepend(int n, const T& value)") {
            Vector a({ 3, 4, 5 });
            REQUIRE(a.prepend(0, T(2))); // n = 0
            check(a).values(3, 4, 5).capacity(3);
            REQUIRE(a.prepend(1, 2)); // n = 1
            check(a).values(2, 3, 4, 5).capacity(4);
            REQUIRE(a.prepend(2, 1)); // n = 2
            check(a).values(1, 1, 2, 3, 4, 5).capacity(6);
            Vector b;
            REQUIRE(b.prepend(3, 1)); // prepend to empty vector
            check(b).values(1, 1, 1).capacity(3);
        }
        SECTION("prepend(const T* values, int n)") {
            const T x[] = { 1, 2 };
            const T y[] = { 3 };
            Vector a({ 4, 5, 6 });
            REQUIRE(a.prepend(y, 0)); // n = 0
            check(a).values(4, 5, 6).capacity(3);
            REQUIRE(a.prepend(y, 1)); // n = 1
            check(a).values(3, 4, 5, 6).capacity(4);
            REQUIRE(a.prepend(x, 2)); // n = 2
            check(a).values(1, 2, 3, 4, 5, 6).capacity(6);
            Vector b;
            REQUIRE(b.prepend(x, 2)); // prepend to empty vector
            check(b).values(1, 2).capacity(2);
        }
        SECTION("prepend(const Vector<T>& vector)") {
            const Vector x({ 1, 2, 3 });
            const Vector y({ 4, 5, 6 });
            const Vector z;
            Vector a;
            REQUIRE(a.prepend(y)); // prepend to empty vector
            check(a).values(4, 5, 6).capacity(3);
            REQUIRE(a.prepend(x));
            check(a).values(1, 2, 3, 4, 5, 6).capacity(6);
            REQUIRE(a.prepend(z)); // prepend from empty vector
            check(a).values(1, 2, 3, 4, 5, 6).capacity(6);
        }
    }

    SECTION("insert()") {
        SECTION("insert(int i, T value)") {
            Vector a({ 2, 4, 5 });
            REQUIRE(a.insert(0, 1)); // i = 0
            check(a).values(1, 2, 4, 5).capacity(4);
            REQUIRE(a.insert(4, 6)); // i = size()
            check(a).values(1, 2, 4, 5, 6).capacity(5);
            REQUIRE(a.insert(2, 3)); // i = size() / 2
            check(a).values(1, 2, 3, 4, 5, 6).capacity(6);
            Vector b;
            REQUIRE(b.insert(0, 1)); // insert to empty vector
            check(b).values(1).capacity(1);
        }
        SECTION("insert(int i, int n, const T& value)") {
            SECTION("i = 0") {
                Vector a({ 3, 4, 5 });
                REQUIRE(a.insert(0, 0, T(2))); // n = 0
                check(a).values(3, 4, 5).capacity(3);
                REQUIRE(a.insert(0, 1, 2)); // n = 1
                check(a).values(2, 3, 4, 5).capacity(4);
                REQUIRE(a.insert(0, 2, 1)); // n = 2
                check(a).values(1, 1, 2, 3, 4, 5).capacity(6);
            }
            SECTION("i = size()") {
                Vector a({ 1, 2, 3 });
                REQUIRE(a.insert(3, 0, T(4))); // n = 0
                check(a).values(1, 2, 3).capacity(3);
                REQUIRE(a.insert(3, 1, 4)); // n = 1
                check(a).values(1, 2, 3, 4).capacity(4);
                REQUIRE(a.insert(4, 2, 5)); // n = 2
                check(a).values(1, 2, 3, 4, 5, 5).capacity(6);
            }
            SECTION("i = size() / 2") {
                Vector a({ 1, 4, 5 });
                REQUIRE(a.insert(1, 0, T(2))); // n = 0
                check(a).values(1, 4, 5).capacity(3);
                REQUIRE(a.insert(1, 1, 2)); // n = 1
                check(a).values(1, 2, 4, 5).capacity(4);
                REQUIRE(a.insert(2, 2, 3)); // n = 2
                check(a).values(1, 2, 3, 3, 4, 5).capacity(6);
            }
            SECTION("size() == 0") {
                Vector a;
                REQUIRE(a.insert(0, 3, 1)); // insert to empty vector
                check(a).values(1, 1, 1).capacity(3);
            }
        }
        SECTION("insert(int i, const T* value, int n)") {
            SECTION("i = 0") {
                const T x[] = { 1, 2 };
                const T y[] = { 3 };
                Vector a({ 4, 5, 6 });
                REQUIRE(a.insert(0, y, 0)); // n = 0
                check(a).values(4, 5, 6).capacity(3);
                REQUIRE(a.insert(0, y, 1)); // n = 1
                check(a).values(3, 4, 5, 6).capacity(4);
                REQUIRE(a.insert(0, x, 2)); // n = 2
                check(a).values(1, 2, 3, 4, 5, 6).capacity(6);
            }
            SECTION("i = size()") {
                const T x[] = { 4 };
                const T y[] = { 5, 6 };
                Vector a({ 1, 2, 3 });
                REQUIRE(a.insert(3, x, 0)); // n = 0
                check(a).values(1, 2, 3).capacity(3);
                REQUIRE(a.insert(3, x, 1)); // n = 1
                check(a).values(1, 2, 3, 4).capacity(4);
                REQUIRE(a.insert(4, y, 2)); // n = 2
                check(a).values(1, 2, 3, 4, 5, 6).capacity(6);
            }
            SECTION("i = size() / 2") {
                const T x[] = { 2 };
                const T y[] = { 3, 4 };
                Vector a({ 1, 5, 6 });
                REQUIRE(a.insert(1, x, 0)); // n = 0
                check(a).values(1, 5, 6).capacity(3);
                REQUIRE(a.insert(1, x, 1)); // n = 1
                check(a).values(1, 2, 5, 6).capacity(4);
                REQUIRE(a.insert(2, y, 2)); // n = 2
                check(a).values(1, 2, 3, 4, 5, 6).capacity(6);
            }
            SECTION("size() == 0") {
                const T x[] = { 1, 2, 3 };
                Vector a;
                REQUIRE(a.insert(0, x, 3)); // insert to empty vector
                check(a).values(1, 2, 3).capacity(3);
            }
        }
        SECTION("insert(int i, const Vector<T>& vector)") {
            SECTION("i = 0") {
                const Vector x({ 1, 2, 3 });
                const Vector y;
                Vector a({ 4, 5, 6 });
                REQUIRE(a.insert(0, x));
                check(a).values(1, 2, 3, 4, 5, 6).capacity(6);
                REQUIRE(a.insert(0, y)); // insert from empty vector
                check(a).values(1, 2, 3, 4, 5, 6).capacity(6);
            }
            SECTION("i = size()") {
                const Vector x({ 4, 5, 6 });
                const Vector y;
                Vector a({ 1, 2, 3 });
                REQUIRE(a.insert(3, x));
                check(a).values(1, 2, 3, 4, 5, 6).capacity(6);
                REQUIRE(a.insert(6, y)); // insert from empty vector
                check(a).values(1, 2, 3, 4, 5, 6).capacity(6);
            }
            SECTION("i = size() / 2") {
                const Vector x({ 2, 3, 4 });
                const Vector y;
                Vector a({ 1, 5, 6 });
                REQUIRE(a.insert(1, x));
                check(a).values(1, 2, 3, 4, 5, 6).capacity(6);
                REQUIRE(a.insert(3, y)); // insert from empty vector
                check(a).values(1, 2, 3, 4, 5, 6).capacity(6);
            }
            SECTION("size() == 0") {
                const Vector x({ 1, 2, 3 });
                Vector a;
                REQUIRE(a.insert(0, x)); // insert to empty vector
                check(a).values(1, 2, 3).capacity(3);
            }
        }
    }

    SECTION("removeAt(int i, int n)") {
        SECTION("i == 0 && n <= size()") {
            Vector a({ 1, 2, 3, 4 });
            a.removeAt(0, 0); // n = 0
            check(a).values(1, 2, 3, 4).capacity(4);
            a.removeAt(0); // n = 1 (default value)
            check(a).values(2, 3, 4).capacity(4);
            a.removeAt(0, 2); // n = 2
            check(a).values(4).capacity(4);
            a.removeAt(0, 1); // remaining element
            check(a).size(0).capacity(4);
        }
        SECTION("i > 0 && i + n == size()") {
            Vector a({ 1, 2, 3, 4 });
            a.removeAt(4, 0); // n = 0
            check(a).values(1, 2, 3, 4).capacity(4);
            a.removeAt(3); // n = 1
            check(a).values(1, 2, 3).capacity(4);
            a.removeAt(1, 2); // n = 2
            check(a).values(1).capacity(4);
        }
        SECTION("i > 0 && i + n < size()") {
            Vector a({ 1, 2, 3, 4, 5 });
            a.removeAt(2, 0); // n = 0
            check(a).values(1, 2, 3, 4, 5).capacity(5);
            a.removeAt(2); // n = 1
            check(a).values(1, 2, 4, 5).capacity(5);
            a.removeAt(1, 2); // n = 2
            check(a).values(1, 5).capacity(5);
        }
        SECTION("n < 0 || n > size() - i") {
            Vector a({ 1, 2, 3, 4, 5 });
            a.removeAt(3, -1); // n = -1
            check(a).values(1, 2, 3).capacity(5);
            a.removeAt(1, 10); // n = 10
            check(a).values(1).capacity(5);
        }
    }

    SECTION("removeOne(const T& value)") {
        Vector a({ 1, 2, 3, 1 });
        REQUIRE(!a.removeOne(4)); // not found
        REQUIRE(a.removeOne(1));
        check(a).values(2, 3, 1).capacity(4);
        REQUIRE(a.removeOne(3));
        check(a).values(2, 1).capacity(4);
        REQUIRE(a.removeOne(1));
        check(a).values(2).capacity(4);
        REQUIRE(a.removeOne(2));
        check(a).size(0).capacity(4);
        REQUIRE(!a.removeOne(1)); // remove from empty array
    }

    SECTION("removeAll(const T& value)") {
        Vector a({ 1, 2, 3, 2, 1 });
        REQUIRE(a.removeAll(4) == 0); // not found
        REQUIRE(a.removeAll(1) == 2);
        check(a).values(2, 3, 2).capacity(5);
        REQUIRE(a.removeAll(3) == 1);
        check(a).values(2, 2).capacity(5);
        REQUIRE(a.removeAll(2) == 2);
        check(a).size(0).capacity(5);
        REQUIRE(a.removeAll(1) == 0); // remove from empty array
    }

    SECTION("takeFirst()") {
        Vector a({ 1, 2, 3 });
        REQUIRE(a.takeFirst() == 1);
        check(a).values(2, 3).capacity(3);
        REQUIRE(a.takeFirst() == 2);
        check(a).values(3).capacity(3);
        REQUIRE(a.takeFirst() == 3);
        check(a).size(0).capacity(3);
    }

    SECTION("takeLast()") {
        Vector a({ 1, 2, 3 });
        REQUIRE(a.takeLast() == 3);
        check(a).values(1, 2).capacity(3);
        REQUIRE(a.takeLast() == 2);
        check(a).values(1).capacity(3);
        REQUIRE(a.takeLast() == 1);
        check(a).size(0).capacity(3);
    }

    SECTION("takeAt(int i)") {
        Vector a({ 1, 2, 3 });
        REQUIRE(a.takeAt(1) == 2);
        check(a).values(1, 3).capacity(3);
        REQUIRE(a.takeAt(1) == 3);
        check(a).values(1).capacity(3);
        REQUIRE(a.takeAt(0) == 1);
        check(a).size(0).capacity(3);
    }

    SECTION("first()") {
        const Vector a({ 1, 2, 3});
        REQUIRE(a.first() == 1);
        Vector b({ 4, 5, 6 });
        REQUIRE(b.first() == 4);
        b.first() = 5;
        check(b).values(5, 5, 6);
    }

    SECTION("last()") {
        const Vector a({ 1, 2, 3});
        REQUIRE(a.last() == 3);
        Vector b({ 4, 5, 6 });
        REQUIRE(b.last() == 6);
        b.last() = 5;
        check(b).values(4, 5, 5);
    }

    SECTION("at(int i)") {
        const Vector a({ 1, 2, 3});
        REQUIRE(a.at(1) == 2);
        Vector b({ 4, 5, 6 });
        REQUIRE(b.at(1) == 5);
        b.at(1) = 4;
        check(b).values(4, 4, 6);
    }

    SECTION("copy(int i, int n)") {
        SECTION("i == 0 && n <= size()") {
            const Vector a({ 1, 2 });
            check(a.copy(0, 0)).size(0).capacity(0); // n = 0
            check(a.copy(0, 1)).values(1).capacity(1); // n = 1
            check(a.copy(0, 2)).values(1, 2).capacity(2); // n = 2
        }
        SECTION("i > 0 && i + n == size()") {
            const Vector a({ 1, 2, 3 });
            check(a.copy(3, 0)).size(0).capacity(0); // n = 0
            check(a.copy(2, 1)).values(3).capacity(1); // n = 1
            check(a.copy(1, 2)).values(2, 3).capacity(2); // n = 2
        }
        SECTION("i > 0 && i + n < size()") {
            const Vector a({ 1, 2, 3, 4 });
            check(a.copy(2, 0)).size(0).capacity(0); // n = 0
            check(a.copy(2, 1)).values(3).capacity(1); // n = 1
            check(a.copy(1, 2)).values(2, 3).capacity(2); // n = 2
        }
        SECTION("n < 0 || n > size() - i") {
            const Vector a({ 1, 2, 3, 4, 5 });
            check(a.copy(3, -1)).values(4, 5).capacity(2); // n = -1
            check(a.copy(1, 10)).values(2, 3, 4, 5).capacity(4); // n = 10
        }
    }

    SECTION("indexOf(const T& value, int i)") {
        const Vector a({ 1, 2, 3, 2, 1 });
        REQUIRE(a.indexOf(1) == 0); // i = 0 (default value)
        REQUIRE(a.indexOf(1, 2) == 4); // i = 2
        REQUIRE(a.indexOf(3) == 2);
        REQUIRE(a.indexOf(4) == -1); // not found
        const Vector b;
        REQUIRE(b.indexOf(1) == -1); // search in empty array
    }

    SECTION("lastIndexOf(const T& value, int i)") {
        const Vector a({ 1, 2, 3, 2, 1 });
        REQUIRE(a.lastIndexOf(1) == 4); // i = size() - 1 (default value)
        REQUIRE(a.lastIndexOf(1, 2) == 0); // i = 2
        REQUIRE(a.lastIndexOf(3) == 2);
        REQUIRE(a.lastIndexOf(4) == -1); // not found
        const Vector b;
        REQUIRE(b.lastIndexOf(1) == -1); // search in empty array
    }

    SECTION("contains(const T& value)") {
        const Vector a({ 1, 2, 3, 2, 1 });
        REQUIRE(a.contains(1));
        REQUIRE(a.contains(3));
        REQUIRE(!a.contains(4)); // not found
        const Vector b;
        REQUIRE(!b.contains(1)); // search in empty array
    }

    SECTION("fill(const T& value)") {
        Vector a({ 1, 2, 3 });
        check(a.fill(0)).values(0, 0, 0).capacity(3); // returns *this
        check(a).values(0, 0, 0).capacity(3);
        Vector b;
        b.fill(0); // fill empty array
        check(b).size(0).capacity(0);
    }

    SECTION("resize(int n") {
        Vector a({ 1, 2, 3 });
        check(a).size(3).capacity(3);
        REQUIRE(a.resize(3)); // resize to the same size
        check(a).size(3).capacity(3);
        REQUIRE(a.resize(5));
        check(a).values(1, 2, 3, 0, 0).capacity(5); // default-constructed elements
        REQUIRE(a.resize(2));
        check(a).values(1, 2).capacity(5);
        REQUIRE(a.resize(4));
        check(a).values(1, 2, 0, 0).capacity(5);
        REQUIRE(a.resize(0));
        check(a).size(0).capacity(5);
        REQUIRE(a.resize(3)); // resize empty array
        check(a).values(0, 0, 0).capacity(5);
    }

    SECTION("reserve(int n") {
        Vector a({ 1, 2, 3 });
        check(a).size(3).capacity(3);
        REQUIRE(a.reserve(3)); // reserve the same capacity
        check(a).size(3).capacity(3);
        REQUIRE(a.reserve(5));
        check(a).size(3).capacity(5);
        REQUIRE(a.reserve(2));
        check(a).size(3).capacity(5);
        REQUIRE(a.append(4));
        check(a).values(1, 2, 3, 4).capacity(5);
    }
}

} // namespace

TEST_CASE("Vector<int>") {
    test::DefaultAllocator::reset();

    using Vector = spark::Vector<int, test::DefaultAllocator>;
    testVector<Vector>();

    test::DefaultAllocator::check();
}

TEST_CASE("Vector<NonTrivialInt>") {
    test::DefaultAllocator::reset();

    using Vector = spark::Vector<NonTrivialInt, test::DefaultAllocator>;
    testVector<Vector>();

    test::DefaultAllocator::check();
    CHECK(NonTrivialInt::instanceCount() == 0);
}
