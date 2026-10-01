"""Compose verification contact sheets from actual rendered frames."""
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
from common import ROOT, REV, jobs, read

def assemble(phase):
    for job in jobs():
        slug=job['slug'];out=REV/'candidate'/slug if phase=='candidate' else Path(job['output'])
        if not (out/'animation_manifest.json').exists():continue
        runs=[c for c in read(out/'animation_manifest.json')['clips'] if c['kind']=='run']
        for c in runs:
            sheet=Image.new('RGB',(1600,2*346),(235,237,240));d=ImageDraw.Draw(sheet)
            for row,view in enumerate(('front','side')):
                folder=out/'gait_preview_r3'/f'{c["suffix"]}_{view}'
                files=sorted(folder.glob('*.png'))
                if not files:continue
                ids=[round((len(files)-1)*x) for x in (0,1/3,2/3,1)] if len(files)>4 else list(range(4))
                for col,i in enumerate(ids):
                    sheet.paste(Image.open(files[i]).convert('RGB').resize((400,320)),(col*400,row*346))
                    d.text((col*400+8,row*346+322),f'{slug} {c["suffix"]} {view} | frame {i+1}',fill=(35,39,44))
            path=REV/phase/f'{slug}_{c["suffix"]}_visual.jpg';path.parent.mkdir(parents=True,exist_ok=True);sheet.save(path,quality=92)
            print(path)

if __name__=='__main__':
    import sys
    assemble('candidate' if '--candidate' in sys.argv else 'final')
