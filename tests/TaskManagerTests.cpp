#include "taskmanager/TaskManager.hpp"

#include <filesystem>
#include <iostream>
#include <string>

namespace
{
    int g_passed = 0;
    int g_failed = 0;

    void check(bool condition, const std::string& name, const std::string& expected)
    {
        if (condition)
        {
            std::cout << "Test: " << name << "\nExpected: " << expected
                      << "\nResult: PASS\n\n";
            ++g_passed;
        }
        else
        {
            std::cout << "Test: " << name << "\nExpected: " << expected
                      << "\nResult: FAIL\n\n";
            ++g_failed;
        }
    }

    std::filesystem::path makeTempPath()
    {
        return std::filesystem::temp_directory_path() / "taskmanager_unit_tests.txt";
    }
}

int main()
{
    const std::filesystem::path storagePath = makeTempPath();
    std::filesystem::remove(storagePath);

    {
        TaskManager manager(storagePath.string());
        const int id = manager.addTask("Learn C++", "Learn classes", Priority::High);

        check(id == 1, "add task", "task id = 1");
        check(manager.getTasks().size() == 1, "add task count", "task count = 1");
        check(manager.findTask(1) != nullptr, "findTask()", "task is found");
        check(manager.findTask(1)->getPriority() == Priority::High,
              "default/explicit priority",
              "priority = HIGH");
        check(manager.findTask(1)->getStatus() == Status::Todo,
              "new task status",
              "status = TODO");
    }

    {
        TaskManager reloaded(storagePath.string());
        check(reloaded.getTasks().size() == 1, "load after restart", "task count = 1");
        check(reloaded.findTask(1) != nullptr && reloaded.findTask(1)->getTitle() == "Learn C++",
              "persist title",
              "title = Learn C++");
    }

    {
        TaskManager manager(storagePath.string());
        check(manager.startTask(1) == OperationResult::Success,
              "start task",
              "status becomes IN_PROGRESS");
        check(manager.findTask(1)->getStatus() == Status::InProgress,
              "start task status",
              "status = IN_PROGRESS");
        check(manager.startTask(1) == OperationResult::InvalidTransition,
              "start already started task",
              "invalid transition");
        check(manager.completeTask(1) == OperationResult::Success,
              "complete task",
              "status = DONE");
        check(manager.findTask(1)->getStatus() == Status::Done,
              "complete task status",
              "status = DONE");
        check(manager.completeTask(1) == OperationResult::InvalidTransition,
              "complete already done task",
              "invalid transition");
    }

    {
        TaskManager manager(storagePath.string());
        const int secondId = manager.addTask("Read book");
        check(secondId == 2, "next id after load", "task id = 2");
        check(manager.findTask(2)->getPriority() == Priority::Medium,
              "default priority",
              "priority = MEDIUM");
        check(manager.updateTask(2, "Read article", {}, Priority::Low) == OperationResult::Success,
              "update task",
              "title and priority updated");
        check(manager.findTask(2)->getTitle() == "Read article",
              "update title",
              "title = Read article");
        check(manager.searchTasks("article").size() == 1,
              "search tasks",
              "1 match");
        check(manager.removeTask(9999) == OperationResult::NotFound,
              "delete nonexistent task",
              "false / NotFound");
        check(manager.removeTask(2) == OperationResult::Success,
              "delete task",
              "task removed");
        check(manager.findTask(2) == nullptr, "find deleted task", "nullptr");
    }

    {
        TaskManager manager(storagePath.string());
        check(manager.startTask(1) == OperationResult::InvalidTransition,
              "cannot start DONE task",
              "invalid transition");
        check(manager.addTask("") == -1, "reject empty title", "id = -1");
    }

    std::filesystem::remove(storagePath);

    std::cout << "Passed: " << g_passed << '\n'
              << "Failed: " << g_failed << '\n';

    return g_failed == 0 ? 0 : 1;
}
