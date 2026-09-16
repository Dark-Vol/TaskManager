#pragma once

#include "taskmanager/Task.hpp"

#include <chrono>
#include <optional>
#include <string>
#include <vector>

enum class CommandType
{
    Add,
    List,
    Show,
    Start,
    Done,
    Reopen,
    Delete,
    Update,
    Search,
    Stats,
    Menu,
    Gui,
    Help,
    Unknown
};

struct Command
{
    CommandType type = CommandType::Unknown;

    std::string title;
    std::string description;
    Priority priority = Priority::Medium;
    int taskId = 0;

    bool hasTitle = false;
    bool hasDescription = false;
    bool hasPriority = false;
    bool hasTaskId = false;

    std::optional<std::chrono::system_clock::time_point> dueDate;
    bool clearDueDate = false;
    std::optional<std::vector<std::string>> tags;

    std::optional<Status> statusFilter;
    std::optional<Priority> priorityFilter;
    std::string tagFilter;
    bool overdueOnly = false;
    std::string sortBy;
    std::string query;

    bool useColor = true;

    std::string error;
};

class CommandParser
{
public:
    Command parse(int argc, char* argv[]) const;
};
