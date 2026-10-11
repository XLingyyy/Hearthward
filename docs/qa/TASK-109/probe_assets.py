"""Read-only first-piece mesh budget through public UE editor APIs."""
import json, os, traceback
from pathlib import Path
import unreal
out=Path(os.environ['HEARTHWARD_TASK109_QA'])
report={'ok':False,'meshes':[]}
try:
    subsystem=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    for path in ['/Game/Hearthward/Assets/TASK-028/props/stone_bone_axe/SM_stone_bone_axe',
                 '/Game/Hearthward/Assets/TASK-028/furniture/wood_chair/SM_wood_chair',
                 '/Game/Hearthward/Assets/TASK-098/Workbench/SM_Workbench_Practical',
                 '/Game/Hearthward/Assets/TASK-096/Interior/SM_BedroomJoinery',
                 '/Game/Hearthward/Assets/TASK-100/Zones/SM_RiverGate']:
        mesh=unreal.load_asset(path)
        assert mesh,path
        count=subsystem.get_lod_count(mesh)
        sockets={}
        for name in ['Grip','BladeBase','BladeTip']:
            socket=mesh.find_socket(name)
            if socket:
                sockets[name]={'location':str(socket.relative_location),'rotation':str(socket.relative_rotation),'scale':str(socket.relative_scale)}
        report['meshes'].append({'path':path,'lods':count,
            'vertices':[subsystem.get_number_verts(mesh,i) for i in range(count)],
            'triangles':[mesh.get_num_triangles(i) for i in range(count)],
            'screen_sizes':list(subsystem.get_lod_screen_sizes(mesh)),
            'nanite':str(mesh.get_editor_property('nanite_settings')),
            'materials':[str(x.material_interface.get_path_name()) for x in mesh.get_editor_property('static_materials')],
            'sockets':sockets})
    report['ok']=True
except Exception:
    report['error']=traceback.format_exc()
finally:
    (out/'asset-budget.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
