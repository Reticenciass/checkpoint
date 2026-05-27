#pragma once
#ifndef CONFIG_H
#define CONFIG_H

#include <windows.h>

/* Maximum number of binds */
#define MAX_BINDS 256

/* Action types */
typedef enum {
    ACTION_OPEN_URL  = 0,
    ACTION_OPEN_APP  = 1,
    ACTION_FOCUS_WIN = 2,
    ACTION_RUN_CMD   = 3,
    ACTION_COUNT     = 4
} ActionType;

/* A single hotkey -> action mapping */
typedef struct {
    int        id;                  /* unique ID (1-based) */
    BOOL       enabled;
    UINT       modifiers;           /* MOD_ALT | MOD_CONTROL | MOD_SHIFT | MOD_WIN */
    UINT       vkey;                /* Virtual key code */
    WCHAR      name[128];
    ActionType action_type;
    WCHAR      target[MAX_PATH];    /* URL, exe path, window title, or cmd */
    WCHAR      args[512];           /* optional extra arguments */
} Bind;

/* Application configuration */
typedef struct {
    Bind  binds[MAX_BINDS];
    int   count;
    int   language;             /* 0 = EN, 1 = PT */
    BOOL  start_with_windows;
    BOOL  paused;
} Config;

/* Initialize config with defaults */
void config_init(Config *cfg);

/* Returns path to %APPDATA%\Checkpoint\checkpoint.ini */
void config_get_path(WCHAR *out, int maxChars);

/* Load from INI. Returns TRUE on success. */
BOOL config_load(Config *cfg);

/* Save to INI. Returns TRUE on success. */
BOOL config_save(const Config *cfg);

/* Add a copy of bind. Returns index or -1 on failure. */
int config_add_bind(Config *cfg, const Bind *b);

/* Update bind at index. Returns FALSE on bad index. */
BOOL config_update_bind(Config *cfg, int idx, const Bind *b);

/* Remove bind at index, shifting remaining binds. */
BOOL config_remove_bind(Config *cfg, int idx);

/* Returns a human-readable key string, e.g. "Ctrl+Shift+F" */
void bind_key_string(const Bind *b, WCHAR *out, int maxChars);

/* Returns a human-readable action string */
void bind_action_string(const Bind *b, WCHAR *out, int maxChars);

#endif /* CONFIG_H */
