// =============================================================================
// Game Save Backup Tool
// Version:  1.0.0
// Created:  2026-05-13
// Author:   Hiran Tiago Lins Borba
//
// File:     ConfigManager.h
// Purpose:  Configuration manager declaration. Handles persistent storage of
//           user preferences (last backup folder) via config.json.
//
// History:
//   1.0.0  2026-05-16  Stable release, GitHub Actions CI/CD, MIT License
//   0.4.0  2026-05-16  Preview selection, save sizes, open source folder, known-only mode
//   0.3.0  2026-05-16  About dialog, professional headers and versioning
//   0.2.0  2026-05-15  Repack save scanning (CODEX, RUNE, Goldberg)
//   0.1.0  2026-05-14  Expanded to 94 games, Steam multi-drive detection
//   0.0.1  2026-05-13  Initial beta release
// =============================================================================

#pragma once

#include <string>

/// Manages config.json persistence (last backup folder, etc.).
class ConfigManager {
public:
    ConfigManager();

    void load();
    void save();

    std::wstring getLastBackupFolder() const;
    void         setLastBackupFolder(const std::wstring& folder);

private:
    std::wstring m_configPath;
    std::wstring m_lastBackupFolder;
};
