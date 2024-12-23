#include <cstring>

#include "spark_wiring_buffer.h"

#include "util/catch.h"

using namespace particle;

TEST_CASE("Buffer") {
    SECTION("Buffer()") {
        SECTION("constructs an empty buffer") {
            Buffer b;
            CHECK(b.size() == 0);
            CHECK(b.capacity() == 0);
            CHECK(b.isEmpty());
        }
    }

    SECTION("Buffer(size_t)") {
        SECTION("constructs a buffer of a specific size") {
            {
                Buffer b(0);
                CHECK(b.size() == 0);
                CHECK(b.capacity() == 0);
                CHECK(b.isEmpty());
            }
            {
                Buffer b(100);
                CHECK(b.size() == 100);
                CHECK(b.capacity() == 100);
                CHECK(!b.isEmpty());
            }
        }
    }

    SECTION("Buffer(const char*, size_t)") {
        SECTION("constructs a buffer by copying existing data") {
            {
                Buffer b(nullptr, 0);
                CHECK(b.size() == 0);
                CHECK(b.capacity() == 0);
                CHECK(b.isEmpty());
            }
            {
                char d[] = { 0x00, 0x11, 0x22, 0x33 };
                Buffer b(d, 4);
                CHECK(b.size() == 4);
                CHECK(b.capacity() == 4);
                CHECK(!b.isEmpty());
                CHECK(std::memcmp(b.data(), d, 4) == 0);
            }
        }
    }

    SECTION("Buffer(const Buffer&)") {
        SECTION("copy-constructs a buffer") {
            Buffer b;
            REQUIRE(b.reserve(10));
            b.resize(3);
            std::memcpy(b.data(), "abc", 3);

            Buffer b2(b);
            CHECK(b2.size() == 3);
            CHECK(b2.capacity() == 3); // Not the same capacity as the original buffer had
            CHECK(!b2.isEmpty());
            CHECK(std::memcmp(b2.data(), "abc", 3) == 0);
        }
    }

    SECTION("Buffer(Buffer&&)") {
        SECTION("move-constructs a buffer") {
            Buffer b("abc", 3);
            Buffer b2(std::move(b));
            CHECK(b2.size() == 3);
            CHECK(b2.capacity() == 3);
            CHECK(!b2.isEmpty());
            CHECK(std::memcmp(b2.data(), "abc", 3) == 0);
            CHECK(!b.data()); // The underlying buffer was moved to the other instance
        }
    }

    SECTION("data()") {
        SECTION("returns a pointer to the buffer data") {
            Buffer b("abc", 3);
            CHECK(std::memcmp(b.data(), "abc", 3) == 0);
            const Buffer b2("abc", 3);
            CHECK(std::memcmp(b2.data(), "abc", 3) == 0);
        }
    }

    SECTION("size()") {
        SECTION("returns the size of the buffer data") {
            Buffer b("abc", 3);
            CHECK(b.size() == 3);
        }
    }

    SECTION("isEmpty()") {
        SECTION("returns true if the buffer is empty") {
            Buffer b;
            CHECK(b.isEmpty());
        }
        SECTION("returns false if the buffer is not empty") {
            Buffer b("abc", 3);
            CHECK(!b.isEmpty());
        }
    }

    SECTION("resize()") {
        SECTION("can increase the buffer size") {
            Buffer b;
            CHECK(b.resize(3));
            CHECK(b.capacity() == 3);
            CHECK(b.size() == 3);
            std::memcpy(b.data(), "abc", 3);
            CHECK(b.resize(4));
            CHECK(b.capacity() == 4);
            CHECK(b.size() == 4);
            CHECK(std::memcmp(b.data(), "abc", 3) == 0);
        }
        SECTION("can decrease the buffer size") {
            Buffer b("abcd", 4);
            CHECK(b.resize(3));
            CHECK(b.capacity() == 4);
            CHECK(b.size() == 3);
            CHECK(std::memcmp(b.data(), "abc", 3) == 0);
            CHECK(b.resize(0));
            CHECK(b.capacity() == 4);
            CHECK(b.size() == 0);
        }
    }

    SECTION("reserve()") {
        SECTION("can increase the buffer capacity") {
            Buffer b;
            CHECK(b.reserve(3));
            CHECK(b.capacity() == 3);
            CHECK(b.size() == 0);
            CHECK(b.resize(3));
            std::memcpy(b.data(), "abc", 3);
            CHECK(b.reserve(4));
            CHECK(b.capacity() == 4);
            CHECK(b.size() == 3);
            CHECK(std::memcmp(b.data(), "abc", 3) == 0);
            CHECK(b.reserve(0));
            CHECK(b.capacity() == 4);
            CHECK(b.size() == 3);
        }
    }

    SECTION("capacity()") {
        SECTION("returns the capacity of the buffer") {
            Buffer b("abc", 3);
            CHECK(b.capacity() == 3);
            CHECK(b.resize(4));
            CHECK(b.capacity() == 4);
            CHECK(b.resize(2));
            CHECK(b.capacity() == 4);
        }
    }

    SECTION("trimToSize()") {
        SECTION("frees the unused buffer capacity") {
            Buffer b("abc", 3);
            CHECK(b.capacity() == 3);
            CHECK(b.size() == 3);
            CHECK(b.resize(2));
            CHECK(b.trimToSize());
            CHECK(b.capacity() == 2);
            CHECK(b.size() == 2);
            CHECK(std::memcmp(b.data(), "ab", 2) == 0);
        }
    }

    SECTION("slice()") {
        SECTION("returns a copy of a portion of the buffer") {
            Buffer b("abcd", 4);
            CHECK(b.slice(0, 0) == Buffer());
            CHECK(b.slice(0, 1) == Buffer("a", 1));
            CHECK(b.slice(0, 2) == Buffer("ab", 2));
            CHECK(b.slice(0) == Buffer("abcd", 4));
            CHECK(b.slice(1, 0) == Buffer());
            CHECK(b.slice(1, 1) == Buffer("b", 1));
            CHECK(b.slice(1, 2) == Buffer("bc", 2));
            CHECK(b.slice(1) == Buffer("bcd", 3));
            CHECK(b.slice(3, 0) == Buffer());
            CHECK(b.slice(3, 1) == Buffer("d", 1));
            CHECK(b.slice(3, 2) == Buffer("d", 1));
            CHECK(b.slice(3) == Buffer("d", 1));
        }
    }

    SECTION("concat()") {
        SECTION("concatenates the buffer with another buffer") {
            Buffer b;
            b = b.concat(Buffer("a", 1));
            CHECK(b == Buffer("a", 1));
            b = b.concat(Buffer());
            CHECK(b == Buffer("a", 1));
            b = b.concat(Buffer("bcd", 3));
            CHECK(b == Buffer("abcd", 4));
        }
    }

    SECTION("toHex()") {
        SECTION("converts the buffer data to a hex-encoded string") {
            {
                Buffer b;
                CHECK(b.toHex() == String(""));
            }
            {
                Buffer b("\x01\x23\x45\x67\x89\xab\xcd\xef", 8);
                CHECK(b.toHex() == String("0123456789abcdef"));
            }
        }
    }

    SECTION("toHex(char*, size_t)") {
        SECTION("converts the buffer data to a hex-encoded string") {
            {
                Buffer b;
                char out[2];
                std::memset(out, 0xff, sizeof(out));
                CHECK(b.toHex(out, sizeof(out)) == 0);
                CHECK(std::memcmp(out, "\x00\xff", 2) == 0);
            }
            {
                Buffer b("\x01\x23\x45\x67\x89\xab\xcd\xef", 8);
                char out[17];
                std::memset(out, 0xff, sizeof(out));
                CHECK(b.toHex(out, sizeof(out)) == 16);
                CHECK(std::memcmp(out, "0123456789abcdef\0", 17) == 0);
            }
            {
                Buffer b("\x01\x23", 2);
                char out[4];
                std::memset(out, 0xff, sizeof(out));
                CHECK(b.toHex(out, sizeof(out)) == 3);
                CHECK(std::memcmp(out, "012\0", 4) == 0);
            }
        }
    }

    SECTION("fromHex(const char*)") {
        SECTION("constructs a buffer from a hex-encoded string") {
            {
                Buffer b = Buffer::fromHex("");
                CHECK(b.isEmpty());
            }
            {
                Buffer b = Buffer::fromHex("0123456789abcdef");
                CHECK(b.size() == 8);
                CHECK(std::memcmp(b.data(), "\x01\x23\x45\x67\x89\xab\xcd\xef", 8) == 0);
            }
            {
                Buffer b = Buffer::fromHex("0123456789ABCDEF");
                CHECK(b.size() == 8);
                CHECK(std::memcmp(b.data(), "\x01\x23\x45\x67\x89\xab\xcd\xef", 8) == 0);
            }
            {
                Buffer b = Buffer::fromHex("01234");
                CHECK(b.size() == 2);
                CHECK(std::memcmp(b.data(), "\x01\x23", 2) == 0);
            }
            {
                Buffer b = Buffer::fromHex("0123.567");
                CHECK(b.size() == 2);
                CHECK(std::memcmp(b.data(), "\x01\x23", 2) == 0);
            }
            {
                Buffer b = Buffer::fromHex("01234.67");
                CHECK(b.size() == 2);
                CHECK(std::memcmp(b.data(), "\x01\x23", 2) == 0);
            }
        }
    }

    SECTION("fromHex(const char*, size_t)") {
        SECTION("constructs a buffer from a hex-encoded string") {
            {
                Buffer b = Buffer::fromHex(nullptr, 0);
                CHECK(b.isEmpty());
            }
            {
                Buffer b = Buffer::fromHex("0123456789abcdef", 16);
                CHECK(b.size() == 8);
                CHECK(std::memcmp(b.data(), "\x01\x23\x45\x67\x89\xab\xcd\xef", 8) == 0);
            }
            {
                Buffer b = Buffer::fromHex("0123456789abcdef", 15);
                CHECK(b.size() == 7);
                CHECK(std::memcmp(b.data(), "\x01\x23\x45\x67\x89\xab\xcd", 7) == 0);
            }
        }
    }

    SECTION("fromHex(const String&)") {
        SECTION("constructs a buffer from a hex-encoded string") {
            {
                Buffer b = Buffer::fromHex(String());
                CHECK(b.isEmpty());
            }
            {
                Buffer b = Buffer::fromHex(String("0123456789abcdef"));
                CHECK(b.size() == 8);
                CHECK(std::memcmp(b.data(), "\x01\x23\x45\x67\x89\xab\xcd\xef", 8) == 0);
            }
        }
    }

    SECTION("operator==") {
        SECTION("compares the buffer with another buffer") {
            {
                Buffer b;
                Buffer b2;
                CHECK(b == b2);
            }
            {
                Buffer b("abc", 3);
                Buffer b2("abc", 3);
                CHECK(b == b2);
            }
            {
                Buffer b("abc", 3);
                Buffer b2("abcd", 4);
                CHECK(!(b == b2));
            }
            {
                Buffer b("abc", 3);
                Buffer b2("abd", 3);
                CHECK(!(b == b2));
            }
        }
    }

    SECTION("operator!=") {
        SECTION("compares the buffer with another buffer") {
            {
                Buffer b;
                Buffer b2;
                CHECK(!(b != b2));
            }
            {
                Buffer b("abc", 3);
                Buffer b2("abc", 3);
                CHECK(!(b != b2));
            }
            {
                Buffer b("abc", 3);
                Buffer b2("abcd", 4);
                CHECK(b != b2);
            }
            {
                Buffer b("abc", 3);
                Buffer b2("abd", 3);
                CHECK(b != b2);
            }
        }
    }
}
