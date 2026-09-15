#pragma once

#include <chrono>
#include <string>

enum class Status
{
    Todo,
    InProgress,
    Done
};

enum class Priority
{
    Low,
    Medium,
    High
};

class Task
{
public:
    Task(int id,
         std::string title,
         std::string description = "",
         Priority priority = Priority::Medium);

    Task(int id,
         std::string title,
         std::string description,
         Status status,
         Priority priority,
         std::chrono::system_clock::time_point createdAt);

    int getId() const;
    const std::string& getTitle() const;
    const std::string& getDescription() const;
    Status getStatus() const;
    Priority getPriority() const;
    std::chrono::system_clock::time_point getCreatedAt() const;

    void setTitle(const std::string& title);
    void setDescription(const std::string& description);
    void setStatus(Status status);
    void setPriority(Priority priority);

private:
    int id_;
    std::string title_;
    std::string description_;
    Status status_;
    Priority priority_;
    std::chrono::system_clock::time_point createdAt_;
};

std::string toString(Status status);
std::string toString(Priority priority);

bool parseStatus(const std::string& text, Status& out);
bool parsePriority(const std::string& text, Priority& out);

int priorityRank(Priority priority);

std::string formatDate(std::chrono::system_clock::time_point timePoint);
bool parseDate(const std::string& text, std::chrono::system_clock::time_point& out);
