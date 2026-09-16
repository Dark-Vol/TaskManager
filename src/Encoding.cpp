#include "taskmanager/Encoding.hpp"

#ifdef _WIN32

#include <windows.h>

std::wstring toWide(const std::string& utf8)
{
    if (utf8.empty())
    {
        return {};
    }

    const int size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(),
                                         static_cast<int>(utf8.size()), nullptr, 0);
    if (size <= 0)
    {
        return {};
    }

    std::wstring result(static_cast<std::size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()),
                        result.data(), size);
    return result;
}

std::string toUtf8(const std::wstring& wide)
{
    if (wide.empty())
    {
        return {};
    }

    const int size = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(),
                                         static_cast<int>(wide.size()), nullptr, 0,
                                         nullptr, nullptr);
    if (size <= 0)
    {
        return {};
    }

    std::string result(static_cast<std::size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.size()),
                        result.data(), size, nullptr, nullptr);
    return result;
}

std::string toUtf8(const wchar_t* wide)
{
    if (wide == nullptr)
    {
        return {};
    }

    return toUtf8(std::wstring(wide));
}

#endif
