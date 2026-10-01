"""Current-file FBX roundtrip, expanded geometry samples and baked-pose QA."""
import bpy,json,sys,hashlib,math
from pathlib import Path
import numpy as np
from mathutils import Vector
ROOT=Path(r'E:\AiAgent\XLingGame\Resource\Tripo\动物\动作\制作成果')
REV=ROOT/'corrections_20261001_r3'
sys.path.insert(0,str(REV/'scripts'))
sys.path.insert(0,r'E:\AiAgent\XLingGame\GameFactory-3A\operators\gen_motion\funcs\animal_motion')
from audit_refinement import points,angle,difference
from author import curves

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def save(p,v):p.write_text(json.dumps(v,ensure_ascii=False,indent=2),encoding='utf-8')
args=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
jobs=json.loads((ROOT/'jobs.json').read_text('utf-8'));summaries=[]
for job in jobs:
 slug=job['slug']
 if args and slug not in args:continue
 out=Path(job['output']);man=json.loads((out/'animation_manifest.json').read_text('utf-8'));rig=man['rig']
 bpy.ops.wm.read_factory_settings(use_empty=True)
 sk=out/f'SK_{slug}.fbx';bpy.ops.import_scene.fbx(filepath=str(sk),use_image_search=False)
 arm=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE');meshes=[o for o in bpy.context.scene.objects if o.type=='MESH']
 # Blender automatically marks aligned FBX joints as connected. Connected
 # edit bones suppress their animated translations, even though those
 # translations are valid FBX tracks and are honored by Unreal. Match the
 # editable source's disconnected pose joints without changing any rest.
 connected=[b.name for b in arm.data.bones if b.use_connect]
 bpy.context.view_layer.objects.active=arm;arm.select_set(True);bpy.ops.object.mode_set(mode='EDIT')
 for b in arm.data.edit_bones:b.use_connect=False
 bpy.ops.object.mode_set(mode='OBJECT')
 ref={b.name:arm.matrix_world@b.matrix_local for b in arm.data.bones};names=set(ref);base=points(meshes)
 edges=[];offset=0
 for m in meshes:
  a=np.empty(len(m.data.edges)*2,dtype=np.int32);m.data.edges.foreach_get('vertices',a);edges.append(a.reshape(-1,2)+offset);offset+=len(m.data.vertices)
 edges=np.concatenate(edges);lengths=np.linalg.norm(base[edges[:,0]]-base[edges[:,1]],axis=1);valid=lengths>.0003;edges=edges[valid];lengths=lengths[valid]
 report={'schema':'animal.motion.fbx.qa.v2','slug':slug,'skeletal_file':str(sk),'skeletal_sha256':sha(sk),'source_sha256':job['source_sha256'],'bone_count':len(names),'bone_name_match':names==set(rig['bone_heads']),'skin_vertices':len(base),'clips':[],'geometry_sampling':'every frame of locomotion, transitions, rest, rear, scratch, preen, swipe and collapse; five times otherwise'}
 report['blender_auto_connected_joints_released']=connected
 report['pose_translation_policy']='valid animated child translations; same as editable source and native Unreal compressed pose'
 for c in man['clips']:
  assert sha(Path(c['file']))==c['sha256'],c['name']+' changed after manifest'
  scene=bpy.context.scene;scene.render.fps=c['fps'];before=set(bpy.data.objects)
  bpy.ops.import_scene.fbx(filepath=c['file'],use_image_search=False)
  created=set(bpy.data.objects)-before;imp=next(o for o in created if o.type=='ARMATURE');act=imp.animation_data.action
  same=set(b.name for b in imp.data.bones)==names
  rest_error=max(max(abs(ref[b.name][i][j]-(imp.matrix_world@b.matrix_local)[i][j]) for i in range(4) for j in range(4)) for b in imp.data.bones) if same else 1e9
  arm.animation_data_create();arm.animation_data.action=act
  if imp.animation_data.action_slot:arm.animation_data.action_slot=imp.animation_data.action_slot
  for o in created:bpy.data.objects.remove(o,do_unlink=True)
  lo,hi=act.frame_range;n=round(hi-lo)+1;poses=[];sliding=[];runs={k:[] for k in c.get('contact_offsets',{})};strain=[];ground=[];max_delta=0;scale_error=0
  critical=c['kind'] in ('walk','hop','run','trot','start','stop','pounce','lie','rest','up','rear','preen','swipe','scratch','scratch_peck','collapse')
  samples=set(range(n)) if critical else {round((n-1)*x) for x in (0,.25,.5,.75,1)}
  is_ground=job['rig_type'] not in ('aquatic','serpentine') or any(x in c['suffix'] for x in ('Land','Settle','Display'))
  for i in range(n):
   scene.frame_set(round(lo)+i);bpy.context.view_layer.update()
   pose={p.name:((arm.matrix_world@p.matrix).translation.copy(),(arm.matrix_world@p.matrix).to_quaternion()) for p in arm.pose.bones};poses.append(pose)
   scale_error=max(scale_error,max(abs(x-1) for p in arm.pose.bones for x in p.scale))
   if i:max_delta=max(max_delta,max(angle(poses[-2][name][1],pose[name][1]) for name in names))
   for key,offset in c.get('contact_offsets',{}).items():
    if not c['loop']:continue
    phase=(i/(n-1)*c.get('gait_cycles',1)+offset)%1;seq=runs[key]
    if phase<c['contact_duty']:
     if seq and phase<seq[-1][0]:sliding.append(max((v[1]-seq[0][1]).length for v in seq));seq.clear()
     ns=rig['chains'][key];eff=ns[3] if len(ns)>3 else ns[-1]
     seq.append((phase,pose[eff][0]+Vector((c['reference_speed_cm_s']/100*i/c['fps'],0,0))))
    elif seq:sliding.append(max((v[1]-seq[0][1]).length for v in seq));seq.clear()
   if i in samples:
    pts=points(meshes);ratios=np.linalg.norm(pts[edges[:,0]]-pts[edges[:,1]],axis=1)/lengths;strain.append(float(np.percentile(ratios,99)))
    if is_ground:ground.append(float(pts[:,2].min()))
  for seq in runs.values():
   if seq:sliding.append(max((v[1]-seq[0][1]).length for v in seq))
  animated=[name for name in names if any((p[name][0]-poses[0][name][0]).length>.0001 or angle(p[name][1],poses[0][name][1])>.05 for p in poses[1:])]
  drift=max((p['root'][0]-poses[0]['root'][0]).length for p in poses);rotation=max(angle(p['root'][1],poses[0]['root'][1]) for p in poses)
  q={'name':c['name'],'file_sha256':c['sha256'],'imported_bone_names_match':same,'rest_matrix_max_error':rest_error,'curve_count':len(curves(act)),'duration_s':(hi-lo)/c['fps'],'expected_duration_s':c['duration'],'pose_animated':bool(animated),'animated_bones':sorted(animated),'root_translation_drift_m':drift,'root_rotation_drift_deg':rotation,'max_scale_error':scale_error,'max_adjacent_world_rotation_deg':max_delta,'max_stance_sliding_m':max(sliding) if sliding else None,'ground_min_z_m':min(ground) if ground else None,'skin_edge_stretch_p99':max(strain),'skin_edge_stretch_samples':strain,'sampled_mesh_frames':len(samples)}
  if c['loop']:
   seam=difference(poses[0],poses[-1]);q['loop_pose_error_deg']=seam['rotation_deg'];q['loop_position_error_m']=seam['position_cm']/100
  if c['hold'] and c['kind'] in ('corpse','fish'):q['fixed_hold_max_position_drift_m']=max(difference(poses[0],p)['position_cm']/100 for p in poses)
  q['structural_pass']=bool(same and rest_error<.0001 and q['curve_count']>0 and abs(q['duration_s']-c['duration'])<.001 and (animated or c['hold']) and drift<.001 and rotation<.1 and scale_error<.0001 and (not c['loop'] or q['loop_pose_error_deg']<.1 and q['loop_position_error_m']<.001))
  flags=[]
  if q['max_stance_sliding_m'] is not None and q['max_stance_sliding_m']>.02:flags.append('stance sliding exceeds 2 cm')
  if ground and min(ground)<-.005:flags.append('visible ground penetration')
  if max(strain)>1.8:flags.append('p99 skin edge strain exceeds 1.8')
  if max_delta>60:flags.append('large adjacent world-space joint rotation')
  q['review_flags']=flags;report['clips'].append(q);print('ROUNDTRIP',slug,c['suffix'],q['structural_pass'],flags,flush=True)
  arm.animation_data.action=None;bpy.data.actions.remove(act)
 report['structural_pass']=report['bone_name_match'] and all(c['structural_pass'] for c in report['clips'])
 save(out/'fbx_roundtrip_qa.json',report);summaries.append({'slug':slug,'structural_pass':report['structural_pass'],'clips':len(report['clips']),'flags':{c['name']:c['review_flags'] for c in report['clips'] if c['review_flags']}});save(ROOT/'roundtrip_summary.json',summaries)
