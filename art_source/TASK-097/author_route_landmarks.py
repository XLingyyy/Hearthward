"""Original weathered route markers; metre units, grounded pivot; no resource or gameplay components."""
import bpy,math,random,json,sys
from pathlib import Path
ROOT=Path('G:/GameFactory/Hearthward');sys.path.insert(0,str(ROOT.parent))
from pipeline.common.paths import task_output_dir
OUT=task_output_dir('Hearthward','3d_object','TASK-097-route',run_id='20261009',create=True)
scene=bpy.data.scenes.new('TASK097_RouteLandmarks');bpy.context.window.scene=scene
scene.unit_settings.system='METRIC';random.seed(97);parts=[];report=[]
meta=dict(game_id='Hearthward',task_kind='3d_object',task_id='TASK-097-route',run_id='20261009')
def material(name,col,metal=0):
 m=bpy.data.materials.new('M_Route'+name);m.use_nodes=True
 p=next(n for n in m.node_tree.nodes if n.type=='BSDF_PRINCIPLED');p.inputs['Base Color'].default_value=(*col,1);p.inputs['Roughness'].default_value=.92;p.inputs['Metallic'].default_value=metal;return m
stone=material('Stone',(.32,.30,.25));wood=material('Wood',(.21,.14,.075));iron=material('Iron',(.055,.045,.033),.5)
def box(loc,size,mat):
 bpy.ops.mesh.primitive_cube_add(size=1,location=loc);o=bpy.context.object;o.dimensions=size;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
 b=o.modifiers.new('Worn edges','BEVEL');b.width=.012;b.segments=2;bpy.ops.object.modifier_apply(modifier=b.name);o.data.materials.append(mat);parts.append(o);return o
def rock(loc,size):
 bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=2,radius=1,location=loc);o=bpy.context.object
 for v in o.data.vertices:v.co*=random.uniform(.86,1.1)
 o.scale=size;o.rotation_euler.z=random.random()*math.tau;o.data.materials.append(stone);parts.append(o);return o
def save(name):
 bpy.ops.object.select_all(action='DESELECT')
 for o in parts:o.select_set(True)
 bpy.context.view_layer.objects.active=parts[0];bpy.ops.object.join();o=bpy.context.object;o.name=name
 bpy.ops.object.transform_apply(location=False,rotation=True,scale=True);scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
 o.data.calc_loop_triangles();report.append(dict(name=name,triangles=len(o.data.loop_triangles),dimensions_m=list(o.dimensions)))
 bpy.ops.export_scene.fbx(filepath=str(OUT/(name+'.fbx')),use_selection=True,object_types={'MESH'},apply_scale_options='FBX_SCALE_ALL',axis_forward='-Z',axis_up='Y',bake_anim=False,add_leaf_bones=False)
 meta[name+'_path']=name+'.fbx';o.location.x=(len(report)-1)*3;parts.clear()
for z,r,n in [(.14,.44,7),(.35,.31,5),(.55,.21,4),(.73,.13,3)]:
 for i in range(n):
  a=i*math.tau/n;rock((r*.6*math.cos(a),r*.6*math.sin(a),z),(r*.6,r*.48,.15))
rock((0,0,.94),(.16,.12,.19));save('SM_RouteCairn')
post=box((0,0,.95),(.17,.16,1.9),wood);post.rotation_euler.y=.035
# Arrow silhouette; direction follows the existing path at runtime.
coords=[(-.58,-.035,1.38),(.36,-.035,1.38),(.62,-.035,1.51),(.36,-.035,1.64),(-.58,-.035,1.64)]
verts=coords+[(x,y+.065,z) for x,y,z in coords];faces=[(4,3,2,1,0),(5,6,7,8,9)]+[(i,(i+1)%5,(i+1)%5+5,i+5) for i in range(5)]
mesh=bpy.data.meshes.new('Split oak arrow');mesh.from_pydata(verts,[],faces);mesh.update();o=bpy.data.objects.new('Split oak arrow',mesh);scene.collection.objects.link(o);o.data.materials.append(wood);parts.append(o)
for z in [1.43,1.59]:
 bpy.ops.mesh.primitive_uv_sphere_add(segments=12,ring_count=6,radius=.016,location=(0,-.042,z));o=bpy.context.object;o.data.materials.append(iron);parts.append(o)
for i in range(5):rock((random.uniform(-.18,.18),random.uniform(-.16,.16),.09),(.15,.12,.10))
save('SM_RouteSign')
for i in range(8):
 a=i*math.tau/8;rock((.34*math.cos(a),.28*math.sin(a),.14),(.27,.23,.17))
rock((-.09,0,.79),(.42,.33,.72));rock((-.05,0,1.54),(.26,.23,.38))
box((-.05,-.24,1.43),(.18,.035,.035),iron);box((-.05,-.24,1.48),(.035,.035,.13),iron)
save('SM_RouteLookout')
(OUT/'meta.json').write_text(json.dumps(meta,indent=2),encoding='utf-8')
(ROOT/'art_source/TASK-097/route-landmarks.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
bpy.data.libraries.write(str(ROOT/'art_source/TASK-097/RouteLandmarks.blend'),{scene},path_remap='RELATIVE',fake_user=True,compress=True)
print(json.dumps(report))
