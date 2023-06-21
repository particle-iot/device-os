/*
 * Copyright (c) 2016 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SPARK_WIRING_VECTOR_H
#define SPARK_WIRING_VECTOR_H

#include <cstring>
#include <cstdlib>
#include <type_traits>
#include <iterator>
#include <utility>

// GCC didn't support std::is_trivially_copyable trait until 5.1.0
#if defined(__GNUC__) && (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 < 50100)
#define PARTICLE_VECTOR_TRIVIALLY_COPYABLE_TRAIT std::is_pod
#else
#define PARTICLE_VECTOR_TRIVIALLY_COPYABLE_TRAIT std::is_trivially_copyable
#endif

// Helper macros for SFINAE-based method selection
#define PARTICLE_VECTOR_ENABLE_IF_TRIVIALLY_COPYABLE(T) \
        typename EnableT = T, typename std::enable_if<PARTICLE_VECTOR_TRIVIALLY_COPYABLE_TRAIT<EnableT>::value, int>::type = 0

#define PARTICLE_VECTOR_ENABLE_IF_NOT_TRIVIALLY_COPYABLE(T) \
        typename EnableT = T, typename std::enable_if<!PARTICLE_VECTOR_TRIVIALLY_COPYABLE_TRAIT<EnableT>::value, int>::type = 0

namespace spark {

struct DefaultAllocator {
    static void* malloc(size_t size);
    static void* realloc(void* ptr, size_t size);
    static void free(void* ptr);
};

template<typename T, typename AllocatorT = DefaultAllocator>
class Vector {
public:
    typedef T ValueType;
    typedef AllocatorT AllocatorType;
    typedef T* Iterator;
    typedef const T* ConstIterator;

    Vector();
    explicit Vector(int n);
    Vector(int n, const T& value);
    Vector(const T* values, int n);
    Vector(std::initializer_list<T> values);
    Vector(const Vector<T, AllocatorT>& vector);
    Vector(Vector<T, AllocatorT>&& vector);
    ~Vector();

    bool append(T value);
    bool append(int n, const T& value);
    bool append(const T* values, int n);
    bool append(const Vector<T, AllocatorT>& vector);

    bool prepend(T value);
    bool prepend(int n, const T& value);
    bool prepend(const T* values, int n);
    bool prepend(const Vector<T, AllocatorT>& vector);

    bool insert(int i, T value);
    bool insert(int i, int n, const T& value);
    bool insert(int i, const T* values, int n);
    bool insert(int i, const Vector<T, AllocatorT>& vector);

    void removeAt(int i, int n = 1);
    bool removeOne(const T& value);
    int removeAll(const T& value);

    T takeFirst();
    T takeLast();
    T takeAt(int i);

    T& first();
    const T& first() const;
    T& last();
    const T& last() const;
    T& at(int i);
    const T& at(int i) const;

    Vector<T, AllocatorT> copy(int i, int n) const;

    int indexOf(const T& value, int i = 0) const;
    int lastIndexOf(const T& value) const;
    int lastIndexOf(const T& value, int i) const;

    bool contains(const T& value) const;

    Vector<T, AllocatorT>& fill(const T& value);

    bool resize(int n);
    int size() const;
    bool isEmpty() const;

    bool reserve(int n);
    int capacity() const;
    bool trimToSize();

    void clear();

    T* data();
    const T* data() const;

    Iterator begin();
    ConstIterator begin() const;
    Iterator end();
    ConstIterator end() const;

    Iterator insert(ConstIterator pos, T value);
    Iterator erase(ConstIterator pos);

    T& operator[](int i);
    const T& operator[](int i) const;

    bool operator==(const Vector<T, AllocatorT> &vector) const;
    bool operator!=(const Vector<T, AllocatorT> &vector) const;

    Vector<T, AllocatorT>& operator=(Vector<T, AllocatorT> vector);

private:
    T* data_;
    int size_, capacity_;

    template<PARTICLE_VECTOR_ENABLE_IF_TRIVIALLY_COPYABLE(T)>
    bool realloc(int n) {
        T* d = nullptr;
        if (n > 0) {
            d = (T*)AllocatorT::realloc(data_, n * sizeof(T));
            if (!d) {
                return false;
            }
        } else {
            AllocatorT::free(data_);
        }
        data_ = d;
        capacity_ = n;
        return true;
    }

    template<PARTICLE_VECTOR_ENABLE_IF_NOT_TRIVIALLY_COPYABLE(T)>
    bool realloc(int n) {
        T* d = nullptr;
        if (n > 0) {
            d = (T*)AllocatorT::malloc(n * sizeof(T));
            if (!d) {
                return false;
            }
            move(d, data_, data_ + size_);
        }
        AllocatorT::free(data_);
        data_ = d;
        capacity_ = n;
        return true;
    }

    // TODO: Use standard algorithms like std::uninitialized_copy() and std::uninitialized_move()
    // instead of custom implementations
    template<PARTICLE_VECTOR_ENABLE_IF_TRIVIALLY_COPYABLE(T)>
    static void copy(T* dest, const T* p, const T* end) {
        ::memcpy(dest, p, (end - p) * sizeof(T));
    }

    template<PARTICLE_VECTOR_ENABLE_IF_NOT_TRIVIALLY_COPYABLE(T)>
    static void copy(T* dest, const T* p, const T* end) {
        for (; p != end; ++p, ++dest) {
            new(dest) T(*p);
        }
    }

    template<typename IteratorT>
    static void copy(IteratorT dest, IteratorT it, IteratorT end) {
        for (; it != end; ++it, ++dest) {
            new(dest) T(*it);
        }
    }

    template<PARTICLE_VECTOR_ENABLE_IF_TRIVIALLY_COPYABLE(T)>
    static void move(T* dest, const T* p, const T* end) {
        ::memmove(dest, p, (end - p) * sizeof(T));
    }

    template<PARTICLE_VECTOR_ENABLE_IF_NOT_TRIVIALLY_COPYABLE(T)>
    static void move(T* dest, T* p, T* end) {
        if (dest > p && dest < end) {
            // Move elements in reverse order
            --p;
            --end;
            dest += end - p - 1;
            for (; end != p; --end, --dest) {
                new(dest) T(std::move(*end));
                end->~T();
            }
        } else if (dest != p) {
            for (; p != end; ++p, ++dest) {
                new(dest) T(std::move(*p));
                p->~T();
            }
        }
    }

    static T* find(T* p, const T* end, const T& value) {
        for (; p != end; ++p) {
            if (*p == value) {
                return p;
            }
        }
        return nullptr;
    }

    static T* rfind(T* p, const T* end, const T& value) {
        for (; p != end; --p) {
            if (*p == value) {
                return p;
            }
        }
        return nullptr;
    }

    template<typename... ArgsT>
    static void construct(T* p, const T* end, ArgsT&&... args) {
        for (; p != end; ++p) {
            new(p) T(std::forward<ArgsT>(args)...);
        }
    }

    static void destruct(T* p, const T* end) {
        for (; p != end; ++p) {
            p->~T();
        }
    }

    template<typename V, typename A>
    friend void swap(Vector<V, A>& vector, Vector<V, A>& vector2);
};

template<typename T, typename AllocatorT>
void swap(Vector<T, AllocatorT>& vector, Vector<T, AllocatorT>& vector2);

} // spark

namespace particle {

using ::spark::Vector;

} // particle

// spark::DefaultAllocator
inline void* spark::DefaultAllocator::malloc(size_t size) {
    return ::malloc(size);
}

inline void* spark::DefaultAllocator::realloc(void* ptr, size_t size) {
    return ::realloc(ptr, size);
}

inline void spark::DefaultAllocator::free(void* ptr) {
    ::free(ptr);
}

// spark::Vector
template<typename T, typename AllocatorT>
inline spark::Vector<T, AllocatorT>::Vector() :
        data_(nullptr),
        size_(0),
        capacity_(0) {
}

template<typename T, typename AllocatorT>
inline spark::Vector<T, AllocatorT>::Vector(int n) : Vector() {
    if (n > 0 && realloc(n)) {
        construct(data_, data_ + n);
        size_ = n;
    }
}

template<typename T, typename AllocatorT>
inline spark::Vector<T, AllocatorT>::Vector(int n, const T& value) : Vector() {
    if (n > 0 && realloc(n)) {
        construct(data_, data_ + n, value);
        size_ = n;
    }
}

template<typename T, typename AllocatorT>
inline spark::Vector<T, AllocatorT>::Vector(const T* values, int n) : Vector() {
    if (n > 0 && realloc(n)) {
        copy(data_, values, values + n);
        size_ = n;
    }
}

template<typename T, typename AllocatorT>
inline spark::Vector<T, AllocatorT>::Vector(std::initializer_list<T> values) : Vector() {
    const size_t n = values.size();
    if (n > 0 && realloc(n)) {
        copy(data_, values.begin(), values.end());
        size_ = n;
    }
}

template<typename T, typename AllocatorT>
inline spark::Vector<T, AllocatorT>::Vector(const Vector<T, AllocatorT>& vector) : Vector() {
    if (vector.size_ > 0 && realloc(vector.size_)) {
        copy(data_, vector.data_, vector.data_ + vector.size_);
        size_ = vector.size_;
    }
}

template<typename T, typename AllocatorT>
inline spark::Vector<T, AllocatorT>::Vector(Vector<T, AllocatorT>&& vector) : Vector() {
    swap(*this, vector);
}

template<typename T, typename AllocatorT>
inline spark::Vector<T, AllocatorT>::~Vector() {
    destruct(data_, data_ + size_);
    AllocatorT::free(data_);
}

template<typename T, typename AllocatorT>
inline bool spark::Vector<T, AllocatorT>::append(T value) {
    return insert(size_, std::move(value));
}

template<typename T, typename AllocatorT>
inline bool spark::Vector<T, AllocatorT>::append(int n, const T& value) {
    return insert(size_, n, value);
}

template<typename T, typename AllocatorT>
inline bool spark::Vector<T, AllocatorT>::append(const T* values, int n) {
    return insert(size_, values, n);
}

template<typename T, typename AllocatorT>
inline bool spark::Vector<T, AllocatorT>::append(const Vector<T, AllocatorT> &vector) {
    return insert(size_, vector);
}

template<typename T, typename AllocatorT>
inline bool spark::Vector<T, AllocatorT>::prepend(T value) {
    return insert(0, std::move(value));
}

template<typename T, typename AllocatorT>
inline bool spark::Vector<T, AllocatorT>::prepend(int n, const T& value) {
    return insert(0, n, value);
}

template<typename T, typename AllocatorT>
inline bool spark::Vector<T, AllocatorT>::prepend(const T* values, int n) {
    return insert(0, values, n);
}

template<typename T, typename AllocatorT>
inline bool spark::Vector<T, AllocatorT>::prepend(const Vector<T, AllocatorT> &vector) {
    return insert(0, vector);
}

template<typename T, typename AllocatorT>
inline bool spark::Vector<T, AllocatorT>::insert(int i, T value) {
    if (size_ + 1 > capacity_ && !realloc(size_ + 1)) {
        return false;
    }
    T* const p = data_ + i;
    move(p + 1, p, data_ + size_);
    new(p) T(std::move(value));
    ++size_;
    return true;
}

template<typename T, typename AllocatorT>
inline bool spark::Vector<T, AllocatorT>::insert(int i, int n, const T& value) {
    if (size_ + n > capacity_ && !realloc(size_ + n)) {
        return false;
    }
    T* const p = data_ + i;
    move(p + n, p, data_ + size_);
    construct(p, p + n, value);
    size_ += n;
    return true;
}

template<typename T, typename AllocatorT>
inline bool spark::Vector<T, AllocatorT>::insert(int i, const T* values, int n) {
    if (size_ + n > capacity_ && !realloc(size_ + n)) {
        return false;
    }
    T* const p = data_ + i;
    move(p + n, p, data_ + size_);
    copy(p, values, values + n);
    size_ += n;
    return true;
}

template<typename T, typename AllocatorT>
inline bool spark::Vector<T, AllocatorT>::insert(int i, const Vector<T, AllocatorT> &vector) {
    return insert(i, vector.data_, vector.size_);
}

template<typename T, typename AllocatorT>
inline void spark::Vector<T, AllocatorT>::removeAt(int i, int n) {
    if (n < 0 || i + n > size_) {
        n = size_ - i;
    }
    T* const p = data_ + i;
    destruct(p, p + n);
    move(p, p + n, data_ + size_);
    size_ -= n;
}

template<typename T, typename AllocatorT>
inline bool spark::Vector<T, AllocatorT>::removeOne(const T &value) {
    T* const p = find(data_, data_ + size_, value);
    if (!p) {
        return false;
    }
    p->~T();
    move(p, p + 1, data_ + size_);
    --size_;
    return true;
}

template<typename T, typename AllocatorT>
inline int spark::Vector<T, AllocatorT>::removeAll(const T &value) {
    T* p = data_;
    T* end = p + size_;
    while ((p = find(p, end, value))) {
        p->~T();
        move(p, p + 1, end);
        --end;
    }
    const int n = size_ - (end - data_);
    size_ -= n;
    return n;
}

template<typename T, typename AllocatorT>
inline T spark::Vector<T, AllocatorT>::takeFirst() {
    return takeAt(0);
}

template<typename T, typename AllocatorT>
inline T spark::Vector<T, AllocatorT>::takeLast() {
    return takeAt(size_ - 1);
}

template<typename T, typename AllocatorT>
inline T spark::Vector<T, AllocatorT>::takeAt(int i) {
    T* const p = data_ + i;
    T v(std::move(*p));
    p->~T();
    move(p, p + 1, data_ + size_);
    --size_;
    return v;
}

template<typename T, typename AllocatorT>
inline T& spark::Vector<T, AllocatorT>::first() {
    return data_[0];
}

template<typename T, typename AllocatorT>
inline const T& spark::Vector<T, AllocatorT>::first() const {
    return data_[0];
}

template<typename T, typename AllocatorT>
inline T& spark::Vector<T, AllocatorT>::last() {
    return data_[size_ - 1];
}

template<typename T, typename AllocatorT>
inline const T& spark::Vector<T, AllocatorT>::last() const {
    return data_[size_ - 1];
}

template<typename T, typename AllocatorT>
inline T& spark::Vector<T, AllocatorT>::at(int i) {
    return data_[i];
}

template<typename T, typename AllocatorT>
inline const T& spark::Vector<T, AllocatorT>::at(int i) const {
    return data_[i];
}

template<typename T, typename AllocatorT>
inline spark::Vector<T, AllocatorT> spark::Vector<T, AllocatorT>::copy(int i, int n) const {
    if (n < 0 || i + n > size_) {
        n = size_ - i;
    }
    Vector<T, AllocatorT> v;
    if (n > 0 && v.realloc(n)) {
        const T* const p = data_ + i;
        copy(v.data_, p, p + n);
        v.size_ = n;
    }
    return v;
}

template<typename T, typename AllocatorT>
inline int spark::Vector<T, AllocatorT>::indexOf(const T &value, int i) const {
    const T* const p = find(data_ + i, data_ + size_, value);
    if (!p) {
        return -1;
    }
    return p - data_;
}

template<typename T, typename AllocatorT>
inline int spark::Vector<T, AllocatorT>::lastIndexOf(const T &value) const {
    return lastIndexOf(value, size_ - 1);
}

template<typename T, typename AllocatorT>
inline int spark::Vector<T, AllocatorT>::lastIndexOf(const T &value, int i) const {
    const T* const p = rfind(data_ + i, data_ - 1, value);
    if (!p) {
        return -1;
    }
    return p - data_;
}

template<typename T, typename AllocatorT>
inline bool spark::Vector<T, AllocatorT>::contains(const T &value) const {
    return find(data_, data_ + size_, value);
}

template<typename T, typename AllocatorT>
inline spark::Vector<T, AllocatorT>& spark::Vector<T, AllocatorT>::fill(const T& value) {
    destruct(data_, data_ + size_);
    construct(data_, data_ + size_, value);
    return *this;
}

template<typename T, typename AllocatorT>
inline bool spark::Vector<T, AllocatorT>::resize(int n) {
    if (n > size_) {
        if (n > capacity_ && !realloc(n)) {
            return false;
        }
        construct(data_ + size_, data_ + n);
        size_ = n;
    } else if (n >= 0) {
        destruct(data_ + n, data_ + size_);
        size_ = n;
    }
    return true;
}

template<typename T, typename AllocatorT>
inline int spark::Vector<T, AllocatorT>::size() const {
    return size_;
}

template<typename T, typename AllocatorT>
inline bool spark::Vector<T, AllocatorT>::isEmpty() const {
    return size_ == 0;
}

template<typename T, typename AllocatorT>
inline bool spark::Vector<T, AllocatorT>::reserve(int n) {
    if (n > capacity_ && !realloc(n)) {
        return false;
    }
    return true;
}

template<typename T, typename AllocatorT>
inline int spark::Vector<T, AllocatorT>::capacity() const {
    return capacity_;
}

template<typename T, typename AllocatorT>
inline bool spark::Vector<T, AllocatorT>::trimToSize() {
    if (capacity_ > size_ && !realloc(size_)) {
        return false;
    }
    return true;
}

template<typename T, typename AllocatorT>
inline void spark::Vector<T, AllocatorT>::clear() {
    destruct(data_, data_ + size_);
    size_ = 0;
}

template<typename T, typename AllocatorT>
inline T* spark::Vector<T, AllocatorT>::data() {
    return data_;
}

template<typename T, typename AllocatorT>
inline const T* spark::Vector<T, AllocatorT>::data() const {
    return data_;
}

template<typename T, typename AllocatorT>
inline typename spark::Vector<T, AllocatorT>::Iterator spark::Vector<T, AllocatorT>::begin() {
    return data_;
}

template<typename T, typename AllocatorT>
inline typename spark::Vector<T, AllocatorT>::ConstIterator spark::Vector<T, AllocatorT>::begin() const {
    return data_;
}

template<typename T, typename AllocatorT>
inline typename spark::Vector<T, AllocatorT>::Iterator spark::Vector<T, AllocatorT>::end() {
    return data_ + size_;
}

template<typename T, typename AllocatorT>
inline typename spark::Vector<T, AllocatorT>::ConstIterator spark::Vector<T, AllocatorT>::end() const {
    return data_ + size_;
}

template<typename T, typename AllocatorT>
inline typename spark::Vector<T, AllocatorT>::Iterator spark::Vector<T, AllocatorT>::insert(ConstIterator pos, T value) {
    int i = pos - data_;
    if (!insert(i, std::move(value))) {
        return data_ + size_;
    }
    return data_ + i;
}

template<typename T, typename AllocatorT>
inline typename spark::Vector<T, AllocatorT>::Iterator spark::Vector<T, AllocatorT>::erase(ConstIterator pos) {
    int i = pos - data_;
    removeAt(i);
    return data_ + i;
}

template<typename T, typename AllocatorT>
inline T& spark::Vector<T, AllocatorT>::operator[](int i) {
    return data_[i];
}

template<typename T, typename AllocatorT>
inline const T& spark::Vector<T, AllocatorT>::operator[](int i) const {
    return data_[i];
}

template<typename T, typename AllocatorT>
inline bool spark::Vector<T, AllocatorT>::operator==(const Vector<T, AllocatorT> &vector) const {
    if (size_ != vector.size_) {
        return false;
    }
    const T* p = data_;
    const T* p2 = vector.data_;
    const T* const end = p + size_;
    for (; p != end; ++p, ++p2) {
        if (*p != *p2) {
            return false;
        }
    }
    return true;
}

template<typename T, typename AllocatorT>
inline bool spark::Vector<T, AllocatorT>::operator!=(const Vector<T, AllocatorT> &vector) const {
    return !(*this == vector);
}

template<typename T, typename AllocatorT>
inline spark::Vector<T, AllocatorT>& spark::Vector<T, AllocatorT>::operator=(Vector<T, AllocatorT> vector) {
    swap(*this, vector);
    return *this;
}

// spark::
template<typename T, typename AllocatorT>
inline void spark::swap(Vector<T, AllocatorT>& vector, Vector<T, AllocatorT>& vector2) {
    using std::swap;
    swap(vector.data_, vector2.data_);
    swap(vector.size_, vector2.size_);
    swap(vector.capacity_, vector2.capacity_);
}

#endif // SPARK_WIRING_VECTOR_H
