#include "taskmanager/TaskManager.hpp"

#include <filesystem>
#include <fstream>
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
        check(manager.reopenTask(1) == OperationResult::Success,
              "reopen DONE task",
              "status = TODO");
        check(manager.findTask(1)->getStatus() == Status::Todo,
              "reopen task status",
              "status = TODO");
        check(manager.reopenTask(1) == OperationResult::InvalidTransition,
              "reopen TODO task",
              "invalid transition");
    }

    std::filesystem::remove(storagePath);

    {
        TaskManager manager(storagePath.string());

        TaskDraft draft;
        draft.title = "Write report";
        draft.priority = Priority::High;
        draft.tags = splitTags("work, urgent, work");

        std::chrono::system_clock::time_point due{};
        parseDate("2020-01-01", due);
        draft.dueDate = due;

        const int id = manager.addTask(draft);

        check(id == 1, "add task with draft", "task id = 1");
        check(manager.findTask(id)->getTags().size() == 2,
              "tags are deduplicated",
              "2 tags");
        check(manager.findTask(id)->hasTag("URGENT"),
              "hasTag() ignores case",
              "tag is found");
        check(manager.findTask(id)->getDueDate().has_value(),
              "due date is stored",
              "due date is set");
        check(manager.findTask(id)->isOverdue(std::chrono::system_clock::now()),
              "past due date is overdue",
              "overdue = true");
        check(manager.searchTasks("urgent").size() == 1,
              "search by tag",
              "1 match");
    }

    {
        TaskManager reloaded(storagePath.string());
        const Task* task = reloaded.findTask(1);

        check(task != nullptr && task->getTags().size() == 2,
              "tags survive a restart",
              "2 tags");
        check(task != nullptr && task->getDueDate().has_value(),
              "due date survives a restart",
              "due date is set");

        TaskPatch patch;
        patch.clearDueDate = true;
        check(reloaded.updateTask(1, patch) == OperationResult::Success,
              "clear due date",
              "due date is removed");
        check(!reloaded.findTask(1)->getDueDate().has_value(),
              "due date is empty",
              "no due date");

        const Statistics stats = reloaded.getStatistics();
        check(stats.total == 1 && stats.todo == 1 && stats.high == 1 && stats.overdue == 0,
              "statistics",
              "1 task, TODO, HIGH, no overdue");
    }

    std::filesystem::remove(storagePath);

    {
        // A file written by the first version has 6 fields and must still load.
        std::ofstream legacy(storagePath.string(), std::ios::trunc);
        legacy << "# id|title|description|status|priority|createdAt\n"
               << "4|Legacy task|Old format|IN_PROGRESS|LOW|2026-09-15\n";
        legacy.close();

        TaskManager manager(storagePath.string());
        const Task* task = manager.findTask(4);

        check(task != nullptr, "load legacy 6-field record", "task is found");
        check(task != nullptr && task->getStatus() == Status::InProgress,
              "legacy status",
              "status = IN_PROGRESS");
        check(task != nullptr && task->getTags().empty(),
              "legacy record has no tags",
              "0 tags");
        check(manager.addTask("Next task") == 5,
              "next id after legacy load",
              "task id = 5");
    }

    std::filesystem::remove(storagePath);

    std::cout << "Passed: " << g_passed << '\n'
              << "Failed: " << g_failed << '\n';

    return g_failed == 0 ? 0 : 1;
}
