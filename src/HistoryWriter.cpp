// =============================================================================
// Game Save Backup Tool
// Version:  0.3.0
// Created:  2026-05-13
// Author:   Hiran Tiago Lins Borba
//
// File:     HistoryWriter.cpp
// Purpose:  Backup history writer implementation. Creates or prepends to
//           backup-history.md with timestamped tables of backup results,
//           keeping the most recent entries at the top of the file.
//
// History:
//   0.3.0  2026-05-16  About dialog, professional headers and versioning
//   0.2.0  2026-05-15  Repack save scanning (CODEX, RUNE, Goldberg)
//   0.1.0  2026-05-14  Expanded to 94 games, Steam multi-drive detection
//   0.0.1  2026-05-13  Initial beta release
// =============================================================================

#include "HistoryWriter.h"
#include "Utils.h"
#include "Logger.h"

#include <fstream>
#include <sstream>

void HistoryWriter::writeEntry(const std::wstring& backupRoot,
                               const std::string& timestamp,
                               const std::vector<BackupResult>& results) {
    std::wstring historyPath = backupRoot + L"\\backup-history.md";
    const std::string HEADER = "# Game Save Backup History\n\n";

    // Read existing content (if any)
    std::string existingContent;
    {
        std::ifstream ifs(historyPath);
        if (ifs.is_open()) {
            std::ostringstream oss;
            oss << ifs.rdbuf();
            existingContent = oss.str();
        }
    }

    // Remove the header from existing content (we'll re-add it)
    if (existingContent.substr(0, HEADER.size()) == HEADER) {
        existingContent = existingContent.substr(HEADER.size());
    }

    // Build the new entry
    std::ostringstream entry;

    // Readable timestamp (replace _ with space, - with : in time part)
    std::string readableTs = timestamp;
    // timestamp format: YYYY-MM-DD_HH-MM-SS
    if (readableTs.size() >= 19) {
        readableTs[10] = ' ';  // _ -> space
        readableTs[13] = ':';  // - -> :
        readableTs[16] = ':';  // - -> :
    }

    entry << "## " << readableTs << "\n\n";
    entry << "Backup destination: `" << Utils::wstringToString(backupRoot) << "`\n\n";
    entry << "### Games backed up\n\n";
    entry << "| Game | Source Path | Backup Path | Status |\n";
    entry << "|------|-------------|-------------|--------|\n";

    std::vector<std::string> errorMessages;

    for (auto& r : results) {
        std::string status = r.success ? "Success" : "Failed";
        entry << "| " << r.gameName
              << " | " << Utils::wstringToString(r.sourcePath)
              << " | " << Utils::wstringToString(r.backupPath)
              << " | " << status << " |\n";

        if (!r.success) {
            for (auto& err : r.errors) {
                errorMessages.push_back("- Failed to backup " + r.gameName +
                                        "\n  - Source: " +
                                        Utils::wstringToString(r.sourcePath) +
                                        "\n  - Reason: " + err);
            }
            if (r.errors.empty()) {
                errorMessages.push_back("- Failed to backup " + r.gameName +
                                        "\n  - Source: " +
                                        Utils::wstringToString(r.sourcePath) +
                                        "\n  - Reason: Unknown error");
            }
        }
    }

    entry << "\n### Errors\n\n";
    if (errorMessages.empty()) {
        entry << "None.\n";
    } else {
        for (auto& err : errorMessages) {
            entry << err << "\n";
        }
    }
    entry << "\n---\n\n";

    // Write the complete file: header + new entry + old entries
    std::ofstream ofs(historyPath, std::ios::trunc);
    if (ofs.is_open()) {
        ofs << HEADER << entry.str() << existingContent;
        Logger::instance().log("Updated backup-history.md");
    } else {
        Logger::instance().log("ERROR: Could not write backup-history.md");
    }
}
