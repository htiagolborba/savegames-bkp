// =============================================================================
// Game Save Backup Tool
// Version:  0.3.0
// Created:  2026-05-13
// Author:   Hiran Tiago Lins Borba
//
// File:     Utils.cpp
// Purpose:  Utility function implementations. Handles Win32 environment
//           variable expansion, wildcard directory enumeration, recursive
//           file copy with timestamp preservation, and UTF-8/UTF-16 conversion.
//
// History:
//   0.3.0  2026-05-16  About dialog, professional headers and versioning
//   0.2.0  2026-05-15  Repack save scanning (CODEX, RUNE, Goldberg)
//   0.1.0  2026-05-14  Expanded to 94 games, Steam multi-drive detection
//   0.0.1  2026-05-13  Initial beta release
// =============================================================================

#include "Utils.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Shlwapi.h>
#include <ShlObj.h>

#include <chrono>
#include <sstream>
#include <iomanip>
#include <algorithm>

#pragma comment(lib, "Shlwapi.lib")

namespace Utils {

// ---------------------------------------------------------------------------
// String conversions
// ---------------------------------------------------------------------------
std::wstring stringToWstring(const std::string& str) {
    if (str.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(),
                                   static_cast<int>(str.size()), nullptr, 0);
    std::wstring result(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(),
                        static_cast<int>(str.size()), &result[0], size);
    return result;
}

std::string wstringToString(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(),
                                   static_cast<int>(wstr.size()),
                                   nullptr, 0, nullptr, nullptr);
    std::string result(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(),
                        static_cast<int>(wstr.size()),
                        &result[0], size, nullptr, nullptr);
    return result;
}

// ---------------------------------------------------------------------------
// Path operations
// ---------------------------------------------------------------------------
std::wstring expandEnvVars(const std::wstring& path) {
    wchar_t buffer[MAX_PATH * 2];
    DWORD result = ExpandEnvironmentStringsW(path.c_str(), buffer, MAX_PATH * 2);
    if (result == 0 || result > MAX_PATH * 2) return path;
    return std::wstring(buffer);
}

// Split a path by backslash separators
static std::vector<std::wstring> splitPath(const std::wstring& path) {
    std::vector<std::wstring> parts;
    std::wstring current;
    for (wchar_t c : path) {
        if (c == L'\\' || c == L'/') {
            if (!current.empty()) {
                parts.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }
    if (!current.empty()) parts.push_back(current);
    return parts;
}

// Recursively expand wildcard segments in a path
static void expandWildcardsRecursive(const std::wstring& basePath,
                                     const std::vector<std::wstring>& parts,
                                     size_t index,
                                     std::vector<std::wstring>& results) {
    if (index >= parts.size()) {
        if (directoryExists(basePath)) {
            results.push_back(basePath);
        }
        return;
    }

    const std::wstring& part = parts[index];

    if (part.find(L'*') != std::wstring::npos ||
        part.find(L'?') != std::wstring::npos) {
        // Wildcard segment — enumerate matching directories
        std::wstring searchPattern = basePath + L"\\" + part;
        WIN32_FIND_DATAW fd;
        HANDLE hFind = FindFirstFileW(searchPattern.c_str(), &fd);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    std::wstring name(fd.cFileName);
                    if (name != L"." && name != L"..") {
                        std::wstring newBase = basePath + L"\\" + name;
                        expandWildcardsRecursive(newBase, parts, index + 1, results);
                    }
                }
            } while (FindNextFileW(hFind, &fd));
            FindClose(hFind);
        }
    } else {
        // Normal segment
        std::wstring newBase = basePath + L"\\" + part;
        expandWildcardsRecursive(newBase, parts, index + 1, results);
    }
}

std::vector<std::wstring> expandWildcards(const std::wstring& path) {
    std::vector<std::wstring> results;
    std::wstring expanded = expandEnvVars(path);

    if (expanded.find(L'*') == std::wstring::npos &&
        expanded.find(L'?') == std::wstring::npos) {
        results.push_back(expanded);
        return results;
    }

    std::vector<std::wstring> parts = splitPath(expanded);
    if (parts.empty()) return results;

    // Build initial base (drive letter, e.g. "C:")
    std::wstring base;
    size_t startIndex = 0;
    if (parts[0].size() >= 2 && parts[0][1] == L':') {
        base = parts[0];
        startIndex = 1;
    }

    expandWildcardsRecursive(base, parts, startIndex, results);
    return results;
}

std::wstring normalizePath(const std::wstring& path) {
    std::wstring result = path;
    std::replace(result.begin(), result.end(), L'/', L'\\');
    // Remove trailing backslash
    while (!result.empty() && result.back() == L'\\') {
        result.pop_back();
    }
    // Lowercase for comparison
    std::transform(result.begin(), result.end(), result.begin(), ::towlower);
    return result;
}

std::wstring getExeDirectory() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    PathRemoveFileSpecW(path);
    return std::wstring(path);
}

bool directoryExists(const std::wstring& path) {
    DWORD attrs = GetFileAttributesW(path.c_str());
    return (attrs != INVALID_FILE_ATTRIBUTES &&
            (attrs & FILE_ATTRIBUTE_DIRECTORY));
}

bool createDirectoryRecursive(const std::wstring& path) {
    int result = SHCreateDirectoryExW(nullptr, path.c_str(), nullptr);
    return (result == ERROR_SUCCESS || result == ERROR_ALREADY_EXISTS);
}

bool isSubPath(const std::wstring& parent, const std::wstring& child) {
    std::wstring normParent = normalizePath(parent);
    std::wstring normChild  = normalizePath(child);
    if (normChild.length() <= normParent.length()) return false;
    return normChild.substr(0, normParent.length()) == normParent &&
           normChild[normParent.length()] == L'\\';
}

// ---------------------------------------------------------------------------
// File operations
// ---------------------------------------------------------------------------
uint64_t getFolderSize(const std::wstring& path) {
    uint64_t totalSize = 0;
    std::wstring searchPath = path + L"\\*";

    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) return 0;

    do {
        std::wstring name(fd.cFileName);
        if (name == L"." || name == L"..") continue;

        std::wstring fullPath = path + L"\\" + name;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            totalSize += getFolderSize(fullPath);
        } else {
            LARGE_INTEGER fileSize;
            fileSize.HighPart = fd.nFileSizeHigh;
            fileSize.LowPart  = fd.nFileSizeLow;
            totalSize += fileSize.QuadPart;
        }
    } while (FindNextFileW(hFind, &fd));

    FindClose(hFind);
    return totalSize;
}

bool copyDirectoryRecursive(const std::wstring& src, const std::wstring& dst,
                            std::vector<std::string>& errors) {
    if (!createDirectoryRecursive(dst)) {
        errors.push_back("Failed to create directory: " + wstringToString(dst));
        return false;
    }

    std::wstring searchPath = src + L"\\*";
    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) {
        errors.push_back("Cannot access source: " + wstringToString(src));
        return false;
    }

    bool allOk = true;

    do {
        std::wstring name(fd.cFileName);
        if (name == L"." || name == L"..") continue;

        std::wstring srcFull = src + L"\\" + name;
        std::wstring dstFull = dst + L"\\" + name;

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (!copyDirectoryRecursive(srcFull, dstFull, errors)) {
                allOk = false;
            }
        } else {
            if (!CopyFileW(srcFull.c_str(), dstFull.c_str(), TRUE)) {
                DWORD err = GetLastError();
                errors.push_back("Failed to copy: " + wstringToString(srcFull) +
                                 " (error " + std::to_string(err) + ")");
                allOk = false;
            } else {
                // Preserve original file timestamps
                HANDLE hSrc = CreateFileW(srcFull.c_str(), GENERIC_READ,
                    FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
                if (hSrc != INVALID_HANDLE_VALUE) {
                    FILETIME ftCreate, ftAccess, ftWrite;
                    if (GetFileTime(hSrc, &ftCreate, &ftAccess, &ftWrite)) {
                        HANDLE hDst = CreateFileW(dstFull.c_str(),
                            FILE_WRITE_ATTRIBUTES, 0, nullptr,
                            OPEN_EXISTING, 0, nullptr);
                        if (hDst != INVALID_HANDLE_VALUE) {
                            SetFileTime(hDst, &ftCreate, &ftAccess, &ftWrite);
                            CloseHandle(hDst);
                        }
                    }
                    CloseHandle(hSrc);
                }
            }
        }
    } while (FindNextFileW(hFind, &fd));

    FindClose(hFind);
    return allOk;
}

// ---------------------------------------------------------------------------
// Time helpers
// ---------------------------------------------------------------------------
std::string getCurrentTimestamp() {
    auto now  = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    struct tm tm_buf;
    localtime_s(&tm_buf, &time);

    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%d_%H-%M-%S");
    return oss.str();
}

std::string getCurrentTimestampReadable() {
    auto now  = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    struct tm tm_buf;
    localtime_s(&tm_buf, &time);

    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

} // namespace Utils
