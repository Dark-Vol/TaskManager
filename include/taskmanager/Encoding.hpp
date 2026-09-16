#pragma once

#include <string>

#ifdef _WIN32

// The program keeps every string in UTF-8, while the Windows API works with
// UTF-16. These two helpers are the only place where the conversion happens.
std::wstring toWide(const std::string& utf8);
std::string toUtf8(const std::wstring& wide);
std::string toUtf8(const wchar_t* wide);

#endif
