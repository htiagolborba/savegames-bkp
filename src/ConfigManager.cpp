// =============================================================================
// Game Save Backup Tool
// Version:  0.4.0
// Created:  2026-05-13
// Author:   Hiran Tiago Lins Borba
//
// File:     ConfigManager.cpp
// Purpose:  Configuration manager implementation. Reads and writes config.json
//           using nlohmann/json to persist user settings between sessions.
//
// History:
//   0.4.0  2026-05-16  Preview selection, save sizes, open source folder, known-only mode
//   0.3.0  2026-05-16  About dialog, professional headers and versioning
//   0.2.0  2026-05-15  Repack save scanning (CODEX, RUNE, Goldberg)
//   0.1.0  2026-05-14  Expanded to 94 games, Steam multi-drive detection
//   0.0.1  2026-05-13  Initial beta release
// =============================================================================

#include "ConfigManager.h"
#include "Utils.h"
#include "json.hpp"
#include <fstream>

using json = nlohmann::json;

ConfigManager::ConfigManager() {
    m_configPath = Utils::getExeDirectory() + L"\\config.json";
}

void ConfigManager::load() {
    std::ifstream ifs(m_configPath);
    if (!ifs.is_open()) return;

    try {
        json j;
        ifs >> j;
        if (j.contains("lastBackupFolder")) {
            m_lastBackupFolder =
                Utils::stringToWstring(j["lastBackupFolder"].get<std::string>());
        }
    } catch (...) {
        // Ignore parse errors — will use defaults
    }
}

void ConfigManager::save() {
    json j;
    j["lastBackupFolder"] = Utils::wstringToString(m_lastBackupFolder);

    std::ofstream ofs(m_configPath);
    if (ofs.is_open()) {
        ofs << j.dump(2);
    }
}

std::wstring ConfigManager::getLastBackupFolder() const {
    return m_lastBackupFolder;
}

void ConfigManager::setLastBackupFolder(const std::wstring& folder) {
    m_lastBackupFolder = folder;
}
