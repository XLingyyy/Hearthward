"""Prepare game materials and the independent native map through public UEClient."""
import json,os,sys,time,traceback
import argparse
from pathlib import Path
GAME=Path(__file__).resolve().parents[2]
from paths import SOURCE,configure_factory,load_jobs,ue_root
configure_factory(source_metadata=True)
from engine_adapters.ue5 import UEClient
from pipeline.common.paths import task_output_dir,write_task_meta
from PIL import Image
OUT=GAME/'docs/qa/TASK-051'
def save(p,v):Path(p).write_text(json.dumps(v,ensure_ascii=False,indent=2),encoding='utf-8')
def desc(task,kind,key):return dict(game_id='Hearthward',run_id='animal_runtime_20261001',task_kind=kind,task_id=task,artifact_key=key)
def main():
    parser=argparse.ArgumentParser();parser.add_argument('--map-only',action='store_true');args=parser.parse_args()
    imported=json.loads((OUT/'import_report.json').read_text('utf-8'));assert imported['ok']
    jobs=load_jobs()
    materials=[]
    for name,color in [('Grass',(84,110,68)),('Wood',(113,81,50))]:
        task='animal_demo_'+name.lower();folder=task_output_dir('Hearthward','3d_object',task,run_id='animal_runtime_20261001')
        Image.new('RGB',(8,8),color).save(folder/'Color.png');Image.new('L',(8,8),235).save(folder/'Roughness.png')
        d=desc(task,'3d_object','material_source_path');write_task_meta(folder,{**d,'material_source_path':str(folder/'Color.png')})
        materials.append((name,d,folder))
    floor_folder=task_output_dir('Hearthward','3d_object','animal_demo_floor',run_id='animal_runtime_20261001')
    (floor_folder/'SM_demo_base.obj').write_text('o SM_demo_base\nv -50 -50 -50\nv 50 -50 -50\nv 50 50 -50\nv -50 50 -50\nv -50 -50 50\nv 50 -50 50\nv 50 50 50\nv -50 50 50\nf 4 3 2 1\nf 5 6 7 8\nf 1 2 6 5\nf 2 3 7 6\nf 3 4 8 7\nf 4 1 5 8\n',encoding='utf-8')
    floor_src=desc('animal_demo_floor','3d_object','model_path');write_task_meta(floor_folder,{**floor_src,'model_path':str(floor_folder/'SM_demo_base.obj')})
    scene_folder=task_output_dir('Hearthward','3d_scene','animal_demo_20261001',run_id='animal_runtime_20261001')
    client=UEClient(project_path=GAME/'Hearthward.uproject',ue_root=ue_root(),port=30031,runtime_port=30032)
    launch=client.runtime.launch_editor(extra_args=['-Unattended','-NullRHI','-NoSound','-NoLiveCoding','-NoSourceControl','-nosplash','-RCWebControlEnable'])
    save(OUT/'world_launch.json',launch);assert launch['ok'],launch['errors']
    report=json.loads((OUT/'world_report.json').read_text('utf-8')) if args.map_only else {'ok':False,'animal_materials':[],'habitat_materials':[],'scene_source':str(scene_folder/'scene.json')}
    try:
        for _ in range(50):
            status=client.observe.check_status(timeout=3)
            if status.get('payload',{}).get('python_execution',{}).get('ok'):break
            time.sleep(2)
        else:raise RuntimeError('Editor Python not ready')
        for a,j in ([] if args.map_only else zip(imported['animals'],jobs)):
            assert a['slug']==j['slug']
            src={'game_id':j['game_id'],'run_id':j['run_id'],'task_kind':'motion','task_id':j['slug'],'artifact_key':'skeletal_fbx_path'}
            r=client.bindings.bind_pbr_material(asset_id=a['slug'],source=src,mesh_assets=[a['mesh']['primary_asset']['path']],destination='/Game/Hearthward/Animals/MotionR3/'+a['slug'])
            assert r['ok'] and not r['payload'].get('skipped'),r
            report['animal_materials'].append(r);save(OUT/'world_report.json',report);print('ANIMAL_MATERIAL',a['slug'],flush=True)
        for name,src,folder in ([] if args.map_only else materials):
            r=client.bindings.bind_pbr_material(asset_id=name,source=src,mesh_assets=[],destination='/Game/Hearthward/Tests/AnimalMotion',options={'auto':False,'textures':{'base_color':str(folder/'Color.png'),'roughness':str(folder/'Roughness.png')}})
            assert r['ok'] and not r['payload'].get('skipped'),r
            report['habitat_materials'].append(r);save(OUT/'world_report.json',report)
        floor=client.assets.import_prop(floor_src,destination='/Game/Hearthward/Tests/AnimalMotion/Geometry',options={'generate_collision':True});report['floor']=floor;assert floor['ok'],floor
        a=floor['artifacts'][0]
        scene={'world_id':'animal_demo_20261001','representation':'mesh_layout','entities':[{'id':'animal_demo_floor','role':'environment','artifact_id':a['artifact_id'],'collision':True,'transform':{'location':{'x':0,'y':0,'z':-20},'scale':{'x':51,'y':39,'z':.4}}}],'add_default_lighting':True,'add_default_ground':False,'spawn_point':{'location':{'x':0,'y':-1850,'z':100}},'metadata':{'task':'TASK-051','runtime_game_mode':'/Script/Hearthward.HearthwardAnimalDemoGameMode','runtime_species':[a['slug'] for a in imported['animals']]}}
        save(scene_folder/'scene.json',scene);d=desc('animal_demo_20261001','3d_scene','scene_json_path');write_task_meta(scene_folder,{**d,'scene_json_path':str(scene_folder/'scene.json')})
        r=client.world.build(d,options={'world_id':'animal_demo_20261001','project_id':'Hearthward','publish':True,'preview_in_editor':False,'replace_existing':False})
        report['world']=r;save(OUT/'world_report.json',report);assert r['ok'],r['errors']
        report['ok']=True;save(OUT/'world_report.json',report);print('DEMO_WORLD_PREPARED',flush=True)
    except Exception:
        save(OUT/'world_error.json',{'error':traceback.format_exc()});raise
    finally:save(OUT/'world_stop.json',client.runtime.stop_editor(launch['payload']['process_id']))
if __name__=='__main__':main()
