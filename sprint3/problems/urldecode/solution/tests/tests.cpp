#define BOOST_TEST_MODULE urlencode tests
#include <boost/test/unit_test.hpp>

#include "../src/urldecode.h"

BOOST_AUTO_TEST_CASE(UrlDecode_tests) {
    using namespace std::literals;

    // Пустая строка
    BOOST_TEST(UrlDecode(""sv) == ""s);
    
    // Строка без %-последовательностей
    BOOST_TEST(UrlDecode("Hello World"sv) == "Hello World"s);
    BOOST_TEST(UrlDecode("simple+string"sv) == "simple string"s);
    BOOST_TEST(UrlDecode("no+spaces"sv) == "no spaces"s);
    
    // Строка с валидными %-последовательностями в разном регистре
    BOOST_TEST(UrlDecode("Hello%20World"sv) == "Hello World"s);
    BOOST_TEST(UrlDecode("Hello%20World%21"sv) == "Hello World!"s);
    BOOST_TEST(UrlDecode("%2F"sv) == "/"s);
    BOOST_TEST(UrlDecode("%2f"sv) == "/"s);
    BOOST_TEST(UrlDecode("%3A%2F%2F"sv) == "://"s);
    BOOST_TEST(UrlDecode("Hello%2BWorld"sv) == "Hello+World"s);
    BOOST_TEST(UrlDecode("Hello%20World+%21"sv) == "Hello World !"s);
    BOOST_TEST(UrlDecode("%41%42%43"sv) == "ABC"s);
    BOOST_TEST(UrlDecode("%61%62%63"sv) == "abc"s);
    BOOST_TEST(UrlDecode("100%25"sv) == "100%"s);
    BOOST_TEST(UrlDecode("%E2%82%AC"sv) == "€"s); // Euro sign in UTF-8
    
    // Строка с символом +
    BOOST_TEST(UrlDecode("+"sv) == " "s);
    BOOST_TEST(UrlDecode("a+b+c"sv) == "a b c"s);
    BOOST_TEST(UrlDecode("+a+b+"sv) == " a b "s);
    
    // Строка с невалидными %-последовательностями
    BOOST_CHECK_THROW(UrlDecode("%"sv), std::invalid_argument);
    BOOST_CHECK_THROW(UrlDecode("%2"sv), std::invalid_argument);
    BOOST_CHECK_THROW(UrlDecode("%GG"sv), std::invalid_argument);
    BOOST_CHECK_THROW(UrlDecode("%2G"sv), std::invalid_argument);
    BOOST_CHECK_THROW(UrlDecode("%G2"sv), std::invalid_argument);
    
    // Строка с неполными %-последовательностями
    BOOST_CHECK_THROW(UrlDecode("abc%"sv), std::invalid_argument);
    BOOST_CHECK_THROW(UrlDecode("abc%2"sv), std::invalid_argument);
    BOOST_CHECK_THROW(UrlDecode("%2"sv), std::invalid_argument);
    
    // Смешанные случаи
    BOOST_TEST(UrlDecode("Hello%20World%21+How+are+you%3F"sv) == "Hello World! How are you?"s);
    BOOST_TEST(UrlDecode("Unreserved%2D_.~"sv) == "Unreserved-_.~"s);
    BOOST_TEST(UrlDecode("Reserved%21%23%24%26%27%28%29%2A%2B%2C%2F%3A%3B%3D%3F%40%5B%5D"sv) 
               == "Reserved!#$&'()*+,/:;=?@[]"s);
}
