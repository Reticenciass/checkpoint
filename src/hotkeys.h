#pragma once
#ifndef HOTKEYS_H
#define HOTKEYS_H

#include <windows.h>
#include "config.h"

/* Register all enabled binds in cfg. */
void hotkeys_register_all(HWND hwnd, Config *cfg);

/* Unregister all binds. */
void hotkeys_unregister_all(HWND hwnd, Config *cfg);

/* Register / unregister a single bind. Returns TRUE on success. */
BOOL hotkey_register(HWND hwnd, const Bind *b);
void hotkey_unregister(HWND hwnd, const Bind *b);

/* Convert bind index to RegisterHotKey ID (0-based index -> ID offset) */
#define HOTKEY_ID_BASE  100
#define BIND_TO_HOTKEY_ID(idx) (HOTKEY_ID_BASE + (idx))
#define HOTKEY_ID_TO_BIND(id)  ((id) - HOTKEY_ID_BASE)

#endif /* HOTKEYS_H */
