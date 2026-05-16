// =============================================================================
// Game Save Backup Tool
// Version:  1.0.0
// Created:  2026-05-13
// Author:   Hiran Tiago Lins Borba
//
// File:     Utils.h
// Purpose:  Utility function declarations. Provides environment variable
//           expansion, wildcard path resolution, recursive directory copy,
//           timestamp formatting, and string conversion helpers.
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
#include <vector>
#include <cstdint>

namespace Utils {

    // ---- String conversions ----
    std::wstring stringToWstring(const std::string& str);
    std::string  wstringToString(const std::wstring& wstr);

    // ---- Path operations ----
    std::wstring expandEnvVars(const std::wstring& path);
    std::vector<std::wstring> expandWildcards(const std::wstring& path);
    std::wstring normalizePath(const std::wstring& path);
    std::wstring getExeDirectory();

    bool directoryExists(const std::wstring& path);
    bool createDirectoryRecursive(const std::wstring& path);
    bool isSubPath(const std::wstring& parent, const std::wstring& child);

    // ---- File operations ----
    uint64_t getFolderSize(const std::wstring& path);
    bool copyDirectoryRecursive(const std::wstring& src, const std::wstring& dst,
                                std::vector<std::string>& errors);

    // ---- Time helpers ----
    std::string getCurrentTimestamp();       // YYYY-MM-DD_HH-mm-ss
    std::string getCurrentTimestampReadable(); // YYYY-MM-DD HH:MM:SS

    // ---- Display helpers ----
    std::string formatFileSize(uint64_t bytes); // e.g. "12.3 MB"

} // namespace Utils
