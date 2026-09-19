"""Measure rendered compass ink, including the user's taller aspect ratio."""
from pathlib import Path
import json
import numpy as np
from PIL import Image
root=Path(__file__).resolve().parent
results=[]
for path in sorted(root.glob('fix01-hud-*.png')):
    pixels=np.asarray(Image.open(path).convert('RGB'),dtype=float)
    height,width=pixels.shape[:2]
    scale=min(width/1672,height/941);oy=(height-941*scale)/2
    left=int(width/2-250*scale);right=int(width/2+250*scale)
    top=int(oy+50*scale);bottom=int(oy+87*scale)
    ink=pixels[top:bottom,left:right]
    mask=(ink[:,:,0]>80)&(ink[:,:,1]>75)&(ink[:,:,0]-ink[:,:,2]>15)
    _,xs=np.nonzero(mask)
    assert xs.size,path
    center=left+(int(xs.min())+int(xs.max())+1)/2
    error=center-width/2
    results.append({'image':path.name,'screen_center_x':width/2,'rendered_ink_center_x':center,'error_pixels':error,'passed':abs(error)<=3*scale})
assert len(results)==12
report={'passed':all(row['passed'] for row in results),'tolerance':'3 design pixels; measures visible glyph bounds, excluding shadow, not just the layout box','results':results}
(root/'alignment-measurements.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print(json.dumps(report,indent=2))
assert report['passed']
