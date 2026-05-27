#pragma once
#ifndef RESOURCE_H
#define RESOURCE_H

#ifndef IDC_STATIC
#define IDC_STATIC -1
#endif

/* ---- Icons ---- */
#define IDI_ICON_MAIN       101

/* ---- Menus ---- */
#define IDM_TRAY            200
#define IDM_MAIN            201

/* Tray menu items */
#define ID_TRAY_OPEN        1001
#define ID_TRAY_PAUSE       1002
#define ID_TRAY_EXIT        1003

/* Main menu items */
#define ID_FILE_EXIT        2001
#define ID_HELP_ABOUT       2002
#define ID_VIEW_SETTINGS    2003

/* ---- Dialogs ---- */
#define IDD_EDIT_BIND       301
#define IDD_ABOUT           302
#define IDD_SETTINGS        303

/* ---- Controls (Edit Bind) ---- */
#define IDC_EDIT_NAME           1100
#define IDC_EDIT_SHORTCUT       1101
#define IDC_BTN_CAPTURE         1102
#define IDC_COMBO_ACTION        1103
#define IDC_EDIT_TARGET         1104
#define IDC_BTN_BROWSE          1105
#define IDC_EDIT_ARGS           1106
#define IDC_CHK_ENABLED         1107
#define IDC_LBL_TARGET_HINT     1108
#define IDC_LBL_CAPTURE_HELP    1109

/* ---- Controls (Main Window) ---- */
#define IDC_LIST_BINDS      1200
#define IDC_BTN_ADD         1201
#define IDC_BTN_EDIT        1202
#define IDC_BTN_REMOVE      1203
#define IDC_BTN_TOGGLE      1204
#define IDC_BTN_SETTINGS_MW 1205
#define IDC_STATUSBAR       1206
#define IDC_BTN_LANG        1207

/* ---- Controls (Settings) ---- */
#define IDC_COMBO_LANG      1300
#define IDC_CHK_STARTUP     1301

/* ---- Controls (About) ---- */
#define IDC_ABOUT_ICON      1400

#endif /* RESOURCE_H */
