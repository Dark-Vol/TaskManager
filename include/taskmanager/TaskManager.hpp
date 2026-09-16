#pragma once

#include "taskmanager/Task.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

enum class OperationResult
{
    Success,
    NotFound,
    InvalidTransition,
    EmptyTitle
};

struct TaskDraft
{
    std::string title;
    std::string description;
    Priority priority = Priority::Medium;
    std::optional<std::chrono::system_clock::time_point> dueDate;
    std::vector<std::string> tags;
};

struct TaskPatch
{
    std::optional<std::string> title;
    std::optional<std::string> description;
    std::optional<Priority> priority;
    std::optional<std::chrono::system_clock::time_point> dueDate;
    bool clearDueDate = false;
    std::optional<std::vector<std::string>> tags;

    bool empty() const;
};

struct Statistics
{
    std::size_t total = 0;
    std::size_t todo = 0;
    std::size_t inProgress = 0;
    std::size_t done = 0;
    std::size_t low = 0;
    std::size_t medium = 0;
    std::size_t high = 0;
    std::size_t overdue = 0;
};

class TaskManager
{
public:
    explicit TaskManager(std::string storagePath = "data/tasks.txt");

    int addTask(const std::string& title,
                const std::string& description = "",
                Priority priority = Priority::Medium);

    int addTask(const TaskDraft& draft);

    OperationResult removeTask(int id);
    OperationResult startTask(int id);
    OperationResult completeTask(int id);
    OperationResult reopenTask(int id);

    OperationResult updateTask(int id, const TaskPatch& patch);

    OperationResult updateTask(int id,
                               const std::optional<std::string>& title,
                               const std::optional<std::string>& description = {},
                               const std::optional<Priority>& priority = {});

    Task* findTask(int id);
    const Task* findTask(int id) const;

    const std::vector<Task>& getTasks() const;
    std::vector<Task> searchTasks(const std::string& query) const;
    Statistics getStatistics() const;

    const std::string& getStoragePath() const;

    void load();
    void save() const;

private:
    std::vector<Task> tasks_;
    int nextId_ = 1;
    std::string storagePath_;

    void persist();
};
