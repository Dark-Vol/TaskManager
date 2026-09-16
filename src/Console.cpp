#include "taskmanager/Console.hpp"

#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#endif

namespace
{
    bool g_colorsEnabled = false;

    bool stdoutIsTerminal()
    {
#ifdef _WIN32
        return _isatty(_fileno(stdout)) != 0;
#else
        return isatty(fileno(stdout)) != 0;
#endif
    }

    bool colorsDisabledByEnvironment()
    {
        const char* noColor = std::getenv("NO_COLOR");
        return noColor != nullptr && noColor[0] != '\0';
    }

    bool enableVirtualTerminal()
    {
#ifdef _WIN32
        HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
        if (handle == INVALID_HANDLE_VALUE)
        {
            return false;
        }

        DWORD mode = 0;
        if (GetConsoleMode(handle, &mode) == 0)
        {
            return false;
        }

        return SetConsoleMode(handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
#else
        return true;
#endif
    }

    const char* codeFor(console::Color color)
    {
        switch (color)
        {
        case console::Color::Red:
            return "\x1b[31m";
        case console::Color::Green:
            return "\x1b[32m";
        case console::Color::Yellow:
            return "\x1b[33m";
        case console::Color::Cyan:
            return "\x1b[36m";
        case console::Color::Gray:
            return "\x1b[90m";
        case console::Color::Bold:
            return "\x1b[1m";
        case console::Color::None:
            break;
        }

        return "";
    }
}

namespace console
{
    void initialize(bool userWantsColor)
    {
        g_colorsEnabled = userWantsColor && stdoutIsTerminal() &&
                          !colorsDisabledByEnvironment() && enableVirtualTerminal();
    }

    bool colorsEnabled()
    {
        return g_colorsEnabled;
    }

    std::string colorize(const std::string& text, Color color)
    {
        if (!g_colorsEnabled || color == Color::None || text.empty())
        {
            return text;
        }

        return std::string(codeFor(color)) + text + "\x1b[0m";
    }

    std::string cell(const std::string& text, std::size_t width, Color color)
    {
        std::string result = colorize(text, color);

        if (text.size() < width)
        {
            result.append(width - text.size(), ' ');
        }

        return result;
    }
}
