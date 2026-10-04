"""Run the native inventory, save, survival, combat and gameplay regressions."""
import hashlib
import json
from pathlib import Path
import subprocess
import uuid

from verify_quick_client import GAME, UEClient, ue_root

out = GAME / 'docs/qa/TASK-053/quick-items' / ('native_' + uuid.uuid4().hex[:8])
out.mkdir(parents=True)
sources = [p for p in (GAME / 'Source').rglob('*') if p.is_file()]
sources += [GAME / 'Binaries/Win64/UnrealEditor-Hearthward.dll', Path(__file__)]
(out / 'source-manifest.json').write_text(json.dumps(dict(
    head=subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=GAME, text=True).strip(),
    fingerprints={p.relative_to(GAME).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest() for p in sources}
), indent=2), encoding='utf-8')
client = UEClient(project_path=GAME / 'Hearthward.uproject', ue_root=ue_root())
summary = {}
for group in ['Survival', 'Gameplay', 'Combat', 'Save', 'Inventory']:
    profile = GAME / 'TestClient/Runs' / out.name / group
    profile.mkdir(parents=True)
    result = client.testing.run_automation_tests(
        'Hearthward.' + group, report_dir=str(out / group),
        extra_args=['-NullRHI', '-CoreLimit=4', '-UserDir=' + str(profile),
                    '-HearthwardSaveTestPool=' + str(uuid.uuid4())], timeout=300)
    (out / (group + '-result.json')).write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
    index = out / group / 'index.json'
    report = json.loads(index.read_text(encoding='utf-8-sig')) if index.is_file() else {}
    summary[group] = dict(ok=result['ok'], passed=report.get('succeeded', 0), failed=report.get('failed', 0),
                          warnings=report.get('succeededWithWarnings', 0), report_file=str(index))
    (out / 'summary.json').write_text(json.dumps(summary, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps(dict(group=group, **summary[group], evidence=str(out)), ensure_ascii=False), flush=True)
raise SystemExit(0 if all(s['ok'] and s['passed'] > 0 and s['failed'] == 0 for s in summary.values()) else 1)
