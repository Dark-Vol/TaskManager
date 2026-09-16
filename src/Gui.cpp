#include "taskmanager/Gui.hpp"

#ifndef _WIN32

#include <iostream>

int runGui(TaskManager&)
{
    std::cerr << "Error: The window is only available on Windows. "
                 "Use 'taskmanager menu' or the single commands.\n";
    return 1;
}

#else

#include "taskmanager/Encoding.hpp"

// Must come before the Windows headers: it makes macros such as
// ListView_SetItemText and IDC_ARROW resolve to their wide-character variants.
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <commctrl.h>

#include <algorithm>
#include <string>
#include <vector>

namespace
{
    // Control identifiers of the main window.
    enum : int
    {
        IdList = 1000,
        IdSearch,
        IdAdd,
        IdEdit,
        IdStart,
        IdDone,
        IdReopen,
        IdDelete,
        IdReload,
        IdStatusBar
    };

    // Control identifiers of the task dialog.
    enum : int
    {
        IdFormTitle = 2000,
        IdFormDescription,
        IdFormPriority,
        IdFormDue,
        IdFormTags,
        IdFormOk,
        IdFormCancel
    };

    constexpr wchar_t kMainClass[] = L"TaskManagerMainWindow";
    constexpr wchar_t kFormClass[] = L"TaskManagerTaskForm";

    constexpr int kButtonWidth = 96;
    constexpr int kButtonHeight = 28;
    constexpr int kMargin = 10;
    constexpr int kStatusHeight = 24;

    // Values shown in the create/edit dialog. Wide strings, because they are
    // written straight into the Windows controls.
    struct TaskForm
    {
        std::wstring caption;
        std::wstring title;
        std::wstring description;
        Priority priority = Priority::Medium;
        std::wstring due;
        std::wstring tags;
        bool accepted = false;
    };

    struct MainState
    {
        TaskManager* manager = nullptr;
        HWND list = nullptr;
        HWND search = nullptr;
        HWND status = nullptr;
        std::vector<HWND> buttons;
        // Task ids in the same order as the rows, so the selected row maps to a task.
        std::vector<int> visibleIds;
    };

    void applyDefaultFont(HWND window)
    {
        auto font = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        SendMessageW(window, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    }

    HWND createButton(HWND parent, int id, const wchar_t* text)
    {
        HWND button = CreateWindowExW(0, L"BUTTON", text,
                                      WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                      0, 0, kButtonWidth, kButtonHeight,
                                      parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                                      nullptr, nullptr);
        applyDefaultFont(button);
        return button;
    }

    std::wstring readText(HWND control)
    {
        const int length = GetWindowTextLengthW(control);
        if (length <= 0)
        {
            return {};
        }

        std::wstring text(static_cast<std::size_t>(length) + 1, L'\0');
        const int copied = GetWindowTextW(control, text.data(), length + 1);
        text.resize(static_cast<std::size_t>(std::max(copied, 0)));
        return text;
    }

    std::wstring trim(std::wstring text)
    {
        const auto first = text.find_first_not_of(L" \t\r\n");
        if (first == std::wstring::npos)
        {
            return {};
        }

        const auto last = text.find_last_not_of(L" \t\r\n");
        return text.substr(first, last - first + 1);
    }

    void showMessage(HWND parent, const std::wstring& text, UINT icon)
    {
        MessageBoxW(parent, text.c_str(), L"Task Manager", MB_OK | icon);
    }

    // ---------------------------------------------------------------- dialog

    LRESULT CALLBACK formProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

    void createFormControls(HWND window, TaskForm& form)
    {
        const int labelWidth = 90;
        const int fieldLeft = kMargin + labelWidth;
        const int fieldWidth = 320;
        int top = kMargin;

        const auto addLabel = [&](const wchar_t* text, int height) {
            HWND label = CreateWindowExW(0, L"STATIC", text, WS_CHILD | WS_VISIBLE,
                                         kMargin, top + 3, labelWidth, 20, window,
                                         nullptr, nullptr, nullptr);
            applyDefaultFont(label);
            (void)height;
        };

        addLabel(L"Title:", 24);
        HWND title = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", form.title.c_str(),
                                     WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                     fieldLeft, top, fieldWidth, 24, window,
                                     reinterpret_cast<HMENU>(IdFormTitle), nullptr, nullptr);
        applyDefaultFont(title);
        top += 34;

        addLabel(L"Description:", 60);
        HWND description = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", form.description.c_str(),
                                          WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL |
                                              ES_MULTILINE | ES_AUTOVSCROLL,
                                          fieldLeft, top, fieldWidth, 60, window,
                                          reinterpret_cast<HMENU>(IdFormDescription), nullptr,
                                          nullptr);
        applyDefaultFont(description);
        top += 70;

        addLabel(L"Priority:", 24);
        HWND priority = CreateWindowExW(0, L"COMBOBOX", L"",
                                        WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST,
                                        fieldLeft, top, 140, 24, window,
                                        reinterpret_cast<HMENU>(IdFormPriority), nullptr, nullptr);
        applyDefaultFont(priority);
        SendMessageW(priority, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"LOW"));
        SendMessageW(priority, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"MEDIUM"));
        SendMessageW(priority, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"HIGH"));
        SendMessageW(priority, CB_SETCURSEL, static_cast<WPARAM>(priorityRank(form.priority) - 1), 0);
        top += 34;

        addLabel(L"Due (YYYY-MM-DD):", 24);
        HWND due = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", form.due.c_str(),
                                   WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                   fieldLeft, top, 140, 24, window,
                                   reinterpret_cast<HMENU>(IdFormDue), nullptr, nullptr);
        applyDefaultFont(due);
        top += 34;

        addLabel(L"Tags:", 24);
        HWND tags = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", form.tags.c_str(),
                                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                    fieldLeft, top, fieldWidth, 24, window,
                                    reinterpret_cast<HMENU>(IdFormTags), nullptr, nullptr);
        applyDefaultFont(tags);
        top += 44;

        HWND ok = CreateWindowExW(0, L"BUTTON", L"OK",
                                  WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                                  fieldLeft, top, kButtonWidth, kButtonHeight, window,
                                  reinterpret_cast<HMENU>(IdFormOk), nullptr, nullptr);
        applyDefaultFont(ok);

        HWND cancel = CreateWindowExW(0, L"BUTTON", L"Cancel",
                                      WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                      fieldLeft + kButtonWidth + kMargin, top, kButtonWidth,
                                      kButtonHeight, window,
                                      reinterpret_cast<HMENU>(IdFormCancel), nullptr, nullptr);
        applyDefaultFont(cancel);

        SetFocus(title);
    }

    // Reads the controls back into the form and validates the input.
    bool collectForm(HWND window, TaskForm& form)
    {
        const std::wstring title = trim(readText(GetDlgItem(window, IdFormTitle)));
        if (title.empty())
        {
            showMessage(window, L"Title cannot be empty.", MB_ICONWARNING);
            return false;
        }

        const std::wstring due = trim(readText(GetDlgItem(window, IdFormDue)));
        if (!due.empty())
        {
            std::chrono::system_clock::time_point parsed{};
            if (!parseDate(toUtf8(due), parsed))
            {
                showMessage(window, L"Invalid date. Use the YYYY-MM-DD format.", MB_ICONWARNING);
                return false;
            }
        }

        const LRESULT selected = SendMessageW(GetDlgItem(window, IdFormPriority), CB_GETCURSEL, 0, 0);

        form.title = title;
        form.description = trim(readText(GetDlgItem(window, IdFormDescription)));
        form.due = due;
        form.tags = trim(readText(GetDlgItem(window, IdFormTags)));
        form.priority = selected == 0 ? Priority::Low
                                      : (selected == 2 ? Priority::High : Priority::Medium);
        form.accepted = true;
        return true;
    }

    LRESULT CALLBACK formProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        auto* form = reinterpret_cast<TaskForm*>(GetWindowLongPtrW(window, GWLP_USERDATA));

        switch (message)
        {
        case WM_CREATE:
        {
            auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
            form = static_cast<TaskForm*>(create->lpCreateParams);
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(form));
            createFormControls(window, *form);
            return 0;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
            case IdFormOk:
                if (form != nullptr && collectForm(window, *form))
                {
                    DestroyWindow(window);
                }
                return 0;

            case IdFormCancel:
                DestroyWindow(window);
                return 0;

            default:
                break;
            }
            break;

        case WM_CLOSE:
            DestroyWindow(window);
            return 0;

        case WM_DESTROY:
            // Leaves the modal loop started by showTaskForm().
            PostQuitMessage(0);
            return 0;

        default:
            break;
        }

        return DefWindowProcW(window, message, wParam, lParam);
    }

    // Shows the dialog and blocks until the user closes it. Returns true when OK
    // was pressed and the input passed validation.
    bool showTaskForm(HWND parent, TaskForm& form)
    {
        const int width = 470;
        const int height = 330;

        RECT parentRect{};
        GetWindowRect(parent, &parentRect);
        const int x = parentRect.left + ((parentRect.right - parentRect.left) - width) / 2;
        const int y = parentRect.top + ((parentRect.bottom - parentRect.top) - height) / 2;

        HWND dialog = CreateWindowExW(WS_EX_DLGMODALFRAME, kFormClass, form.caption.c_str(),
                                      WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
                                      x, y, width, height, parent, nullptr, nullptr, &form);
        if (dialog == nullptr)
        {
            return false;
        }

        // Own message loop makes the dialog modal without a resource file.
        EnableWindow(parent, FALSE);

        MSG message{};
        while (GetMessageW(&message, nullptr, 0, 0) > 0)
        {
            if (IsDialogMessageW(dialog, &message))
            {
                continue;
            }

            TranslateMessage(&message);
            DispatchMessageW(&message);
        }

        EnableWindow(parent, TRUE);
        SetForegroundWindow(parent);
        return form.accepted;
    }

    // ------------------------------------------------------------ main window

    MainState* stateOf(HWND window)
    {
        return reinterpret_cast<MainState*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    }

    void setColumn(HWND list, int index, const wchar_t* title, int width)
    {
        LVCOLUMNW column{};
        column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        column.iSubItem = index;
        column.pszText = const_cast<wchar_t*>(title);
        column.cx = width;
        ListView_InsertColumn(list, index, &column);
    }

    std::wstring dueText(const Task& task)
    {
        if (!task.getDueDate().has_value())
        {
            return L"-";
        }

        return toWide(formatDate(*task.getDueDate()));
    }

    void updateStatus(const MainState& state)
    {
        const Statistics stats = state.manager->getStatistics();

        std::wstring text = L"Total: " + std::to_wstring(stats.total) +
                            L"    TODO: " + std::to_wstring(stats.todo) +
                            L"    IN_PROGRESS: " + std::to_wstring(stats.inProgress) +
                            L"    DONE: " + std::to_wstring(stats.done) +
                            L"    Overdue: " + std::to_wstring(stats.overdue) +
                            L"    File: " + toWide(state.manager->getStoragePath());

        SetWindowTextW(state.status, text.c_str());
    }

    // Refills the table, keeping the search box as a filter and remembering which
    // task id belongs to which row.
    void refreshList(HWND window, int idToSelect = 0)
    {
        MainState* state = stateOf(window);
        if (state == nullptr)
        {
            return;
        }

        const std::string query = toUtf8(trim(readText(state->search)));
        const std::vector<Task> tasks =
            query.empty() ? state->manager->getTasks() : state->manager->searchTasks(query);

        SendMessageW(state->list, WM_SETREDRAW, FALSE, 0);
        ListView_DeleteAllItems(state->list);
        state->visibleIds.clear();

        int row = 0;
        for (const Task& task : tasks)
        {
            std::wstring id = std::to_wstring(task.getId());

            LVITEMW item{};
            item.mask = LVIF_TEXT | LVIF_PARAM;
            item.iItem = row;
            item.pszText = id.data();
            item.lParam = task.getId();
            ListView_InsertItem(state->list, &item);

            const std::wstring status = toWide(toString(task.getStatus()));
            const std::wstring priority = toWide(toString(task.getPriority()));
            const std::wstring due = dueText(task);
            const std::wstring tags = task.getTags().empty() ? L"-" : toWide(joinTags(task.getTags()));
            const std::wstring title = toWide(task.getTitle());

            ListView_SetItemText(state->list, row, 1, const_cast<wchar_t*>(status.c_str()));
            ListView_SetItemText(state->list, row, 2, const_cast<wchar_t*>(priority.c_str()));
            ListView_SetItemText(state->list, row, 3, const_cast<wchar_t*>(due.c_str()));
            ListView_SetItemText(state->list, row, 4, const_cast<wchar_t*>(tags.c_str()));
            ListView_SetItemText(state->list, row, 5, const_cast<wchar_t*>(title.c_str()));

            state->visibleIds.push_back(task.getId());
            ++row;
        }

        SendMessageW(state->list, WM_SETREDRAW, TRUE, 0);
        InvalidateRect(state->list, nullptr, TRUE);

        if (idToSelect > 0)
        {
            const auto found = std::find(state->visibleIds.begin(), state->visibleIds.end(), idToSelect);
            if (found != state->visibleIds.end())
            {
                const int index = static_cast<int>(std::distance(state->visibleIds.begin(), found));
                ListView_SetItemState(state->list, index, LVIS_SELECTED | LVIS_FOCUSED,
                                      LVIS_SELECTED | LVIS_FOCUSED);
                ListView_EnsureVisible(state->list, index, FALSE);
            }
        }

        updateStatus(*state);
    }

    int selectedTaskId(const MainState& state)
    {
        const int index = ListView_GetNextItem(state.list, -1, LVNI_SELECTED);
        if (index < 0 || static_cast<std::size_t>(index) >= state.visibleIds.size())
        {
            return 0;
        }

        return state.visibleIds[static_cast<std::size_t>(index)];
    }

    bool requireSelection(HWND window, const MainState& state, int& id)
    {
        id = selectedTaskId(state);
        if (id == 0)
        {
            showMessage(window, L"Select a task first.", MB_ICONINFORMATION);
            return false;
        }

        return true;
    }

    void reportFailure(HWND window, OperationResult result, const Task* task,
                       const std::wstring& action)
    {
        if (result == OperationResult::NotFound)
        {
            showMessage(window, L"The task no longer exists.", MB_ICONWARNING);
            return;
        }

        const std::wstring status = task != nullptr ? toWide(toString(task->getStatus())) : L"?";
        showMessage(window, L"The task cannot be " + action + L" from " + status + L".",
                    MB_ICONWARNING);
    }

    void onAdd(HWND window)
    {
        MainState* state = stateOf(window);

        TaskForm form;
        form.caption = L"New task";

        if (!showTaskForm(window, form))
        {
            return;
        }

        TaskDraft draft;
        draft.title = toUtf8(form.title);
        draft.description = toUtf8(form.description);
        draft.priority = form.priority;
        draft.tags = splitTags(toUtf8(form.tags));

        if (!form.due.empty())
        {
            std::chrono::system_clock::time_point due{};
            if (parseDate(toUtf8(form.due), due))
            {
                draft.dueDate = due;
            }
        }

        const int id = state->manager->addTask(draft);
        refreshList(window, id);
    }

    void onEdit(HWND window)
    {
        MainState* state = stateOf(window);

        int id = 0;
        if (!requireSelection(window, *state, id))
        {
            return;
        }

        const Task* task = state->manager->findTask(id);
        if (task == nullptr)
        {
            showMessage(window, L"The task no longer exists.", MB_ICONWARNING);
            refreshList(window);
            return;
        }

        TaskForm form;
        form.caption = L"Task #" + std::to_wstring(id);
        form.title = toWide(task->getTitle());
        form.description = toWide(task->getDescription());
        form.priority = task->getPriority();
        form.tags = toWide(joinTags(task->getTags()));
        if (task->getDueDate().has_value())
        {
            form.due = toWide(formatDate(*task->getDueDate()));
        }

        if (!showTaskForm(window, form))
        {
            return;
        }

        TaskPatch patch;
        patch.title = toUtf8(form.title);
        patch.description = toUtf8(form.description);
        patch.priority = form.priority;
        patch.tags = splitTags(toUtf8(form.tags));

        if (form.due.empty())
        {
            patch.clearDueDate = true;
        }
        else
        {
            std::chrono::system_clock::time_point due{};
            if (parseDate(toUtf8(form.due), due))
            {
                patch.dueDate = due;
            }
        }

        state->manager->updateTask(id, patch);
        refreshList(window, id);
    }

    void onTransition(HWND window, int command)
    {
        MainState* state = stateOf(window);

        int id = 0;
        if (!requireSelection(window, *state, id))
        {
            return;
        }

        const Task* task = state->manager->findTask(id);

        OperationResult result = OperationResult::NotFound;
        std::wstring action;

        switch (command)
        {
        case IdStart:
            result = state->manager->startTask(id);
            action = L"started";
            break;
        case IdDone:
            result = state->manager->completeTask(id);
            action = L"completed";
            break;
        default:
            result = state->manager->reopenTask(id);
            action = L"reopened";
            break;
        }

        if (result != OperationResult::Success)
        {
            reportFailure(window, result, task, action);
            return;
        }

        refreshList(window, id);
    }

    void onDelete(HWND window)
    {
        MainState* state = stateOf(window);

        int id = 0;
        if (!requireSelection(window, *state, id))
        {
            return;
        }

        const Task* task = state->manager->findTask(id);
        if (task == nullptr)
        {
            refreshList(window);
            return;
        }

        const std::wstring question = L"Delete task #" + std::to_wstring(id) + L" \"" +
                                     toWide(task->getTitle()) + L"\"?";

        if (MessageBoxW(window, question.c_str(), L"Task Manager",
                        MB_YESNO | MB_ICONQUESTION) != IDYES)
        {
            return;
        }

        state->manager->removeTask(id);
        refreshList(window);
    }

    void layout(HWND window)
    {
        MainState* state = stateOf(window);
        if (state == nullptr)
        {
            return;
        }

        RECT client{};
        GetClientRect(window, &client);

        int left = kMargin;
        for (HWND button : state->buttons)
        {
            MoveWindow(button, left, kMargin, kButtonWidth, kButtonHeight, TRUE);
            left += kButtonWidth + 6;
        }

        const int searchTop = kMargin + kButtonHeight + kMargin;
        MoveWindow(state->search, kMargin + 60, searchTop, 260, 24, TRUE);

        const int listTop = searchTop + 24 + kMargin;
        const int listHeight = client.bottom - listTop - kStatusHeight - kMargin;
        MoveWindow(state->list, kMargin, listTop, client.right - 2 * kMargin,
                   std::max(listHeight, 40), TRUE);

        MoveWindow(state->status, kMargin, client.bottom - kStatusHeight,
                   client.right - 2 * kMargin, 20, TRUE);
    }

    void createMainControls(HWND window, MainState& state)
    {
        state.buttons.push_back(createButton(window, IdAdd, L"Add"));
        state.buttons.push_back(createButton(window, IdEdit, L"Edit"));
        state.buttons.push_back(createButton(window, IdStart, L"Start"));
        state.buttons.push_back(createButton(window, IdDone, L"Done"));
        state.buttons.push_back(createButton(window, IdReopen, L"Reopen"));
        state.buttons.push_back(createButton(window, IdDelete, L"Delete"));
        state.buttons.push_back(createButton(window, IdReload, L"Reload"));

        HWND searchLabel = CreateWindowExW(0, L"STATIC", L"Search:", WS_CHILD | WS_VISIBLE,
                                           kMargin, kMargin + kButtonHeight + kMargin + 4, 55, 20,
                                           window, nullptr, nullptr, nullptr);
        applyDefaultFont(searchLabel);

        state.search = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                                       WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                       0, 0, 260, 24, window,
                                       reinterpret_cast<HMENU>(IdSearch), nullptr, nullptr);
        applyDefaultFont(state.search);

        state.list = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
                                     WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT |
                                         LVS_SINGLESEL | LVS_SHOWSELALWAYS,
                                     0, 0, 100, 100, window,
                                     reinterpret_cast<HMENU>(IdList), nullptr, nullptr);
        applyDefaultFont(state.list);
        ListView_SetExtendedListViewStyle(state.list, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

        setColumn(state.list, 0, L"ID", 50);
        setColumn(state.list, 1, L"STATUS", 110);
        setColumn(state.list, 2, L"PRIORITY", 90);
        setColumn(state.list, 3, L"DUE", 100);
        setColumn(state.list, 4, L"TAGS", 140);
        setColumn(state.list, 5, L"TITLE", 320);

        state.status = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP,
                                       0, 0, 100, 20, window,
                                       reinterpret_cast<HMENU>(IdStatusBar), nullptr, nullptr);
        applyDefaultFont(state.status);
    }

    // Paints overdue rows red, the same idea as the colored console output.
    LRESULT handleCustomDraw(HWND window, NMLVCUSTOMDRAW* draw)
    {
        MainState* state = stateOf(window);

        switch (draw->nmcd.dwDrawStage)
        {
        case CDDS_PREPAINT:
            return CDRF_NOTIFYITEMDRAW;

        case CDDS_ITEMPREPAINT:
        {
            const auto id = static_cast<int>(draw->nmcd.lItemlParam);
            const Task* task = state->manager->findTask(id);

            if (task != nullptr && task->isOverdue(std::chrono::system_clock::now()))
            {
                draw->clrText = RGB(200, 0, 0);
            }

            return CDRF_DODEFAULT;
        }

        default:
            return CDRF_DODEFAULT;
        }
    }

    LRESULT CALLBACK mainProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch (message)
        {
        case WM_CREATE:
        {
            auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
            auto* state = static_cast<MainState*>(create->lpCreateParams);
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));

            createMainControls(window, *state);
            layout(window);
            refreshList(window);
            return 0;
        }

        case WM_SIZE:
            layout(window);
            return 0;

        case WM_COMMAND:
        {
            const int id = LOWORD(wParam);

            // Typing in the search box filters the table right away.
            if (id == IdSearch && HIWORD(wParam) == EN_CHANGE)
            {
                refreshList(window);
                return 0;
            }

            switch (id)
            {
            case IdAdd:
                onAdd(window);
                return 0;
            case IdEdit:
                onEdit(window);
                return 0;
            case IdStart:
            case IdDone:
            case IdReopen:
                onTransition(window, id);
                return 0;
            case IdDelete:
                onDelete(window);
                return 0;
            case IdReload:
                // Picks up changes made by the console commands meanwhile.
                stateOf(window)->manager->load();
                refreshList(window);
                return 0;
            default:
                break;
            }
            break;
        }

        case WM_NOTIFY:
        {
            auto* header = reinterpret_cast<NMHDR*>(lParam);
            if (header->idFrom != IdList)
            {
                break;
            }

            if (header->code == NM_DBLCLK)
            {
                onEdit(window);
                return 0;
            }

            if (header->code == NM_CUSTOMDRAW)
            {
                return handleCustomDraw(window, reinterpret_cast<NMLVCUSTOMDRAW*>(lParam));
            }

            break;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        default:
            break;
        }

        return DefWindowProcW(window, message, wParam, lParam);
    }

    bool registerClasses(HINSTANCE instance)
    {
        WNDCLASSEXW mainClass{};
        mainClass.cbSize = sizeof(mainClass);
        mainClass.lpfnWndProc = mainProc;
        mainClass.hInstance = instance;
        mainClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        mainClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
        mainClass.lpszClassName = kMainClass;
        mainClass.hIcon = LoadIconW(nullptr, IDI_APPLICATION);

        WNDCLASSEXW formClass = mainClass;
        formClass.lpfnWndProc = formProc;
        formClass.lpszClassName = kFormClass;

        return RegisterClassExW(&mainClass) != 0 && RegisterClassExW(&formClass) != 0;
    }

    // When the program is started by double-clicking, Windows creates a console
    // just for it. That console is useless next to the window, so it is hidden.
    void hideOwnConsole()
    {
        DWORD processes[2]{};
        if (GetConsoleProcessList(processes, 2) == 1)
        {
            ShowWindow(GetConsoleWindow(), SW_HIDE);
        }
    }
}

int runGui(TaskManager& manager)
{
    hideOwnConsole();

    INITCOMMONCONTROLSEX controls{};
    controls.dwSize = sizeof(controls);
    controls.dwICC = ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&controls);

    HINSTANCE instance = GetModuleHandleW(nullptr);
    if (!registerClasses(instance))
    {
        return 1;
    }

    MainState state;
    state.manager = &manager;

    HWND window = CreateWindowExW(0, kMainClass, L"Task Manager",
                                  WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                  CW_USEDEFAULT, CW_USEDEFAULT, 960, 560,
                                  nullptr, nullptr, instance, &state);
    if (window == nullptr)
    {
        return 1;
    }

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0)
    {
        if (IsDialogMessageW(window, &message))
        {
            continue;
        }

        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    return 0;
}

#endif
