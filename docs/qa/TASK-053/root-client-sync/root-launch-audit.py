"""Verify the root's normal persistent game startup without driving the player."""
import hashlib
import json
from pathlib import Path
import subprocess
import time

QA = Path(__file__).resolve().parent
GAME = QA.parents[3]

def read(path):
    return json.loads(path.read_text(encoding='utf-8'))

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def query(command):
    result = subprocess.run(['powershell','-NoProfile','-Command',command],capture_output=True,text=True,encoding='utf-8')
    if result.returncode:
        raise RuntimeError(result.stderr)
    return json.loads(result.stdout) if result.stdout.strip() else None

launch = read(GAME/'TestClient/Logs/latest-launch.json')
metadata = read(GAME/'TestClient/Logs/latest-source.json')
pid = int(launch['payload']['process_id'])
command = launch['payload']['command']
log_path = Path(next(value.removeprefix('-abslog=') for value in command if value.startswith('-abslog=')))
loaded = 'LogGlobalStatus: UEngine::LoadMap Load map complete /Game/Hearthward/Bootstrap/L_Bootstrap'
deadline = time.monotonic()+60
while time.monotonic()<deadline:
    log = log_path.read_text(encoding='utf-8',errors='replace') if log_path.is_file() else ''
    if loaded in log and 'Hearthward loading end' in log.split(loaded)[-1]:
        break
    time.sleep(1)
identity = query(f"[Console]::OutputEncoding=[System.Text.Encoding]::UTF8; Get-CimInstance Win32_Process -Filter 'ProcessId={pid}' | Select-Object ProcessId,ExecutablePath,CommandLine | ConvertTo-Json -Compress")
samples = []
if identity:
    for index in range(3):
        samples.append(query(f"[Console]::OutputEncoding=[System.Text.Encoding]::UTF8; Get-Process -Id {pid} | Select-Object Id,Responding,MainWindowTitle | ConvertTo-Json -Compress"))
        if index<2:
            time.sleep(2)
before = read(QA/'client-state-before.json')
state = read(GAME/'TestClient/client.json')
baseline = read(QA/'baseline-source.json')['fingerprints']
runtime_inputs = {name:digest for name,digest in baseline.items() if not name.startswith('scripts/ui/')}
checks = dict(root_launcher_success=launch['ok'],
    normal_visible_game='-game' in command and '-RenderOffscreen' not in command,
    bootstrap_loaded_and_loading_finished=loaded in log and 'Hearthward loading end' in log.split(loaded)[-1],
    no_preview_or_verification_flags=not any(value.startswith(('-HearthwardHUDPreview','-HearthwardMapPreview','-HearthwardInputVerify','-HearthwardMapVerify')) for value in command),
    persistent_profile_preserved=Path(metadata['profile'])==(GAME/'TestClient/Profile') and Path(state['profile'])==Path(before['profile']),
    persistent_save_pool_preserved=metadata['save_pool']==before['save_pool']==state['save_pool'],
    newest_237_runtime_inputs_match=all(metadata['fingerprints'].get(name)==digest for name,digest in runtime_inputs.items()),
    all_launch_fingerprints_current=all(sha(GAME.parent/name.removeprefix('parent/') if name.startswith('parent/') else GAME/name)==digest for name,digest in metadata['fingerprints'].items()),
    no_fatal_error='Fatal error!' not in log)
if identity:
    checks['three_responsive_development_samples']=len(samples)==3 and all(sample and sample['Responding'] and 'Development' in sample['MainWindowTitle'] for sample in samples)
else:
    checks['orderly_game_exit']='UGameEngine::Tick.ViewportClosed' in log and 'LogExit: Exiting.' in log
report = dict(passed=all(checks.values()),checks=checks,process_id=pid,samples=samples,
    entry='E:/AiAgent/XLingGame/启动测试版游戏.cmd',startup_log=str(log_path),
    fingerprints=len(metadata['fingerprints']),baseline_runtime_inputs=len(runtime_inputs),
    preview_left_open=bool(identity),scope='Actual normal root game entry, existing profile and save pool. No automatic new game or map-preview flags; no physical Windows playthrough claimed.')
(QA/'root-launch.json').write_text(json.dumps(launch,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
(QA/'root-source.json').write_text(json.dumps(metadata,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
(QA/'root-startup.log').write_text(log,encoding='utf-8')
(QA/'root-launch-smoke.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(report,ensure_ascii=False,indent=2))
raise SystemExit(0 if report['passed'] else 1)
