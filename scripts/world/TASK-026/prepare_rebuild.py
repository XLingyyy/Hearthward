"""Prepare the reference-led natural terrain and task-owned source assets.

Run with the GameFactory host Python. Does not launch or control Unreal.
"""
from pathlib import Path
import concurrent.futures
import json
import math
import zipfile

import numpy as np
from PIL import Image, ImageDraw, ImageFont
from scipy.ndimage import gaussian_filter, label, zoom
import requests

ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / 'art_source/TASK-026/Rebuild'
DOC = ROOT / 'docs/world/TASK-026'
OUT.mkdir(parents=True, exist_ok=True)
DOC.mkdir(parents=True, exist_ok=True)
SIZE = 2017
SIDE = 4032.0


def river_x(y):
    return 610 + 210 * np.sin(y / 490) + 90 * np.sin(y / 210)


def water_height(y):
    return 96 + (y + 2016) * .023 + 64 / (1 + np.exp(-(y + 650) / 28))


def make_terrain():
    if (OUT / 'height_m.npy').exists():
        raise RuntimeError('Existing terrain is immutable input. Use a bounded ReworkV2 height patch.')
    axis = np.linspace(-2016, 2016, SIZE, dtype=np.float32)
    x, y = np.meshgrid(axis, axis)
    rng = np.random.default_rng(260922)
    noise = np.zeros_like(x)
    for scale, amplitude in [(260, 30), (110, 13), (45, 5), (17, 1.6), (6, .45)]:
        count = int(SIDE / scale) + 3
        coarse = rng.uniform(-1, 1, (count, count))
        noise += amplitude * zoom(coarse, SIZE / count, order=3, mode='reflect')[:SIZE,:SIZE]
    terrace = 64 / (1 + np.exp(-(y + 650) / 28))
    h = 108 + (y + 2016) * .023 + terrace + noise
    # Long folded ridges, interrupted by two broad cross-valley passes.
    spine = -300 + 150 * np.sin(y / 610)
    passes = 1 - .83 * np.exp(-((y + 950) / 260) ** 2) - .82 * np.exp(-((y - 650) / 250) ** 2)
    h += 255 * np.exp(-((x - spine) / 230) ** 2) * passes
    h += 180 * np.exp(-((x + 1600 + 110 * np.sin(y / 440)) / 250) ** 2)
    for px, py, sx, sy, height in [(-250, 1400, 290, 360, 210), (-1450, 1000, 260, 450, 220),
                                  (-1200, -1500, 370, 290, 230), (1120, 1400, 230, 300, 180)]:
        h += height * np.exp(-((x - px) / sx) ** 2 - ((y - py) / sy) ** 2)
    # Watercourses are carved before any vegetation distribution.
    distance = np.abs(x - river_x(y))
    width = 27 + 9 * np.cos(y / 340)
    blend = np.exp(-(distance / (width + 19)) ** 4)
    riverbed = water_height(y) - 5 + 6 * (distance / (width + 12)) ** 2
    h = h * (1 - blend) + riverbed * blend
    # Wide eastern lake upstream of the falls; western sheltered forest lake.
    lakes = [(-930, -360, 240, 330, 157), (river_x(180), 180, 240, 330, float(water_height(180)))]
    for cx, cy, rx, ry, level in lakes:
        d = np.sqrt(((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2)
        angle = np.arctan2((y-cy)/ry,(x-cx)/rx)
        d *= 1 + .10*np.sin(3*angle) + .06*np.sin(5*angle+.7) + .03*np.sin(9*angle)
        bank = np.clip((1.40 - d) / .35, 0, 1)
        bank = bank * bank * (3 - 2 * bank)
        bed = level - 9 + 10 * np.clip(d, 0, 1.3) ** 3
        h = h * (1 - bank) + bed * bank
    # Natural coastal escarpment with an irregular, continuous shoreline.
    coast_x = 1710 + 90 * np.sin(y / 240) + 45 * np.sin(y / 93)
    coastal = 1 / (1 + np.exp(np.clip(-(x - coast_x) / 22, -80, 80)))
    h = h * (1 - coastal) + (-22 + noise * .1) * coastal
    # Two broad shallow crossings. No bridge or engineered road surface.
    for fy in [-1120, 680]:
        w = np.clip(np.minimum((320 - np.abs(x - river_x(fy))) / 230,
                               (120 - np.abs(y - fy)) / 70), 0, 1)
        w = w * w * (3 - 2 * w)
        h = (water_height(y) + .20) * w + h * (1 - w)
    # Grade only the small candidate sites, keeping irregular natural edges.
    clearings = [(-980, -750, 110, 'CAMP_A'), (1090, 450, 160, 'HOMELAND_B')]
    for cx, cy, radius, name in clearings:
        z = float(h[round((cy + 2016) / 2), round((cx + 2016) / 2)])
        if name=='CAMP_A':z=max(z,161)
        d = np.sqrt((x - cx) ** 2 + (y - cy) ** 2)
        w = np.clip((radius * 1.3 - d) / (radius * .6), 0, 1)
        w = w * w * (3 - 2 * w)
        h = h * (1 - w) + (z + .01 * (x - cx)) * w
    h = gaussian_filter(h, .7)
    np.save(OUT / 'height_m.npy', h)
    height16 = np.clip(np.rint(32768 + h * 32), 0, 65535).astype(np.uint16)
    Image.fromarray(height16).save(OUT / 'height_2017.png')
    rgba = np.zeros((SIZE, SIZE, 4), dtype=np.uint8)
    rgba[:, :, 0] = height16 >> 8
    rgba[:, :, 1] = height16 & 255
    rgba[:, :, 3] = 255
    Image.fromarray(rgba).save(OUT / 'height_rg.png')
    dy, dx = np.gradient(h, 2)
    slope = np.hypot(dx, dy)
    water = (distance < width + 2) & (h < water_height(y) - .15)
    for cx, cy, rx, ry, level in lakes:
        angle=np.arctan2((y-cy)/ry,(x-cx)/rx)
        radius=np.sqrt(((x-cx)/rx)**2+((y-cy)/ry)**2)
        radius*=1+.10*np.sin(3*angle)+.06*np.sin(5*angle+.7)+.03*np.sin(9*angle)
        water |= (radius<1.02) & (h<level-.15)
    water |= h < 0
    walkable = (slope < math.tan(math.radians(32))) & ~water
    labels, _ = label(walkable)
    counts = np.bincount(labels.ravel()); counts[0] = 0
    largest = int(counts.argmax())
    connected = labels == largest
    Image.fromarray((connected * 255).astype('uint8')).save(OUT / 'walkable_mask.png')
    # RGB weights: grass, earth, exposed rock. Macro colour has low contrast.
    rock = np.clip((slope - .35) / .65 + (h - 400) / 450, 0, 1)
    soil = np.clip((55 - distance) / 45, 0, .8) * (1 - rock)
    mask = np.stack([1 - rock - soil, soil, rock], axis=2)
    Image.fromarray((mask * 255).astype('uint8')).save(OUT / 'biome_weights.png')
    # A readable plan is evidence of intended layout, separate from UE images.
    base = np.zeros((SIZE, SIZE, 3), dtype=float)
    forest = x < -200
    base[:] = [150, 151, 96]
    base[forest] = [64, 101, 68]
    base = base * (1 - rock[:, :, None] * .8) + np.array([156, 153, 143]) * rock[:, :, None] * .8
    light = np.clip(.90 - .23 * dx + .25 * dy, .55, 1.2)
    base *= light[:, :, None]
    base[water] = [62, 125, 145]
    plan = Image.fromarray(np.clip(base, 0, 255).astype('uint8')).resize((1200, 1200))
    draw = ImageDraw.Draw(plan)
    font_path = 'C:/Windows/Fonts/msyh.ttc'
    font = ImageFont.truetype(font_path, 23)
    small = ImageFont.truetype(font_path, 17)
    def pos(px, py): return ((px + 2016) / SIDE * 1200, (py + 2016) / SIDE * 1200)
    for px, py, text in [(-1300,-1350,'西部林地山脊'),(-930,-300,'林间湖'),(850,0,'东部草原 / 主湖'),
                         (580,-650,'台地瀑布'),(-250,1400,'北部双峰'),(1550,1050,'海岸断崖')]:
        xx, yy = pos(px,py); draw.text((xx+2,yy+2),text,font=font,fill='#152322',stroke_width=3,stroke_fill='#eee9d4')
    for cx,cy,radius,name in clearings:
        xx,yy=pos(cx,cy); draw.ellipse((xx-8,yy-8,xx+8,yy+8),fill='#f6e5ab',outline='black',width=2)
        draw.text((xx+12,yy),name,font=small,fill='white',stroke_width=2,stroke_fill='#243529')
    draw.rectangle((25,25,585,108),fill='#182e2b')
    draw.text((44,36),'HEARTHWARD  /  自然世界总体图',font=font,fill='#f1ebd8')
    draw.text((44,72),'4.032 × 4.032 km  ·  北向 +Y（图下方） ·  单位：米',font=small,fill='#cbd6c6')
    draw.line((50,1145,50+500/SIDE*1200,1145),fill='white',width=5)
    draw.text((50,1154),'500 m',font=small,fill='white',stroke_width=2,stroke_fill='black')
    plan.save(DOC / 'rebuild-masterplan.png')
    result = {'samples':SIZE,'spacing_m':2,'side_m':SIDE,'height_min_m':float(h.min()),'height_max_m':float(h.max()),
              'largest_connected_walkable_km2':float(counts[largest]*4/1e6),'walkable_method':'2m height samples; slope <=32deg; water excluded; four-neighbour largest connected component; geometric estimate, not playtest',
              'lakes':[list(map(float,l)) for l in lakes], 'clearings':[list(v) for v in clearings],
              'fords_y_m':[-1120,680], 'seed':260922,'landscape_z_scale':400}
    (OUT/'layout.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
    print(json.dumps(result,ensure_ascii=False),flush=True)


def fetch_asset(name):
    dest=OUT/'polyhaven'/name; dest.mkdir(parents=True,exist_ok=True)
    manifest=requests.get(f'https://api.polyhaven.com/files/{name}',timeout=40).json()
    entry=manifest['fbx']['1k']['fbx']
    path=dest/(name+'.fbx')
    if not path.exists():
        with requests.get(entry['url'],stream=True,timeout=90) as response:
            response.raise_for_status()
            with path.open('wb') as stream:
                for chunk in response.iter_content(1024*1024): stream.write(chunk)
    (dest/'source.json').write_text(json.dumps({'source':f'https://polyhaven.com/a/{name}','license':'CC0',
        'download_url':entry['url'],'date':'2026-09-22','bytes':path.stat().st_size,'textures':'existing TASK-004 originals, read-only'},indent=2),encoding='utf-8')
    print(f'Fetched {name}: {path.stat().st_size} bytes',flush=True)


def fetch_cliff_surface():
    dest=OUT/'polyhaven/rock_3';dest.mkdir(parents=True,exist_ok=True)
    manifest=requests.get('https://api.polyhaven.com/files/rock_3',timeout=40).json()
    entries=[manifest['Diffuse']['2k']['jpg'],manifest['nor_gl']['2k']['png']]
    for entry in entries:
        path=dest/entry['url'].rsplit('/',1)[-1]
        if not path.exists():
            response=requests.get(entry['url'],timeout=90);response.raise_for_status()
            path.write_bytes(response.content)
    (dest/'source.json').write_text(json.dumps({'source':'https://polyhaven.com/a/rock_3','license':'CC0',
        'date':'2026-09-22','downloads':entries},indent=2),encoding='utf-8')


if __name__ == '__main__':
    if (OUT / 'height_m.npy').exists():
        raise SystemExit('Initial generation refused: Rebuild exists. No source files were replaced.')
    make_terrain()
    with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:
        list(pool.map(fetch_asset,['jacaranda_tree','shrub_01','tree_stump_01']))
    fetch_cliff_surface()
    for name in ['中岩石','小岩石']:
        source=ROOT/'art_source/TASK-004/sketchfab/岩石'/name/'source'
        archive=next(source.glob('*.zip'))
        dest=OUT/'sketchfab'/name; dest.mkdir(parents=True,exist_ok=True)
        with zipfile.ZipFile(archive) as z:
            for member in z.infolist():
                if member.filename.lower().endswith(('.fbx','.obj')):
                    (dest/Path(member.filename).name).write_bytes(z.read(member))
    print('Source preparation complete',flush=True)
