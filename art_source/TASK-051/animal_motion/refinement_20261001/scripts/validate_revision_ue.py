"""Native import and compressed animation sampling via public UEClient APIs."""
import os,sys,json,time,math,traceback,hashlib
from pathlib import Path
REPO=Path(r'E:\AiAgent\XLingGame\GameFactory-3A');sys.path.insert(0,str(REPO))
ROOT=Path(r'E:\AiAgent\XLingGame\Resource\Tripo\动物\动作\制作成果')
os.environ['AAAGF_OUTPUT_ROOT']=str(ROOT)
from engine_adapters.ue5 import UEClient
PROJECT=Path(r'E:\AiAgent\XLingGame\.animal-qa\HW\qa\pipeline\UEQA\UEQA.uproject')

def save(path,value):path.write_text(json.dumps(value,ensure_ascii=False,indent=2),encoding='utf-8')
def descriptor(job,key):return {'game_id':job['game_id'],'run_id':job['run_id'],'task_kind':'motion','task_id':job['slug'],'artifact_key':key}
def distance(a,b):return math.sqrt(sum((x-y)**2 for x,y in zip(a,b)))
def angle(a,b):return math.degrees(2*math.acos(min(1,abs(sum(x*y for x,y in zip(a,b))))))
def main():
    pilot='--pilot' in sys.argv
    jobs=json.loads((ROOT/'jobs.json').read_text(encoding='utf-8'))
    if sys.argv[1:]:jobs=[j for j in jobs if j['slug'] in sys.argv[1:]]
    client=UEClient(project_path=PROJECT,ue_root=r'E:\UE_5.8',port=30031,runtime_port=30032)
    launch=client.runtime.launch_editor(extra_args=['-Unattended','-NullRHI','-NoSound','-NoLiveCoding','-NoAssetRegistryCache','-NoSourceControl','-nosplash','-RCWebControlEnable'])
    save(ROOT/'ue_launch.json',launch)
    if not launch['ok']:raise RuntimeError(launch['errors'])
    try:
        status=None
        for attempt in range(30):
            status=client.observe.check_status(timeout=3)
            save(ROOT/'ue_ready.json',status)
            print('UE readiness '+str(attempt)+' '+str(status['ok'])+' '+str(status['errors']),flush=True)
            if status.get('payload',{}).get('python_execution',{}).get('ok'):break
            time.sleep(3)
        save(ROOT/'ue_ready.json',status)
        if not status.get('payload',{}).get('python_execution',{}).get('ok'):raise RuntimeError('Isolated UE Python asset interface did not become ready')
        for job in jobs:
            out=Path(job['output']);manifest=json.loads((out/'animation_manifest.json').read_text(encoding='utf-8'))
            mesh_sha=hashlib.sha256((out/('SK_'+job['slug']+'.fbx')).read_bytes()).hexdigest()
            prior_path=out/'ue_import_qa.json'
            if not pilot and prior_path.is_file():
                prior=json.loads(prior_path.read_text(encoding='utf-8'))
                if prior.get('ok') and prior.get('physical_scale_verified') and prior.get('skeletal_sha256')==mesh_sha and len(prior.get('clips',[]))==len(manifest['clips']) and all(a.get('source_sha256')==b['sha256'] for a,b in zip(prior['clips'],manifest['clips'])):
                    print('UE already verified '+job['slug'],flush=True);continue
            report={'slug':job['slug'],'project':str(PROJECT),'skeletal_sha256':mesh_sha,'clips':[],'ok':False}
            destination=('/Game/AnimalMotionCmPilot/' if pilot else '/Game/AnimalMotionRefined20261001/')+job['slug']
            mesh=client.assets.import_avatar(descriptor(job,'skeletal_fbx_path'),destination=destination,options={'as_skeletal':True})
            report['mesh_import']=mesh;save(out/'ue_import_qa.json',report)
            if not mesh['ok']:raise RuntimeError(mesh['errors'])
            avatar=next(a for a in mesh['artifacts'] if a['type']=='avatar')
            mesh_check=client.reflection.inspect_artifact(avatar['artifact_id']);report['mesh_inspection']=mesh_check
            physical=mesh_check.get('payload',{}).get('inspection',{}).get('skeletal_mesh',{})
            measured=physical.get('imported_size_cm',[0])[0]
            report['expected_length_cm']=manifest['rig']['axis']['target_length_m']*100
            report['physical_scale_verified']=abs(measured-report['expected_length_cm'])<.5
            save(out/'ue_import_qa.json',report)
            if not report['physical_scale_verified']:raise RuntimeError('Native physical size mismatch: '+str(physical))
            resolution=client.animation.resolve_skeleton(avatar['artifact_id']);report['skeleton_resolution']=resolution
            if not resolution['ok']:raise RuntimeError(resolution['errors'])
            skeleton=resolution['payload']['skeleton']
            for clip in (manifest['clips'][:1] if pilot else manifest['clips']):
                result=client.animation.import_motion(descriptor(job,clip['name']+'_path'),skeleton=skeleton,destination=destination+'/Animations',options={'custom_sample_rate':clip['fps']})
                record={'name':clip['name'],'source_sha256':clip['sha256'],'import':result,'ok':False}
                if result['ok']:
                    artifact=next(a for a in result['artifacts'] if a['type']=='motion')
                    inspection=client.reflection.inspect_artifact(artifact['artifact_id'],sample_animation=True)
                    record['inspection']=inspection
                    anim=inspection.get('payload',{}).get('inspection',{}).get('animation',{})
                    if anim.get('verified'):
                        poses=[s['bones'] for s in anim['samples']];names=set(poses[0]);expected=set(manifest['rig']['bone_heads'])
                        root_names=[n for n in names if n.lower()=='root'];root=root_names[0] if root_names else None
                        roots=[p[root] for p in poses] if root else []
                        changed=[n for n in names if any(distance(p[n]['translation_cm'],poses[0][n]['translation_cm'])>.01 or angle(p[n]['rotation_xyzw'],poses[0][n]['rotation_xyzw'])>.05 for p in poses[1:])]
                        metrics={'bone_names_match':names==expected,'native_bone_count':len(names),'root_translation_drift_cm':max(distance(p['translation_cm'],roots[0]['translation_cm']) for p in roots) if roots else None,'root_rotation_drift_deg':max(angle(p['rotation_xyzw'],roots[0]['rotation_xyzw']) for p in roots) if roots else None,'animated_bones':changed,'duration_error_s':abs(anim['duration_s']-clip['duration']),'native_sample_rate':anim['num_frames']/anim['duration_s']}
                        metrics['root_scale_error']=max(abs(v-1) for p in roots for v in p['scale']) if roots else None
                        if clip['loop']:
                            metrics['loop_position_error_cm']=max(distance(poses[0][n]['translation_cm'],poses[-1][n]['translation_cm']) for n in names)
                            metrics['loop_rotation_error_deg']=max(angle(poses[0][n]['rotation_xyzw'],poses[-1][n]['rotation_xyzw']) for n in names)
                        record['metrics']=metrics
                        record['ok']=bool(metrics['bone_names_match'] and root and metrics['root_scale_error']<.0001 and metrics['root_translation_drift_cm']<.1 and metrics['root_rotation_drift_deg']<.1 and metrics['duration_error_s']<.001 and abs(metrics['native_sample_rate']-clip['fps'])<.01 and (changed or clip['hold']) and (not clip['loop'] or (metrics['loop_position_error_cm']<.1 and metrics['loop_rotation_error_deg']<.1)))
                report['clips'].append(record);save(out/'ue_import_qa.json',report)
                print('UE '+job['slug']+' '+clip['suffix']+' '+str(record['ok']),flush=True)
                if not result['ok']:raise RuntimeError(result['errors'])
            report['ok']=all(c['ok'] for c in report['clips']);save(out/'ue_import_qa.json',report)
            if not pilot:
                manifest['ue_import']='PASS' if report['ok'] else 'REVIEW_REQUIRED';save(out/'animation_manifest.json',manifest)
    except Exception:
        save(ROOT/'ue_error.json',{'error':traceback.format_exc()});raise
    finally:
        save(ROOT/'ue_stop.json',client.runtime.stop_editor(launch['payload']['process_id']))

if __name__=='__main__':main()
