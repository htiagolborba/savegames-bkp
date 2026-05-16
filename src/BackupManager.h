// =============================================================================
// Game Save Backup Tool
// Version:  0.4.0
// Created:  2026-05-13
// Author:   Hiran Tiago Lins Borba
//
// File:     BackupManager.h
// Purpose:  Backup engine declaration. Handles recursive file copying into
//           timestamped folders with progress reporting and error handling.
//
// History:
//   0.4.0  2026-05-16  Preview selection, save sizes, open source folder, known-only mode
//   0.3.0  2026-05-16  About dialog, professional headers and versioning
//   0.2.0  2026-05-15  Repack save scanning (CODEX, RUNE, Goldberg)
//   0.1.0  2026-05-14  Expanded to 94 games, Steam multi-drive detection
//   0.0.1  2026-05-13  Initial beta release
// =============================================================================

#pragma once

#include "GameScanner.h"
#include <string>
#include <vector>
#include <functional>

/// Result for a single game backup.
struct BackupResult {
    std::string  gameName;
    std::wstring sourcePath;
    std::wstring backupPath;
    bool         success;
    std::vector<std::string> errors;
};

/// Performs the actual file-copy backup for detected games.
class BackupManager {
public:
    /// Callback for progress reporting (game name, message).
    using ProgressCallback = std::function<void(const std::string&, const std::string&)>;

    void setProgressCallback(ProgressCallback cb);

    /// Run backup for all "Found" games into the given root folder.
    /// Returns results for each game.
    std::vector<BackupResult> performBackup(const std::wstring& backupRoot,
                                            const std::vector<GameEntry>& games);

private:
    ProgressCallback m_progressCb;

    void report(const std::string& game, const std::string& msg);

    /// Create a unique timestamped folder, appending _1, _2 etc. if needed.
    std::wstring createTimestampedFolder(const std::wstring& basePath,
                                         const std::string& timestamp);
};
