#pragma once

#include "taskmanager/TaskManager.hpp"

// Opens the application window: a task table with buttons for every operation.
// Returns the exit code of the program. On systems other than Windows it reports
// that the window is unavailable, so the console modes stay usable there.
int runGui(TaskManager& manager);
