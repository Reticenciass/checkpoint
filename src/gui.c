/*
 * gui.c  --  All Win32 GUI: main window, ListView, Edit-bind dialog,
 *            Settings dialog, About dialog, key-capture subclass.
 *            Features full Dark Mode & GDI Custom Draw for a premium look.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlobj.h>
#include <dwmapi.h>
#include <wchar.h>
#include <stdio.h>

#include "gui.h"
#include "config.h"
#include "hotkeys.h"
#include "actions.h"
#include "strings.h"
#include "../resources/resource.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "dwmapi.lib")

/* =========================================================================
 *  Colors & Styling (Premium Dark Mode)
 * ========================================================================= */
#define COL_BG          RGB(13, 17, 23)    /* #0D1117 */
#define COL_TEXT        RGB(240, 246, 252) /* #F0F6FC */
#define COL_TEXT_MUTED  RGB(139, 148, 158) /* #8B949E */
#define COL_BTN         RGB(33, 38, 45)    /* #21262D */
#define COL_BTN_HOVER   RGB(48, 54, 61)    /* #30363D */
#define COL_ACCENT      RGB(0, 255, 136)   /* #00FF88 (Neon Green) */
#define COL_LIST_BG     RGB(9, 11, 15)     /* Darker list bg */
#define COL_LIST_SEL    RGB(22, 27, 34)    /* #161B22 */

static HBRUSH s_hbrBg      = NULL;
static HBRUSH s_hbrBtn     = NULL;
static HBRUSH s_hbrBtnHov  = NULL;
static HBRUSH s_hbrListBg  = NULL;

/* Hover tracking for buttons */
static HWND s_hwndHoveredBtn = NULL;

/* =========================================================================
 *  Constants / helpers
 * ========================================================================= */
#define WM_TRAYICON     (WM_APP + 1)
#define TRAY_UID        1
#define BTN_HEIGHT      28
#define BTN_MARGIN      6
#define TOOLBAR_H       (BTN_HEIGHT + BTN_MARGIN * 2)
#define STATUSBAR_H     22
#define COL_NAME_W      160
#define COL_SHORTCUT_W  120
#define COL_ACTION_W    100
#define COL_TARGET_W    220
#define COL_STATUS_W    70

static HWND s_hwndList   = NULL;
static HWND s_hwndStatus = NULL;
static HWND s_hwndBtnAdd = NULL, s_hwndBtnEdit = NULL;
static HWND s_hwndBtnRem = NULL, s_hwndBtnTog  = NULL;
static HWND s_hwndBtnSet = NULL, s_hwndBtnLang = NULL;

static NOTIFYICONDATA s_nid;
static BOOL           s_trayAdded = FALSE;
static HFONT          s_hFont     = NULL;
static HFONT          s_hFontBold = NULL;

/* Forward declarations */
static LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);
static void    MainWnd_OnCreate(HWND hwnd);
static void    MainWnd_OnSize(HWND hwnd, int cx, int cy);
static void    MainWnd_OnCommand(HWND hwnd, WPARAM wParam);
static void    MainWnd_OnHotKey(HWND hwnd, int id);
static void    MainWnd_OnTray(HWND hwnd, LPARAM lParam);
static LRESULT MainWnd_OnNotify(HWND hwnd, LPARAM lParam);

static void    AddTrayIcon(HWND hwnd);
static void    RemoveTrayIcon(void);
static void    ShowTrayMenu(HWND hwnd);
static void    UpdateTrayTip(void);

static void    PauseToggle(HWND hwnd);
static void    ToggleSelectedBind(HWND hwnd);
static void    RemoveSelectedBind(HWND hwnd);
static void    EditSelectedBind(HWND hwnd);
static void    AddNewBind(HWND hwnd);

/* =========================================================================
 *  Theme Helpers
 * ========================================================================= */
static void ApplyDarkMode(HWND hwnd) {
    BOOL dark = TRUE;
    /* DWMWA_USE_IMMERSIVE_DARK_MODE is 20 */
    DwmSetWindowAttribute(hwnd, 20, &dark, sizeof(dark));
}

static HFONT CreateAppFont(int ptSize, BOOL bold) {
    HDC hdc = GetDC(NULL);
    int h = -MulDiv(ptSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    ReleaseDC(NULL, hdc);
    return CreateFont(h, 0, 0, 0,
        bold ? FW_SEMIBOLD : FW_NORMAL,
        FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI");
}

/* Common OwnerDraw Button proc for hover & drawing */
static LRESULT CALLBACK BtnSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    (void)uIdSubclass; (void)dwRefData;
    switch (msg) {
    case WM_MOUSEMOVE:
        if (s_hwndHoveredBtn != hwnd) {
            s_hwndHoveredBtn = hwnd;
            InvalidateRect(hwnd, NULL, TRUE);
            TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hwnd, 0 };
            TrackMouseEvent(&tme);
        }
        break;
    case WM_MOUSELEAVE:
        if (s_hwndHoveredBtn == hwnd) {
            s_hwndHoveredBtn = NULL;
            InvalidateRect(hwnd, NULL, TRUE);
        }
        break;
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

static void DrawFlatButton(LPDRAWITEMSTRUCT dis) {
    HDC hdc = dis->hDC;
    RECT rc = dis->rcItem;
    BOOL isHovered = (s_hwndHoveredBtn == dis->hwndItem);
    BOOL isPressed = (dis->itemState & ODS_SELECTED);
    BOOL isDisabled = (dis->itemState & ODS_DISABLED);

    HBRUSH bgBrush = isPressed ? s_hbrBtn : (isHovered ? s_hbrBtnHov : s_hbrBtn);
    if (isDisabled) bgBrush = s_hbrBg;

    FillRect(hdc, &rc, bgBrush);

    /* Text */
    WCHAR txt[128];
    GetWindowText(dis->hwndItem, txt, 128);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, isDisabled ? COL_TEXT_MUTED : COL_TEXT);
    
    /* Slight shift when pressed */
    if (isPressed) { rc.top++; rc.left++; }

    SelectObject(hdc, s_hFont);
    DrawText(hdc, txt, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

/* Make a button owner-drawn and track hover */
static HWND CreateCustomButton(int id, const WCHAR *txt, int x, int y, int w, int h, HWND parent) {
    HWND btn = CreateWindow(L"BUTTON", txt,
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        x, y, w, h, parent, (HMENU)(INT_PTR)id, g_hInstance, NULL);
    SetWindowSubclass(btn, BtnSubclassProc, 1, 0);
    return btn;
}

/* =========================================================================
 *  Public: create main window
 * ========================================================================= */
HWND gui_create_main_window(HINSTANCE hInstance) {
    s_hbrBg     = CreateSolidBrush(COL_BG);
    s_hbrBtn    = CreateSolidBrush(COL_BTN);
    s_hbrBtnHov = CreateSolidBrush(COL_BTN_HOVER);
    s_hbrListBg = CreateSolidBrush(COL_LIST_BG);
    s_hFont     = CreateAppFont(10, FALSE);
    s_hFontBold = CreateAppFont(10, TRUE);

    WNDCLASSEX wc = {0};
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = MainWndProc;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON_MAIN));
    if (!wc.hIcon)   wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hIconSm       = wc.hIcon;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = s_hbrBg; /* Custom dark bg */
    wc.lpszMenuName  = MAKEINTRESOURCE(IDM_MAIN);
    wc.lpszClassName = STR_APP_CLASS;
    RegisterClassEx(&wc);

    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);
    int ww = 780, wh = 500;

    HWND hwnd = CreateWindowEx(
        WS_EX_APPWINDOW,
        STR_APP_CLASS, STR_APP_TITLE,
        WS_OVERLAPPEDWINDOW,
        (sw - ww) / 2, (sh - wh) / 2, ww, wh,
        NULL, NULL, hInstance, NULL);
    
    ApplyDarkMode(hwnd);
    return hwnd;
}

/* =========================================================================
 *  Public: refresh ListView
 * ========================================================================= */
void gui_refresh_list(HWND hwndList, Config *cfg) {
    ListView_DeleteAllItems(hwndList);
    for (int i = 0; i < cfg->count; i++) {
        const Bind *b = &cfg->binds[i];
        LVITEM lvi = {0};
        lvi.mask    = LVIF_TEXT | LVIF_PARAM;
        lvi.iItem   = i;
        lvi.iSubItem = 0;
        lvi.lParam  = (LPARAM)i;
        lvi.pszText = (LPWSTR)b->name;
        int row = ListView_InsertItem(hwndList, &lvi);

        WCHAR keyStr[64] = {0};
        bind_key_string(b, keyStr, 64);
        ListView_SetItemText(hwndList, row, 1, keyStr);

        WCHAR actStr[32] = {0};
        bind_action_string(b, actStr, 32);
        ListView_SetItemText(hwndList, row, 2, actStr);

        WCHAR tgt[64] = {0};
        _snwprintf_s(tgt, 64, _TRUNCATE, L"%s", b->target);
        ListView_SetItemText(hwndList, row, 3, tgt);

        WCHAR *status = b->enabled ? (WCHAR *)STR_STATUS_ON : (WCHAR *)STR_STATUS_OFF;
        ListView_SetItemText(hwndList, row, 4, status);
    }
}

/* =========================================================================
 *  Public: update status bar
 * ========================================================================= */
void gui_update_status(HWND hwndStatus, Config *cfg, BOOL paused) {
    WCHAR buf[256];
    if (paused) {
        _snwprintf_s(buf, 256, _TRUNCATE, L"  %s", STR_SB_PAUSED);
    } else {
        int active = 0;
        for (int i = 0; i < cfg->count; i++)
            if (cfg->binds[i].enabled) active++;
        _snwprintf_s(buf, 256, _TRUNCATE, L"  ");
        WCHAR fmt[200];
        _snwprintf_s(fmt, 200, _TRUNCATE, STR_SB_ACTIVE_FMT, active);
        wcsncat_s(buf, 256, fmt, _TRUNCATE);
    }
    SetWindowText(hwndStatus, buf);
}

/* =========================================================================
 *  Public: re-apply language to main window controls
 * ========================================================================= */
void gui_apply_language(HWND hwnd) {
    SetWindowText(hwnd, STR_APP_TITLE);
    SetWindowText(s_hwndBtnAdd,  STR_BTN_ADD);
    SetWindowText(s_hwndBtnEdit, STR_BTN_EDIT);
    SetWindowText(s_hwndBtnRem,  STR_BTN_REMOVE);
    SetWindowText(s_hwndBtnTog,  STR_BTN_TOGGLE);
    SetWindowText(s_hwndBtnSet,  STR_BTN_SETTINGS);
    SetWindowText(s_hwndBtnLang, g_lang == LANG_PT ? L"EN" : L"PT");

    /* Rebuild column headers */
    HWND hwndList = s_hwndList;
    LVCOLUMN lvc = {0};
    lvc.mask = LVCF_TEXT;
    lvc.pszText = (LPWSTR)STR_COL_NAME;     ListView_SetColumn(hwndList, 0, &lvc);
    lvc.pszText = (LPWSTR)STR_COL_SHORTCUT; ListView_SetColumn(hwndList, 1, &lvc);
    lvc.pszText = (LPWSTR)STR_COL_ACTION;   ListView_SetColumn(hwndList, 2, &lvc);
    lvc.pszText = (LPWSTR)STR_COL_TARGET;   ListView_SetColumn(hwndList, 3, &lvc);
    lvc.pszText = (LPWSTR)STR_COL_STATUS;   ListView_SetColumn(hwndList, 4, &lvc);

    gui_refresh_list(hwndList, &g_config);
    gui_update_status(s_hwndStatus, &g_config, g_paused);
}

/* =========================================================================
 *  Tray helpers
 * ========================================================================= */
static void AddTrayIcon(HWND hwnd) {
    ZeroMemory(&s_nid, sizeof(s_nid));
    s_nid.cbSize           = sizeof(NOTIFYICONDATA);
    s_nid.hWnd             = hwnd;
    s_nid.uID              = TRAY_UID;
    s_nid.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    s_nid.uCallbackMessage = WM_TRAYICON;
    s_nid.hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_ICON_MAIN));
    if (!s_nid.hIcon) s_nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcscpy_s(s_nid.szTip, 128, STR_TRAY_TIP);
    Shell_NotifyIcon(NIM_ADD, &s_nid);
    s_trayAdded = TRUE;
}

static void RemoveTrayIcon(void) {
    if (s_trayAdded) { Shell_NotifyIcon(NIM_DELETE, &s_nid); s_trayAdded = FALSE; }
}

static void UpdateTrayTip(void) {
    if (!s_trayAdded) return;
    wcscpy_s(s_nid.szTip, 128, g_paused ? STR_TRAY_TIP_PAUSED : STR_TRAY_TIP);
    s_nid.uFlags = NIF_TIP;
    Shell_NotifyIcon(NIM_MODIFY, &s_nid);
}

static void ShowTrayMenu(HWND hwnd) {
    POINT pt; GetCursorPos(&pt);
    HMENU hMenu = CreatePopupMenu();
    AppendMenu(hMenu, MF_STRING, ID_TRAY_OPEN,  STR_TRAY_OPEN);
    AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hMenu, MF_STRING, ID_TRAY_PAUSE, g_paused ? STR_TRAY_RESUME : STR_TRAY_PAUSE);
    AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hMenu, MF_STRING, ID_TRAY_EXIT,  STR_TRAY_EXIT);

    MENUITEMINFO mii = { sizeof(mii), MIIM_STATE };
    mii.fState = MFS_DEFAULT;
    SetMenuItemInfo(hMenu, ID_TRAY_OPEN, FALSE, &mii);

    SetForegroundWindow(hwnd);
    TrackPopupMenu(hMenu, TPM_RIGHTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON,
                   pt.x, pt.y, 0, hwnd, NULL);
    PostMessage(hwnd, WM_NULL, 0, 0);
    DestroyMenu(hMenu);
}

/* =========================================================================
 *  Main Window Creation
 * ========================================================================= */
static void MainWnd_OnCreate(HWND hwnd) {
    int x = BTN_MARGIN;
    s_hwndBtnAdd  = CreateCustomButton(IDC_BTN_ADD,         STR_BTN_ADD,      x, BTN_MARGIN, 110, BTN_HEIGHT, hwnd); x += 110 + BTN_MARGIN;
    s_hwndBtnEdit = CreateCustomButton(IDC_BTN_EDIT,        STR_BTN_EDIT,     x, BTN_MARGIN,  80, BTN_HEIGHT, hwnd); x +=  80 + BTN_MARGIN;
    s_hwndBtnRem  = CreateCustomButton(IDC_BTN_REMOVE,      STR_BTN_REMOVE,   x, BTN_MARGIN,  90, BTN_HEIGHT, hwnd); x +=  90 + BTN_MARGIN;
    s_hwndBtnTog  = CreateCustomButton(IDC_BTN_TOGGLE,      STR_BTN_TOGGLE,   x, BTN_MARGIN, 135, BTN_HEIGHT, hwnd); x += 135 + BTN_MARGIN;
    s_hwndBtnSet  = CreateCustomButton(IDC_BTN_SETTINGS_MW, STR_BTN_SETTINGS, x, BTN_MARGIN, 120, BTN_HEIGHT, hwnd);

    RECT rc; GetClientRect(hwnd, &rc);
    s_hwndBtnLang = CreateCustomButton(IDC_BTN_LANG, g_lang == LANG_PT ? L"EN" : L"PT",
        rc.right - 50, BTN_MARGIN, 44, BTN_HEIGHT, hwnd);

    /* ---- ListView ---- */
    /* Remove CLIENTEDGE for a flatter, modern look */
    s_hwndList = CreateWindowEx(0, WC_LISTVIEW, L"",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_OWNERDRAWFIXED,
        0, TOOLBAR_H, rc.right, rc.bottom - TOOLBAR_H - STATUSBAR_H,
        hwnd, (HMENU)(INT_PTR)IDC_LIST_BINDS, g_hInstance, NULL);
    
    /* We don't use GRIDLINES for a cleaner look */
    ListView_SetExtendedListViewStyle(s_hwndList, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    SendMessage(s_hwndList, WM_SETFONT, (WPARAM)s_hFont, TRUE);
    ListView_SetBkColor(s_hwndList, COL_LIST_BG);
    ListView_SetTextBkColor(s_hwndList, COL_LIST_BG);
    ListView_SetTextColor(s_hwndList, COL_TEXT);

    struct { const WCHAR *title; int w; } cols[5];
    cols[0].title = STR_COL_NAME;     cols[0].w = COL_NAME_W;
    cols[1].title = STR_COL_SHORTCUT; cols[1].w = COL_SHORTCUT_W;
    cols[2].title = STR_COL_ACTION;   cols[2].w = COL_ACTION_W;
    cols[3].title = STR_COL_TARGET;   cols[3].w = COL_TARGET_W;
    cols[4].title = STR_COL_STATUS;   cols[4].w = COL_STATUS_W;
    for (int i = 0; i < 5; i++) {
        LVCOLUMN lvc = {0};
        lvc.mask    = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        lvc.pszText = (LPWSTR)cols[i].title;
        lvc.cx      = cols[i].w;
        lvc.iSubItem = i;
        ListView_InsertColumn(s_hwndList, i, &lvc);
    }

    /* ---- Status bar ---- */
    s_hwndStatus = CreateWindow(STATUSCLASSNAME, L"",
        WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
        0, 0, 0, 0, hwnd, (HMENU)(INT_PTR)IDC_STATUSBAR, g_hInstance, NULL);
    SendMessage(s_hwndStatus, WM_SETFONT, (WPARAM)s_hFont, TRUE);

    gui_refresh_list(s_hwndList, &g_config);
    gui_update_status(s_hwndStatus, &g_config, g_paused);
    AddTrayIcon(hwnd);
}

static void MainWnd_OnSize(HWND hwnd, int cx, int cy) {
    SetWindowPos(s_hwndBtnLang, NULL, cx - 50, BTN_MARGIN, 44, BTN_HEIGHT, SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(s_hwndList, NULL, 0, TOOLBAR_H, cx, cy - TOOLBAR_H - STATUSBAR_H, SWP_NOZORDER | SWP_NOACTIVATE);
    SendMessage(s_hwndStatus, WM_SIZE, 0, 0);
    (void)hwnd;
}

/* =========================================================================
 *  Bind actions
 * ========================================================================= */
static int GetSelectedIndex(void) {
    return ListView_GetNextItem(s_hwndList, -1, LVNI_SELECTED);
}

static void AddNewBind(HWND hwnd) {
    if (g_config.count >= MAX_BINDS) return;
    if (gui_show_edit_dialog(hwnd, &g_config, -1)) {
        int idx = g_config.count - 1;
        if (g_config.binds[idx].enabled && !g_paused) hotkey_register(hwnd, &g_config.binds[idx]);
        config_save(&g_config);
        gui_refresh_list(s_hwndList, &g_config);
        gui_update_status(s_hwndStatus, &g_config, g_paused);
    }
}
static void EditSelectedBind(HWND hwnd) {
    int idx = GetSelectedIndex();
    if (idx < 0) return;
    hotkey_unregister(hwnd, &g_config.binds[idx]);
    if (gui_show_edit_dialog(hwnd, &g_config, idx)) {
        if (g_config.binds[idx].enabled && !g_paused) hotkey_register(hwnd, &g_config.binds[idx]);
        config_save(&g_config);
        gui_refresh_list(s_hwndList, &g_config);
        gui_update_status(s_hwndStatus, &g_config, g_paused);
    } else {
        if (g_config.binds[idx].enabled && !g_paused) hotkey_register(hwnd, &g_config.binds[idx]);
    }
}
static void RemoveSelectedBind(HWND hwnd) {
    int idx = GetSelectedIndex();
    if (idx < 0) return;
    if (MessageBox(hwnd, STR_MSG_CONFIRM_REMOVE, STR_APP_NAME, MB_YESNO | MB_ICONQUESTION) != IDYES) return;
    hotkey_unregister(hwnd, &g_config.binds[idx]);
    config_remove_bind(&g_config, idx);
    hotkeys_unregister_all(hwnd, &g_config);
    if (!g_paused) hotkeys_register_all(hwnd, &g_config);
    config_save(&g_config);
    gui_refresh_list(s_hwndList, &g_config);
    gui_update_status(s_hwndStatus, &g_config, g_paused);
}
static void ToggleSelectedBind(HWND hwnd) {
    int idx = GetSelectedIndex();
    if (idx < 0) return;
    Bind *b = &g_config.binds[idx];
    b->enabled = !b->enabled;
    if (b->enabled && !g_paused) hotkey_register(hwnd, b); else hotkey_unregister(hwnd, b);
    config_save(&g_config);
    gui_refresh_list(s_hwndList, &g_config);
    gui_update_status(s_hwndStatus, &g_config, g_paused);
}
static void PauseToggle(HWND hwnd) {
    g_paused = !g_paused;
    g_config.paused = g_paused;
    if (g_paused) hotkeys_unregister_all(hwnd, &g_config); else hotkeys_register_all(hwnd, &g_config);
    UpdateTrayTip();
    gui_update_status(s_hwndStatus, &g_config, g_paused);
}
static void ToggleLanguage(HWND hwnd) {
    g_lang = (g_lang == LANG_EN) ? LANG_PT : LANG_EN;
    g_config.language = (int)g_lang;
    config_save(&g_config);
    gui_apply_language(hwnd);
}

/* =========================================================================
 *  WM_COMMAND
 * ========================================================================= */
static void MainWnd_OnCommand(HWND hwnd, WPARAM wParam) {
    switch (LOWORD(wParam)) {
    case IDC_BTN_ADD:         AddNewBind(hwnd);                 break;
    case IDC_BTN_EDIT:        EditSelectedBind(hwnd);           break;
    case IDC_BTN_REMOVE:      RemoveSelectedBind(hwnd);         break;
    case IDC_BTN_TOGGLE:      ToggleSelectedBind(hwnd);         break;
    case IDC_BTN_SETTINGS_MW:
        if (gui_show_settings_dialog(hwnd, &g_config)) {
            g_lang = (AppLang)g_config.language;
            config_save(&g_config);
            gui_apply_language(hwnd);
        }
        break;
    case IDC_BTN_LANG:        ToggleLanguage(hwnd);             break;
    case ID_TRAY_OPEN:        ShowWindow(hwnd, SW_RESTORE); SetForegroundWindow(hwnd); break;
    case ID_TRAY_PAUSE:       PauseToggle(hwnd);                break;
    case ID_TRAY_EXIT:
    case ID_FILE_EXIT:        DestroyWindow(hwnd);              break;
    case ID_HELP_ABOUT:       gui_show_about_dialog(hwnd);      break;
    case ID_VIEW_SETTINGS:
        if (gui_show_settings_dialog(hwnd, &g_config)) {
            g_lang = (AppLang)g_config.language;
            config_save(&g_config);
            gui_apply_language(hwnd);
        }
        break;
    }
}

/* =========================================================================
 *  WM_HOTKEY
 * ========================================================================= */
static void MainWnd_OnHotKey(HWND hwnd, int id) {
    (void)hwnd;
    int idx = HOTKEY_ID_TO_BIND(id);
    if (idx < 0 || idx >= g_config.count) return;
    if (!g_config.binds[idx].enabled || g_paused) return;
    action_execute(&g_config.binds[idx]);
}

static void MainWnd_OnTray(HWND hwnd, LPARAM lParam) {
    switch (LOWORD(lParam)) {
    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONUP: ShowWindow(hwnd, SW_RESTORE); SetForegroundWindow(hwnd); break;
    case WM_RBUTTONUP: ShowTrayMenu(hwnd); break;
    }
}

/* =========================================================================
 *  WM_NOTIFY (Custom Draw ListView)
 * ========================================================================= */
static LRESULT MainWnd_OnNotify(HWND hwnd, LPARAM lParam) {
    NMHDR *nmhdr = (NMHDR *)lParam;
    if (nmhdr->idFrom == IDC_LIST_BINDS) {
        if (nmhdr->code == NM_DBLCLK) {
            EditSelectedBind(hwnd);
            return 0;
        }
        if (nmhdr->code == NM_CUSTOMDRAW) {
            NMLVCUSTOMDRAW *lvcd = (NMLVCUSTOMDRAW *)lParam;
            switch (lvcd->nmcd.dwDrawStage) {
            case CDDS_PREPAINT:
                return CDRF_NOTIFYITEMDRAW;
            case CDDS_ITEMPREPAINT:
                return CDRF_NOTIFYSUBITEMDRAW;
            case CDDS_ITEMPREPAINT | CDDS_SUBITEM: {
                int row = (int)lvcd->nmcd.dwItemSpec;
                BOOL isSelected = (ListView_GetItemState(s_hwndList, row, LVIS_SELECTED) == LVIS_SELECTED);
                
                lvcd->clrTextBk = isSelected ? COL_LIST_SEL : COL_LIST_BG;
                
                /* Subitem 4 = Status */
                if (lvcd->iSubItem == 4 && !isSelected) {
                    Bind *b = &g_config.binds[row];
                    lvcd->clrText = b->enabled ? COL_ACCENT : COL_TEXT_MUTED;
                } else {
                    lvcd->clrText = COL_TEXT;
                }

                SelectObject(lvcd->nmcd.hdc, s_hFont);
                return CDRF_NEWFONT;
            }
            }
        }
    }
    return 0;
}

/* =========================================================================
 *  Main Window Procedure
 * ========================================================================= */
static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        MainWnd_OnCreate(hwnd);
        return 0;
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED) MainWnd_OnSize(hwnd, LOWORD(lParam), HIWORD(lParam));
        return 0;
    case WM_COMMAND:
        MainWnd_OnCommand(hwnd, wParam);
        return 0;
    case WM_HOTKEY:
        MainWnd_OnHotKey(hwnd, (int)wParam);
        return 0;
    case WM_TRAYICON:
        MainWnd_OnTray(hwnd, lParam);
        return 0;
    case WM_NOTIFY:
        return MainWnd_OnNotify(hwnd, lParam);
    case WM_DRAWITEM:
        DrawFlatButton((LPDRAWITEMSTRUCT)lParam);
        return TRUE;
    case WM_CLOSE:
        ShowWindow(hwnd, SW_HIDE);
        return 0;
    case WM_DESTROY:
        RemoveTrayIcon();
        if (s_hbrBg)     DeleteObject(s_hbrBg);
        if (s_hbrBtn)    DeleteObject(s_hbrBtn);
        if (s_hbrBtnHov) DeleteObject(s_hbrBtnHov);
        if (s_hbrListBg) DeleteObject(s_hbrListBg);
        if (s_hFont)     DeleteObject(s_hFont);
        if (s_hFontBold) DeleteObject(s_hFontBold);
        PostQuitMessage(0);
        return 0;
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
        SetBkMode((HDC)wParam, TRANSPARENT);
        SetTextColor((HDC)wParam, COL_TEXT);
        return (LRESULT)s_hbrBg;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

/* =========================================================================
 *  KEY CAPTURE subclass for the Edit-Bind dialog
 * ========================================================================= */
typedef struct {
    BOOL  *pCapturing;
    UINT  *pModifiers;
    UINT  *pVkey;
    HWND   hwndShortcutEdit;
    HWND   hwndCapBtn;
} CaptureCtx;

static LRESULT CALLBACK CaptureSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    CaptureCtx *ctx = (CaptureCtx *)dwRefData;
    if (*ctx->pCapturing && (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN)) {
        UINT vk = (UINT)wParam;
        if (vk == VK_SHIFT || vk == VK_CONTROL || vk == VK_MENU || vk == VK_LWIN || vk == VK_RWIN) return 0;
        if (vk == VK_ESCAPE) {
            *ctx->pCapturing = FALSE;
            SetWindowText(ctx->hwndCapBtn, STR_BTN_CAPTURE);
            return 0;
        }
        UINT mods = 0;
        if (GetKeyState(VK_SHIFT)   & 0x8000) mods |= MOD_SHIFT;
        if (GetKeyState(VK_CONTROL) & 0x8000) mods |= MOD_CONTROL;
        if (GetKeyState(VK_MENU)    & 0x8000) mods |= MOD_ALT;
        if ((GetKeyState(VK_LWIN) | GetKeyState(VK_RWIN)) & 0x8000) mods |= MOD_WIN;
        *ctx->pModifiers = mods;
        *ctx->pVkey      = vk;
        *ctx->pCapturing = FALSE;
        SetWindowText(ctx->hwndCapBtn, STR_BTN_CAPTURE);
        Bind tmp = {0}; tmp.modifiers = mods; tmp.vkey = vk;
        WCHAR keyStr[64]; bind_key_string(&tmp, keyStr, 64);
        SetWindowText(ctx->hwndShortcutEdit, keyStr);
        return 0;
    }
    if (*ctx->pCapturing && msg == WM_KILLFOCUS) {
        *ctx->pCapturing = FALSE;
        SetWindowText(ctx->hwndCapBtn, STR_BTN_CAPTURE);
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

/* =========================================================================
 *  Edit-Bind Dialog
 * ========================================================================= */
typedef struct {
    Config *cfg;
    int     bindIdx;
    Bind    result;
    BOOL    ok;
    BOOL    capturing;
    UINT    capturedMods;
    UINT    capturedVkey;
    CaptureCtx captureCtx;
} EditDlgData;

static void EditDlg_InitControls(HWND hdlg, EditDlgData *d) {
    SetWindowText(hdlg, d->bindIdx < 0 ? STR_DLG_ADD_BIND : STR_DLG_EDIT_BIND);

    HWND hName     = GetDlgItem(hdlg, IDC_EDIT_NAME);
    HWND hShortcut = GetDlgItem(hdlg, IDC_EDIT_SHORTCUT);
    HWND hCapBtn   = GetDlgItem(hdlg, IDC_BTN_CAPTURE);
    HWND hCombo    = GetDlgItem(hdlg, IDC_COMBO_ACTION);
    HWND hTarget   = GetDlgItem(hdlg, IDC_EDIT_TARGET);
    HWND hArgs     = GetDlgItem(hdlg, IDC_EDIT_ARGS);
    HWND hEnabled  = GetDlgItem(hdlg, IDC_CHK_ENABLED);
    HWND hHint     = GetDlgItem(hdlg, IDC_LBL_TARGET_HINT);
    HWND hHelp     = GetDlgItem(hdlg, IDC_LBL_CAPTURE_HELP);

    /* Convert buttons to ownerdraw in dialog */
    SetWindowLong(hCapBtn, GWL_STYLE, GetWindowLong(hCapBtn, GWL_STYLE) | BS_OWNERDRAW);
    SetWindowSubclass(hCapBtn, BtnSubclassProc, 1, 0);
    HWND hBtnBrowse = GetDlgItem(hdlg, IDC_BTN_BROWSE);
    SetWindowLong(hBtnBrowse, GWL_STYLE, GetWindowLong(hBtnBrowse, GWL_STYLE) | BS_OWNERDRAW);
    SetWindowSubclass(hBtnBrowse, BtnSubclassProc, 1, 0);
    HWND hBtnOk = GetDlgItem(hdlg, IDOK);
    SetWindowLong(hBtnOk, GWL_STYLE, GetWindowLong(hBtnOk, GWL_STYLE) | BS_OWNERDRAW);
    SetWindowSubclass(hBtnOk, BtnSubclassProc, 1, 0);
    HWND hBtnCancel = GetDlgItem(hdlg, IDCANCEL);
    SetWindowLong(hBtnCancel, GWL_STYLE, GetWindowLong(hBtnCancel, GWL_STYLE) | BS_OWNERDRAW);
    SetWindowSubclass(hBtnCancel, BtnSubclassProc, 1, 0);

    SetDlgItemText(hdlg, IDC_BTN_CAPTURE, STR_BTN_CAPTURE);
    SetDlgItemText(hdlg, IDC_BTN_BROWSE,  STR_BTN_BROWSE);
    SetDlgItemText(hdlg, IDOK,            STR_BTN_OK);
    SetDlgItemText(hdlg, IDCANCEL,        STR_BTN_CANCEL);
    SetDlgItemText(hdlg, IDC_CHK_ENABLED, STR_LBL_ENABLED);

    HWND hChild = GetWindow(hdlg, GW_CHILD);
    while (hChild) {
        SendMessage(hChild, WM_SETFONT, (WPARAM)s_hFont, TRUE);
        hChild = GetWindow(hChild, GW_HWNDNEXT);
    }

    SendMessage(hCombo, CB_RESETCONTENT, 0, 0);
    SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)STR_ACTION_OPEN_URL);
    SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)STR_ACTION_OPEN_APP);
    SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)STR_ACTION_FOCUS_WIN);
    SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)STR_ACTION_RUN_CMD);

    const Bind *b = &d->result;
    SetWindowText(hName, b->name);
    WCHAR keyStr[64]; bind_key_string(b, keyStr, 64);
    SetWindowText(hShortcut, keyStr);
    SendMessage(hCombo, CB_SETCURSEL, (WPARAM)b->action_type, 0);
    SetWindowText(hTarget, b->target);
    SetWindowText(hArgs,   b->args);
    SendMessage(hEnabled, BM_SETCHECK, b->enabled ? BST_CHECKED : BST_UNCHECKED, 0);

    const WCHAR *hints[4];
    hints[0] = STR_HINT_URL; hints[1] = STR_HINT_APP; hints[2] = STR_HINT_WIN; hints[3] = STR_HINT_CMD;
    SetWindowText(hHint, hints[(int)b->action_type]);
    SetWindowText(hHelp, S(L"Click 'Capture Key' then press the desired key combination.  ESC cancels.",
                           L"Clique em 'Capturar Tecla' e pressione a combina\x00E7\x00E3o desejada.  ESC cancela."));

    d->captureCtx.pCapturing = &d->capturing;
    d->captureCtx.pModifiers = &d->capturedMods;
    d->captureCtx.pVkey = &d->capturedVkey;
    d->captureCtx.hwndShortcutEdit = hShortcut;
    d->captureCtx.hwndCapBtn = hCapBtn;
    SetWindowSubclass(hShortcut, CaptureSubclassProc, 1, (DWORD_PTR)&d->captureCtx);

    d->capturedMods = b->modifiers;
    d->capturedVkey = b->vkey;
}

static void EditDlg_UpdateHint(HWND hdlg) {
    int sel = (int)SendDlgItemMessage(hdlg, IDC_COMBO_ACTION, CB_GETCURSEL, 0, 0);
    const WCHAR *hints[4];
    hints[0] = STR_HINT_URL; hints[1] = STR_HINT_APP; hints[2] = STR_HINT_WIN; hints[3] = STR_HINT_CMD;
    if (sel >= 0 && sel < ACTION_COUNT) SetDlgItemText(hdlg, IDC_LBL_TARGET_HINT, hints[sel]);
    EnableWindow(GetDlgItem(hdlg, IDC_BTN_BROWSE), (sel == ACTION_OPEN_APP || sel == ACTION_RUN_CMD));
}

static BOOL EditDlg_Browse(HWND hdlg) {
    OPENFILENAME ofn = {0};
    WCHAR buf[MAX_PATH] = {0};
    GetDlgItemText(hdlg, IDC_EDIT_TARGET, buf, MAX_PATH);
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner   = hdlg;
    ofn.lpstrFile   = buf;
    ofn.nMaxFile    = MAX_PATH;
    ofn.lpstrFilter = L"Executables\0*.exe;*.bat;*.cmd\0All Files\0*.*\0\0";
    ofn.Flags       = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (GetOpenFileName(&ofn)) { SetDlgItemText(hdlg, IDC_EDIT_TARGET, buf); return TRUE; }
    return FALSE;
}

static BOOL EditDlg_Validate(HWND hdlg, EditDlgData *d) {
    if (d->capturedVkey == 0) { MessageBox(hdlg, STR_MSG_INVALID_KEY, STR_APP_NAME, MB_ICONWARNING); return FALSE; }
    WCHAR tgt[MAX_PATH]; GetDlgItemText(hdlg, IDC_EDIT_TARGET, tgt, MAX_PATH);
    if (tgt[0] == L'\0') { MessageBox(hdlg, STR_MSG_INVALID_TARGET, STR_APP_NAME, MB_ICONWARNING); return FALSE; }
    Config *cfg = d->cfg;
    for (int i = 0; i < cfg->count; i++) {
        if (i == d->bindIdx) continue;
        if (cfg->binds[i].modifiers == d->capturedMods && cfg->binds[i].vkey == d->capturedVkey) {
            MessageBox(hdlg, STR_MSG_CONFLICT, STR_APP_NAME, MB_ICONWARNING); return FALSE;
        }
    }
    return TRUE;
}

static void EditDlg_Collect(HWND hdlg, EditDlgData *d) {
    Bind *b = &d->result;
    GetDlgItemText(hdlg, IDC_EDIT_NAME,   b->name,   128);
    GetDlgItemText(hdlg, IDC_EDIT_TARGET, b->target, MAX_PATH);
    GetDlgItemText(hdlg, IDC_EDIT_ARGS,   b->args,   512);
    b->modifiers   = d->capturedMods;
    b->vkey        = d->capturedVkey;
    b->action_type = (ActionType)SendDlgItemMessage(hdlg, IDC_COMBO_ACTION, CB_GETCURSEL, 0, 0);
    b->enabled     = (SendDlgItemMessage(hdlg, IDC_CHK_ENABLED, BM_GETCHECK, 0, 0) == BST_CHECKED);
    if (b->name[0] == L'\0') wcscpy_s(b->name, 128, L"Unnamed");
}

static INT_PTR CALLBACK EditBindDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    EditDlgData *d = (EditDlgData *)GetWindowLongPtr(hdlg, GWLP_USERDATA);
    switch (msg) {
    case WM_INITDIALOG:
        ApplyDarkMode(hdlg);
        SetWindowLongPtr(hdlg, GWLP_USERDATA, lParam);
        d = (EditDlgData *)lParam;
        EditDlg_InitControls(hdlg, d);
        EditDlg_UpdateHint(hdlg);
        return TRUE;
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
        SetBkMode((HDC)wParam, TRANSPARENT);
        SetTextColor((HDC)wParam, COL_TEXT);
        return (INT_PTR)s_hbrBg;
    case WM_DRAWITEM:
        DrawFlatButton((LPDRAWITEMSTRUCT)lParam);
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_BTN_CAPTURE:
            d->capturing = TRUE;
            SetWindowText((HWND)lParam, STR_BTN_CAPTURING);
            SetFocus(GetDlgItem(hdlg, IDC_EDIT_SHORTCUT));
            break;
        case IDC_BTN_BROWSE: EditDlg_Browse(hdlg); break;
        case IDC_COMBO_ACTION: if (HIWORD(wParam) == CBN_SELCHANGE) EditDlg_UpdateHint(hdlg); break;
        case IDOK:
            EditDlg_Collect(hdlg, d);
            if (!EditDlg_Validate(hdlg, d)) break;
            d->ok = TRUE;
            RemoveWindowSubclass(GetDlgItem(hdlg, IDC_EDIT_SHORTCUT), CaptureSubclassProc, 1);
            EndDialog(hdlg, IDOK);
            break;
        case IDCANCEL:
            RemoveWindowSubclass(GetDlgItem(hdlg, IDC_EDIT_SHORTCUT), CaptureSubclassProc, 1);
            EndDialog(hdlg, IDCANCEL);
            break;
        }
        return TRUE;
    }
    return FALSE;
}

BOOL gui_show_edit_dialog(HWND hwndParent, Config *cfg, int bindIdx) {
    EditDlgData d = {0};
    d.cfg = cfg; d.bindIdx = bindIdx; d.ok = FALSE;
    if (bindIdx >= 0 && bindIdx < cfg->count) d.result = cfg->binds[bindIdx]; else d.result.enabled = TRUE;
    if (DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_EDIT_BIND), hwndParent, EditBindDlgProc, (LPARAM)&d) == IDOK && d.ok) {
        if (bindIdx < 0) config_add_bind(cfg, &d.result); else config_update_bind(cfg, bindIdx, &d.result);
        return TRUE;
    }
    return FALSE;
}

/* =========================================================================
 *  Settings Dialog
 * ========================================================================= */
typedef struct { Config *cfg; BOOL ok; } SettingsDlgData;

static INT_PTR CALLBACK SettingsDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    SettingsDlgData *d = (SettingsDlgData *)GetWindowLongPtr(hdlg, GWLP_USERDATA);
    switch (msg) {
    case WM_INITDIALOG: {
        ApplyDarkMode(hdlg);
        SetWindowLongPtr(hdlg, GWLP_USERDATA, lParam);
        d = (SettingsDlgData *)lParam;
        SetWindowText(hdlg, STR_DLG_SETTINGS);
        SetDlgItemText(hdlg, IDOK,     STR_BTN_OK);
        SetDlgItemText(hdlg, IDCANCEL, STR_BTN_CANCEL);
        SetDlgItemText(hdlg, IDC_CHK_STARTUP, STR_LBL_STARTUP);

        HWND hBtnOk = GetDlgItem(hdlg, IDOK);
        SetWindowLong(hBtnOk, GWL_STYLE, GetWindowLong(hBtnOk, GWL_STYLE) | BS_OWNERDRAW);
        SetWindowSubclass(hBtnOk, BtnSubclassProc, 1, 0);
        HWND hBtnCancel = GetDlgItem(hdlg, IDCANCEL);
        SetWindowLong(hBtnCancel, GWL_STYLE, GetWindowLong(hBtnCancel, GWL_STYLE) | BS_OWNERDRAW);
        SetWindowSubclass(hBtnCancel, BtnSubclassProc, 1, 0);

        HWND hChild = GetWindow(hdlg, GW_CHILD);
        while (hChild) { SendMessage(hChild, WM_SETFONT, (WPARAM)s_hFont, TRUE); hChild = GetWindow(hChild, GW_HWNDNEXT); }

        HWND hCombo = GetDlgItem(hdlg, IDC_COMBO_LANG);
        SendMessage(hCombo, CB_RESETCONTENT, 0, 0);
        SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)STR_LANG_EN);
        SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)STR_LANG_PT);
        SendMessage(hCombo, CB_SETCURSEL, (WPARAM)d->cfg->language, 0);
        SendDlgItemMessage(hdlg, IDC_CHK_STARTUP, BM_SETCHECK, d->cfg->start_with_windows ? BST_CHECKED : BST_UNCHECKED, 0);
        return TRUE;
    }
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
        SetBkMode((HDC)wParam, TRANSPARENT);
        SetTextColor((HDC)wParam, COL_TEXT);
        return (INT_PTR)s_hbrBg;
    case WM_DRAWITEM:
        DrawFlatButton((LPDRAWITEMSTRUCT)lParam);
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK: {
            int lang = (int)SendDlgItemMessage(hdlg, IDC_COMBO_LANG, CB_GETCURSEL, 0, 0);
            BOOL startup = (SendDlgItemMessage(hdlg, IDC_CHK_STARTUP, BM_GETCHECK, 0, 0) == BST_CHECKED);
            d->cfg->language = (lang >= 0) ? lang : 0;
            d->cfg->start_with_windows = startup;
            HKEY hKey;
            if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
                if (startup) {
                    WCHAR exePath[MAX_PATH]; GetModuleFileName(NULL, exePath, MAX_PATH);
                    RegSetValueEx(hKey, L"Checkpoint", 0, REG_SZ, (BYTE*)exePath, (DWORD)((wcslen(exePath)+1)*sizeof(WCHAR)));
                } else RegDeleteValue(hKey, L"Checkpoint");
                RegCloseKey(hKey);
            }
            d->ok = TRUE; EndDialog(hdlg, IDOK); break;
        }
        case IDCANCEL: EndDialog(hdlg, IDCANCEL); break;
        }
        return TRUE;
    }
    return FALSE;
}

BOOL gui_show_settings_dialog(HWND hwndParent, Config *cfg) {
    SettingsDlgData d = { cfg, FALSE };
    return (DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_SETTINGS), hwndParent, SettingsDlgProc, (LPARAM)&d) == IDOK && d.ok);
}

/* =========================================================================
 *  About Dialog
 * ========================================================================= */
static INT_PTR CALLBACK AboutDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    (void)lParam;
    switch (msg) {
    case WM_INITDIALOG:
        ApplyDarkMode(hdlg);
        SetWindowText(hdlg, STR_DLG_ABOUT);
        SetDlgItemText(hdlg, IDOK, STR_BTN_CLOSE);
        {
            HWND hBtnOk = GetDlgItem(hdlg, IDOK);
            SetWindowLong(hBtnOk, GWL_STYLE, GetWindowLong(hBtnOk, GWL_STYLE) | BS_OWNERDRAW);
            SetWindowSubclass(hBtnOk, BtnSubclassProc, 1, 0);

            HWND hChild = GetWindow(hdlg, GW_CHILD);
            while (hChild) { SendMessage(hChild, WM_SETFONT, (WPARAM)s_hFont, TRUE); hChild = GetWindow(hChild, GW_HWNDNEXT); }

            HICON hIco = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_ICON_MAIN));
            if (!hIco) hIco = LoadIcon(NULL, IDI_APPLICATION);
            SendDlgItemMessage(hdlg, IDC_ABOUT_ICON, STM_SETICON, (WPARAM)hIco, 0);
            SetDlgItemText(hdlg, 1401, STR_ABOUT_BODY);
        }
        return TRUE;
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
        SetBkMode((HDC)wParam, TRANSPARENT);
        SetTextColor((HDC)wParam, COL_TEXT);
        return (INT_PTR)s_hbrBg;
    case WM_DRAWITEM:
        DrawFlatButton((LPDRAWITEMSTRUCT)lParam);
        return TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) EndDialog(hdlg, IDOK);
        return TRUE;
    }
    return FALSE;
}

void gui_show_about_dialog(HWND hwndParent) {
    DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_ABOUT), hwndParent, AboutDlgProc);
}
