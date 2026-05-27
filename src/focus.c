#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wchar.h>
#include <stdio.h>

/* Helper structure for window enumeration */
typedef struct {
    const WCHAR *targetTitle;
    HWND foundHwnd;
} FocusCtx;

static BOOL CALLBACK EnumWindowsProcForFocus(HWND hwnd, LPARAM lParam) {
    FocusCtx *ctx = (FocusCtx *)lParam;
    if (!IsWindowVisible(hwnd)) return TRUE;

    WCHAR title[256];
    if (GetWindowText(hwnd, title, 256) > 0) {
        /* Case-insensitive substring search */
        /* To lower */
        WCHAR lowerTitle[256];
        WCHAR lowerTarget[256];
        wcscpy_s(lowerTitle, 256, title);
        wcscpy_s(lowerTarget, 256, ctx->targetTitle);
        _wcslwr_s(lowerTitle, 256);
        _wcslwr_s(lowerTarget, 256);

        if (wcsstr(lowerTitle, lowerTarget) != NULL) {
            ctx->foundHwnd = hwnd;
            return FALSE; /* Stop enumeration */
        }
    }
    return TRUE;
}

int wmain(int argc, WCHAR **argv) {
    if (argc < 2) {
        wprintf(L"Usage: focus.exe <window_title_substring>\n");
        return 1;
    }

    FocusCtx ctx = { argv[1], NULL };
    EnumWindows(EnumWindowsProcForFocus, (LPARAM)&ctx);

    if (ctx.foundHwnd) {
        HWND hwnd = ctx.foundHwnd;
        /* Anti focus-stealing bypass using AttachThreadInput */
        DWORD fgThread = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
        DWORD myThread = GetCurrentThreadId();

        if (fgThread != myThread) {
            AttachThreadInput(myThread, fgThread, TRUE);
            SetForegroundWindow(hwnd);
            AttachThreadInput(myThread, fgThread, FALSE);
        } else {
            SetForegroundWindow(hwnd);
        }
        
        if (IsIconic(hwnd)) {
            ShowWindow(hwnd, SW_RESTORE);
        }
        wprintf(L"Focused.\n");
        return 0;
    }
    wprintf(L"Not found.\n");
    return 1;
}
