#include "spark_wiring_flags.h"

#include "tools/catch.h"

#include <limits>

TEST_CASE("particle::Flags<TagT, ValueT>") {
    struct FlagType; // Tag type
    typedef particle::Flags<FlagType> Flags; // Using default value type

    const Flags::FlagType FLAG_1(0x01);
    const Flags::FlagType FLAG_2(0x02);
    const Flags::FlagType FLAG_NONE(0x00);
    const Flags::FlagType FLAG_ALL(std::numeric_limits<Flags::ValueType>::max());

    SECTION("Flags()") {
        // Flags()
        Flags f1;
        CHECK(f1.value() == 0x00);
        // Flags(Flags::FlagType)
        Flags f2(FLAG_1);
        CHECK(f2.value() == 0x01);
    }

    SECTION("operator|()") {
        // operator|(Flags::FlagType, Flags::FlagType)
        Flags f1;
        f1 = FLAG_NONE | FLAG_1;
        CHECK(f1.value() == 0x01);
        f1 = FLAG_1 | FLAG_1;
        CHECK(f1.value() == 0x01);
        f1 = FLAG_1 | FLAG_2;
        CHECK(f1.value() == 0x03);
        // operator|(Flags::FlagType, Flags)
        Flags f2;
        f2 = FLAG_NONE | Flags(FLAG_1);
        CHECK(f2.value() == 0x01);
        f2 = FLAG_1 | Flags(FLAG_1);
        CHECK(f2.value() == 0x01);
        f2 = FLAG_1 | Flags(FLAG_2);
        CHECK(f2.value() == 0x03);
        // operator|(Flags, Flags::FlagType)
        Flags f3;
        f3 = Flags(FLAG_NONE) | FLAG_1;
        CHECK(f3.value() == 0x01);
        f3 = Flags(FLAG_1) | FLAG_1;
        CHECK(f3.value() == 0x01);
        f3 = Flags(FLAG_1) | FLAG_2;
        CHECK(f3.value() == 0x03);
        // operator|(Flags, Flags)
        Flags f4;
        f4 = Flags(FLAG_NONE) | Flags(FLAG_1);
        CHECK(f4.value() == 0x01);
        f4 = Flags(FLAG_1) | Flags(FLAG_1);
        CHECK(f4.value() == 0x01);
        f4 = Flags(FLAG_1) | Flags(FLAG_2);
        CHECK(f4.value() == 0x03);
    }

    SECTION("operator|=()") {
        // operator|=(Flags::FlagType)
        Flags f1;
        f1 |= FLAG_1;
        CHECK(f1.value() == 0x01);
        f1 |= FLAG_1;
        CHECK(f1.value() == 0x01);
        f1 |= FLAG_2;
        CHECK(f1.value() == 0x03);
        // operator|=(Flags)
        Flags f2;
        f2 |= Flags(FLAG_1);
        CHECK(f2.value() == 0x01);
        f2 |= Flags(FLAG_1);
        CHECK(f2.value() == 0x01);
        f2 |= Flags(FLAG_2);
        CHECK(f2.value() == 0x03);
    }

    SECTION("operator&()") {
        // operator&(Flags::FlagType, Flags)
        Flags f1;
        f1 = FLAG_NONE & Flags(FLAG_1);
        CHECK(f1.value() == 0x00);
        f1 = FLAG_1 & Flags(FLAG_1);
        CHECK(f1.value() == 0x01);
        f1 = FLAG_1 & Flags(FLAG_2);
        CHECK(f1.value() == 0x00);
        // operator&(Flags, Flags::FlagType)
        Flags f2;
        f2 = Flags(FLAG_NONE) & FLAG_1;
        CHECK(f2.value() == 0x00);
        f2 = Flags(FLAG_1) & FLAG_1;
        CHECK(f2.value() == 0x01);
        f2 = Flags(FLAG_1) & FLAG_2;
        CHECK(f2.value() == 0x00);
        // operator&(Flags, Flags)
        Flags f3;
        f3 = Flags(FLAG_NONE) & Flags(FLAG_1);
        CHECK(f3.value() == 0x00);
        f3 = Flags(FLAG_1) & Flags(FLAG_1);
        CHECK(f3.value() == 0x01);
        f3 = Flags(FLAG_1) & Flags(FLAG_2);
        CHECK(f3.value() == 0x00);
    }

    SECTION("operator&=()") {
        // operator&=(Flags::FlagType)
        Flags f1(FLAG_1 | FLAG_2);
        f1 &= FLAG_1;
        CHECK(f1.value() == 0x01);
        f1 &= FLAG_1;
        CHECK(f1.value() == 0x01);
        f1 &= FLAG_2;
        CHECK(f1.value() == 0x00);
        // operator&=(Flags)
        Flags f2(FLAG_1 | FLAG_2);
        f2 &= Flags(FLAG_1);
        CHECK(f2.value() == 0x01);
        f2 &= Flags(FLAG_1);
        CHECK(f2.value() == 0x01);
        f2 &= Flags(FLAG_2);
        CHECK(f2.value() == 0x00);
    }

    SECTION("operator^()") {
        // operator^(Flags::FlagType, Flags)
        Flags f2;
        f2 = FLAG_NONE ^ Flags(FLAG_1);
        CHECK(f2.value() == 0x01);
        f2 = FLAG_1 ^ Flags(FLAG_1);
        CHECK(f2.value() == 0x00);
        f2 = FLAG_1 ^ Flags(FLAG_2);
        CHECK(f2.value() == 0x03);
        // operator^(Flags, Flags::FlagType)
        Flags f3;
        f3 = Flags(FLAG_NONE) ^ FLAG_1;
        CHECK(f3.value() == 0x01);
        f3 = Flags(FLAG_1) ^ FLAG_1;
        CHECK(f3.value() == 0x00);
        f3 = Flags(FLAG_1) ^ FLAG_2;
        CHECK(f3.value() == 0x03);
        // operator^(Flags, Flags)
        Flags f4;
        f4 = Flags(FLAG_NONE) ^ Flags(FLAG_1);
        CHECK(f4.value() == 0x01);
        f4 = Flags(FLAG_1) ^ Flags(FLAG_1);
        CHECK(f4.value() == 0x00);
        f4 = Flags(FLAG_1) ^ Flags(FLAG_2);
        CHECK(f4.value() == 0x03);
    }

    SECTION("operator^=()") {
        // operator^=(Flags::FlagType)
        Flags f1;
        f1 ^= FLAG_1;
        CHECK(f1.value() == 0x01);
        f1 ^= FLAG_1;
        CHECK(f1.value() == 0x00);
        f1 ^= FLAG_2;
        CHECK(f1.value() == 0x02);
        // operator^=(Flags)
        Flags f2;
        f2 ^= Flags(FLAG_1);
        CHECK(f2.value() == 0x01);
        f2 ^= Flags(FLAG_1);
        CHECK(f2.value() == 0x00);
        f2 ^= Flags(FLAG_2);
        CHECK(f2.value() == 0x02);
    }

    SECTION("operator~()") {
        Flags f1 = ~Flags(FLAG_NONE);
        CHECK(f1.value() == (Flags::ValueType)FLAG_ALL);
        Flags f2 = ~Flags(FLAG_ALL);
        CHECK(f2.value() == (Flags::ValueType)FLAG_NONE);
    }

    SECTION("operator=()") {
        // operator=(Flags::FlagType)
        Flags f1;
        f1 = FLAG_1;
        CHECK(f1.value() == 0x01);
    }

    SECTION("operator bool()") {
        Flags f1(FLAG_1);
        CHECK(f1);
        Flags f2;
        CHECK_FALSE(f2);
    }

    SECTION("operator!()") {
        Flags f1(FLAG_1);
        CHECK_FALSE(!f1);
        Flags f2;
        CHECK(!f2);
    }

    SECTION("flag cannot be implicitly converted to its value type") {
        struct Test {
            static bool func(Flags) {
                return true;
            }

            static bool func(Flags::ValueType) {
                return false;
            }
        };
        CHECK(Test::func(FLAG_1));
    }
}
