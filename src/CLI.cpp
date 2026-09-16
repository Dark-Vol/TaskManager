#include "taskmanager/CLI.hpp"
#include "taskmanager/CommandParser.hpp"
#include "taskmanager/Console.hpp"
#include "taskmanager/Gui.hpp"
#include "taskmanager/Menu.hpp"
#include "taskmanager/Paths.hpp"
#include "taskmanager/TaskManager.hpp"
#include "taskmanager/View.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace
{
    std::vector<Task> filterTasks(std::vector<Task> tasks, const Command& command)
    {
        const auto now = std::chrono::system_clock::now();

        const auto drop = [&tasks](auto predicate) {
            tasks.erase(std::remove_if(tasks.begin(), tasks.end(), predicate), tasks.end());
        };

        if (command.statusFilter.has_value())
        {
            drop([&](const Task& task) { return task.getStatus() != *command.statusFilter; });
        }

        if (command.priorityFilter.has_value())
        {
            drop([&](const Task& task) { return task.getPriority() != *command.priorityFilter; });
        }

        if (!command.tagFilter.empty())
        {
            drop([&](const Task& task) { return !task.hasTag(command.tagFilter); });
        }

        if (command.overdueOnly)
        {
            drop([&](const Task& task) { return !task.isOverdue(now); });
        }

        return tasks;
    }

    void sortTasks(std::vector<Task>& tasks, const std::string& sortBy)
    {
        if (sortBy == "priority")
        {
            std::sort(tasks.begin(), tasks.end(),
                      [](const Task& left, const Task& right) {
                          if (priorityRank(left.getPriority()) != priorityRank(right.getPriority()))
                          {
                              return priorityRank(left.getPriority()) > priorityRank(right.getPriority());
                          }
                          return left.getId() < right.getId();
                      });
            return;
        }

        if (sortBy == "date")
        {
            std::sort(tasks.begin(), tasks.end(),
                      [](const Task& left, const Task& right) {
                          if (left.getCreatedAt() != right.getCreatedAt())
                          {
                              return left.getCreatedAt() > right.getCreatedAt();
                          }
                          return left.getId() > right.getId();
                      });
            return;
        }

        if (sortBy == "due")
        {
            // Tasks without a due date go last, so the nearest deadline is on top.
            std::sort(tasks.begin(), tasks.end(),
                      [](const Task& left, const Task& right) {
                          if (left.getDueDate().has_value() != right.getDueDate().has_value())
                          {
                              return left.getDueDate().has_value();
                          }

                          if (left.getDueDate().has_value() && *left.getDueDate() != *right.getDueDate())
                          {
                              return *left.getDueDate() < *right.getDueDate();
                          }

                          return left.getId() < right.getId();
                      });
            return;
        }

        if (sortBy == "title")
        {
            std::sort(tasks.begin(), tasks.end(),
                      [](const Task& left, const Task& right) {
                          if (left.getTitle() != right.getTitle())
                          {
                              return left.getTitle() < right.getTitle();
                          }
                          return left.getId() < right.getId();
                      });
            return;
        }

        std::sort(tasks.begin(), tasks.end(),
                  [](const Task& left, const Task& right) {
                      return left.getId() < right.getId();
                  });
    }

    int handleAdd(TaskManager& manager, const Command& command)
    {
        TaskDraft draft;
        draft.title = command.title;
        draft.description = command.hasDescription ? command.description : "";
        draft.priority = command.hasPriority ? command.priority : Priority::Medium;
        draft.dueDate = command.dueDate;

        if (command.tags.has_value())
        {
            draft.tags = *command.tags;
        }

        const int id = manager.addTask(draft);
        if (id < 0)
        {
            view::printError("Missing argument.");
            return 1;
        }

        view::printCreated(id);
        return 0;
    }

    int handleList(const TaskManager& manager, const Command& command)
    {
        std::vector<Task> tasks = filterTasks(manager.getTasks(), command);
        sortTasks(tasks, command.sortBy);
        view::printTaskList(tasks);
        return 0;
    }

    int handleShow(const TaskManager& manager, const Command& command)
    {
        const Task* task = manager.findTask(command.taskId);
        if (task == nullptr)
        {
            view::printNotFound(command.taskId);
            return 1;
        }

        view::printTaskDetails(*task);
        return 0;
    }

    int handleTransition(TaskManager& manager, const Command& command)
    {
        const Task* existing = manager.findTask(command.taskId);

        OperationResult result = OperationResult::NotFound;
        std::string action;
        std::string success;

        switch (command.type)
        {
        case CommandType::Start:
            result = manager.startTask(command.taskId);
            action = "started";
            success = " is now IN PROGRESS.";
            break;
        case CommandType::Done:
            result = manager.completeTask(command.taskId);
            action = "completed";
            success = " completed.";
            break;
        default:
            result = manager.reopenTask(command.taskId);
            action = "reopened";
            success = " is now TODO.";
            break;
        }

        if (result == OperationResult::NotFound)
        {
            view::printNotFound(command.taskId);
            return 1;
        }

        if (result == OperationResult::InvalidTransition)
        {
            view::printTransitionError(*existing, action);
            return 1;
        }

        view::printInfo("Task #" + std::to_string(command.taskId) + success);
        return 0;
    }

    int handleDelete(TaskManager& manager, const Command& command)
    {
        if (manager.removeTask(command.taskId) == OperationResult::NotFound)
        {
            view::printNotFound(command.taskId);
            return 1;
        }

        view::printInfo("Task #" + std::to_string(command.taskId) + " deleted.");
        return 0;
    }

    int handleUpdate(TaskManager& manager, const Command& command)
    {
        TaskPatch patch;

        if (command.hasTitle)
        {
            patch.title = command.title;
        }
        if (command.hasDescription)
        {
            patch.description = command.description;
        }
        if (command.hasPriority)
        {
            patch.priority = command.priority;
        }
        patch.dueDate = command.dueDate;
        patch.clearDueDate = command.clearDueDate;
        patch.tags = command.tags;

        const OperationResult result = manager.updateTask(command.taskId, patch);

        if (result == OperationResult::NotFound)
        {
            view::printNotFound(command.taskId);
            return 1;
        }

        if (result == OperationResult::EmptyTitle)
        {
            view::printError("Missing argument.");
            return 1;
        }

        view::printInfo("Task #" + std::to_string(command.taskId) + " updated.");
        return 0;
    }
}

int CLI::run(int argc, char* argv[])
{
    const CommandParser parser;
    const Command command = parser.parse(argc, argv);

    console::initialize(command.useColor);

    if (!command.error.empty())
    {
        view::printError(command.error);
        return 1;
    }

    if (command.type == CommandType::Help)
    {
        view::printHelp();
        return 0;
    }

    TaskManager manager(resolveDataPath());

    switch (command.type)
    {
    case CommandType::Gui:
        return runGui(manager);
    case CommandType::Menu:
    {
        Menu menu(manager);
        return menu.run();
    }
    case CommandType::Add:
        return handleAdd(manager, command);
    case CommandType::List:
        return handleList(manager, command);
    case CommandType::Show:
        return handleShow(manager, command);
    case CommandType::Start:
    case CommandType::Done:
    case CommandType::Reopen:
        return handleTransition(manager, command);
    case CommandType::Delete:
        return handleDelete(manager, command);
    case CommandType::Update:
        return handleUpdate(manager, command);
    case CommandType::Search:
        view::printSearchResults(manager.searchTasks(command.query));
        return 0;
    case CommandType::Stats:
        view::printStatistics(manager.getStatistics());
        return 0;
    default:
        view::printError("Unknown command.");
        return 1;
    }
}
