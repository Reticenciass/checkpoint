#pragma once
#ifndef STRINGS_H
#define STRINGS_H

#include <windows.h>

typedef enum { LANG_EN = 0, LANG_PT = 1 } AppLang;

extern AppLang g_lang;

/* Macro: picks English or Portuguese at runtime */
#define S(en, pt) (g_lang == LANG_PT ? (pt) : (en))

/* ---- App ---- */
#define STR_APP_NAME        L"Checkpoint"
#define STR_APP_CLASS       L"CheckpointMainWnd"
#define STR_APP_TITLE       S(L"Checkpoint \x2013 Global Hotkey Manager", L"Checkpoint \x2013 Gerenciador de Atalhos Globais")
#define STR_APP_DESC        S(L"Version 1.0 \x2014 Global Hotkey Manager for Windows", L"Vers\x00E3o 1.0 \x2014 Gerenciador de Atalhos Globais para Windows")

/* ---- Column headers ---- */
#define STR_COL_NAME        S(L"Name",     L"Nome")
#define STR_COL_SHORTCUT    S(L"Shortcut", L"Atalho")
#define STR_COL_ACTION      S(L"Action",   L"A\x00E7\x00E3o")
#define STR_COL_TARGET      S(L"Target",   L"Destino")
#define STR_COL_STATUS      S(L"Status",   L"Status")

/* ---- Buttons ---- */
#define STR_BTN_ADD         S(L"  + Add",        L"  + Adicionar")
#define STR_BTN_EDIT        S(L"  Edit",          L"  Editar")
#define STR_BTN_REMOVE      S(L"  Remove",        L"  Remover")
#define STR_BTN_TOGGLE      S(L"  Enable/Disable",L"  Ativar/Desativar")
#define STR_BTN_SETTINGS    S(L"  \x2699 Settings",  L"  \x2699 Configura\x00E7\x00F5es")
#define STR_BTN_OK          S(L"OK",              L"OK")
#define STR_BTN_CANCEL      S(L"Cancel",          L"Cancelar")
#define STR_BTN_CAPTURE     S(L"Capture Key",     L"Capturar Tecla")
#define STR_BTN_CAPTURING   S(L"[Press now...]",  L"[Pressione...]")
#define STR_BTN_BROWSE      S(L"Browse...",       L"Procurar...")
#define STR_BTN_CLOSE       S(L"Close",           L"Fechar")

/* ---- Dialog titles ---- */
#define STR_DLG_ADD_BIND    S(L"Add Bind",   L"Adicionar Bind")
#define STR_DLG_EDIT_BIND   S(L"Edit Bind",  L"Editar Bind")
#define STR_DLG_SETTINGS    S(L"Settings",   L"Configura\x00E7\x00F5es")
#define STR_DLG_ABOUT       S(L"About Checkpoint", L"Sobre o Checkpoint")

/* ---- Labels ---- */
#define STR_LBL_NAME        S(L"Name:",      L"Nome:")
#define STR_LBL_SHORTCUT    S(L"Shortcut:",  L"Atalho:")
#define STR_LBL_ACTION      S(L"Action:",    L"A\x00E7\x00E3o:")
#define STR_LBL_TARGET      S(L"Target:",    L"Destino:")
#define STR_LBL_ARGS        S(L"Arguments:", L"Argumentos:")
#define STR_LBL_ENABLED     S(L"Enabled",    L"Ativado")
#define STR_LBL_LANGUAGE    S(L"Language:",  L"Idioma:")
#define STR_LBL_STARTUP     S(L"Start with Windows", L"Iniciar com o Windows")

/* ---- Action types (combo) ---- */
#define STR_ACTION_OPEN_URL  S(L"Open URL in Browser",  L"Abrir URL no Navegador")
#define STR_ACTION_OPEN_APP  S(L"Open Application",     L"Abrir Aplicativo")
#define STR_ACTION_FOCUS_WIN S(L"Focus Window (by title)",L"Focar Janela (por t\x00EDtulo)")
#define STR_ACTION_RUN_CMD   S(L"Run Command",           L"Executar Comando")

/* ---- Status ---- */
#define STR_STATUS_ON       S(L"\x25CF Active",   L"\x25CF Ativo")
#define STR_STATUS_OFF      S(L"\x25CB Inactive", L"\x25CB Inativo")

/* ---- Tray menu ---- */
#define STR_TRAY_OPEN       S(L"Open Checkpoint",      L"Abrir Checkpoint")
#define STR_TRAY_PAUSE      S(L"Pause All Hotkeys",    L"Pausar Todos os Atalhos")
#define STR_TRAY_RESUME     S(L"Resume All Hotkeys",   L"Retomar Todos os Atalhos")
#define STR_TRAY_EXIT       S(L"Exit",                 L"Sair")
#define STR_TRAY_TIP        S(L"Checkpoint - Hotkey Manager", L"Checkpoint - Gerenciador de Atalhos")
#define STR_TRAY_TIP_PAUSED S(L"Checkpoint - PAUSED",  L"Checkpoint - PAUSADO")

/* ---- Hints per action type ---- */
#define STR_HINT_URL        S(L"e.g.  https://github.com", L"ex:  https://github.com")
#define STR_HINT_APP        S(L"e.g.  C:\\Windows\\notepad.exe", L"ex:  C:\\Windows\\notepad.exe")
#define STR_HINT_WIN        S(L"Part of window title  e.g.  Visual Studio Code", L"Parte do t\x00EDtulo da janela  ex:  Visual Studio Code")
#define STR_HINT_CMD        S(L"e.g.  cmd.exe /c start myapp.exe", L"ex:  cmd.exe /c start myapp.exe")

/* ---- Messages / errors ---- */
#define STR_MSG_CONFIRM_REMOVE  S(L"Remove this bind?", L"Remover este bind?")
#define STR_MSG_INVALID_KEY     S(L"Please capture a key combination first.", L"Por favor, capture uma combina\x00E7\x00E3o de teclas primeiro.")
#define STR_MSG_INVALID_TARGET  S(L"Please enter a target (URL, path, or window title).", L"Por favor, insira um destino (URL, caminho ou t\x00EDtulo da janela).")
#define STR_MSG_MAX_BINDS       S(L"Maximum of 256 binds reached.", L"N\x00FAmero m\x00E1ximo de 256 binds atingido.")
#define STR_MSG_CONFLICT        S(L"This key combination is already used by another bind.", L"Esta combina\x00E7\x00E3o j\x00E1 \x00E9 usada por outro bind.")
#define STR_MSG_HOTKEY_FAIL     S(L"Failed to register hotkey. It may be in use by another program.", L"Falha ao registrar atalho. Pode estar em uso por outro programa.")

/* ---- Status bar ---- */
#define STR_SB_ACTIVE_FMT   S(L"%d bind(s) active  \x2022  Running in tray", L"%d bind(s) ativo(s)  \x2022  Rodando na bandeja")
#define STR_SB_PAUSED       S(L"PAUSED \x2014 All hotkeys disabled", L"PAUSADO \x2014 Todos os atalhos desativados")

/* ---- Language combo ---- */
#define STR_LANG_EN         L"English"
#define STR_LANG_PT         L"Portugu\x00EAs"

/* ---- About ---- */
#define STR_ABOUT_TITLE     S(L"Checkpoint  v1.0", L"Checkpoint  v1.0")
#define STR_ABOUT_BODY      S(\
    L"Global Hotkey Manager for Windows\r\n"\
    L"Version 1.0\r\n\r\n"\
    L"Assign keyboard shortcuts to open apps, URLs,\r\n"\
    L"focus windows, or run commands.\r\n\r\n"\
    L"Runs silently in the system tray.\r\n\r\n"\
    L"Developed in C  \x2014  Win32 API",\
    L"Gerenciador de Atalhos Globais para Windows\r\n"\
    L"Vers\x00E3o 1.0\r\n\r\n"\
    L"Atribua atalhos do teclado para abrir apps, URLs,\r\n"\
    L"focar janelas ou executar comandos.\r\n\r\n"\
    L"Roda silenciosamente na bandeja do sistema.\r\n\r\n"\
    L"Desenvolvido em C  \x2014  Win32 API")

#endif /* STRINGS_H */
