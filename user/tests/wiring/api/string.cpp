#include "testapi.h"

test(api_string) {
    // String()
    String s;
    API_COMPILE(String());
    API_COMPILE(String("abc"));
    API_COMPILE(String("abc", 3));
    API_COMPILE(String(s)); // Copy constructor
    API_COMPILE(String(std::move(s))); // Move constructor
    API_COMPILE(String('a'));
    API_COMPILE(String(123, (unsigned)10 /* base */)); // FIXME: Ambiguous call
    API_COMPILE(String(123.0, 2 /* decimalPlaces */));

    // reserve()
    bool b = false;
    API_COMPILE(b = s.reserve(10));

    // length()
    unsigned u = 0;
    API_COMPILE(u = s.length());

    // concat()
    String s2;
    API_COMPILE(b = s.concat(s2));
    API_COMPILE(b = s.concat("abc"));
    API_COMPILE(b = s.concat('a'));
    API_COMPILE(b = s.concat(123));
    API_COMPILE(b = s.concat(123.0));

    // startsWith(), endsWith()
    API_COMPILE(b = s.startsWith(s2));
    API_COMPILE(b = s.startsWith(s2, 10 /* offset */));
    API_COMPILE(b = s.endsWith(s2));

    // compareTo(), equals(), equalsIgnoreCase()
    int i = 0;
    API_COMPILE(i = s.compareTo(s2));
    API_COMPILE(b = s.equals(s2));
    API_COMPILE(b = s.equals("abc"));
    API_COMPILE(b = s.equalsIgnoreCase(s2));

    // indexOf(), lastIndexOf()
    API_COMPILE(i = s.indexOf('a'));
    API_COMPILE(i = s.indexOf('a', 10 /* fromIndex */));
    API_COMPILE(i = s.indexOf(s2));
    API_COMPILE(i = s.indexOf(s2, 10 /* fromIndex */));
    API_COMPILE(i = s.lastIndexOf('a'));
    API_COMPILE(i = s.lastIndexOf('a', 10 /* fromIndex */));
    API_COMPILE(i = s.lastIndexOf(s2));
    API_COMPILE(i = s.lastIndexOf(s2, 10 /* fromIndex */));

    // substring()
    API_COMPILE(s2 = s.substring(1 /* beginIndex */));
    API_COMPILE(s2 = s.substring(1 /* beginIndex */, 10 /* endIndex */));

    // getBytes(), toCharArray(), c_str()
    char* buf = nullptr;
    const char* cstr = nullptr;
    API_COMPILE(s.getBytes((unsigned char*)buf, 10 /* bufsize */, 1 /* index */));
    API_COMPILE(s.toCharArray(buf, 10 /* bufsize */, 1 /* index */));
    API_COMPILE(cstr = s.c_str());

    // replace()
    String s3;
    API_COMPILE(s.replace('a', 'b'));
    API_COMPILE(s.replace(s2, s3));

    // remove()
    API_COMPILE(s.remove(10 /* index */));
    API_COMPILE(s.remove(10 /* index */, 1 /* count */));

    // toLowerCase(), toUpperCase()
    API_COMPILE(s2 = s.toLowerCase());
    API_COMPILE(s2 = s.toUpperCase());

    // toInt(), toFloat()
    float f = 0.0;
    API_COMPILE(i = s.toInt());
    API_COMPILE(f = s.toFloat());

    // trim()
    API_COMPILE(s2 = s.trim());

    // charAt(), setCharAt(), operator[]()
    char c = 0;
    API_COMPILE(c = s.charAt(10));
    API_COMPILE(s.setCharAt(10, 'a'));
    API_COMPILE(c = s[(unsigned)10]); // FIXME: Ambiguous call
    API_COMPILE(s[(unsigned)10] = 'a'); // FIXME: Ambiguous call

    // format()
    API_COMPILE(s = String::format("%d", 123));

    // operator=()
    API_COMPILE(s = s2);
    API_COMPILE(s = "abc");
    API_COMPILE(s = std::move(s2)); // Move assignment

    // operator+=()
    API_COMPILE(s += s2);
    API_COMPILE(s += "abc");
    API_COMPILE(s += 'a');
    API_COMPILE(s += 123);

    // Comparison operators
    API_COMPILE(s == s2);
    API_COMPILE(s == "abc");
    API_COMPILE(s != s2);
    API_COMPILE(s != "abc");
    API_COMPILE(s < s2);
    API_COMPILE(s <= s2);
    API_COMPILE(s > s2);
    API_COMPILE(s >= s2);

    // operator const char*()
    API_COMPILE(cstr = s);

    // Mute unused variable warnings
    (void)b;
    (void)c;
    (void)i;
    (void)u;
    (void)f;
    (void)cstr;
}

test(api_string_constructor_printable) {
    IPAddress address;
    API_COMPILE(String(address));
    (void)address;
}
