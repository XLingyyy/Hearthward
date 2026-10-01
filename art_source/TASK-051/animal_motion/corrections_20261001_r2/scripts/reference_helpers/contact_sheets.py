"""Assemble actual rendered action frames into readable review sheets."""
from pathlib import Path
import json
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(r'E:\AiAgent\XLingGame\Resource\Tripo\动物\动作\制作成果')
FONT = r'C:\Windows\Fonts\msyh.ttc'

def main():
    folder = ROOT / '视觉检查'
    folder.mkdir(exist_ok=True)
    records = []
    for job in json.loads((ROOT / 'jobs.json').read_text(encoding='utf-8')):
        out = Path(job['output'])
        manifest = json.loads((out / 'animation_manifest.json').read_text(encoding='utf-8'))
        for start in range(0, len(manifest['clips']), 8):
            clips = manifest['clips'][start:start + 8]
            sheet = Image.new('RGB', (900, 80 + 250 * len(clips)), '#1e242d')
            draw = ImageDraw.Draw(sheet)
            draw.text((12, 12), f"{job['name']} / {job['slug']}  {start + 1}–{start + len(clips)}", font=ImageFont.truetype(FONT, 24), fill='white')
            draw.text((12, 47), 'Actual baked mesh poses: start / middle / end', font=ImageFont.truetype(FONT, 17), fill='#c4ccd8')
            for row, clip in enumerate(clips):
                y = 80 + row * 250
                draw.text((10, y), clip['suffix'] + f"   {clip['duration']}s / {clip['fps']}fps", font=ImageFont.truetype(FONT, 18), fill='white')
                for col, frac in enumerate((0, 50, 100)):
                    source = out / 'review' / f"{clip['suffix']}_{frac}.png"
                    if source.is_file():
                        tile = Image.open(source).convert('RGB').resize((300, 225), Image.Resampling.LANCZOS)
                        sheet.paste(tile, (col * 300, y + 25))
            path = folder / f"{job['slug']}_{start // 8 + 1}.jpg"
            sheet.save(path, quality=92)
            records.append({'slug':job['slug'],'actions':[c['name'] for c in clips],'path':str(path)})
    (folder / 'index.json').write_text(json.dumps(records, ensure_ascii=False, indent=2), encoding='utf-8')
    print(f'{len(records)} action review sheets')

if __name__ == '__main__': main()
