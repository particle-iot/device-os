#include "spark_wiring_flags.h"

#include "tools/catch.h"

#include <limits>

namespace {

using namespace particle;

enum FlagUint {
    FLAG_1 = 0x01,
    FLAG_2 = 0x02,
    FLAG_NONE = 0x00,
    FLAG_ALL = std::numeric_limits<unsigned>::max()
};

enum class FlagUint8: uint8_t {
    FLAG_1 = 0x01,
    FLAG_2 = 0x02,
    FLAG_NONE = 0x00,
    FLAG_ALL = 0xff
};

enum class FlagUint64: uint64_t {
    FLAG_1 = 0x01,
    FLAG_2 = 0x02,
    FLAG_NONE = 0x00,
    FLAG_ALL = 0xffffffffffffffffull
};

PARTICLE_DEFINE_FLAG_OPERATORS(FlagUint)
PARTICLE_DEFINE_FLAG_OPERATORS(FlagUint8)
PARTICLE_DEFINE_FLAG_OPERATORS(FlagUint64)

template<typename FlagsT>
void testFlags() {
    using Flags = FlagsT;
    using Enum = typename Flags::EnumType;
    using Value = typename Flags::ValueType;

    SECTION("Flags()") {
        // Flags()
        Flags f1;
        CHECK(f1.value() == 0x00);
        // Flags(ValueType)
        Flags f2(0x01);
        CHECK(f2.value() == 0x01);
        // Flags(T)
        Flags f3(Enum::FLAG_1);
        CHECK(f3.value() == 0x01);
        // Flags(const Flags&)
        Flags f4(f3);
        CHECK(f4.value() == f3.value());
    }

    SECTION("operator|()") {
        // operator|(T, T)
        Flags f1;
        f1 = Enum::FLAG_NONE | Enum::FLAG_1;
        CHECK(f1.value() == 0x01);
        f1 = Enum::FLAG_1 | Enum::FLAG_1;
        CHECK(f1.value() == 0x01);
        f1 = Enum::FLAG_1 | Enum::FLAG_2;
        CHECK(f1.value() == 0x03);
        // operator|(T, Flags)
        Flags f2;
        f2 = Enum::FLAG_NONE | Flags(Enum::FLAG_1);
        CHECK(f2.value() == 0x01);
        f2 = Enum::FLAG_1 | Flags(Enum::FLAG_1);
        CHECK(f2.value() == 0x01);
        f2 = Enum::FLAG_1 | Flags(Enum::FLAG_2);
        CHECK(f2.value() == 0x03);
        // operator|(Flags, T)
        Flags f3;
        f3 = Flags(Enum::FLAG_NONE) | Enum::FLAG_1;
        CHECK(f3.value() == 0x01);
        f3 = Flags(Enum::FLAG_1) | Enum::FLAG_1;
        CHECK(f3.value() == 0x01);
        f3 = Flags(Enum::FLAG_1) | Enum::FLAG_2;
        CHECK(f3.value() == 0x03);
        // operator|(Flags, Flags)
        Flags f4;
        f4 = Flags(Enum::FLAG_NONE) | Flags(Enum::FLAG_1);
        CHECK(f4.value() == 0x01);
        f4 = Flags(Enum::FLAG_1) | Flags(Enum::FLAG_1);
        CHECK(f4.value() == 0x01);
        f4 = Flags(Enum::FLAG_1) | Flags(Enum::FLAG_2);
        CHECK(f4.value() == 0x03);
    }

    SECTION("operator|=()") {
        // operator|=(T)
        Flags f1;
        f1 |= Enum::FLAG_1;
        CHECK(f1.value() == 0x01);
        f1 |= Enum::FLAG_1;
        CHECK(f1.value() == 0x01);
        f1 |= Enum::FLAG_2;
        CHECK(f1.value() == 0x03);
        // operator|=(Flags)
        Flags f2;
        f2 |= Flags(Enum::FLAG_1);
        CHECK(f2.value() == 0x01);
        f2 |= Flags(Enum::FLAG_1);
        CHECK(f2.value() == 0x01);
        f2 |= Flags(Enum::FLAG_2);
        CHECK(f2.value() == 0x03);
    }

    SECTION("operator&()") {
        // operator&(T, Flags)
        Flags f1;
        f1 = Enum::FLAG_NONE & Flags(Enum::FLAG_1);
        CHECK(f1.value() == 0x00);
        f1 = Enum::FLAG_1 & Flags(Enum::FLAG_1);
        CHECK(f1.value() == 0x01);
        f1 = Enum::FLAG_1 & Flags(Enum::FLAG_2);
        CHECK(f1.value() == 0x00);
        // operator&(Flags, T)
        Flags f2;
        f2 = Flags(Enum::FLAG_NONE) & Enum::FLAG_1;
        CHECK(f2.value() == 0x00);
        f2 = Flags(Enum::FLAG_1) & Enum::FLAG_1;
        CHECK(f2.value() == 0x01);
        f2 = Flags(Enum::FLAG_1) & Enum::FLAG_2;
        CHECK(f2.value() == 0x00);
        // operator&(Flags, Flags)
        Flags f3;
        f3 = Flags(Enum::FLAG_NONE) & Flags(Enum::FLAG_1);
        CHECK(f3.value() == 0x00);
        f3 = Flags(Enum::FLAG_1) & Flags(Enum::FLAG_1);
        CHECK(f3.value() == 0x01);
        f3 = Flags(Enum::FLAG_1) & Flags(Enum::FLAG_2);
        CHECK(f3.value() == 0x00);
    }

    SECTION("operator&=()") {
        // operator&=(T)
        Flags f1(0x03);
        f1 &= Enum::FLAG_1;
        CHECK(f1.value() == 0x01);
        f1 &= Enum::FLAG_1;
        CHECK(f1.value() == 0x01);
        f1 &= Enum::FLAG_2;
        CHECK(f1.value() == 0x00);
        // operator&=(Flags)
        Flags f2(0x03);
        f2 &= Flags(Enum::FLAG_1);
        CHECK(f2.value() == 0x01);
        f2 &= Flags(Enum::FLAG_1);
        CHECK(f2.value() == 0x01);
        f2 &= Flags(Enum::FLAG_2);
        CHECK(f2.value() == 0x00);
    }

    SECTION("operator^()") {
        // operator^(T, Flags)
        Flags f2;
        f2 = Enum::FLAG_NONE ^ Flags(Enum::FLAG_1);
        CHECK(f2.value() == 0x01);
        f2 = Enum::FLAG_1 ^ Flags(Enum::FLAG_1);
        CHECK(f2.value() == 0x00);
        f2 = Enum::FLAG_1 ^ Flags(Enum::FLAG_2);
        CHECK(f2.value() == 0x03);
        // operator^(Flags, T)
        Flags f3;
        f3 = Flags(Enum::FLAG_NONE) ^ Enum::FLAG_1;
        CHECK(f3.value() == 0x01);
        f3 = Flags(Enum::FLAG_1) ^ Enum::FLAG_1;
        CHECK(f3.value() == 0x00);
        f3 = Flags(Enum::FLAG_1) ^ Enum::FLAG_2;
        CHECK(f3.value() == 0x03);
        // operator^(Flags, Flags)
        Flags f4;
        f4 = Flags(Enum::FLAG_NONE) ^ Flags(Enum::FLAG_1);
        CHECK(f4.value() == 0x01);
        f4 = Flags(Enum::FLAG_1) ^ Flags(Enum::FLAG_1);
        CHECK(f4.value() == 0x00);
        f4 = Flags(Enum::FLAG_1) ^ Flags(Enum::FLAG_2);
        CHECK(f4.value() == 0x03);
    }

    SECTION("operator^=()") {
        // operator^=(T)
        Flags f1;
        f1 ^= Enum::FLAG_1;
        CHECK(f1.value() == 0x01);
        f1 ^= Enum::FLAG_1;
        CHECK(f1.value() == 0x00);
        f1 ^= Enum::FLAG_2;
        CHECK(f1.value() == 0x02);
        // operator^=(Flags)
        Flags f2;
        f2 ^= Flags(Enum::FLAG_1);
        CHECK(f2.value() == 0x01);
        f2 ^= Flags(Enum::FLAG_1);
        CHECK(f2.value() == 0x00);
        f2 ^= Flags(Enum::FLAG_2);
        CHECK(f2.value() == 0x02);
    }

    SECTION("operator~()") {
        Flags f1 = ~Flags(Enum::FLAG_NONE);
        CHECK(f1.value() == (Value)Enum::FLAG_ALL);
        Flags f2 = ~Flags(Enum::FLAG_ALL);
        CHECK(f2.value() == (Value)Enum::FLAG_NONE);
    }

    SECTION("operator=()") {
        // operator=(T)
        Flags f1;
        f1 = Enum::FLAG_1;
        CHECK(f1.value() == 0x01);
        // operator=(Flags)
        Flags f2;
        f2 = Flags(Enum::FLAG_1);
        CHECK(f2.value() == 0x01);
        // operator=(ValueType)
        Flags f3;
        f3 = 0x01;
        CHECK(f3.value() == 0x01);
    }

    SECTION("operator ValueType()") {
        Flags f1(0x01);
        CHECK((Value)f1 == 0x01);
        CHECK(f1); // acts as implicit operator bool()
        Flags f2;
        CHECK((Value)f2 == 0x00);
        CHECK_FALSE(f2);
    }

    SECTION("operator!()") {
        Flags f1(0x01);
        CHECK_FALSE(!f1);
        Flags f2;
        CHECK(!f2);
    }

    SECTION("underlying type fits all flag values") {
        Flags f1(Enum::FLAG_ALL);
        CHECK(f1.value() == (Value)Enum::FLAG_ALL);
        Flags f2((Value)Enum::FLAG_ALL);
        CHECK(f2.value() == (Value)Enum::FLAG_ALL);
        Flags f3;
        f3 = (Value)Enum::FLAG_ALL;
        CHECK(f3.value() == (Value)Enum::FLAG_ALL);
    }
}

} // namespace

TEST_CASE("Flags<T>") {
    testFlags<Flags<FlagUint>>();
    testFlags<Flags<FlagUint8>>();
    testFlags<Flags<FlagUint64>>();
}
