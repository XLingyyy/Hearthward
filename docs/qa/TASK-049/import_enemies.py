"""Import the two existing, owner-approved TASK-004 production sources."""
import json,shutil,traceback
from pathlib import Path
import unreal
root=Path(unreal.Paths.project_dir()).resolve()
out=root/'Saved/Task049/import.json';out.parent.mkdir(parents=True,exist_ok=True)
report={'ok':False,'meshes':[]}
try:
    tools=unreal.AssetToolsHelpers.get_asset_tools()
    for kind,source_name in [('Guard','short_blade_soldier'),('Heavy','heavy_armored_soldier')]:
        dest='/Game/Hearthward/Campaign/'+kind
        path=dest+'/SK_'+kind
        mesh=unreal.load_asset(path)
        if not mesh:
            source=root/'art_source/TASK-004/Tripo/敌人/outputs'/source_name/(source_name+'_rigged.fbx')
            temporary=out.parent/source.name;shutil.copy2(source,temporary)
            task=unreal.AssetImportTask();task.filename=str(temporary);task.destination_path=dest;task.destination_name='SK_'+kind
            task.automated=True;task.save=True;task.replace_existing=False
            options=unreal.FbxImportUI();options.import_mesh=True;options.import_as_skeletal=True;options.import_animations=False
            options.import_materials=True;options.import_textures=True;options.create_physics_asset=False
            options.mesh_type_to_import=unreal.FBXImportType.FBXIT_SKELETAL_MESH;options.automated_import_should_detect_type=False
            task.options=options;tools.import_asset_tasks([task]);mesh=unreal.load_asset(path)
        assert isinstance(mesh,unreal.SkeletalMesh),path
        reader=unreal.SkeletonModifier();assert reader.set_skeletal_mesh(mesh)
        report['meshes'].append({'kind':kind,'path':path,'bounds':str(mesh.get_bounds()),'bones':[{'name':str(b),'parent':str(reader.get_parent_name(b)),'global':str(reader.get_bone_transform(b,True))} for b in reader.get_all_bone_names()]})
    report['ok']=True
except Exception:report['error']=traceback.format_exc()
out.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
