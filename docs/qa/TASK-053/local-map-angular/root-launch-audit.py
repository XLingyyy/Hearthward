"""Read-only observations of the visible game started through the root CMD."""
import argparse
import json
from pathlib import Path
import subprocess
import time

QA = Path(__file__).resolve().parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('run', type=Path)
RUN = parser.parse_args().run.resolve()
launch = json.loads((RUN / 'launch.json').read_text(encoding='utf-8'))
pid = int(launch['payload']['process_id'])

def query(command):
    result = subprocess.run(['powershell', '-NoProfile', '-Command', command], capture_output=True, text=True, encoding='utf-8')
    if result.returncode:
        raise RuntimeError(result.stderr)
    return json.loads(result.stdout) if result.stdout.strip() else None

identity = query(f"[Console]::OutputEncoding=[System.Text.Encoding]::UTF8; Get-CimInstance Win32_Process -Filter 'ProcessId={pid}' | Select-Object ProcessId,ExecutablePath,CommandLine | ConvertTo-Json -Compress")
samples = []
if identity:
    for index in range(3):
        samples.append(query(f"[Console]::OutputEncoding=[System.Text.Encoding]::UTF8; Get-Process -Id {pid} | Select-Object Id,Responding,MainWindowTitle | ConvertTo-Json -Compress"))
        if index < 2:
            time.sleep(2)
command = identity['CommandLine'] if identity else ' '.join(launch['payload']['command'])
log = (RUN / 'runtime.log').read_text(encoding='utf-8',errors='replace')
world_loaded = 'LogGlobalStatus: UEngine::LoadMap Load map complete /Game/Hearthward/World/Natural/Rebuild/L_HearthwardWilds'
checks = dict(root_launcher_success=launch['ok'], visible_game_requested='-RenderOffscreen' not in command,
    own_project_and_run_command='Hearthward.uproject' in command and str(RUN.name) in command,
    map_preview_parameter='-HearthwardMapPreview' in command,
    normal_world_and_loading_complete=world_loaded in log and 'Hearthward loading end' in log.split(world_loaded)[-1],
    no_fatal_error='Fatal error!' not in log)
if identity:
    checks['three_responsive_samples'] = len(samples)==3 and all(sample and sample['Responding'] for sample in samples)
    checks['development_game_window'] = len(samples)==3 and all(sample and 'Development' in sample['MainWindowTitle'] for sample in samples)
else:
    checks['orderly_game_exit'] = 'UGameEngine::Tick.ViewportClosed' in log and 'LogExit: Exiting.' in log
report = dict(passed=all(checks.values()),checks=checks,process_id=pid,samples=samples,
    run=RUN.name,entry='E:/AiAgent/XLingGame/地图测试版.cmd',preview_left_open=bool(identity),
    responsiveness_observation='three samples' if identity else 'NOT_RUN: window closed before observations; log records normal viewport exit after loading',
    scope='Actual root CMD startup in a visible Development game. No Windows physical-keyboard playthrough claimed.')
(QA / 'root-launch-smoke.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(report,ensure_ascii=False,indent=2))
raise SystemExit(0 if report['passed'] else 1)
