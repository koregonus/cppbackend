#include <catch2/catch_test_macros.hpp>

#include "../src/htmldecode.h"

using namespace std::literals;

TEST_CASE("Text without mnemonics", "[HtmlDecode]") {
    CHECK(HtmlDecode(""sv) == ""s);
    CHECK(HtmlDecode("hello"sv) == "hello"s);
}

TEST_CASE("Text with single mnemonic", "[HtmlDecode]") {
    CHECK(HtmlDecode("hello&amp"sv) == "hello&"s);
}

TEST_CASE("Text with single mnemonic uppercase", "[HtmlDecode]") {
    CHECK(HtmlDecode("hello&AMP"sv) == "hello&"s);
}

TEST_CASE("Text with single mnemonic mix case", "[HtmlDecode]") {
    CHECK(HtmlDecode("hello&AMp&qUoT"sv) == "hello&AMp&qUoT"s);
}

TEST_CASE("Text with mnemonic not ended", "[HtmlDecode]") {
    CHECK(HtmlDecode("hello&am&am"sv) == "hello&am&am"s);
}

TEST_CASE("Text with single mnemonic and ;", "[HtmlDecode]") {
    CHECK(HtmlDecode("hello&amp;"sv) == "hello&"s);
}

TEST_CASE("Text with couple mnemonic mix ;", "[HtmlDecode]") {
    CHECK(HtmlDecode("hello&amp&lt;"sv) == "hello&<"s);
}

TEST_CASE("Text &amp;lt;;", "[HtmlDecode]") {
    CHECK(HtmlDecode("&amp;lt;"sv) == "&lt;"s);
}



// Напишите недостающие тесты самостоятельно
