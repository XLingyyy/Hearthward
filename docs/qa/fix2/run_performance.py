"""Uncapped standalone native-1080p Epic measurement via UEClient and UE CSV Profiler.
Run with the GameFactory venv, from G:/GameFactory. Pass --pool and --save for
an isolated QA save; omit them for a fresh prologue. No fixed timestep or frame generation.
"""
import argparse
import csv
import json
import sys
import time
import uuid
from pathlib import Path
csv.field_size_limit(16*1024*1024)
sys.path.insert(0,str(Path(__file__).resolve().parents[4]))
from engine_adapters.ue5 import UEClient

parser=argparse.ArgumentParser()
parser.add_argument('--pool')
parser.add_argument('--save')
parser.add_argument('--label',default='prologue')
parser.add_argument('--frames',type=int,default=9000)
parser.add_argument('--gpu-stats',action='store_true')
args=parser.parse_args()
root=Path(__file__).resolve().parents[3]
out=root/'Saved/Fix2Performance'/args.label
out.mkdir(parents=True,exist_ok=True)
existing=set((root/'Saved/Profiling/CSV').glob('*.csv'))
ue=UEClient(project_path=str(root/'Hearthward.uproject'),ue_root='G:/UnrealEngine/UE_5.8')
commands=[f'{60+i}:sg.{name}Quality 3' for i,name in enumerate(['ViewDistance','AntiAliasing','Shadow','GlobalIllumination','Reflection','PostProcess','Texture','Effects','Foliage','Shading','Landscape'])]
commands+=['71:r.ScreenPercentage 100','72:r.DynamicRes.OperationMode 0','73:t.MaxFPS 0','74:r.VSync 0','75:r.SetRes 1920x1080w']
# UE's CSV argument parser terminates at nested quotes. These paths have no spaces.
commands+=[f'{n}:Shot SHOWUI filename={(out / ("frame-"+str(n)+".png")).as_posix()} -nosuffix' for n in [100,600,1200]]
options='?game=/Script/Hearthward.HearthwardGameMode?'+('HearthwardLoad='+args.save if args.save else 'HearthwardNewGame=1')
launch=ue.runtime.launch_editor(map_path='/Game/Hearthward/World/Natural/Rebuild/L_HearthwardWilds'+options,
    extra_args=['-game','-UserDir='+root.as_posix(),'-windowed','-ResX=1920','-ResY=1080','-ForceRes','-NoVSync',
                '-HearthwardSaveTestPool='+(args.pool or str(uuid.uuid4())),
                '-csvCaptureFrames='+str(args.frames),'-csvCompression=0','-ExitAfterCsvProfiling',
                '-csvExecCmds='+','.join(commands)]+(['-csvGpuStats'] if args.gpu_stats else []))
(out/'launch.json').write_text(json.dumps(launch,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps({'launch':launch['ok'],'pid':launch.get('payload',{}).get('process_id'),'output':str(out)}),flush=True)
if not launch['ok']:sys.exit(1)
try:
    deadline=time.monotonic()+480
    captured=None
    while time.monotonic()<deadline:
        fresh=set((root/'Saved/Profiling/CSV').glob('*.csv'))-existing
        if fresh:
            candidate=max(fresh,key=lambda p:p.stat().st_mtime)
            # The engine appends the final metadata/header only after ending capture.
            try:
                content=candidate.read_text(encoding='utf-8-sig')
            except PermissionError:
                # Windows locks the CSV while the engine is writing the capture.
                time.sleep(2)
                continue
            if '[HasHeaderRowAtEnd]' in content:
                captured=candidate;break
        time.sleep(2)
    if captured is None:raise TimeoutError('CSV profiling did not complete')
    rows=list(csv.reader(captured.read_text(encoding='utf-8-sig').splitlines()))
    header=rows[0]
    index=header.index('FrameTime')
    frames=[]
    for row in rows[1:]:
        try:frames.append(float(row[index]))
        except (ValueError,IndexError):pass
    # Discard boot, loading, screenshots and initial streaming. Report 30 s or
    # more of real frame timings where possible, including all steady-state hitches.
    samples=frames[1800:]
    samples.sort()
    if not samples:raise ValueError('No steady-state frames')
    percentile=lambda p:samples[min(len(samples)-1,int((len(samples)-1)*p))]
    slowest=samples[-max(1,len(samples)//100):]
    one_percent_low=1000/(sum(slowest)/len(slowest))
    result={'ok':True,'csv':str(captured),'total_frames':len(frames),'measured_frames':len(samples),
            'measured_seconds':sum(samples)/1000,'average_fps':1000/(sum(samples)/len(samples)),
            'p99_frame_ms':percentile(.99),'p95_frame_ms':percentile(.95),
            'one_percent_low_fps':one_percent_low,
            'max_frame_ms':max(samples),'frames_over_16_67ms':sum(x>1000/60 for x in samples),
            'native_resolution':[1920,1080],'quality':3,'screen_percentage':100,
            'target_pass':percentile(.99)<=1000/60 and one_percent_low>=60}
    (out/'report.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
    print(json.dumps(result),flush=True)
finally:
    print(json.dumps(ue.runtime.stop_editor(launch['payload']['process_id'])),flush=True)
