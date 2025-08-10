#include "modern_cpp.h"
#include <Windows.h>
#include <algorithm>
#include <sstream>
#include <cctype>
#include <locale>

namespace string_utils {

std::wstring utf8_to_utf16(string_view utf8_str)
{
    if (utf8_str.empty()) {
        return {};
    }

    // MultiByteToWideChar를 사용한 안전한 변환
    const int utf8_length = static_cast<int>(utf8_str.length());
    const int utf16_length = MultiByteToWideChar(
        CP_UTF8, 0, utf8_str.data(), utf8_length, nullptr, 0);

    if (utf16_length == 0) {
        return {};
    }

    std::wstring result(utf16_length, L'\0');
    MultiByteToWideChar(
        CP_UTF8, 0, utf8_str.data(), utf8_length, result.data(), utf16_length);

    return result;
}

std::string utf16_to_utf8(wstring_view utf16_str)
{
    if (utf16_str.empty()) {
        return {};
    }

    // WideCharToMultiByte를 사용한 안전한 변환
    const int utf16_length = static_cast<int>(utf16_str.length());
    const int utf8_length = WideCharToMultiByte(
        CP_UTF8, 0, utf16_str.data(), utf16_length, nullptr, 0, nullptr, nullptr);

    if (utf8_length == 0) {
        return {};
    }

    std::string result(utf8_length, '\0');
    WideCharToMultiByte(
        CP_UTF8, 0, utf16_str.data(), utf16_length, result.data(), utf8_length, nullptr, nullptr);

    return result;
}

std::wstring to_lower(wstring_view str)
{
    std::wstring result{ str };
    std::transform(result.begin(), result.end(), result.begin(),
        [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
    return result;
}

std::wstring to_upper(wstring_view str)
{
    std::wstring result{ str };
    std::transform(result.begin(), result.end(), result.begin(),
        [](wchar_t c) { return static_cast<wchar_t>(std::towupper(c)); });
    return result;
}

std::vector<std::wstring> split(wstring_view str, wstring_view delimiter)
{
    std::vector<std::wstring> result;
    
    if (str.empty()) {
        return result;
    }

    size_t start = 0;
    size_t end = str.find(delimiter);

    while (end != std::wstring_view::npos) {
        result.emplace_back(str.substr(start, end - start));
        start = end + delimiter.length();
        end = str.find(delimiter, start);
    }

    result.emplace_back(str.substr(start));
    return result;
}

template<typename Container>
std::wstring join(const Container& strings, wstring_view separator)
{
    if (strings.empty()) {
        return {};
    }

    std::wostringstream oss;
    auto it = strings.begin();
    oss << *it;
    ++it;

    for (; it != strings.end(); ++it) {
        oss << separator << *it;
    }

    return oss.str();
}

// 명시적 인스턴스화
template std::wstring join<std::vector<std::wstring>>(
    const std::vector<std::wstring>&, wstring_view);

bool contains_ignore_case(wstring_view str, wstring_view substr)
{
    const auto str_lower = to_lower(str);
    const auto substr_lower = to_lower(substr);
    return str_lower.find(substr_lower) != std::wstring::npos;
}

std::wstring trim_left(wstring_view str)
{
    const auto first_non_space = std::find_if_not(str.begin(), str.end(),
        [](wchar_t c) { return std::iswspace(c); });
    
    return std::wstring{ first_non_space, str.end() };
}

std::wstring trim_right(wstring_view str)
{
    const auto last_non_space = std::find_if_not(str.rbegin(), str.rend(),
        [](wchar_t c) { return std::iswspace(c); }).base();
    
    return std::wstring{ str.begin(), last_non_space };
}

std::wstring trim(wstring_view str)
{
    return trim_left(trim_right(str));
}

} // namespace string_utils
