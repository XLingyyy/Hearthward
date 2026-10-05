"""Launch the current game from its title using a persistent, project-local test profile."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time
import uuid

GAME = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(GAME / 'scripts/animals'))
from paths import configure_factory, ue_root
configure_factory()
from engine_adapters.ue5 import UEClient


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fresh-profile', action='store_true', help='Use a new disposable run; keep the regular test profile')
    parser.add_argument('--verify-input', type=Path, help=argparse.SUPPRESS)
    args = parser.parse_args()
    verifying = args.verify_input is not None
    if verifying:
        args.verify_input = args.verify_input.resolve()
        if args.verify_input.exists():
            parser.error('Verification report already exists; choose a new output directory')
    root = GAME / 'TestClient'
    root.mkdir(exist_ok=True)
    state_path = root / 'client.json'
    state = json.loads(state_path.read_text(encoding='utf-8')) if state_path.is_file() else {}
    if not verifying and state.get('process_id'):
        pid = int(state['process_id'])
        probe = subprocess.run(['powershell.exe', '-NoProfile', '-Command',
            f"$p = Get-CimInstance Win32_Process -Filter 'ProcessId = {pid}'; "
            "[bool]($p -and $p.Name -eq 'UnrealEditor.exe' -and $p.CommandLine -like '*Hearthward.uproject*' -and $p.CommandLine -like '*TestClient*')"],
            capture_output=True, text=True)
        if probe.stdout.strip() == 'True':
            print('项目测试端已在运行，请使用该游戏窗口；关闭后可重新启动。', flush=True)
            return
    pool = state.setdefault('save_pool', str(uuid.uuid4()))
    profile = root / 'Profile'
    if args.fresh_profile or verifying:
        pool = str(uuid.uuid4())
        profile = root / 'Runs' / pool
    profile.mkdir(parents=True, exist_ok=True)
    logs = root / 'Logs'
    logs.mkdir(exist_ok=True)
    if not (GAME / 'Binaries/Win64/UnrealEditor-Hearthward.dll').is_file():
        raise FileNotFoundError('请先构建 HearthwardEditor Development。')
    output = args.verify_input.resolve().parent if verifying else logs
    output.mkdir(parents=True,exist_ok=True)
    sources = [p for directory in ['Source','Resources/UI'] for p in (GAME/directory).rglob('*') if p.is_file()]
    sources += [GAME/name for name in ['Hearthward.uproject','Resources/Data/gameplay.json',
        'Binaries/Win64/UnrealEditor-Hearthward.dll','scripts/ui/launch_test_client.py',
        'TestClient/启动测试端.cmd','启动测试版游戏.cmd']]
    parent_entry = GAME.parent/'启动测试版游戏.cmd'
    if parent_entry.is_file():
        sources.append(parent_entry)
    fingerprints = {(p.relative_to(GAME).as_posix() if p.is_relative_to(GAME) else 'parent/'+p.name):
        hashlib.sha256(p.read_bytes()).hexdigest() for p in sources}
    metadata = dict(head=subprocess.check_output(['git','rev-parse','HEAD'],cwd=GAME,text=True).strip(),
        fingerprints=fingerprints,profile=str(profile),save_pool=pool,
        mode='isolated normal-client input verification' if verifying else 'normal persistent project game')
    (output/('source-manifest.json' if verifying else 'latest-source.json')).write_text(json.dumps(metadata,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    client = UEClient(project_path=GAME / 'Hearthward.uproject', ue_root=ue_root(), port=30151, runtime_port=30152)
    extra = [
        '-game', '-NewConsole', '-CoreLimit=4', '-windowed', '-ResX=1600', '-ResY=1000', '-WinX=80', '-WinY=80',
        '-UserDir=' + str(profile), '-HearthwardSaveTestPool=' + pool,
        '-abslog=' + str(output/('runtime.log' if verifying else 'game-'+uuid.uuid4().hex[:8]+'.log'))]
    if verifying:
        extra += ['-RenderOffscreen','-unattended','-nosound','-HearthwardInputVerify='+str(args.verify_input.resolve())]
    preferences = {p:p.read_bytes() for p in (GAME/'Saved/Config').glob('*/GameUserSettings.ini')} if verifying else {}
    launch = client.runtime.launch_editor(map_path='/Game/Hearthward/Bootstrap/L_Bootstrap', extra_args=extra)
    (output/('launch.json' if verifying else 'latest-launch.json')).write_text(json.dumps(launch, ensure_ascii=False, indent=2), encoding='utf-8')
    if not launch['ok']:
        print(json.dumps(launch, ensure_ascii=False, indent=2), flush=True)
        raise SystemExit(1)
    if verifying:
        print(json.dumps(dict(launch_ok=True,evidence=str(output))),flush=True)
        try:
            deadline = time.monotonic()+300
            while not args.verify_input.is_file() and time.monotonic()<deadline:
                time.sleep(1)
            if not args.verify_input.is_file():
                raise TimeoutError('Normal test client verification did not finish; see '+str(output/'runtime.log'))
            report = json.loads(args.verify_input.read_text(encoding='utf-8'))
            print(json.dumps(dict(passed=report['passed'],checks=len(report['checks']),
                failed=[key for key,value in report['checks'].items() if not value],evidence=str(output))),flush=True)
            raise SystemExit(0 if report['passed'] else 1)
        finally:
            (output/'stop.json').write_text(json.dumps(client.runtime.stop_editor(launch['payload']['process_id']),indent=2),encoding='utf-8')
            for path in set(preferences)|set((GAME/'Saved/Config').glob('*/GameUserSettings.ini')):
                if path in preferences:
                    path.write_bytes(preferences[path])
                else:
                    path.unlink()
    state['process_id'] = int(launch['payload']['process_id'])
    state['profile'] = str(profile)
    state_path.write_text(json.dumps(state, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print('归火项目测试端已启动，从登录主界面进入。存档、设置及日志均位于项目 TestClient 内。', flush=True)
    print('Tab 背包 / K 技能 / J 日志 / M 地图 / B 建造 / T 交流 / F6 存读档 / Esc 或 P 暂停。', flush=True)


if __name__ == '__main__':
    main()
