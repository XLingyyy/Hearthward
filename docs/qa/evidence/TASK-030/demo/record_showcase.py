import sys,json,time,subprocess
from pathlib import Path
sys.path.insert(0,'G:/GameFactory')
from engine_adapters.ue5 import UEClient
import imageio_ffmpeg
root=Path.cwd();out=root/'Saved/DemoValidation';take=out/'take';take.mkdir(exist_ok=True)
pool=next(x for x in json.loads((out/'launch.json').read_text())['payload']['command'] if x.startswith('-HearthwardSaveTestPool='))
ue=UEClient(project_path=root/'Hearthward.uproject',ue_root='G:/UnrealEngine/UE_5.8',port=30129,runtime_port=30130)
r=ue.runtime.launch_editor(map_path='/Game/Hearthward/Bootstrap/L_Bootstrap',extra_args=['-ExecutePythonScript='+str(out/'showcase.py'),pool,'-RenderOffscreen','-NoSound','-NoLiveCoding','-Unattended','-A3PlaytestOutput='+str(take),'-A3PlaytestFps=10','-A3PlaytestDuration=70','-ResX=1280','-ResY=720','-abslog='+str(out/'showcase.log')])
(out/'showcase-launch.json').write_text(json.dumps(r),encoding='utf-8')
try:
 end=time.monotonic()+240
 while not (take/'_editor_report.json').exists() and time.monotonic()<end:time.sleep(2)
 assert (take/'_editor_report.json').exists(),'recorder timeout'
 print((take/'_editor_report.json').read_text(),flush=True)
finally:
 ue.runtime.stop_editor(r['payload']['process_id'])
frames=sorted((take/'frames').glob('f*.png'));print('frames',len(frames),flush=True)
subprocess.run([imageio_ffmpeg.get_ffmpeg_exe(),'-y','-framerate','10','-i',str(take/'frames/f%05d.png'),'-vf','scale=1280:-2','-c:v','libx264','-pix_fmt','yuv420p','-crf','23',str(take/'demo.mp4')],check=True,capture_output=True)
print('video',str(take/'demo.mp4'),flush=True)
