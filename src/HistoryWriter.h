// =============================================================================
// Game Save Backup Tool
// Version:  1.0.0
// Created:  2026-05-13
// Author:   Hiran Tiago Lins Borba
//
// File:     HistoryWriter.h
// Purpose:  Backup history writer declaration. Generates and updates
//           backup-history.md with Markdown-formatted session records.
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

#include "BackupManager.h"
#include <string>
#include <vector>

/// Writes and updates the backup-history.md file in the backup root.
class HistoryWriter {
public:
    /// Write a new backup session entry to backup-history.md.
    /// New entries are prepended (most recent first).
    static void writeEntry(const std::wstring& backupRoot,
                           const std::string& timestamp,
                           const std::vector<BackupResult>& results);
};
