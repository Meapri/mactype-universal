#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "modern_cpp.h"

namespace {

using namespace string_utils;

// Test fixture for string utilities
class StringUtilsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Common setup if needed
    }

    void TearDown() override {
        // Common cleanup if needed
    }
};

// Test cases for UTF-8 to UTF-16 conversion
TEST_F(StringUtilsTest, Utf8ToUtf16_EmptyString) {
    std::string empty_utf8;
    std::wstring result = utf8_to_utf16(empty_utf8);
    EXPECT_TRUE(result.empty());
}

TEST_F(StringUtilsTest, Utf8ToUtf16_ASCII) {
    std::string ascii = "Hello World";
    std::wstring result = utf8_to_utf16(ascii);
    std::wstring expected = L"Hello World";
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilsTest, Utf8ToUtf16_UTF8Characters) {
    std::string utf8 = "Hello 世界";
    std::wstring result = utf8_to_utf16(utf8);
    // The result should not be empty and should contain the characters
    EXPECT_FALSE(result.empty());
    EXPECT_EQ(result.length(), 8); // "Hello " (6) + "世界" (2)
}

// Test cases for UTF-16 to UTF-8 conversion
TEST_F(StringUtilsTest, Utf16ToUtf8_EmptyString) {
    std::wstring empty_utf16;
    std::string result = utf16_to_utf8(empty_utf16);
    EXPECT_TRUE(result.empty());
}

TEST_F(StringUtilsTest, Utf16ToUtf8_ASCII) {
    std::wstring ascii = L"Hello World";
    std::string result = utf16_to_utf8(ascii);
    std::string expected = "Hello World";
    EXPECT_EQ(result, expected);
}

// Test cases for string case conversion
TEST_F(StringUtilsTest, ToLower_EmptyString) {
    wstring_view empty;
    std::wstring result = to_lower(empty);
    EXPECT_TRUE(result.empty());
}

TEST_F(StringUtilsTest, ToLower_MixedCase) {
    wstring_view mixed = L"HeLLo WoRLD";
    std::wstring result = to_lower(mixed);
    std::wstring expected = L"hello world";
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilsTest, ToLower_AlreadyLower) {
    wstring_view lower = L"hello world";
    std::wstring result = to_lower(lower);
    EXPECT_EQ(result, lower);
}

TEST_F(StringUtilsTest, ToUpper_EmptyString) {
    wstring_view empty;
    std::wstring result = to_upper(empty);
    EXPECT_TRUE(result.empty());
}

TEST_F(StringUtilsTest, ToUpper_MixedCase) {
    wstring_view mixed = L"HeLLo WoRLD";
    std::wstring result = to_upper(mixed);
    std::wstring expected = L"HELLO WORLD";
    EXPECT_EQ(result, expected);
}

// Test cases for string splitting
TEST_F(StringUtilsTest, Split_EmptyString) {
    std::string empty;
    auto result = split(empty, ",");
    EXPECT_TRUE(result.empty());
}

TEST_F(StringUtilsTest, Split_NoDelimiter) {
    std::string no_delim = "hello";
    auto result = split(no_delim, ",");
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], "hello");
}

TEST_F(StringUtilsTest, Split_SingleDelimiter) {
    std::string single = "hello,world";
    auto result = split(single, ",");
    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0], "hello");
    EXPECT_EQ(result[1], "world");
}

TEST_F(StringUtilsTest, Split_MultipleDelimiters) {
    std::string multiple = "a,b,c,d";
    auto result = split(multiple, ",");
    ASSERT_EQ(result.size(), 4);
    EXPECT_EQ(result[0], "a");
    EXPECT_EQ(result[1], "b");
    EXPECT_EQ(result[2], "c");
    EXPECT_EQ(result[3], "d");
}

TEST_F(StringUtilsTest, Split_EmptyTokens) {
    std::string empty_tokens = "a,,b,";
    auto result = split(empty_tokens, ",");
    ASSERT_EQ(result.size(), 4);
    EXPECT_EQ(result[0], "a");
    EXPECT_EQ(result[1], "");
    EXPECT_EQ(result[2], "b");
    EXPECT_EQ(result[3], "");
}

TEST_F(StringUtilsTest, Split_WideString) {
    std::wstring wide = L"hello,world,测试";
    auto result = split(wide, L",");
    ASSERT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], L"hello");
    EXPECT_EQ(result[1], L"world");
    EXPECT_EQ(result[2], L"测试");
}

} // namespace
