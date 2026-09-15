#pragma once

#include "taskmanager/Task.hpp"

#include <string>
#include <vector>
#include <optional>

enum class OperationResult
{
    Success,
    NotFound,
    InvalidTransition,
    EmptyTitle
};

class TaskManager
{
public:
    explicit TaskManager(std::string storagePath = "data/tasks.txt");

    int addTask(const std::string& title,
                const std::string& description = "",
                Priority priority = Priority::Medium);

    OperationResult removeTask(int id);
    OperationResult startTask(int id);
    OperationResult completeTask(int id);

    OperationResult updateTask(int id,
                               const std::optional<std::string>& title = {},
                               const std::optional<std::string>& description = {},
                               const std::optional<Priority>& priority = {});

    Task* findTask(int id);
    const Task* findTask(int id) const;

    const std::vector<Task>& getTasks() const;
    std::vector<Task> searchTasks(const std::string& query) const;

    void load();
    void save() const;

private:
    std::vector<Task> tasks_;
    int nextId_ = 1;
    std::string storagePath_;

    void persist();
};
