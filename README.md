# Checkpoint

**Global Hotkey Manager for Windows** — Assign keyboard shortcuts to open apps, URLs, focus windows, or run commands. Runs silently in the system tray.

![Checkpoint](checkpoint_icon.png)

---

## Features

- **Global Hotkeys** — Shortcuts work in any app, even games
- **Open URL** — Open any URL in your default browser
- **Open App** — Launch any executable with optional arguments
- **Focus Window** — Bring a running window to foreground by title
- **Run Command** — Execute shell commands silently
- **Profiles** — Switch between different sets of binds (e.g. Gaming, Work)
- **Statistics** — Track which binds you use the most
- **Import/Export** — Share your binds as JSON files
- **Auto-backup** — Automatic config backups before changes
- **Persistent** — Config saved to `%APPDATA%\Checkpoint\config.json`
- **System Tray** — Minimize to tray, pause all hotkeys from menu
- **Start with Windows** — Optional auto-start via Settings

---

## Getting Started (Electron Version)

### Requirements
- [Node.js](https://nodejs.org/) (v18 or higher)
- Windows 10/11

### Install & Run

```bash
git clone https://github.com/Reticenciass/checkpoint.git
cd checkpoint
npm install
npm start
```

### Build Executable

```bash
npm install
npm run build
```

Output: `dist/Checkpoint-win32-x64/Checkpoint.exe`

---

## Native Version (C/Win32)

The `src/` directory contains a standalone native version written in C using the Win32 API.

### Requirements
- Visual Studio 2022 (Community or higher) with **Desktop development with C++** workload

### Build

1. Open `Checkpoint.sln` in Visual Studio
2. Select **Release | x64** configuration
3. Press **Ctrl+Shift+B** to build
4. Output: `bin\x64\Release\Checkpoint.exe`

> The `.ico` icon is already pre-generated in `resources\checkpoint.ico`. To regenerate from PNG:
> ```powershell
> .\Convert-Icon.ps1
> ```

---

## Usage

### Adding a Bind

1. Click **+ Add** (or double-click an existing bind to edit)
2. Enter a **Name** (e.g. "Open GitHub")
3. Click **Capture Key** and press your desired combination (e.g. `Shift+F`)
4. Choose an **Action type**:
   - **Open URL in Browser** — enter a URL like `https://github.com`
   - **Open Application** — enter the `.exe` path (use Browse...)
   - **Focus Window** — enter part of a window title (e.g. `Visual Studio Code`)
   - **Run Command** — enter a shell command (e.g. `cmd.exe /c start notepad`)
5. Click **Save**

### Examples

| Name | Shortcut | Action | Target |
|------|----------|--------|--------|
| Open GitHub | Shift+F | URL | `https://github.com` |
| Open VS Code | Alt+G | App | `C:\...\Code.exe` |
| Focus Terminal | Ctrl+T | Window | `Windows Terminal` |
| Run build | Alt+B | Command | `cmd.exe /c build.bat` |

### Tray Icon

- **Double-click** — open manager window
- **Right-click** — context menu with Pause/Resume and Exit

---

## Project Structure

```
Checkpoint/
├── main.js                 Electron main process
├── preload.js              Context bridge (IPC)
├── renderer.js             UI logic
├── index.html              App layout
├── style.css               Styles
├── package.json            Dependencies & scripts
├── build.js                Build script for standalone .exe
├── checkpoint_icon.png     App icon (512x512)
├── Convert-Icon.ps1        PNG to ICO converter
│
├── src/                    Native C version
│   ├── main.c              Entry point (wWinMain)
│   ├── config.c/h          INI-based config
│   ├── hotkeys.c/h         RegisterHotKey management
│   ├── actions.c/h         ShellExecute + window focus
│   ├── gui.c/h             Main window, dialogs
│   └── strings.h           Bilingual string table (EN/PT)
│
├── resources/
│   ├── resource.h           Resource IDs
│   ├── checkpoint.rc        Icon, menus, dialog templates
│   └── checkpoint.ico       Multi-size icon
│
├── Checkpoint.sln           Visual Studio solution
└── Checkpoint.vcxproj       VS project file
```

---

## Config File (Native Version)

Stored at `%APPDATA%\Checkpoint\checkpoint.ini`:

```ini
[settings]
language=1
start_with_windows=0
paused=0
bind_count=2

[bind0]
name=Open GitHub
enabled=1
action_type=0
target=https://github.com
modifiers=4
vkey=F

[bind1]
name=Open VS Code
enabled=1
action_type=1
target=C:\Program Files\Microsoft VS Code\Code.exe
modifiers=1
vkey=G
```

**Modifier bitmask**: `MOD_ALT=1`, `MOD_CONTROL=2`, `MOD_SHIFT=4`, `MOD_WIN=8`

---

## License

MIT — free for personal and commercial use.
