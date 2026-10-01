"""Create labeled QA thumbnails; keep every native engine screenshot intact."""
import json
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

GAME = Path(__file__).resolve().parents[2]
OUT = GAME / 'docs/qa/TASK-051'


def main():
    runtime = json.loads((OUT / 'runtime_result.json').read_text('utf-8'))
    source = Path(runtime['evidence']) / 'native/screenshots'
    species = [row['slug'] for row in json.loads((OUT / 'import_report.json').read_text('utf-8'))['animals']]
    font = ImageFont.truetype('C:/Windows/Fonts/arial.ttf', 20)
    for state in ('natural', 'flee', 'dead'):
        sheet = Image.new('RGB', (1920, 1200), '#17212b')
        draw = ImageDraw.Draw(sheet)
        for index, animal in enumerate(species):
            col, row = index % 4, index // 4
            x, y = col * 480, row * 300
            raw = Image.open(source / f'{animal}_{state}.png').convert('RGB')
            raw.thumbnail((480, 270))
            sheet.paste(raw, (x, y + 30))
            draw.text((x + 8, y + 3), animal + ' / ' + state, fill='white', font=font)
        sheet.save(OUT / f'contact_{state}.jpg', quality=92)
        print('CONTACT_SHEET', state, len(species))


if __name__ == '__main__':
    main()
