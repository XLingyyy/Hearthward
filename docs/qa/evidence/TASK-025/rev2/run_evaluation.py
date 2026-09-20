"""Run from the GameFactory Python environment with PYTHONPATH pointing at GameFactory."""
from pathlib import Path
import datetime, hashlib, json, os, platform, shutil, subprocess, sys, time, uuid
from engine_adapters.ue5 import UEClient

r=Path(__file__).resolve().parents[5]
ev=Path(__file__).resolve().parent
mode=sys.argv[1]
script={'timeout':'verify_timeout.py','interactions':'verify_interactions.py','physicalb':'observe_workshop_ui.py','physical':'observe_agent.py','regression':'verify_regression.py','workshop':'verify_workshop.py','playercraft':'verify_playercraft.py','playerrepair':'verify_playerrepair.py'}.get(mode,'verify_agent.py')
def git(*args):return subprocess.check_output(['git',*args],cwd=r,stderr=subprocess.DEVNULL)
diff=git('diff','--binary','HEAD','--','Source','config','Resources/UI')
for relative in git('ls-files','--others','--exclude-standard','Source','config').decode().splitlines():
    diff+=relative.encode()+b'\0'+(r/relative).read_bytes()
testset=ev/('heldout.json' if mode=='heldout' else 'exposed-heldout.json' if mode=='exposed-heldout' else 'dev.json')
manifest={'started_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'mode':mode,
          'code_sha':git('rev-parse','HEAD').decode().strip(),'source_diff_sha256':hashlib.sha256(diff).hexdigest(),'source_worktree_dirty':bool(diff),
          'testset_sha256':hashlib.sha256(testset.read_bytes()).hexdigest(),'os':platform.platform(),
          'processor':platform.processor(),'command':sys.argv,'runtime':json.loads((r/'config/local-ai.lock.json').read_text(encoding='utf-8')),
          'gpu_memory_scope':'combined UE and model device usage; not per-process VRAM'}
if mode!='build':
    built=json.loads((ev/'build-manifest.json').read_text(encoding='utf-8'))
    assert json.loads((ev/'build-results.json').read_text(encoding='utf-8'))['ok'],'Recorded build failed'
    assert built['source_diff_sha256']==manifest['source_diff_sha256'],'Source changed since recorded build; build before validation'
    manifest['build_started_utc']=built['started_utc']
manifest['scenario_sha256']={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in [ev/'fault_scenarios.py',ev/'lifecycle_scenarios.py',ev/'workshop_scenarios.py',ev/'interaction_scenarios.py',ev/'timeout_scenarios.py'] if p.exists()}
manifest['harness_sha256']=hashlib.sha256((ev/script).read_bytes()).hexdigest()
u=UEClient(project_path=str(r/'Hearthward.uproject'),ue_root='G:/UnrealEngine/UE_5.8')
if mode=='build':result=u.build.project(target='HearthwardEditor',configuration='Development',timeout=1200)
elif mode=='native':result=u.testing.run_automation_tests(sys.argv[2] if len(sys.argv)>2 else 'Hearthward.',report_dir=str(r/'Saved/Task025Rev2/automation'),extra_args=['-NullRHI'],timeout=300)
else:
    start=time.time();resultpath=r/'Saved/Task025Rev2'/f'{mode}.json';samples=[]
    os.environ['HEARTHWARD_REV2_MODE']=mode
    backend='cpu' if mode=='cpu' else 'vulkan';manifest['backend']=backend
    launch=u.runtime.launch_editor(map_path='/Game/Hearthward/Bootstrap/L_Bootstrap',extra_args=['-ExecutePythonScript='+str(ev/script),'-HearthwardSaveTestPool='+str(uuid.uuid4()),'-HearthwardAIBackend='+backend,'-HearthwardAIGpuLayers=32']+([] if mode in ['physical','physicalb'] else ['-unattended']))
    assert launch['ok'];manifest['launch_arguments']=['-ExecutePythonScript='+str(ev/script),'-HearthwardSaveTestPool=<isolated UUID>','-HearthwardAIBackend='+backend,'-HearthwardAIGpuLayers=32']+([] if mode in ['physical','physicalb'] else ['-unattended']);print('started',mode,launch['payload']['process_id'],flush=True)
    try:
        while time.time()-start<3600:
            if resultpath.exists() and resultpath.stat().st_mtime>start:
                result=json.loads(resultpath.read_text(encoding='utf-8'));break
            sample=subprocess.run(['nvidia-smi','--query-gpu=name,memory.used,memory.total,driver_version','--format=csv,noheader,nounits'],capture_output=True,text=True)
            if sample.returncode==0:samples.append({'seconds':round(time.time()-start,2),'gpu':sample.stdout.strip()})
            time.sleep(2)
        else:raise TimeoutError('No completed evaluation report')
    finally:print(u.runtime.stop_editor(launch['payload']['process_id']),flush=True)
    manifest['gpu_samples']=samples
manifest['finished_utc']=datetime.datetime.now(datetime.timezone.utc).isoformat()
(ev/(mode+'-manifest.json')).write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')
(ev/(mode+'-results.json')).write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps({k:v for k,v in result.items() if k not in ['cases','checks','payload','model_trace','trace']},ensure_ascii=False),flush=True)
assert result.get('passed',result.get('ok',False))
