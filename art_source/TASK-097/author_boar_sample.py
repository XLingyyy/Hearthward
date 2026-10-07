"""Approved boar derivative; preserves the established R3 armature and actions."""
from pathlib import Path
import bpy,json,math,sys
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT.parent))
from pipeline.common.paths import task_output_dir,write_task_meta
source=ROOT/'art_source/TASK-051/animal_motion/Hearthward/animal_motion_20260930/assets/motion/pig/AS_pig.blend'
bpy.ops.wm.open_mainfile(filepath=str(source))
rig=next(o for o in bpy.data.objects if o.type=='ARMATURE')
body=next(o for o in bpy.data.objects if o.type=='MESH')
original_bones=[(b.name,list(b.head_local),list(b.tail_local),b.parent.name if b.parent else None) for b in rig.data.bones]
rig.animation_data_clear();rig.data.pose_position='REST'
for v in body.data.vertices:
 weight=max(0,min(1,(v.co.x-.47)/.27))
 v.co.x+=.15*weight
 v.co.y*=1-.12*weight
body.name='SK_Boar_Practical'
for mat in body.data.materials:
 mat.name='M_Boar_Coat';mat.use_nodes=True
 bsdf=next(n for n in mat.node_tree.nodes if n.type=='BSDF_PRINCIPLED')
 socket=bsdf.inputs['Base Color'];links=list(socket.links)
 if links:
  output=links[0].from_socket;mat.node_tree.links.remove(links[0])
  mix=mat.node_tree.nodes.new('ShaderNodeMixRGB');mix.blend_type='MULTIPLY';mix.inputs[0].default_value=1;mix.inputs[2].default_value=(.20,.115,.065,1)
  mat.node_tree.links.new(output,mix.inputs[1]);mat.node_tree.links.new(mix.outputs[0],socket)
 else: socket.default_value=(.085,.038,.016,1)
 bsdf.inputs['Roughness'].default_value=.92

def material(name,color):
 m=bpy.data.materials.new(name);m.diffuse_color=(*color,1);m.use_nodes=True;n=m.node_tree.nodes.get('Principled BSDF');n.inputs['Base Color'].default_value=(*color,1);n.inputs['Roughness'].default_value=.85;return m
ivory=material('M_Boar_Tusk',(.65,.52,.31));hair=material('M_Boar_Bristle',(.027,.019,.012))
parts=[body]
def bind(obj,bone):
 obj.vertex_groups.new(name=bone).add(list(range(len(obj.data.vertices))),1,'REPLACE')
 mod=obj.modifiers.new('R3_Armature','ARMATURE');mod.object=rig;obj.parent=rig;parts.append(obj)
for side in [-1,1]:
 curve=bpy.data.curves.new('Tusk','CURVE');curve.dimensions='3D';curve.resolution_u=8;curve.bevel_depth=.025;curve.bevel_resolution=2
 spline=curve.splines.new('BEZIER');spline.bezier_points.add(3)
 for point,co,radius in zip(spline.bezier_points,[(.77,side*.14,.62),(.83,side*.20,.64),(.86,side*.23,.71),(.84,side*.24,.79)],[1,.9,.6,.02]):
  point.co=co;point.radius=radius;point.handle_left_type='AUTO';point.handle_right_type='AUTO'
 obj=bpy.data.objects.new('Tusk',curve);bpy.context.collection.objects.link(obj);obj.data.materials.append(ivory)
 bpy.ops.object.select_all(action='DESELECT');obj.select_set(True);bpy.context.view_layer.objects.active=obj;bpy.ops.object.convert(target='MESH');bind(bpy.context.object,'Head_1')
for index in range(52):
 x=-.46+index*.016
 for lane in range(7):
  y=(lane-3)*.012+(index%2)*.004
  surface=[v.co.z for v in body.data.vertices if abs(v.co.x-x)<.035 and abs(v.co.y-y)<.04]
  if not surface: continue
  z=max(surface)-.008
  length=.045+.034*math.sin((index+lane)*1.7)**2
  bpy.ops.mesh.primitive_cone_add(vertices=3,radius1=.0035,radius2=0,depth=length,location=(x,y,z+length*.35),rotation=(0,-.5,0))
  obj=bpy.context.object;obj.name='Bristle';obj.data.materials.append(hair)
  bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
  bone=min([b for b in rig.data.bones if b.name.startswith('Spine_')],key=lambda b:(b.head_local-Vector((x,y,z))).length).name;bind(obj,bone)
bpy.ops.object.select_all(action='DESELECT')
for obj in parts:obj.select_set(True)
bpy.context.view_layer.objects.active=body;bpy.ops.object.join()
assert original_bones==[(b.name,list(b.head_local),list(b.tail_local),b.parent.name if b.parent else None) for b in rig.data.bones]
scene=bpy.context.scene;scene.world=bpy.data.worlds.new('BoarReviewWorld');scene.frame_set(1);scene.render.engine='CYCLES';scene.cycles.device='CPU';scene.cycles.samples=16
scene.render.resolution_x=1100;scene.render.resolution_y=780;scene.render.resolution_percentage=100;scene.world.color=(.18,.18,.18);scene.view_settings.view_transform='AgX'
qa=ROOT/'docs/qa/TASK-097/samples';qa.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.save_as_mainfile(filepath=str(Path(__file__).parent/'Boar.blend'))
# Keep the editable original material graph. FBX delivery requires a baked coat before UE import.
for name,loc,energy,size in [('Key',(3,-4,5),750,4),('Fill',(-2,3,3),500,3)]:
 bpy.ops.object.light_add(type='AREA',location=loc);o=bpy.context.object;o.data.energy=energy;o.data.size=size;o.rotation_euler=(Vector((0,0,.5))-o.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add();camera=bpy.context.object;scene.camera=camera;camera.data.type='ORTHO';camera.data.ortho_scale=2.5
for name,loc in [('front',(3,-4,2.4)),('side',(0,-4,1.6)),('back',(-3,3,2.4))]:
 camera.location=loc;camera.rotation_euler=(Vector((.08,0,.52))-camera.location).to_track_quat('-Z','Y').to_euler();scene.render.filepath=str(qa/('Boar-'+name+'.png'));bpy.ops.render.render(write_still=True)
(qa/'boar-source.json').write_text(json.dumps({'source':str(source.relative_to(ROOT)),'bone_count':len(rig.data.bones),'bone_rest_and_hierarchy_unchanged':True,'actions_preserved':[a.name for a in bpy.data.actions],'changes':['snout extension','two Head_1 weighted tusks','spine weighted mane','non-destructive coat material tint'],'ue_import':'NOT_RUN','motion_review':'NOT_RUN','owner_visual':'NOT_RUN'},ensure_ascii=False,indent=2),encoding='utf-8')
print('BOAR_SOURCE_SAMPLE_COMPLETE')
