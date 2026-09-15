#pragma once

#include "taskmanager/Task.hpp"

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
    Delete,
    Update,
    Search,
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

    std::optional<Status> statusFilter;
    std::optional<Priority> priorityFilter;
    std::string sortBy;
    std::string query;

    std::string error;
};

class CommandParser
{
public:
    Command parse(int argc, char* argv[]) const;
};
