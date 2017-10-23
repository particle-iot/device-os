#include "spark_wiring_diagnostics.h"

#include "tools/catch.h"

#include <functional>
#include <unordered_set>
#include <cassert>

namespace {

using namespace particle;

class EnumSourcesCallback {
public:
    typedef std::function<int(const diag_source*)> Func;

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

    static int callback(const diag_source* src, void* data) {
        assert(data);
        const auto d = static_cast<EnumSourcesCallback*>(data);
        return (d->func_ ? d->func_(src) : 0);
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
        return 0;
    }

private:
    diag_source_get_cmd_data* d_;
};

class DiagSource {
public:
    typedef std::function<int(GetData)> GetFunc; // Getter function

    explicit DiagSource(uint16_t id = DIAG_ID_INVALID) :
            d_(new Data) {
        d_->src = { sizeof(diag_source), 0 /* flags */, id, DIAG_TYPE_INT, nullptr /* name */, d_.get() /* data */,
                callback };
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
        case DIAG_SOURCE_CMD_GET: {
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
        REQUIRE(diag_command(DIAG_SERVICE_CMD_RESET, nullptr, nullptr) == 0);
    }

    void start() {
        REQUIRE(diag_command(DIAG_SERVICE_CMD_START, nullptr, nullptr) == 0);
    }

    static std::vector<const diag_source*> sources() {
        std::vector<const diag_source*> srcs;
        EnumSourcesCallback cb([&srcs](const diag_source* src) {
            srcs.push_back(src);
            return 0;
        });
        size_t count = 0;
        REQUIRE(diag_enum_sources(cb.func(), &count, cb.data(), nullptr) == 0);
        REQUIRE(srcs.size() == count);
        return srcs;
    }
};

// Dummy storage for persistent diagnostic data
template<typename ValueT>
class NonPersistentStorage {
public:
    typedef ValueT ValueType;

    NonPersistentStorage() :
            val_(ValueT()),
            storedVal_(ValueT()) {
    }

    void init(DiagnosticDataId id, const ValueT& val) {
        val_ = val;
        storedVal_ = val;
    }

    void update(DiagnosticDataId id) {
        storedVal_ = val_;
    }

    ValueT& value() {
        return val_;
    }

    const ValueT& value() const {
        return val_;
    }

    operator ValueT() const {
        return storedVal_;
    }

private:
    ValueT val_, storedVal_;
};

template<template<typename ConcurrencyT> class DiagnosticDataT, typename ConcurrencyT>
void testIntegerDiagnosticData(DiagService& diag) {
    using IntType = AbstractIntegerDiagnosticData::IntType;
    using DiagnosticData = DiagnosticDataT<ConcurrencyT>;

    DiagnosticData d(1);
    diag.start();

    SECTION("operator=(IntType)") {
        CHECK(&(d = 1) == &d);
        IntType val = -1;
        CHECK(AbstractIntegerDiagnosticData::get(1, val) == 0);
        CHECK(val == 1);
        CHECK(&(d = 2) == &d);
        CHECK(AbstractIntegerDiagnosticData::get(1, val) == 0);
        CHECK(val == 2);
    }

    SECTION("operator IntType()") {
        d = 1;
        IntType val = d;
        CHECK(val == 1);
        d = 2;
        val = d;
        CHECK(val == 2);
    }

    SECTION("operator++()") {
        CHECK(++d == 1);
        CHECK(++d == 2);
        CHECK(d == 2);
    }

    SECTION("operator++(int)") {
        CHECK(d++ == 0);
        CHECK(d++ == 1);
        CHECK(d == 2);
    }

    SECTION("operator--()") {
        CHECK(--d == -1);
        CHECK(--d == -2);
        CHECK(d == -2);
    }

    SECTION("operator--(int)") {
        CHECK(d-- == 0);
        CHECK(d-- == -1);
        CHECK(d == -2);
    }

    SECTION("operator+=(IntType)") {
        CHECK((d += 1) == 1);
        CHECK((d += 2) == 3);
        CHECK(d == 3);
    }

    SECTION("operator-=(IntType)") {
        CHECK((d -= 1) == -1);
        CHECK((d -= 2) == -3);
        CHECK(d == -3);
    }
}

template<template<typename StorageT, typename ConcurrencyT> class DiagnosticDataT, typename ConcurrencyT>
void testPersistentIntegerDiagnosticData(DiagService& diag) {
    using IntType = AbstractIntegerDiagnosticData::IntType;
    using DiagnosticData = DiagnosticDataT<NonPersistentStorage<IntType>, ConcurrencyT>;

    NonPersistentStorage<IntType> s;
    DiagnosticData d(s, 1);
    diag.start();

    SECTION("operator=(IntType)") {
        CHECK(&(d = 1) == &d);
        IntType val = -1;
        CHECK(AbstractIntegerDiagnosticData::get(1, val) == 0);
        CHECK(val == 1);
        CHECK(s == 1);
        CHECK(&(d = 2) == &d);
        CHECK(AbstractIntegerDiagnosticData::get(1, val) == 0);
        CHECK(val == 2);
        CHECK(s == 2);
    }

    SECTION("operator IntType()") {
        d = 1;
        IntType val = d;
        CHECK(val == 1);
        d = 2;
        val = d;
        CHECK(val == 2);
    }

    SECTION("operator++()") {
        CHECK(++d == 1);
        CHECK(++d == 2);
        CHECK(d == 2);
        CHECK(s == 2);
    }

    SECTION("operator++(int)") {
        CHECK(d++ == 0);
        CHECK(d++ == 1);
        CHECK(d == 2);
        CHECK(s == 2);
    }

    SECTION("operator--()") {
        CHECK(--d == -1);
        CHECK(--d == -2);
        CHECK(d == -2);
        CHECK(s == -2);
    }

    SECTION("operator--(int)") {
        CHECK(d-- == 0);
        CHECK(d-- == -1);
        CHECK(d == -2);
        CHECK(s == -2);
    }

    SECTION("operator+=(IntType)") {
        CHECK((d += 1) == 1);
        CHECK((d += 2) == 3);
        CHECK(d == 3);
        CHECK(s == 3);
    }

    SECTION("operator-=(IntType)") {
        CHECK((d -= 1) == -1);
        CHECK((d -= 2) == -3);
        CHECK(d == -3);
        CHECK(s == -3);
    }
}

template<template<typename EnumT, typename ConcurrencyT> class DiagnosticDataT, typename ConcurrencyT>
void testEnumDiagnosticData(DiagService& diag) {
    enum Enum {
        ZERO,
        ONE,
        TWO
    };

    using IntType = AbstractIntegerDiagnosticData::IntType;
    using DiagnosticData = DiagnosticDataT<Enum, ConcurrencyT>;

    DiagnosticData d(1, ZERO);
    diag.start();

    SECTION("operator=(EnumT)") {
        CHECK(&(d = ONE) == &d);
        IntType val = -1;
        CHECK(AbstractIntegerDiagnosticData::get(1, val) == 0);
        CHECK(val == ONE);
        CHECK(&(d = TWO) == &d);
        CHECK(AbstractIntegerDiagnosticData::get(1, val) == 0);
        CHECK(val == TWO);
    }

    SECTION("operator EnumT()") {
        d = ONE;
        Enum val = d;
        CHECK(val == ONE);
        d = TWO;
        val = d;
        CHECK(val == TWO);
    }
}

template<template<typename EnumT, typename StorageT, typename ConcurrencyT> class DiagnosticDataT, typename ConcurrencyT>
void testPersistentEnumDiagnosticData(DiagService& diag) {
    enum Enum {
        ZERO,
        ONE,
        TWO
    };

    using IntType = AbstractIntegerDiagnosticData::IntType;
    using DiagnosticData = DiagnosticDataT<Enum, NonPersistentStorage<IntType>, ConcurrencyT>;

    NonPersistentStorage<IntType> s;
    DiagnosticData d(s, 1, ZERO);
    diag.start();

    SECTION("operator=(EnumT)") {
        CHECK(&(d = ONE) == &d);
        IntType val = -1;
        CHECK(AbstractIntegerDiagnosticData::get(1, val) == 0);
        CHECK(val == ONE);
        CHECK(s == ONE);
        CHECK(&(d = TWO) == &d);
        CHECK(AbstractIntegerDiagnosticData::get(1, val) == 0);
        CHECK(val == TWO);
        CHECK(s == TWO);
    }

    SECTION("operator EnumT()") {
        d = ONE;
        Enum val = d;
        CHECK(val == ONE);
        d = TWO;
        val = d;
        CHECK(val == TWO);
    }
}

} // namespace

TEST_CASE("Service API") {
    DiagService diag;

    SECTION("diag_register_source()") {
        SECTION("fails if the service is not in its initial stopped state") {
            auto d1 = DiagSource(1);
            CHECK(diag_register_source(&d1, nullptr) == 0);
            diag.start();
            auto d2 = DiagSource(2);
            CHECK(diag_register_source(&d1, nullptr) == SYSTEM_ERROR_INVALID_STATE);
        }

        SECTION("registers a new data source") {
            auto d1 = DiagSource(1);
            CHECK(diag_register_source(&d1, nullptr) == 0);
            diag.start();
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
            diag.start();
            auto all = diag.sources();
            REQUIRE(all.size() == sources.size());
            for (const diag_source* d: all) {
                REQUIRE(ids.erase(d->id) == 1);
            }
        }

        SECTION("fails to register a data source with a duplicate ID") {
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

        SECTION("fails if the service is not started") {
            EnumSourcesCallback cb;
            size_t count = 0;
            CHECK(diag_enum_sources(cb.func(), &count, cb.data(), nullptr) == SYSTEM_ERROR_INVALID_STATE);
        }

        SECTION("enumerates all registered data sources") {
            diag.start();
            EnumSourcesCallback cb([=](const diag_source* d) {
                REQUIRE((d->id == d1.id() || d->id == d2.id() || d->id == d3.id()));
                return 0;
            });
            size_t count = 0;
            CHECK(diag_enum_sources(cb.func(), &count, cb.data(), nullptr) == 0);
            CHECK(count == 3);
        }

        SECTION("forwards callback errors to the caller code") {
            diag.start();
            EnumSourcesCallback cb([=](const diag_source* d) {
                return SYSTEM_ERROR_UNKNOWN;
            });
            size_t count = 0;
            CHECK(diag_enum_sources(cb.func(), &count, cb.data(), nullptr) == SYSTEM_ERROR_UNKNOWN);
        }

        SECTION("accepts NULL as the callback argument") {
            diag.start();
            size_t count = 0;
            CHECK(diag_enum_sources(nullptr /* callback */, &count, nullptr /* data */, nullptr) == 0);
            CHECK(count == 3);
        }

        SECTION("accepts NULL as the count argument") {
            diag.start();
            EnumSourcesCallback cb;
            CHECK(diag_enum_sources(cb.func(), nullptr /* count */, cb.data(), nullptr) == 0);
        }
    }

    SECTION("diag_get_source()") {
        auto d1 = DiagSource(1).add();
        auto d2 = DiagSource(2).add();
        auto d3 = DiagSource(3).add();

        SECTION("fails if the service is not started") {
            const diag_source* d = nullptr;
            CHECK(diag_get_source(1, &d, nullptr) == SYSTEM_ERROR_INVALID_STATE);
        }

        SECTION("returns a data source handle for a given ID") {
            diag.start();
            const diag_source* d = nullptr;
            CHECK(diag_get_source(d2.id(), &d, nullptr) == 0);
            REQUIRE(d != nullptr);
            CHECK(d->id == d2.id());
            CHECK(diag_get_source(4, &d, nullptr) == SYSTEM_ERROR_NOT_FOUND);
        }

        SECTION("accepts NULL as the handle argument") {
            diag.start();
            CHECK(diag_get_source(d3.id(), nullptr /* src */, nullptr) == 0);
            CHECK(diag_get_source(4, nullptr, nullptr) == SYSTEM_ERROR_NOT_FOUND);
        }
    }

    SECTION("diag_command()") {
        SECTION("can be used to start the diagnostics service") {
            CHECK(diag_command(DIAG_SERVICE_CMD_START, nullptr, nullptr) == 0);
        }
    }
}

TEST_CASE("Wiring API") {
    DiagService diag;

    SECTION("AbstractDiagnosticData") {
        SECTION("get()") {
            auto d1 = DiagSource(1).type(DIAG_TYPE_INT).get([](GetData d) {
                return d.setInt(1234);
            }).add();
            auto d2 = DiagSource(2).type(DIAG_TYPE_INT).get([](GetData) {
                return SYSTEM_ERROR_UNKNOWN;
            }).add();

            diag.start();

            SECTION("can access a data source by ID") {
                int32_t val = 0;
                size_t size = sizeof(val);
                CHECK(AbstractDiagnosticData::get(1, &val, size) == 0);
                CHECK(val == 1234);
                CHECK(size == sizeof(val));
            }

            SECTION("can access a data source via handle") {
                int32_t val = 0;
                size_t size = sizeof(val);
                CHECK(AbstractDiagnosticData::get(&d1, &val, size) == 0);
                CHECK(val == 1234);
                CHECK(size == sizeof(val));
            }

            SECTION("accepts NULL as the data argument") {
                size_t size = 0;
                CHECK(AbstractDiagnosticData::get(1, nullptr /* data */, size) == 0);
                CHECK(size == sizeof(int32_t));
            }

            SECTION("forwards getter errors to the caller code") {
                int32_t val = 0;
                size_t size = sizeof(val);
                CHECK(AbstractDiagnosticData::get(&d2, &val, size) == SYSTEM_ERROR_UNKNOWN);
            }
        }
    }

    SECTION("AbstractIntegerDiagnosticData") {
        SECTION("get()") {
            auto d1 = DiagSource(1).type(DIAG_TYPE_INT).get([](GetData d) {
                return d.setInt(1234);
            }).add();
            auto d2 = DiagSource(2).type(DIAG_TYPE_INT).get([](GetData) {
                return SYSTEM_ERROR_UNKNOWN;
            }).add();

            diag.start();

            SECTION("can access a data source by ID") {
                int32_t val = 0;
                CHECK(AbstractIntegerDiagnosticData::get(1, val) == 0);
                CHECK(val == 1234);
            }

            SECTION("can access a data source via handle") {
                int32_t val = 0;
                CHECK(AbstractIntegerDiagnosticData::get(&d1, val) == 0);
                CHECK(val == 1234);
            }

            SECTION("forwards getter errors to the caller code") {
                int32_t val = 0;
                CHECK(AbstractIntegerDiagnosticData::get(&d2, val) == SYSTEM_ERROR_UNKNOWN);
            }
        }
    }

    SECTION("IntegerDiagnosticData") {
        testIntegerDiagnosticData<IntegerDiagnosticData, NoConcurrency>(diag);
        testIntegerDiagnosticData<IntegerDiagnosticData, AtomicConcurrency>(diag);
    }

    SECTION("PersistentIntegerDiagnosticData") {
        testPersistentIntegerDiagnosticData<PersistentIntegerDiagnosticData, NoConcurrency>(diag);
        // testPersistentIntegerDiagnosticData<PersistentIntegerDiagnosticData, AtomicConcurrency>(diag);
    }

    SECTION("EnumDiagnosticData") {
        testEnumDiagnosticData<EnumDiagnosticData, NoConcurrency>(diag);
        testEnumDiagnosticData<EnumDiagnosticData, AtomicConcurrency>(diag);
    }

    SECTION("PersistentEnumDiagnosticData") {
        testPersistentEnumDiagnosticData<PersistentEnumDiagnosticData, NoConcurrency>(diag);
        // testPersistentEnumDiagnosticData<PersistentEnumDiagnosticData, AtomicConcurrency>(diag);
    }
}
