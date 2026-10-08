"""Bake the existing costume sample's cloth-only mask without changing its mesh."""
from pathlib import Path
import bpy,json,sys

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT.parent))
from pipeline.common.paths import task_output_dir

role=Path(bpy.data.filepath).stem
assert role in ('Hero','Brother'),role
body=next(o for o in bpy.context.scene.objects if o.type=='MESH')
assert body.data.color_attributes.get('ClothBlend')
out=task_output_dir('Hearthward','3d_object','TASK-095-costumes','cloth-20261008')
out.mkdir(parents=True,exist_ok=True)
scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.device='CPU';scene.cycles.samples=1
scene.render.bake.margin=8
material=bpy.data.materials.new('ClothMaskBakeTemporary');material.use_nodes=True
nodes=material.node_tree.nodes;nodes.clear()
attribute=nodes.new('ShaderNodeVertexColor');attribute.layer_name='ClothBlend'
emission=nodes.new('ShaderNodeEmission');output=nodes.new('ShaderNodeOutputMaterial')
material.node_tree.links.new(attribute.outputs['Color'],emission.inputs['Color'])
material.node_tree.links.new(emission.outputs[0],output.inputs['Surface'])
mask=bpy.data.images.new('T_'+role+'_ClothMask',width=1024,height=1024,alpha=False)
mask.colorspace_settings.name='Non-Color'
target=nodes.new('ShaderNodeTexImage');target.image=mask;nodes.active=target
previous=body.data.materials[0];body.data.materials[0]=material
bpy.ops.object.select_all(action='DESELECT');body.select_set(True);bpy.context.view_layer.objects.active=body
try:
 bpy.ops.object.bake(type='EMIT')
 mask.filepath_raw=str(out/(mask.name+'.png'));mask.file_format='PNG';mask.save()
finally:
 body.data.materials[0]=previous;bpy.data.materials.remove(material)
values=body.data.color_attributes['ClothBlend'].data
report={'role':role,'source':str(ROOT/'art_source/TASK-095'/(role+'.blend')),
        'mask':mask.filepath_raw,'resolution':[1024,1024],
        'mask_min':min(v.color[0] for v in values),'mask_max':max(v.color[0] for v in values),
        'mesh_changed':False,'source_saved':False}
(out/(role+'-mask.json')).write_text(json.dumps(report,indent=2),encoding='utf-8')
print(json.dumps(report))
