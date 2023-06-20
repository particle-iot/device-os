#include <string>
#include <vector>

#include "spark_wiring_map.h"

#include "util/catch.h"

using namespace particle;

namespace {

template<typename MapT>
void checkEntries(const MapT& map, const std::vector<typename MapT::Entry>& expectedEntries) {
    auto& entries = map.entries();
    REQUIRE(entries.size() == expectedEntries.size());
    auto keys = map.keys();
    REQUIRE(keys.size() == entries.size());
    auto values = map.values();
    REQUIRE(values.size() == entries.size());
    for (int i = 0; i < entries.size(); ++i) {
        CHECK(entries[i] == expectedEntries[i]);
        CHECK(keys[i] == entries[i].first);
        CHECK(values[i] == entries[i].second);
        CHECK(map.has(keys[i]));
    }
    CHECK(map.size() == entries.size());
    CHECK(map.capacity() >= map.size());
    CHECK(((map.isEmpty() && map.size() == 0) || (!map.isEmpty() && map.size() > 0)));
}

} // namespace

TEST_CASE("Map") {
    SECTION("Map()") {
        Map<std::string, int> m;
        checkEntries(m, {});
    }

    SECTION("set()") {
        Map<std::string, int> m;
        m.set("b", 2);
        checkEntries(m, { { "b", 2 } });
        m.set("c", 3);
        checkEntries(m, { { "b", 2 }, { "c", 3 } });
        m.set("a", 1);
        checkEntries(m, { { "a", 1 }, { "b", 2 }, { "c", 3 } });
    }

    SECTION("get()") {
        Map<std::string, int> m({ { "a", 1 }, { "b", 2 }, { "c", 3 } });
        CHECK(m.get("a") == 1);
        CHECK(m.get("b") == 2);
        CHECK(m.get("c") == 3);
        CHECK(m.get("d", 4) == 4);
    }

    SECTION("take()") {
        Map<std::string, int> m({ { "a", 1 }, { "b", 2 }, { "c", 3 } });
        CHECK(m.take("b") == 2);
        checkEntries(m, { { "a", 1 }, { "c", 3 } });
        CHECK(m.take("c") == 3);
        checkEntries(m, { { "a", 1 } });
        CHECK(m.take("d", 4) == 4);
        checkEntries(m, { { "a", 1 } });
        CHECK(m.take("a") == 1);
        checkEntries(m, {});
    }

    SECTION("remove()") {
        Map<std::string, int> m({ { "a", 1 }, { "b", 2 }, { "c", 3 } });
        CHECK(m.remove("b"));
        checkEntries(m, { { "a", 1 }, { "c", 3 } });
        CHECK(m.remove("c"));
        checkEntries(m, { { "a", 1 } });
        CHECK(!m.remove("d"));
        checkEntries(m, { { "a", 1 } });
        CHECK(m.remove("a"));
        checkEntries(m, {});
    }

    SECTION("operator[]") {
        Map<std::string, int> m;
        m["b"] = 2;
        checkEntries(m, { { "b", 2 } });
        m["c"] = 3;
        checkEntries(m, { { "b", 2 }, { "c", 3 } });
        m["a"] = 1;
        checkEntries(m, { { "a", 1 }, { "b", 2 }, { "c", 3 } });
        CHECK(m["a"] == 1);
        CHECK(m["b"] == 2);
        CHECK(m["c"] == 3);
    }
}
