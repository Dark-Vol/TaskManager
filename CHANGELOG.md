# Changelog

## v1.2

### Added

- Application window built on the Win32 API, without third-party libraries
- Starting the program without arguments now opens the window; the text menu moved to the
  `menu` command, and the window can also be opened explicitly with `gui`
- Window contents: task table (ID, status, priority, due date, tags, title), buttons
  Add, Edit, Start, Done, Reopen, Delete, Reload, a search box that filters while typing,
  and a status line with statistics and the path of the tasks file
- Modal dialog for creating and editing a task: title, description, priority, due date,
  and tags, with validation of the title and the date format
- Overdue tasks are drawn in red, the same idea as the colored console output
- Double-clicking a row opens the edit dialog; deleting asks for confirmation

### Changed files

Modified files:

- `include/taskmanager/CommandParser.hpp` — added the `Gui` command type
- `src/CommandParser.cpp` — recognizes the `gui` and `window` commands, and no arguments
  now means the window instead of the text menu
- `src/CLI.cpp` — dispatches the `Gui` command to `runGui()`
- `src/View.cpp` — help text mentions the window and the new `gui` command
- `src/main.cpp` — uses the shared helpers from `Encoding` instead of its own local copy
  of the UTF-16 conversion
- `CMakeLists.txt` — `Gui.cpp` and `Encoding.cpp` added to the library, and `comctl32`,
  `user32`, `gdi32` linked on Windows
- `.vscode/tasks.json` — the build task lists the new sources and links `comctl32`
  and `gdi32`
- `.vscode/settings.json` — the same for the C/C++ Compile Run extension, plus the linker
  flags for `F6`
- `CHANGELOG.md` — this file

## v1.1

### Added

- Interactive text menu
- Due dates (`--due YYYY-MM-DD`, `--due none` to clear)
- Tags (`--tag work,home`) and filtering by tag
- `stats` command with counters by status, priority, and overdue tasks
- `reopen` command, moving a task back to `TODO`
- `--overdue` filter and `--sort due`
- Colored output, with `--no-color` and `NO_COLOR` support

### Changed

- Storage format extended to `id|title|description|status|priority|createdAt|dueDate|tags`;
  old six-field records still load
- Output moved from `CLI` into a separate `View` + `Console` layer, shared with the menu

### Fixed

- Tasks file is now resolved relative to the executable and project root, so every build
  uses the same `data/tasks.txt` instead of one file per working directory
- Non-ASCII titles on Windows: the command line is read as UTF-16 and converted to UTF-8,
  so Cyrillic is no longer mangled in the output and in the tasks file
- Byte order mark in piped input is ignored by the menu

### Changed files

Headers, `include/taskmanager/`:

- `Task.hpp` (modified) — `dueDate` and `tags` fields, `hasTag()`, `isOverdue()`, and the
  shared helpers `toLowerCopy()`, `splitTags()`, `joinTags()`
- `TaskManager.hpp` (modified) — `TaskDraft`, `TaskPatch`, and `Statistics` structures,
  `reopenTask()`, `getStatistics()`, `getStoragePath()`
- `CommandParser.hpp` (modified) — command types `Reopen`, `Stats`, `Menu`, and fields for
  due date, tags, tag filter, overdue filter, and color
- `Console.hpp` (new) — color API and column padding that ignores escape sequences
- `View.hpp` (new) — declarations of every user-facing output function
- `Menu.hpp` (new) — the `Menu` class of the interactive mode
- `Paths.hpp` (new) — `resolveDataPath()`, the location of the tasks file

Sources, `src/`:

- `main.cpp` (modified) — reads the Windows command line as UTF-16 and converts it to UTF-8
- `Task.cpp` (modified) — due dates, tags, `isOverdue()`, and tag parsing
- `TaskManager.cpp` (modified) — due dates and tags in save/load with backward
  compatibility, statistics, `reopenTask()`, patch-based update, search by tag
- `CommandParser.cpp` (modified) — parsing of `--due`, `--tag`, `--overdue`, `--no-color`,
  `--sort due`, and the new commands
- `CLI.cpp` (modified) — reduced to command dispatch, filtering, and sorting; printing
  moved to `View`
- `Console.cpp` (new) — ANSI colors, Windows virtual terminal, and terminal detection
- `View.cpp` (new) — task list, task details, statistics, help, and error messages
- `Menu.cpp` (new) — the interactive loop with nine menu items and delete confirmation
- `Paths.cpp` (new) — looks for the project root next to the executable and honours
  `TASKMANAGER_DATA`

Tests:

- `tests/TaskManagerTests.cpp` (modified) — 18 new checks: due dates, tags, statistics,
  `reopen`, and loading of the old six-field format (41 checks in total)

Build and documentation:

- `CMakeLists.txt` (modified) — new source files in the library, `shell32` linked on Windows
- `.vscode/tasks.json` (modified) — build tasks list all sources
- `.vscode/settings.json` (modified) — the C/C++ Compile Run extension compiles all sources
  instead of the single open file
- `README.md` (modified) — menu, new commands and options, tasks file location, and the
  updated architecture diagram
- `data/.gitkeep` (modified) — two lines, `/build` and `/.vscode`, were added; they have no
  effect here, because this file only keeps the empty `data/` directory in git, and ignore
  rules belong in `.gitignore`

## v1.0

### Added

- Task model with id, title, description, status, priority, and creation date
- `TaskManager` with add, delete, update, start, complete, find, and search
- CLI commands: `add`, `list`, `show`, `start`, `done`, `update`, `delete`, `search`, `help`
- Status flow `TODO` -> `IN_PROGRESS` -> `DONE` with invalid transitions rejected
- Filters by status and priority, sorting by id, priority, date, and title
- Persistence in `data/tasks.txt`
- Error handling for unknown commands, missing arguments, invalid ids, and missing tasks
- Unit tests, `CMakeLists.txt`, and editor build tasks in `.vscode/`
