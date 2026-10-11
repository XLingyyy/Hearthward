"""UE Python: add distance LODs to the two measured 95k+ props, retaining LOD0."""
import json,os,shutil,traceback
from pathlib import Path
import unreal
out=Path(os.environ['HEARTHWARD_TASK109_QA'])
root=Path(unreal.Paths.project_dir())
report={'ok':False,'meshes':[]}
def vector(value):
    return [value.x,value.y,value.z]
def sockets(mesh):
    result={}
    for name in ['Grip','BladeBase','BladeTip']:
        socket=mesh.find_socket(name)
        if socket:
            result[name]={'location':vector(socket.relative_location),'rotation':[socket.relative_rotation.pitch,socket.relative_rotation.yaw,socket.relative_rotation.roll],'scale':vector(socket.relative_scale)}
    return result
try:
    subsystem=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    for relative in ['Hearthward/Assets/TASK-028/props/stone_bone_axe/SM_stone_bone_axe',
                     'Hearthward/Assets/TASK-028/furniture/wood_chair/SM_wood_chair']:
        asset=unreal.load_asset('/Game/'+relative)
        assert asset and subsystem.get_lod_count(asset)==1, 'Expected measured original with one LOD'
        source=root/'Content'/(relative+'.uasset')
        backup=out/'original'/source.name
        backup.parent.mkdir(exist_ok=True)
        assert not backup.exists(), 'Keep original evidence'
        shutil.copy2(source,backup)
        before={'triangles':asset.get_num_triangles(0),'vertices':subsystem.get_number_verts(asset,0),
                'sockets':sockets(asset),'materials':[m.material_interface.get_path_name() for m in asset.static_materials]}
        options=unreal.StaticMeshReductionOptions()
        options.auto_compute_lod_screen_size=False
        settings=[]
        for fraction,screen in [(1.,1.),(.35,.18),(.10,.07)]:
            row=unreal.StaticMeshReductionSettings()
            row.percent_triangles=fraction;row.screen_size=screen;settings.append(row)
        options.reduction_settings=settings
        assert subsystem.set_lods(asset,options)==3
        assert asset.get_num_triangles(0)==before['triangles'] and subsystem.get_number_verts(asset,0)==before['vertices']
        assert sockets(asset)==before['sockets']
        assert [m.material_interface.get_path_name() for m in asset.static_materials]==before['materials']
        assert unreal.EditorAssetLibrary.save_loaded_asset(asset)
        report['meshes'].append({'path':'/Game/'+relative,'before':before,
            'triangles':[asset.get_num_triangles(i) for i in range(3)],
            'screen_sizes':list(subsystem.get_lod_screen_sizes(asset)), 'backup':str(backup)})
    report['ok']=True
except Exception:
    report['error']=traceback.format_exc()
finally:
    (out/'prop-lods.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
