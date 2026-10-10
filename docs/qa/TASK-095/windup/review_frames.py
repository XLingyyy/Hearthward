import json,subprocess
from pathlib import Path
from PIL import Image,ImageDraw
import imageio_ffmpeg
p=Path(__file__).parent/'visual';d=json.loads((p/'result.json').read_text(encoding='utf-8'))
for item in ['spear','longblade_2','axe']:
    rows=[r for r in d['samples'] if r['item']==item]
    im=Image.new('RGB',(1440,1110));draw=ImageDraw.Draw(im)
    for i,r in enumerate(rows[:9]):
        x=(i%3)*480;y=(i//3)*370
        im.paste(Image.open(p/item/('{:03}.png'.format(r['frame']))).resize((480,360)),(x,y))
        draw.text((x+4,y+3),'frame {} | +{:.2f}s | HP {}'.format(r['frame'],r['world_time']-rows[0]['world_time'],r['health']),fill='white')
    im.save(p/(item+'-contact.jpg'))
    lines=[]
    for i,r in enumerate(rows):
        lines.append("file '"+str((p/item/('{:03}.png'.format(r['frame']))).resolve()).replace('\\','/')+"'")
        lines.append('duration '+str(rows[i+1]['world_time']-r['world_time'] if i+1<len(rows) else .14))
    listing=p/(item+'.txt');listing.write_text('\n'.join(lines)+'\n',encoding='utf-8')
    writer=imageio_ffmpeg.write_frames(str(p/(item+'.mp4')),(960,720),fps=30,codec='libx264',output_params=['-crf','23'])
    writer.send(None)
    try:
        sent=0
        for i,r in enumerate(rows):
            end=(rows[i+1]['world_time'] if i+1<len(rows) else r['world_time']+.14)-rows[0]['world_time']
            pixels=Image.open(p/item/('{:03}.png'.format(r['frame']))).convert('RGB').tobytes()
            while sent<round(end*30):writer.send(pixels);sent+=1
    finally:writer.close()
print('review grids and time-based videos saved')
