"""Articulate the original longbow without changing its baked material or grip."""
import bpy,bmesh,math,json,sys
from pathlib import Path
from mathutils import Matrix,Vector
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT.parent))
from pipeline.common.paths import task_output_dir
assert Path(bpy.data.filepath).stem=='ArcheryKit'
bow=bpy.data.objects['SM_Longbow_Practical']
for obj in list(bpy.data.objects):
 if obj!=bow:bpy.data.objects.remove(obj,do_unlink=True)
bow.matrix_world=Matrix.Identity(4);bow.name='SK_PlayerBow';bow.hide_set(False);bow.hide_render=False
bm=bmesh.new();bm.from_mesh(bow.data)
string=[v for v in bm.verts if abs(v.co.x+.15)<.0011 and abs(v.co.y)<.0011]
assert len(string)==12,len(string)
uv=bm.loops.layers.uv.active;string_uv=next(l[uv].uv.copy() for v in string for l in v.link_loops)
bmesh.ops.delete(bm,geom=string,context='VERTS');bm.to_mesh(bow.data);bm.free()
bpy.ops.object.armature_add();rig=bpy.context.object;rig.name='Armature'
bpy.ops.object.mode_set(mode='EDIT');root=rig.data.edit_bones[0];root.name='BowGrip';root.head=(0,0,0);root.tail=(0,0,.05)
for name,point in [('BowUpper',(-.15,0,.75)),('BowLower',(-.15,0,-.75)),('BowNock',(-.15,0,.033))]:
 b=rig.data.edit_bones.new(name);b.head=point;b.tail=Vector(point)+Vector((0,0,.05));b.parent=root
bpy.ops.object.mode_set(mode='OBJECT')
bow.vertex_groups.clear()
for n in ['BowGrip','BowUpper','BowLower']:bow.vertex_groups.new(name=n)
for v in bow.data.vertices:
 weight=min(1,(abs(v.co.z)/.75)**2)
 bow.vertex_groups['BowUpper' if v.co.z>=0 else 'BowLower'].add([v.index],weight,'REPLACE')
 bow.vertex_groups['BowGrip'].add([v.index],1-weight,'REPLACE')
verts=[];faces=[];weights=[]
for sign,tip in [(-1,'BowLower'),(1,'BowUpper')]:
 start=len(verts)
 for i in range(17):
  t=i/16
  for j in range(6):
   verts.append((-.15+math.cos(j*math.tau/6)*.0009,math.sin(j*math.tau/6)*.0009,sign*.75*t+.033*(1-t)));weights.append((tip,t))
  if i:
   for j in range(6):
    a=start+(i-1)*6+j;b=start+(i-1)*6+(j+1)%6;faces.append((a,b,b+6,a+6))
data=bpy.data.meshes.new('DrawString');data.from_pydata(verts,[],faces);data.update()
string=bpy.data.objects.new('DrawString',data);bpy.context.collection.objects.link(string)
data.materials.append(bow.data.materials[0]);uv=data.uv_layers.new()
for l in uv.data:l.uv=string_uv
for n in ['BowUpper','BowLower','BowNock']:string.vertex_groups.new(name=n)
for i,(name,t) in enumerate(weights):
 string.vertex_groups[name].add([i],t,'REPLACE');string.vertex_groups['BowNock'].add([i],1-t,'REPLACE')
bpy.ops.object.select_all(action='DESELECT');bow.select_set(True);string.select_set(True);bpy.context.view_layer.objects.active=bow;bpy.ops.object.join()
mod=bow.modifiers.new('Bow articulation','ARMATURE');mod.object=rig
bow.parent=rig
rig.animation_data_create();action=bpy.data.actions.new('A_PlayerBow_Draw');action.use_fake_user=True;rig.animation_data.action=action
for i in range(61):
 t=i/60;draw=t*t*(3-2*t)
 for n,sign in [('BowUpper',1),('BowLower',-1)]:
  p=rig.pose.bones[n];p.location=(-.10*draw,-sign*.035*draw,0);p.keyframe_insert('location',frame=i+1)
 p=rig.pose.bones['BowNock'];p.location=(-.35*draw,0,0);p.keyframe_insert('location',frame=i+1)
scene=bpy.context.scene;scene.render.fps=60;scene.render.fps_base=1;scene.frame_start=1;scene.frame_end=61;scene.frame_set(1)
out=task_output_dir('Hearthward','3d_object','TASK-095-player-bow','ranged-20261008');out.mkdir(parents=True,exist_ok=True)
# Apply centimetres to both bind geometry and authored translations; runtime root scale is one.
bow.data.transform(Matrix.Scale(100,4));rig.data.transform(Matrix.Scale(100,4))
for slot in action.slots:
 for layer in action.layers:
  for strip in layer.strips:
   bag=strip.channelbag(slot)
   if bag:
    for fc in bag.fcurves:
     if fc.data_path.endswith('.location'):
      for k in fc.keyframe_points:k.co.y*=100;k.handle_left.y*=100;k.handle_right.y*=100
scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=.01;scene.frame_set(1)
bpy.ops.object.select_all(action='DESELECT');rig.select_set(True);bow.select_set(True);bpy.context.view_layer.objects.active=rig
kwargs=dict(use_selection=True,add_leaf_bones=False,use_armature_deform_only=False,axis_forward='-Z',axis_up='Y',apply_scale_options='FBX_SCALE_ALL',bake_anim_use_all_bones=True,bake_anim_use_nla_strips=False,bake_anim_use_all_actions=False,bake_anim_force_startend_keying=True,bake_anim_step=1,bake_anim_simplify_factor=0)
bpy.ops.export_scene.fbx(filepath=str(out/'SK_PlayerBow.fbx'),object_types={'ARMATURE','MESH'},bake_anim=False,**kwargs)
bpy.ops.export_scene.fbx(filepath=str(out/'A_PlayerBow_Draw.fbx'),object_types={'ARMATURE'},bake_anim=True,**kwargs)
bpy.data.orphans_purge(do_local_ids=True,do_linked_ids=False,do_recursive=True)
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'art_source/TASK-095/PlayerBow.blend'))
report={'bones':len(rig.data.bones),'seconds':1,'brace_cm':15,'additional_draw_cm':35,'tip_shift_cm':10,'reference_root_scale':1,'material':'Existing original M_Longbow_Practical','output':str(out)}
(out/'author.json').write_text(json.dumps(report,indent=2),encoding='utf-8');print('PLAYER_BOW',json.dumps(report))
