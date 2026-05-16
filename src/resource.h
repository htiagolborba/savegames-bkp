// =============================================================================
// Game Save Backup Tool
// Version:  0.3.0
// Created:  2026-05-13
// Author:   Hiran Tiago Lins Borba
//
// File:     resource.h
// Purpose:  Resource definitions. Control IDs for Win32 UI elements and
//           custom window message identifiers for inter-thread communication.
//
// History:
//   0.3.0  2026-05-16  About dialog, professional headers and versioning
//   0.2.0  2026-05-15  Repack save scanning (CODEX, RUNE, Goldberg)
//   0.1.0  2026-05-14  Expanded to 94 games, Steam multi-drive detection
//   0.0.1  2026-05-13  Initial beta release
// =============================================================================

#pragma once

// Control IDs
#define IDC_STATIC_TITLE    100
#define IDC_BTN_CHOOSE      101
#define IDC_EDIT_PATH       102
#define IDC_BTN_SCAN        103
#define IDC_LISTVIEW        104
#define IDC_BTN_BACKUP      105
#define IDC_BTN_OPEN        106
#define IDC_EDIT_LOG        107
#define IDC_BTN_ABOUT       108

// Custom window messages
#define WM_APP_LOG          (WM_APP + 1)
#define WM_APP_SCAN_DONE    (WM_APP + 2)
#define WM_APP_BACKUP_DONE  (WM_APP + 3)
#define WM_APP_UPDATE_ITEM  (WM_APP + 4)
