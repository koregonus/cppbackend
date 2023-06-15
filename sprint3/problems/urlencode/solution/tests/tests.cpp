#include <gtest/gtest.h>

#include "../src/urlencode.h"

using namespace std::literals;

TEST(UrlEncodeTestSuite, OrdinaryCharsAreNotEncoded) {
    EXPECT_EQ(UrlEncode("hello"sv), "hello"s);
}


TEST(UrlEncodeTestSuite, BackspaceEncoding) {
    EXPECT_EQ(UrlEncode("hello world"sv), "hello+world"s);
}

TEST(UrlEncodeTestSuite, Lower32Enc) {
    EXPECT_EQ(UrlEncode("helloworld\t"sv), "helloworld%09"s);
}


TEST(UrlEncodeTestSuite, SpecialSymbols) {
    std::string str("hello world!$");
    EXPECT_EQ(UrlEncode(str), "hello+world%21%24"s);
}

/* Напишите остальные тесты самостоятельно */
