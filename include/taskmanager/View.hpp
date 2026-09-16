#pragma once

#include "taskmanager/Task.hpp"
#include "taskmanager/TaskManager.hpp"

#include <string>
#include <vector>

// Everything the user sees lives here, so that both the command line and the
// interactive menu print tasks the same way.
namespace view
{
    void printHelp();
    void printTaskList(const std::vector<Task>& tasks);
    void printTaskDetails(const Task& task);
    void printStatistics(const Statistics& stats);
    void printSearchResults(const std::vector<Task>& tasks);

    void printCreated(int id);
    void printNotFound(int id);
    void printTransitionError(const Task& task, const std::string& action);
    void printError(const std::string& message);
    void printInfo(const std::string& message);
}
