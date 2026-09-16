#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>

#include <string>
#include <vector>
#endif

#include "taskmanager/CLI.hpp"

#ifdef _WIN32
namespace
{
    // Windows hands plain argv to the program in the local ANSI code page, so
    // non-ASCII titles have to be taken from the wide command line instead.
    std::string toUtf8(const wchar_t *text)
    {
        const int size = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
        if (size <= 1)
        {
            return {};
        }

        std::string result(static_cast<std::size_t>(size - 1), '\0');
        WideCharToMultiByte(CP_UTF8, 0, text, -1, result.data(), size, nullptr, nullptr);
        return result;
    }
}
#endif

int main(int argc, char *argv[])
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    int wideArgc = 0;
    wchar_t **wideArgv = CommandLineToArgvW(GetCommandLineW(), &wideArgc);

    if (wideArgv != nullptr)
    {
        std::vector<std::string> arguments;
        arguments.reserve(static_cast<std::size_t>(wideArgc));

        for (int i = 0; i < wideArgc; ++i)
        {
            arguments.push_back(toUtf8(wideArgv[i]));
        }

        LocalFree(wideArgv);

        std::vector<char *> utf8Argv;
        utf8Argv.reserve(arguments.size() + 1);

        for (std::string &argument : arguments)
        {
            utf8Argv.push_back(argument.data());
        }

        utf8Argv.push_back(nullptr);

        CLI cli;
        return cli.run(wideArgc, utf8Argv.data());
    }
#endif

    CLI cli;
    return cli.run(argc, argv);
}
