// =============================================================================
// Game Save Backup Tool
// Version:  0.4.0
// Created:  2026-05-13
// Author:   Hiran Tiago Lins Borba
//
// File:     GameScanner.cpp
// Purpose:  Game save scanner implementation. Parses known_games.json with
//           environment variable and wildcard expansion, performs generic
//           keyword scanning, and deduplicates overlapping results.
//           Detects Steam on any drive via Registry and scans repack saves.
//
// History:
//   0.4.0  2026-05-16  Preview selection, save sizes, open source folder, known-only mode
//   0.3.0  2026-05-16  About dialog, professional headers and versioning
//   0.2.0  2026-05-15  Repack save scanning (CODEX, RUNE, Goldberg)
//   0.1.0  2026-05-14  Expanded to 94 games, Steam multi-drive detection
//   0.0.1  2026-05-13  Initial beta release
// =============================================================================

#include "GameScanner.h"
#include "Utils.h"
#include "Logger.h"
#include "json.hpp"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <fstream>
#include <algorithm>
#include <set>

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Steam detection — find Steam userdata on any drive
// ---------------------------------------------------------------------------
std::vector<std::wstring> GameScanner::detectSteamUserDataPaths() {
    std::set<std::wstring> found; // Use set to avoid duplicates

    auto tryAdd = [&](const std::wstring& steamRoot) {
        std::wstring udPath = steamRoot + L"\\userdata";
        if (Utils::directoryExists(udPath)) {
            found.insert(Utils::normalizePath(udPath));
            Logger::instance().log("Found Steam userdata at: " +
                                   Utils::wstringToString(udPath));
        }
    };

    // 1. Check Windows Registry (HKLM — 64-bit and 32-bit views)
    auto readRegSteamPath = [&](HKEY root, const wchar_t* subkey,
                                const wchar_t* valueName) {
        HKEY hKey;
        if (RegOpenKeyExW(root, subkey, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            wchar_t buf[MAX_PATH];
            DWORD size = sizeof(buf);
            DWORD type = 0;
            if (RegQueryValueExW(hKey, valueName, nullptr, &type,
                                 reinterpret_cast<LPBYTE>(buf), &size) == ERROR_SUCCESS) {
                if (type == REG_SZ) {
                    tryAdd(std::wstring(buf));
                }
            }
            RegCloseKey(hKey);
        }
    };

    readRegSteamPath(HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\WOW6432Node\\Valve\\Steam", L"InstallPath");
    readRegSteamPath(HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Valve\\Steam", L"InstallPath");
    readRegSteamPath(HKEY_CURRENT_USER,
        L"SOFTWARE\\Valve\\Steam", L"SteamPath");

    // 2. Check default locations
    tryAdd(Utils::expandEnvVars(L"%PROGRAMFILES(X86)%\\Steam"));
    tryAdd(Utils::expandEnvVars(L"%PROGRAMFILES%\\Steam"));

    // 3. Scan all drive letters for common Steam install patterns
    wchar_t drives[256];
    DWORD len = GetLogicalDriveStringsW(255, drives);
    for (DWORD i = 0; i < len; ) {
        std::wstring drive(&drives[i]);
        i += static_cast<DWORD>(drive.size()) + 1;

        // Remove trailing backslash (e.g., "C:\" -> "C:")
        if (!drive.empty() && drive.back() == L'\\') drive.pop_back();

        // Common Steam locations on any drive
        tryAdd(drive + L"\\Steam");
        tryAdd(drive + L"\\SteamLibrary\\Steam");
        tryAdd(drive + L"\\Program Files (x86)\\Steam");
        tryAdd(drive + L"\\Program Files\\Steam");
        tryAdd(drive + L"\\Games\\Steam");
    }

    return std::vector<std::wstring>(found.begin(), found.end());
}

// ---------------------------------------------------------------------------
// Path expansion with Steam substitution
// ---------------------------------------------------------------------------
std::vector<std::wstring> GameScanner::expandGamePath(const std::wstring& rawPath) {
    std::vector<std::wstring> results;

    // Check if this path references Steam userdata
    static const std::wstring STEAM_PATTERN = L"\\steam\\userdata";
    std::wstring lowerPath = rawPath;
    std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::towlower);

    size_t steamPos = lowerPath.find(STEAM_PATTERN);
    if (steamPos != std::wstring::npos) {
        // Extract the portion after "Steam\userdata" (e.g., "\*\2050650")
        size_t afterSteamUD = steamPos + STEAM_PATTERN.size();
        std::wstring suffix = rawPath.substr(afterSteamUD);

        // Try each detected Steam userdata path
        for (auto& steamUD : m_steamPaths) {
            std::wstring candidate = steamUD + suffix;
            auto expanded = Utils::expandWildcards(candidate);
            results.insert(results.end(), expanded.begin(), expanded.end());
        }

        // Also try the original path as-is (in case env var works)
        auto original = Utils::expandWildcards(Utils::expandEnvVars(rawPath));
        results.insert(results.end(), original.begin(), original.end());
    } else {
        // Non-Steam path — expand normally
        std::wstring expanded = Utils::expandEnvVars(rawPath);
        auto wildcardResults = Utils::expandWildcards(expanded);
        results.insert(results.end(), wildcardResults.begin(), wildcardResults.end());
    }

    // Remove duplicates
    std::set<std::wstring> unique;
    std::vector<std::wstring> deduped;
    for (auto& p : results) {
        std::wstring norm = Utils::normalizePath(p);
        if (unique.insert(norm).second) {
            deduped.push_back(p);
        }
    }
    return deduped;
}

// ---------------------------------------------------------------------------
// Known-games scan
// ---------------------------------------------------------------------------
std::vector<GameEntry> GameScanner::scanKnownGames(const std::wstring& jsonPath) {
    std::vector<GameEntry> entries;

    // Detect Steam installations across all drives
    m_steamPaths = detectSteamUserDataPaths();
    Logger::instance().log("Detected " + std::to_string(m_steamPaths.size()) +
                           " Steam userdata location(s)");

    std::ifstream ifs(jsonPath);
    if (!ifs.is_open()) {
        Logger::instance().log("Could not open known_games.json");
        return entries;
    }

    json games;
    try {
        ifs >> games;
    } catch (const std::exception& e) {
        Logger::instance().log(std::string("JSON parse error: ") + e.what());
        return entries;
    }

    for (auto& game : games) {
        std::string name = game.value("name", "Unknown");
        auto paths = game.value("paths", std::vector<std::string>{});

        bool found = false;
        for (auto& rawPath : paths) {
            std::wstring wPath = Utils::stringToWstring(rawPath);
            auto expanded = expandGamePath(wPath);

            for (auto& resolvedPath : expanded) {
                if (Utils::directoryExists(resolvedPath)) {
                    GameEntry entry;
                    entry.name       = name;
                    entry.sourcePath = resolvedPath;
                    entry.status     = "Found";
                    entries.push_back(entry);
                    found = true;
                    Logger::instance().log("Found " + name + " at " +
                                           Utils::wstringToString(resolvedPath));
                }
            }
        }

        if (!found) {
            GameEntry entry;
            entry.name   = name;
            entry.status = "Not found";
            entries.push_back(entry);
        }
    }

    return entries;
}

// ---------------------------------------------------------------------------
// Repack / crack emulator save locations
// ---------------------------------------------------------------------------
std::vector<GameEntry> GameScanner::scanRepackLocations() {
    std::vector<GameEntry> results;

    // CODEX / RUNE saves
    std::vector<std::pair<std::wstring, std::string>> repackPaths = {
        { L"C:\\Users\\Public\\Documents\\Steam\\CODEX",   "CODEX" },
        { L"C:\\Users\\Public\\Documents\\Steam\\RUNE",    "RUNE" },
        { L"C:\\Users\\Public\\Documents\\Steam\\TENOKE",  "TENOKE" },
        { L"C:\\Users\\Public\\Documents\\Steam\\PLAZA",   "PLAZA" },
    };

    for (auto& [basePath, emulator] : repackPaths) {
        if (!Utils::directoryExists(basePath)) continue;

        Logger::instance().log("Scanning " + emulator + " saves at: " +
                               Utils::wstringToString(basePath));

        // Each subfolder is a Steam App ID
        std::wstring searchPath = basePath + L"\\*";
        WIN32_FIND_DATAW fd;
        HANDLE hFind = FindFirstFileW(searchPath.c_str(), &fd);
        if (hFind == INVALID_HANDLE_VALUE) continue;

        do {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
            std::wstring name(fd.cFileName);
            if (name == L"." || name == L"..") continue;

            std::wstring fullPath = basePath + L"\\" + name;
            GameEntry entry;
            entry.name       = emulator + " App " + Utils::wstringToString(name);
            entry.sourcePath = fullPath;
            entry.status     = "Found";
            results.push_back(entry);

            Logger::instance().log("Found " + emulator + " save: App " +
                                   Utils::wstringToString(name));
        } while (FindNextFileW(hFind, &fd));

        FindClose(hFind);
    }

    // Goldberg emulator saves
    std::wstring goldbergPath = Utils::expandEnvVars(
        L"%APPDATA%\\Goldberg SteamEmu Saves");
    if (Utils::directoryExists(goldbergPath)) {
        Logger::instance().log("Scanning Goldberg saves...");

        std::wstring searchPath = goldbergPath + L"\\*";
        WIN32_FIND_DATAW fd;
        HANDLE hFind = FindFirstFileW(searchPath.c_str(), &fd);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
                std::wstring name(fd.cFileName);
                if (name == L"." || name == L"..") continue;

                std::wstring fullPath = goldbergPath + L"\\" + name;
                GameEntry entry;
                entry.name       = "Goldberg: " + Utils::wstringToString(name);
                entry.sourcePath = fullPath;
                entry.status     = "Found";
                results.push_back(entry);
            } while (FindNextFileW(hFind, &fd));
            FindClose(hFind);
        }
    }

    return results;
}

// ---------------------------------------------------------------------------
// Generic-location scan
// ---------------------------------------------------------------------------

// Save-related keywords (lowercase)
static const std::vector<std::wstring> SAVE_KEYWORDS = {
    L"save", L"saves", L"savedata", L"savegame", L"savegames",
    L"profile", L"profiles", L"userdata", L"slot",
    L"checkpoint", L"autosave"
};

bool GameScanner::isIgnoredFolder(const std::wstring& name) {
    static const std::set<std::wstring> IGNORED = {
        L"cache", L"shadercache", L"shader_cache", L"logs", L"log",
        L"temp", L"tmp", L"crash", L"crashes", L"screenshots",
        L"videos", L"movies", L"binaries", L"bin", L"engine",
        L"node_modules", L"__pycache__", L".git", L".svn",
        L"thumbnails", L"webcache", L"gpucache", L"code cache"
    };

    std::wstring lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);
    return IGNORED.count(lower) > 0;
}

void GameScanner::scanDirectory(const std::wstring& basePath, int depth,
                                int maxDepth,
                                std::vector<GameEntry>& results) {
    if (depth > maxDepth) return;

    std::wstring searchPath = basePath + L"\\*";
    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;

        std::wstring name(fd.cFileName);
        if (name == L"." || name == L"..") continue;
        if (isIgnoredFolder(name)) continue;

        std::wstring fullPath = basePath + L"\\" + name;

        // Check if folder name matches a save keyword
        std::wstring lower = name;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);

        for (auto& kw : SAVE_KEYWORDS) {
            if (lower == kw || lower.find(kw) != std::wstring::npos) {
                // Check size limit
                uint64_t size = Utils::getFolderSize(fullPath);
                if (size <= MAX_GENERIC_SIZE) {
                    // Derive a game name from the parent directory
                    std::wstring parentName = basePath.substr(
                        basePath.find_last_of(L"\\/") + 1);

                    GameEntry entry;
                    entry.name = Utils::wstringToString(parentName) +
                                 " (" + Utils::wstringToString(name) + ")";
                    entry.sourcePath = fullPath;
                    entry.status     = "Found";
                    results.push_back(entry);

                    Logger::instance().log("Generic scan found: " + entry.name +
                                           " at " + Utils::wstringToString(fullPath));
                }
                break;
            }
        }

        // Recurse deeper
        scanDirectory(fullPath, depth + 1, maxDepth, results);

    } while (FindNextFileW(hFind, &fd));

    FindClose(hFind);
}

std::vector<GameEntry> GameScanner::scanGenericLocations() {
    std::vector<GameEntry> results;

    // Common root directories to scan
    std::vector<std::wstring> roots = {
        Utils::expandEnvVars(L"%USERPROFILE%\\Documents"),
        Utils::expandEnvVars(L"%USERPROFILE%\\Saved Games"),
        Utils::expandEnvVars(L"%APPDATA%"),
        Utils::expandEnvVars(L"%LOCALAPPDATA%"),
        Utils::expandEnvVars(L"%LOCALAPPDATA%\\..\\LocalLow"),
    };

    for (auto& root : roots) {
        if (Utils::directoryExists(root)) {
            Logger::instance().log("Generic scan: " + Utils::wstringToString(root));
            scanDirectory(root, 0, 4, results);
        }
    }

    return results;
}

// ---------------------------------------------------------------------------
// Deduplication
// ---------------------------------------------------------------------------
void GameScanner::deduplicate(std::vector<GameEntry>& entries) {
    std::vector<GameEntry> result;

    for (size_t i = 0; i < entries.size(); ++i) {
        if (entries[i].status == "Not found") {
            bool hasFound = false;
            for (size_t j = 0; j < entries.size(); ++j) {
                if (i != j && entries[j].name == entries[i].name &&
                    entries[j].status == "Found") {
                    hasFound = true;
                    break;
                }
            }
            if (!hasFound) result.push_back(entries[i]);
            continue;
        }

        bool isRedundant = false;
        for (size_t j = 0; j < entries.size(); ++j) {
            if (i == j) continue;
            if (entries[j].status != "Found") continue;

            std::wstring normI = Utils::normalizePath(entries[i].sourcePath);
            std::wstring normJ = Utils::normalizePath(entries[j].sourcePath);

            if (normI == normJ && i > j) {
                isRedundant = true;
                break;
            }
            if (Utils::isSubPath(normJ, normI)) {
                isRedundant = true;
                break;
            }
        }

        if (!isRedundant) {
            result.push_back(entries[i]);
        }
    }

    entries = result;
}
