#pragma once 
#include <string>
#ifdef WIN32
#include <windows.h>
#endif // WIN32

static inline  std::string to_utf8(const std::string& str) {
#ifdef WIN32
	// Преобразуем строку из локальной кодировки (CP_ACP) в std::wstring
	int wide_size = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, nullptr, 0);
	std::wstring wide_str(wide_size, L'\0');
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, wide_str.data(), wide_size);

	// Преобразуем std::wstring в UTF-8
	int utf8_size = WideCharToMultiByte(CP_UTF8, 0, wide_str.c_str(), -1, nullptr, 0, nullptr, nullptr);
	std::string utf8_str(utf8_size, '\0');
	WideCharToMultiByte(CP_UTF8, 0, wide_str.c_str(), -1, utf8_str.data(), utf8_size, nullptr, nullptr);
#endif

	return utf8_str;
}