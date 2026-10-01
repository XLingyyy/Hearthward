"""Import the approved paired animal delivery using only public UEClient APIs."""
import os,sys,json,time,hashlib,traceback
from pathlib import Path
GAME=Path(__file__).resolve().parents[2]
from paths import SOURCE,configure_factory,load_jobs,source_path,ue_root
configure_factory(source_metadata=True)
from engine_adapters.ue5 import UEClient
OUT=GAME/'docs/qa/TASK-051';OUT.mkdir(parents=True,exist_ok=True)
def read(p):return json.loads(Path(p).read_text('utf-8'))
def save(p,v):Path(p).write_text(json.dumps(v,ensure_ascii=False,indent=2),encoding='utf-8')
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def descriptor(j,k):return {'game_id':j['game_id'],'run_id':j['run_id'],'task_kind':'motion','task_id':j['slug'],'artifact_key':k}
def artifact(r,t):
    assert r['ok'],r['errors']
    return next(a for a in r['artifacts'] if a['type']==t)
def main():
    jobs=load_jobs();report={'project':str(GAME/'Hearthward.uproject'),'source_revision':'R3 paired delivery','animals':[],'ok':False}
    client=UEClient(project_path=GAME/'Hearthward.uproject',ue_root=ue_root(),port=30031,runtime_port=30032)
    launch=client.runtime.launch_editor(extra_args=['-Unattended','-NullRHI','-NoSound','-NoLiveCoding','-NoSourceControl','-nosplash','-RCWebControlEnable'])
    save(OUT/'import_launch.json',launch);assert launch['ok'],launch['errors']
    try:
        for attempt in range(50):
            ready=client.observe.check_status(timeout=3)
            if ready.get('payload',{}).get('python_execution',{}).get('ok'):break
            time.sleep(2)
        else:raise RuntimeError('Game editor Python did not become ready')
        for job in jobs:
            folder=Path(job['output']);manifest=read(folder/'animation_manifest.json');sk=folder/f'SK_{job["slug"]}.fbx'
            assert sha(sk)==read(folder/'ue_import_qa.json')['skeletal_sha256']
            destination='/Game/Hearthward/Animals/MotionR3/'+job['slug']
            print('IMPORT_BEGIN',job['slug'],flush=True)
            avatar=artifact(client.assets.import_avatar(descriptor(job,'skeletal_fbx_path'),destination=destination,options={'as_skeletal':True}), 'avatar')
            mesh=client.reflection.inspect_artifact(avatar['artifact_id']);assert mesh['ok'],mesh['errors']
            skeleton_result=client.animation.resolve_skeleton(avatar['artifact_id']);assert skeleton_result['ok'],skeleton_result['errors']
            skeleton=skeleton_result['payload']['skeleton']
            row={'slug':job['slug'],'mesh':avatar,'skeleton':skeleton,'skeletal_sha256':sha(sk),'source_blend_sha256':sha(folder/f'AS_{job["slug"]}.blend'),'mesh_inspection':mesh,'clips':[]}
            report['animals'].append(row);save(OUT/'import_report.json',report)
            for clip in manifest['clips']:
                clip['file']=str(source_path(clip['file']))
                assert sha(clip['file'])==clip['sha256']
                a=artifact(client.animation.import_motion(descriptor(job,clip['name']+'_path'),skeleton=skeleton,destination=destination+'/Animations',options={'custom_sample_rate':clip['fps']}),'motion')
                check=client.reflection.inspect_artifact(a['artifact_id'],sample_animation=True);assert check['ok'],check['errors']
                anim=check.get('payload',{}).get('inspection',{}).get('animation',{})
                assert anim.get('verified') and abs(anim['duration_s']-clip['duration'])<.002,(job['slug'],clip['suffix'],anim)
                assert a['metadata']['skeleton_path']==skeleton,(a['metadata'],skeleton)
                row['clips'].append({'suffix':clip['suffix'],'name':clip['name'],'kind':clip['kind'],'asset':a['primary_asset']['path'],'source_sha256':clip['sha256'],'duration':clip['duration'],'fps':clip['fps'],'loop':clip['loop'],'hold':clip['hold'],'reference_speed_cm_s':clip.get('reference_speed_cm_s',0),'bone_count':anim.get('bone_count',len(anim['samples'][0]['bones'])),'native_duration':anim['duration_s'],'ok':True})
                save(OUT/'import_report.json',report)
                if len(row['clips'])%5==0:print('IMPORTED',job['slug'],len(row['clips']),len(manifest['clips']),flush=True)
            row['ok']=True;save(OUT/'import_report.json',report);print('IMPORT_COMPLETE',job['slug'],len(row['clips']),flush=True)
        report['ok']=len(report['animals'])==14 and sum(len(a['clips']) for a in report['animals'])==303
        save(OUT/'import_report.json',report);assert report['ok']
        print('ALL_PAIRED_ASSETS_IMPORTED',14,303,flush=True)
    except Exception:
        save(OUT/'import_error.json',{'error':traceback.format_exc()});raise
    finally:save(OUT/'import_stop.json',client.runtime.stop_editor(launch['payload']['process_id']))
if __name__=='__main__':main()
