// =============================================================================
// Game Save Backup Tool
// Version:  0.4.0
// Created:  2026-05-13
// Author:   Hiran Tiago Lins Borba
//
// File:     App.h
// Purpose:  Main application window class declaration. Manages the Win32 UI,
//           user interactions, and orchestrates scanning and backup operations.
//
// History:
//   0.4.0  2026-05-16  Preview selection, save sizes, open source folder, known-only mode
//   0.3.0  2026-05-16  About dialog, professional headers and versioning
//   0.2.0  2026-05-15  Repack save scanning (CODEX, RUNE, Goldberg)
//   0.1.0  2026-05-14  Expanded to 94 games, Steam multi-drive detection
//   0.0.1  2026-05-13  Initial beta release
// =============================================================================

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <CommCtrl.h>

#include "GameScanner.h"
#include "BackupManager.h"
#include "ConfigManager.h"

#include <vector>
#include <string>
#include <thread>
#include <mutex>

/// Main application window — owns all UI controls and orchestrates operations.
class App {
public:
    App(HINSTANCE hInstance);
    ~App();

    /// Create and show the main window. Returns false on failure.
    bool create();

    /// Run the message loop. Returns the exit code.
    int run();

private:
    // ---- Window procedure ----
    static LRESULT CALLBACK wndProcStatic(HWND, UINT, WPARAM, LPARAM);
    LRESULT wndProc(HWND, UINT, WPARAM, LPARAM);

    // ---- UI creation ----
    void createControls(HWND hwnd);
    void resizeControls(int width, int height);

    // ---- Event handlers ----
    void onChooseFolder();
    void onScan();
    void onBackup();
    void onOpenFolder();
    void onOpenSourceFolder();
    void onAbout();

    // ---- UI helpers ----
    void appendLog(const std::string& message);
    void populateListView();
    void updateListViewItemStatus(int index, const std::string& status);

    // ---- State ----
    HINSTANCE m_hInstance;
    HWND      m_hwnd       = nullptr;
    HFONT     m_hFontTitle = nullptr;
    HFONT     m_hFontUI    = nullptr;

    // Controls
    HWND m_lblTitle   = nullptr;
    HWND m_btnChoose  = nullptr;
    HWND m_editPath   = nullptr;
    HWND m_btnScan    = nullptr;
    HWND m_listView   = nullptr;
    HWND m_btnBackup     = nullptr;
    HWND m_btnOpen       = nullptr;
    HWND m_btnOpenSource = nullptr;
    HWND m_btnAbout      = nullptr;
    HWND m_chkKnownOnly  = nullptr;
    HWND m_editLog       = nullptr;

    // Data
    ConfigManager           m_config;
    std::vector<GameEntry>  m_games;
    std::wstring            m_backupFolder;
    std::thread             m_workerThread;
    bool                    m_isWorking = false;
};
