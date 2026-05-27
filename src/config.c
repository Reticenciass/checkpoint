/*
 * config.c
 * Load / save configuration from %APPDATA%\Checkpoint\checkpoint.ini
 * Uses Windows GetPrivateProfileString / WritePrivateProfileString (INI API).
 */
#include "config.h"
#include "strings.h"
#include <shlobj.h>
#include <stdio.h>
#include <wchar.h>

/* Virtual-key name table -------------------------------------------------- */
typedef struct { UINT vk; const WCHAR *name; } VKeyName;

static const VKeyName s_vkNames[] = {
    {VK_F1,  L"F1"},  {VK_F2,  L"F2"},  {VK_F3,  L"F3"},  {VK_F4,  L"F4"},
    {VK_F5,  L"F5"},  {VK_F6,  L"F6"},  {VK_F7,  L"F7"},  {VK_F8,  L"F8"},
    {VK_F9,  L"F9"},  {VK_F10, L"F10"}, {VK_F11, L"F11"}, {VK_F12, L"F12"},
    {VK_RETURN,   L"Enter"},  {VK_SPACE,  L"Space"},  {VK_TAB,   L"Tab"},
    {VK_BACK,     L"Back"},   {VK_DELETE, L"Delete"}, {VK_INSERT,L"Insert"},
    {VK_HOME,     L"Home"},   {VK_END,    L"End"},    {VK_PRIOR, L"PgUp"},
    {VK_NEXT,     L"PgDn"},   {VK_LEFT,   L"Left"},   {VK_RIGHT, L"Right"},
    {VK_UP,       L"Up"},     {VK_DOWN,   L"Down"},   {VK_ESCAPE,L"Esc"},
    {VK_NUMPAD0,  L"Num0"},   {VK_NUMPAD1,L"Num1"},   {VK_NUMPAD2,L"Num2"},
    {VK_NUMPAD3,  L"Num3"},   {VK_NUMPAD4,L"Num4"},   {VK_NUMPAD5,L"Num5"},
    {VK_NUMPAD6,  L"Num6"},   {VK_NUMPAD7,L"Num7"},   {VK_NUMPAD8,L"Num8"},
    {VK_NUMPAD9,  L"Num9"},   {VK_MULTIPLY,L"Num*"},  {VK_ADD,   L"Num+"},
    {VK_SUBTRACT, L"Num-"},   {VK_DIVIDE, L"Num/"},   {VK_DECIMAL,L"Num."},
    {VK_OEM_1,    L";"},      {VK_OEM_PLUS,L"="},     {VK_OEM_COMMA,L","},
    {VK_OEM_MINUS,L"-"},      {VK_OEM_PERIOD,L"."},   {VK_OEM_2,L"/"},
    {VK_OEM_3,    L"`"},      {VK_OEM_4,  L"["},      {VK_OEM_5, L"\\"},
    {VK_OEM_6,    L"]"},      {VK_OEM_7,  L"'"},
    {0, NULL}
};

/* Return a readable name for a VK code */
static const WCHAR *vk_to_name(UINT vk) {
    for (int i = 0; s_vkNames[i].vk != 0; i++) {
        if (s_vkNames[i].vk == vk) return s_vkNames[i].name;
    }
    return NULL;
}

/* Return VK code for a stored name string (reverse lookup) */
static UINT name_to_vk(const WCHAR *name) {
    /* Single letter / digit? */
    if (name[0] != L'\0' && name[1] == L'\0') {
        WCHAR ch = name[0];
        /* A-Z */
        if (ch >= L'A' && ch <= L'Z') return (UINT)ch;
        /* 0-9 */
        if (ch >= L'0' && ch <= L'9') return (UINT)ch;
    }
    for (int i = 0; s_vkNames[i].vk != 0; i++) {
        if (_wcsicmp(s_vkNames[i].name, name) == 0) return s_vkNames[i].vk;
    }
    return 0;
}

/* -------------------------------------------------------------------------- */

void config_init(Config *cfg) {
    ZeroMemory(cfg, sizeof(*cfg));
    cfg->language = 0; /* EN */
    cfg->start_with_windows = FALSE;
    cfg->paused = FALSE;
}

void config_get_path(WCHAR *out, int maxChars) {
    WCHAR appData[MAX_PATH] = {0};
    SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, appData);
    /* Ensure directory exists */
    WCHAR dir[MAX_PATH];
    _snwprintf_s(dir, MAX_PATH, _TRUNCATE, L"%s\\Checkpoint", appData);
    CreateDirectory(dir, NULL);
    _snwprintf_s(out, maxChars, _TRUNCATE, L"%s\\checkpoint.ini", dir);
}

BOOL config_load(Config *cfg) {
    WCHAR path[MAX_PATH];
    config_get_path(path, MAX_PATH);

    /* Settings */
    cfg->language           = (int)GetPrivateProfileInt(L"settings", L"language",           0, path);
    cfg->start_with_windows = (BOOL)GetPrivateProfileInt(L"settings", L"start_with_windows", 0, path);
    cfg->paused             = (BOOL)GetPrivateProfileInt(L"settings", L"paused",             0, path);

    /* Count */
    cfg->count = (int)GetPrivateProfileInt(L"settings", L"bind_count", 0, path);
    if (cfg->count < 0)        cfg->count = 0;
    if (cfg->count > MAX_BINDS) cfg->count = MAX_BINDS;

    for (int i = 0; i < cfg->count; i++) {
        WCHAR sec[32];
        _snwprintf_s(sec, 32, _TRUNCATE, L"bind%d", i);
        Bind *b = &cfg->binds[i];
        b->id = i + 1;

        GetPrivateProfileString(sec, L"name",   L"Unnamed",  b->name,   128,      path);
        b->enabled     = (BOOL)GetPrivateProfileInt(sec, L"enabled",     1, path);
        b->action_type = (ActionType)GetPrivateProfileInt(sec, L"action_type", 0, path);
        GetPrivateProfileString(sec, L"target", L"",         b->target, MAX_PATH, path);
        GetPrivateProfileString(sec, L"args",   L"",         b->args,   512,      path);

        /* Modifiers stored as bitmask */
        b->modifiers = (UINT)GetPrivateProfileInt(sec, L"modifiers", 0, path);

        /* VKey stored as its name string */
        WCHAR vkStr[32] = {0};
        GetPrivateProfileString(sec, L"vkey", L"", vkStr, 32, path);
        b->vkey = name_to_vk(vkStr);
    }
    return TRUE;
}

static void write_int(const WCHAR *sec, const WCHAR *key, int val, const WCHAR *path) {
    WCHAR buf[32];
    _snwprintf_s(buf, 32, _TRUNCATE, L"%d", val);
    WritePrivateProfileString(sec, key, buf, path);
}

BOOL config_save(const Config *cfg) {
    WCHAR path[MAX_PATH];
    config_get_path(path, MAX_PATH);

    write_int(L"settings", L"language",           cfg->language,           path);
    write_int(L"settings", L"start_with_windows", cfg->start_with_windows, path);
    write_int(L"settings", L"paused",             cfg->paused,             path);
    write_int(L"settings", L"bind_count",         cfg->count,              path);

    for (int i = 0; i < cfg->count; i++) {
        WCHAR sec[32];
        _snwprintf_s(sec, 32, _TRUNCATE, L"bind%d", i);
        const Bind *b = &cfg->binds[i];

        WritePrivateProfileString(sec, L"name",        b->name,   path);
        write_int(sec, L"enabled",     (int)b->enabled,     path);
        write_int(sec, L"action_type", (int)b->action_type, path);
        WritePrivateProfileString(sec, L"target",      b->target, path);
        WritePrivateProfileString(sec, L"args",        b->args,   path);
        write_int(sec, L"modifiers",   (int)b->modifiers,   path);

        /* Store vkey as name string */
        const WCHAR *vkName = vk_to_name(b->vkey);
        if (vkName) {
            WritePrivateProfileString(sec, L"vkey", vkName, path);
        } else if (b->vkey >= L'A' && b->vkey <= L'Z') {
            WCHAR ch[3] = {(WCHAR)b->vkey, 0};
            WritePrivateProfileString(sec, L"vkey", ch, path);
        } else if (b->vkey >= L'0' && b->vkey <= L'9') {
            WCHAR ch[3] = {(WCHAR)b->vkey, 0};
            WritePrivateProfileString(sec, L"vkey", ch, path);
        } else {
            WritePrivateProfileString(sec, L"vkey", L"", path);
        }
    }

    /* Remove orphan sections from old binds */
    for (int i = cfg->count; i < MAX_BINDS; i++) {
        WCHAR sec[32];
        _snwprintf_s(sec, 32, _TRUNCATE, L"bind%d", i);
        WCHAR tmp[8];
        if (GetPrivateProfileString(sec, L"name", L"__none__", tmp, 8, path) == 0 ||
            wcscmp(tmp, L"__none__") == 0) break;
        WritePrivateProfileString(sec, NULL, NULL, path); /* delete section */
    }
    return TRUE;
}

int config_add_bind(Config *cfg, const Bind *b) {
    if (cfg->count >= MAX_BINDS) return -1;
    int idx = cfg->count;
    cfg->binds[idx] = *b;
    cfg->binds[idx].id = idx + 1;
    cfg->count++;
    return idx;
}

BOOL config_update_bind(Config *cfg, int idx, const Bind *b) {
    if (idx < 0 || idx >= cfg->count) return FALSE;
    int savedId = cfg->binds[idx].id;
    cfg->binds[idx] = *b;
    cfg->binds[idx].id = savedId;
    return TRUE;
}

BOOL config_remove_bind(Config *cfg, int idx) {
    if (idx < 0 || idx >= cfg->count) return FALSE;
    for (int i = idx; i < cfg->count - 1; i++) {
        cfg->binds[i] = cfg->binds[i + 1];
        cfg->binds[i].id = i + 1;
    }
    cfg->count--;
    return TRUE;
}

/* Human-readable key string, e.g. "Ctrl+Shift+F" */
void bind_key_string(const Bind *b, WCHAR *out, int maxChars) {
    out[0] = L'\0';
    if (b->vkey == 0 && b->modifiers == 0) {
        _snwprintf_s(out, maxChars, _TRUNCATE, L"(none)");
        return;
    }
    if (b->modifiers & MOD_WIN)     wcsncat_s(out, maxChars, L"Win+",  _TRUNCATE);
    if (b->modifiers & MOD_CONTROL) wcsncat_s(out, maxChars, L"Ctrl+", _TRUNCATE);
    if (b->modifiers & MOD_ALT)     wcsncat_s(out, maxChars, L"Alt+",  _TRUNCATE);
    if (b->modifiers & MOD_SHIFT)   wcsncat_s(out, maxChars, L"Shift+",_TRUNCATE);

    if (b->vkey == 0) return;

    const WCHAR *vkName = vk_to_name(b->vkey);
    if (vkName) {
        wcsncat_s(out, maxChars, vkName, _TRUNCATE);
    } else if (b->vkey >= L'A' && b->vkey <= L'Z') {
        WCHAR ch[2] = {(WCHAR)b->vkey, 0};
        wcsncat_s(out, maxChars, ch, _TRUNCATE);
    } else if (b->vkey >= L'0' && b->vkey <= L'9') {
        WCHAR ch[2] = {(WCHAR)b->vkey, 0};
        wcsncat_s(out, maxChars, ch, _TRUNCATE);
    } else {
        WCHAR hex[16];
        _snwprintf_s(hex, 16, _TRUNCATE, L"0x%02X", b->vkey);
        wcsncat_s(out, maxChars, hex, _TRUNCATE);
    }
}

void bind_action_string(const Bind *b, WCHAR *out, int maxChars) {
    static const WCHAR *actEN[] = { L"URL", L"App", L"Window", L"Command" };
    static const WCHAR *actPT[] = { L"URL", L"App", L"Janela",  L"Comando" };
    int t = (int)b->action_type;
    if (t < 0 || t >= ACTION_COUNT) t = 0;
    _snwprintf_s(out, maxChars, _TRUNCATE, L"%s", g_lang == LANG_PT ? actPT[t] : actEN[t]);
}
