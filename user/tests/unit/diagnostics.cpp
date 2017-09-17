#include "spark_wiring_diagnostics.h"

#include "tools/catch.h"

#include <functional>
#include <unordered_set>
#include <cassert>

namespace {

using namespace particle;

class EnumSourcesCallback {
public:
    typedef std::function<void(const diag_source*)> Func;

    explicit EnumSourcesCallback(Func func = nullptr) :
            func_(std::move(func)) {
    }

    diag_enum_sources_callback func() const {
        return callback;
    }

    void* data() const {
        return const_cast<EnumSourcesCallback*>(this);
    }

private:
    Func func_;

    static void callback(const diag_source* src, void* data) {
        assert(data);
        const auto d = static_cast<EnumSourcesCallback*>(data);
        if (d->func_) {
            d->func_(src);
        }
    }
};

class GetData {
public:
    explicit GetData(diag_source_get_cmd_data* data) :
            d_(data) {
    }

    int setInt(int32_t val) {
        if (d_->data) {
            if (d_->data_size < sizeof(int32_t)) {
                return SYSTEM_ERROR_TOO_LARGE;
            }
            *(int32_t*)d_->data = val;
        }
        d_->data_size = sizeof(int32_t);
        return SYSTEM_ERROR_NONE;
    }

private:
    diag_source_get_cmd_data* d_;
};

class DiagSource {
public:
    typedef std::function<int(GetData)> GetFunc; // Getter function

    explicit DiagSource(uint16_t id = DIAG_SOURCE_INVALID) :
            d_(new Data) {
        d_->src = { sizeof(diag_source) /* size */, id, DIAG_TYPE_INT, nullptr /* name */, 0 /* flags */,
                d_.get() /* data */, callback };
    }

    DiagSource& id(uint16_t id) {
        d_->src.id = id;
        return *this;
    }

    uint16_t id() const {
        return d_->src.id;
    }

    DiagSource& name(std::string name) {
        d_->name = std::move(name);
        d_->src.name = d_->name.data();
        return *this;
    }

    const std::string& name() const {
        return d_->name;
    }

    DiagSource& type(diag_type type) {
        d_->src.type = (uint16_t)type;
        return *this;
    }

    diag_type type() const {
        return (diag_type)d_->src.type;
    }

    DiagSource& get(GetFunc func) {
        d_->get = std::move(func);
        return *this;
    }

    DiagSource& add() {
        REQUIRE(diag_register_source(&d_->src, nullptr) == 0);
        return *this;
    }

    const diag_source* operator&() const {
        return &d_->src;
    }

private:
    struct Data {
        diag_source src;
        std::string name;
        GetFunc get;
    };

    std::shared_ptr<Data> d_;

    static int callback(const diag_source* src, int cmd, void* data) {
        const auto d = static_cast<Data*>(src->data);
        switch (cmd) {
        case DIAG_CMD_GET: {
            if (!d->get) {
                return SYSTEM_ERROR_NOT_SUPPORTED;
            }
            const auto cmdData = static_cast<diag_source_get_cmd_data*>(data);
            return d->get(GetData(cmdData));
        }
        default:
            return SYSTEM_ERROR_NOT_SUPPORTED;
        }
    }
};

class DiagService {
public:
    DiagService() {
    }

    ~DiagService() {
        REQUIRE(diag_service_cmd(DIAG_CMD_RESET, nullptr, nullptr) == 0);
    }

    void enabled(bool enabled) {
        REQUIRE(diag_service_cmd(enabled ? DIAG_CMD_ENABLE : DIAG_CMD_DISABLE, nullptr, nullptr) == 0);
    }

    static std::vector<const diag_source*> sources() {
        std::vector<const diag_source*> srcs;
        EnumSourcesCallback cb([&srcs](const diag_source* src) {
            srcs.push_back(src);
        });
        size_t count = 0;
        REQUIRE(diag_enum_sources(cb.func(), &count, cb.data(), nullptr) == 0);
        REQUIRE(srcs.size() == count);
        return srcs;
    }
};

enum class TestEnum {
    ONE,
    TWO,
    THREE
};

template<typename DiagnosticDataT>
void testInt(DiagService& diag) {
    DiagnosticDataT d(1);
    diag.enabled(true);

    SECTION("operator=(); operator int32_t()") {
        CHECK(d == 0);
        CHECK((d = 1) == 1);
        CHECK(d == 1);
    }

    SECTION("operator++()") {
        CHECK(++d == 1);
        CHECK(++d == 2);
        CHECK(++d == 3);
        CHECK(d == 3);
    }

    SECTION("operator++(int)") {
        CHECK(d++ == 0);
        CHECK(d++ == 1);
        CHECK(d++ == 2);
        CHECK(d == 3);
    }

    SECTION("operator--()") {
        CHECK(--d == -1);
        CHECK(--d == -2);
        CHECK(--d == -3);
        CHECK(d == -3);
    }

    SECTION("operator--(int)") {
        CHECK(d-- == 0);
        CHECK(d-- == -1);
        CHECK(d-- == -2);
        CHECK(d == -3);
    }

    SECTION("operator+=()") {
        CHECK((d += 1) == 1);
        CHECK((d += 2) == 3);
        CHECK((d += 3) == 6);
        CHECK(d == 6);
    }

    SECTION("operator-=()") {
        CHECK((d -= 1) == -1);
        CHECK((d -= 2) == -3);
        CHECK((d -= 3) == -6);
        CHECK(d == -6);
    }
}

template<typename DiagnosticDataT>
void testEnum(DiagService& diag) {
    DiagnosticDataT d(1, TestEnum::ONE);
    diag.enabled(true);

    SECTION("operator T()") {
        CHECK(d == TestEnum::ONE);
    }

    SECTION("operator=()") {
        CHECK((d = TestEnum::ONE) == TestEnum::ONE);
        CHECK((d = TestEnum::TWO) == TestEnum::TWO);
    }
}

} // namespace

TEST_CASE("Service API") {
    DiagService diag;

    SECTION("diag_register_source()") {
        SECTION("data sources can be registered only when the service is in its initial disabled state") {
            auto d1 = DiagSource(1);
            CHECK(diag_register_source(&d1, nullptr) == 0);
            diag.enabled(true);
            auto d2 = DiagSource(2);
            CHECK(diag_register_source(&d1, nullptr) == SYSTEM_ERROR_INVALID_STATE);
        }

        SECTION("no data sources are registered initially") {
            diag.enabled(true);
            CHECK(diag.sources().empty());
        }

        SECTION("registers a new data source") {
            auto d1 = DiagSource(1);
            CHECK(diag_register_source(&d1, nullptr) == 0);
            diag.enabled(true);
            auto all = diag.sources();
            REQUIRE(all.size() == 1);
            CHECK(all.front()->id == d1.id());
        }

        SECTION("can be used to register multiple data sources") {
            const size_t count = 50;
            std::vector<DiagSource> sources;
            std::unordered_set<uint16_t> ids;
            for (size_t i = 0; i < count; ++i) {
                const uint16_t id = i + 1;
                sources.push_back(DiagSource(id));
                REQUIRE(diag_register_source(&sources.back(), nullptr) == 0);
                ids.insert(id);
            }
            diag.enabled(true);
            auto all = diag.sources();
            REQUIRE(all.size() == sources.size());
            for (const diag_source* d: all) {
                REQUIRE(ids.erase(d->id) == 1);
            }
        }

        SECTION("attempt to register a data source with a duplicate ID fails") {
            auto d1 = DiagSource(1);
            CHECK(diag_register_source(&d1, nullptr) == 0);
            auto d2 = DiagSource(1);
            CHECK(diag_register_source(&d2, nullptr) == SYSTEM_ERROR_ALREADY_EXISTS);
        }
    }

    SECTION("diag_enum_sources()") {
        auto d1 = DiagSource(1).add();
        auto d2 = DiagSource(2).add();
        auto d3 = DiagSource(3).add();

        SECTION("the service needs to be enabled before enumerating data sources") {
            EnumSourcesCallback cb;
            size_t count = 0;
            CHECK(diag_enum_sources(cb.func(), &count, cb.data(), nullptr) == SYSTEM_ERROR_INVALID_STATE);
        }

        SECTION("enumerates all registered data sources") {
            diag.enabled(true);
            EnumSourcesCallback cb([=](const diag_source* d) {
                REQUIRE((d->id == d1.id() || d->id == d2.id() || d->id == d3.id()));
            });
            size_t count = 0;
            CHECK(diag_enum_sources(cb.func(), &count, cb.data(), nullptr) == 0);
            CHECK(count == 3);
        }

        SECTION("the callback argument is optional") {
            diag.enabled(true);
            size_t count = 0;
            CHECK(diag_enum_sources(nullptr /* callback */, &count, nullptr /* data */, nullptr) == 0);
            CHECK(count == 3);
        }

        SECTION("the count argument is optional") {
            diag.enabled(true);
            EnumSourcesCallback cb;
            CHECK(diag_enum_sources(cb.func(), nullptr /* count */, cb.data(), nullptr) == 0);
        }
    }

    SECTION("diag_get_source()") {
        auto d1 = DiagSource(1).add();
        auto d2 = DiagSource(2).add();
        auto d3 = DiagSource(3).add();

        SECTION("the service needs to be enabled before retrieving a registered data source") {
            const diag_source* d = nullptr;
            CHECK(diag_get_source(1, &d, nullptr) == SYSTEM_ERROR_INVALID_STATE);
        }

        SECTION("retrieves a data source handle for a given ID") {
            diag.enabled(true);
            const diag_source* d = nullptr;
            CHECK(diag_get_source(d2.id(), &d, nullptr) == 0);
            REQUIRE(d != nullptr);
            CHECK(d->id == d2.id());
            CHECK(diag_get_source(4, &d, nullptr) == SYSTEM_ERROR_NOT_FOUND);
        }

        SECTION("the handle argument is optional") {
            diag.enabled(true);
            CHECK(diag_get_source(d3.id(), nullptr /* src */, nullptr) == 0);
            CHECK(diag_get_source(4, nullptr, nullptr) == SYSTEM_ERROR_NOT_FOUND);
        }
    }

    SECTION("diag_service_cmd()") {
        SECTION("can be used to enable the diagnostics service") {
            CHECK(diag_service_cmd(DIAG_CMD_ENABLE, nullptr, nullptr) == 0);
        }
    }
}

TEST_CASE("Wiring API") {
    DiagService diag;

    SECTION("DiagnosticData") {
        SECTION("get()") {
            auto d1 = DiagSource(1).type(DIAG_TYPE_INT).get([](GetData d) {
                return d.setInt(1234);
            }).add();
            auto d2 = DiagSource(2).type(DIAG_TYPE_INT).get([](GetData) {
                return SYSTEM_ERROR_UNKNOWN;
            }).add();

            diag.enabled(true);

            SECTION("can access a data source by ID") {
                int32_t val = 0;
                size_t size = sizeof(val);
                CHECK(DiagnosticData::get(1, &val, size) == 0);
                CHECK(val == 1234);
                CHECK(size == sizeof(val));
            }

            SECTION("can access a data source via its handle") {
                int32_t val = 0;
                size_t size = sizeof(val);
                CHECK(DiagnosticData::get(&d1, &val, size) == 0);
                CHECK(val == 1234);
                CHECK(size == sizeof(val));
            }

            SECTION("the data argument is optional") {
                size_t size = 0;
                CHECK(DiagnosticData::get(1, nullptr /* data */, size) == 0);
                CHECK(size == sizeof(int32_t));
            }

            SECTION("getter errors are forwarded to the caller code") {
                int32_t val = 0;
                size_t size = sizeof(val);
                CHECK(DiagnosticData::get(&d2, &val, size) == SYSTEM_ERROR_UNKNOWN);
            }
        }
    }

    SECTION("IntegerDiagnosticData") {
        SECTION("get()") {
            auto d1 = DiagSource(1).type(DIAG_TYPE_INT).get([](GetData d) {
                return d.setInt(1234);
            }).add();
            auto d2 = DiagSource(2).type(DIAG_TYPE_INT).get([](GetData) {
                return SYSTEM_ERROR_UNKNOWN;
            }).add();

            diag.enabled(true);

            SECTION("can access a data source by ID") {
                int32_t val = 0;
                CHECK(IntegerDiagnosticData::get(1, val) == 0);
                CHECK(val == 1234);
            }

            SECTION("can access a data source via its handle") {
                int32_t val = 0;
                CHECK(IntegerDiagnosticData::get(&d1, val) == 0);
                CHECK(val == 1234);
            }

            SECTION("getter errors are forwarded to the caller code") {
                int32_t val = 0;
                CHECK(IntegerDiagnosticData::get(&d2, val) == SYSTEM_ERROR_UNKNOWN);
            }
        }
    }

    SECTION("SimpleIntegerDiagnosticData") {
        testInt<SimpleIntegerDiagnosticData>(diag);
    }

    SECTION("AtomicIntegerDiagnosticData") {
        testInt<AtomicIntegerDiagnosticData>(diag);
    }

    SECTION("SimpleEnumDiagnosticData") {
        testEnum<SimpleEnumDiagnosticData<TestEnum>>(diag);
    }

    SECTION("AtomicEnumDiagnosticData") {
        testEnum<AtomicEnumDiagnosticData<TestEnum>>(diag);
    }
}
