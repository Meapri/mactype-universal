#include "common.h"

std::wstring to_utf16le(const std::wstring& input) {
	std::wstring utf16_string;
	int len = input.length();
	char* content = (char*)input.c_str();
	utf16_string.reserve(len);
	for (size_t i = 0; i < len; i += 2) {
		char16_t code_unit = (static_cast<char16_t>(content[i]) << 8) |
			static_cast<char16_t>(content[i + 1]);
		utf16_string.push_back(code_unit);
	}
	return utf16_string;
}

std::wstring to_wide_string(const std::string& input)
{
	if (input.empty()) return {};
	
	int len = MultiByteToWideChar(CP_UTF8, 0, input.c_str(), -1, nullptr, 0);
	if (len == 0) return {};
	
	std::wstring result(len - 1, 0); // -1 to exclude null terminator
	MultiByteToWideChar(CP_UTF8, 0, input.c_str(), -1, &result[0], len);
	return result;
}

// convert wstring to string 
std::string to_byte_string(const std::wstring& input)
{
	if (input.empty()) return {};
	
	int len = WideCharToMultiByte(CP_UTF8, 0, input.c_str(), -1, nullptr, 0, nullptr, nullptr);
	if (len == 0) return {};
	
	std::string result(len - 1, 0); // -1 to exclude null terminator
	WideCharToMultiByte(CP_UTF8, 0, input.c_str(), -1, &result[0], len, nullptr, nullptr);
	return result;
}

std::wstring to_lower_case(std::wstring_view str) {
    std::wstring lowered(str.begin(), str.end());
    std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](wchar_t c) {
        return std::towlower(c);
    });
    return lowered;
}
