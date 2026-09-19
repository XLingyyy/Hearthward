from pathlib import Path
import json,os,time,uuid,sys,subprocess,hashlib,datetime,platform
from engine_adapters.ue5 import UEClient
r=Path(__file__).resolve().parents[5];base=r/'.agent-local/baseline';ev=Path(__file__).resolve().parent
u=UEClient(project_path=str(base/'Hearthward.uproject'),ue_root='G:/UnrealEngine/UE_5.8')
if sys.argv[1]=='build':
    result=u.build.project(target='HearthwardEditor',configuration='Development',timeout=1200)
    (ev/'baseline-build.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
    print(json.dumps({k:v for k,v in result.items() if k!='payload'},ensure_ascii=False));assert result['ok']
else:
    start=time.time();path=base/'Saved/Task025Rev2/baseline.json'
    manifest={'code_sha':subprocess.check_output(['git','rev-parse','HEAD'],cwd=base,text=True).strip(),'source_diff_sha256':hashlib.sha256(subprocess.check_output(['git','diff','--binary','HEAD','--','Source','config'],cwd=base)).hexdigest(),'testset_sha256':hashlib.sha256((ev/'dev.json').read_bytes()).hexdigest(),'started_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'harness_sha256':hashlib.sha256((ev/'verify_baseline.py').read_bytes()).hexdigest(),'command':sys.argv,'os':platform.platform(),'runtime':json.loads((base/'config/local-ai.lock.json').read_text(encoding='utf-8')),'backend':'vulkan','gpu_layers':32,'gpu_samples':[]}
    launch=u.runtime.launch_editor(map_path='/Game/Hearthward/Bootstrap/L_Bootstrap',extra_args=['-ExecutePythonScript='+str(ev/'verify_baseline.py'),'-HearthwardSaveTestPool='+str(uuid.uuid4()),'-HearthwardAIBackend=vulkan','-HearthwardAIGpuLayers=32','-unattended'])
    assert launch['ok'];print(launch['payload']['process_id'],flush=True)
    try:
        while time.time()-start<2400:
            if path.exists() and path.stat().st_mtime>start:
                result=json.loads(path.read_text(encoding='utf-8'));(ev/'baseline-results.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8');break
            q=subprocess.run(['nvidia-smi','--query-gpu=name,memory.used,memory.total,driver_version','--format=csv,noheader,nounits'],capture_output=True,text=True)
            if q.returncode==0:manifest['gpu_samples'].append({'seconds':round(time.time()-start,2),'gpu':q.stdout.strip()})
            time.sleep(2)
        else:raise TimeoutError('baseline')
    finally:print(u.runtime.stop_editor(launch['payload']['process_id']),flush=True)
    manifest['continuation_of']='baseline-partial-manifest.json';manifest['finished_utc']=datetime.datetime.now(datetime.timezone.utc).isoformat();(ev/'baseline-manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')
    print({k:v for k,v in result.items() if k not in ['cases','checks']},flush=True)
