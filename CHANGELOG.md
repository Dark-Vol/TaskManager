# Changelog

## v1.1

### Added

- Interactive menu, started when the program runs without arguments
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
- New modules: `View`, `Console`, `Menu`, `Paths`
- Tests cover due dates, tags, statistics, `reopen`, and the legacy file format

### Fixed

- Tasks file is now resolved relative to the executable and project root, so every build
  uses the same `data/tasks.txt` instead of one file per working directory
- Non-ASCII titles on Windows: the command line is read as UTF-16 and converted to UTF-8,
  so Cyrillic is no longer mangled in the output and in the tasks file
- Byte order mark in piped input is ignored by the menu

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
