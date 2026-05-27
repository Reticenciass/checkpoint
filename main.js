const { app, BrowserWindow, ipcMain, globalShortcut, Tray, Menu, shell, dialog } = require('electron');
const path = require('path');
const fs = require('fs');
const { exec } = require('child_process');

let mainWindow;
let tray;
const CONFIG_PATH = path.join(app.getPath('userData'), 'config.json');
const BACKUP_DIR = path.join(app.getPath('userData'), 'backups');

// Resolve icon path: works both in dev (__dirname) and packaged (resourcesPath)
function getIconPath() {
  const devPath = path.join(__dirname, 'checkpoint_icon.png');
  if (fs.existsSync(devPath)) return devPath;
  return path.join(process.resourcesPath, 'checkpoint_icon.png');
}

// Default config
let config = {
  paused: false,
  binds: [],
  profiles: {},
  activeProfile: 'default',
  settings: { notifications: true, autoBackup: true, startup: false }
};

// ============================================================
// CONFIG
// ============================================================
function loadConfig() {
  if (fs.existsSync(CONFIG_PATH)) {
    try {
      config = JSON.parse(fs.readFileSync(CONFIG_PATH, 'utf-8'));
      // Backward compat: ensure new fields exist
      config.binds = (config.binds || []).map(bind => ({
        ...bind,
        usageCount: bind.usageCount || 0,
        lastUsed: bind.lastUsed || null
      }));
      config.profiles = config.profiles || {};
      config.activeProfile = config.activeProfile || 'default';
      config.settings = config.settings || { notifications: true, autoBackup: true, startup: false };
    } catch(e) { console.error('Error loading config:', e); }
  }
}

function saveConfig() {
  fs.writeFileSync(CONFIG_PATH, JSON.stringify(config, null, 2));
}

// ============================================================
// BACKUP
// ============================================================
function createAutoBackup() {
  if (!fs.existsSync(CONFIG_PATH)) return;
  try {
    if (!fs.existsSync(BACKUP_DIR)) {
      fs.mkdirSync(BACKUP_DIR, { recursive: true });
    }
    const timestamp = new Date().toISOString().replace(/[:.]/g, '-');
    const backupPath = path.join(BACKUP_DIR, `config-backup-${timestamp}.json`);
    fs.copyFileSync(CONFIG_PATH, backupPath);
    // Keep only last 10 backups
    const files = fs.readdirSync(BACKUP_DIR)
      .filter(f => f.startsWith('config-backup-'))
      .sort()
      .reverse();
    files.slice(10).forEach(f => fs.unlinkSync(path.join(BACKUP_DIR, f)));
  } catch (e) { console.error('Backup failed:', e); }
}

// ============================================================
// ACTIONS
// ============================================================
function executeAction(bind) {
  if (config.paused || !bind.enabled) return;

  // Track usage
  bind.usageCount = (bind.usageCount || 0) + 1;
  bind.lastUsed = new Date().toISOString();
  saveConfig();

  try {
    switch (bind.actionType) {
      case 'url':
        shell.openExternal(bind.target);
        break;
      case 'app':
        shell.openPath(bind.target);
        break;
      case 'cmd':
        exec(bind.target);
        break;
      case 'focus':
        const safeTitle = bind.target.replace(/'/g, "''");
        const psContent = `
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
using System.Text;
using System.Collections.Generic;
public class Win32 {
    public delegate bool EnumWindowsProc(IntPtr hwnd, IntPtr lParam);
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern int GetWindowText(IntPtr hWnd, StringBuilder lpString, int nMaxCount);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);
    [DllImport("user32.dll")] public static extern bool IsIconic(IntPtr hWnd);
    public static List<IntPtr> FindWindows(string fragment) {
        var result = new List<IntPtr>();
        string lower = fragment.ToLower();
        EnumWindows((hwnd, _) => {
            if (!IsWindowVisible(hwnd)) return true;
            var sb = new StringBuilder(512);
            GetWindowText(hwnd, sb, 512);
            string title = sb.ToString().ToLower();
            if (title.Length > 0 && title.Contains(lower)) result.Add(hwnd);
            return true;
        }, IntPtr.Zero);
        return result;
    }
}
'@
$wins = [Win32]::FindWindows('${safeTitle}')
if ($wins.Count -gt 0) {
    $hwnd = $wins[0]
    if ([Win32]::IsIconic($hwnd)) { [Win32]::ShowWindow($hwnd, 9) }
    [Win32]::SetForegroundWindow($hwnd)
}
`;
        const tmpFile = path.join(app.getPath('temp'), 'checkpoint_focus.ps1');
        fs.writeFileSync(tmpFile, psContent);
        exec(`powershell.exe -ExecutionPolicy Bypass -NoProfile -File "${tmpFile}"`);
        break;
    }
  } catch(e) {
    console.error('Action failed:', e);
  }
}

// ============================================================
// HOTKEYS
// ============================================================
function registerAllHotkeys() {
  globalShortcut.unregisterAll();
  if (config.paused) return;

  config.binds.forEach(bind => {
    if (!bind.enabled || !bind.shortcut) return;
    try {
      globalShortcut.register(bind.shortcut, () => {
        executeAction(bind);
      });
    } catch(e) { console.error('Failed to register:', bind.shortcut); }
  });
}

// ============================================================
// TRAY
// ============================================================
function createTray() {
  tray = new Tray(getIconPath());
  updateTrayMenu();
  tray.setToolTip('Checkpoint');
  tray.on('double-click', () => {
    mainWindow.show();
  });
}

function updateTrayMenu() {
  if (!tray) return;
  const contextMenu = Menu.buildFromTemplate([
    { label: 'Open Checkpoint', click: () => mainWindow.show() },
    { type: 'separator' },
    {
      label: config.paused ? 'Resume Hotkeys' : 'Pause All Hotkeys',
      click: () => {
        config.paused = !config.paused;
        saveConfig();
        registerAllHotkeys();
        updateTrayMenu();
      }
    },
    { type: 'separator' },
    { label: 'Exit', click: () => { app.isQuiting = true; app.quit(); } }
  ]);
  tray.setContextMenu(contextMenu);
}

// ============================================================
// WINDOW
// ============================================================
function createWindow() {
  mainWindow = new BrowserWindow({
    width: 960,
    height: 620,
    frame: false,
    transparent: true,
    resizable: true,
    minWidth: 800,
    minHeight: 500,
    icon: getIconPath(),
    webPreferences: {
      preload: path.join(__dirname, 'preload.js'),
      contextIsolation: true,
      nodeIntegration: false
    }
  });

  mainWindow.loadFile('index.html');

  // Hide to tray on minimize
  mainWindow.on('minimize', (e) => {
    e.preventDefault();
    mainWindow.hide();
  });

  // Hide instead of close
  mainWindow.on('close', (e) => {
    if (!app.isQuiting) {
      e.preventDefault();
      mainWindow.hide();
    }
  });
}

// ============================================================
// SINGLE INSTANCE
// ============================================================
const gotTheLock = app.requestSingleInstanceLock();
if (!gotTheLock) {
  app.quit();
} else {
  app.on('second-instance', () => {
    if (mainWindow) {
      if (!mainWindow.isVisible()) mainWindow.show();
      if (mainWindow.isMinimized()) mainWindow.restore();
      mainWindow.focus();
    }
  });

  app.whenReady().then(() => {
    loadConfig();
    createWindow();
    createTray();
    registerAllHotkeys();
  });
}

app.on('will-quit', () => {
  globalShortcut.unregisterAll();
});

// ============================================================
// IPC: WINDOW
// ============================================================
ipcMain.on('window-minimize', () => mainWindow.hide());
ipcMain.on('window-close', () => mainWindow.hide());

// ============================================================
// IPC: CONFIG
// ============================================================
ipcMain.handle('get-config', () => config);

ipcMain.handle('save-config', (e, newConfig) => {
  if (config.settings && config.settings.autoBackup) {
    createAutoBackup();
  }
  config = newConfig;
  saveConfig();
  registerAllHotkeys();
  updateTrayMenu();
  return true;
});

// ============================================================
// IPC: DIALOGS
// ============================================================
ipcMain.handle('show-open-dialog', async () => {
  const result = await dialog.showOpenDialog(mainWindow, {
    properties: ['openFile'],
    filters: [
      { name: 'Executables', extensions: ['exe', 'bat', 'cmd'] },
      { name: 'JSON Files', extensions: ['json'] },
      { name: 'All Files', extensions: ['*'] }
    ]
  });
  return result.canceled ? null : result.filePaths[0];
});

ipcMain.handle('show-save-dialog', async (event, options) => {
  const result = await dialog.showSaveDialog(mainWindow, {
    ...options,
    filters: [{ name: 'JSON Files', extensions: ['json'] }, { name: 'All Files', extensions: ['*'] }]
  });
  return result.canceled ? null : result.filePath;
});

// ============================================================
// IPC: IMPORT/EXPORT
// ============================================================
ipcMain.handle('export-config', async (event, filePath) => {
  try {
    fs.writeFileSync(filePath, JSON.stringify(config, null, 2));
    return true;
  } catch (e) { return false; }
});

ipcMain.handle('import-config', async () => {
  const result = await dialog.showOpenDialog(mainWindow, {
    properties: ['openFile'],
    filters: [{ name: 'JSON Files', extensions: ['json'] }, { name: 'All Files', extensions: ['*'] }]
  });
  if (result.canceled) return null;

  try {
    const data = JSON.parse(fs.readFileSync(result.filePaths[0], 'utf-8'));
    if (data && Array.isArray(data.binds)) {
      return data;
    }
    return false;
  } catch (e) { return false; }
});

// ============================================================
// IPC: PROFILES
// ============================================================
ipcMain.handle('switch-profile', (event, profileName) => {
  if (!config.profiles) config.profiles = {};
  // Save current binds to current profile
  config.profiles[config.activeProfile || 'default'] = { binds: [...config.binds] };
  // Load new profile
  if (config.profiles[profileName]) {
    config.binds = config.profiles[profileName].binds || [];
    config.activeProfile = profileName;
  }
  saveConfig();
  registerAllHotkeys();
  return config;
});

ipcMain.handle('create-profile', (event, profileName) => {
  if (!config.profiles) config.profiles = {};
  config.profiles[profileName] = { binds: [] };
  saveConfig();
  return config.profiles;
});

ipcMain.handle('delete-profile', (event, profileName) => {
  if (profileName === 'default') return false;
  if (config.profiles && config.profiles[profileName]) {
    delete config.profiles[profileName];
    if (config.activeProfile === profileName) {
      config.activeProfile = 'default';
      config.binds = config.profiles.default?.binds || [];
    }
    saveConfig();
    registerAllHotkeys();
  }
  return true;
});

// ============================================================
// IPC: BACKUPS
// ============================================================
ipcMain.handle('create-backup', () => {
  createAutoBackup();
  return true;
});

ipcMain.handle('list-backups', () => {
  if (!fs.existsSync(BACKUP_DIR)) return [];
  return fs.readdirSync(BACKUP_DIR)
    .filter(f => f.startsWith('config-backup-'))
    .sort()
    .reverse();
});

ipcMain.handle('restore-backup', (event, backupFileName) => {
  const backupPath = path.join(BACKUP_DIR, backupFileName);
  if (!fs.existsSync(backupPath)) return false;
  try {
    const data = JSON.parse(fs.readFileSync(backupPath, 'utf-8'));
    config = data;
    // Ensure backward compat
    config.binds = (config.binds || []).map(bind => ({
      ...bind,
      usageCount: bind.usageCount || 0,
      lastUsed: bind.lastUsed || null
    }));
    config.profiles = config.profiles || {};
    config.activeProfile = config.activeProfile || 'default';
    config.settings = config.settings || { notifications: true, autoBackup: true, startup: false };
    saveConfig();
    registerAllHotkeys();
    return config;
  } catch (e) { return false; }
});

// ============================================================
// IPC: EXTERNAL
// ============================================================
ipcMain.handle('open-external', (event, url) => {
  shell.openExternal(url);
});
