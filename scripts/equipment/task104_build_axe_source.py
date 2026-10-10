"""Blender: deterministically narrow only the existing axe's handle, never its head.

blender -b --python scripts/equipment/task104_build_axe_source.py -- --source ORIGINAL.fbx
No UE package is edited. Output OBJ uses Blender world metres; the delta manifest
uses original UE LOD0 vertex IDs and centimetres, avoiding import-axis ambiguity.
"""
import argparse
import hashlib
import json
import sys
from pathlib import Path

import bpy
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[2]
SOURCE_SHA = "35cdf73c81a92870b63a694e6e7377c4322522735d0b66e0e56c706b7c0b1909"
GRIP = Vector((22.647987604141235, -17.993159770965576, -27.5))
AXIS = (Vector((19.044886589050293, -16.212165594100952, -22.5))
        - Vector((26.028714895248413, -19.51271870136261, -32.5))).normalized()
WORLD_SCALE = .7
TARGET_DIAMETER_CM = 1.8
CORE_HALF_LENGTH_CM = 10.0  # raw axial cm; 14 cm physical grasp region
TRANSITION_END_CM = 23.0
HEAD_PRESERVE_Z_CM = -5.0


def smooth(t):
    t = max(0., min(1., t))
    return t*t*(3.-2.*t)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=Path, required=True)
    args = parser.parse_args(sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else [])
    assert hashlib.sha256(args.source.read_bytes()).hexdigest() == SOURCE_SHA, "Original source SHA mismatch"
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=str(args.source.resolve()), use_anim=False)
    objects = [o for o in bpy.context.scene.objects if o.type == 'MESH']
    assert len(objects) == 1
    obj = objects[0]
    mesh = obj.data
    points = [obj.matrix_world @ v.co for v in mesh.vertices]
    original = [Vector((p.x*100, -p.y*100, p.z*100)) for p in points]
    assert len(original) == 47631
    evidence = json.loads((ROOT/'docs/qa/TASK-055/stone-axe-local-section-candidate-vertices.json').read_text(encoding='utf-8'))
    correspondence = max((original[r[0]]-Vector(r[1:4])).length for r in evidence['vertices_id_original_xyz_new_xyz'])
    assert correspondence < .00002, "FBX to UE vertex ID/coordinate mapping changed"
    axial = [(p-GRIP).dot(AXIS) for p in original]
    radii = [(p-GRIP-AXIS*t).length for p,t in zip(original,axial)]
    core_radius = max(r for r,t,p in zip(radii,axial,original) if abs(t)<=CORE_HALF_LENGTH_CM and p.z<HEAD_PRESERVE_Z_CM)
    radial_factor = TARGET_DIAMETER_CM/(2*WORLD_SCALE*core_radius)
    assert 0 < radial_factor < 1
    corrected=[]; edits=[]
    for i,(p,t) in enumerate(zip(original,axial)):
        amount = 1-smooth((abs(t)-CORE_HALF_LENGTH_CM)/(TRANSITION_END_CM-CORE_HALF_LENGTH_CM))
        # Keep the entire axe head unchanged, with an additional smooth shoulder.
        amount *= 1-smooth((p.z+12.)/7.)
        radial = p-GRIP-AXIS*t
        q = p-radial*((1-radial_factor)*amount)
        corrected.append(q)
        if (q-p).length > .000001:
            edits.append([i,*p,*q])
    assert max(abs((q-p).dot(AXIS)) for p,q in zip(original,corrected)) < .00001
    assert all((p-q).length == 0 for p,q in zip(original,corrected) if p.z >= HEAD_PRESERVE_Z_CM)
    # Preserve the exact source face topology, material indices and UV loop values.
    out=ROOT/'art_source/TASK-104/grip';out.mkdir(parents=True,exist_ok=True)
    uv = mesh.uv_layers.active
    assert uv
    lines=['# Hearthward TASK-104 corrected handle; Blender world metres, +Z up',
           '# Source sha256 '+SOURCE_SHA, 'o stone_bone_axe_handle_fit']
    for p in corrected: lines.append('v %.9f %.9f %.9f' % (p.x/100,-p.y/100,p.z/100))
    for loop in uv.data: lines.append('vt %.9f %.9f' % tuple(loop.uv))
    lines.append('s 1')
    last_material=None
    for face in mesh.polygons:
        if face.material_index != last_material:
            lines.append('usemtl original_material_%d'%face.material_index);last_material=face.material_index
        lines.append('f '+' '.join('%d/%d'%(mesh.loops[i].vertex_index+1,i+1) for i in face.loop_indices))
    obj_path=out/'stone_bone_axe_handle_fit.obj'
    obj_path.write_text('\n'.join(lines)+'\n',encoding='utf-8')
    data={'schema':1,'source_fbx_sha256':SOURCE_SHA,
          'source_uasset_sha256':'a54c68a246c86497664c48aad3f2714fca384588318ba6245eed0a63409de5bf',
          'mesh_path':'/Game/Hearthward/Assets/TASK-028/props/stone_bone_axe/SM_stone_bone_axe',
          'source_vertex_count':len(original),'source_polygon_count':len(mesh.polygons),
          'source_to_ue_coordinate_rule':'Blender world metres times (100,-100,100); original vertex IDs verified against archived UE LOD0',
          'source_to_ue_max_error_cm':correspondence,'world_scale':WORLD_SCALE,
          'target_core_max_diameter_world_cm':TARGET_DIAMETER_CM,'core_half_length_mesh_cm':CORE_HALF_LENGTH_CM,
          'transition_end_mesh_cm':TRANSITION_END_CM,'head_preserve_min_z_mesh_cm':HEAD_PRESERVE_Z_CM,
          'radial_factor':radial_factor,'original_core_max_diameter_world_cm':2*WORLD_SCALE*core_radius,
          'grip_mesh_cm':list(GRIP),'axis_mesh_unit':list(AXIS),
          'head_displacement_max_cm':0,'axial_displacement_max_cm':max(abs((q-p).dot(AXIS)) for p,q in zip(original,corrected)),
          'obj_sha256':hashlib.sha256(obj_path.read_bytes()).hexdigest(),
          'vertices_id_original_xyz_new_xyz':edits,
          'limitations':['UE rebuild/save/reload and native tests pending','Diameter correction is not proof of finger-surface clearance in all animations','Original material and sockets are preserved by UE application; OBJ is authoring source only']}
    (out/'handle_fit_delta.json').write_text(json.dumps(data,separators=(',',':'))+'\n',encoding='utf-8')
    summary={k:v for k,v in data.items() if k!='vertices_id_original_xyz_new_xyz'}
    summary['changed_vertices']=len(edits);summary['obj_bytes']=obj_path.stat().st_size
    (ROOT/'docs/qa/TASK-104/grip/source-build.json').write_text(json.dumps(summary,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(summary,indent=2))

if __name__=='__main__': main()
