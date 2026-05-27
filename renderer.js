// ============================================================
// CHECKPOINT RENDERER
// ============================================================

// Window controls
document.getElementById('btn-minimize').addEventListener('click', () => window.api.minimize());
document.getElementById('btn-close').addEventListener('click', () => window.api.close());

// ============================================================
// STATE
// ============================================================
let config = { paused: false, binds: [], profiles: {}, activeProfile: 'default', settings: {} };
let editingIndex = -1;
let dragSourceIndex = null;

// ============================================================
// DOM ELEMENTS
// ============================================================
const bindsContainer = document.getElementById('binds-container');
const modal = document.getElementById('modal');
const modalTitle = document.getElementById('modal-title');
const inputName = document.getElementById('bind-name');
const inputShortcut = document.getElementById('bind-shortcut');
const selectAction = document.getElementById('bind-action');
const inputTarget = document.getElementById('bind-target');
const lblTarget = document.getElementById('lbl-target');
const btnBrowse = document.getElementById('btn-browse');
const btnCapture = document.getElementById('btn-capture');
const searchInput = document.getElementById('search-input');
const statusIndicator = document.getElementById('status-indicator');
const statusText = document.getElementById('status-text');
const bindCountBadge = document.getElementById('bind-count');

// ============================================================
// ACTION TYPE ICONS
// ============================================================
const ACTION_ICONS = {
  url: `<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M10 13a5 5 0 0 0 7.54.54l3-3a5 5 0 0 0-7.07-7.07l-1.72 1.71"/><path d="M14 11a5 5 0 0 0-7.54-.54l-3 3a5 5 0 0 0 7.07 7.07l1.71-1.71"/></svg>`,
  app: `<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="3" width="18" height="18" rx="2"/><path d="M12 8v8"/><path d="M8 12h8"/></svg>`,
  cmd: `<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="4 17 10 11 4 5"/><line x1="12" y1="19" x2="20" y2="19"/></svg>`,
  focus: `<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M4 15s1-1 4-1 5 2 8 2 4-1 4-1V3s-1 1-4 1-5-2-8-2-4 1-4 1z"/><line x1="4" y1="22" x2="4" y2="15"/></svg>`
};

const ACTION_NAMES = {
  url: 'URL',
  app: 'App',
  cmd: 'Command',
  focus: 'Window'
};

// ============================================================
// UTILITY
// ============================================================
function escapeHtml(text) {
  const div = document.createElement('div');
  div.textContent = text;
  return div.innerHTML;
}

// ============================================================
// CUSTOM PROMPT / CONFIRM (native ones don't work in frameless windows)
// ============================================================
function showPrompt(title, label, placeholder) {
  return new Promise((resolve) => {
    const overlay = document.getElementById('prompt-modal');
    const titleEl = document.getElementById('prompt-title');
    const labelEl = document.getElementById('prompt-label');
    const input = document.getElementById('prompt-input');
    const okBtn = document.getElementById('prompt-ok');
    const cancelBtn = document.getElementById('prompt-cancel');

    titleEl.textContent = title;
    labelEl.textContent = label;
    input.value = '';
    input.placeholder = placeholder || '';
    overlay.classList.add('active');
    setTimeout(() => input.focus(), 100);

    const cleanup = () => {
      overlay.classList.remove('active');
      okBtn.replaceWith(okBtn.cloneNode(true));
      cancelBtn.replaceWith(cancelBtn.cloneNode(true));
      input.removeEventListener('keydown', onKey);
    };

    const onKey = (e) => {
      if (e.key === 'Enter') { cleanup(); resolve(input.value); }
      if (e.key === 'Escape') { cleanup(); resolve(null); }
    };
    input.addEventListener('keydown', onKey);

    document.getElementById('prompt-ok').addEventListener('click', () => {
      cleanup();
      resolve(input.value);
    });
    document.getElementById('prompt-cancel').addEventListener('click', () => {
      cleanup();
      resolve(null);
    });
  });
}

function showConfirm(message) {
  return new Promise((resolve) => {
    const overlay = document.getElementById('prompt-modal');
    const titleEl = document.getElementById('prompt-title');
    const labelEl = document.getElementById('prompt-label');
    const input = document.getElementById('prompt-input');
    const okBtn = document.getElementById('prompt-ok');
    const cancelBtn = document.getElementById('prompt-cancel');

    titleEl.textContent = 'Confirm';
    labelEl.textContent = message;
    input.style.display = 'none';
    overlay.classList.add('active');

    const cleanup = () => {
      overlay.classList.remove('active');
      input.style.display = '';
      okBtn.replaceWith(okBtn.cloneNode(true));
      cancelBtn.replaceWith(cancelBtn.cloneNode(true));
      document.removeEventListener('keydown', onKey);
    };

    const onKey = (e) => {
      if (e.key === 'Enter') { cleanup(); resolve(true); }
      if (e.key === 'Escape') { cleanup(); resolve(false); }
    };
    document.addEventListener('keydown', onKey);

    document.getElementById('prompt-ok').addEventListener('click', () => {
      cleanup();
      resolve(true);
    });
    document.getElementById('prompt-cancel').addEventListener('click', () => {
      cleanup();
      resolve(false);
    });
  });
}

function timeAgo(isoString) {
  if (!isoString) return 'Never';
  const seconds = Math.floor((Date.now() - new Date(isoString).getTime()) / 1000);
  if (seconds < 60) return 'Just now';
  if (seconds < 3600) return Math.floor(seconds / 60) + 'm ago';
  if (seconds < 86400) return Math.floor(seconds / 3600) + 'h ago';
  if (seconds < 604800) return Math.floor(seconds / 86400) + 'd ago';
  return new Date(isoString).toLocaleDateString();
}

// ============================================================
// TOAST NOTIFICATIONS
// ============================================================
function showToast(message, type = 'info', duration = 3000) {
  const container = document.getElementById('toast-container');
  const toast = document.createElement('div');
  toast.className = `toast ${type}`;
  toast.innerHTML = `<span>${escapeHtml(message)}</span>`;
  container.appendChild(toast);

  setTimeout(() => {
    toast.classList.add('exit');
    setTimeout(() => toast.remove(), 200);
  }, duration);
}

// ============================================================
// INIT
// ============================================================
async function init() {
  config = await window.api.getConfig();
  // Backward compat
  config.binds = (config.binds || []).map(b => ({
    ...b,
    usageCount: b.usageCount || 0,
    lastUsed: b.lastUsed || null
  }));
  config.profiles = config.profiles || {};
  config.activeProfile = config.activeProfile || 'default';
  config.settings = config.settings || { notifications: true, autoBackup: true, startup: false };

  renderBinds();
  updateStatusBar();
  updateBindCount();
  initDragAndDrop();
  initNavigation();
  initKeyboardShortcuts();
}

// ============================================================
// STATUS BAR
// ============================================================
function updateStatusBar() {
  if (config.paused) {
    statusIndicator.classList.add('paused');
    statusText.textContent = 'Paused';
  } else {
    statusIndicator.classList.remove('paused');
    const activeCount = config.binds.filter(b => b.enabled).length;
    statusText.textContent = `${activeCount} active bind${activeCount !== 1 ? 's' : ''}`;
  }
}

function updateBindCount() {
  bindCountBadge.textContent = config.binds.length;
}

// ============================================================
// NAVIGATION / ROUTER
// ============================================================
function initNavigation() {
  document.querySelectorAll('.nav-item[data-page]').forEach(item => {
    item.addEventListener('click', () => navigateTo(item.dataset.page));
  });
}

function navigateTo(page) {
  // Update nav items
  document.querySelectorAll('.nav-item').forEach(item => {
    item.classList.toggle('active', item.dataset.page === page);
  });

  // Update pages
  document.querySelectorAll('.page').forEach(p => {
    p.classList.toggle('active', p.id === `page-${page}`);
  });

  // Render page content
  switch (page) {
    case 'binds': renderBinds(); break;
    case 'profiles': renderProfiles(); break;
    case 'stats': renderStats(); break;
    case 'settings': renderSettings(); break;
    case 'about': renderAbout(); break;
  }
}

// ============================================================
// RENDER BINDS
// ============================================================
function renderBinds() {
  bindsContainer.innerHTML = '';
  const searchTerm = (searchInput?.value || '').toLowerCase();

  const filteredBinds = config.binds
    .map((bind, index) => ({ bind, index }))
    .filter(({ bind }) => {
      if (!searchTerm) return true;
      return (bind.name || '').toLowerCase().includes(searchTerm) ||
             (bind.shortcut || '').toLowerCase().includes(searchTerm) ||
             (bind.target || '').toLowerCase().includes(searchTerm) ||
             (ACTION_NAMES[bind.actionType] || '').toLowerCase().includes(searchTerm);
    });

  if (config.binds.length === 0) {
    renderEmptyState();
    return;
  }

  if (filteredBinds.length === 0) {
    bindsContainer.innerHTML = `
      <div class="no-results">
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="11" cy="11" r="8"/><line x1="21" y1="21" x2="16.65" y2="16.65"/></svg>
        <span>No binds match "${escapeHtml(searchTerm)}"</span>
      </div>`;
    return;
  }

  filteredBinds.forEach(({ bind, index }) => {
    const el = document.createElement('div');
    el.className = `bind-card ${!bind.enabled ? 'disabled' : ''}`;
    el.setAttribute('data-action', bind.actionType);
    el.setAttribute('data-index', index);
    el.draggable = true;

    let targetDisplay = bind.target || '';
    if (bind.actionType === 'app') {
      const parts = bind.target.split('\\');
      targetDisplay = parts[parts.length - 1];
    }

    el.innerHTML = `
      <div class="bind-drag-handle" title="Drag to reorder">
        <svg width="10" height="10" viewBox="0 0 24 24" fill="currentColor">
          <circle cx="8" cy="6" r="2"/><circle cx="16" cy="6" r="2"/>
          <circle cx="8" cy="12" r="2"/><circle cx="16" cy="12" r="2"/>
          <circle cx="8" cy="18" r="2"/><circle cx="16" cy="18" r="2"/>
        </svg>
      </div>
      <div class="bind-type-badge">${ACTION_ICONS[bind.actionType] || ''}</div>
      <div class="bind-info">
        <div class="bind-name">${escapeHtml(bind.name || 'Unnamed')}</div>
        <div class="bind-target">${escapeHtml(ACTION_NAMES[bind.actionType] || 'Target')}: ${escapeHtml(targetDisplay)}</div>
      </div>
      <div class="bind-meta">
        <div class="kbd">${escapeHtml((bind.shortcut || '').replace(/\+/g, ' + '))}</div>
        <div class="actions">
          <label class="switch">
            <input type="checkbox" ${bind.enabled ? 'checked' : ''}>
            <span class="slider"></span>
          </label>
          <button class="btn-delete" title="Delete bind">&#10005;</button>
        </div>
      </div>`;

    // Toggle handler
    const checkbox = el.querySelector('input[type="checkbox"]');
    checkbox.addEventListener('change', (e) => {
      e.stopPropagation();
      toggleBind(index);
    });

    // Delete handler
    const deleteBtn = el.querySelector('.btn-delete');
    deleteBtn.addEventListener('click', (e) => {
      e.stopPropagation();
      deleteBind(index);
    });

    // Double-click to edit
    el.addEventListener('dblclick', (e) => {
      if (e.target.tagName !== 'INPUT' && e.target.tagName !== 'SPAN' && !e.target.classList.contains('btn-delete')) {
        openModal(index);
      }
    });

    bindsContainer.appendChild(el);
  });
}

function renderEmptyState() {
  bindsContainer.innerHTML = `
    <div class="empty-state">
      <div class="empty-state-icon">
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5">
          <rect x="2" y="4" width="20" height="16" rx="2"/>
          <path d="M6 8h.01M10 8h.01M14 8h.01M18 8h.01"/>
          <path d="M6 12h.01M10 12h.01M14 12h.01M18 12h.01"/>
          <path d="M8 16h8"/>
        </svg>
      </div>
      <h3>No binds yet</h3>
      <p>Create your first keyboard shortcut to get started. Binds let you launch apps, open URLs, and more.</p>
      <button class="btn-primary" onclick="document.getElementById('btn-add').click()">
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" width="14" height="14"><line x1="12" y1="5" x2="12" y2="19"/><line x1="5" y1="12" x2="19" y2="12"/></svg>
        Create First Bind
      </button>
    </div>`;
}

// ============================================================
// BIND ACTIONS
// ============================================================
window.toggleBind = async (index) => {
  config.binds[index].enabled = !config.binds[index].enabled;
  await window.api.saveConfig(config);
  renderBinds();
  updateStatusBar();
  showToast(config.binds[index].enabled ? 'Bind enabled' : 'Bind disabled', 'info');
};

window.deleteBind = async (index) => {
  const name = config.binds[index].name || 'Unnamed';
  config.binds.splice(index, 1);
  await window.api.saveConfig(config);
  renderBinds();
  updateStatusBar();
  updateBindCount();
  showToast(`Deleted "${name}"`, 'info');
};

// ============================================================
// SEARCH
// ============================================================
searchInput.addEventListener('input', () => renderBinds());

// ============================================================
// ADD BUTTON
// ============================================================
document.getElementById('btn-add').addEventListener('click', () => openModal(-1));

// ============================================================
// MODAL LOGIC
// ============================================================
document.getElementById('btn-cancel').addEventListener('click', () => {
  modal.classList.remove('active');
  stopCapture();
});

document.getElementById('btn-save').addEventListener('click', async () => {
  const name = inputName.value.trim() || 'Unnamed Bind';
  const shortcut = inputShortcut.value;
  const actionType = selectAction.value;
  const target = inputTarget.value.trim();

  if (!shortcut || !target) {
    showToast('Shortcut and Target are required.', 'error');
    return;
  }

  const newBind = { name, shortcut, actionType, target, enabled: true, usageCount: 0, lastUsed: null };

  if (editingIndex >= 0) {
    newBind.enabled = config.binds[editingIndex].enabled;
    newBind.usageCount = config.binds[editingIndex].usageCount || 0;
    newBind.lastUsed = config.binds[editingIndex].lastUsed || null;
    config.binds[editingIndex] = newBind;
  } else {
    config.binds.push(newBind);
  }

  await window.api.saveConfig(config);
  renderBinds();
  updateStatusBar();
  updateBindCount();
  modal.classList.remove('active');
  showToast(editingIndex >= 0 ? 'Bind updated' : 'Bind created', 'success');
});

function openModal(index) {
  editingIndex = index;
  stopCapture();

  if (index >= 0) {
    modalTitle.textContent = 'Edit Bind';
    const b = config.binds[index];
    inputName.value = b.name;
    inputShortcut.value = b.shortcut;
    selectAction.value = b.actionType;
    inputTarget.value = b.target;
  } else {
    modalTitle.textContent = 'Add New Bind';
    inputName.value = '';
    inputShortcut.value = '';
    selectAction.value = 'url';
    inputTarget.value = '';
  }

  updateTargetLabel();
  modal.classList.add('active');
}

selectAction.addEventListener('change', updateTargetLabel);

function updateTargetLabel() {
  const val = selectAction.value;
  btnBrowse.style.display = (val === 'app' || val === 'cmd') ? 'block' : 'none';
  switch (val) {
    case 'url': lblTarget.textContent = 'Target (URL)'; break;
    case 'app': lblTarget.textContent = 'Target (.exe path)'; break;
    case 'cmd': lblTarget.textContent = 'Target (Shell Command)'; break;
    case 'focus': lblTarget.textContent = 'Target (Window Title Substring)'; break;
  }
}

btnBrowse.addEventListener('click', async () => {
  const path = await window.api.showOpenDialog();
  if (path) inputTarget.value = path;
});

// ============================================================
// KEY CAPTURE
// ============================================================
let isCapturing = false;

btnCapture.addEventListener('click', () => {
  if (isCapturing) stopCapture();
  else startCapture();
});

function startCapture() {
  isCapturing = true;
  inputShortcut.value = 'Press keys... (ESC to cancel)';
  btnCapture.textContent = 'Stop';
  document.addEventListener('keydown', handleKeyDown);
}

function stopCapture() {
  isCapturing = false;
  btnCapture.textContent = 'Capture';
  document.removeEventListener('keydown', handleKeyDown);
  if (inputShortcut.value.startsWith('Press')) inputShortcut.value = '';
}

function handleKeyDown(e) {
  e.preventDefault();

  if (e.key === 'Escape') {
    stopCapture();
    return;
  }

  if (['Control', 'Shift', 'Alt', 'Meta'].includes(e.key)) return;

  const keys = [];
  if (e.ctrlKey) keys.push('CommandOrControl');
  if (e.altKey) keys.push('Alt');
  if (e.shiftKey) keys.push('Shift');
  if (e.metaKey) keys.push('Super');

  let keyStr = e.key.toUpperCase();
  if (e.code.startsWith('Key')) keyStr = e.code.replace('Key', '');
  if (e.code.startsWith('Digit')) keyStr = e.code.replace('Digit', '');

  const map = {
    ' ': 'Space',
    'ARROWUP': 'Up', 'ARROWDOWN': 'Down', 'ARROWLEFT': 'Left', 'ARROWRIGHT': 'Right',
    'ENTER': 'Enter', 'TAB': 'Tab'
  };

  if (map[keyStr]) keyStr = map[keyStr];
  keys.push(keyStr);
  inputShortcut.value = keys.join('+');
  stopCapture();
}

// ============================================================
// KEYBOARD SHORTCUTS (UI)
// ============================================================
function initKeyboardShortcuts() {
  document.addEventListener('keydown', (e) => {
    const modalActive = modal.classList.contains('active');

    // Ctrl+N: New bind
    if (e.ctrlKey && e.key === 'n' && !modalActive) {
      e.preventDefault();
      openModal(-1);
    }

    // Ctrl+F: Focus search
    if (e.ctrlKey && e.key === 'f' && !modalActive) {
      e.preventDefault();
      searchInput.focus();
    }

    // Ctrl+E: Export
    if (e.ctrlKey && e.key === 'e' && !modalActive) {
      e.preventDefault();
      document.getElementById('btn-export')?.click();
    }

    // Escape: Close modal
    if (e.key === 'Escape' && modalActive && !isCapturing) {
      modal.classList.remove('active');
    }
  });
}

// ============================================================
// DRAG AND DROP
// ============================================================
function initDragAndDrop() {
  bindsContainer.addEventListener('dragstart', (e) => {
    const card = e.target.closest('.bind-card');
    if (!card) return;
    dragSourceIndex = parseInt(card.dataset.index);
    card.classList.add('dragging');
    e.dataTransfer.effectAllowed = 'move';
  });

  bindsContainer.addEventListener('dragover', (e) => {
    e.preventDefault();
    e.dataTransfer.dropEffect = 'move';
    const card = e.target.closest('.bind-card');
    if (card) {
      document.querySelectorAll('.bind-card.drag-over').forEach(c => c.classList.remove('drag-over'));
      card.classList.add('drag-over');
    }
  });

  bindsContainer.addEventListener('dragleave', (e) => {
    const card = e.target.closest('.bind-card');
    if (card) card.classList.remove('drag-over');
  });

  bindsContainer.addEventListener('drop', async (e) => {
    e.preventDefault();
    const card = e.target.closest('.bind-card');
    if (!card || dragSourceIndex === null) return;

    const dropIndex = parseInt(card.dataset.index);
    if (dragSourceIndex !== dropIndex) {
      const [moved] = config.binds.splice(dragSourceIndex, 1);
      config.binds.splice(dropIndex, 0, moved);
      await window.api.saveConfig(config);
      renderBinds();
      showToast('Bind reordered', 'info');
    }
    dragSourceIndex = null;
  });

  bindsContainer.addEventListener('dragend', () => {
    dragSourceIndex = null;
    document.querySelectorAll('.bind-card.dragging').forEach(c => c.classList.remove('dragging'));
    document.querySelectorAll('.bind-card.drag-over').forEach(c => c.classList.remove('drag-over'));
  });
}

// ============================================================
// IMPORT / EXPORT
// ============================================================
document.getElementById('btn-export').addEventListener('click', async () => {
  const filePath = await window.api.showSaveDialog({
    defaultPath: `checkpoint-binds-${new Date().toISOString().slice(0, 10)}.json`
  });
  if (filePath) {
    const success = await window.api.exportConfig(filePath);
    showToast(success ? 'Binds exported successfully' : 'Export failed', success ? 'success' : 'error');
  }
});

document.getElementById('btn-import').addEventListener('click', async () => {
  const data = await window.api.importConfig();
  if (data) {
    const merge = await showConfirm('Merge with existing binds? (Cancel to replace all)');
    if (merge) {
      config.binds = [...config.binds, ...data.binds];
    } else {
      config.binds = data.binds;
      if (data.paused !== undefined) config.paused = data.paused;
    }
    await window.api.saveConfig(config);
    renderBinds();
    updateStatusBar();
    updateBindCount();
    showToast(`Imported ${data.binds.length} bind(s)`, 'success');
  } else if (data === null) {
    // User cancelled the dialog - do nothing
  } else {
    showToast('Invalid bind file', 'error');
  }
});

// ============================================================
// PROFILES PAGE
// ============================================================
window.switchProfile = async (profileName) => {
  config = await window.api.switchProfile(profileName);
  config.binds = (config.binds || []).map(b => ({
    ...b,
    usageCount: b.usageCount || 0,
    lastUsed: b.lastUsed || null
  }));
  renderBinds();
  renderProfiles();
  updateStatusBar();
  updateBindCount();
  showToast(`Switched to "${profileName}"`, 'success');
};

window.createProfile = async () => {
  const name = await showPrompt('New Profile', 'Profile name', 'e.g. Gaming, Work');
  if (!name || !name.trim()) return;
  await window.api.createProfile(name.trim());
  config.profiles[name.trim()] = { binds: [] };
  renderProfiles();
  showToast(`Profile "${name.trim()}" created`, 'success');
};

window.deleteProfile = async (profileName) => {
  if (!(await showConfirm(`Delete profile "${profileName}"?`))) return;
  await window.api.deleteProfile(profileName);
  delete config.profiles[profileName];
  if (config.activeProfile === profileName) {
    config.activeProfile = 'default';
    config.binds = config.profiles.default?.binds || [];
  }
  renderBinds();
  renderProfiles();
  updateStatusBar();
  updateBindCount();
  showToast(`Profile "${profileName}" deleted`, 'info');
};

function renderProfiles() {
  const container = document.getElementById('profiles-container');
  const profiles = config.profiles || {};
  const activeProfile = config.activeProfile || 'default';

  // Ensure default profile exists in display
  if (!profiles.default) {
    profiles.default = { binds: config.binds };
  }

  const grid = document.createElement('div');
  grid.className = 'profiles-grid';

  Object.entries(profiles).forEach(([name, profile]) => {
    const card = document.createElement('div');
    card.className = `profile-card ${name === activeProfile ? 'active' : ''}`;
    card.dataset.profile = name;

    card.innerHTML = `
      <div class="profile-header">
        <h3>${escapeHtml(name)}</h3>
        ${name === activeProfile ? '<span class="profile-active-badge">Active</span>' : ''}
      </div>
      <p class="profile-count">${(profile.binds || []).length} bind${(profile.binds || []).length !== 1 ? 's' : ''}</p>
      <div class="profile-actions"></div>`;

    const actionsDiv = card.querySelector('.profile-actions');

    const switchBtn = document.createElement('button');
    switchBtn.className = 'btn-secondary';
    switchBtn.textContent = name === activeProfile ? 'Active' : 'Switch';
    switchBtn.addEventListener('click', () => switchProfile(name));
    actionsDiv.appendChild(switchBtn);

    if (name !== 'default') {
      const deleteBtn = document.createElement('button');
      deleteBtn.className = 'btn-delete';
      deleteBtn.textContent = 'Delete';
      deleteBtn.addEventListener('click', () => deleteProfile(name));
      actionsDiv.appendChild(deleteBtn);
    }

    grid.appendChild(card);
  });

  // Add profile card
  const addCard = document.createElement('div');
  addCard.className = 'profile-card add-profile';
  addCard.addEventListener('click', () => createProfile());
  addCard.innerHTML = '<div class="add-profile-icon">+</div><p style="font-size:13px;">New Profile</p>';
  grid.appendChild(addCard);

  container.innerHTML = '';
  container.appendChild(grid);
}

// ============================================================
// STATISTICS PAGE
// ============================================================
function renderStats() {
  const container = document.getElementById('stats-container');
  const totalUsage = config.binds.reduce((sum, b) => sum + (b.usageCount || 0), 0);
  const mostUsed = [...config.binds].sort((a, b) => (b.usageCount || 0) - (a.usageCount || 0));

  container.innerHTML = `
    <div class="stats-overview">
      <div class="stat-card">
        <div class="stat-value">${config.binds.length}</div>
        <div class="stat-label">Total Binds</div>
      </div>
      <div class="stat-card">
        <div class="stat-value">${config.binds.filter(b => b.enabled).length}</div>
        <div class="stat-label">Active</div>
      </div>
      <div class="stat-card">
        <div class="stat-value">${totalUsage}</div>
        <div class="stat-label">Total Uses</div>
      </div>
    </div>
    ${mostUsed.length > 0 ? `
      <h3 class="section-title">Most Used</h3>
      <div class="stats-list">
        ${mostUsed.slice(0, 10).map((bind, i) => `
          <div class="stat-row">
            <span class="stat-rank">#${i + 1}</span>
            <span class="stat-name">${escapeHtml(bind.name || 'Unnamed')}</span>
            <span class="stat-uses">${bind.usageCount || 0} uses</span>
            <span class="stat-last">${timeAgo(bind.lastUsed)}</span>
          </div>
        `).join('')}
      </div>
    ` : `
      <div class="no-results">
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" width="24" height="24"><path d="M18 20V10"/><path d="M12 20V4"/><path d="M6 20v-6"/></svg>
        <span>Usage statistics will appear here once you start using your binds</span>
      </div>
    `}`;
}

// ============================================================
// SETTINGS PAGE
// ============================================================
window.createBackupNow = async () => {
  const result = await window.api.createBackup();
  showToast(result ? 'Backup created' : 'Backup failed', result ? 'success' : 'error');
};

window.restoreBackup = async () => {
  const backups = await window.api.listBackups();
  if (!backups || backups.length === 0) {
    showToast('No backups found', 'warning');
    return;
  }
  const list = backups.slice(0, 5).map((b, i) => `${i + 1}. ${b}`).join('\n');
  const choice = await showPrompt('Restore Backup', `Available backups:\n${list}`, 'Enter number (1-5)');
  const idx = parseInt(choice) - 1;
  if (isNaN(idx) || idx < 0 || idx >= backups.length) return;
  const result = await window.api.restoreBackup(backups[idx]);
  if (result) {
    config = result;
    config.binds = (config.binds || []).map(b => ({ ...b, usageCount: b.usageCount || 0, lastUsed: b.lastUsed || null }));
    renderBinds();
    updateStatusBar();
    updateBindCount();
    showToast('Backup restored', 'success');
  } else {
    showToast('Restore failed', 'error');
  }
};

window.resetAllBinds = async () => {
  if (!(await showConfirm('Delete all binds? This cannot be undone.'))) return;
  config.binds = [];
  await window.api.saveConfig(config);
  renderBinds();
  updateStatusBar();
  updateBindCount();
  showToast('All binds cleared', 'info');
};

function renderSettings() {
  const container = document.getElementById('settings-container');
  const settings = config.settings || {};

  container.innerHTML = `
    <div class="settings-section">
      <h3>General</h3>
      <div class="setting-row">
        <div class="setting-info">
          <div class="setting-label">Start with Windows</div>
          <div class="setting-desc">Launch Checkpoint when Windows starts</div>
        </div>
        <label class="switch">
          <input type="checkbox" id="setting-startup" ${settings.startup ? 'checked' : ''}>
          <span class="slider"></span>
        </label>
      </div>
      <div class="setting-row">
        <div class="setting-info">
          <div class="setting-label">Show notifications</div>
          <div class="setting-desc">Display a toast when a hotkey action fires</div>
        </div>
        <label class="switch">
          <input type="checkbox" id="setting-notifications" ${settings.notifications !== false ? 'checked' : ''}>
          <span class="slider"></span>
        </label>
      </div>
      <div class="setting-row">
        <div class="setting-info">
          <div class="setting-label">Auto-backup</div>
          <div class="setting-desc">Create a backup of your config before changes</div>
        </div>
        <label class="switch">
          <input type="checkbox" id="setting-autobackup" ${settings.autoBackup !== false ? 'checked' : ''}>
          <span class="slider"></span>
        </label>
      </div>
    </div>
    <div class="settings-section">
      <h3>Data</h3>
      <div class="setting-row setting-row-buttons">
        <button class="btn-secondary" onclick="createBackupNow()">Create Backup Now</button>
        <button class="btn-secondary" onclick="restoreBackup()">Restore from Backup</button>
      </div>
    </div>
    <div class="settings-section">
      <h3>Danger Zone</h3>
      <div class="setting-row setting-row-buttons">
        <button class="btn-danger" onclick="resetAllBinds()">Reset All Binds</button>
      </div>
    </div>`;

  // Toggle handlers
  const toggleHandler = async (key, el) => {
    config.settings = config.settings || {};
    config.settings[key] = el.checked;
    await window.api.saveConfig(config);
    showToast(`${key} ${el.checked ? 'enabled' : 'disabled'}`, 'info');
  };

  document.getElementById('setting-startup').addEventListener('change', function() { toggleHandler('startup', this); });
  document.getElementById('setting-notifications').addEventListener('change', function() { toggleHandler('notifications', this); });
  document.getElementById('setting-autobackup').addEventListener('change', function() { toggleHandler('autoBackup', this); });
}

// ============================================================
// ABOUT PAGE
// ============================================================
function renderAbout() {
  const container = document.getElementById('about-container');
  container.innerHTML = `
    <div class="about-page">
      <div class="about-logo">
        <img src="checkpoint_icon.png" alt="Checkpoint">
      </div>
      <h2>Checkpoint</h2>
      <p class="about-version">v2.0.0</p>
      <p class="about-desc">Global Hotkey Manager for Windows</p>
      <div class="about-links">
        <a href="#" onclick="window.api.openExternal('https://github.com')">GitHub</a>
        <span class="about-sep">|</span>
        <a href="#" onclick="window.api.openExternal('https://github.com/issues')">Report Issue</a>
      </div>
      <div class="about-credits">
        <p>Built with Electron + Win32 API</p>
        <p>Font: Outfit by Google Fonts</p>
      </div>
    </div>`;
}

// ============================================================
// START
// ============================================================
init();
