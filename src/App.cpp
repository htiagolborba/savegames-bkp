// =============================================================================
// Game Save Backup Tool
// Version:  0.3.0
// Created:  2026-05-13
// Author:   Hiran Tiago Lins Borba
//
// File:     App.cpp
// Purpose:  Main application window implementation. Handles Win32 window
//           creation, dark-themed UI controls, folder selection, game list
//           display, and threaded backup execution with real-time logging.
//
// History:
//   0.3.0  2026-05-16  About dialog, professional headers and versioning
//   0.2.0  2026-05-15  Repack save scanning (CODEX, RUNE, Goldberg)
//   0.1.0  2026-05-14  Expanded to 94 games, Steam multi-drive detection
//   0.0.1  2026-05-13  Initial beta release
// =============================================================================

#include "App.h"
#include "resource.h"
#include "Utils.h"
#include "Logger.h"
#include "HistoryWriter.h"

#include <ShlObj.h>
#include <ShellAPI.h>

#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Ole32.lib")

// Window class name
static const wchar_t* WND_CLASS = L"GameSaveBackupToolWndClass";

// ============================================================================
// Constructor / Destructor
// ============================================================================
App::App(HINSTANCE hInstance) : m_hInstance(hInstance) {}

App::~App() {
    if (m_workerThread.joinable()) m_workerThread.join();
    if (m_hFontTitle) DeleteObject(m_hFontTitle);
    if (m_hFontUI) DeleteObject(m_hFontUI);
}

// ============================================================================
// Window creation
// ============================================================================
bool App::create() {
    // Load config
    m_config.load();
    m_backupFolder = m_config.getLastBackupFolder();

    // Register window class
    WNDCLASSEXW wc   = {};
    wc.cbSize        = sizeof(WNDCLASSEXW);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = wndProcStatic;
    wc.hInstance     = m_hInstance;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(30, 30, 30));
    wc.lpszClassName = WND_CLASS;
    wc.hIcon         = LoadIcon(nullptr, IDI_APPLICATION);

    if (!RegisterClassExW(&wc)) return false;

    // Create window
    m_hwnd = CreateWindowExW(
        0, WND_CLASS, L"Game Save Backup Tool",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 900, 700,
        nullptr, nullptr, m_hInstance, this);

    if (!m_hwnd) return false;

    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);

    return true;
}

int App::run() {
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
}

// ============================================================================
// Window procedure
// ============================================================================
LRESULT CALLBACK App::wndProcStatic(HWND hwnd, UINT msg,
                                     WPARAM wParam, LPARAM lParam) {
    if (msg == WM_NCCREATE) {
        auto cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                          reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
    }

    auto* app = reinterpret_cast<App*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (app) return app->wndProc(hwnd, msg, wParam, lParam);
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT App::wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {

    case WM_CREATE:
        createControls(hwnd);
        if (!m_backupFolder.empty()) {
            SetWindowTextW(m_editPath, m_backupFolder.c_str());
        }
        appendLog("Application started. Ready.");
        return 0;

    case WM_SIZE: {
        int w = LOWORD(lParam);
        int h = HIWORD(lParam);
        resizeControls(w, h);
        return 0;
    }

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT: {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        SetTextColor(hdc, RGB(230, 230, 230));
        SetBkColor(hdc, RGB(40, 40, 40));
        static HBRUSH hBrush = CreateSolidBrush(RGB(40, 40, 40));
        return reinterpret_cast<LRESULT>(hBrush);
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_BTN_CHOOSE:  onChooseFolder(); break;
        case IDC_BTN_SCAN:    onScan();          break;
        case IDC_BTN_BACKUP:  onBackup();        break;
        case IDC_BTN_OPEN:    onOpenFolder();    break;
        case IDC_BTN_ABOUT:   onAbout();         break;
        }
        return 0;

    case WM_APP_LOG: {
        // wParam = pointer to heap-allocated string
        auto* str = reinterpret_cast<std::string*>(wParam);
        appendLog(*str);
        delete str;
        return 0;
    }

    case WM_APP_BACKUP_DONE:
        m_isWorking = false;
        EnableWindow(m_btnBackup, TRUE);
        EnableWindow(m_btnScan, TRUE);
        EnableWindow(m_btnChoose, TRUE);
        appendLog("=== Backup session complete ===");
        // Refresh statuses in list view
        populateListView();
        return 0;

    case WM_APP_UPDATE_ITEM: {
        // wParam = index, lParam = pointer to status string
        int index = static_cast<int>(wParam);
        auto* status = reinterpret_cast<std::string*>(lParam);
        updateListViewItemStatus(index, *status);
        delete status;
        return 0;
    }

    case WM_NOTIFY: {
        auto* nmhdr = reinterpret_cast<LPNMHDR>(lParam);
        if (nmhdr->idFrom == IDC_LISTVIEW) {
            if (nmhdr->code == NM_CUSTOMDRAW) {
                auto* lvcd = reinterpret_cast<LPNMLVCUSTOMDRAW>(lParam);
                switch (lvcd->nmcd.dwDrawStage) {
                case CDDS_PREPAINT:
                    return CDRF_NOTIFYITEMDRAW;
                case CDDS_ITEMPREPAINT:
                    lvcd->clrText   = RGB(220, 220, 220);
                    lvcd->clrTextBk = RGB(45, 45, 45);
                    return CDRF_NOTIFYSUBITEMDRAW;
                case CDDS_ITEMPREPAINT | CDDS_SUBITEM:
                    if (lvcd->iSubItem == 2) {
                        // Color the status column
                        wchar_t buf[64];
                        ListView_GetItemText(nmhdr->hwndFrom,
                            static_cast<int>(lvcd->nmcd.dwItemSpec), 2, buf, 64);
                        std::wstring status(buf);
                        if (status == L"Found")
                            lvcd->clrText = RGB(100, 220, 100);
                        else if (status == L"Not found")
                            lvcd->clrText = RGB(150, 150, 150);
                        else if (status == L"Backed up")
                            lvcd->clrText = RGB(80, 180, 255);
                        else if (status == L"Error")
                            lvcd->clrText = RGB(255, 100, 100);
                    }
                    return CDRF_DODEFAULT;
                }
            }
        }
        break;
    }

    case WM_GETMINMAXINFO: {
        auto* mmi = reinterpret_cast<MINMAXINFO*>(lParam);
        mmi->ptMinTrackSize.x = 700;
        mmi->ptMinTrackSize.y = 550;
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// ============================================================================
// Control creation
// ============================================================================
void App::createControls(HWND hwnd) {
    // Create fonts
    m_hFontTitle = CreateFontW(
        28, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

    m_hFontUI = CreateFontW(
        16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

    // Title
    m_lblTitle = CreateWindowExW(
        0, L"STATIC", L"\U0001F3AE  Game Save Backup Tool",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        20, 15, 500, 35,
        hwnd, reinterpret_cast<HMENU>(IDC_STATIC_TITLE), m_hInstance, nullptr);
    SendMessageW(m_lblTitle, WM_SETFONT, (WPARAM)m_hFontTitle, TRUE);

    // About button (top-right area)
    m_btnAbout = CreateWindowExW(
        0, L"BUTTON", L"About",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        780, 18, 80, 28,
        hwnd, reinterpret_cast<HMENU>(IDC_BTN_ABOUT), m_hInstance, nullptr);
    SendMessageW(m_btnAbout, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    // Choose folder button
    m_btnChoose = CreateWindowExW(
        0, L"BUTTON", L"Choose Backup Folder",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        20, 60, 180, 32,
        hwnd, reinterpret_cast<HMENU>(IDC_BTN_CHOOSE), m_hInstance, nullptr);
    SendMessageW(m_btnChoose, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    // Path display (read-only edit)
    m_editPath = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_READONLY | ES_AUTOHSCROLL,
        210, 62, 400, 28,
        hwnd, reinterpret_cast<HMENU>(IDC_EDIT_PATH), m_hInstance, nullptr);
    SendMessageW(m_editPath, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    // Scan button
    m_btnScan = CreateWindowExW(
        0, L"BUTTON", L"Scan for Savegames",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        20, 102, 180, 32,
        hwnd, reinterpret_cast<HMENU>(IDC_BTN_SCAN), m_hInstance, nullptr);
    SendMessageW(m_btnScan, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    // ListView for game entries
    m_listView = CreateWindowExW(
        WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_NOSORTHEADER,
        20, 145, 840, 280,
        hwnd, reinterpret_cast<HMENU>(IDC_LISTVIEW), m_hInstance, nullptr);
    SendMessageW(m_listView, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    // Dark background for ListView
    ListView_SetBkColor(m_listView, RGB(45, 45, 45));
    ListView_SetTextBkColor(m_listView, RGB(45, 45, 45));
    ListView_SetTextColor(m_listView, RGB(220, 220, 220));
    ListView_SetExtendedListViewStyle(m_listView,
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

    // Add columns
    LVCOLUMNW lvc = {};
    lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

    lvc.iSubItem = 0;
    lvc.cx       = 220;
    lvc.pszText  = const_cast<LPWSTR>(L"Game Name");
    ListView_InsertColumn(m_listView, 0, &lvc);

    lvc.iSubItem = 1;
    lvc.cx       = 480;
    lvc.pszText  = const_cast<LPWSTR>(L"Save Path");
    ListView_InsertColumn(m_listView, 1, &lvc);

    lvc.iSubItem = 2;
    lvc.cx       = 120;
    lvc.pszText  = const_cast<LPWSTR>(L"Status");
    ListView_InsertColumn(m_listView, 2, &lvc);

    // Backup Now button
    m_btnBackup = CreateWindowExW(
        0, L"BUTTON", L"Backup Now",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        20, 435, 140, 35,
        hwnd, reinterpret_cast<HMENU>(IDC_BTN_BACKUP), m_hInstance, nullptr);
    SendMessageW(m_btnBackup, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    // Open Backup Folder button
    m_btnOpen = CreateWindowExW(
        0, L"BUTTON", L"Open Backup Folder",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        175, 435, 180, 35,
        hwnd, reinterpret_cast<HMENU>(IDC_BTN_OPEN), m_hInstance, nullptr);
    SendMessageW(m_btnOpen, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    // Log area (multiline read-only edit)
    m_editLog = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL |
        ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
        20, 480, 840, 160,
        hwnd, reinterpret_cast<HMENU>(IDC_EDIT_LOG), m_hInstance, nullptr);
    SendMessageW(m_editLog, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);
}

void App::resizeControls(int width, int height) {
    const int PAD = 20;
    const int usableW = width - PAD * 2;

    // Path edit stretches
    int pathW = usableW - 200;
    if (pathW < 100) pathW = 100;
    MoveWindow(m_editPath, 210, 62, pathW, 28, TRUE);

    // About button stays at top-right
    MoveWindow(m_btnAbout, width - PAD - 80, 18, 80, 28, TRUE);

    // ListView
    int lvH = height - 145 - 250;
    if (lvH < 100) lvH = 100;
    MoveWindow(m_listView, PAD, 145, usableW, lvH, TRUE);

    // Buttons row
    int btnY = 145 + lvH + 10;
    MoveWindow(m_btnBackup, PAD, btnY, 140, 35, TRUE);
    MoveWindow(m_btnOpen, 175, btnY, 180, 35, TRUE);

    // Log area
    int logY = btnY + 45;
    int logH = height - logY - PAD;
    if (logH < 60) logH = 60;
    MoveWindow(m_editLog, PAD, logY, usableW, logH, TRUE);
}

// ============================================================================
// Event handlers
// ============================================================================
void App::onChooseFolder() {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    BROWSEINFOW bi = {};
    bi.hwndOwner = m_hwnd;
    bi.lpszTitle = L"Select Backup Destination Folder";
    bi.ulFlags   = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_USENEWUI;

    PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
    if (pidl) {
        wchar_t path[MAX_PATH];
        if (SHGetPathFromIDListW(pidl, path)) {
            m_backupFolder = path;
            SetWindowTextW(m_editPath, m_backupFolder.c_str());

            m_config.setLastBackupFolder(m_backupFolder);
            m_config.save();

            appendLog("Backup folder set to: " +
                      Utils::wstringToString(m_backupFolder));
        }
        CoTaskMemFree(pidl);
    }

    CoUninitialize();
}

void App::onScan() {
    if (m_isWorking) return;

    appendLog("Scanning for savegames...");
    m_games.clear();
    ListView_DeleteAllItems(m_listView);

    GameScanner scanner;

    // Scan known games
    std::wstring jsonPath = Utils::getExeDirectory() + L"\\known_games.json";
    auto known = scanner.scanKnownGames(jsonPath);

    // Scan generic locations
    appendLog("Running generic scan...");
    auto generic = scanner.scanGenericLocations();

    // Scan repack / crack emulator save locations
    appendLog("Scanning repack save locations (CODEX, RUNE, Goldberg)...");
    auto repack = scanner.scanRepackLocations();

    // Merge and deduplicate
    m_games = known;
    m_games.insert(m_games.end(), generic.begin(), generic.end());
    m_games.insert(m_games.end(), repack.begin(), repack.end());
    GameScanner::deduplicate(m_games);

    populateListView();

    int foundCount = 0;
    for (auto& g : m_games) {
        if (g.status == "Found") foundCount++;
    }

    appendLog("Scan complete. Found " + std::to_string(foundCount) +
              " save location(s) across " + std::to_string(m_games.size()) +
              " game(s).");
}

void App::onBackup() {
    if (m_isWorking) return;

    if (m_backupFolder.empty()) {
        MessageBoxW(m_hwnd,
            L"Please choose a backup folder first.",
            L"No Backup Folder", MB_OK | MB_ICONWARNING);
        return;
    }

    if (m_games.empty()) {
        MessageBoxW(m_hwnd,
            L"Please scan for savegames first.",
            L"No Games Found", MB_OK | MB_ICONWARNING);
        return;
    }

    // Check if there are any found games
    bool hasFound = false;
    for (auto& g : m_games) {
        if (g.status == "Found") { hasFound = true; break; }
    }
    if (!hasFound) {
        MessageBoxW(m_hwnd,
            L"No savegames were found to backup.",
            L"Nothing to Backup", MB_OK | MB_ICONINFORMATION);
        return;
    }

    m_isWorking = true;
    EnableWindow(m_btnBackup, FALSE);
    EnableWindow(m_btnScan, FALSE);
    EnableWindow(m_btnChoose, FALSE);

    // Set up logger
    Logger::instance().setLogPath(m_backupFolder + L"\\backup-log.txt");
    Logger::instance().log("Backup session started");

    appendLog("=== Starting backup ===");

    // Run backup on worker thread
    HWND hwnd = m_hwnd;
    std::wstring backupRoot = m_backupFolder;
    std::vector<GameEntry> games = m_games;

    if (m_workerThread.joinable()) m_workerThread.join();

    m_workerThread = std::thread([hwnd, backupRoot, games, this]() {
        BackupManager mgr;
        mgr.setProgressCallback([hwnd](const std::string& game,
                                       const std::string& msg) {
            // Post log message to UI thread
            auto* str = new std::string("[" + game + "] " + msg);
            PostMessageW(hwnd, WM_APP_LOG, reinterpret_cast<WPARAM>(str), 0);
        });

        auto results = mgr.performBackup(backupRoot, games);

        // Update game statuses
        for (size_t i = 0; i < games.size(); ++i) {
            for (auto& r : results) {
                if (r.gameName == games[i].name &&
                    r.sourcePath == games[i].sourcePath) {
                    auto* status = new std::string(r.success ? "Backed up" : "Error");
                    PostMessageW(hwnd, WM_APP_UPDATE_ITEM,
                                 static_cast<WPARAM>(i),
                                 reinterpret_cast<LPARAM>(status));
                    // Also update our local copy
                    break;
                }
            }
        }

        // Write history
        std::string timestamp = Utils::getCurrentTimestamp();
        HistoryWriter::writeEntry(backupRoot, timestamp, results);

        // Signal completion
        PostMessageW(hwnd, WM_APP_BACKUP_DONE, 0, 0);
    });
}

void App::onOpenFolder() {
    if (m_backupFolder.empty()) {
        MessageBoxW(m_hwnd,
            L"No backup folder selected.",
            L"Info", MB_OK | MB_ICONINFORMATION);
        return;
    }
    ShellExecuteW(nullptr, L"open", m_backupFolder.c_str(),
                  nullptr, nullptr, SW_SHOWNORMAL);
}

void App::onAbout() {
    std::wstring aboutText =
        L"Game Save Backup Tool\n"
        L"Version 0.3.0 (Beta)\n"
        L"\n"
        L"Created: 2026-05-13\n"
        L"Author: Hiran Tiago Lins Borba\n"
        L"\n"
        L"\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\n"
        L"\n"
        L"Protects your game saves before formatting,\n"
        L"swapping SSDs, or reinstalling Windows.\n"
        L"\n"
        L"\u2022  94+ known games with auto-detection\n"
        L"\u2022  Smart Steam detection across all drives\n"
        L"\u2022  Repack support (CODEX, RUNE, Goldberg)\n"
        L"\u2022  Generic save folder scanning\n"
        L"\u2022  Timestamped, non-destructive backups\n"
        L"\u2022  Markdown backup history\n"
        L"\n"
        L"Built with C++17 \u2022 Win32 API \u2022 nlohmann/json\n"
        L"\n"
        L"\u00A9 2026 Hiran Tiago Lins Borba. All rights reserved.";

    MessageBoxW(m_hwnd, aboutText.c_str(),
                L"About \u2014 Game Save Backup Tool",
                MB_OK | MB_ICONINFORMATION);
}

// ============================================================================
// UI helpers
// ============================================================================
void App::appendLog(const std::string& message) {
    std::wstring wmsg = Utils::stringToWstring(message) + L"\r\n";

    int len = GetWindowTextLengthW(m_editLog);
    SendMessageW(m_editLog, EM_SETSEL, len, len);
    SendMessageW(m_editLog, EM_REPLACESEL, FALSE,
                 reinterpret_cast<LPARAM>(wmsg.c_str()));

    // Auto-scroll to bottom
    SendMessageW(m_editLog, EM_SCROLLCARET, 0, 0);
}

void App::populateListView() {
    ListView_DeleteAllItems(m_listView);

    for (int i = 0; i < static_cast<int>(m_games.size()); ++i) {
        auto& game = m_games[i];

        std::wstring wName   = Utils::stringToWstring(game.name);
        std::wstring wPath   = game.sourcePath;
        std::wstring wStatus = Utils::stringToWstring(game.status);

        LVITEMW lvi = {};
        lvi.mask    = LVIF_TEXT;
        lvi.iItem   = i;
        lvi.iSubItem = 0;
        lvi.pszText = const_cast<LPWSTR>(wName.c_str());
        ListView_InsertItem(m_listView, &lvi);

        ListView_SetItemText(m_listView, i, 1,
                             const_cast<LPWSTR>(wPath.c_str()));
        ListView_SetItemText(m_listView, i, 2,
                             const_cast<LPWSTR>(wStatus.c_str()));
    }
}

void App::updateListViewItemStatus(int index, const std::string& status) {
    if (index < 0 || index >= static_cast<int>(m_games.size())) return;

    m_games[index].status = status;
    std::wstring wStatus = Utils::stringToWstring(status);
    ListView_SetItemText(m_listView, index, 2,
                         const_cast<LPWSTR>(wStatus.c_str()));
}
