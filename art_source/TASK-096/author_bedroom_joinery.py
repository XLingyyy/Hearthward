"""Stonehold room joinery fitted to existing 12 x 10 m playable shell."""
from pathlib import Path
import bpy,json,math,sys
from mathutils import Matrix
ROOT=Path('G:/GameFactory/Hearthward')
sys.path.insert(0,str(ROOT.parent))
from pipeline.common.paths import task_output_dir
out=task_output_dir('Hearthward','3d_object','TASK-096-interior',run_id='20261009',create=True)
previous=[s for s in bpy.data.scenes if s.name.startswith('TASK096_BedroomJoinery')]
scene=bpy.data.scenes.new('TASK096_BedroomJoinery');bpy.context.window.scene=scene
for old in previous:
 for obj in list(old.objects):bpy.data.objects.remove(obj,do_unlink=True)
 bpy.data.scenes.remove(old)
scene.name='TASK096_BedroomJoinery'
scene.unit_settings.system='METRIC'
def material(name,color,metal=0):
 m=bpy.data.materials.get(name) or bpy.data.materials.new(name);m.use_nodes=True;m.diffuse_color=(*color,1)
 p=next(n for n in m.node_tree.nodes if n.type=='BSDF_PRINCIPLED')
 p.inputs['Base Color'].default_value=(*color,1);p.inputs['Roughness'].default_value=.87;p.inputs['Metallic'].default_value=metal
 return m
wood=material('M_JoineryWood',(.18,.10,.045));iron=material('M_JoineryIron',(.07,.075,.073),.65)
parts=[]
def box(name,loc,size,mat=wood,bevel=.012):
 bpy.ops.mesh.primitive_cube_add(size=1,location=loc);o=bpy.context.object;o.name=name;o.dimensions=size
 bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
 if bevel:
  b=o.modifiers.new('Soft worn edges','BEVEL');b.width=bevel;b.segments=2;bpy.ops.object.modifier_apply(modifier=b.name)
 o.data.materials.append(mat);parts.append(o);return o
# Base boards and wall posts sit against masonry, outside the centre route.
box('South skirting',(0,-4.96,.13),(12,.06,.26))
box('East skirting',(5.96,0,.13),(.06,10,.26))
for x in [-5.8,0,5.8]:
 box('South wall post',(x,-4.92,2.48),(.16,.12,4.96))
box('South top rail',(0,-4.9,4.92),(11.8,.15,.20))
for y in [-4.8,0,4.8]:box('East wall post',(5.9,y,2.48),(.17,.18,4.96))
# 5.6 m window opening: a timber frame and three readable bays.
for y in [-2.72,-.91,.91,2.72]:
 box('Window upright',(-6.01,y,2.90),(.16,.14,3.2))
for z in [1.36,4.43]:box('Window crosspiece',(-6.01,0,z),(.22,5.58,.16))
for y in [-1.82,0,1.82]:
 box('Lower window bar',(-6.01,y,1.82),(.10,1.65,.09))
 # Open shutters rest beside each outer pier; the centre bay remains open.
for y in [-3.30,3.30]:
 for i in range(4):box('Open shutter plank',(-5.93,y+(i-1.5)*.24,2.87),(.055,.225,2.8))
 for z in [1.70,4.02]:box('Shutter iron hinge',(-5.89,y,z),(.025,.91,.045),iron,.003)
# Low pegs and a narrow wall shelf; no new loot or interaction implied.
box('Peg rail',(5.78,-2.6,1.65),(.11,1.6,.14))
for y in [-3.15,-2.8,-2.45,-2.1]:box('Wooden peg',(5.64,y,1.65),(.22,.035,.035))
box('Wall shelf',(5.68,2.4,1.12),(.55,1.8,.055))
for y in [1.68,3.12]:box('Shelf bracket',(5.83,y,.97),(.23,.07,.30))
# Additional ceiling cross ties stay above all existing character clearances.
for y in [-3.8,3.8]:box('Ceiling cross tie',(0,y,5.12),(11.9,.20,.22))
bpy.ops.object.select_all(action='DESELECT')
for o in parts:o.select_set(True)
bpy.context.view_layer.objects.active=parts[0];bpy.ops.object.join();o=bpy.context.object;o.name='SM_BedroomJoinery'
scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.uv.smart_project(island_margin=.01);bpy.ops.object.mode_set(mode='OBJECT')
o.data.calc_loop_triangles()
# FBX -> UE reflects the Blender Y axis. This assembly uses gameplay-space
# coordinates, so compensate on export and keep the editable source unchanged.
o.data.transform(Matrix.Diagonal((1,-1,1,1)));o.data.flip_normals()
bpy.ops.export_scene.fbx(filepath=str(out/(o.name+'.fbx')),use_selection=True,object_types={'MESH'},apply_scale_options='FBX_SCALE_ALL',axis_forward='-Z',axis_up='Y',bake_anim=False,add_leaf_bones=False)
o.data.transform(Matrix.Diagonal((1,-1,1,1)));o.data.flip_normals()
(out/'meta.json').write_text(json.dumps(dict(game_id='Hearthward',run_id='20261009',task_kind='3d_object',task_id='TASK-096-interior',model_path=o.name+'.fbx')),encoding='utf-8')
o.data.calc_loop_triangles()
spec=dict(mesh=o.name,triangles=len(o.data.loop_triangles),dimensions_m=list(o.dimensions),front='-Y',export_y_reflected=True,origin='bedroom floor centre',source='Original authored geometry',collision='None; existing fortress authority unchanged')
(ROOT/'art_source/TASK-096/bedroom-joinery.json').write_text(json.dumps(spec,indent=2),encoding='utf-8')
bpy.data.libraries.write(str(ROOT/'art_source/TASK-096/BedroomJoinery.blend'),{scene},path_remap='RELATIVE',fake_user=True,compress=True)
print(json.dumps(spec))
