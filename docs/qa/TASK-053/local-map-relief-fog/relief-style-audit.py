"""Measure the relief treatment and check that it preserves the observed geography."""
import hashlib
import json
from pathlib import Path
import numpy as np
from PIL import Image

QA = Path(__file__).resolve().parent
GAME = QA.parents[3]
cartography = json.loads((QA / 'cartography.json').read_text(encoding='utf-8'))
previous = json.loads((QA.parent / 'local-map-realistic/cartography.json').read_text(encoding='utf-8'))
scene = json.loads(Path(cartography['scene']).read_text(encoding='utf-8'))
config = json.loads((GAME / 'Resources/UI/interface.json').read_text(encoding='utf-8'))
before_config = json.loads((QA / 'before/Resources__UI__interface.json.snapshot').read_text(encoding='utf-8'))
generated = json.loads((QA / 'generated-asset.json').read_text(encoding='utf-8'))

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

image = np.array(Image.open(GAME / cartography['output']).convert('RGBA'))
old_image = np.array(Image.open(QA / 'before/Resources__UI__Art__map-local-terrain.png.snapshot').convert('RGB'))
size = image.shape[0]
yy, xx = np.mgrid[:size, :size]
wx, wy = 670 + (xx + .5) * 500 / size, 285 + (yy + .5) * 500 / size
radius = np.hypot(wx - 920, wy - 535)
regions = {'valley': (810, 674, 15), 'home': (1000, 557, 15), 'hills': (1020, 704, 50)}
measurements = {}
for name, source in [('before', old_image), ('after', image[..., :3])]:
    luminance = source @ np.array([.2126, .7152, .0722])
    measurements[name] = {}
    for region, (x, y, extent) in regions.items():
        values = luminance[np.hypot(wx-x, wy-y) < extent]
        lo, hi = np.percentile(values, [10, 90])
        measurements[name][region] = dict(mean=float(values.mean()), contrast_10_90=float(hi-lo))
heights = np.array(scene['heightsM']).reshape(251, 251)
gy, gx = np.mgrid[:251, :251]
sx, sy = 670 + gx * 2, 285 + gy * 2
height_summary = {}
for region, (x, y, extent) in regions.items():
    values = heights[np.hypot(sx-x, sy-y) < extent]
    height_summary[region] = dict(mean=float(values.mean()), range_m=[float(values.min()),float(values.max())])

checks = {key + '_preserved': cartography[key] == previous[key] for key in
          ['scene_sha256', 'center_m', 'diameter_m', 'height_samples_sha256', 'wet_mask_sha256', 'actor_footprints_sha256']}
checks.update({
    'equal_plan_scale_and_region_coordinates_preserved': all(config['localMap'][k] == before_config['localMap'][k]
        for k in ['centerCm', 'radiusCm', 'diameterPixels', 'regions']),
    'terrain_output_fingerprint_matches': sha(GAME / cartography['output']) == cartography['output_sha256'],
    'all_outside_terrain_pixels_transparent': bool(np.all(image[radius >= 250, 3] == 0)),
    'gray_dark_low_saturation_land': cartography['land_mean_luminance'] < 100 and cartography['land_mean_saturation'] < .2,
    'valley_floor_measured_lower_than_home': height_summary['home']['mean'] - height_summary['valley']['mean'] > 10,
    'valley_floor_visibly_darker_than_upland': measurements['after']['home']['mean'] - measurements['after']['valley']['mean'] > 12,
    'slope_region_contains_real_undulations': height_summary['hills']['range_m'][1] - height_summary['hills']['range_m'][0] > 8,
    'slope_relief_contrast_increased': measurements['after']['hills']['contrast_10_90'] > measurements['before']['hills']['contrast_10_90'] * 1.2,
    'new_fog_original_preserved': sha(GAME / generated['output']) == generated['sha256'] == sha(Path(generated['source'])),
    'flame_atlas_unchanged': sha(GAME / 'Resources/UI/Art/map-flames.png') == json.loads((QA.parent / 'local-map-realistic/verify_25725b6d/source-manifest.json').read_text(encoding='utf-8'))['fingerprints']['Resources/UI/Art/map-flames.png'],
})
result = dict(passed=all(checks.values()), checks=checks, measurements=measurements,
              actual_height_regions=height_summary, display_vertical_exaggeration=cartography['display_vertical_exaggeration'],
              scope='Image contrast aids visual QA; geometry uses unchanged measurements. Vertical exaggeration applies to shading only, with no world or XY deformation.')
(QA / 'relief-style-audit.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(result,ensure_ascii=False,indent=2))
raise SystemExit(0 if result['passed'] else 1)
