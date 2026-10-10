"""Bake dominant contact surfaces from the existing TASK-026 landscape material inputs.

No UE/map edits. Requires Pillow and numpy already used by terrain authoring. LFS
pointers are resolved read-only from the local object cache, never guessed/rebuilt.
"""
from pathlib import Path
import argparse
import hashlib
import io
import json
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[2]
SOURCES = [
    'art_source/TASK-026/Rebuild/biome_weights.png',
    'art_source/TASK-026/Rebuild/ReworkV2/S1_Blend.png',
    'art_source/TASK-026/Rebuild/ReworkV2/S1_Surface_Detail.png',
]
OUTPUT = ROOT / 'Resources/Audio/TASK-104/terrain-surfaces.rle'
MANIFEST = ROOT / 'art_source/TASK-104/terrain-surfaces.json'


def source_bytes(relative):
    data = (ROOT / relative).read_bytes()
    if data.startswith(b'version https://git-lfs.github.com/spec/v1'):
        fields = dict(line.split(' ', 1) for line in data.decode('ascii').splitlines())
        digest = fields['oid'].removeprefix('sha256:')
        common = Path(subprocess.check_output(['git', 'rev-parse', '--git-common-dir'], cwd=ROOT, text=True).strip())
        common = common if common.is_absolute() else ROOT / common
        cached = common / 'lfs/objects' / digest[:2] / digest[2:4] / digest
        if not cached.exists():
            raise RuntimeError(f'Hydrate {relative} with git lfs fetch before baking terrain; no substitute source is allowed.')
        data = cached.read_bytes()
        if len(data) != int(fields['size']) or hashlib.sha256(data).hexdigest() != digest:
            raise RuntimeError(f'LFS source integrity failure: {relative}')
    return data


def decode(data):
    """Strict decoder shared by headless integrity tests (runtime uses equivalent C++)."""
    if len(data) < 8 or data[:4] != b'HWS1' or (len(data)-8) % 3:
        raise ValueError('invalid header or truncated run')
    side, = struct.unpack_from('<I', data, 4)
    if side != 2017:
        raise ValueError('unexpected grid size')
    cells = bytearray()
    for offset in range(8, len(data), 3):
        surface, count = struct.unpack_from('<BH', data, offset)
        if surface not in (1, 2, 3) or count == 0 or len(cells)+count > side*side:
            raise ValueError('invalid run')
        cells.extend(bytes([surface])*count)
    if len(cells) != side*side:
        raise ValueError('incomplete grid')
    return side, cells


def build():
    import numpy as np
    from PIL import Image
    raw = [source_bytes(path) for path in SOURCES]
    base, blend, surface = [np.asarray(Image.open(io.BytesIO(data)).convert(mode), dtype=np.float64)/255
                            for data, mode in zip(raw, ['RGB', 'L', 'RGBA'])]
    if base.shape != (2017, 2017, 3) or blend.shape != (2017, 2017) or surface.shape != (2017, 2017, 4):
        raise ValueError('authoritative material texture dimensions changed')
    soil, rock = base[:, :, 1], base[:, :, 2]
    overlay = blend*surface[:, :, 0]
    weights = np.stack([(1-soil)*(1-rock)*(1-overlay),
                        soil*(1-rock)*(1-overlay)+overlay,
                        rock*(1-overlay)], axis=-1)
    cells = np.asarray(weights.argmax(axis=-1)+1, dtype=np.uint8).ravel()
    boundaries = np.flatnonzero(np.diff(cells))+1
    starts = np.concatenate(([0], boundaries))
    ends = np.concatenate((boundaries, [cells.size]))
    result = bytearray(b'HWS1'+struct.pack('<I', 2017))
    for start, end in zip(starts, ends):
        count = int(end-start)
        while count:
            run = min(count, 65535)
            result.extend(struct.pack('<BH', int(cells[start]), run))
            count -= run
    manifest = {
        'schema_version': 1,
        'source_material': '/Game/Hearthward/Assets/NaturalWorld/Rebuild/Materials/M_Landscape.M_Landscape',
        'sources': [{'path': path, 'sha256': hashlib.sha256(data).hexdigest()} for path, data in zip(SOURCES, raw)],
        'authoring': ['scripts/world/TASK-026/rebuild_terrain.py', 'scripts/world/TASK-026/rework_s1_surface.py',
                      'scripts/world/TASK-026/rework_s1_detail_assets.py'],
        'world_uv': '(ImpactPoint.XY + 201600cm) / 403200cm; no Y inversion',
        'size': 2017,
        'sampling': 'nearest texel dominant blend (~2m); visual texture bilinear/mips can soften boundaries',
        'formula': 'base=((1-G)*(1-B),G*(1-B),B); A=S1_Blend.R*S1_Surface_Detail.R; final=(base.grass*(1-A),base.dirt*(1-A)+A,base.stone*(1-A)); argmax grass,dirt,stone',
        'surface_ids': {'1': 'grass', '2': 'dirt', '3': 'stone'},
        'cell_counts': {name: int((cells == i).sum()) for i, name in enumerate(['grass', 'dirt', 'stone'], 1)},
        'output': OUTPUT.relative_to(ROOT).as_posix(),
        'output_sha256': hashlib.sha256(result).hexdigest(),
        'output_bytes': len(result),
        'license': 'Derived solely from existing project procedural biome/mask data; no external audio or imagery copied into this lookup.',
        'rebuild': 'python scripts/audio/prepare_footstep_terrain.py; regenerate if the landscape material masks/UV change',
    }
    return bytes(result), (json.dumps(manifest, ensure_ascii=False, indent=2)+'\n').encode('utf-8')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    data, manifest = build()
    decode(data)
    if args.check:
        if OUTPUT.read_bytes() != data or MANIFEST.read_bytes() != manifest:
            raise SystemExit('Terrain bake differs from authoritative inputs; regenerate and review.')
        print('Terrain surface bake matches authoritative inputs.')
    else:
        OUTPUT.parent.mkdir(parents=True, exist_ok=True)
        MANIFEST.parent.mkdir(parents=True, exist_ok=True)
        OUTPUT.write_bytes(data)
        MANIFEST.write_bytes(manifest)
        print(f'Wrote {len(data)} bytes: {OUTPUT.relative_to(ROOT)}')


if __name__ == '__main__':
    main()
