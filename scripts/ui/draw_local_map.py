"""Draw a 1:1 gray-dark realistic map from the current game's collision and model export.

Uses NumPy and Pillow from the bundled workspace runtime. No generated geography,
invented landmarks or edits to the reference images. The PNG contains no text.
"""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import struct
import numpy as np
from PIL import Image, ImageDraw, ImageFilter

GAME = Path(__file__).resolve().parents[2]
SIZE = 1800
ORIGIN = np.array([670., 285.])
SPAN = 500.


def glb(path):
    data = path.read_bytes()
    assert data[:4] == b'glTF'
    chunks = {}
    offset = 12
    while offset < len(data):
        size, kind = struct.unpack_from('<II', data, offset)
        chunks[kind] = data[offset + 8:offset + 8 + size]
        offset += 8 + size
    meta = json.loads(chunks[0x4e4f534a])
    binary = chunks[0x004e4942]

    def accessor(index):
        a = meta['accessors'][index]
        view = meta['bufferViews'][a['bufferView']]
        dtype = {5121: np.dtype('u1'), 5123: np.dtype('<u2'), 5125: np.dtype('<u4'), 5126: np.dtype('<f4')}[a['componentType']]
        count = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3, 'VEC4': 4}[a['type']]
        start = view.get('byteOffset', 0) + a.get('byteOffset', 0)
        stride = view.get('byteStride', count * dtype.itemsize)
        return np.ndarray((a['count'], count), dtype=dtype, buffer=binary, offset=start, strides=(stride, dtype.itemsize)).copy()

    prim = meta['meshes'][0]['primitives'][0]
    return accessor(prim['attributes']['POSITION']), accessor(prim['indices']).reshape(-1, 3)


def surface_levels(scene):
    levels = np.full((SIZE, SIZE), np.nan, dtype=np.float32)
    mappings = []
    for actor in scene['water']:
        name = actor['mesh'].removeprefix('SM_')
        path = GAME / 'art_source/TASK-026/Rebuild/water' / (name + '.glb')
        if not path.is_file():
            raise FileNotFoundError('Actual water mesh has no source: ' + str(path))
        verts, faces = glb(path)
        scale = np.asarray(actor.get('scale', [1, 1, 1]))
        target = np.asarray(actor['boundsM'])
        candidates = []
        # Infer the actual importer orientation from its measured world bounds.
        for swap, sign_x, sign_y in itertools.product((False, True), (-1, 1), (-1, 1)):
            xy = verts[:, [2, 0] if swap else [0, 2]].astype(float) * scale[:2] * [sign_x, sign_y] + [actor['x'], actor['y']]
            bounds = np.r_[xy.min(axis=0), xy.max(axis=0)]
            candidates.append((float(np.max(np.abs(bounds - target))), swap, sign_x, sign_y, xy))
        error, swap, sx, sy, xy = min(candidates, key=lambda c: c[0])
        if error > .25:
            raise ValueError(f'{name}: source geometry disagrees with runtime bounds by {error:.3f} m')
        # Break perfectly symmetric cases in favor of the authoring script's +Y convention.
        matches = [c for c in candidates if c[0] <= error + .001]
        _, swap, sx, sy, xy = min(matches, key=lambda c: int(c[1]) * 4 + int(c[2] < 0) * 2 + int(c[3] < 0))
        mappings.append(dict(mesh=name, max_bounds_error_m=error, swap_xy=swap, signs=[sx, sy]))
        xy = (xy - ORIGIN) / SPAN * SIZE
        z = verts[:, 1] * scale[2] + actor['z']
        for face in faces:
            p = xy[face]
            low = np.maximum(np.floor(p.min(axis=0)).astype(int), 0)
            high = np.minimum(np.ceil(p.max(axis=0)).astype(int) + 1, SIZE)
            if np.any(high <= low):
                continue
            x, y = np.meshgrid(np.arange(low[0], high[0]) + .5, np.arange(low[1], high[1]) + .5)
            denominator = (p[1, 1] - p[2, 1]) * (p[0, 0] - p[2, 0]) + (p[2, 0] - p[1, 0]) * (p[0, 1] - p[2, 1])
            if abs(denominator) < 1e-9:
                continue
            a = ((p[1, 1] - p[2, 1]) * (x - p[2, 0]) + (p[2, 0] - p[1, 0]) * (y - p[2, 1])) / denominator
            b = ((p[2, 1] - p[0, 1]) * (x - p[2, 0]) + (p[0, 0] - p[2, 0]) * (y - p[2, 1])) / denominator
            c = 1 - a - b
            inside = (a >= -1e-5) & (b >= -1e-5) & (c >= -1e-5)
            block = levels[low[1]:high[1], low[0]:high[0]]
            height = a * z[face[0]] + b * z[face[1]] + c * z[face[2]]
            # A distant sea plane may underlie lakes and rivers. Keep the upper
            # actual water surface wherever mesh footprints overlap.
            block[inside] = np.fmax(block[inside], height[inside])
    return levels, mappings


def contours(image, heights):
    draw = ImageDraw.Draw(image)
    lookup = {1: [(3, 0)], 2: [(0, 1)], 3: [(3, 1)], 4: [(1, 2)], 5: [(3, 0), (1, 2)],
              6: [(0, 2)], 7: [(3, 2)], 8: [(2, 3)], 9: [(0, 2)], 10: [(0, 1), (2, 3)],
              11: [(1, 2)], 12: [(3, 1)], 13: [(0, 1)], 14: [(3, 0)]}
    edge_vertices = [(0, 1), (1, 2), (2, 3), (3, 0)]
    for level in np.arange(np.floor(heights.min() / 4) * 4, heights.max() + 1, 4):
        pattern = ((heights[:-1, :-1] >= level).astype(np.uint8) + 2 * (heights[:-1, 1:] >= level)
                   + 4 * (heights[1:, 1:] >= level) + 8 * (heights[1:, :-1] >= level))
        for iy, ix in np.argwhere((pattern != 0) & (pattern != 15)):
            corners = np.array([[ix, iy], [ix + 1, iy], [ix + 1, iy + 1], [ix, iy + 1]], dtype=float)
            values = [heights[iy, ix], heights[iy, ix + 1], heights[iy + 1, ix + 1], heights[iy + 1, ix]]

            def point(edge):
                a, b = edge_vertices[edge]
                t = (level - values[a]) / (values[b] - values[a])
                return tuple((corners[a] * (1 - t) + corners[b] * t) / (heights.shape[0] - 1) * SIZE)

            for a, b in lookup[int(pattern[iy, ix])]:
                p, q = point(a), point(b)
                major = round(float(level)) % 12 == 0
                draw.line([(p[0] + 2, p[1] + 3), (q[0] + 2, q[1] + 3)], fill=(93, 78, 50, 120 if major else 70), width=5 if major else 3)
                draw.line([p, q], fill=(211, 203, 133, 165 if major else 92), width=2)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('scene', type=Path)
    parser.add_argument('--report-dir', type=Path, help='Cartography report directory; defaults to the scene directory')
    args = parser.parse_args()
    scene = json.loads(args.scene.read_text(encoding='utf-8'))
    small = np.asarray(scene['heightsM'], dtype=np.float32).reshape(scene['samples'], scene['samples'])
    assert np.all(np.isfinite(small)), 'Missing actual terrain observations'
    assert scene['originM'] == [670, 285] and scene['spacingM'] == 2
    heights = np.asarray(Image.fromarray(small).resize((SIZE, SIZE), Image.Resampling.BICUBIC))
    from realistic_local_map import render_realistic
    levels, mappings = surface_levels(scene)
    materials = GAME / 'Resources/UI/Art/map-surface-materials.png'
    image, style = render_realistic(scene, heights, levels, materials, ORIGIN, SPAN, SIZE)
    pixels = np.asarray(image)
    target = GAME / 'Resources/UI/Art/map-local-terrain.png'
    image.save(target)
    manifest = dict(scene=args.scene.resolve().as_posix(), scene_sha256=hashlib.sha256(args.scene.read_bytes()).hexdigest(),
        output=target.relative_to(GAME).as_posix(), output_sha256=hashlib.sha256(target.read_bytes()).hexdigest(),
        resolution=[SIZE, SIZE], center_m=[920, 535], diameter_m=500, projection='+X right, +Y down; identical X/Y scale',
        height_range_m=[float(small.min()), float(small.max())], water_mapping=mappings,
        trees=len(scene['trees']), rocks=len(scene['rocks']), houses=len(scene['houses']),
        text_in_png=False, **style,
        provenance='Actual Unreal collision and actor observations; checked water GLB footprints; built-in image_gen surface materials sampled on original geometry, no synthetic geography')
    report_dir = args.report_dir.resolve() if args.report_dir else args.scene.resolve().parent
    report_dir.mkdir(parents=True, exist_ok=True)
    (report_dir / 'cartography.json').write_text(json.dumps(manifest, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps(manifest, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
