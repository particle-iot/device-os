#include <string>
#include <vector>

#include "spark_wiring_map.h"

#include "util/catch.h"

using namespace particle;

namespace {

template<typename MapT>
void checkMap(const MapT& map, const std::vector<typename MapT::Entry>& expectedEntries) {
    auto& entries = map.entries();
    REQUIRE(entries.size() == expectedEntries.size());
    for (int i = 0; i < entries.size(); ++i) {
        CHECK(entries[i] == expectedEntries[i]);
        CHECK(map.has(entries[i].first));
    }
    CHECK(map.size() == entries.size());
    CHECK(map.capacity() >= map.size());
    CHECK(((map.isEmpty() && map.size() == 0) || (!map.isEmpty() && map.size() > 0)));
}

} // namespace

TEST_CASE("Map") {
    SECTION("Map()") {
        Map<std::string, int> m;
        checkMap(m, {});
    }

    SECTION("set()") {
        Map<std::string, int> m;
        m.set("b", 2);
        checkMap(m, { { "b", 2 } });
        m.set("c", 3);
        checkMap(m, { { "b", 2 }, { "c", 3 } });
        m.set("a", 1);
        checkMap(m, { { "a", 1 }, { "b", 2 }, { "c", 3 } });
    }

    SECTION("get()") {
        Map<std::string, int> m({ { "a", 1 }, { "b", 2 }, { "c", 3 } });
        CHECK(m.get("a") == 1);
        CHECK(m.get("b") == 2);
        CHECK(m.get("c") == 3);
        CHECK(m.get("d", 4) == 4);
    }

    SECTION("remove()") {
        Map<std::string, int> m({ { "a", 1 }, { "b", 2 }, { "c", 3 } });
        CHECK(m.remove("b"));
        checkMap(m, { { "a", 1 }, { "c", 3 } });
        CHECK(m.remove("c"));
        checkMap(m, { { "a", 1 } });
        CHECK(!m.remove("d"));
        checkMap(m, { { "a", 1 } });
        CHECK(m.remove("a"));
        checkMap(m, {});
    }

    SECTION("operator[]") {
        Map<std::string, int> m;
        m["b"] = 2;
        checkMap(m, { { "b", 2 } });
        m["c"] = 3;
        checkMap(m, { { "b", 2 }, { "c", 3 } });
        m["a"] = 1;
        checkMap(m, { { "a", 1 }, { "b", 2 }, { "c", 3 } });
        CHECK(m["a"] == 1);
        CHECK(m["b"] == 2);
        CHECK(m["c"] == 3);
    }
}
