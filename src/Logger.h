// =============================================================================
// Game Save Backup Tool
// Version:  0.4.0
// Created:  2026-05-13
// Author:   Hiran Tiago Lins Borba
//
// File:     Logger.h
// Purpose:  Thread-safe logger declaration. Writes timestamped diagnostic
//           messages to backup-log.txt for troubleshooting.
//
// History:
//   0.4.0  2026-05-16  Preview selection, save sizes, open source folder, known-only mode
//   0.3.0  2026-05-16  About dialog, professional headers and versioning
//   0.2.0  2026-05-15  Repack save scanning (CODEX, RUNE, Goldberg)
//   0.1.0  2026-05-14  Expanded to 94 games, Steam multi-drive detection
//   0.0.1  2026-05-13  Initial beta release
// =============================================================================

#pragma once

#include <string>
#include <mutex>

/// Thread-safe logger that writes timestamped lines to backup-log.txt.
class Logger {
public:
    static Logger& instance();

    /// Set the path for the log file (typically inside the backup folder).
    void setLogPath(const std::wstring& path);

    /// Append a timestamped message to the log file.
    void log(const std::string& message);

private:
    Logger() = default;
    std::mutex  m_mutex;
    std::wstring m_logPath;
};
