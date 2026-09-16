#include "taskmanager/CommandParser.hpp"

#include <sstream>

namespace
{
    bool parsePositiveId(const std::string &text, int &out)
    {
        if (text.empty())
        {
            return false;
        }

        std::size_t index = 0;
        try
        {
            const int value = std::stoi(text, &index);
            if (index != text.size() || value <= 0)
            {
                return false;
            }

            out = value;
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    CommandType commandFromName(const std::string &name)
    {
        const std::string value = toLowerCopy(name);

        if (value == "add")
            return CommandType::Add;
        if (value == "list")
            return CommandType::List;
        if (value == "show")
            return CommandType::Show;
        if (value == "start")
            return CommandType::Start;
        if (value == "done")
            return CommandType::Done;
        if (value == "reopen")
            return CommandType::Reopen;
        if (value == "delete")
            return CommandType::Delete;
        if (value == "update")
            return CommandType::Update;
        if (value == "search")
            return CommandType::Search;
        if (value == "stats")
            return CommandType::Stats;
        if (value == "menu")
            return CommandType::Menu;
        if (value == "help" || value == "-h" || value == "--help")
            return CommandType::Help;

        return CommandType::Unknown;
    }

    std::string join(const std::vector<std::string> &parts)
    {
        std::ostringstream stream;
        for (std::size_t i = 0; i < parts.size(); ++i)
        {
            if (i > 0)
            {
                stream << ' ';
            }
            stream << parts[i];
        }
        return stream.str();
    }

    bool isFlagWithoutValue(const std::string &key)
    {
        return key == "overdue" || key == "no-color";
    }
}

Command CommandParser::parse(int argc, char *argv[]) const
{
    Command command;

    if (argc < 2)
    {
        command.type = CommandType::Menu;
        return command;
    }

    command.type = commandFromName(argv[1]);
    if (command.type == CommandType::Unknown)
    {
        command.error = "Unknown command '" + std::string(argv[1]) +
                        "'. Use 'help' to see available commands.";
        return command;
    }

    std::vector<std::string> positionals;
    std::string rawTags;
    bool hasRawTags = false;

    for (int i = 2; i < argc; ++i)
    {
        const std::string arg = argv[i];

        std::string key;
        std::string value;
        bool hasValue = false;

        if (arg == "-h" || arg == "--help")
        {
            command.type = CommandType::Help;
            command.error.clear();
            return command;
        }

        if (arg.rfind("--", 0) == 0)
        {
            const auto equals = arg.find('=');
            if (equals != std::string::npos)
            {
                key = toLowerCopy(arg.substr(2, equals - 2));
                value = arg.substr(equals + 1);
                hasValue = true;
            }
            else
            {
                key = toLowerCopy(arg.substr(2));

                if (!isFlagWithoutValue(key) && i + 1 < argc)
                {
                    const std::string next = argv[i + 1];
                    if (next.rfind("-", 0) != 0)
                    {
                        value = next;
                        hasValue = true;
                        ++i;
                    }
                }
            }
        }
        else if (arg == "-p" || arg == "-d" || arg == "-t" || arg == "-s")
        {
            if (arg == "-p")
                key = "priority";
            if (arg == "-d")
                key = "description";
            if (arg == "-t")
                key = "title";
            if (arg == "-s")
                key = "status";

            if (i + 1 >= argc)
            {
                command.error = "Missing value for " + arg + ".";
                return command;
            }

            value = argv[++i];
            hasValue = true;
        }
        else if (arg.rfind("-", 0) == 0)
        {
            command.error = "Unknown option '" + arg + "'.";
            return command;
        }
        else
        {
            positionals.push_back(arg);
            continue;
        }

        if (key == "overdue")
        {
            command.overdueOnly = true;
            continue;
        }

        if (key == "no-color")
        {
            command.useColor = false;
            continue;
        }

        if (!hasValue)
        {
            command.error = "Missing value for --" + key + ".";
            return command;
        }

        if (key == "priority")
        {
            if (!parsePriority(value, command.priority))
            {
                command.error = "Invalid priority. Use low, medium, or high.";
                return command;
            }
            command.hasPriority = true;
        }
        else if (key == "description")
        {
            command.description = value;
            command.hasDescription = true;
        }
        else if (key == "title")
        {
            command.title = value;
            command.hasTitle = true;
        }
        else if (key == "status")
        {
            Status status = Status::Todo;
            if (!parseStatus(value, status))
            {
                command.error = "Invalid status. Use todo, in_progress, or done.";
                return command;
            }
            command.statusFilter = status;
        }
        else if (key == "due")
        {
            if (toLowerCopy(value) == "none")
            {
                command.clearDueDate = true;
            }
            else
            {
                std::chrono::system_clock::time_point dueDate{};
                if (!parseDate(value, dueDate))
                {
                    command.error = "Invalid date. Use the YYYY-MM-DD format.";
                    return command;
                }
                command.dueDate = dueDate;
            }
        }
        else if (key == "tag" || key == "tags")
        {
            rawTags = value;
            hasRawTags = true;
        }
        else if (key == "sort")
        {
            const std::string sort = toLowerCopy(value);
            if (sort != "id" && sort != "priority" && sort != "date" && sort != "due" &&
                sort != "title")
            {
                command.error = "Invalid sort field. Use id, priority, date, due, or title.";
                return command;
            }
            command.sortBy = sort;
        }
        else
        {
            command.error = "Unknown option '--" + key + "'.";
            return command;
        }
    }

    const bool tagsAreFilter = command.type == CommandType::List;

    if (hasRawTags)
    {
        if (tagsAreFilter)
        {
            command.tagFilter = rawTags;
        }
        else
        {
            command.tags = splitTags(rawTags);
        }
    }

    switch (command.type)
    {
    case CommandType::Add:
        if (!positionals.empty())
        {
            command.title = positionals[0];
            command.hasTitle = true;
        }
        if (positionals.size() > 1 && !command.hasDescription)
        {
            command.description = positionals[1];
            command.hasDescription = true;
        }
        if (!command.hasTitle || command.title.empty())
        {
            command.error = "Missing argument. Usage: taskmanager add \"<title>\" [--priority low|medium|high]";
        }
        break;

    case CommandType::Show:
    case CommandType::Start:
    case CommandType::Done:
    case CommandType::Reopen:
    case CommandType::Delete:
        if (positionals.empty() || !parsePositiveId(positionals[0], command.taskId))
        {
            command.error = "Invalid ID.";
        }
        else
        {
            command.hasTaskId = true;
        }
        break;

    case CommandType::Update:
        if (positionals.empty() || !parsePositiveId(positionals[0], command.taskId))
        {
            command.error = "Invalid ID.";
            break;
        }
        command.hasTaskId = true;
        if (!command.hasTitle && !command.hasDescription && !command.hasPriority &&
            !command.dueDate.has_value() && !command.clearDueDate && !command.tags.has_value())
        {
            command.error = "Missing argument. Usage: taskmanager update <id> [--title ...] [--description ...] [--priority ...] [--due ...] [--tag ...]";
        }
        break;

    case CommandType::Search:
        command.query = join(positionals);
        if (command.query.empty())
        {
            command.error = "Missing argument. Usage: taskmanager search <query>";
        }
        break;

    case CommandType::List:
        if (command.hasPriority)
        {
            command.priorityFilter = command.priority;
        }
        break;

    default:
        break;
    }

    return command;
}
