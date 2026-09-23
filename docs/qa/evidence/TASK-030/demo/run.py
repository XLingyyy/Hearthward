import sys,time,json,uuid
from pathlib import Path
sys.path.insert(0,'G:/GameFactory')
from engine_adapters.ue5 import UEClient
root=Path('G:/GameFactory/Hearthward-ai-npc-fix');out=root/'Saved/DemoValidation'
ue=UEClient(project_path=root/'Hearthward.uproject',ue_root='G:/UnrealEngine/UE_5.8',port=30129,runtime_port=30130)
mode=sys.argv[1]
if mode=='native':r=ue.testing.run_automation_tests('Hearthward.',report_dir=str(out/'native'),extra_args=['-NullRHI'],timeout=300)
else:
 report=out/'result.json';assert not report.exists()
 args=['-ExecutePythonScript='+str(out/'verify_demo_pie.py'),'-HearthwardSaveTestPool='+str(uuid.uuid4()),'-NoLiveCoding','-NoSound','-Unattended','-abslog='+str(out/'runtime.log')]
 args+=['-windowed','-ResX=1280','-ResY=720','-HearthwardDemoHold'] if mode=='ui' else ['-RenderOffscreen']
 launched=ue.runtime.launch_editor(map_path='/Game/Hearthward/Bootstrap/L_Bootstrap',extra_args=args)
 (out/'launch.json').write_text(json.dumps(launched),encoding='utf-8')
 assert launched['ok'],launched.get('errors')
 try:
  end=time.monotonic()+1200
  while not report.exists() and time.monotonic()<end:time.sleep(2)
  r=json.loads(report.read_text(encoding='utf-8')) if report.exists() else {'ok':False,'error':'timeout'}
 finally:
  time.sleep(3);ue.runtime.stop_editor(launched['payload']['process_id'])
(out/(mode+'-result.json')).write_text(json.dumps(r,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps({'ok':r.get('ok'),'errors':r.get('errors'),'error':r.get('error'),'checks':len(r.get('checks',{}))},ensure_ascii=False),flush=True)
