"""Smoke-check loading release for a map with the animal demo's separate HUD."""
import hashlib
import json
from pathlib import Path
import sys
import time
import uuid

GAME = Path(__file__).resolve().parents[4]
OUT = Path(__file__).parent / ('animal_compat_' + uuid.uuid4().hex[:8])
OUT.mkdir(parents=True)
sys.path.insert(0, str(GAME / 'scripts/animals'))
from paths import configure_factory, ue_root
configure_factory()
from engine_adapters.ue5 import UEClient

client = UEClient(project_path=GAME / 'Hearthward.uproject', ue_root=ue_root(), port=30161, runtime_port=30162)
profile = GAME / 'TestClient/Runs' / OUT.name
profile.mkdir(parents=True)
log = OUT / 'runtime.log'
launch = client.runtime.launch_editor(
    map_path='/Game/Imported/Scenes/animal_demo_20261001/Map/animal_demo_20261001?game=/Script/Hearthward.HearthwardAnimalDemoGameMode',
    extra_args=['-game', '-RenderOffscreen', '-unattended', '-nosound', '-CoreLimit=4', '-windowed',
                '-ResX=1280', '-ResY=720', '-UserDir=' + str(profile),
                '-HearthwardSaveTestPool=' + str(uuid.uuid4()), '-abslog=' + str(log)])
(OUT / 'launch.json').write_text(json.dumps(launch, indent=2), encoding='utf-8')
print(json.dumps(dict(launch_ok=launch['ok'], evidence=str(OUT))), flush=True)
if not launch['ok']:
    raise SystemExit(1)
try:
    deadline = time.monotonic() + 120
    released = False
    while time.monotonic() < deadline:
        if log.is_file():
            contents = log.read_text(encoding='utf-8', errors='replace')
            released = 'Hearthward loading end' in contents
            if released:
                break
        time.sleep(1)
    result = dict(passed=released, loading_released=released, profile=str(profile),
                  scope='Animal demo map startup and loading release only; no animal behavior assertions',
                  compiled_dll_sha256=hashlib.sha256((GAME / 'Binaries/Win64/UnrealEditor-Hearthward.dll').read_bytes()).hexdigest(),
                  loading_source_sha256=hashlib.sha256((GAME / 'Source/Hearthward/UI/HearthwardLoadingSubsystem.cpp').read_bytes()).hexdigest())
    (OUT / 'result.json').write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps(dict(passed=released, evidence=str(OUT))), flush=True)
    raise SystemExit(0 if released else 1)
finally:
    stopped = client.runtime.stop_editor(launch['payload']['process_id'])
    (OUT / 'stop.json').write_text(json.dumps(stopped, indent=2), encoding='utf-8')
