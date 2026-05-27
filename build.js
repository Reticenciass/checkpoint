const { execSync } = require('child_process');
const fs = require('fs');
const path = require('path');
const os = require('os');

const PROJ = __dirname;
const DIST = path.join(PROJ, 'dist', 'Checkpoint-win32-x64');
const STAGING = path.join(os.tmpdir(), 'checkpoint-app');

// Electron version from package.json
const pkg = JSON.parse(fs.readFileSync(path.join(PROJ, 'package.json'), 'utf8'));
const electronVersion = pkg.devDependencies.electron.replace(/[^0-9.]/g, '');

// Find cached electron zip
const electronCache = path.join(os.homedir(), 'AppData', 'Local', 'electron', 'Cache');
const zipName = `electron-v${electronVersion}-win32-x64.zip`;

let zipPath = path.join(electronCache, zipName);
if (!fs.existsSync(zipPath)) {
  const dirs = fs.readdirSync(electronCache);
  for (const dir of dirs) {
    const candidate = path.join(electronCache, dir, zipName);
    if (fs.existsSync(candidate)) { zipPath = candidate; break; }
  }
}

if (!fs.existsSync(zipPath)) {
  console.error('Electron cache not found. Run "npm start" first to download Electron.');
  process.exit(1);
}

console.log('Cleaning dist...');
fs.rmSync(path.join(PROJ, 'dist'), { recursive: true, force: true });
fs.mkdirSync(DIST, { recursive: true });

console.log('Extracting Electron...');
const extractCmd = process.platform === 'win32'
  ? `powershell -Command "Expand-Archive -Path '${zipPath}' -DestinationPath '${DIST}' -Force"`
  : `unzip -o "${zipPath}" -d "${DIST}"`;
execSync(extractCmd, { stdio: 'inherit' });

console.log('Renaming electron.exe -> Checkpoint.exe...');
fs.renameSync(path.join(DIST, 'electron.exe'), path.join(DIST, 'Checkpoint.exe'));

console.log('Packing app.asar...');
fs.rmSync(STAGING, { recursive: true, force: true });
fs.mkdirSync(STAGING, { recursive: true });

const appFiles = ['main.js', 'preload.js', 'renderer.js', 'index.html', 'style.css', 'checkpoint_icon.png', 'package.json'];
for (const file of appFiles) {
  fs.copyFileSync(path.join(PROJ, file), path.join(STAGING, file));
}

const resourcesDir = path.join(DIST, 'resources');
fs.mkdirSync(resourcesDir, { recursive: true });

execSync(`npx asar pack "${STAGING}" "${path.join(resourcesDir, 'app.asar')}"`, { stdio: 'inherit' });

fs.copyFileSync(path.join(PROJ, 'checkpoint_icon.png'), path.join(resourcesDir, 'checkpoint_icon.png'));

console.log('Cleaning up...');
fs.rmSync(STAGING, { recursive: true, force: true });

const size = (fs.statSync(path.join(DIST, 'Checkpoint.exe')).size / 1024 / 1024).toFixed(0);
console.log(`\nDone! dist/Checkpoint-win32-x64/Checkpoint.exe (${size} MB)`);
