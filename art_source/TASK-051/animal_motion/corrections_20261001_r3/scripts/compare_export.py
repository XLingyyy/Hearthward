import bpy,sys,numpy as np
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from common import jobs,read
from audit_refinement import pose,angle,points
j=jobs(['black_bear'])[0];out=Path(j['output']);m=read(out/'animation_manifest.json')
bpy.ops.wm.open_mainfile(filepath=str(out/'AS_black_bear.blend'));arm=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE');meshes=[o for o in bpy.context.scene.objects if o.type=='MESH']
stored={}
for suffix,f in [('Idle',1),('Rest',30),('Lope',10)]:
 c=next(c for c in m['clips'] if c['suffix']==suffix);arm.animation_data.action=bpy.data.actions[c['name']];bpy.context.scene.frame_set(f);stored[suffix]=(pose(arm),points(meshes));print('SOURCE',suffix,f,arm.pose.bones['pelvis'].location[:],{n:(arm.pose.bones[n].location[:],arm.data.bones[n].use_local_location,arm.data.bones[n].use_inherit_rotation) for n in m['rig']['chains']['FR']},flush=True)
bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath=str(out/'SK_black_bear.fbx'),use_image_search=False);arm=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE');meshes=[o for o in bpy.context.scene.objects if o.type=='MESH']
print('CONNECTED',[b.name for b in arm.data.bones if b.use_connect],flush=True)
bpy.context.view_layer.objects.active=arm;arm.select_set(True);bpy.ops.object.mode_set(mode='EDIT')
for b in arm.data.edit_bones:b.use_connect=False
bpy.ops.object.mode_set(mode='OBJECT')
for suffix,f in [('Idle',1),('Rest',30),('Lope',10)]:
 c=next(c for c in m['clips'] if c['suffix']==suffix);bpy.context.scene.render.fps=c['fps'];before=set(bpy.data.objects);bpy.ops.import_scene.fbx(filepath=c['file'],use_image_search=False);created=set(bpy.data.objects)-before;imp=next(o for o in created if o.type=='ARMATURE');act=imp.animation_data.action;arm.animation_data_create();arm.animation_data.action=act;arm.animation_data.action_slot=imp.animation_data.action_slot
 for o in created:bpy.data.objects.remove(o,do_unlink=True)
 print('RANGE',suffix,act.frame_range[:],flush=True);bpy.context.scene.frame_set(round(act.frame_range[0])+f-1);bpy.context.view_layer.update();p=pose(arm);old,xyz=stored[suffix];print('IMPORTED',suffix,f,arm.pose.bones['pelvis'].location[:],{n:(arm.pose.bones[n].location[:],arm.data.bones[n].use_local_location,arm.data.bones[n].use_inherit_rotation) for n in m['rig']['chains']['FR']},flush=True)
 p={b.name:((arm.matrix_world@b.matrix).translation,(arm.matrix_world@b.matrix).to_quaternion()) for b in arm.pose.bones}
 diffs=sorted([((old[n][0]-p[n][0]).length*100,angle(old[n][1],p[n][1]),n) for n in p],reverse=True)
 print('DIFFERENCES',suffix,f,diffs[:10],flush=True);print('GROUND',xyz[:,2].min()*100,points(meshes)[:,2].min()*100,flush=True)
