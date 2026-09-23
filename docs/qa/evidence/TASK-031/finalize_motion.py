"""Restore FBX bone units after IK export; derive gameplay action segments."""
import unreal,json,traceback,math
from pathlib import Path
out=Path(unreal.Paths.project_saved_dir())/'HeroValidation/motion-final5.json'
report={'ok':False};tools=unreal.AssetToolsHelpers.get_asset_tools();fps=24

def mul(a,b):
 x,y,z,w=a.x,a.y,a.z,a.w;X,Y,Z,W=b.x,b.y,b.z,b.w
 return unreal.Quat(w*X+x*W+y*Z-z*Y,w*Y-x*Z+y*W+z*X,w*Z+x*Y-y*X+z*W,w*W-x*X-y*Y-z*Z)
def points(pose,parents):
 xyz={};rot={};scales={}
 for bone,t in pose.items():
  p=parents[bone];q=rot.get(p,unreal.Quat(0,0,0,1));s=scales.get(p,unreal.Vector(1,1,1));v=t.translation
  v=mul(mul(q,unreal.Quat(v.x*s.x,v.y*s.y,v.z*s.z,0)),unreal.Quat(-q.x,-q.y,-q.z,q.w));o=xyz.get(p,unreal.Vector())
  xyz[bone]=unreal.Vector(o.x+v.x,o.y+v.y,o.z+v.z);rot[bone]=mul(q,t.rotation);scales[bone]=unreal.Vector(s.x*t.scale3d.x,s.y*t.scale3d.y,s.z*t.scale3d.z)
 return xyz

def publish(path,skeleton,poses,duration):
 seq=unreal.load_asset(path)
 if not seq:
  # Duplicate an imported 24 fps clip to retain its platform compression sampling rate.
  prefix=path.rsplit('_',1)[0];template=prefix+('_Chop' if path.endswith('_Attack') else '_Jump')
  seq=unreal.EditorAssetLibrary.duplicate_asset(template,path);assert seq
 c=seq.get_editor_property('controller');c.open_bracket('TASK-031 imported motion adaptation',False);c.remove_all_bone_tracks(False)
 c.set_frame_rate(unreal.FrameRate(120,1),False);c.set_frame_rate(unreal.FrameRate(fps,1),False);c.set_number_of_frames(unreal.FrameNumber(len(poses)-1),False)
 for bone in poses[0]:
  values=[p[bone] for p in poses];c.add_bone_curve(bone,False);assert c.set_bone_track_keys(bone,[v.translation for v in values],[v.rotation for v in values],[v.scale3d for v in values],False)
 c.close_bracket(False);seq.set_editor_property('enable_root_motion',False);seq.set_editor_property('force_root_lock',False);assert unreal.EditorAssetLibrary.save_loaded_asset(seq)
 return seq

def sample(seq,bones,t):return {b:unreal.AnimationLibrary.get_bone_pose_for_time(seq,b,t,False) for b in bones}

def run():
 for character in ['Hero','Brother']:
  folder='/Game/Characters/'+character+('/AnimationV2' if character=='Hero' else '/Animation')
  mesh=unreal.load_asset('/Game/Characters/'+character+'/UE5/SK_'+character);skel=mesh.get_editor_property('skeleton');reader=unreal.SkeletonModifier();assert reader.set_skeletal_mesh(mesh)
  bones=[str(x) for x in reader.get_all_bone_names()];parents={b:str(reader.get_parent_name(b)) for b in bones};ref={b:reader.get_bone_transform(b,False) for b in bones}
  if character=='Hero':
   for name in ['Idle','Walk','Run','Dig','Chop','Wait','Jump','Climb']:
    path=folder+'/A_Hero_'+name;seq=unreal.load_asset(folder+'/Retarget/Baked/A_Baked_'+name);length=seq.get_play_length();n=round(length*fps)
    if unreal.AnimationLibrary.get_bone_pose_for_time(seq,'root',0.,False).scale3d.x<2:
     poses=[sample(seq,bones,length*i/n) for i in range(n+1)]
     for pose in poses:
      for b,t in pose.items():
       s=reader.get_bone_transform(parents[b],True).scale3d if parents[b]!='None' else unreal.Vector(1,1,1);v=t.translation
       t.translation=unreal.Vector(v.x/s.x,v.y/s.y,v.z/s.z);t.scale3d=ref[b].scale3d;pose[b]=t
     publish(path,skel,poses,length)
  chop=unreal.load_asset(folder+'/A_'+character+'_Chop');length=chop.get_play_length();n=round(length*fps)
  curve=[points(sample(chop,bones,length*i/n),parents)['hand_r'].z for i in range(n+1)]
  peak=max(range(1,n//2),key=lambda i:curve[i]);end=min(range(peak+1,min(n,peak+fps*2)+1),key=lambda i:curve[i]);start=max(0,peak-fps//2)
  count=round(.9*fps)
  attack=[sample(chop,bones,length*(start+(end-start)*i/count)/n) for i in range(count+1)]
  publish(folder+'/A_'+character+'_Attack',skel,attack,.9)
  report[character]={'attack_source_window':[length*start/n,length*end/n],'hand_z_range':[min(curve),max(curve)]}
  if character=='Hero':
   jump=unreal.load_asset(folder+'/A_Hero_Jump');length=jump.get_play_length();n=round(length*fps);poses=[sample(jump,bones,length*i/n) for i in range(n+1)];hips=[points(p,parents)['pelvis'].z for p in poses]
   apex=max(range(n+1),key=lambda i:hips[i]);rest=points(ref,parents)['pelvis'].z
   report['jump']={'apex_time':length*apex/n,'hip_z_range':[min(hips),max(hips)],'reference_hip_z':rest}
   for name,duration,lo,hi in [('JumpStart',.3,max(0,apex-fps//3),apex),('Fall',.4,apex,apex),('Land',.2,min(n,apex+fps//3),n)]:
    count=round(duration*fps);keys=[]
    for i in range(count+1):
     pose=sample(jump,bones,length*(lo+(hi-lo)*i/count)/n)
     if name!='Land':
      delta=points(pose,parents)['pelvis'].z-rest;root=pose['root'];v=root.translation;root.translation=unreal.Vector(v.x,v.y,v.z-delta);pose['root']=root
     keys.append(pose)
    publish(folder+'/A_Hero_'+name,skel,keys,duration)
 report['ok']=True
try:run()
except Exception:report['error']=traceback.format_exc()
out.write_text(json.dumps(report,indent=2),encoding='utf-8');unreal.SystemLibrary.quit_editor()
