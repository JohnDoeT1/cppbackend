#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <sstream>

#include "../src/controller.h"

SCENARIO("Controller", "[Controller]") {
    using namespace std::literals;
    GIVEN("Controller and TV") {
        TV tv;
        std::istringstream input;
        std::ostringstream output;
        Menu menu{input, output};
        Controller controller{tv, menu};

        auto run_menu_command = [&menu, &input](std::string command) {
            input.str(std::move(command));
            input.clear();
            menu.Run();
        };
        auto expect_output = [&output](std::string_view expected) {
            CHECK(output.str() == std::string{expected});
        };
        auto expect_extra_arguments_error = [&expect_output](std::string_view command) {
            expect_output("Error: the "s.append(command).append(
                " command does not require any arguments\n"sv));
        };
        auto expect_empty_output = [&expect_output] { expect_output({}); };

        WHEN("The TV is turned off") {
            AND_WHEN("Info command is entered without arguments") {
                run_menu_command("Info"s);
                THEN("output contains info that TV is turned off") {
                    expect_output("TV is turned off\n"s);
                }
            }
            AND_WHEN("Info command is entered with some arguments") {
                run_menu_command("Info some extra arguments");
                THEN("Error message is printed") {
                    expect_extra_arguments_error("Info"s);
                }
            }
            AND_WHEN("Info command has trailing spaces") {
                run_menu_command("Info  "s);
                THEN("output contains information that TV is turned off") {
                    expect_output("TV is turned off\n"s);
                }
            }
            AND_WHEN("TurnOn command is entered without arguments") {
                run_menu_command("TurnOn"s);
                THEN("TV is turned on") {
                    CHECK(tv.IsTurnedOn());
                    expect_empty_output();
                }
            }
            AND_WHEN("TurnOn command is entered with some arguments") {
                run_menu_command("TurnOn some args"s);
                THEN("the error message is printed and TV is not turned on") {
                    CHECK(!tv.IsTurnedOn());
                    expect_extra_arguments_error("TurnOn"s);
                }
            }
            AND_WHEN("SelectChannel command is entered while TV is off") {
                run_menu_command("SelectChannel 10"s);
                THEN("error 'TV is turned off' is printed and channel is not changed") {
                    expect_output("TV is turned off\n"s);
                    CHECK(!tv.IsTurnedOn());
                }
            }
            AND_WHEN("SelectPreviousChannel command is entered while TV is off") {
                run_menu_command("SelectPreviousChannel"s);
                THEN("error 'TV is turned off' is printed") {
                    expect_output("TV is turned off\n"s);
                }
            }
            AND_WHEN("TurnOff command is entered while TV is already off") {
                run_menu_command("TurnOff"s);
                THEN("TV remains off and no error is printed") {
                    CHECK(!tv.IsTurnedOn());
                    expect_empty_output();
                }
            }
        }

        WHEN("The TV is turned on") {
            tv.TurnOn();
            AND_WHEN("TurnOff command is entered without arguments") {
                run_menu_command("TurnOff"s);
                THEN("TV is turned off") {
                    CHECK(!tv.IsTurnedOn());
                    expect_empty_output();
                }
            }
            AND_WHEN("TurnOff command is entered with some arguments") {
                run_menu_command("TurnOff some args");
                THEN("the error message is printed and TV is not turned off") {
                    CHECK(tv.IsTurnedOn());
                    expect_extra_arguments_error("TurnOff"s);
                }
            }
            AND_WHEN("Info command is entered without arguments") {
                tv.SelectChannel(12);
                run_menu_command("Info"s);
                THEN("current channel is printed") {
                    expect_output("TV is turned on\nChannel number is 12\n"s);
                }
            }
            AND_WHEN("SelectChannel command is entered with a valid channel") {
                run_menu_command("SelectChannel 7"s);
                THEN("the channel is selected and output is empty") {
                    CHECK(tv.GetChannel() == 7);
                    expect_empty_output();
                }
            }
            AND_WHEN("SelectChannel command is entered with a channel out of range") {
                run_menu_command("SelectChannel 100"s);
                THEN("'Channel is out of range' is printed") {
                    expect_output("Channel is out of range\n"s);
                    CHECK(tv.GetChannel() == 1);
                }
            }
            AND_WHEN("SelectChannel command is entered without a number") {
                run_menu_command("SelectChannel"s);
                THEN("'Invalid channel' is printed") {
                    expect_output("Invalid channel\n"s);
                }
            }
            AND_WHEN("SelectChannel command is entered with a non-numeric argument") {
                run_menu_command("SelectChannel abc"s);
                THEN("'Invalid channel' is printed") {
                    expect_output("Invalid channel\n"s);
                }
            }
            AND_WHEN("SelectPreviousChannel command is entered after selecting two channels") {
                tv.SelectChannel(3);
                tv.SelectChannel(8);
                run_menu_command("SelectPreviousChannel"s);
                THEN("previous channel becomes current and output is empty") {
                    CHECK(tv.GetChannel() == 3);
                    expect_empty_output();
                }
            }
        }
    }
}
