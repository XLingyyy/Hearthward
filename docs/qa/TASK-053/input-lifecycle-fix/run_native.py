import json
from pathlib import Path
import sys
import uuid

GAME = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(GAME / 'scripts/animals'))
from paths import configure_factory, ue_root
configure_factory()
from engine_adapters.ue5 import UEClient

out = Path(__file__).parent / ('native_' + uuid.uuid4().hex[:8])
out.mkdir()
profile = GAME / 'TestClient/Runs' / out.name
profile.mkdir(parents=True)
client = UEClient(project_path=GAME / 'Hearthward.uproject', ue_root=ue_root())
result = client.testing.run_automation_tests('Hearthward.Experience.BindingConflictsAndContexts',
    report_dir=str(out / 'automation'), extra_args=['-NullRHI', '-UserDir=' + str(profile)], timeout=240)
(out / 'result.json').write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
print(json.dumps({'ok': result['ok'], 'evidence': str(out),
                  'tests_passed': result['payload']['tests_passed'],
                  'tests_failed': result['payload']['tests_failed'], 'errors': result['errors']}, ensure_ascii=False), flush=True)
raise SystemExit(0 if result['ok'] else 1)
