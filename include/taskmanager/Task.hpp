#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

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
    const std::optional<std::chrono::system_clock::time_point>& getDueDate() const;
    const std::vector<std::string>& getTags() const;

    void setTitle(const std::string& title);
    void setDescription(const std::string& description);
    void setStatus(Status status);
    void setPriority(Priority priority);
    void setDueDate(std::optional<std::chrono::system_clock::time_point> dueDate);
    void setTags(std::vector<std::string> tags);

    bool hasTag(const std::string& tag) const;
    bool isOverdue(std::chrono::system_clock::time_point now) const;

private:
    int id_;
    std::string title_;
    std::string description_;
    Status status_;
    Priority priority_;
    std::chrono::system_clock::time_point createdAt_;
    std::optional<std::chrono::system_clock::time_point> dueDate_;
    std::vector<std::string> tags_;
};

std::string toString(Status status);
std::string toString(Priority priority);

bool parseStatus(const std::string& text, Status& out);
bool parsePriority(const std::string& text, Priority& out);

int priorityRank(Priority priority);

std::string formatDate(std::chrono::system_clock::time_point timePoint);
bool parseDate(const std::string& text, std::chrono::system_clock::time_point& out);

std::string toLowerCopy(std::string text);
std::vector<std::string> splitTags(const std::string& text);
std::string joinTags(const std::vector<std::string>& tags);
