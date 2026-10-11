"""Bake the existing journal selection masks; no new art or runtime dependency."""
import argparse
import json
import math
from pathlib import Path

from PIL import Image


def prepare(root: Path, check: bool = False) -> None:
    theme_path = root / 'interface.json'
    theme = json.loads(theme_path.read_text(encoding='utf-8'))
    assets = theme['assets']
    changed = False
    for name, entry in list(assets.items()):
        if not name.startswith('journalCategory') or name == 'journalCategorymain' or name.endswith('Highlight'):
            continue
        x, y, width, height = entry['uv']
        with Image.open(root / entry['file']) as atlas:
            source = atlas.convert('RGBA').crop((x, y, x + width, y + height))
        pixels = [source.getpixel((column, row)) for row in range(height) for column in range(width)]
        peak = max(21, max(max(r, g, b) for r, g, b, _ in pixels))
        mask = Image.new('RGBA', source.size)
        mask.putdata([(255, 242, 207, math.floor(a * max(0., min(1., (max(r, g, b) - 20) / (peak - 20))) ** .7 + .5))
                      for r, g, b, a in pixels])
        relative = 'Generated/' + name + 'Highlight.png'
        destination = root / relative
        expected = {'file': relative}
        if check:
            if assets.get(name + 'Highlight') != expected:
                raise ValueError('Missing or stale theme entry: ' + name)
            with Image.open(destination) as existing:
                if existing.size != mask.size or existing.convert('RGBA').tobytes() != mask.tobytes():
                    raise ValueError('Stale journal mask: ' + str(destination))
        else:
            destination.parent.mkdir(parents=True, exist_ok=True)
            mask.save(destination)
            if assets.get(name + 'Highlight') != expected:
                assets[name + 'Highlight'] = expected
                changed = True
    if changed:
        theme_path.write_text(json.dumps(theme, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    prepare(Path(__file__).resolve().parents[2] / 'Resources/UI', args.check)
    print('Journal selection masks: ' + ('verified' if args.check else 'prepared'))
