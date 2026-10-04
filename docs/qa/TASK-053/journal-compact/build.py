"""Build the current editor Development target through the public UEClient API."""
import hashlib
import json
from pathlib import Path
import sys

QA = Path(__file__).resolve().parent
GAME = QA.parents[3]
sys.path.insert(0, str(GAME / 'scripts/animals'))
from paths import configure_factory, ue_root
configure_factory()
from engine_adapters.ue5 import UEClient

client = UEClient(project_path=GAME / 'Hearthward.uproject', ue_root=ue_root())
result = client.build.project(target='HearthwardEditor', configuration='Development', timeout=1200)
(QA / 'build.json').write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
if result['ok']:
    files = [p for p in (GAME / 'Source').rglob('*') if p.is_file()]
    files.append(GAME / 'Binaries/Win64/UnrealEditor-Hearthward.dll')
    fingerprints = {p.relative_to(GAME).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest() for p in files}
    (QA / 'compiled-source.json').write_text(json.dumps(fingerprints, indent=2), encoding='utf-8')
print(json.dumps(dict(ok=result['ok'], diagnostics=result['diagnostics'], errors=result['errors']), ensure_ascii=False, indent=2), flush=True)
raise SystemExit(0 if result['ok'] else 1)
