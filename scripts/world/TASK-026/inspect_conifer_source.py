"""Inspect a TASK-004 Poly Haven conifer in portable Blender without editing it."""
from pathlib import Path
import json
import sys

import bpy


root = Path(__file__).resolve().parents[3]
name = sys.argv[sys.argv.index('--') + 1]
assert name in ('fir_tree_01', 'pine_tree_01')
folder = '冷杉树' if name == 'fir_tree_01' else '松树'
source = root / 'art_source/TASK-004/polyhaven/树木' / folder / f'{name}_4k.blend'
bpy.ops.wm.open_mainfile(filepath=str(source))
report = {
    'source': source.relative_to(root).as_posix(),
    'objects': [
        {
            'name': obj.name,
            'dimensions_m': [round(v, 3) for v in obj.dimensions],
            'location_m': [round(v, 3) for v in obj.location],
            'vertices': len(obj.data.vertices),
            'polygons': len(obj.data.polygons),
            'materials': [slot.name for slot in obj.material_slots],
            'uv_layers': [layer.name for layer in obj.data.uv_layers],
        }
        for obj in bpy.context.scene.objects if obj.type == 'MESH'
    ],
    'twig_material': {},
}
depsgraph = bpy.context.evaluated_depsgraph_get()
report['lod_uvs'] = {}
for variant in ('a', 'c'):
    for lod in (1, 2):
        obj = bpy.data.objects.get(f'{name}_{variant}_LOD{lod}')
        if not obj:
            continue
        evaluated = obj.evaluated_get(depsgraph)
        mesh = evaluated.to_mesh()
        report['lod_uvs'][obj.name] = {
            'modifiers': [[mod.name, mod.type] for mod in obj.modifiers],
            'source_uv_layers': [uv.name for uv in obj.data.uv_layers],
            'source_attributes': [[attr.name, attr.data_type, attr.domain]
                                  for attr in obj.data.attributes],
            'evaluated_uv_layers': [uv.name for uv in mesh.uv_layers],
            'evaluated_attributes': [[attr.name, attr.data_type, attr.domain]
                                     for attr in mesh.attributes],
        }
        evaluated.to_mesh_clear()
material = bpy.data.materials.get(f'{name}_twig')
if material and material.use_nodes:
    report['twig_material'] = {
        'nodes': [{'name': node.name, 'type': node.bl_idname,
                   'image': node.image.name if node.bl_idname == 'ShaderNodeTexImage'
                   and node.image else None} for node in material.node_tree.nodes],
        'links': [[link.from_node.name, link.from_socket.name,
                   link.to_node.name, link.to_socket.name]
                  for link in material.node_tree.links],
        'attribute_names': [node.attribute_name for node in material.node_tree.nodes
                            if node.bl_idname == 'ShaderNodeAttribute'],
    }
out = root / f'docs/qa/evidence/TASK-026/rework-v2/{name}-source.json'
out.parent.mkdir(parents=True, exist_ok=True)
out.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
print(json.dumps({'source': report['source'], 'objects': len(report['objects']),
                  'lod_uvs': report['lod_uvs']}, ensure_ascii=False))
