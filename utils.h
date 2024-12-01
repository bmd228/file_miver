#pragma once 
#include <string>
#ifdef WIN32
#include <windows.h>
#endif // WIN32
#include <random>

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
static size_t generateRandom() {
	// Инициализаци генератора случайных чисел
	std::random_device rd;              // Источник случайности
	std::mt19937_64 gen(rd());          // Генератор случайных чисел (Mersenne Twister, 64-битный)

	// ќпределение распределени¤ с диапазоном дл¤ заполнени¤ всего size_t
	std::uniform_int_distribution<size_t> dist(
		std::numeric_limits<size_t>::min(),
		std::numeric_limits<size_t>::max()
	);

	// √енераци¤ случайного числа типа size_t, полностью заполненного
	size_t random_number = dist(gen);
	return random_number;
}