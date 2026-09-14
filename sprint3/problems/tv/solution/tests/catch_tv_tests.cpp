#include <catch2/catch_test_macros.hpp>
#include <iostream>

#include "../src/tv.h"

namespace Catch {

template <>
struct StringMaker<std::nullopt_t> {
    static std::string convert(std::nullopt_t) { return "nullopt"; }
};

template <typename T>
struct StringMaker<std::optional<T>> {
    static std::string convert(const std::optional<T>& opt_value) {
        if (opt_value) return StringMaker<T>::convert(*opt_value);
        return StringMaker<std::nullopt_t>::convert(std::nullopt);
    }
};

}  // namespace Catch

SCENARIO("TV", "[TV]") {
    GIVEN("A TV") {
        TV tv;

        SECTION("Initially it is off and doesn't show any channel") {
            CHECK(!tv.IsTurnedOn());
            CHECK(!tv.GetChannel().has_value());
        }

        WHEN("it is turned off") {
            REQUIRE(!tv.IsTurnedOn());

            THEN("it can't select any channel") {
                CHECK_THROWS_AS(tv.SelectChannel(10), std::logic_error);
                CHECK(tv.GetChannel() == std::nullopt);
                tv.TurnOn();
                CHECK(tv.GetChannel() == 1);
            }

            THEN("it can't select last viewed channel") {
                CHECK_THROWS_AS(tv.SelectLastViewedChannel(), std::logic_error);
            }
        }

        WHEN("it is turned on first time") {
            tv.TurnOn();

            THEN("it is turned on and shows channel #1") {
                CHECK(tv.IsTurnedOn());
                CHECK(tv.GetChannel() == 1);

                AND_WHEN("it is turned off") {
                    tv.TurnOff();

                    THEN("it is turned off and doesn't show any channel") {
                        CHECK(!tv.IsTurnedOn());
                        CHECK(tv.GetChannel() == std::nullopt);
                    }
                }
            }

            AND_THEN("it can select channel from 1 to 99") {
                for (int ch = TV::MIN_CHANNEL; ch <= TV::MAX_CHANNEL; ++ch) {
                    tv.SelectChannel(ch);
                    CHECK(tv.GetChannel() == ch);
                }
            }

            AND_THEN("it throws out_of_range for channels below 1") {
                CHECK_THROWS_AS(tv.SelectChannel(0), std::out_of_range);
                CHECK_THROWS_AS(tv.SelectChannel(-5), std::out_of_range);
                CHECK(tv.GetChannel() == 1);  // канал не изменился
            }

            AND_THEN("it throws out_of_range for channels above 99") {
                CHECK_THROWS_AS(tv.SelectChannel(100), std::out_of_range);
                CHECK_THROWS_AS(tv.SelectChannel(1000), std::out_of_range);
                CHECK(tv.GetChannel() == 1);
            }

            AND_THEN("selecting the same channel does nothing") {
                tv.SelectChannel(5);
                tv.SelectChannel(5);  // не должно изменить last_viewed
                tv.SelectChannel(7);
                tv.SelectLastViewedChannel();
                // Если бы повторный SelectChannel(5) изменил last_viewed,
                // то здесь оказалось бы 5, а не 1.
                CHECK(tv.GetChannel() == 5);
            }

            AND_THEN("SelectLastViewedChannel switches between two channels") {
                tv.SelectChannel(3);
                tv.SelectChannel(8);
                tv.SelectLastViewedChannel();
                CHECK(tv.GetChannel() == 3);
                tv.SelectLastViewedChannel();
                CHECK(tv.GetChannel() == 8);
                tv.SelectLastViewedChannel();
                CHECK(tv.GetChannel() == 3);
            }

            AND_THEN("channel is preserved after turning off and on") {
                tv.SelectChannel(42);
                tv.TurnOff();
                tv.TurnOn();
                CHECK(tv.GetChannel() == 42);
            }
        }
    }
}
