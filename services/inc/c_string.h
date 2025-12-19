/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
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

#pragma once

#include <utility>
#include <cstdlib>
#include <cstring>

namespace particle {

class CString {
public:
    struct Less;

    CString() :
            s_(nullptr) {
    }

    CString(const char* str) :
            s_(str ? strdup(str) : nullptr) {
    }

    CString(const char* str, size_t n) :
            s_(str ? strndup(str, n) : nullptr) {
    }

    CString(const CString& str) :
            s_(str.s_ ? strdup(str.s_) : nullptr) {
    }

    CString(CString&& str) : CString() {
        swap(*this, str);
    }

    ~CString() {
        free(const_cast<char*>(s_));
    }

    char* unwrap() {
        const auto s = const_cast<char*>(s_);
        s_ = nullptr;
        return s;
    }

    CString& operator=(CString str) {
        swap(*this, str);
        return *this;
    }

    operator const char*() const {
        return s_;
    }

    static int compare(const CString& str1, const CString& str2) {
        return compare(str1.s_, str2.s_);
    }

    static int compare(const CString& str1, const char* str2) {
        return compare(str1.s_, str2);
    }

    static int compare(const char* str1, const CString& str2) {
        return compare(str1, str2.s_);
    }

    static CString wrap(char* str) {
        CString s;
        s.s_ = str;
        return s;
    }

    // Declare comparison operators as deleted to avoid their misuse
    //
    // TODO: Is this due to the implicit const char* operator? If so, we should probably remove
    // that and enable the comparison operators back
    bool operator==(const CString&) const = delete;
    bool operator==(const char*) const = delete;
    bool operator!=(const CString&) const = delete;
    bool operator!=(const char*) const = delete;
    bool operator<(const CString&) const = delete;
    bool operator<(const char*) const = delete;
    bool operator<=(const CString&) const = delete;
    bool operator<=(const char*) const = delete;
    bool operator>(const CString&) const = delete;
    bool operator>(const char*) const = delete;
    bool operator>=(const CString&) const = delete;
    bool operator>=(const char*) const = delete;

private:
    const char* s_;

    static int compare(const char* str1, const char* str2) {
        if (!str1) {
            if (!str2) {
                return 0; // str1 == NULL && str2 == NULL
            }
            return -1; // str1 == NULL && str2 != NULL
        }
        if (!str2) {
            return 1; // str1 != NULL && str2 == NULL
        }
        return strcmp(str1, str2); // str1 != NULL && str2 != NULL
    }

    friend void swap(CString& str1, CString& str2) {
        using std::swap;
        swap(str1.s_, str2.s_);
    }
};

struct CString::Less {
    bool operator()(const CString& str1, const CString& str2) const {
        return CString::compare(str1, str2) < 0;
    }

    bool operator()(const CString& str1, const char* str2) const {
        return CString::compare(str1, str2) < 0;
    }

    bool operator()(const char* str1, const CString& str2) const {
        return CString::compare(str1, str2) < 0;
    }
};

} // particle
