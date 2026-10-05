"""Root-only public editor runner for the real held-axe pose evidence."""
import argparse
import json
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[6]))
from engine_adapters.ue5 import UEClient

root = Path(__file__).resolve().parents[3]
qa = root / 'docs/qa/TASK-055'
parser = argparse.ArgumentParser()
mode = parser.add_mutually_exclusive_group()
mode.add_argument('--grip-candidate', action='store_true')
mode.add_argument('--palm-candidate', action='store_true')
mode.add_argument('--handle-topology', action='store_true')
mode.add_argument('--local-section-candidate', action='store_true')
mode.add_argument('--socket-action', choices=('author', 'probe'))
args = parser.parse_args()
label = 'stone-axe-grip-candidate' if args.grip_candidate else 'stone-axe-pose'
folder = 'stone-axe-grip-candidate' if args.grip_candidate else 'stone-axe-single-node'
output = root / 'Saved/Task055' / folder / 'pose-and-axe-facts.json'
script = 'sample_stone_axe_hero_pose.py'
extra = ['-Task055GripCandidate'] if args.grip_candidate else []
if args.palm_candidate:
    label = folder = 'stone-axe-palm-candidate'
    output = root / 'Saved/Task055' / folder / 'pose-and-axe-facts.json'
    extra = ['-Task055PalmCandidate']
if args.handle_topology:
    label = folder = 'stone-axe-handle-topology'
    output = root / 'Saved/Task055' / folder / 'pose-and-axe-facts.json'
    extra = ['-Task055HandleTopology']
if args.local_section_candidate:
    label = folder = 'stone-axe-local-section-candidate'
    output = root / 'Saved/Task055' / folder / 'pose-and-axe-facts.json'
    extra = ['-Task055LocalSectionCandidate']
if args.socket_action:
    label = 'stone-axe-sockets-' + args.socket_action
    output = root / 'Saved/Task055' / (label + '.json')
    script = 'author_stone_axe_sockets.py'
    extra = ['-Task055SocketAction=' + args.socket_action]
ue = UEClient(project_path=str(root / 'Hearthward.uproject'), ue_root='G:/UnrealEngine/UE_5.8')
started = time.time()
launch = ue.runtime.launch_editor(
    map_path='/Game/Hearthward/Tests/Graybox/L_GrayboxValidation',
    extra_args=['-ExecutePythonScript=' + str(qa / script),
                '-NoSound', '-NoSplash', '-culture=en', '-Unattended', '-RenderOffscreen']
                + extra)
(qa / (label + '-launch.json')).write_text(json.dumps(launch, ensure_ascii=False, indent=2), encoding='utf-8')
print(json.dumps({'launch_ok': launch['ok']}), flush=True)
if not launch['ok']:
    raise SystemExit(1)
complete = False
try:
    deadline = time.monotonic() + 360
    while time.monotonic() < deadline:
        if output.exists() and output.stat().st_mtime >= started:
            report = json.loads(output.read_text(encoding='utf-8-sig'))
            (qa / (label + '-results.json')).write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
            complete = report.get('sampling_complete', report.get('ok', False)) and not report.get('error')
            print(json.dumps({'complete': complete, 'error': report.get('error')}, ensure_ascii=False), flush=True)
            break
        time.sleep(1)
    else:
        print(json.dumps({'complete': False, 'error': 'Pose evidence timed out'}), flush=True)
finally:
    closed = ue.runtime.stop_editor(launch['payload']['process_id'])
    (qa / (label + '-stop.json')).write_text(json.dumps(closed, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps({'closed': closed['ok']}), flush=True)
raise SystemExit(0 if complete and closed['ok'] else 1)
