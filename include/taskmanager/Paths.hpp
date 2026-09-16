#pragma once

#include <string>

// Resolves the tasks file so that the program always works with the same list,
// no matter which directory it was started from.
//
// Order of resolution:
//   1. TASKMANAGER_DATA environment variable
//   2. data/tasks.txt inside the project root found next to the executable
//   3. data/tasks.txt inside the project root found from the current directory
//   4. data/tasks.txt next to the executable
std::string resolveDataPath();
