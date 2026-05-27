const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('api', {
  // Window controls
  minimize: () => ipcRenderer.send('window-minimize'),
  close: () => ipcRenderer.send('window-close'),

  // Config management
  getConfig: () => ipcRenderer.invoke('get-config'),
  saveConfig: (config) => ipcRenderer.invoke('save-config', config),

  // Dialogs
  showOpenDialog: () => ipcRenderer.invoke('show-open-dialog'),
  showSaveDialog: (options) => ipcRenderer.invoke('show-save-dialog', options),

  // Import/Export
  exportConfig: (filePath) => ipcRenderer.invoke('export-config', filePath),
  importConfig: () => ipcRenderer.invoke('import-config'),

  // Profiles
  switchProfile: (name) => ipcRenderer.invoke('switch-profile', name),
  createProfile: (name) => ipcRenderer.invoke('create-profile', name),
  deleteProfile: (name) => ipcRenderer.invoke('delete-profile', name),

  // Backups
  createBackup: () => ipcRenderer.invoke('create-backup'),
  listBackups: () => ipcRenderer.invoke('list-backups'),
  restoreBackup: (fileName) => ipcRenderer.invoke('restore-backup', fileName),

  // External links
  openExternal: (url) => ipcRenderer.invoke('open-external', url),
});
