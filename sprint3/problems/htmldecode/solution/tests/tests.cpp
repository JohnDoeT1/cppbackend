#include <catch2/catch_test_macros.hpp>

#include "../src/htmldecode.h"

using namespace std::literals;

TEST_CASE("Text without mnemonics", "[HtmlDecode]") {
    CHECK(HtmlDecode(""sv) == ""s);
    CHECK(HtmlDecode("hello"sv) == "hello"s);
    CHECK(HtmlDecode("abc123 !@#"sv) == "abc123 !@#"s);
    CHECK(HtmlDecode("Johnson&Johnson"sv) == "Johnson&Johnson"s);
}

TEST_CASE("Single mnemonics, lowercase", "[HtmlDecode]") {
    CHECK(HtmlDecode("&lt;"sv)   == "<"s);
    CHECK(HtmlDecode("&gt;"sv)   == ">"s);
    CHECK(HtmlDecode("&amp;"sv)  == "&"s);
    CHECK(HtmlDecode("&apos;"sv) == "'"s);
    CHECK(HtmlDecode("&quot;"sv) == "\""s);
}

TEST_CASE("Single mnemonics, uppercase", "[HtmlDecode]") {
    CHECK(HtmlDecode("&LT;"sv)   == "<"s);
    CHECK(HtmlDecode("&GT;"sv)   == ">"s);
    CHECK(HtmlDecode("&AMP;"sv)  == "&"s);
    CHECK(HtmlDecode("&APOS;"sv) == "'"s);
    CHECK(HtmlDecode("&QUOT;"sv) == "\""s);
}

TEST_CASE("Mnemonics without trailing semicolon", "[HtmlDecode]") {
    CHECK(HtmlDecode("&lt"sv)   == "<"s);
    CHECK(HtmlDecode("&gt"sv)   == ">"s);
    CHECK(HtmlDecode("&amp"sv)  == "&"s);
    CHECK(HtmlDecode("&apos"sv) == "'"s);
    CHECK(HtmlDecode("&quot"sv) == "\""s);
}

TEST_CASE("Mixed case is not a mnemonic", "[HtmlDecode]") {
    CHECK(HtmlDecode("&Lt;"sv)   == "&Lt;"s);
    CHECK(HtmlDecode("&lT;"sv)   == "&lT;"s);
    CHECK(HtmlDecode("&aPos;"sv) == "&aPos;"s);
    CHECK(HtmlDecode("&AmP;"sv)  == "&AmP;"s);
    CHECK(HtmlDecode("&QuoT;"sv) == "&QuoT;"s);
}

TEST_CASE("Mnemonics at start, middle and end", "[HtmlDecode]") {
    CHECK(HtmlDecode("&lt;tag&gt;"sv)     == "<tag>"s);
    CHECK(HtmlDecode("a&amp;b"sv)         == "a&b"s);
    CHECK(HtmlDecode("value &lt;"sv)      == "value <"s);
    CHECK(HtmlDecode("x &lt; y &gt; z"sv) == "x < y > z"s);
}

TEST_CASE("Example M&M's from the statement", "[HtmlDecode]") {
    CHECK(HtmlDecode("M&amp;M&APOSs"sv)  == "M&M's"s);
    CHECK(HtmlDecode("M&amp;M&apos;s"sv) == "M&M's"s);
}

TEST_CASE("Johnson&Johnson in all spellings", "[HtmlDecode]") {
    const std::string expected = "Johnson&Johnson"s;
    CHECK(HtmlDecode("Johnson&Johnson"sv)     == expected);
    CHECK(HtmlDecode("Johnson&amp;Johnson"sv) == expected);
    CHECK(HtmlDecode("Johnson&ampJohnson"sv)  == expected);
    CHECK(HtmlDecode("Johnson&AMP;Johnson"sv) == expected);
    CHECK(HtmlDecode("Johnson&AMPJohnson"sv)  == expected);
}

TEST_CASE("Incomplete and unknown mnemonics", "[HtmlDecode]") {
    CHECK(HtmlDecode("&abracadabra"sv) == "&abracadabra"s);
    CHECK(HtmlDecode("&"sv)            == "&"s);
    CHECK(HtmlDecode("&l"sv)           == "&l"s);
    CHECK(HtmlDecode("&l;"sv)          == "&l;"s);
    CHECK(HtmlDecode("&llt;"sv)        == "&llt;"s);
    CHECK(HtmlDecode("&am"sv)          == "&am"s);
}

TEST_CASE("No repeated decoding", "[HtmlDecode]") {
    // &amp;lt; сначала даёт &lt;, но повторно декодировать уже нельзя
    CHECK(HtmlDecode("&amp;lt;"sv)  == "&lt;"s);
    CHECK(HtmlDecode("&amp;lt"sv)   == "&lt"s);
    CHECK(HtmlDecode("&amp;amp;"sv) == "&amp;"s);
}

TEST_CASE("Mnemonics inside HTML-like text", "[HtmlDecode]") {
    CHECK(HtmlDecode("<a href=\"page?x=1&amp;y=2\">link</a>"sv)
          == "<a href=\"page?x=1&y=2\">link</a>"s);
    CHECK(HtmlDecode("&lt;b&gt;bold&lt;/b&gt;"sv) == "<b>bold</b>"s);
}
