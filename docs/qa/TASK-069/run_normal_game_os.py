"""Root-only bounded uncooked game launch; no world fixture or UI automation."""
import argparse
import json
import sys
import time
import uuid
from pathlib import Path

sys.path.insert(0, 'G:/GameFactory')
from engine_adapters.ue5 import UEClient

parser = argparse.ArgumentParser()
parser.add_argument('--pool', type=uuid.UUID, default=uuid.uuid4())
parser.add_argument('--timeout', type=int, default=300)
args = parser.parse_args()
if not 0 < args.timeout <= 600:
    parser.error('timeout must be 1..600 seconds')
root = Path(__file__).resolve().parents[3]
out = root/'Saved/Task069'/('normal-'+str(args.pool)+'-'+str(uuid.uuid4()))
out.mkdir(parents=True, exist_ok=False)
ue = UEClient(project_path=root/'Hearthward.uproject', ue_root='G:/UnrealEngine/UE_5.8')
status = {'pool': str(args.pool), 'output': str(out), 'method': 'Uncooked Editor -game; actual Root OS input',
          'fixture': False, 'owner_acceptance': 'NOT_EVALUATED', 'shipping_credit': False,
          'host_result': 'LAUNCH_PENDING', 'finish_marker': str(out/'finish.json')}
pid = None
try:
    launch = ue.runtime.launch_editor(map_path='/Game/Hearthward/Bootstrap/L_Bootstrap', extra_args=[
        '-game', '-windowed', '-ResX=1280', '-ResY=720', '-NoSound', '-culture=en',
        '-HearthwardSaveTestPool='+str(args.pool), '-abslog='+str(out/'normal-game.log')])
    (out/'launch.json').write_text(json.dumps(launch, ensure_ascii=False, indent=2), encoding='utf-8')
    pid = launch.get('payload', {}).get('process_id')
    status.update(process_id=pid, host_result='WAITING_FOR_OS_INPUT' if launch['ok'] else 'LAUNCH_FAILED')
    (out/'progress.json').write_text(json.dumps(status, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps(status, ensure_ascii=False), flush=True)
    if launch['ok'] and pid:
        deadline = time.monotonic()+args.timeout
        status['host_result'] = 'TIMEOUT'
        while time.monotonic() < deadline:
            marker = out/'finish.json'
            if marker.exists():
                status.update(root_observation=json.loads(marker.read_text(encoding='utf-8-sig')), host_result='ROOT_FINISHED')
                break
            time.sleep(1)
except Exception as error:
    status.update(host_result='HOST_ERROR', error=repr(error))
finally:
    if pid:
        stop = ue.runtime.stop_editor(pid)
        (out/'stop.json').write_text(json.dumps(stop, ensure_ascii=False, indent=2), encoding='utf-8')
        status['stop_ok'] = bool(stop['ok'])
    (out/'results.json').write_text(json.dumps(status, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps(status, ensure_ascii=False), flush=True)
raise SystemExit(0 if status['host_result'] == 'ROOT_FINISHED' and status.get('stop_ok') else 1)
