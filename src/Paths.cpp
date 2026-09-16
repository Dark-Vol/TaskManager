#include "taskmanager/Paths.hpp"

#include <cstdlib>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#endif

namespace
{
    constexpr int kMaxLevelsUp = 6;

    std::filesystem::path executableDirectory()
    {
#ifdef _WIN32
        std::wstring buffer(MAX_PATH, L'\0');

        for (;;)
        {
            const DWORD written = GetModuleFileNameW(nullptr, buffer.data(),
                                                     static_cast<DWORD>(buffer.size()));
            if (written == 0)
            {
                return {};
            }

            if (written < buffer.size())
            {
                buffer.resize(written);
                break;
            }

            buffer.resize(buffer.size() * 2);
        }

        return std::filesystem::path(buffer).parent_path();
#else
        std::error_code error;
        const std::filesystem::path self = std::filesystem::read_symlink("/proc/self/exe", error);
        if (error)
        {
            return {};
        }

        return self.parent_path();
#endif
    }

    bool looksLikeProjectRoot(const std::filesystem::path& directory)
    {
        std::error_code error;

        if (std::filesystem::exists(directory / "CMakeLists.txt", error))
        {
            return true;
        }

        return std::filesystem::is_directory(directory / "data", error);
    }

    bool findProjectRoot(std::filesystem::path start, std::filesystem::path& out)
    {
        if (start.empty())
        {
            return false;
        }

        for (int level = 0; level < kMaxLevelsUp; ++level)
        {
            if (looksLikeProjectRoot(start))
            {
                out = start;
                return true;
            }

            const std::filesystem::path parent = start.parent_path();
            if (parent.empty() || parent == start)
            {
                return false;
            }

            start = parent;
        }

        return false;
    }
}

std::string resolveDataPath()
{
    if (const char* fromEnvironment = std::getenv("TASKMANAGER_DATA"))
    {
        if (fromEnvironment[0] != '\0')
        {
            return fromEnvironment;
        }
    }

    const std::filesystem::path exeDirectory = executableDirectory();
    std::filesystem::path root;

    if (findProjectRoot(exeDirectory, root))
    {
        return (root / "data" / "tasks.txt").string();
    }

    std::error_code error;
    if (findProjectRoot(std::filesystem::current_path(error), root) && !error)
    {
        return (root / "data" / "tasks.txt").string();
    }

    if (!exeDirectory.empty())
    {
        return (exeDirectory / "data" / "tasks.txt").string();
    }

    return "data/tasks.txt";
}
