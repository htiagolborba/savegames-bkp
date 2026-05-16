// =============================================================================
// Game Save Backup Tool
// Version:  1.0.0
// Created:  2026-05-13
// Author:   Hiran Tiago Lins Borba
//
// File:     BackupManager.cpp
// Purpose:  Backup engine implementation. Creates timestamped backup folders,
//           recursively copies save files preserving structure and timestamps,
//           and reports progress via callbacks.
//
// History:
//   1.0.0  2026-05-16  Stable release, GitHub Actions CI/CD, MIT License
//   0.4.0  2026-05-16  Preview selection, save sizes, open source folder, known-only mode
//   0.3.0  2026-05-16  About dialog, professional headers and versioning
//   0.2.0  2026-05-15  Repack save scanning (CODEX, RUNE, Goldberg)
//   0.1.0  2026-05-14  Expanded to 94 games, Steam multi-drive detection
//   0.0.1  2026-05-13  Initial beta release
// =============================================================================

#include "BackupManager.h"
#include "Utils.h"
#include "Logger.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

void BackupManager::setProgressCallback(ProgressCallback cb) {
    m_progressCb = cb;
}

void BackupManager::report(const std::string& game, const std::string& msg) {
    Logger::instance().log(game + ": " + msg);
    if (m_progressCb) m_progressCb(game, msg);
}

std::wstring BackupManager::createTimestampedFolder(const std::wstring& basePath,
                                                     const std::string& timestamp) {
    std::wstring tsWide = Utils::stringToWstring(timestamp);
    std::wstring folder = basePath + L"\\" + tsWide;

    if (!Utils::directoryExists(folder)) {
        Utils::createDirectoryRecursive(folder);
        return folder;
    }

    // Folder already exists — append suffix
    for (int i = 1; i < 1000; ++i) {
        std::wstring candidate = folder + L"_" + std::to_wstring(i);
        if (!Utils::directoryExists(candidate)) {
            Utils::createDirectoryRecursive(candidate);
            return candidate;
        }
    }

    // Fallback (should never happen)
    Utils::createDirectoryRecursive(folder);
    return folder;
}

std::vector<BackupResult> BackupManager::performBackup(
    const std::wstring& backupRoot,
    const std::vector<GameEntry>& games)
{
    std::vector<BackupResult> results;
    std::string timestamp = Utils::getCurrentTimestamp();

    Logger::instance().log("Starting backup at " + timestamp);

    for (auto& game : games) {
        // Only backup games that were found
        if (game.status != "Found") continue;

        BackupResult result;
        result.gameName   = game.name;
        result.sourcePath = game.sourcePath;

        report(game.name, "Starting backup...");

        // Create destination: backupRoot/GameName/savegames/timestamp/
        std::wstring gameName = Utils::stringToWstring(game.name);
        std::wstring gameDir  = backupRoot + L"\\" + gameName + L"\\savegames";

        if (!Utils::createDirectoryRecursive(gameDir)) {
            result.success = false;
            result.errors.push_back("Failed to create backup directory");
            report(game.name, "ERROR: Failed to create backup directory");
            results.push_back(result);
            continue;
        }

        std::wstring destDir = createTimestampedFolder(gameDir, timestamp);
        result.backupPath = destDir;

        // Perform recursive copy
        std::vector<std::string> copyErrors;
        bool ok = Utils::copyDirectoryRecursive(game.sourcePath, destDir, copyErrors);

        result.success = ok;
        result.errors  = copyErrors;

        if (ok) {
            report(game.name, "Backup completed successfully");
        } else {
            for (auto& err : copyErrors) {
                report(game.name, "ERROR: " + err);
            }
            if (copyErrors.empty()) {
                report(game.name, "Backup completed with some errors");
            }
        }

        results.push_back(result);
    }

    Logger::instance().log("Backup session finished");
    return results;
}
