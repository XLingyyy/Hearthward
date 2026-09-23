"""Adapt existing CC0 LOD1 tree to an isolated TASK-026 FBX and review views."""
from pathlib import Path
import json
import math
import bpy
from mathutils import Vector

root=Path(__file__).resolve().parents[3]
source=root/'art_source/TASK-004/polyhaven/树木/岛树/island_tree_02_4k.blend'
out=root/'art_source/TASK-026/Rebuild/ReworkV2/island_tree'
out.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=str(source))
for obj in bpy.context.scene.objects:
    obj.hide_render=True
    obj.select_set(False)
tree=bpy.data.objects['island_tree_02_LOD1']
tree.hide_set(False);tree.hide_render=False;tree.hide_viewport=False
tree.select_set(True);bpy.context.view_layer.objects.active=tree
for image in bpy.data.images:
    candidates=list((source.parent/'textures').glob(Path(image.name).stem+'_4k.*'))
    assert len(candidates)==1, image.name
    image.filepath=str(candidates[0])
    image.reload()
    assert image.size[0]>0 and image.has_data, image.name
bpy.ops.export_scene.fbx(filepath=str(out/'island_tree_02.fbx'),use_selection=True,
    object_types={'MESH'},axis_forward='-Y',axis_up='Z',apply_unit_scale=True,
    bake_anim=False,path_mode='STRIP',use_mesh_modifiers=True)
tree.data.calc_loop_triangles()
(out/'provenance.json').write_text(json.dumps({'source':source.relative_to(root).as_posix(),
    'provider':'Poly Haven','license':'CC0','source_url':'https://polyhaven.com/a/island_tree_02',
    'source_object':tree.name,'triangles':len(tree.data.loop_triangles),
    'dimensions_m':list(tree.dimensions),'up_axis':'+Z','pivot':'source trunk root',
    'source_unchanged':True,'use':'3-5m forest-edge small tree; S1 only'},indent=2),encoding='utf-8')
scene=bpy.context.scene
scene.render.engine='CYCLES'
scene.cycles.device='CPU';scene.cycles.samples=8
scene.render.resolution_x=800;scene.render.resolution_y=800;scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG'
scene.render.film_transparent=False
scene.world=bpy.data.worlds.new('S1ReviewWorld');scene.world.use_nodes=True
scene.world.node_tree.nodes['Background'].inputs['Color'].default_value=(.22,.25,.29,1)
scene.world.node_tree.nodes['Background'].inputs['Strength'].default_value=.65
light=bpy.data.lights.new('S1ReviewSun','SUN');light.energy=2
sun=bpy.data.objects.new('S1ReviewSun',light);scene.collection.objects.link(sun)
sun.rotation_euler=(math.radians(30),math.radians(-25),math.radians(-35))
camera=bpy.data.cameras.new('S1ReviewCamera');obj=bpy.data.objects.new('S1ReviewCamera',camera)
scene.collection.objects.link(obj);scene.camera=obj;camera.type='ORTHO';camera.ortho_scale=5.7
center=Vector((.05,-.8,1.7))
for name,direction in [('front',(0,-1,.12)),('back',(0,1,.12)),('side',(1,0,.12))]:
    obj.location=center+Vector(direction)*9
    obj.rotation_euler=(center-obj.location).to_track_quat('-Z','Y').to_euler()
    scene.render.filepath=str(out/(name+'.png'))
    bpy.ops.render.render(write_still=True)
