#define BOOST_TEST_MODULE urlencode tests
#include <boost/test/unit_test.hpp>

#include "../src/urldecode.h"

bool pred(std::invalid_argument const& ec)
{
    return true;
}

BOOST_AUTO_TEST_CASE(UrlDecode_tests) {
    using namespace std::literals;

    BOOST_TEST(UrlDecode(""sv) == ""s);
    BOOST_TEST(UrlDecode("Hello+world"sv) == "Hello world"s);
    BOOST_TEST(UrlDecode("Hello%2fworld"sv) == "Hello/world"s);
    BOOST_TEST(UrlDecode("Hello%2Fworld"sv) == "Hello/world"s);
    BOOST_CHECK_EXCEPTION(UrlDecode("Hello%#"sv), std::invalid_argument, pred);
    BOOST_CHECK_EXCEPTION(UrlDecode("Hello%2H"sv), std::invalid_argument, pred);
    BOOST_CHECK_EXCEPTION(UrlDecode("Hello%HE"sv), std::invalid_argument, pred);
    BOOST_CHECK_EXCEPTION(UrlDecode("Hello%2"sv), std::invalid_argument, pred);
    BOOST_CHECK_EXCEPTION(UrlDecode("Hello world"sv), std::invalid_argument, pred);

    // Напишите остальные тесты для функции UrlDecode самостоятельно
}