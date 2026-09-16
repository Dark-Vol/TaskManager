#pragma once

#include "taskmanager/TaskManager.hpp"

// Interactive mode: started when the program runs without arguments, so the
// executable is also usable by double-clicking it.
class Menu
{
public:
    explicit Menu(TaskManager& manager);

    int run();

private:
    TaskManager& manager_;

    void showMenu() const;

    void addTask();
    void listTasks() const;
    void showTask() const;
    void startTask();
    void completeTask();
    void updateTask();
    void deleteTask();
    void searchTasks() const;
    void showStatistics() const;

    bool askId(int& id) const;
};
