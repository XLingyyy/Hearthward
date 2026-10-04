"""Verify the actual water-colour change against the saved pre-change map."""
import hashlib
import json
from pathlib import Path
import sys

import numpy as np
from PIL import Image, ImageFilter

QA = Path(__file__).resolve().parent
GAME = QA.parents[3]
sys.path.insert(0, str(GAME / 'scripts/ui'))
from draw_local_map import surface_levels, SIZE, ORIGIN, SPAN

def read(path):
    return json.loads(path.read_text(encoding='utf-8'))

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

scene_path = QA.parent / 'local-map-angular/verify_639ec35e/scene.json'
scene = read(scene_path)
small = np.asarray(scene['heightsM'],dtype=np.float32).reshape(scene['samples'],scene['samples'])
heights = np.asarray(Image.fromarray(small).resize((SIZE,SIZE),Image.Resampling.BICUBIC))
levels, _ = surface_levels(scene)
wet = np.isfinite(levels) & (heights < levels - .015)
interior = np.asarray(Image.fromarray(np.uint8(wet*255)).filter(ImageFilter.MinFilter(15))) > 0
old = np.array(Image.open(QA / 'before/Resources__UI__Art__map-local-terrain.png.snapshot').convert('RGBA')).astype(int)
new = np.array(Image.open(GAME / 'Resources/UI/Art/map-local-terrain.png').convert('RGBA')).astype(int)
yy, xx = np.mgrid[:SIZE,:SIZE]
wx = ORIGIN[0] + (xx+.5)/SIZE*SPAN
wy = ORIGIN[1] + (yy+.5)/SIZE*SPAN
safe_water = interior & (new[...,3] == 255)
lake = safe_water & (np.hypot(wx-790,wy-465) < 25)
old_lake_mean = old[lake,:3].mean(axis=0)
new_lake_mean = new[lake,:3].mean(axis=0)
change = np.any(old != new,axis=2)
chroma = (new[...,1]-new[...,0] == 17) & (new[...,2]-new[...,0] == 22)
chroma_ratio = float(np.mean(chroma[safe_water]))
# Compare pixels on either side of the actual model-level join, excluding banks.
# The old tint produced a hard vertical stripe here; neighbouring ripples remain.
seam = safe_water[:,:-1] & safe_water[:,1:] & (np.abs(np.diff(levels,axis=1)) > .5)
seam &= (wx[:,:-1] > 805) & (wx[:,:-1] < 870) & (wy[:,:-1] > 330) & (wy[:,:-1] < 510)
old_delta = np.max(np.abs(np.diff(old[...,:3],axis=1)),axis=2)[seam]
new_delta = np.max(np.abs(np.diff(new[...,:3],axis=1)),axis=2)[seam]
if not len(old_delta):
    raise RuntimeError('Actual lake/river join was not sampled')
before_cartography = read(QA / 'baseline-cartography.json')
current_cartography = read(QA / 'cartography.json')
preserved_fields = ['wet_mask_sha256','actor_footprints_sha256','height_samples_sha256',
    'material_sha256','water_mapping','display_vertical_exaggeration','land_mean_luminance','land_mean_saturation']
baseline = read(QA / 'baseline-source.json')['fingerprints']
cpp_and_dll = {name:digest for name,digest in baseline.items()
    if name.startswith('Source/') or name.endswith('UnrealEditor-Hearthward.dll')}
checks = dict(all_changed_pixels_are_water=bool(np.all(wet[change])),
    every_non_water_rgba_pixel_unchanged=bool(np.array_equal(old[~wet],new[~wet])),
    every_alpha_pixel_unchanged=bool(np.array_equal(old[...,3],new[...,3])),
    observed_water_land_and_object_geometry_preserved=all(before_cartography[k]==current_cartography[k] for k in preserved_fields),
    lake_reference_appearance_within_two_rgb_levels=bool(np.max(np.abs(new_lake_mean-old_lake_mean)) <= 2),
    lake_and_river_share_one_water_chroma=chroma_ratio > .999,
    sampled_mesh_join_has_no_colour_step=len(new_delta) > 100 and float(np.median(old_delta)) >= 8 and float(np.median(new_delta)) <= 1 and float(np.percentile(new_delta,95)) <= 3,
    water_ripple_detail_remains=float(np.std(new[lake,0])) > .5,
    angular_outline_default_view_and_regions_preserved=sha(GAME/'Resources/UI/interface.json')==sha(QA/'before/Resources__UI__interface.json.snapshot'),
    other_ui_layout_preserved=sha(GAME/'Resources/UI/layout.json')==sha(QA/'before/Resources__UI__layout.json.snapshot'),
    compiled_source_and_dll_unchanged=all(sha(GAME/name)==digest for name,digest in cpp_and_dll.items()),
    terrain_output_matches_cartography_sha=sha(GAME/'Resources/UI/Art/map-local-terrain.png')==current_cartography['output_sha256'])
report = dict(passed=all(checks.values()),checks=checks,changed_water_pixels=int(change.sum()),
    unchanged_non_water_pixels=int((~wet).sum()),unchanged_cpp_and_dll_files=len(cpp_and_dll),
    reference_sample=dict(center_m=[790,465],radius_m=25,pixels=int(lake.sum()),old_mean_rgb=old_lake_mean.tolist(),
        new_mean_rgb=new_lake_mean.tolist(),max_mean_channel_difference=float(np.max(np.abs(new_lake_mean-old_lake_mean))),
        new_water_detail_standard_deviation=float(np.std(new[lake,0]))),
    common_water_chroma_ratio=chroma_ratio,
    actual_mesh_join=dict(pixel_pairs=len(old_delta),old_median_rgb_step=float(np.median(old_delta)),
        new_median_rgb_step=float(np.median(new_delta)),new_95th_percentile_rgb_step=float(np.percentile(new_delta,95))),
    reused_build='local-map-angular/build_3fb31c6c; source and DLL unchanged, no new C++ build needed',
    source_scene=str(scene_path),terrain_sha256=sha(GAME/'Resources/UI/Art/map-local-terrain.png'))
(QA/'water-audit.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(report,ensure_ascii=False,indent=2))
raise SystemExit(0 if report['passed'] else 1)
