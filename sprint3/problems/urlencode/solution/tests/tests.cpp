#include <gtest/gtest.h>

#include "../src/urlencode.h"

using namespace std::literals;

TEST(UrlEncodeTestSuite, EmptyString) {
    EXPECT_EQ(UrlEncode(""sv), ""s);
}

TEST(UrlEncodeTestSuite, OrdinaryCharsAreNotEncoded) {
    EXPECT_EQ(UrlEncode("hello"sv), "hello"s);
    EXPECT_EQ(UrlEncode("HelloWorld123"sv), "HelloWorld123"s);
    EXPECT_EQ(UrlEncode("abc-_.~"sv), "abc-_.~"s);
    EXPECT_EQ(UrlEncode("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.~"sv), 
              "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.~"s);
}

TEST(UrlEncodeTestSuite, SpaceIsEncodedAsPlus) {
    EXPECT_EQ(UrlEncode(" "sv), "+"s);
    EXPECT_EQ(UrlEncode("Hello World"sv), "Hello+World"s);
    EXPECT_EQ(UrlEncode("  multiple   spaces  "sv), "++multiple+++spaces++"s);
}

TEST(UrlEncodeTestSuite, ReservedCharsAreEncoded) {
    EXPECT_EQ(UrlEncode("!"sv), "%21"s);
    EXPECT_EQ(UrlEncode("#"sv), "%23"s);
    EXPECT_EQ(UrlEncode("$"sv), "%24"s);
    EXPECT_EQ(UrlEncode("&"sv), "%26"s);
    EXPECT_EQ(UrlEncode("'"sv), "%27"s);
    EXPECT_EQ(UrlEncode("("sv), "%28"s);
    EXPECT_EQ(UrlEncode(")"sv), "%29"s);
    EXPECT_EQ(UrlEncode("*"sv), "%2A"s);
    EXPECT_EQ(UrlEncode("+"sv), "%2B"s);
    EXPECT_EQ(UrlEncode(","sv), "%2C"s);
    EXPECT_EQ(UrlEncode("/"sv), "%2F"s);
    EXPECT_EQ(UrlEncode(":"sv), "%3A"s);
    EXPECT_EQ(UrlEncode(";"sv), "%3B"s);
    EXPECT_EQ(UrlEncode("="sv), "%3D"s);
    EXPECT_EQ(UrlEncode("?"sv), "%3F"s);
    EXPECT_EQ(UrlEncode("@"sv), "%40"s);
    EXPECT_EQ(UrlEncode("["sv), "%5B"s);
    EXPECT_EQ(UrlEncode("]"sv), "%5D"s);
    
    // Все зарезервированные символы вместе
    EXPECT_EQ(UrlEncode("!#$&'()*+,/:;=?@[]"sv), 
              "%21%23%24%26%27%28%29%2A%2B%2C%2F%3A%3B%3D%3F%40%5B%5D"s);
}

TEST(UrlEncodeTestSuite, ControlCharsAreEncoded) {
    EXPECT_EQ(UrlEncode("\x01"sv), "%01"s);
    EXPECT_EQ(UrlEncode("\x1F"sv), "%1F"s);
    EXPECT_EQ(UrlEncode("\x00"sv), "%00"s);
    EXPECT_EQ(UrlEncode("\x0A"sv), "%0A"s);
    EXPECT_EQ(UrlEncode("\x0D"sv), "%0D"s);
    
    // Символ с кодом 31 (максимальный control character)
    EXPECT_EQ(UrlEncode("\x1F"sv), "%1F"s);
}

TEST(UrlEncodeTestSuite, HighCharsAreEncoded) {
    // Символ с кодом 128
    EXPECT_EQ(UrlEncode("\x80"sv), "%80"s);
    
    // Символ с кодом 255
    EXPECT_EQ(UrlEncode("\xFF"sv), "%FF"s);
    
    // Символ с кодом 200
    EXPECT_EQ(UrlEncode("\xC8"sv), "%C8"s);
}

TEST(UrlEncodeTestSuite, MixedStrings) {
    EXPECT_EQ(UrlEncode("Hello World!"sv), "Hello+World%21"s);
    EXPECT_EQ(UrlEncode("abc*"sv), "abc%2A"s);
    EXPECT_EQ(UrlEncode("test@example.com"sv), "test%40example.com"s);
    EXPECT_EQ(UrlEncode("path/to/file"sv), "path%2Fto%2Ffile"s);
    EXPECT_EQ(UrlEncode("query?key=value&lang=en"sv), 
              "query%3Fkey%3Dvalue%26lang%3Den"s);
    EXPECT_EQ(UrlEncode("Hello World! How are you?"sv), 
              "Hello+World%21+How+are+you%3F"s);
}
