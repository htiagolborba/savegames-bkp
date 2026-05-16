// =============================================================================
// Game Save Backup Tool
// Version:  0.3.0
// Created:  2026-05-13
// Author:   Hiran Tiago Lins Borba
//
// File:     main.cpp
// Purpose:  Application entry point (WinMain). Initializes common controls,
//           sets DPI awareness, and launches the main application window.
//
// History:
//   0.3.0  2026-05-16  About dialog, professional headers and versioning
//   0.2.0  2026-05-15  Repack save scanning (CODEX, RUNE, Goldberg)
//   0.1.0  2026-05-14  Expanded to 94 games, Steam multi-drive detection
//   0.0.1  2026-05-13  Initial beta release
// =============================================================================

#include "App.h"

#include <CommCtrl.h>

#pragma comment(lib, "Comctl32.lib")

// Enable visual styles (modern Windows controls)
#pragma comment(linker, "\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/,
                      LPWSTR /*lpCmdLine*/, int /*nCmdShow*/) {
    // Initialize common controls (for ListView, etc.)
    INITCOMMONCONTROLSEX icc = {};
    icc.dwSize = sizeof(icc);
    icc.dwICC  = ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icc);

    // Set DPI awareness for crisp rendering
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    App app(hInstance);
    if (!app.create()) {
        MessageBoxW(nullptr, L"Failed to create application window.",
                    L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    return app.run();
}
