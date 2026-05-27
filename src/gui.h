#pragma once
#ifndef GUI_H
#define GUI_H

#include <windows.h>
#include "config.h"

/* Create and register the main application window.
   Returns NULL on failure. */
HWND gui_create_main_window(HINSTANCE hInstance);

/* Refresh the ListView to reflect current config state. */
void gui_refresh_list(HWND hwndList, Config *cfg);

/* Update the status-bar text based on current config/pause state. */
void gui_update_status(HWND hwndStatus, Config *cfg, BOOL paused);

/* Show the Add/Edit Bind dialog.
   If bindIdx == -1 it is an Add; otherwise Edit of that index.
   Returns TRUE if the user confirmed and config was modified. */
BOOL gui_show_edit_dialog(HWND hwndParent, Config *cfg, int bindIdx);

/* Show the Settings dialog.
   Returns TRUE if settings changed. */
BOOL gui_show_settings_dialog(HWND hwndParent, Config *cfg);

/* Show the About dialog. */
void gui_show_about_dialog(HWND hwndParent);

/* Re-apply language to all main window controls (called after language change). */
void gui_apply_language(HWND hwnd);

/* External references set by main.c */
extern HINSTANCE g_hInstance;
extern Config    g_config;
extern BOOL      g_paused;

#endif /* GUI_H */
