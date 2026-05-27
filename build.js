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
const appVersion = pkg.version;

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

console.log('Cleaning up staging...');
fs.rmSync(STAGING, { recursive: true, force: true });

const exeSize = (fs.statSync(path.join(DIST, 'Checkpoint.exe')).size / 1024 / 1024).toFixed(0);
console.log(`  Unpacked: dist/Checkpoint-win32-x64/Checkpoint.exe (${exeSize} MB)`);

// Create self-extracting portable .exe
const sevenZip = 'C:\\Program Files\\7-Zip\\7z.exe';
const sfxModule = 'C:\\Program Files\\7-Zip\\7z.sfx';

if (fs.existsSync(sevenZip) && fs.existsSync(sfxModule)) {
  console.log('Creating portable .exe...');

  const archivePath = path.join(PROJ, 'dist', 'Checkpoint.7z');
  const outputExe = path.join(PROJ, 'dist', `Checkpoint-${appVersion}-portable.exe`);

  // Create 7z archive
  execSync(`"${sevenZip}" a -mx5 "${archivePath}" "${DIST}\\*"`, { stdio: 'inherit' });

  // SFX config
  const sfxConfig = Buffer.from(
    `;!@Install@!UTF-8!\r\nTitle="Checkpoint v${appVersion}"\r\nRunProgram="Checkpoint-win32-x64\\Checkpoint.exe"\r\nGUIFlags="8+32"\r\n;!@InstallEnd@!\r\n`
  );

  // Combine SFX + config + archive
  const sfx = fs.readFileSync(sfxModule);
  const archive = fs.readFileSync(archivePath);
  fs.writeFileSync(outputExe, Buffer.concat([sfx, sfxConfig, archive]));

  // Clean up 7z archive
  fs.unlinkSync(archivePath);

  const portableSize = (fs.statSync(outputExe).size / 1024 / 1024).toFixed(0);
  console.log(`  Portable: dist/Checkpoint-${appVersion}-portable.exe (${portableSize} MB)`);
} else {
  console.log('7-Zip not found, skipping portable .exe (install 7-Zip to enable)');
}

console.log('\nDone!');
