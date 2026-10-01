"""Inspect true bind head orientation and current goat limb joint geometry."""
import bpy,json,sys,math,numpy as np
from pathlib import Path
from mathutils import Vector
ROOT=Path(r'E:\AiAgent\XLingGame\Resource\Tripo\动物\动作\制作成果');REV=ROOT/'corrections_20261001_r2'
sys.path.insert(0,str(REV/'scripts/reference_helpers'));sys.path.insert(0,str(ROOT/'refinement_20261001/scripts'))
from preview import setup
from refine_motion import reset,snapshot
from audit_refinement import points
results={}
for slug in ('stag_a','ram','red_fox','goat'):
 out=ROOT/'Hearthward/animal_motion_20260930/assets/motion'/slug;m=json.loads((out/'animation_manifest.json').read_text('utf-8'));rig=m['rig'];L=rig['axis']['target_length_m']
 bpy.ops.wm.open_mainfile(filepath=str(out/f'AS_{slug}.blend'));arm=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE');meshes=[o for o in bpy.context.scene.objects if o.type=='MESH'];scene=bpy.context.scene
 camera,_=setup(arm,L);scene.render.resolution_x=500;scene.render.resolution_y=400
 folder=REV/'inspection/before'/slug;folder.mkdir(parents=True,exist_ok=True)
 if slug!='goat':
  arm.animation_data.action=None;reset(arm);bpy.context.view_layer.update()
  head=arm.data.bones[rig['head']];related=set(rig['neck'])|{b.name for n in rig['neck'] for b in arm.data.bones[n].children_recursive}
  weighted=[]
  for obj in meshes:
   names={g.index:g.name for g in obj.vertex_groups}
   for v in obj.data.vertices:
    w=sum(g.weight for g in v.groups if names[g.group] in related)
    if w>.5:weighted.append(list(v.co))
  pts=np.array(weighted);h=np.array(head.head_local);subset=pts[(pts[:,2]>h[2]-.19*L)&(pts[:,2]<h[2]+.035*L)&(pts[:,0]>h[0]-.1*L)]
  nose=subset[subset[:,0]>subset[:,0].max()-.025*L].mean(axis=0);d=nose-h
  yaw=math.degrees(math.atan2(d[1],d[0]))
  groups={}
  for obj in meshes:
   for g in obj.vertex_groups:
    vs=[v.co for v in obj.data.vertices if any(a.group==g.index and a.weight>.3 for a in v.groups)]
    if vs:
     values=np.array(vs);groups[g.name]={'count':len(vs),'mean':values.mean(axis=0).tolist(),'parent':arm.data.bones[g.name].parent.name if arm.data.bones.get(g.name) and arm.data.bones[g.name].parent else None,'head':list(arm.data.bones[g.name].head_local) if arm.data.bones.get(g.name) else None}
  results[slug]={'head':head.name,'head_pivot':list(head.head_local),'muzzle_landmark':nose.tolist(),'muzzle_heading_deg':yaw,'head_vertex_count':len(pts),'head_related_bones':sorted(related),'head_mesh_bounds':[pts.min(axis=0).tolist(),pts.max(axis=0).tolist()],'skin_groups':groups}
  target=Vector((h[0],h[1],h[2]+(.08*L if slug=='stag_a' else 0)))
  views={'front':(Vector((L*1.7,0,L*.62)),Vector((0,0,L*.5))),'top':(Vector((0,0,L*2.7)),Vector((0,0,0))),'head_front':(target+Vector((L*.75,0,L*.09)),target),'head_top':(target+Vector((0,0,L*.75)),target)}
 else:
  arm.animation_data.action=bpy.data.actions['AN_goat_Rest'];scene.frame_set(1);bpy.context.view_layer.update()
  results[slug]={'rest_joint_positions':{k:[list(arm.pose.bones[n].matrix.translation) for n in ns] for k,ns in rig['chains'].items()},'rest_bone_lengths':{k:[b.length for b in (arm.data.bones[n] for n in ns)] for k,ns in rig['chains'].items()},'standing_joint_positions':{k:[list(arm.data.bones[n].head_local) for n in ns] for k,ns in rig['chains'].items()}}
  views={'front':(Vector((L*1.8,0,L*.45)),Vector((0,0,L*.24))),'side':(Vector((0,-L*1.7,L*.4)),Vector((0,0,L*.24))),'top':(Vector((0,0,L*2)),Vector((0,0,0)))}
 for name,(location,target) in views.items():
  camera.location=location;camera.rotation_euler=(target-location).to_track_quat('-Z','Y').to_euler();scene.render.filepath=str(folder/f'{name}.png');bpy.ops.render.render(write_still=True)
 print('ISSUE_PROBE',slug,flush=True)
(REV/'issue_baseline.json').write_text(json.dumps(results,indent=2),encoding='utf-8')

