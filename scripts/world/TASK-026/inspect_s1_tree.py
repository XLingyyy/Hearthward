"""Read the existing island-tree source in Blender without changing TASK-004."""
from pathlib import Path
import json
import bpy

root = Path(__file__).resolve().parents[3]
source = root/'art_source/TASK-004/polyhaven/树木/岛树/island_tree_02_4k.blend'
bpy.ops.wm.open_mainfile(filepath=str(source))
report = {'source':source.relative_to(root).as_posix(), 'objects':[],
          'images':[{'name':i.name,'source':i.source,'filepath':i.filepath,'size':list(i.size)} for i in bpy.data.images]}
for o in bpy.context.scene.objects:
    if o.type != 'MESH':
        continue
    report['objects'].append({'name':o.name, 'dimensions_m':list(o.dimensions),
        'location':list(o.location), 'vertices':len(o.data.vertices),
        'polygons':len(o.data.polygons), 'materials':[s.name for s in o.material_slots],
        'modifiers':[[m.name,m.type] for m in o.modifiers], 'bounds':[list(p) for p in o.bound_box]})
out = root/'docs/qa/evidence/TASK-026/rework-v2/S1-tree-source'
out.mkdir(parents=True,exist_ok=True)
(out/'inventory.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print(json.dumps(report['images']))
