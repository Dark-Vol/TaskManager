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

bool TaskPatch::empty() const
{
    return !title.has_value() && !description.has_value() && !priority.has_value() &&
           !dueDate.has_value() && !clearDueDate && !tags.has_value();
}

int TaskManager::addTask(const std::string& title,
                         const std::string& description,
                         Priority priority)
{
    TaskDraft draft;
    draft.title = title;
    draft.description = description;
    draft.priority = priority;
    return addTask(draft);
}

int TaskManager::addTask(const TaskDraft& draft)
{
    if (draft.title.empty())
    {
        return -1;
    }

    const int id = nextId_++;

    Task task(id, draft.title, draft.description, draft.priority);
    task.setDueDate(draft.dueDate);
    task.setTags(draft.tags);

    tasks_.push_back(std::move(task));
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

OperationResult TaskManager::reopenTask(int id)
{
    Task* task = findTask(id);
    if (task == nullptr)
    {
        return OperationResult::NotFound;
    }

    if (task->getStatus() == Status::Todo)
    {
        return OperationResult::InvalidTransition;
    }

    task->setStatus(Status::Todo);
    persist();
    return OperationResult::Success;
}

OperationResult TaskManager::updateTask(int id, const TaskPatch& patch)
{
    Task* task = findTask(id);
    if (task == nullptr)
    {
        return OperationResult::NotFound;
    }

    if (patch.title.has_value())
    {
        if (patch.title->empty())
        {
            return OperationResult::EmptyTitle;
        }
        task->setTitle(*patch.title);
    }

    if (patch.description.has_value())
    {
        task->setDescription(*patch.description);
    }

    if (patch.priority.has_value())
    {
        task->setPriority(*patch.priority);
    }

    if (patch.clearDueDate)
    {
        task->setDueDate(std::nullopt);
    }
    else if (patch.dueDate.has_value())
    {
        task->setDueDate(patch.dueDate);
    }

    if (patch.tags.has_value())
    {
        task->setTags(*patch.tags);
    }

    persist();
    return OperationResult::Success;
}

OperationResult TaskManager::updateTask(int id,
                                        const std::optional<std::string>& title,
                                        const std::optional<std::string>& description,
                                        const std::optional<Priority>& priority)
{
    TaskPatch patch;
    patch.title = title;
    patch.description = description;
    patch.priority = priority;
    return updateTask(id, patch);
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
            containsIgnoreCase(task.getDescription(), query) ||
            containsIgnoreCase(joinTags(task.getTags()), query))
        {
            result.push_back(task);
        }
    }

    return result;
}

Statistics TaskManager::getStatistics() const
{
    const auto now = std::chrono::system_clock::now();
    Statistics stats;
    stats.total = tasks_.size();

    for (const Task& task : tasks_)
    {
        switch (task.getStatus())
        {
        case Status::Todo:
            ++stats.todo;
            break;
        case Status::InProgress:
            ++stats.inProgress;
            break;
        case Status::Done:
            ++stats.done;
            break;
        }

        switch (task.getPriority())
        {
        case Priority::Low:
            ++stats.low;
            break;
        case Priority::Medium:
            ++stats.medium;
            break;
        case Priority::High:
            ++stats.high;
            break;
        }

        if (task.isOverdue(now))
        {
            ++stats.overdue;
        }
    }

    return stats;
}

const std::string& TaskManager::getStoragePath() const
{
    return storagePath_;
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

        // Records written before due dates and tags existed have only 6 fields.
        const std::vector<std::string> fields = splitRecord(line);
        if (fields.size() != 6 && fields.size() != 8)
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

        Task task(id, fields[1], fields[2], status, priority, createdAt);

        if (fields.size() == 8)
        {
            std::chrono::system_clock::time_point dueDate{};
            if (!fields[6].empty() && parseDate(fields[6], dueDate))
            {
                task.setDueDate(dueDate);
            }

            task.setTags(splitTags(fields[7]));
        }

        tasks_.push_back(std::move(task));
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

    output << "# id|title|description|status|priority|createdAt|dueDate|tags\n";

    for (const Task& task : tasks_)
    {
        output << task.getId() << '|'
               << escapeField(task.getTitle()) << '|'
               << escapeField(task.getDescription()) << '|'
               << toString(task.getStatus()) << '|'
               << toString(task.getPriority()) << '|'
               << formatDate(task.getCreatedAt()) << '|'
               << (task.getDueDate().has_value() ? formatDate(*task.getDueDate()) : "") << '|'
               << escapeField(joinTags(task.getTags())) << '\n';
    }
}

void TaskManager::persist()
{
    save();
}
