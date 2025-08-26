/**
 * @file test_settings.cpp
 * @brief Unit tests for MacType settings system
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <filesystem>

#include "modern_cpp.h"
#include "settings.h"

// Mock for Windows API calls in settings
class MockWindowsAPI {
public:
    MOCK_METHOD(DWORD, GetModuleFileNameW, (HMODULE, LPWSTR, DWORD), ());
    MOCK_METHOD(BOOL, GetCurrentDirectoryW, (DWORD, LPWSTR), ());
};

class SettingsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary directory for test files
        testDir_ = std::filesystem::temp_directory_path() / "mactype_test";
        std::filesystem::create_directories(testDir_);

        // Create test INI file
        testIniPath_ = testDir_ / "test.ini";
        CreateTestIniFile();
    }

    void TearDown() override {
        // Clean up test files
        std::filesystem::remove_all(testDir_);
    }

    void CreateTestIniFile() {
        std::ofstream iniFile(testIniPath_);
        iniFile << "[General]\n";
        iniFile << "TestValue=42\n";
        iniFile << "TestString=Hello World\n";
        iniFile << "[FreeType]\n";
        iniFile << "FontSize=12\n";
        iniFile.close();
    }

    std::filesystem::path testDir_;
    std::filesystem::path testIniPath_;
};

TEST_F(SettingsTest, StringConversions) {
    using namespace string_conversions;

    // Test UTF-8 to UTF-16 conversion
    std::string utf8 = "Hello 世界";
    std::wstring utf16 = to_wstring(utf8);
    EXPECT_FALSE(utf16.empty());
    EXPECT_EQ(utf16.length(), 8); // "Hello " (6) + "世界" (2)

    // Test UTF-16 to UTF-8 conversion
    std::string backToUtf8 = to_utf8(utf16);
    EXPECT_EQ(backToUtf8, utf8);
}

TEST_F(SettingsTest, ConfigConstants) {
    using namespace config_constants;

    // Test configuration constants
    EXPECT_EQ(GENERAL_SECTION, "General");
    EXPECT_EQ(FREETYPE_SECTION, "FreeType");
    EXPECT_EQ(DIRECTWRITE_SECTION, "DirectWrite");

    // Test value ranges
    EXPECT_GE(HINTING_MAX, HINTING_MIN);
    EXPECT_GE(AAMODE_MAX, AAMODE_MIN);
    EXPECT_GE(GAMMAVALUE_MAX, GAMMAVALUE_MIN);
}

TEST_F(SettingsTest, LegacyCompatStringTokenizer) {
    using namespace legacy_compat;

    StringTokenizer tokenizer;

    // Test parsing
    int count = tokenizer.Parse("apple,banana,cherry");
    EXPECT_EQ(count, 3);

    EXPECT_STREQ(tokenizer.GetArgument(0), "apple");
    EXPECT_STREQ(tokenizer.GetArgument(1), "banana");
    EXPECT_STREQ(tokenizer.GetArgument(2), "cherry");
    EXPECT_EQ(tokenizer.GetArgument(3), nullptr);

    // Test reset
    tokenizer.Reset();
    EXPECT_EQ(tokenizer.GetCount(), 0);
}

TEST_F(SettingsTest, ModernStringTokenizer) {
    using namespace legacy_compat;

    StringTokenizer tokenizer;

    // Test modern string_view parsing
    int count = tokenizer.Parse("  spaced  ,  tokens  ,  here  "sv);
    EXPECT_EQ(count, 3);

    EXPECT_STREQ(tokenizer.GetArgument(0), "spaced");
    EXPECT_STREQ(tokenizer.GetArgument(1), "tokens");
    EXPECT_STREQ(tokenizer.GetArgument(2), "here");
}

TEST_F(SettingsTest, SplitStringUtility) {
    using namespace legacy_compat;

    // Test basic splitting
    auto result = SplitString("a,b,c,d");
    ASSERT_EQ(result.size(), 4);
    EXPECT_EQ(result[0], 0); // "a" -> 0 (invalid number)
    EXPECT_EQ(result[1], 0); // "b" -> 0
    EXPECT_EQ(result[2], 0); // "c" -> 0
    EXPECT_EQ(result[3], 0); // "d" -> 0

    // Test numeric splitting
    auto numericResult = SplitString("1,2,3,4");
    ASSERT_EQ(numericResult.size(), 4);
    EXPECT_EQ(numericResult[0], 1);
    EXPECT_EQ(numericResult[1], 2);
    EXPECT_EQ(numericResult[2], 3);
    EXPECT_EQ(numericResult[3], 4);
}

TEST_F(SettingsTest, EmptyAndInvalidInputs) {
    using namespace legacy_compat;

    StringTokenizer tokenizer;

    // Test empty string
    int count = tokenizer.Parse("");
    EXPECT_EQ(count, 1); // Empty string creates one empty token

    // Test nullptr
    count = tokenizer.Parse(nullptr);
    EXPECT_EQ(count, 0);
}

// Integration test for settings loading
TEST_F(SettingsTest, SettingsLoadingIntegration) {
    // This test would require mocking Windows API calls
    // For now, test the utility functions used by settings

    using namespace string_utils;

    // Test path utilities
    std::filesystem::path testPath = "C:\\Program Files\\MacType\\settings.ini";
    EXPECT_TRUE(testPath.has_extension());

    // Test string splitting used in settings
    auto sections = split("General,FreeType,DirectWrite", ",");
    ASSERT_EQ(sections.size(), 3);
    EXPECT_EQ(sections[0], "General");
    EXPECT_EQ(sections[1], "FreeType");
    EXPECT_EQ(sections[2], "DirectWrite");
}
