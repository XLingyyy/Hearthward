"""Bake the approved coat and reuse the established R3 centimetre FBX exporter."""
from pathlib import Path
import json
import runpy
import sys

import bpy

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT.parent))
from pipeline.common.paths import task_output_dir, write_task_meta

bpy.ops.wm.open_mainfile(filepath=str(Path(__file__).parent / 'Boar.blend'))
rig = next(obj for obj in bpy.data.objects if obj.type == 'ARMATURE')
body = bpy.data.objects['SK_Boar_Practical']
before = [(bone.name, list(bone.head_local), list(bone.tail_local),
           bone.parent.name if bone.parent else None) for bone in rig.data.bones]
out = task_output_dir('Hearthward', '3d_object', 'TASK-097-boar', 'approved-20261007')
out.mkdir(parents=True, exist_ok=True)
scene = bpy.context.scene
scene.render.engine = 'CYCLES'
scene.cycles.device = 'CPU'
scene.cycles.samples = 8
bpy.ops.object.select_all(action='DESELECT')
body.select_set(True)
bpy.context.view_layer.objects.active = body

targets = []
for mat in body.data.materials:
    size = 2048 if mat.name == 'M_Boar_Coat' else 32
    texture = bpy.data.images.new('T_' + mat.name[2:], width=size, height=size, alpha=False)
    texture.colorspace_settings.name = 'sRGB'
    node = mat.node_tree.nodes.new('ShaderNodeTexImage')
    node.image = texture
    mat.node_tree.nodes.active = node
    targets.append((mat, node, texture))

scene.render.bake.use_pass_direct = False
scene.render.bake.use_pass_indirect = False
scene.render.bake.use_pass_color = True
scene.render.bake.margin = 12
bpy.ops.object.bake(type='DIFFUSE', pass_filter={'COLOR'}, use_clear=True)
for mat, node, texture in targets:
    if mat.name == 'M_Boar_Coat':
        texture.filepath_raw = str(out / 'T_Boar_Coat.png')
        texture.file_format = 'PNG'
        texture.save()
        bsdf = next(n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
        for link in list(bsdf.inputs['Base Color'].links):
            mat.node_tree.links.remove(link)
        mat.node_tree.links.new(node.outputs['Color'], bsdf.inputs['Base Color'])
        for old in list(mat.node_tree.nodes):
            if old.type in {'TEX_IMAGE', 'MIX_RGB'} and old != node:
                mat.node_tree.nodes.remove(old)
        texture.pack()
    else:
        mat.node_tree.nodes.remove(node)
        bpy.data.images.remove(texture)

exporter = runpy.run_path(str(ROOT / 'art_source/TASK-051/tooling/GameFactory-3A/operators/gen_motion/funcs/animal_motion/author.py'))
fbx = out / 'SK_Boar_Practical.fbx'
exporter['export_fbx'](rig, [body], fbx, geometry=True, animation=False)
assert before == [(bone.name, list(bone.head_local), list(bone.tail_local),
                   bone.parent.name if bone.parent else None) for bone in rig.data.bones]
bpy.ops.wm.save_as_mainfile(filepath=str(Path(__file__).parent / 'Boar-Delivery.blend'))
write_task_meta(out, {'game_id': 'Hearthward', 'task_kind': '3d_object',
                     'task_id': 'TASK-097-boar', 'run_id': 'approved-20261007',
                     'artifact_key': 'model_path', 'model_path': str(fbx),
                     'source_route': 'approved R3 pig derivative', 'units': 'centimetres'})
report = {'method': 'Diffuse-color-only coat bake; original R3 exporter, geometry only',
          'source': str(fbx), 'coat_texture': str(out / 'T_Boar_Coat.png'),
          'bone_count': len(before), 'bone_rest_and_hierarchy_unchanged': True,
          'animation_exported': False, 'ue_import': 'NOT_RUN'}
(ROOT / 'docs/qa/TASK-097/samples/boar-export.json').write_text(
    json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
print('BOAR_DELIVERY_EXPORTED', flush=True)
