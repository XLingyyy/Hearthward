"""Export the repaired 22-bone source using the established centimetre exporter."""
from pathlib import Path
import json
import runpy
import sys

import bpy
from mathutils import Matrix

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT.parent))
from pipeline.common.paths import task_output_dir, write_task_meta

assert Path(bpy.data.filepath).resolve() == (ROOT / 'art_source/TASK-095/Archer-Repaired.blend').resolve()
rig = bpy.data.objects['Armature']
meshes = [o for o in bpy.context.scene.objects if o.type == 'MESH']
assert len(rig.data.bones) == 22 and len(meshes) == 4
before = [(b.name, list(b.head_local), list(b.tail_local), b.parent.name if b.parent else None)
          for b in rig.data.bones]
report = {'bone_count': len(before), 'meshes': [], 'ue_import': 'NOT_RUN',
          'gameplay_binding': 'NOT_RUN', 'owner_visual': 'NOT_RUN'}
for obj in meshes:
    obj.data.calc_loop_triangles()
    missing = [v.index for v in obj.data.vertices if sum(g.weight for g in v.groups) < .99]
    assert not missing, (obj.name, len(missing))
    report['meshes'].append({'name': obj.name, 'vertices': len(obj.data.vertices),
                            'triangles': len(obj.data.loop_triangles),
                            'unweighted_vertices': len(missing),
                            'materials': [m.name for m in obj.data.materials]})
directory = task_output_dir('Hearthward', '3d_object', 'TASK-095-archer', 'repair-20261008')
directory.mkdir(parents=True, exist_ok=True)
exporter = runpy.run_path(str(ROOT / 'art_source/TASK-051/tooling/GameFactory-3A/operators/gen_motion/funcs/animal_motion/author.py'))
fbx = directory / 'SK_Archer_Practical.fbx'
# The original humanoid FBX keeps its centre offset on the armature and mesh
# objects. Bake those transforms on disposable copies before the R3 exporter,
# whose input contract is identity object transforms in metre coordinates.
original_name = rig.name
rig.name = 'OriginalArtRig'
export_rig = rig.copy()
export_rig.data = rig.data.copy()
bpy.context.collection.objects.link(export_rig)
export_rig.name = 'Armature'
export_rig.data.transform(rig.matrix_world)
export_rig.parent = None
export_rig.matrix_world = Matrix.Identity(4)
export_meshes = []
try:
    for obj in meshes:
        copy = obj.copy()
        copy.data = obj.data.copy()
        bpy.context.collection.objects.link(copy)
        copy.data.transform(obj.matrix_world)
        copy.parent = None
        copy.matrix_world = Matrix.Identity(4)
        for modifier in copy.modifiers:
            if modifier.type == 'ARMATURE':
                modifier.object = export_rig
        export_meshes.append(copy)
    exporter['export_fbx'](export_rig, export_meshes, fbx, geometry=True, animation=False)
finally:
    for obj in export_meshes:
        data = obj.data
        bpy.data.objects.remove(obj, do_unlink=True)
        bpy.data.meshes.remove(data)
    data = export_rig.data
    bpy.data.objects.remove(export_rig, do_unlink=True)
    bpy.data.armatures.remove(data)
    rig.name = original_name
assert before == [(b.name, list(b.head_local), list(b.tail_local), b.parent.name if b.parent else None)
                  for b in rig.data.bones]
write_task_meta(directory, {'game_id': 'Hearthward', 'task_kind': '3d_object',
                           'task_id': 'TASK-095-archer', 'run_id': 'repair-20261008',
                           'artifact_key': 'model_path', 'model_path': str(fbx),
                           'units': 'centimetres', 'source_route': 'existing guard derivative and original archery kit'})
report.update(fbx=str(fbx), bone_rest_and_hierarchy_unchanged=True,
              source_rights='Inherited Tripo source rights remain pending; archery kit and sling authored in project')
qa = ROOT / 'docs/qa/TASK-095/archer-repair'
qa.mkdir(parents=True, exist_ok=True)
(qa / 'source-export.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
print('ARCHER_EXPORTED', json.dumps(report))
