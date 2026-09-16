#include "taskmanager/View.hpp"
#include "taskmanager/Console.hpp"

#include <iostream>

namespace
{
    console::Color colorFor(Status status)
    {
        switch (status)
        {
        case Status::Todo:
            return console::Color::Yellow;
        case Status::InProgress:
            return console::Color::Cyan;
        case Status::Done:
            return console::Color::Green;
        }

        return console::Color::None;
    }

    console::Color colorFor(Priority priority)
    {
        switch (priority)
        {
        case Priority::High:
            return console::Color::Red;
        case Priority::Medium:
            return console::Color::Yellow;
        case Priority::Low:
            return console::Color::Gray;
        }

        return console::Color::None;
    }

    std::string dueColumn(const Task& task)
    {
        if (!task.getDueDate().has_value())
        {
            return "-";
        }

        return formatDate(*task.getDueDate());
    }
}

namespace view
{
    void printHelp()
    {
        std::cout
            << console::colorize("Task Manager", console::Color::Bold) << "\n\n"
            << "Usage:\n"
            << "  taskmanager <command> [options]\n"
            << "  taskmanager                      Start the interactive menu\n\n"
            << "Commands:\n\n"
            << "  add       Create a new task\n"
            << "  list      Show tasks\n"
            << "  show      Show task details\n"
            << "  start     Start a task\n"
            << "  done      Complete a task\n"
            << "  reopen    Move a task back to TODO\n"
            << "  update    Update a task\n"
            << "  delete    Delete a task\n"
            << "  search    Search tasks\n"
            << "  stats     Show statistics\n"
            << "  menu      Start the interactive menu\n"
            << "  help      Show help\n\n"
            << "Options:\n\n"
            << "  --priority low|medium|high   Priority, MEDIUM by default\n"
            << "  --description <text>         Task description\n"
            << "  --title <text>               New title for update\n"
            << "  --due YYYY-MM-DD             Due date (use 'none' to clear it)\n"
            << "  --tag work,home              Tags, comma separated\n"
            << "  --status todo|in_progress|done   Filter by status\n"
            << "  --overdue                    Show only overdue tasks\n"
            << "  --sort id|priority|date|due|title   Sort the list\n"
            << "  --no-color                   Disable colored output\n\n"
            << "Examples:\n"
            << "  taskmanager add \"Learn C++\" --priority high --due 2026-09-20 --tag study\n"
            << "  taskmanager list --status todo --sort due\n"
            << "  taskmanager update 1 --due none --tag study,cpp\n"
            << "  taskmanager stats\n";
    }

    void printTaskList(const std::vector<Task>& tasks)
    {
        if (tasks.empty())
        {
            std::cout << "No tasks found.\n";
            return;
        }

        const auto now = std::chrono::system_clock::now();

        std::cout << console::colorize("ID   STATUS        PRIORITY   DUE          TAGS            TITLE",
                                       console::Color::Bold)
                  << '\n'
                  << "----------------------------------------------------------------------------\n";

        for (const Task& task : tasks)
        {
            const bool overdue = task.isOverdue(now);

            std::cout << console::cell(std::to_string(task.getId()), 5)
                      << console::cell(toString(task.getStatus()), 14, colorFor(task.getStatus()))
                      << console::cell(toString(task.getPriority()), 11, colorFor(task.getPriority()))
                      << console::cell(dueColumn(task), 13,
                                       overdue ? console::Color::Red : console::Color::None)
                      << console::cell(task.getTags().empty() ? "-" : joinTags(task.getTags()), 16,
                                       console::Color::Gray)
                      << task.getTitle() << '\n';
        }
    }

    void printTaskDetails(const Task& task)
    {
        const auto now = std::chrono::system_clock::now();
        const bool overdue = task.isOverdue(now);

        std::cout << console::colorize("Task #" + std::to_string(task.getId()),
                                       console::Color::Bold)
                  << "\n\n"
                  << "Title:       " << task.getTitle() << '\n'
                  << "Description: "
                  << (task.getDescription().empty() ? "-" : task.getDescription()) << '\n'
                  << "Status:      "
                  << console::colorize(toString(task.getStatus()), colorFor(task.getStatus())) << '\n'
                  << "Priority:    "
                  << console::colorize(toString(task.getPriority()), colorFor(task.getPriority()))
                  << '\n'
                  << "Tags:        "
                  << (task.getTags().empty() ? "-" : joinTags(task.getTags())) << '\n'
                  << "Created:     " << formatDate(task.getCreatedAt()) << '\n'
                  << "Due:         "
                  << console::colorize(dueColumn(task) + (overdue ? "  (overdue)" : ""),
                                       overdue ? console::Color::Red : console::Color::None)
                  << '\n';
    }

    void printStatistics(const Statistics& stats)
    {
        std::cout << console::colorize("Statistics", console::Color::Bold) << "\n\n"
                  << "Total:        " << stats.total << '\n'
                  << "TODO:         " << console::colorize(std::to_string(stats.todo),
                                                           console::Color::Yellow) << '\n'
                  << "IN_PROGRESS:  " << console::colorize(std::to_string(stats.inProgress),
                                                           console::Color::Cyan) << '\n'
                  << "DONE:         " << console::colorize(std::to_string(stats.done),
                                                           console::Color::Green) << '\n'
                  << '\n'
                  << "HIGH:         " << stats.high << '\n'
                  << "MEDIUM:       " << stats.medium << '\n'
                  << "LOW:          " << stats.low << '\n'
                  << '\n'
                  << "Overdue:      " << console::colorize(std::to_string(stats.overdue),
                                                           stats.overdue > 0 ? console::Color::Red
                                                                             : console::Color::None)
                  << '\n';
    }

    void printSearchResults(const std::vector<Task>& tasks)
    {
        if (tasks.empty())
        {
            std::cout << "No tasks found.\n";
            return;
        }

        for (const Task& task : tasks)
        {
            std::cout << '#' << task.getId() << "  " << task.getTitle() << '\n';
        }
    }

    void printCreated(int id)
    {
        std::cout << console::colorize("Task created successfully!", console::Color::Green) << '\n'
                  << "ID: " << id << '\n';
    }

    void printNotFound(int id)
    {
        printError("Task #" + std::to_string(id) + " does not exist.");
    }

    void printTransitionError(const Task& task, const std::string& action)
    {
        printError("Task #" + std::to_string(task.getId()) + " cannot be " + action + " from " +
                   toString(task.getStatus()) + ".");
    }

    void printError(const std::string& message)
    {
        std::cerr << console::colorize("Error: ", console::Color::Red) << message << '\n';
    }

    void printInfo(const std::string& message)
    {
        std::cout << message << '\n';
    }
}
