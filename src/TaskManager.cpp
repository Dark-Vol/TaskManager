#include "taskmanager/TaskManager.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <utility>

namespace
{
    std::string escapeField(const std::string& text)
    {
        std::string result;
        result.reserve(text.size());

        for (char ch : text)
        {
            if (ch == '\\' || ch == '|')
            {
                result.push_back('\\');
            }
            result.push_back(ch);
        }

        return result;
    }

    std::vector<std::string> splitRecord(const std::string& line)
    {
        std::vector<std::string> fields;
        std::string current;
        bool escaped = false;

        for (char ch : line)
        {
            if (escaped)
            {
                current.push_back(ch);
                escaped = false;
                continue;
            }

            if (ch == '\\')
            {
                escaped = true;
                continue;
            }

            if (ch == '|')
            {
                fields.push_back(current);
                current.clear();
                continue;
            }

            current.push_back(ch);
        }

        fields.push_back(current);
        return fields;
    }

    std::string toLowerCopy(std::string text)
    {
        std::transform(text.begin(), text.end(), text.begin(),
                       [](unsigned char ch) {
                           return static_cast<char>(std::tolower(ch));
                       });
        return text;
    }

    bool containsIgnoreCase(const std::string& text, const std::string& query)
    {
        const std::string haystack = toLowerCopy(text);
        const std::string needle = toLowerCopy(query);
        return haystack.find(needle) != std::string::npos;
    }
}

TaskManager::TaskManager(std::string storagePath)
    : storagePath_(std::move(storagePath))
{
    load();
}

int TaskManager::addTask(const std::string& title,
                         const std::string& description,
                         Priority priority)
{
    if (title.empty())
    {
        return -1;
    }

    const int id = nextId_++;
    tasks_.push_back(Task(id, title, description, priority));
    persist();
    return id;
}

OperationResult TaskManager::removeTask(int id)
{
    const auto it = std::find_if(tasks_.begin(), tasks_.end(),
                                 [id](const Task& task) {
                                     return task.getId() == id;
                                 });

    if (it == tasks_.end())
    {
        return OperationResult::NotFound;
    }

    tasks_.erase(it);
    persist();
    return OperationResult::Success;
}

OperationResult TaskManager::startTask(int id)
{
    Task* task = findTask(id);
    if (task == nullptr)
    {
        return OperationResult::NotFound;
    }

    if (task->getStatus() != Status::Todo)
    {
        return OperationResult::InvalidTransition;
    }

    task->setStatus(Status::InProgress);
    persist();
    return OperationResult::Success;
}

OperationResult TaskManager::completeTask(int id)
{
    Task* task = findTask(id);
    if (task == nullptr)
    {
        return OperationResult::NotFound;
    }

    if (task->getStatus() != Status::InProgress)
    {
        return OperationResult::InvalidTransition;
    }

    task->setStatus(Status::Done);
    persist();
    return OperationResult::Success;
}

OperationResult TaskManager::updateTask(int id,
                                        const std::optional<std::string>& title,
                                        const std::optional<std::string>& description,
                                        const std::optional<Priority>& priority)
{
    Task* task = findTask(id);
    if (task == nullptr)
    {
        return OperationResult::NotFound;
    }

    if (title.has_value())
    {
        if (title->empty())
        {
            return OperationResult::EmptyTitle;
        }
        task->setTitle(*title);
    }

    if (description.has_value())
    {
        task->setDescription(*description);
    }

    if (priority.has_value())
    {
        task->setPriority(*priority);
    }

    persist();
    return OperationResult::Success;
}

Task* TaskManager::findTask(int id)
{
    for (Task& task : tasks_)
    {
        if (task.getId() == id)
        {
            return &task;
        }
    }

    return nullptr;
}

const Task* TaskManager::findTask(int id) const
{
    for (const Task& task : tasks_)
    {
        if (task.getId() == id)
        {
            return &task;
        }
    }

    return nullptr;
}

const std::vector<Task>& TaskManager::getTasks() const
{
    return tasks_;
}

std::vector<Task> TaskManager::searchTasks(const std::string& query) const
{
    std::vector<Task> result;

    for (const Task& task : tasks_)
    {
        if (containsIgnoreCase(task.getTitle(), query) ||
            containsIgnoreCase(task.getDescription(), query))
        {
            result.push_back(task);
        }
    }

    return result;
}

void TaskManager::load()
{
    tasks_.clear();
    nextId_ = 1;

    std::ifstream input(storagePath_);
    if (!input)
    {
        return;
    }

    std::string line;
    while (std::getline(input, line))
    {
        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        const std::vector<std::string> fields = splitRecord(line);
        if (fields.size() != 6)
        {
            continue;
        }

        int id = 0;
        Status status = Status::Todo;
        Priority priority = Priority::Medium;
        std::chrono::system_clock::time_point createdAt{};

        try
        {
            id = std::stoi(fields[0]);
        }
        catch (...)
        {
            continue;
        }

        if (id <= 0 || !parseStatus(fields[3], status) ||
            !parsePriority(fields[4], priority) || !parseDate(fields[5], createdAt))
        {
            continue;
        }

        tasks_.push_back(Task(id, fields[1], fields[2], status, priority, createdAt));
        nextId_ = std::max(nextId_, id + 1);
    }
}

void TaskManager::save() const
{
    const std::filesystem::path path(storagePath_);
    if (path.has_parent_path() && !path.parent_path().empty())
    {
        std::filesystem::create_directories(path.parent_path());
    }

    std::ofstream output(storagePath_, std::ios::trunc);
    if (!output)
    {
        std::cerr << "Error: Failed to open tasks file.\n";
        return;
    }

    output << "# id|title|description|status|priority|createdAt\n";

    for (const Task& task : tasks_)
    {
        output << task.getId() << '|'
               << escapeField(task.getTitle()) << '|'
               << escapeField(task.getDescription()) << '|'
               << toString(task.getStatus()) << '|'
               << toString(task.getPriority()) << '|'
               << formatDate(task.getCreatedAt()) << '\n';
    }
}

void TaskManager::persist()
{
    save();
}
