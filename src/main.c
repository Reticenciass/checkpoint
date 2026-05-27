/*
 * main.c  --  Entry point for Checkpoint
 *
 * Responsibilities:
 *   - Single-instance enforcement (named mutex)
 *   - Initialize Common Controls
 *   - Load config and set global language
 *   - Create main window, register hotkeys, run message loop
 *   - Save config and clean up on exit
 */
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <commctrl.h>
#include <wchar.h>

#include "strings.h"
#include "config.h"
#include "hotkeys.h"
#include "actions.h"
#include "gui.h"

/* ---- Globals (declared extern in gui.h) ---- */
HINSTANCE g_hInstance = NULL;
Config    g_config;
BOOL      g_paused    = FALSE;
AppLang   g_lang      = LANG_EN;

/* ---- Entry point ---- */
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    LPWSTR lpCmdLine, int nCmdShow)
{
    (void)hPrevInstance;
    (void)lpCmdLine;

    /* Single-instance guard */
    HANDLE hMutex = CreateMutex(NULL, TRUE, L"CheckpointSingleInstanceMutex_v1");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        /* Bring existing instance to foreground */
        HWND hwndExisting = FindWindow(STR_APP_CLASS, NULL);
        if (hwndExisting) {
            ShowWindow(hwndExisting, SW_RESTORE);
            SetForegroundWindow(hwndExisting);
        }
        if (hMutex) CloseHandle(hMutex);
        return 0;
    }

    g_hInstance = hInstance;

    /* Enable visual styles / common controls (v6) */
    INITCOMMONCONTROLSEX icc = {
        sizeof(icc),
        ICC_WIN95_CLASSES | ICC_LISTVIEW_CLASSES |
        ICC_BAR_CLASSES   | ICC_STANDARD_CLASSES
    };
    InitCommonControlsEx(&icc);

    /* Load configuration */
    config_init(&g_config);
    config_load(&g_config);
    g_lang   = (AppLang)g_config.language;
    g_paused = g_config.paused;

    /* Create main window */
    HWND hwndMain = gui_create_main_window(hInstance);
    if (!hwndMain) {
        MessageBox(NULL, L"Failed to create main window.",
                   STR_APP_NAME, MB_ICONERROR);
        if (hMutex) { ReleaseMutex(hMutex); CloseHandle(hMutex); }
        return 1;
    }

    /* Register global hotkeys */
    if (!g_paused)
        hotkeys_register_all(hwndMain, &g_config);

    /* Show window */
    ShowWindow(hwndMain, nCmdShow);
    UpdateWindow(hwndMain);

    /* Message loop */
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        /* IsDialogMessage lets Tab/Enter work in child dialogs */
        if (!IsDialogMessage(hwndMain, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    /* Cleanup */
    hotkeys_unregister_all(hwndMain, &g_config);
    config_save(&g_config);

    if (hMutex) { ReleaseMutex(hMutex); CloseHandle(hMutex); }
    return (int)msg.wParam;
}
