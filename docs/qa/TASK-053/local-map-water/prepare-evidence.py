"""Snapshot this water-colour revision and measure the existing lake appearance."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

import numpy as np
from PIL import Image, ImageFilter

QA = Path(__file__).resolve().parent
GAME = QA.parents[3]
sys.path.insert(0, str(GAME / 'scripts/ui'))
from draw_local_map import surface_levels, SIZE, ORIGIN, SPAN

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def write(name, value):
    (QA / name).write_text(json.dumps(value, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')

before = QA / 'before'
before.mkdir(exist_ok=True)
names = ['README.md', 'TestClient/README.md', 'docs/tasks/TASK-053.json',
    'docs/handoffs/TASK-053.md', 'scripts/ui/realistic_local_map.py',
    'scripts/ui/draw_local_map.py', 'scripts/ui/launch_map_test.py',
    'Resources/UI/interface.json', 'Resources/UI/layout.json',
    'Resources/UI/Art/map-local-terrain.png', 'docs/qa/TASK-053/local-map-angular/REPORT.md',
    '地图测试版_预览.png', 'parent/地图测试版_预览.png', 'parent/地图测试版_移动验证.png']
for name in names:
    source = GAME.parent / name.removeprefix('parent/') if name.startswith('parent/') else GAME / name
    target = before / (name.replace('/', '__') + '.snapshot')
    if target.exists():
        raise RuntimeError('Refusing to replace existing before evidence: ' + str(target))
    target.write_bytes(source.read_bytes())
write('manual-profile-before.json', {p.relative_to(GAME).as_posix(): sha(p)
    for p in (GAME / 'TestClient/Profile').rglob('*') if p.is_file()})
write('baseline-source.json', json.loads((QA.parent / 'local-map-angular/verify_639ec35e/source-manifest.json').read_text(encoding='utf-8')))
write('baseline-cartography.json', json.loads((QA.parent / 'local-map-relief-fog/cartography.json').read_text(encoding='utf-8')))
write('context.json', dict(head=subprocess.check_output(['git','rev-parse','HEAD'], cwd=GAME,text=True).strip(),
    branch=subprocess.check_output(['git','branch','--show-current'],cwd=GAME,text=True).strip(),
    reference='C:/Users/22543/AppData/Local/Temp/codex-clipboard-be5cca0b-8a92-4f2b-8212-cb894ede0ad1.png',
    snapshot_count=len(names), scope='Unify river and lake map water colour with the existing lake appearance.'))
scene = json.loads((QA.parent / 'local-map-angular/verify_639ec35e/scene.json').read_text(encoding='utf-8'))
small = np.asarray(scene['heightsM'], dtype=np.float32).reshape(scene['samples'],scene['samples'])
heights = np.asarray(Image.fromarray(small).resize((SIZE,SIZE),Image.Resampling.BICUBIC))
levels, _ = surface_levels(scene)
wet = np.isfinite(levels) & (heights < levels - .015)
interior = np.asarray(Image.fromarray(np.uint8(wet * 255)).filter(ImageFilter.MinFilter(15))) > 0
yy, xx = np.mgrid[:SIZE, :SIZE]
wx = ORIGIN[0] + (xx + .5) / SIZE * SPAN
wy = ORIGIN[1] + (yy + .5) / SIZE * SPAN
pixels = np.array(Image.open(GAME / 'Resources/UI/Art/map-local-terrain.png').convert('RGBA'))
lake = interior & (np.hypot(wx - 771, wy - 394) < 35) & (pixels[...,3] == 255)
depth = np.clip(np.nan_to_num(levels - heights) / 8, 0, 1)
ref_depth = float(np.median(depth[lake]))
palette = np.array([63.,79.,82.]) * (1-ref_depth) + np.array([39.,56.,66.]) * ref_depth
measurement = dict(lake_reference_world_center_m=[771,394], reference_radius_m=35,
    lake_interior_pixels=int(lake.sum()), median_previous_depth_fraction=ref_depth,
    measured_lake_rgb_median=np.median(pixels[lake,:3],axis=0).tolist(),
    measured_lake_rgb_mean=np.mean(pixels[lake,:3],axis=0).tolist(),
    depth_palette_at_lake_median=palette.tolist(), proposed_shared_rgb=np.rint(palette).astype(int).tolist(),
    old_water_colour='Shallow [63,79,82] to deep [39,56,66], selected by each mesh surface level and terrain depth.',
    water_actors=scene['water'])
write('lake-reference.json',measurement)
print(json.dumps({k:v for k,v in measurement.items() if k!='water_actors'},ensure_ascii=False,indent=2))
