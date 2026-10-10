"""UE editor-only controlled reimport of TASK-104 vertex deltas into the exact axe.

Default is a read-only probe. To apply, explicitly launch a fresh editor with
-Task104AxeApply -Task104AxeLockId=<your verified LFS lock id>. Verify the editor has
no unsaved changes first. Only the existing axe package may be saved. Run again
without Apply in a fresh editor to prove persisted geometry and sockets.
"""
import hashlib
import json
import re
import subprocess
import traceback
from pathlib import Path

import unreal

ROOT = Path(unreal.Paths.project_dir()).resolve()
MANIFEST = ROOT/'art_source/TASK-104/grip/handle_fit_delta.json'
ASSET_FILE = 'Content/Hearthward/Assets/TASK-028/props/stone_bone_axe/SM_stone_bone_axe.uasset'
command = unreal.SystemLibrary.get_command_line()
apply = '-task104axeapply' in command.lower()
report = {'action':'apply' if apply else 'probe','ok':False,'saved_assets':[]}
RECEIPT = Path(unreal.Paths.project_saved_dir())/'Task104/axe-grip-accepted.json'


def xyz(v): return [float(v.x),float(v.y),float(v.z)]


def socket_snapshot(mesh):
    result={}
    for name in ('Grip','BladeBase','BladeTip'):
        s=mesh.find_socket(name)
        assert s is not None, 'Required authored socket missing: '+name
        rotation=s.get_editor_property('relative_rotation')
        result[name]={'location':xyz(s.get_editor_property('relative_location')),
                      'rotation':[float(rotation.pitch),float(rotation.yaw),float(rotation.roll)],
                      'scale':xyz(s.get_editor_property('relative_scale'))}
    return result


def position_signature(description):
    positions=[]
    for i in range(description.get_vertex_count()):
        vid=unreal.VertexID(id_value=i)
        assert description.is_vertex_valid(vid), 'Non-dense source vertex IDs'
        positions.append(xyz(description.get_vertex_position(vid)))
    return hashlib.sha256(json.dumps(positions,separators=(',',':')).encode()).hexdigest()


def topology_uv_signature(description, channels):
    # IDs are dense in this exact imported source; validate before every read.
    vertices=[]
    for i in range(description.get_vertex_instance_count()):
        vid=unreal.VertexInstanceID(id_value=i)
        assert description.is_vertex_instance_valid(vid), 'Non-dense source vertex-instance IDs; stop rather than guess'
        uvs=[]
        for channel in range(channels):
            uv=description.get_vertex_instance_uv(vid,channel)
            uvs.append([float(uv.x),float(uv.y)])
        vertices.append([description.get_vertex_instance_vertex(vid).id_value,uvs])
    triangles=[]
    for i in range(description.get_triangle_count()):
        tid=unreal.TriangleID(id_value=i)
        assert description.is_triangle_valid(tid), 'Non-dense source triangle IDs; stop rather than guess'
        triangles.append([[v.id_value for v in description.get_triangle_vertex_instances(tid)],
                          description.get_triangle_polygon_group(tid).id_value])
    return hashlib.sha256(json.dumps([vertices,triangles],separators=(',',':')).encode()).hexdigest()


try:
    data=json.loads(MANIFEST.read_text(encoding='utf-8'))
    mesh=unreal.load_asset(data['mesh_path'])
    assert isinstance(mesh,unreal.StaticMesh), 'Production axe missing'
    assert unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world() is None, 'Refusing to operate during PIE or simulation'
    dirty=unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()
    dirty_maps=unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()
    assert not dirty and not dirty_maps, 'Refusing to operate in an editor with unsaved Content or map packages'
    before_sockets=socket_snapshot(mesh)
    expected=json.loads((ROOT/'docs/qa/TASK-055/stone-axe-measured-endpoints.json').read_text(encoding='utf-8'))['candidates']
    for name,point in expected.items():
        assert (unreal.Vector(*before_sockets[name]['location'])-unreal.Vector(*point)).length()<.00002
    materials=[mesh.get_material(i) for i in range(len(mesh.get_editor_property('static_materials')))]
    editor=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    collisions=[editor.get_simple_collision_count(mesh),editor.get_convex_collision_count(mesh)]
    description=mesh.get_static_mesh_description(0)
    assert description and description.get_vertex_count()==data['source_vertex_count'], 'LOD0 topology changed'
    channels=editor.get_num_uv_channels(mesh,0)
    assert channels>0, 'No material UV channels'
    topology_before=topology_uv_signature(description,channels)
    report['topology_uv_sha256_before']=topology_before
    report['uv_channels']=channels
    old=[];new=[]
    for row in data['vertices_id_original_xyz_new_xyz']:
        vid=unreal.VertexID(id_value=row[0]);assert description.is_vertex_valid(vid)
        actual=description.get_vertex_position(vid)
        old.append((actual-unreal.Vector(*row[1:4])).length())
        new.append((actual-unreal.Vector(*row[4:7])).length())
    already_applied=max(new)<.00003
    report['already_applied']=already_applied
    if already_applied:
        assert RECEIPT.exists(), 'Corrected geometry without original success receipt cannot be rebaselined'
        receipt=json.loads(RECEIPT.read_text(encoding='utf-8'))
        assert receipt.get('ok') and receipt['topology_uv_sha256_before']==topology_before
        assert receipt['whole_mesh_positions_sha256']==position_signature(description), 'Persisted head/body geometry changed'
        assert receipt['sockets']==before_sockets and receipt['material_paths']==[m.get_path_name() if m else None for m in materials] and receipt['collision_counts']==collisions, 'Preserved asset properties changed'
    if apply and not already_applied:
        assert not RECEIPT.exists(), 'Existing preservation baseline needs explicit review; do not overwrite it'
        match=re.search(r'-Task104AxeLockId=([^\s"]+)',command,re.I)
        assert match, 'Explicit LFS lock ID is required'
        locks=json.loads(subprocess.check_output(['git','lfs','locks','--verify','--json'],cwd=ROOT,text=True,timeout=60))
        assert any(str(x['id'])==match.group(1) and x['path']==ASSET_FILE for x in locks.get('ours',[])), 'Exact lock is not owned by current authenticated Git account'
        assert hashlib.sha256((ROOT/ASSET_FILE).read_bytes()).hexdigest()==data['source_uasset_sha256'], 'Production source uasset hash changed; review before reauthoring'
        assert max(old)<.00003, 'Original LOD0 no longer matches source delta'
        assert all(description.is_vertex_valid(unreal.VertexID(id_value=i)) for i in range(description.get_vertex_count())), 'Non-dense original vertex IDs'
        all_before=[description.get_vertex_position(unreal.VertexID(id_value=i)) for i in range(description.get_vertex_count())]
        for row in data['vertices_id_original_xyz_new_xyz']:
            description.set_vertex_position(unreal.VertexID(id_value=row[0]),unreal.Vector(*row[4:7]))
        mesh.build_from_static_mesh_descriptions([description],False,False)
        options=editor.get_lod_build_settings(mesh,0)
        options.set_editor_property('recompute_normals',True)
        options.set_editor_property('recompute_tangents',True)
        options.set_editor_property('generate_lightmap_u_vs',False)
        options.set_editor_property('remove_degenerates',False)
        editor.set_lod_build_settings(mesh,0,options)
        unreal.AutomationLibrary.finish_loading_before_screenshot()
        changed_ids={r[0] for r in data['vertices_id_original_xyz_new_xyz']}
        current=mesh.get_static_mesh_description(0)
        assert max((current.get_vertex_position(unreal.VertexID(id_value=i))-p).length()
                   for i,p in enumerate(all_before) if i not in changed_ids)<.00003, 'Unselected head/body vertices changed'
    description=mesh.get_static_mesh_description(0)
    report['changed_vertex_max_error_cm']=max((description.get_vertex_position(unreal.VertexID(id_value=r[0]))-unreal.Vector(*r[4:7])).length() for r in data['vertices_id_original_xyz_new_xyz'])
    assert report['changed_vertex_max_error_cm']<.00003, 'Corrected geometry is not present; run approved Apply before accepting this probe'
    assert editor.get_num_uv_channels(mesh,0)==channels, 'UV channel count changed'
    report['topology_uv_sha256_after']=topology_uv_signature(description,channels)
    assert report['topology_uv_sha256_after']==topology_before, 'UV values, triangle topology or material groups changed'
    assert socket_snapshot(mesh)==before_sockets, 'Rebuild changed authored sockets'
    assert len(mesh.get_editor_property('static_materials'))==len(materials), 'Material slot count changed'
    assert [mesh.get_material(i) for i in range(len(materials))]==materials, 'Rebuild changed materials'
    assert [editor.get_simple_collision_count(mesh),editor.get_convex_collision_count(mesh)]==collisions, 'Rebuild changed collision'
    report.update({'sockets':before_sockets,'material_paths':[m.get_path_name() if m else None for m in materials],
                   'collision_counts':collisions,'render_triangles':mesh.get_num_triangles(0)})
    report['whole_mesh_positions_sha256']=position_signature(description)
    if apply and not already_applied:
        assert unreal.EditorAssetLibrary.save_loaded_asset(mesh,only_if_is_dirty=False), 'Axe save failed'
        report['saved_assets']=[mesh.get_path_name()]
    report['ok']=True
    if apply and not already_applied:
        RECEIPT.parent.mkdir(parents=True,exist_ok=True)
        assert not RECEIPT.exists(), 'Refusing to overwrite an existing preservation baseline'
        RECEIPT.write_text(json.dumps(report,indent=2),encoding='utf-8')
except Exception:
    report['ok']=False
    report['error']=traceback.format_exc()
finally:
    out=Path(unreal.Paths.project_saved_dir())/'Task104';out.mkdir(parents=True,exist_ok=True)
    (out/('axe-grip-'+report['action']+'.json')).write_text(json.dumps(report,indent=2),encoding='utf-8')
    unreal.log(json.dumps(report))
