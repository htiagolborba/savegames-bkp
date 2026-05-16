// =============================================================================
// Game Save Backup Tool
// Version:  0.3.0
// Created:  2026-05-13
// Author:   Hiran Tiago Lins Borba
//
// File:     Logger.cpp
// Purpose:  Thread-safe logger implementation. Uses a mutex to safely append
//           timestamped messages from any thread to the log file.
//
// History:
//   0.3.0  2026-05-16  About dialog, professional headers and versioning
//   0.2.0  2026-05-15  Repack save scanning (CODEX, RUNE, Goldberg)
//   0.1.0  2026-05-14  Expanded to 94 games, Steam multi-drive detection
//   0.0.1  2026-05-13  Initial beta release
// =============================================================================

#include "Logger.h"
#include "Utils.h"
#include <fstream>

Logger& Logger::instance() {
    static Logger inst;
    return inst;
}

void Logger::setLogPath(const std::wstring& path) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_logPath = path;
}

void Logger::log(const std::string& message) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_logPath.empty()) return;

    std::ofstream ofs(m_logPath, std::ios::app);
    if (ofs.is_open()) {
        ofs << "[" << Utils::getCurrentTimestampReadable() << "] "
            << message << "\n";
    }
}
