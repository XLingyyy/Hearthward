"""TASK-031: material binding and native IK retargeting of the supplied clips."""
import unreal,json,traceback
from pathlib import Path
out=Path(unreal.Paths.project_saved_dir())/'HeroValidation/prepared-normalized2.json'
report={'ok':False}
tools=unreal.AssetToolsHelpers.get_asset_tools()
def asset(path,cls,factory):
 return unreal.load_asset(path) or tools.create_asset(path.rsplit('/',1)[1],path.rsplit('/',1)[0],cls,factory)
def material(character):
 dest='/Game/Characters/'+character+'/UE5';mesh=unreal.load_asset(dest+'/SK_'+character)
 assert mesh.get_editor_property('materials')[0].material_interface.get_name()=='M_'+character
 reader=unreal.SkeletonModifier();assert reader.set_skeletal_mesh(mesh);bounds=mesh.get_bounds()
 report[character]={'height':bounds.box_extent.z*2,'bones':[str(b) for b in reader.get_all_bone_names()]}
 return mesh

def normalized_mesh(mesh,label):
 path='/Game/Characters/Hero/AnimationV2/Retarget/SK_'+label+'_Proxy'
 proxy=unreal.load_asset(path) or unreal.EditorAssetLibrary.duplicate_asset(mesh.get_path_name(),path)
 reader=unreal.SkeletonModifier();assert reader.set_skeletal_mesh(mesh)
 modifier=unreal.SkeletonModifier();assert modifier.set_skeletal_mesh(proxy)
 for bone in reader.get_all_bone_names():
  t=reader.get_bone_transform(bone,False);parent=str(reader.get_parent_name(bone));scale=reader.get_bone_transform(parent,True).scale3d if parent!='None' else unreal.Vector(1,1,1);v=t.translation
  t.translation=unreal.Vector(v.x*scale.x,v.y*scale.y,v.z*scale.z);t.scale3d=unreal.Vector(1,1,1);assert modifier.set_bone_transform(bone,t,True)
 assert modifier.commit_skeleton_to_skeletal_mesh();assert unreal.EditorAssetLibrary.save_loaded_asset(proxy)
 return proxy

def normalized_clip(name,mesh):
 source=unreal.load_asset('/Game/Characters/Brother/Animation/A_Brother_'+name)
 path='/Game/Characters/Hero/AnimationV2/Retarget/A_Source_'+name
 seq=unreal.load_asset(path) or unreal.EditorAssetLibrary.duplicate_asset(source.get_path_name(),path)
 reader=unreal.SkeletonModifier();reader.set_skeletal_mesh(mesh);bones=[str(b) for b in reader.get_all_bone_names()]
 frames=unreal.AnimationLibrary.get_num_frames(source);length=source.get_play_length()
 keys={b:[unreal.AnimationLibrary.get_bone_pose_for_time(source,b,length*i/frames,False) for i in range(frames+1)] for b in bones}
 c=seq.get_editor_property('controller');c.open_bracket('Normalize FBX source units for IK',False);c.remove_all_bone_tracks(False)
 for bone in bones:
  parent=str(reader.get_parent_name(bone));scale=reader.get_bone_transform(parent,True).scale3d if parent!='None' else unreal.Vector(1,1,1)
  values=keys[bone];positions=[]
  for t in values:
   v=t.translation;positions.append(unreal.Vector(v.x*scale.x,v.y*scale.y,v.z*scale.z))
  c.add_bone_curve(bone,False);assert c.set_bone_track_keys(bone,positions,[t.rotation for t in values],[unreal.Vector(1,1,1)]*(frames+1),False)
 c.close_bracket(False);assert unreal.EditorAssetLibrary.save_loaded_asset(seq)
 return unreal.EditorAssetLibrary.find_asset_data(path)

try:
 original_source=material('Brother');original_target=material('Hero');source=normalized_mesh(original_source,'Brother');target=normalized_mesh(original_target,'Hero');dest='/Game/Characters/Hero/AnimationV2'
 chains={'Spine':('spine_01','spine_03'),'Head':('neck_01','head')}
 for side in ['l','r']:
  chains.update({key+'_'+side:(first+'_'+side,last+'_'+side) for key,first,last in [('Arm','upperarm','hand'),('Clavicle','clavicle','clavicle'),('Leg','thigh','foot'),('Toe','ball','ball')]})
  for finger in ['thumb','index','middle','ring','pinky']:chains[finger+'_'+side]=(finger+'_01_'+side,finger+'_03_'+side)
 rigs=[]
 for name,mesh in [('Brother',source),('Hero',target)]:
  rig=asset(dest+'/Retarget/IK_'+name,unreal.IKRigDefinition,unreal.IKRigDefinitionFactory());c=unreal.IKRigController.get_controller(rig);assert c.set_skeletal_mesh(mesh)
  for existing in c.get_retarget_chains():c.remove_retarget_chain(existing.chain_name)
  c.set_retarget_root('pelvis')
  for chain,(first,last) in chains.items():c.add_retarget_chain(chain,first,last,'None')
  unreal.EditorAssetLibrary.save_loaded_asset(rig);rigs.append(rig)
 retarget=asset(dest+'/Retarget/RTG_Brother_Hero',unreal.IKRetargeter,unreal.IKRetargetFactory());c=unreal.IKRetargeterController.get_controller(retarget)
 src=unreal.RetargetSourceOrTarget.SOURCE;dst=unreal.RetargetSourceOrTarget.TARGET
 c.remove_all_ops();c.set_ik_rig(src,rigs[0]);c.set_ik_rig(dst,rigs[1]);c.set_preview_mesh(src,source);c.set_preview_mesh(dst,target);c.add_default_ops()
 c.set_retarget_op_enabled(c.get_index_of_op_by_name('Root Motion'),False)
 c.auto_map_chains(unreal.AutoMapChainType.EXACT,True);c.reset_retarget_pose(c.get_current_retarget_pose_name(dst),[],dst);c.auto_align_all_bones(dst,unreal.RetargetAutoAlignMethod.CHAIN_TO_CHAIN)
 unreal.EditorAssetLibrary.save_loaded_asset(retarget)
 inputs=unreal.IKRetargetBatchOperationInputs()
 inputs.assets_to_retarget=[normalized_clip(name,original_source) for name in ['Idle','Walk','Run','Dig','Chop','Wait','Jump','Climb']]
 inputs.source_mesh=source;inputs.target_mesh=target;inputs.ik_retarget_asset=retarget;inputs.search='A_Source_';inputs.replace='A_Baked_';inputs.target_path=dest+'/Retarget/Baked';inputs.overwrite_existing_files=True;inputs.include_referenced_assets=False
 products=unreal.IKRetargetBatchOperation.run_batch_retarget(inputs);assert len(products)==8
 report['clips']=[]
 for product in products:
  clip=product.get_asset();clip.set_editor_property('enable_root_motion',False);clip.set_editor_property('force_root_lock',False);assert unreal.EditorAssetLibrary.save_loaded_asset(clip)
  report['clips'].append({'path':clip.get_path_name(),'duration':clip.get_play_length(),'root':str(unreal.AnimationLibrary.get_bone_pose_for_time(clip,'root',0.,False)),'pelvis':str(unreal.AnimationLibrary.get_bone_pose_for_time(clip,'pelvis',0.,False))})
 report['ok']=True
except Exception:report['error']=traceback.format_exc()
out.write_text(json.dumps(report,indent=2),encoding='utf-8')
unreal.SystemLibrary.quit_editor()
