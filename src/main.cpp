#ifdef _WIN32
#include <windows.h>
#endif

#include "taskmanager/CLI.hpp"

int main(int argc, char* argv[])
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    CLI cli;
    return cli.run(argc, argv);
}
