"""Retarget Epic's bundled third-person locomotion using UE's native IK Retargeter."""
import json
import math
import traceback
from pathlib import Path
import unreal

DEST='/Game/Characters/Hero/Animation'
out=Path(unreal.Paths.project_saved_dir())/'Task027/template-retarget.json'
report={'passed':False}

def multiply(a,b):
    x,y,z,w=a.x,a.y,a.z,a.w; X,Y,Z,W=b.x,b.y,b.z,b.w
    return unreal.Quat(w*X+x*W+y*Z-z*Y,w*Y-x*Z+y*W+z*X,w*Z+x*Y-y*X+z*W,w*W-x*X-y*Y-z*Z)

def joint_positions(pose,parents):
    points={}; rotations={}; scales={}
    for bone,value in pose.items():
        parent=parents[bone]
        q=rotations.get(parent,unreal.Quat(0,0,0,1))
        scale=scales.get(parent,unreal.Vector(1,1,1))
        p=value.translation
        v=multiply(multiply(q,unreal.Quat(p.x*scale.x,p.y*scale.y,p.z*scale.z,0)),unreal.Quat(-q.x,-q.y,-q.z,q.w))
        origin=points.get(parent,unreal.Vector())
        points[bone]=unreal.Vector(origin.x+v.x,origin.y+v.y,origin.z+v.z)
        rotations[bone]=multiply(q,value.rotation)
        scales[bone]=unreal.Vector(scale.x*value.scale3d.x,scale.y*value.scale3d.y,scale.z*value.scale3d.z)
    return points

def asset(name, cls, factory):
    path=DEST+'/Retarget/'+name
    return unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,DEST+'/Retarget',cls,factory)

def run():
    source=unreal.load_asset('/Game/Mannequin/Character/Mesh/SK_Mannequin')
    original_mesh=unreal.load_asset('/Game/Characters/Hero/Tripo/SK_Hero_Tripo')
    assert source and original_mesh
    reader=unreal.SkeletonModifier()
    assert reader.set_skeletal_mesh(original_mesh)
    bones=[str(b) for b in reader.get_all_bone_names()]
    parents={b:str(reader.get_parent_name(b)) for b in bones}
    reference={b:reader.get_bone_transform(b,False) for b in bones}
    parent_scales={b:reader.get_bone_transform(parents[b],True).scale3d if parents[b]!='None' else unreal.Vector(1,1,1) for b in bones}
    ankles=['0_Right_Limb_1','1_Left_Limb_1']
    standing_height=min(reader.get_bone_transform(b,True).translation.z for b in ankles)
    mesh_path=DEST+'/Retarget/SK_Hero_RetargetProxy'
    target=unreal.load_asset(mesh_path) if unreal.EditorAssetLibrary.does_asset_exist(mesh_path) else unreal.EditorAssetLibrary.duplicate_asset(original_mesh.get_path_name(),mesh_path)
    modifier=unreal.SkeletonModifier()
    assert modifier.set_skeletal_mesh(target)
    turn=unreal.Quat(0,0,math.sqrt(.5),math.sqrt(.5))
    undo_turn=unreal.Quat(0,0,-math.sqrt(.5),math.sqrt(.5))
    desired_root=multiply(turn,reference['Root'].rotation)
    current_root=modifier.get_bone_transform('Root',False)
    dot=sum(getattr(current_root.rotation,k)*getattr(desired_root,k) for k in ['x','y','z','w'])
    if current_root.scale3d.x>2 or abs(dot)<.99999:
        for bone in bones:
            value=reference[bone]; p=value.translation; scale=parent_scales[bone]
            value.translation=unreal.Vector(p.x*scale.x,p.y*scale.y,p.z*scale.z)
            value.scale3d=unreal.Vector(1,1,1)
            if bone=='Root': value.rotation=desired_root
            assert modifier.set_bone_transform(bone,value,True)
        # Transform-only changes update this mesh's reference pose, not its shared Skeleton asset.
        assert modifier.commit_skeleton_to_skeletal_mesh()
    unreal.EditorAssetLibrary.save_loaded_asset(target)
    src_chains={'Spine':('spine_01','spine_03'),'Head':('head','head'),
        'Clavicle_L':('clavicle_l','clavicle_l'),'Clavicle_R':('clavicle_r','clavicle_r'),
        'Arm_L':('upperarm_l','hand_l'),'Arm_R':('upperarm_r','hand_r'),
        'Leg_L':('thigh_l','foot_l'),'Leg_R':('thigh_r','foot_r')}
    dst_chains={'Spine':('Spine_1','0_Left_Limb_0'),'Head':('bone_4','bone_4'),
        'Clavicle_L':('Head_0','Head_0'),'Clavicle_R':('0_Left_Limb_1','0_Left_Limb_1'),
        'Arm_L':('bone_6','bone_8'),'Arm_R':('0_Left_Limb_2','0_Left_Limb_4'),
        'Leg_L':('bone_17','0_Right_Limb_1'),'Leg_R':('bone_21','1_Left_Limb_1')}
    rigs=[]
    for name,mesh,root,chains in [('IK_UE_Template',source,'pelvis',src_chains),('IK_Hero',target,'Spine_0',dst_chains)]:
        rig=asset(name,unreal.IKRigDefinition,unreal.IKRigDefinitionFactory())
        c=unreal.IKRigController.get_controller(rig)
        assert c.set_skeletal_mesh(mesh)
        for existing in c.get_retarget_chains(): c.remove_retarget_chain(existing.chain_name)
        c.set_retarget_root(root)
        for chain,(first,last) in chains.items(): c.add_retarget_chain(chain,first,last,'None')
        unreal.EditorAssetLibrary.save_loaded_asset(rig)
        rigs.append(rig)
    retarget=asset('RTG_Template_Hero',unreal.IKRetargeter,unreal.IKRetargetFactory())
    c=unreal.IKRetargeterController.get_controller(retarget)
    src=unreal.RetargetSourceOrTarget.SOURCE; dst=unreal.RetargetSourceOrTarget.TARGET
    c.remove_all_ops()
    c.set_ik_rig(src,rigs[0]);c.set_ik_rig(dst,rigs[1])
    c.set_preview_mesh(src,source);c.set_preview_mesh(dst,target)
    c.add_default_ops()
    # Locomotion is in-place; capsule movement remains authoritative.
    c.set_retarget_op_enabled(c.get_index_of_op_by_name('Root Motion'),False)
    c.auto_map_chains(unreal.AutoMapChainType.EXACT,True)
    c.reset_retarget_pose(c.get_current_retarget_pose_name(dst),[],dst)
    c.auto_align_all_bones(dst,unreal.RetargetAutoAlignMethod.CHAIN_TO_CHAIN)
    unreal.EditorAssetLibrary.save_loaded_asset(retarget)
    report['ops']=[str(c.get_op_name(i)) for i in range(c.get_num_retarget_ops())]
    report['chains']={'source':src_chains,'target':dst_chains}
    report['output']=[]
    unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous([DEST],True)
    for original,result in [('ThirdPersonIdle','A_Hero_Idle'),('ThirdPersonWalk','A_Hero_Walk'),('ThirdPersonRun','A_Hero_Sprint')]:
        candidate=DEST+'/'+result+'1'
        if unreal.EditorAssetLibrary.does_asset_exist(candidate): unreal.EditorAssetLibrary.delete_asset(candidate)
        inputs=unreal.IKRetargetBatchOperationInputs()
        inputs.assets_to_retarget=[unreal.EditorAssetLibrary.find_asset_data('/Game/Mannequin/Animations/'+original)]
        inputs.source_mesh=source;inputs.target_mesh=target;inputs.ik_retarget_asset=retarget
        inputs.search=original;inputs.replace=result;inputs.target_path=DEST
        inputs.overwrite_existing_files=True;inputs.include_referenced_assets=False
        products=unreal.IKRetargetBatchOperation.run_batch_retarget(inputs)
        assert products, original
        for product in products:
            normalized=product.get_asset()
            length=normalized.get_play_length()
            # Template FBX clips use fractional rates. Resample to the game's 30 fps clips.
            frames=round(length*30)
            keys={b:[unreal.AnimationLibrary.get_bone_pose_for_time(normalized,b,length*i/frames,False) for i in range(frames+1)] for b in bones}
            for bone,values in keys.items():
                scale=parent_scales[bone]
                for value in values:
                    p=value.translation
                    value.translation=unreal.Vector(p.x/scale.x,p.y/scale.y,p.z/scale.z)
                    if bone=='Root': value.rotation=multiply(undo_turn,value.rotation)
                    value.scale3d=reader.get_bone_transform(bone,False).scale3d
            # Tripo thighs branch from Root, bypassing the pelvis retarget chain.
            # Bake support height into Root so the in-place cycles sit on the floor.
            for i in range(frames+1):
                points=joint_positions({b:keys[b][i] for b in bones},parents)
                root=keys['Root'][i];p=root.translation
                root.translation=unreal.Vector(p.x,p.y,p.z+standing_height-min(points[b].z for b in ankles))
            published=unreal.load_asset(DEST+'/'+result)
            controller=published.get_editor_property('controller')
            controller.open_bracket('Publish UE template in imported skeleton units',False)
            controller.remove_all_bone_tracks(False)
            controller.set_frame_rate(unreal.FrameRate(30,1),False)
            controller.set_number_of_frames(unreal.FrameNumber(frames),False)
            for bone in bones:
                values=keys[bone]
                positions=[v.translation for v in values]
                rotations=[v.rotation for v in values]
                controller.add_bone_curve(bone,False)
                controller.set_bone_track_keys(bone,positions,rotations,[reader.get_bone_transform(bone,False).scale3d]*(frames+1),False)
            controller.close_bracket(False)
            published.set_editor_property('force_root_lock',False)
            assert unreal.EditorAssetLibrary.save_loaded_asset(published)
            report['output'].append(DEST+'/'+result)
            report[result+'_root']=str(unreal.AnimationLibrary.get_bone_pose_for_time(published,'Root',0.,False))
            if normalized != published:
                unreal.EditorAssetLibrary.delete_asset(str(product.package_name))
    report['passed']=True

try: run()
except Exception: report['error']=traceback.format_exc()
out.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
if report['passed'] and '-Task027TemplatePreview' in unreal.SystemLibrary.get_command_line():
    preview=Path(unreal.Paths.project_dir())/'scripts/characters/TASK-027/preview_hero.py'
    exec(compile(preview.read_text(encoding='utf-8'),str(preview),'exec'),{'__name__':'__main__'})
else:
    unreal.SystemLibrary.quit_editor()
