#include "taskmanager/Task.hpp"

#include <algorithm>
#include <cctype>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <utility>

std::string toLowerCopy(std::string text)
{
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char ch) {
                       return static_cast<char>(std::tolower(ch));
                   });
    return text;
}

Task::Task(int id, std::string title, std::string description, Priority priority)
    : Task(id,
           std::move(title),
           std::move(description),
           Status::Todo,
           priority,
           std::chrono::system_clock::now())
{
}

Task::Task(int id,
           std::string title,
           std::string description,
           Status status,
           Priority priority,
           std::chrono::system_clock::time_point createdAt)
    : id_(id)
    , title_(std::move(title))
    , description_(std::move(description))
    , status_(status)
    , priority_(priority)
    , createdAt_(createdAt)
{
}

int Task::getId() const
{
    return id_;
}

const std::string& Task::getTitle() const
{
    return title_;
}

const std::string& Task::getDescription() const
{
    return description_;
}

Status Task::getStatus() const
{
    return status_;
}

Priority Task::getPriority() const
{
    return priority_;
}

std::chrono::system_clock::time_point Task::getCreatedAt() const
{
    return createdAt_;
}

const std::optional<std::chrono::system_clock::time_point>& Task::getDueDate() const
{
    return dueDate_;
}

const std::vector<std::string>& Task::getTags() const
{
    return tags_;
}

void Task::setTitle(const std::string& title)
{
    title_ = title;
}

void Task::setDescription(const std::string& description)
{
    description_ = description;
}

void Task::setStatus(Status status)
{
    status_ = status;
}

void Task::setPriority(Priority priority)
{
    priority_ = priority;
}

void Task::setDueDate(std::optional<std::chrono::system_clock::time_point> dueDate)
{
    dueDate_ = dueDate;
}

void Task::setTags(std::vector<std::string> tags)
{
    tags_ = std::move(tags);
}

bool Task::hasTag(const std::string& tag) const
{
    const std::string needle = toLowerCopy(tag);

    return std::any_of(tags_.begin(), tags_.end(),
                       [&needle](const std::string& own) {
                           return toLowerCopy(own) == needle;
                       });
}

bool Task::isOverdue(std::chrono::system_clock::time_point now) const
{
    if (!dueDate_.has_value() || status_ == Status::Done)
    {
        return false;
    }

    return *dueDate_ < now;
}

std::string toString(Status status)
{
    switch (status)
    {
    case Status::Todo:
        return "TODO";
    case Status::InProgress:
        return "IN_PROGRESS";
    case Status::Done:
        return "DONE";
    }

    return "TODO";
}

std::string toString(Priority priority)
{
    switch (priority)
    {
    case Priority::Low:
        return "LOW";
    case Priority::Medium:
        return "MEDIUM";
    case Priority::High:
        return "HIGH";
    }

    return "MEDIUM";
}

bool parseStatus(const std::string& text, Status& out)
{
    const std::string value = toLowerCopy(text);

    if (value == "todo")
    {
        out = Status::Todo;
        return true;
    }

    if (value == "in_progress" || value == "in-progress" ||
        value == "inprogress" || value == "in progress")
    {
        out = Status::InProgress;
        return true;
    }

    if (value == "done")
    {
        out = Status::Done;
        return true;
    }

    return false;
}

bool parsePriority(const std::string& text, Priority& out)
{
    const std::string value = toLowerCopy(text);

    if (value == "low")
    {
        out = Priority::Low;
        return true;
    }

    if (value == "medium" || value == "med")
    {
        out = Priority::Medium;
        return true;
    }

    if (value == "high")
    {
        out = Priority::High;
        return true;
    }

    return false;
}

int priorityRank(Priority priority)
{
    switch (priority)
    {
    case Priority::High:
        return 3;
    case Priority::Medium:
        return 2;
    case Priority::Low:
        return 1;
    }

    return 0;
}

std::string formatDate(std::chrono::system_clock::time_point timePoint)
{
    const std::time_t time = std::chrono::system_clock::to_time_t(timePoint);
    std::tm localTime{};

#ifdef _WIN32
    localtime_s(&localTime, &time);
#else
    localtime_r(&time, &localTime);
#endif

    std::ostringstream stream;
    stream << std::put_time(&localTime, "%Y-%m-%d");
    return stream.str();
}

bool parseDate(const std::string& text, std::chrono::system_clock::time_point& out)
{
    std::tm localTime{};
    std::istringstream stream(text);
    stream >> std::get_time(&localTime, "%Y-%m-%d");

    if (stream.fail())
    {
        return false;
    }

    localTime.tm_hour = 12;
    localTime.tm_min = 0;
    localTime.tm_sec = 0;
    localTime.tm_isdst = -1;

    const std::time_t time = std::mktime(&localTime);
    if (time == -1)
    {
        return false;
    }

    out = std::chrono::system_clock::from_time_t(time);
    return true;
}

std::vector<std::string> splitTags(const std::string& text)
{
    std::vector<std::string> tags;
    std::string current;

    const auto flush = [&tags, &current]() {
        const auto first = current.find_first_not_of(" \t");
        if (first == std::string::npos)
        {
            current.clear();
            return;
        }

        const auto last = current.find_last_not_of(" \t");
        std::string tag = current.substr(first, last - first + 1);
        current.clear();

        const bool duplicate = std::any_of(tags.begin(), tags.end(),
                                           [&tag](const std::string& own) {
                                               return toLowerCopy(own) == toLowerCopy(tag);
                                           });
        if (!duplicate)
        {
            tags.push_back(std::move(tag));
        }
    };

    for (char ch : text)
    {
        if (ch == ',' || ch == ';')
        {
            flush();
            continue;
        }

        current.push_back(ch);
    }

    flush();
    return tags;
}

std::string joinTags(const std::vector<std::string>& tags)
{
    std::string result;

    for (const std::string& tag : tags)
    {
        if (!result.empty())
        {
            result.push_back(',');
        }
        result += tag;
    }

    return result;
}
