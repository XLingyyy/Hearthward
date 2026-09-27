"""Host runner; retains the UEClient that owns the editor until it is stopped."""
import argparse,json,sys,time
from pathlib import Path
p=argparse.ArgumentParser()
p.add_argument('--framework',default='G:/GameFactory');p.add_argument('--engine',default='G:/UnrealEngine/UE_5.8')
p.add_argument('--label',required=True);p.add_argument('--build',action='store_true');p.add_argument('--native',default='');p.add_argument('--null-rhi',action='store_true')
p.add_argument('--script',default='docs/qa/TASK-049/verify_campaign_natural.py');p.add_argument('--map',default='/Game/Hearthward/Bootstrap/L_Bootstrap')
p.add_argument('--result',default='Saved/Task049/pie/results.json');p.add_argument('--timeout',type=int,default=700)
args=p.parse_args();sys.path.insert(0,args.framework)
from engine_adapters.ue5 import UEClient
root=Path(__file__).resolve().parents[3];out=root/'docs/qa/TASK-049'
ue=UEClient(project_path=str(root/'Hearthward.uproject'),ue_root=args.engine)
def save(name,data):
    (out/(args.label+'-'+name+'.json')).write_text(json.dumps(data,ensure_ascii=False,indent=2),encoding='utf-8')
    print(json.dumps({'stage':name,'ok':data.get('ok'),'diagnostics':data.get('diagnostics',[])},ensure_ascii=False),flush=True)
if args.build:
    r=ue.build.project(target='HearthwardEditor',configuration='Development',timeout=1200);save('build',r)
    if not r['ok']:sys.exit(1)
if args.native:
    r=ue.testing.run_automation_tests(args.native,report_dir=str(root/'Saved/Task049'/args.label),extra_args=['-NullRHI'],timeout=300);save('native',r)
    if not r['ok']:sys.exit(1)
if not args.script:sys.exit(0)
result=root/args.result
if result.exists():result.rename(result.with_name(args.label+'-previous.json'))
stop=root/'Saved/Task049/stop-request'
if stop.exists():stop.unlink()
r=ue.runtime.launch_editor(map_path=args.map,extra_args=['-ExecutePythonScript='+str(root/args.script),'-NoSound','-unattended','-silent','-windowed','-ResX=1280','-ResY=720']+(['-NullRHI'] if args.null_rhi else []));save('launch',r)
if not r['ok']:sys.exit(1)
try:
    deadline=time.monotonic()+args.timeout
    while time.monotonic()<deadline and not result.exists() and not stop.exists():time.sleep(1)
    data=json.loads(result.read_text(encoding='utf-8')) if result.exists() else {'ok':False,'error':'No result before timeout or explicit stop'}
    save('pie',data);print(json.dumps({k:v for k,v in data.items() if k not in ['state','final_state','meshes','camp','house_collision']},ensure_ascii=False),flush=True)
finally:save('stop',ue.runtime.stop_editor(r['payload']['process_id']))

if not data.get("ok",False):sys.exit(1)
