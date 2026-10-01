"""Host runner; retains the UEClient that owns the editor until it is stopped."""
import argparse,json,sys,time,uuid
from pathlib import Path
p=argparse.ArgumentParser()
p.add_argument('--framework',default='G:/GameFactory');p.add_argument('--engine',default='G:/UnrealEngine/UE_5.8')
p.add_argument('--label',required=True);p.add_argument('--build',action='store_true');p.add_argument('--native',default='');p.add_argument('--null-rhi',action='store_true')
p.add_argument('--interactive',action='store_true')
p.add_argument('--isolated-pool',action='store_true')
p.add_argument('--script',default='docs/qa/TASK-051/verify_experience_pie.py');p.add_argument('--map',default='/Game/Hearthward/Bootstrap/L_Bootstrap')
p.add_argument('--result',default='Saved/Task051/pie/results.json');p.add_argument('--timeout',type=int,default=700)
args=p.parse_args();sys.path.insert(0,args.framework)
from engine_adapters.ue5 import UEClient
root=Path(__file__).resolve().parents[3];out=root/'docs/qa/TASK-051'
ue=UEClient(project_path=str(root/'Hearthward.uproject'),ue_root=args.engine)
def save(name,data):
    (out/(args.label+'-'+name+'.json')).write_text(json.dumps(data,ensure_ascii=False,indent=2),encoding='utf-8')
    print(json.dumps({'stage':name,'ok':data.get('ok'),'diagnostics':data.get('diagnostics',[])},ensure_ascii=False),flush=True)
if args.build:
    r=ue.build.project(target='HearthwardEditor',configuration='Development',timeout=1200);save('build',r)
    if not r['ok']:sys.exit(1)
if args.native:
    r=ue.testing.run_automation_tests(args.native,report_dir=str(root/'Saved/Task051'/args.label),extra_args=['-NullRHI'],timeout=300);save('native',r)
    if not r['ok']:sys.exit(1)
if not args.script:sys.exit(0)
result=root/args.result
if result.exists():result.rename(result.with_name(args.label+'-previous-'+str(time.time_ns())+'.json'))
stop=root/'Saved/Task051/stop-request'
if stop.exists():stop.unlink()
extra=['-UserDir='+str(root/'Saved/Task051/device'),'-ExecutePythonScript='+str(root/args.script),'-windowed','-ResX=1920','-ResY=1080']
if not args.interactive:extra.extend(['-unattended','-silent'])
if args.isolated_pool:extra.append('-HearthwardSaveTestPool='+str(uuid.uuid4()))
if args.null_rhi:extra.append('-NullRHI')
r=ue.runtime.launch_editor(map_path=args.map,extra_args=extra);save('launch',r)
if not r['ok']:sys.exit(1)
# Disable selection-paused output only in this owned editor's log console.
# No global console preferences or other processes are changed.
import ctypes
kernel=ctypes.WinDLL("kernel32",use_last_error=True)
kernel.CreateFileW.argtypes=[ctypes.c_wchar_p,ctypes.c_ulong,ctypes.c_ulong,ctypes.c_void_p,ctypes.c_ulong,ctypes.c_ulong,ctypes.c_void_p]
kernel.CreateFileW.restype=ctypes.c_void_p
kernel.FreeConsole()
for attempt in range(80):
    if kernel.AttachConsole(r['payload']['process_id']):
        handle=kernel.CreateFileW("CONIN$",0xc0000000,3,None,3,0,None)
        mode=ctypes.c_ulong()
        if kernel.GetConsoleMode(ctypes.c_void_p(handle),ctypes.byref(mode)):
            kernel.SetConsoleMode(ctypes.c_void_p(handle),(mode.value|0x80)&~0x40)
        kernel.CloseHandle(ctypes.c_void_p(handle))
        kernel.FreeConsole()
        break
    time.sleep(.05)
try:
    deadline=time.monotonic()+args.timeout
    while time.monotonic()<deadline and not result.exists() and not stop.exists():time.sleep(1)
    data=json.loads(result.read_text(encoding='utf-8')) if result.exists() else {'ok':False,'error':'No result before timeout or explicit stop'}
    save('pie',data);print(json.dumps({k:v for k,v in data.items() if k not in ['state','final_state','meshes','camp','house_collision','nature']},ensure_ascii=False),flush=True)
finally:save('stop',ue.runtime.stop_editor(r['payload']['process_id']))

if not data.get("ok",False):sys.exit(1)
