#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>

#include <string>
#include <vector>

#include "taskmanager/Encoding.hpp"
#endif

#include "taskmanager/CLI.hpp"

int main(int argc, char* argv[])
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // Windows hands plain argv to the program in the local ANSI code page, so
    // non-ASCII titles have to be taken from the wide command line instead.
    int wideArgc = 0;
    wchar_t** wideArgv = CommandLineToArgvW(GetCommandLineW(), &wideArgc);

    if (wideArgv != nullptr)
    {
        std::vector<std::string> arguments;
        arguments.reserve(static_cast<std::size_t>(wideArgc));

        for (int i = 0; i < wideArgc; ++i)
        {
            arguments.push_back(toUtf8(wideArgv[i]));
        }

        LocalFree(wideArgv);

        std::vector<char*> utf8Argv;
        utf8Argv.reserve(arguments.size() + 1);

        for (std::string& argument : arguments)
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
