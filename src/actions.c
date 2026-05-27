/*
 * actions.c
 * Execute the action bound to a hotkey:
 *   - Open URL  : ShellExecute with "open" verb
 *   - Open App  : ShellExecute with optional args
 *   - Focus Win : Enumerate top-level windows, find by title fragment
 *   - Run Cmd   : ShellExecute with "runas" or cmd.exe /c
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <wchar.h>
#include "actions.h"
#include "config.h"

/* ------------------------------------------------------------------
 * Focus-window callback context
 * ------------------------------------------------------------------ */
typedef struct {
    const WCHAR *fragment;
    HWND         found;
} FocusCtx;

static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    FocusCtx *ctx = (FocusCtx *)lParam;
    if (!IsWindowVisible(hwnd)) return TRUE;

    WCHAR title[512] = {0};
    GetWindowText(hwnd, title, 512);

    /* Case-insensitive substring search */
    WCHAR titleLow[512], fragLow[256];
    for (int i = 0; i < 512 && title[i]; i++)   titleLow[i] = (WCHAR)towlower(title[i]);
    for (int i = 0; i < 256 && ctx->fragment[i]; i++) fragLow[i] = (WCHAR)towlower(ctx->fragment[i]);
    titleLow[511] = fragLow[255] = L'\0';

    if (wcsstr(titleLow, fragLow)) {
        ctx->found = hwnd;
        return FALSE; /* stop enum */
    }
    return TRUE;
}

BOOL action_focus_window(const WCHAR *fragment) {
    if (!fragment || fragment[0] == L'\0') return FALSE;
    FocusCtx ctx = { fragment, NULL };
    EnumWindows(EnumWindowsProc, (LPARAM)&ctx);
    if (!ctx.found) return FALSE;

    /* Restore if minimized */
    if (IsIconic(ctx.found)) ShowWindow(ctx.found, SW_RESTORE);

    /* Force foreground (workaround for Windows focus stealing prevention) */
    DWORD fgThread = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
    DWORD myThread = GetCurrentThreadId();
    AttachThreadInput(myThread, fgThread, TRUE);
    SetForegroundWindow(ctx.found);
    SetFocus(ctx.found);
    AttachThreadInput(myThread, fgThread, FALSE);
    return TRUE;
}

/* ------------------------------------------------------------------
 * Main dispatch
 * ------------------------------------------------------------------ */
void action_execute(const Bind *b) {
    switch (b->action_type) {

    case ACTION_OPEN_URL:
        /* Open URL in default browser */
        ShellExecute(NULL, L"open", b->target, NULL, NULL, SW_SHOWNORMAL);
        break;

    case ACTION_OPEN_APP: {
        /* Open executable, with optional args */
        const WCHAR *args = (b->args[0] != L'\0') ? b->args : NULL;
        ShellExecute(NULL, L"open", b->target, args, NULL, SW_SHOWNORMAL);
        break;
    }

    case ACTION_FOCUS_WIN:
        /* Try to focus first; if not found, try to open */
        if (!action_focus_window(b->target)) {
            if (b->target[0] != L'\0')
                ShellExecute(NULL, L"open", b->target, b->args[0] ? b->args : NULL, NULL, SW_SHOWNORMAL);
        }
        break;

    case ACTION_RUN_CMD: {
        /* Run via cmd.exe /c to support shell commands */
        WCHAR cmd[MAX_PATH + 512];
        _snwprintf_s(cmd, MAX_PATH + 512, _TRUNCATE, L"/c %s %s", b->target, b->args);
        ShellExecute(NULL, L"open", L"cmd.exe", cmd, NULL, SW_HIDE);
        break;
    }

    default:
        break;
    }
}
