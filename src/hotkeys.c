/*
 * hotkeys.c
 * Global hotkey registration using Windows RegisterHotKey API.
 * Each bind uses a unique ID: HOTKEY_ID_BASE + bind_index.
 */
#include "hotkeys.h"
#include <windows.h>

void hotkeys_register_all(HWND hwnd, Config *cfg) {
    for (int i = 0; i < cfg->count; i++) {
        if (cfg->binds[i].enabled) {
            hotkey_register(hwnd, &cfg->binds[i]);
        }
    }
}

void hotkeys_unregister_all(HWND hwnd, Config *cfg) {
    for (int i = 0; i < cfg->count; i++) {
        hotkey_unregister(hwnd, &cfg->binds[i]);
    }
}

BOOL hotkey_register(HWND hwnd, const Bind *b) {
    if (b->vkey == 0) return FALSE;
    int hkId = BIND_TO_HOTKEY_ID(b->id - 1);
    /* MOD_NOREPEAT prevents repeated triggers on key hold */
    return RegisterHotKey(hwnd, hkId, b->modifiers | MOD_NOREPEAT, b->vkey);
}

void hotkey_unregister(HWND hwnd, const Bind *b) {
    int hkId = BIND_TO_HOTKEY_ID(b->id - 1);
    UnregisterHotKey(hwnd, hkId);
}
