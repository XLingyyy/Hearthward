"""Export selected Poly Haven conifer LOD meshes without altering TASK-004."""
from pathlib import Path
import json
import sys

import bpy


root = Path(__file__).resolve().parents[3]
name = sys.argv[sys.argv.index('--') + 1]
assert name in ('fir_tree_01', 'pine_tree_01')
folder = '冷杉树' if name == 'fir_tree_01' else '松树'
lod = 1 if name == 'fir_tree_01' else 2
source = root / 'art_source/TASK-004/polyhaven/树木' / folder / f'{name}_4k.blend'
out = root / 'art_source/TASK-026/Rebuild/ReworkV2/conifers'
out.mkdir(parents=True, exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=str(source))

exports = []
for variant in ('a', 'c'):
    for obj in bpy.context.scene.objects:
        obj.select_set(False)
    tree = bpy.data.objects[f'{name}_{variant}_LOD{lod}']
    tree.hide_set(False)
    tree.hide_viewport = False
    tree.location = (0, 0, 0)
    tree.data = tree.data.copy()
    if tree.data.uv_layers:
        uv_source = tree.data.uv_layers[0].name
        export_uv = tree.data.uv_layers[0]
    else:
        # Fir LODs store texture coordinates as generic corner vectors;
        # FBX ignores these until converted to a standard UV layer.
        source_uv = tree.data.attributes.get('UVMap')
        assert source_uv and source_uv.data_type == 'FLOAT_VECTOR'
        assert source_uv.domain == 'CORNER' and len(source_uv.data) == len(tree.data.loops)
        export_uv = tree.data.uv_layers.new(name='CampUV')
        for index, value in enumerate(source_uv.data):
            export_uv.data[index].uv = value.vector[:2]
        uv_source = 'UVMap generic CORNER vector'
    tree.select_set(True)
    bpy.context.view_layer.objects.active = tree
    path = out / f'{name}_{variant}.fbx'
    bpy.ops.export_scene.fbx(
        filepath=str(path), use_selection=True, object_types={'MESH'},
        axis_forward='-Y', axis_up='Z', apply_unit_scale=True,
        bake_anim=False, path_mode='STRIP', use_mesh_modifiers=True,
    )
    tree.data.calc_loop_triangles()
    exports.append({
        'object': tree.name, 'fbx': path.relative_to(root).as_posix(),
        'triangles': len(tree.data.loop_triangles),
        'dimensions_m': [round(v, 4) for v in tree.dimensions],
        'materials': [slot.name for slot in tree.material_slots],
        'uv_source': uv_source, 'uv_export': export_uv.name,
    })

report = {
    'source': source.relative_to(root).as_posix(),
    'source_url': f'https://polyhaven.com/a/{name}', 'license': 'CC0',
    'source_unchanged': True, 'lod': lod, 'exports': exports,
}
(out / f'{name}.export.json').write_text(
    json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
print(json.dumps(report, ensure_ascii=False))
