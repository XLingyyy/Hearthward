from pathlib import Path
import json,bisect,math
import numpy as np
from PIL import Image
import imageio_ffmpeg

root=Path('G:/GameFactory/Hearthward/.agent-local/qa/TASK-096/vfx-preview')
r=json.loads((root/'report.json').read_text(encoding='utf-8'))
samples=r['samples'];times=[s['wall_seconds'] for s in samples]
fps=8;duration=times[-1]+.25;size=(1280,864)
video=root/'Hearthward-Fire-Smoke-First-Article.mp4'
writer=imageio_ffmpeg.write_frames(str(video),size,fps=fps,codec='libx264',pix_fmt_in='rgb24',pix_fmt_out='yuv420p',quality=8,macro_block_size=1)
writer.send(None)
previous=-1
for i in range(math.ceil(duration*fps)):
    index=max(0,bisect.bisect_right(times,i/fps)-1)
    if index!=previous:
        frame=np.asarray(Image.open(root/'frames'/f'{index:03}.png').convert('RGB').resize(size,Image.Resampling.LANCZOS))
        previous=index
    writer.send(frame)
writer.close()
r['video']={'path':str(video),'fps':fps,'resolution':size,'duration_seconds':math.ceil(duration*fps)/fps,'frame_mapping':'hold each captured image until its recorded wall-clock timestamp successor; no speed-up','raw_resolution':list(Image.open(root/'frames/000.png').size),'stop_seconds':next(s['wall_seconds'] for s in samples if s['stopped'])}
r['visual_review']='TECHNICAL_PASS: visible flame and alpha smoke; stop tail clears; Owner appearance approval pending'
r['owner_approval']='PENDING'
(root/'report.json').write_text(json.dumps(r,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(r['video'],ensure_ascii=False))
