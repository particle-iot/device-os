#include <map>

#include "sparse_buffer.h"

#include "util/catch.h"

using namespace particle;

namespace {

void checkBuffer(const SparseBuffer& buf, const std::string& data) {
    std::map<size_t, std::string> seg; // Expected segments
    size_t size = 0; // Expected size
    size_t i = 0;
    while (i < data.size()) {
        if (data[i] != buf.fill()) {
            size_t j = i + 1;
            while (j < data.size() && data[j] != buf.fill()) {
                ++j;
            }
            seg.insert({ { i, data.substr(i, j - i) } });
            size = j;
            i = j;
        } else {
            ++i;
        }
    }
    CHECK(buf.segments() == seg);
    CHECK(buf.size() == size);
    CHECK(buf.isEmpty() == !size);
    CHECK(buf.read(0, size) == data);
}

} // namespace

TEST_CASE("SparseBuffer") {
    SparseBuffer buf('.');

    SECTION("initial state") {
        checkBuffer(buf, "");
    }

    SECTION("writing non-adjacent segments") {
        SECTION("") {
            buf.write(0, "a");
            checkBuffer(buf, "a");
        }
        SECTION("") {
            buf.write(3, "ab");
            checkBuffer(buf, "...ab");
        }
        SECTION("") {
            buf.write(1, "ab");
            checkBuffer(buf, ".ab");
            buf.write(6, "cde");
            checkBuffer(buf, ".ab...cde");
            buf.write(4, "f");
            checkBuffer(buf, ".ab.f.cde");
        }
    }

    SECTION("writing adjacent segments") {
        SECTION("") {
            buf.write(0, "a");
            checkBuffer(buf, "a");
            buf.write(1, "b");
            checkBuffer(buf, "ab");
        }
        SECTION("") {
            buf.write(1, "ab");
            checkBuffer(buf, ".ab");
            buf.write(0, "c");
            checkBuffer(buf, "cab");
        }
        SECTION("") {
            buf.write(4, "abc");
            checkBuffer(buf, "....abc");
            buf.write(1, "d");
            checkBuffer(buf, ".d..abc");
            buf.write(2, "ef");
            checkBuffer(buf, ".defabc");
        }
    }

    SECTION("writing overlapping segments") {
        SECTION("") {
            buf.write(0, "a");
            checkBuffer(buf, "a");
            buf.write(0, "b");
            checkBuffer(buf, "b");
        }
        SECTION("") {
            buf.write(1, "abc");
            checkBuffer(buf, ".abc");
            buf.write(0, "de");
            checkBuffer(buf, "debc");
        }
        SECTION("") {
            buf.write(1, "abc");
            checkBuffer(buf, ".abc");
            buf.write(3, "de");
            checkBuffer(buf, ".abde");
        }
        SECTION("") {
            buf.write(1, "a");
            checkBuffer(buf, ".a");
            buf.write(4, "bc");
            checkBuffer(buf, ".a..bc");
            buf.write(7, "d");
            checkBuffer(buf, ".a..bc.d");
            buf.write(9, "efg");
            checkBuffer(buf, ".a..bc.d.efg");
            buf.write(3, "hijkl");
            checkBuffer(buf, ".a.hijkl.efg");
        }
    }

    SECTION("reading empty buffer") {
        CHECK(buf.read(0, 3) == "...");
    }

    SECTION("reading entire buffer") {
        buf.write(0, "abc");
        checkBuffer(buf, "abc");
        CHECK(buf.read(0, 3) == "abc");
    }

    SECTION("reading past the last segment") {
        buf.write(1, "a");
        checkBuffer(buf, ".a");
        buf.write(3, "bc");
        checkBuffer(buf, ".a.bc");
        CHECK(buf.read(3, 5) == "bc...");
    }

    SECTION("reading part of a segment") {
        SECTION("") {
            buf.write(1, "abc");
            checkBuffer(buf, ".abc");
            CHECK(buf.read(0, 3) == ".ab");
        }
        SECTION("") {
            buf.write(1, "abc");
            checkBuffer(buf, ".abc");
            CHECK(buf.read(2, 2) == "bc");
        }
        SECTION("") {
            buf.write(1, "abcde");
            checkBuffer(buf, ".abcde");
            CHECK(buf.read(2, 2) == "bc");
        }
    }

    SECTION("reading multiple segments") {
        buf.write(1, "a");
        checkBuffer(buf, ".a");
        buf.write(3, "bcd");
        checkBuffer(buf, ".a.bcd");
        buf.write(8, "e");
        checkBuffer(buf, ".a.bcd..e");
        buf.write(10, "fg");
        checkBuffer(buf, ".a.bcd..e.fg");
        buf.write(13, "h");
        checkBuffer(buf, ".a.bcd..e.fg.h");
        CHECK(buf.read(5, 7) == "d..e.fg");
    }

    SECTION("erasing empty buffer") {
        buf.erase(0, 3);
        checkBuffer(buf, "");
    }

    SECTION("erasing entire buffer") {
        buf.write(0, "abc");
        checkBuffer(buf, "abc");
        buf.erase(0, 3);
        checkBuffer(buf, "");
    }

    SECTION("erasing past the last segment") {
        buf.write(1, "a");
        checkBuffer(buf, ".a");
        buf.write(3, "bc");
        checkBuffer(buf, ".a.bc");
        buf.erase(3, 5);
        checkBuffer(buf, ".a");
    }

    SECTION("erasing part of a segment") {
        SECTION("") {
            buf.write(1, "abc");
            checkBuffer(buf, ".abc");
            buf.erase(0, 3);
            checkBuffer(buf, "...c");
        }
        SECTION("") {
            buf.write(1, "abc");
            checkBuffer(buf, ".abc");
            buf.erase(2, 2);
            checkBuffer(buf, ".a");
        }
        SECTION("") {
            buf.write(1, "abcde");
            checkBuffer(buf, ".abcde");
            buf.erase(2, 2);
            checkBuffer(buf, ".a..de");
        }
    }

    SECTION("erasing multiple segments") {
        buf.write(1, "a");
        checkBuffer(buf, ".a");
        buf.write(3, "bcd");
        checkBuffer(buf, ".a.bcd");
        buf.write(8, "e");
        checkBuffer(buf, ".a.bcd..e");
        buf.write(10, "fg");
        checkBuffer(buf, ".a.bcd..e.fg");
        buf.write(13, "h");
        checkBuffer(buf, ".a.bcd..e.fg.h");
        buf.erase(5, 7);
        checkBuffer(buf, ".a.bc........h");
    }

    SECTION("clearing buffer") {
        buf.write(1, "a");
        checkBuffer(buf, ".a");
        buf.write(3, "bc");
        checkBuffer(buf, ".a.bc");
        buf.clear();
        checkBuffer(buf, "");
    }
}
