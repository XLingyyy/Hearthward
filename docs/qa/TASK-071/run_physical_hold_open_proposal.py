"""Root-only bounded UEClient host. Exit 0 means captured/closed, not product PASS."""
import argparse
import json
import re
import sys
import time
import uuid
from pathlib import Path

sys.path.insert(0, 'G:/GameFactory')
from engine_adapters.ue5 import UEClient

parser = argparse.ArgumentParser()
parser.add_argument('--project', type=Path, default=Path('G:/GameFactory/Hearthward/.agent-local/task051/Hearthward.uproject'))
parser.add_argument('--label', default='root-os-keyboard-mouse')
parser.add_argument('--pool', type=uuid.UUID, default=uuid.uuid4())
parser.add_argument('--timeout', type=int, default=900)
args = parser.parse_args()
if not re.fullmatch(r'[A-Za-z0-9_-]+', args.label) or not 0 < args.timeout <= 900:
    parser.error('label permits letters/digits/_/-; timeout must be 1..900 seconds')
project = args.project.resolve()
out = project.parent/'Saved/Task071'/('physical-'+str(args.pool))
out.mkdir(parents=True, exist_ok=False)
ue = UEClient(project_path=str(project), ue_root='G:/UnrealEngine/UE_5.8')
status = {'pool': str(args.pool), 'label': args.label, 'out': str(out),
          'finish_marker': str(out/'finish.json'), 'acceptance': 'NOT_EVALUATED',
          'timeout_seconds': args.timeout, 'host_result': 'LAUNCH_PENDING', 'recording_complete': False}
pid = None
stopped = {'ok': False, 'reason': 'No launched PID available'}


def write(name, value):
    (out/name).write_text(json.dumps(value, ensure_ascii=False, indent=2), encoding='utf-8')


try:
    deadline = time.monotonic()+args.timeout
    launch = ue.runtime.launch_editor(map_path='/Game/Hearthward/Tests/Graybox/L_GrayboxValidation', extra_args=[
        '-ExecutePythonScript='+str(Path(__file__).with_name('physical_hold_open_monitor_proposal.py').resolve()),
        '-HearthwardSaveTestPool='+str(args.pool), '-Task071PhysicalLabel='+args.label,
        '-NoSound', '-NoSplash', '-culture=en'])
    pid = launch.get('payload', {}).get('process_id')
    status.update(process_id=pid, host_result='WAITING_FOR_ROOT_INPUT' if launch['ok'] and pid else 'LAUNCH_FAILED')
    write('launch.json', launch)
    write('host-progress.json', status)
    print(json.dumps(status, ensure_ascii=False), flush=True)
    if launch['ok'] and pid:
        status['host_result'] = 'TIMEOUT'
        while time.monotonic() < deadline:
            result = out/'results.json'
            if result.exists():
                try:
                    result_report = json.loads(result.read_text(encoding='utf-8-sig'))
                except json.JSONDecodeError:
                    time.sleep(.1)
                    continue
                status.update(recording_complete=bool(result_report.get('recording_complete')),
                              host_result='CAPTURED' if result_report.get('recording_complete') and not result_report.get('error') else 'MONITOR_ERROR',
                              monitor_stage=result_report.get('stage'), monitor_error=result_report.get('error'),
                              model_calls_observed_nonzero=result_report.get('model_calls_observed_nonzero'),
                              server_process_observed_nonzero=result_report.get('server_process_observed_nonzero'))
                break
            time.sleep(1)
except Exception as error:
    status.update(host_result='HOST_ERROR', host_error=repr(error))
finally:
    if pid:
        try:
            stopped = ue.runtime.stop_editor(pid)
        except Exception as error:
            stopped = {'ok': False, 'error': repr(error), 'process_id': pid}
    write('stop.json', stopped)
    status['stop_ok'] = bool(stopped.get('ok'))
    write('host-result.json', status)
    print(json.dumps(status, ensure_ascii=False), flush=True)
raise SystemExit(0 if status['host_result'] == 'CAPTURED' and status['stop_ok'] else 1)
