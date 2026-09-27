"""Native IK retargeting; source assets remain read-only, outputs stay in Campaign."""
import json,traceback
from pathlib import Path
import unreal
root=Path(unreal.Paths.project_dir()).resolve();out=root/'Saved/Task049/retarget.json'
report={'ok':False,'clips':[]};tools=unreal.AssetToolsHelpers.get_asset_tools();dest='/Game/Hearthward/Campaign'
def asset(path,cls,factory):return unreal.load_asset(path) or tools.create_asset(path.rsplit('/',1)[1],path.rsplit('/',1)[0],cls,factory)
def normalized(mesh,path):
    proxy=unreal.load_asset(path) or unreal.EditorAssetLibrary.duplicate_asset(mesh.get_path_name(),path)
    reader=unreal.SkeletonModifier();assert reader.set_skeletal_mesh(mesh)
    writer=unreal.SkeletonModifier();assert writer.set_skeletal_mesh(proxy)
    for bone in reader.get_all_bone_names():
        t=reader.get_bone_transform(bone,False);parent=str(reader.get_parent_name(bone))
        scale=reader.get_bone_transform(parent,True).scale3d if parent!='None' else unreal.Vector(1,1,1);v=t.translation
        t.translation=unreal.Vector(v.x*scale.x,v.y*scale.y,v.z*scale.z);t.scale3d=unreal.Vector(1,1,1)
        assert writer.set_bone_transform(bone,t,True)
    assert writer.commit_skeleton_to_skeletal_mesh();assert unreal.EditorAssetLibrary.save_loaded_asset(proxy)
    return proxy
def source_clip(name,mesh):
    original=unreal.load_asset('/Game/Characters/Brother/Animation/A_Brother_'+name)
    path=dest+'/Retarget/A_Source_'+name
    seq=unreal.load_asset(path) or unreal.EditorAssetLibrary.duplicate_asset(original.get_path_name(),path)
    reader=unreal.SkeletonModifier();reader.set_skeletal_mesh(mesh);bones=reader.get_all_bone_names()
    frames=unreal.AnimationLibrary.get_num_frames(original);length=original.get_play_length()
    keys={str(b):[unreal.AnimationLibrary.get_bone_pose_for_time(original,b,length*i/frames,False) for i in range(frames+1)] for b in bones}
    c=seq.get_editor_property('controller');c.open_bracket('Normalize source FBX units',False);c.remove_all_bone_tracks(False)
    for bone in bones:
        parent=str(reader.get_parent_name(bone));scale=reader.get_bone_transform(parent,True).scale3d if parent!='None' else unreal.Vector(1,1,1)
        values=keys[str(bone)];positions=[unreal.Vector(t.translation.x*scale.x,t.translation.y*scale.y,t.translation.z*scale.z) for t in values]
        c.add_bone_curve(bone,False);assert c.set_bone_track_keys(bone,positions,[t.rotation for t in values],[unreal.Vector(1,1,1)]*(frames+1),False)
    c.close_bracket(False);assert unreal.EditorAssetLibrary.save_loaded_asset(seq)
    return unreal.EditorAssetLibrary.find_asset_data(path)
try:
    original=unreal.load_asset('/Game/Characters/Brother/UE5/SK_Brother')
    source=normalized(original,dest+'/Retarget/SK_Source')
    clips=[source_clip(name,original) for name in ['Idle','Walk','Run','Attack']]
    src_chains={'Spine':('spine_01','spine_03'),'Head':('neck_01','head'),'Arm_L':('upperarm_l','hand_l'),'Arm_R':('upperarm_r','hand_r'),'Leg_L':('thigh_l','foot_l'),'Leg_R':('thigh_r','foot_r')}
    for kind in ['Guard','Heavy']:
        target=normalized(unreal.load_asset(dest+'/'+kind+'/SK_'+kind),dest+'/'+kind+'/SK_'+kind+'_Runtime')
        if kind=='Guard':
            hip='Spine_0';chains={'Spine':('Spine_0','Spine_2'),'Head':('Head_0','Head_1'),'Arm_L':('1_Left_Limb_1','1_Left_Limb_3'),'Arm_R':('0_Right_Limb_1','0_Right_Limb_2'),'Leg_L':('1_Right_Limb_0','1_Right_Limb_2'),'Leg_R':('0_Left_Limb_0','0_Left_Limb_2')}
        else:
            hip='0_Left_Limb_0';chains={'Spine':('Spine_0','bone_5'),'Head':('Head_0','Head_0'),'Arm_L':('bone_16','bone_18'),'Arm_R':('bone_8','bone_10'),'Leg_L':('bone_24','bone_26'),'Leg_R':('0_Left_Limb_1','0_Left_Limb_3')}
        rigs=[]
        for name,mesh,retarget_root,mapping in [('Source',source,'pelvis',src_chains),(kind,target,hip,chains)]:
            rig=asset(dest+'/Retarget/IK_'+name,unreal.IKRigDefinition,unreal.IKRigDefinitionFactory());c=unreal.IKRigController.get_controller(rig);assert c.set_skeletal_mesh(mesh)
            for existing in c.get_retarget_chains():c.remove_retarget_chain(existing.chain_name)
            c.set_retarget_root(retarget_root)
            for chain,(first,last) in mapping.items():c.add_retarget_chain(chain,first,last,'None')
            unreal.EditorAssetLibrary.save_loaded_asset(rig);rigs.append(rig)
        retarget=asset(dest+'/Retarget/RTG_'+kind,unreal.IKRetargeter,unreal.IKRetargetFactory());c=unreal.IKRetargeterController.get_controller(retarget)
        src=unreal.RetargetSourceOrTarget.SOURCE;dst=unreal.RetargetSourceOrTarget.TARGET
        c.remove_all_ops();c.set_ik_rig(src,rigs[0]);c.set_ik_rig(dst,rigs[1]);c.set_preview_mesh(src,source);c.set_preview_mesh(dst,target);c.add_default_ops()
        c.set_retarget_op_enabled(c.get_index_of_op_by_name('Root Motion'),False)
        c.auto_map_chains(unreal.AutoMapChainType.EXACT,True);c.reset_retarget_pose(c.get_current_retarget_pose_name(dst),[],dst);c.auto_align_all_bones(dst,unreal.RetargetAutoAlignMethod.CHAIN_TO_CHAIN)
        unreal.EditorAssetLibrary.save_loaded_asset(retarget)
        inputs=unreal.IKRetargetBatchOperationInputs();inputs.assets_to_retarget=clips;inputs.source_mesh=source;inputs.target_mesh=target;inputs.ik_retarget_asset=retarget
        inputs.search='A_Source_';inputs.replace='A_'+kind+'_';inputs.target_path=dest+'/'+kind;inputs.overwrite_existing_files=True;inputs.include_referenced_assets=False
        products=unreal.IKRetargetBatchOperation.run_batch_retarget(inputs);assert len(products)==4
        for product in products:
            seq=product.get_asset();seq.set_editor_property('enable_root_motion',False);seq.set_editor_property('force_root_lock',False);assert unreal.EditorAssetLibrary.save_loaded_asset(seq)
            report['clips'].append({'path':seq.get_path_name(),'seconds':seq.get_play_length()})
    report['ok']=True
except Exception:report['error']=traceback.format_exc()
out.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
