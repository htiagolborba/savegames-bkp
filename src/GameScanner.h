// =============================================================================
// Game Save Backup Tool
// Version:  0.3.0
// Created:  2026-05-13
// Author:   Hiran Tiago Lins Borba
//
// File:     GameScanner.h
// Purpose:  Game save scanner declaration. Detects savegame locations using a
//           known-games JSON database and generic keyword-based scanning.
//
// History:
//   0.3.0  2026-05-16  About dialog, professional headers and versioning
//   0.2.0  2026-05-15  Repack save scanning (CODEX, RUNE, Goldberg)
//   0.1.0  2026-05-14  Expanded to 94 games, Steam multi-drive detection
//   0.0.1  2026-05-13  Initial beta release
// =============================================================================

#pragma once

#include <string>
#include <vector>

/// Represents a detected game save location.
struct GameEntry {
    std::string  name;       // e.g. "Resident Evil 4 Remake"
    std::wstring sourcePath; // Full resolved path to the save folder
    std::string  status;     // "Found", "Not found", "Backed up", "Error"
};

/// Scans the filesystem for known and generic game save locations.
class GameScanner {
public:
    /// Load known_games.json and check which save paths exist.
    /// Automatically detects Steam installations on all drives.
    std::vector<GameEntry> scanKnownGames(const std::wstring& jsonPath);

    /// Scan common directories for save-related folder names.
    std::vector<GameEntry> scanGenericLocations();

    /// Scan CODEX / RUNE / Goldberg emulator save locations (for repacks).
    std::vector<GameEntry> scanRepackLocations();

    /// Remove duplicate / overlapping entries.
    static void deduplicate(std::vector<GameEntry>& entries);

    /// Detect all Steam userdata directories across all drives.
    /// Checks Windows Registry and scans drive letters.
    static std::vector<std::wstring> detectSteamUserDataPaths();

private:
    /// Max folder size (2 GB) for generic scan hits.
    static constexpr uint64_t MAX_GENERIC_SIZE = 2ULL * 1024 * 1024 * 1024;

    /// Cached Steam userdata paths (populated on first scan).
    std::vector<std::wstring> m_steamPaths;

    /// Folders to ignore during generic scanning.
    static bool isIgnoredFolder(const std::wstring& name);

    /// Recursive scan for save-keyword folders.
    void scanDirectory(const std::wstring& basePath, int depth, int maxDepth,
                       std::vector<GameEntry>& results);

    /// Expand a single JSON path, substituting Steam paths when needed.
    std::vector<std::wstring> expandGamePath(const std::wstring& rawPath);
};
