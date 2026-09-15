#include "taskmanager/CLI.hpp"
#include "taskmanager/CommandParser.hpp"
#include "taskmanager/TaskManager.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace
{
    void printHelp()
    {
        std::cout
            << "Task Manager\n\n"
            << "Usage:\n"
            << "  taskmanager <command> [options]\n\n"
            << "Commands:\n\n"
            << "  add       Create a new task\n"
            << "  list      Show tasks\n"
            << "  show      Show task details\n"
            << "  start     Start a task\n"
            << "  done      Complete a task\n"
            << "  update    Update a task\n"
            << "  delete    Delete a task\n"
            << "  search    Search tasks\n"
            << "  help      Show help\n\n"
            << "Examples:\n"
            << "  taskmanager add \"Learn C++\" --priority high\n"
            << "  taskmanager list --status todo --sort priority\n"
            << "  taskmanager start 1\n"
            << "  taskmanager done 1\n";
    }

    void printNotFound(int id)
    {
        std::cerr << "Error: Task #" << id << " does not exist.\n";
    }

    void printTransitionError(const Task& task, const std::string& action)
    {
        std::cerr << "Error: Task #" << task.getId()
                  << " cannot be " << action << " from "
                  << toString(task.getStatus()) << ".\n";
    }

    void printTaskDetails(const Task& task)
    {
        std::cout << "Task #" << task.getId() << "\n\n"
                  << "Title:       " << task.getTitle() << '\n'
                  << "Description: "
                  << (task.getDescription().empty() ? "-" : task.getDescription()) << '\n'
                  << "Status:      " << toString(task.getStatus()) << '\n'
                  << "Priority:    " << toString(task.getPriority()) << '\n'
                  << "Created:     " << formatDate(task.getCreatedAt()) << '\n';
    }

    std::vector<Task> filterTasks(std::vector<Task> tasks, const Command& command)
    {
        if (command.statusFilter.has_value())
        {
            tasks.erase(std::remove_if(tasks.begin(), tasks.end(),
                                       [&](const Task& task) {
                                           return task.getStatus() != *command.statusFilter;
                                       }),
                        tasks.end());
        }

        if (command.priorityFilter.has_value())
        {
            tasks.erase(std::remove_if(tasks.begin(), tasks.end(),
                                       [&](const Task& task) {
                                           return task.getPriority() != *command.priorityFilter;
                                       }),
                        tasks.end());
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

    void printTaskList(const std::vector<Task>& tasks)
    {
        if (tasks.empty())
        {
            std::cout << "No tasks found.\n";
            return;
        }

        std::cout << std::left
                  << std::setw(5) << "ID"
                  << std::setw(14) << "STATUS"
                  << std::setw(11) << "PRIORITY"
                  << "TITLE\n"
                  << "--------------------------------------------------\n";

        for (const Task& task : tasks)
        {
            std::cout << std::left
                      << std::setw(5) << task.getId()
                      << std::setw(14) << toString(task.getStatus())
                      << std::setw(11) << toString(task.getPriority())
                      << task.getTitle() << '\n';
        }
    }

    int handleAdd(TaskManager& manager, const Command& command)
    {
        const int id = manager.addTask(
            command.title,
            command.hasDescription ? command.description : "",
            command.hasPriority ? command.priority : Priority::Medium);

        if (id < 0)
        {
            std::cerr << "Error: Missing argument.\n";
            return 1;
        }

        std::cout << "Task created successfully!\n"
                  << "ID: " << id << '\n';
        return 0;
    }

    int handleList(const TaskManager& manager, const Command& command)
    {
        std::vector<Task> tasks = filterTasks(manager.getTasks(), command);
        sortTasks(tasks, command.sortBy);
        printTaskList(tasks);
        return 0;
    }

    int handleShow(const TaskManager& manager, const Command& command)
    {
        const Task* task = manager.findTask(command.taskId);
        if (task == nullptr)
        {
            printNotFound(command.taskId);
            return 1;
        }

        printTaskDetails(*task);
        return 0;
    }

    int handleStart(TaskManager& manager, const Command& command)
    {
        const Task* existing = manager.findTask(command.taskId);
        const OperationResult result = manager.startTask(command.taskId);

        if (result == OperationResult::NotFound)
        {
            printNotFound(command.taskId);
            return 1;
        }

        if (result == OperationResult::InvalidTransition)
        {
            printTransitionError(*existing, "started");
            return 1;
        }

        std::cout << "Task #" << command.taskId << " is now IN PROGRESS.\n";
        return 0;
    }

    int handleDone(TaskManager& manager, const Command& command)
    {
        const Task* existing = manager.findTask(command.taskId);
        const OperationResult result = manager.completeTask(command.taskId);

        if (result == OperationResult::NotFound)
        {
            printNotFound(command.taskId);
            return 1;
        }

        if (result == OperationResult::InvalidTransition)
        {
            printTransitionError(*existing, "completed");
            return 1;
        }

        std::cout << "Task #" << command.taskId << " completed.\n";
        return 0;
    }

    int handleDelete(TaskManager& manager, const Command& command)
    {
        const OperationResult result = manager.removeTask(command.taskId);
        if (result == OperationResult::NotFound)
        {
            printNotFound(command.taskId);
            return 1;
        }

        std::cout << "Task #" << command.taskId << " deleted.\n";
        return 0;
    }

    int handleUpdate(TaskManager& manager, const Command& command)
    {
        std::optional<std::string> title;
        std::optional<std::string> description;
        std::optional<Priority> priority;

        if (command.hasTitle)
        {
            title = command.title;
        }
        if (command.hasDescription)
        {
            description = command.description;
        }
        if (command.hasPriority)
        {
            priority = command.priority;
        }

        const OperationResult result = manager.updateTask(command.taskId, title, description, priority);
        if (result == OperationResult::NotFound)
        {
            printNotFound(command.taskId);
            return 1;
        }

        if (result == OperationResult::EmptyTitle)
        {
            std::cerr << "Error: Missing argument.\n";
            return 1;
        }

        std::cout << "Task #" << command.taskId << " updated.\n";
        return 0;
    }

    int handleSearch(const TaskManager& manager, const Command& command)
    {
        const std::vector<Task> tasks = manager.searchTasks(command.query);
        if (tasks.empty())
        {
            std::cout << "No tasks found.\n";
            return 0;
        }

        for (const Task& task : tasks)
        {
            std::cout << '#' << task.getId() << "  " << task.getTitle() << '\n';
        }

        return 0;
    }
}

int CLI::run(int argc, char* argv[])
{
    const CommandParser parser;
    const Command command = parser.parse(argc, argv);

    if (!command.error.empty())
    {
        std::cerr << "Error: " << command.error << '\n';
        return 1;
    }

    if (command.type == CommandType::Help)
    {
        printHelp();
        return 0;
    }

    TaskManager manager("data/tasks.txt");

    switch (command.type)
    {
    case CommandType::Add:
        return handleAdd(manager, command);
    case CommandType::List:
        return handleList(manager, command);
    case CommandType::Show:
        return handleShow(manager, command);
    case CommandType::Start:
        return handleStart(manager, command);
    case CommandType::Done:
        return handleDone(manager, command);
    case CommandType::Delete:
        return handleDelete(manager, command);
    case CommandType::Update:
        return handleUpdate(manager, command);
    case CommandType::Search:
        return handleSearch(manager, command);
    default:
        std::cerr << "Error: Unknown command.\n";
        return 1;
    }
}
