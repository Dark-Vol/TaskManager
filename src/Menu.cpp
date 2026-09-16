#include "taskmanager/Menu.hpp"
#include "taskmanager/Console.hpp"
#include "taskmanager/View.hpp"

#include <iostream>
#include <limits>
#include <string>

namespace
{
    bool readLine(const std::string& prompt, std::string& out)
    {
        std::cout << prompt;
        std::cout.flush();

        if (!std::getline(std::cin, out))
        {
            return false;
        }

        if (!out.empty() && out.back() == '\r')
        {
            out.pop_back();
        }

        // Piped input may start with a UTF-8 byte order mark.
        if (out.rfind("\xEF\xBB\xBF", 0) == 0)
        {
            out.erase(0, 3);
        }

        return true;
    }

    std::string trim(const std::string& text)
    {
        const auto first = text.find_first_not_of(" \t");
        if (first == std::string::npos)
        {
            return {};
        }

        const auto last = text.find_last_not_of(" \t");
        return text.substr(first, last - first + 1);
    }
}

Menu::Menu(TaskManager& manager)
    : manager_(manager)
{
}

int Menu::run()
{
    std::cout << console::colorize("Task Manager", console::Color::Bold) << '\n'
              << "Tasks file: " << manager_.getStoragePath() << "\n";

    for (;;)
    {
        showMenu();

        std::string choice;
        if (!readLine("Your choice: ", choice))
        {
            std::cout << '\n';
            return 0;
        }

        choice = trim(choice);
        std::cout << '\n';

        if (choice == "0" || toLowerCopy(choice) == "q" || toLowerCopy(choice) == "exit")
        {
            view::printInfo("Bye!");
            return 0;
        }

        if (choice == "1") listTasks();
        else if (choice == "2") addTask();
        else if (choice == "3") showTask();
        else if (choice == "4") startTask();
        else if (choice == "5") completeTask();
        else if (choice == "6") updateTask();
        else if (choice == "7") deleteTask();
        else if (choice == "8") searchTasks();
        else if (choice == "9") showStatistics();
        else if (choice.empty()) continue;
        else view::printError("Unknown menu item '" + choice + "'.");

        std::cout << '\n';
    }
}

void Menu::showMenu() const
{
    std::cout << console::colorize("\n=== Menu ===", console::Color::Bold) << '\n'
              << "  1  List tasks\n"
              << "  2  Add task\n"
              << "  3  Show task\n"
              << "  4  Start task\n"
              << "  5  Complete task\n"
              << "  6  Update task\n"
              << "  7  Delete task\n"
              << "  8  Search tasks\n"
              << "  9  Statistics\n"
              << "  0  Exit\n\n";
}

void Menu::addTask()
{
    TaskDraft draft;

    std::string title;
    if (!readLine("Title: ", title))
    {
        return;
    }

    draft.title = trim(title);
    if (draft.title.empty())
    {
        view::printError("Title cannot be empty.");
        return;
    }

    std::string description;
    if (readLine("Description (optional): ", description))
    {
        draft.description = trim(description);
    }

    std::string priority;
    if (readLine("Priority [low/medium/high, default medium]: ", priority))
    {
        priority = trim(priority);
        if (!priority.empty() && !parsePriority(priority, draft.priority))
        {
            view::printError("Invalid priority. Use low, medium, or high.");
            return;
        }
    }

    std::string due;
    if (readLine("Due date [YYYY-MM-DD, optional]: ", due))
    {
        due = trim(due);
        if (!due.empty())
        {
            std::chrono::system_clock::time_point dueDate{};
            if (!parseDate(due, dueDate))
            {
                view::printError("Invalid date. Use the YYYY-MM-DD format.");
                return;
            }
            draft.dueDate = dueDate;
        }
    }

    std::string tags;
    if (readLine("Tags [comma separated, optional]: ", tags))
    {
        draft.tags = splitTags(tags);
    }

    const int id = manager_.addTask(draft);
    if (id < 0)
    {
        view::printError("Title cannot be empty.");
        return;
    }

    view::printCreated(id);
}

void Menu::listTasks() const
{
    view::printTaskList(manager_.getTasks());
}

void Menu::showTask() const
{
    int id = 0;
    if (!askId(id))
    {
        return;
    }

    const Task* task = manager_.findTask(id);
    if (task == nullptr)
    {
        view::printNotFound(id);
        return;
    }

    view::printTaskDetails(*task);
}

void Menu::startTask()
{
    int id = 0;
    if (!askId(id))
    {
        return;
    }

    const Task* task = manager_.findTask(id);
    const OperationResult result = manager_.startTask(id);

    if (result == OperationResult::NotFound)
    {
        view::printNotFound(id);
        return;
    }

    if (result == OperationResult::InvalidTransition)
    {
        view::printTransitionError(*task, "started");
        return;
    }

    view::printInfo("Task #" + std::to_string(id) + " is now IN PROGRESS.");
}

void Menu::completeTask()
{
    int id = 0;
    if (!askId(id))
    {
        return;
    }

    const Task* task = manager_.findTask(id);
    const OperationResult result = manager_.completeTask(id);

    if (result == OperationResult::NotFound)
    {
        view::printNotFound(id);
        return;
    }

    if (result == OperationResult::InvalidTransition)
    {
        view::printTransitionError(*task, "completed");
        return;
    }

    view::printInfo("Task #" + std::to_string(id) + " completed.");
}

void Menu::updateTask()
{
    int id = 0;
    if (!askId(id))
    {
        return;
    }

    if (manager_.findTask(id) == nullptr)
    {
        view::printNotFound(id);
        return;
    }

    view::printInfo("Leave a field empty to keep the current value.");

    TaskPatch patch;

    std::string title;
    if (readLine("New title: ", title))
    {
        title = trim(title);
        if (!title.empty())
        {
            patch.title = title;
        }
    }

    std::string description;
    if (readLine("New description: ", description))
    {
        description = trim(description);
        if (!description.empty())
        {
            patch.description = description;
        }
    }

    std::string priority;
    if (readLine("New priority [low/medium/high]: ", priority))
    {
        priority = trim(priority);
        if (!priority.empty())
        {
            Priority parsed = Priority::Medium;
            if (!parsePriority(priority, parsed))
            {
                view::printError("Invalid priority. Use low, medium, or high.");
                return;
            }
            patch.priority = parsed;
        }
    }

    std::string due;
    if (readLine("New due date [YYYY-MM-DD or 'none']: ", due))
    {
        due = trim(due);
        if (toLowerCopy(due) == "none")
        {
            patch.clearDueDate = true;
        }
        else if (!due.empty())
        {
            std::chrono::system_clock::time_point dueDate{};
            if (!parseDate(due, dueDate))
            {
                view::printError("Invalid date. Use the YYYY-MM-DD format.");
                return;
            }
            patch.dueDate = dueDate;
        }
    }

    std::string tags;
    if (readLine("New tags [comma separated]: ", tags))
    {
        tags = trim(tags);
        if (!tags.empty())
        {
            patch.tags = splitTags(tags);
        }
    }

    if (patch.empty())
    {
        view::printInfo("Nothing to update.");
        return;
    }

    const OperationResult result = manager_.updateTask(id, patch);
    if (result == OperationResult::NotFound)
    {
        view::printNotFound(id);
        return;
    }

    if (result == OperationResult::EmptyTitle)
    {
        view::printError("Title cannot be empty.");
        return;
    }

    view::printInfo("Task #" + std::to_string(id) + " updated.");
}

void Menu::deleteTask()
{
    int id = 0;
    if (!askId(id))
    {
        return;
    }

    const Task* task = manager_.findTask(id);
    if (task == nullptr)
    {
        view::printNotFound(id);
        return;
    }

    std::string confirmation;
    if (!readLine("Delete \"" + task->getTitle() + "\"? [y/N]: ", confirmation))
    {
        return;
    }

    const std::string answer = toLowerCopy(trim(confirmation));
    if (answer != "y" && answer != "yes")
    {
        view::printInfo("Cancelled.");
        return;
    }

    if (manager_.removeTask(id) == OperationResult::NotFound)
    {
        view::printNotFound(id);
        return;
    }

    view::printInfo("Task #" + std::to_string(id) + " deleted.");
}

void Menu::searchTasks() const
{
    std::string query;
    if (!readLine("Search: ", query))
    {
        return;
    }

    query = trim(query);
    if (query.empty())
    {
        view::printError("Search query cannot be empty.");
        return;
    }

    view::printSearchResults(manager_.searchTasks(query));
}

void Menu::showStatistics() const
{
    view::printStatistics(manager_.getStatistics());
}

bool Menu::askId(int& id) const
{
    std::string input;
    if (!readLine("Task ID: ", input))
    {
        return false;
    }

    input = trim(input);

    try
    {
        std::size_t index = 0;
        const int value = std::stoi(input, &index);
        if (index != input.size() || value <= 0)
        {
            view::printError("Invalid ID.");
            return false;
        }

        id = value;
        return true;
    }
    catch (...)
    {
        view::printError("Invalid ID.");
        return false;
    }
}
