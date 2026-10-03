from pathlib import Path
import json,sys,uuid,time
sys.path.insert(0,str(Path(__file__).resolve().parents[6]))
from engine_adapters.ue5 import UEClient
root=Path(__file__).resolve().parents[3]
pool=str(uuid.uuid4())
ue=UEClient(project_path=str(root/"Hearthward.uproject"),ue_root="G:/UnrealEngine/UE_5.8")
r=ue.runtime.launch_editor(map_path="/Game/Hearthward/World/Natural/Rebuild/L_HearthwardWilds",extra_args=["-ExecutePythonScript="+str(root/"docs/qa/TASK-052/verify_time_pie.py"),"-HearthwardSaveTestPool="+pool,"-NoSound","-NoSplash"])
(root/"docs/qa/TASK-052/launch.json").write_text(json.dumps({"save_pool":pool,"launch":r},ensure_ascii=False,indent=2),encoding="utf-8")
print(json.dumps(r,ensure_ascii=False),flush=True)
if not r['ok']:raise SystemExit(1)
started=time.time(); deadline=time.monotonic()+900; passed=False
result=root/'Saved/Task052/pie/results.json'
stop=root/('Saved/Task052/pie/stop-'+pool)
try:
    while time.monotonic()<deadline and not stop.exists():
        if result.exists() and result.stat().st_mtime>=started:
            report=json.loads(result.read_text(encoding='utf-8-sig'))
            passed=report['ok']
            print(json.dumps({'ok':report['ok'],'error':report.get('error'),'checks':report['checks']},ensure_ascii=False),flush=True)
            break
        time.sleep(1)
finally:
    closed=ue.runtime.stop_editor(r['payload']['process_id'])
    (root/'docs/qa/TASK-052/stop.json').write_text(json.dumps(closed,ensure_ascii=False,indent=2),encoding='utf-8')
    print(json.dumps({'closed':closed['ok']},ensure_ascii=False),flush=True)
raise SystemExit(0 if passed and closed['ok'] else 1)
