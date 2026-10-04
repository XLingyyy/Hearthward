"""Audit the actual normal root entry against the newly compiled source and persistent profile."""
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import time

QA = Path(__file__).resolve().parent
GAME = QA.parents[3]

def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

launch = read(GAME / 'TestClient/Logs/latest-launch.json')
manifest = read(GAME / 'TestClient/Logs/latest-source.json')
command = launch['payload']['command']
pid = launch['payload']['process_id']
log = Path(next(arg.split('=', 1)[1] for arg in command if arg.startswith('-abslog=')))
samples = []
deadline = time.monotonic() + 60
while time.monotonic() < deadline:
    text = log.read_text(encoding='utf-8', errors='replace') if log.exists() else ''
    process = subprocess.run(['powershell', '-NoProfile', '-Command',
        '[Console]::OutputEncoding=[System.Text.Encoding]::UTF8; Get-Process -Id ' + str(pid) + ' -ErrorAction SilentlyContinue | Select-Object Id,Responding,MainWindowTitle | ConvertTo-Json -Compress'],
        capture_output=True, text=True, encoding='utf-8', errors='replace')
    if process.stdout.strip() and 'Hearthward loading end' in text:
        sample = json.loads(process.stdout)
        samples.append(sample)
    if len(samples) >= 3:
        break
    time.sleep(3)

compiled = read(QA / 'compiled-source.json')
paths = lambda name: GAME.parent / name.removeprefix('parent/') if name.startswith('parent/') else GAME / name
previous = read(QA / 'client-before.json')
state = read(GAME / 'TestClient/client.json')
profile_before = read(QA / 'profile-before.json')
protected = {n: d for n, d in profile_before.items() if '/Config/' in n or '/SaveGames/' in n}
checks = dict(root_launcher_success=launch['ok'],
    normal_visible_game='-game' in command and '-RenderOffscreen' not in command and '-unattended' not in command,
    no_automatic_preview_flags=not any('HearthwardMapPreview' in a or 'HearthwardHUDPreview' in a or 'HearthwardInputVerify' in a for a in command),
    loading_complete='Hearthward loading end' in text,
    persistent_profile_same=state['profile']==previous['profile'],
    persistent_save_pool_same=state['save_pool']==previous['save_pool'],
    all_compiled_inputs_current=all(sha(GAME / n)==d==manifest['fingerprints'].get(n) for n,d in compiled.items()),
    all_282_launch_fingerprints_current=len(manifest['fingerprints'])==282 and all(sha(paths(n))==d for n,d in manifest['fingerprints'].items()),
    original_player_settings_and_save_files_same=all((GAME / n).exists() and sha(GAME / n)==d for n,d in protected.items()),
    no_fatal_error='Fatal error' not in text and 'Unhandled Exception' not in text,
    three_responsive_samples=len(samples)==3 and all(s['Responding'] and 'Development' in s['MainWindowTitle'] for s in samples))
shutil.copy2(GAME / 'TestClient/Logs/latest-launch.json', QA / 'root-launch.json')
shutil.copy2(GAME / 'TestClient/Logs/latest-source.json', QA / 'root-source.json')
shutil.copy2(log, QA / 'root-startup.log')
result = dict(passed=all(checks.values()), checks=checks, process_id=pid, samples=samples,
    original_protected_profile_files=len(protected), launch_fingerprints=len(manifest['fingerprints']),
    compiled_inputs=len(compiled), root_entry=str(GAME.parent / '启动测试版游戏.cmd'),
    preview_left_open=True, scope='Actual normal title entry, existing player profile. Automated input was checked separately; no physical Windows playthrough claimed.')
(QA / 'root-smoke.json').write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
print(json.dumps(result, ensure_ascii=False, indent=2))
raise SystemExit(0 if result['passed'] else 1)
