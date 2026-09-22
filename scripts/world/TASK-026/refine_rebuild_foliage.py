"""Retain fine leaf coverage and shade short grass as foliage."""
from pathlib import Path
import json,sys,runpy
import unreal
root=Path(unreal.Paths.project_dir());asset='/Game/Hearthward/Assets/NaturalWorld/Rebuild'
ml=unreal.MaterialEditingLibrary
leaf=unreal.load_asset(asset+'/Materials/M_Tree_leaves')
leaf.set_editor_property('opacity_mask_clip_value',.2)
changed=[]
for node in unreal.ObjectIterator(unreal.MaterialExpressionTextureSample):
    if node.get_outer()!=leaf:continue
    texture=node.get_editor_property('texture')
    if texture and texture.get_name()=='T_Tree_leaves_Alpha':
        node.set_editor_properties(dict(mip_value_mode=unreal.TextureMipValueMode.TMVM_MIP_BIAS,const_mip_value=-2))
        changed.append(node.get_name())
assert changed
ml.recompile_material(leaf);unreal.EditorAssetLibrary.save_loaded_asset(leaf)
gm=unreal.load_asset(asset+'/Meshes/GrassCards/StaticMeshes/SM_GrassCards')
meshes=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
settings=meshes.get_nanite_settings(gm);settings.set_editor_property('enabled',False);meshes.set_nanite_settings(gm,settings)
unreal.EditorAssetLibrary.save_loaded_asset(gm)
mat=unreal.load_asset(asset+'/Materials/M_GrassCards')
mat.set_editor_properties(dict(shading_model=unreal.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE,tangent_space_normal=False,opacity_mask_clip_value=.25))
for node in list(unreal.ObjectIterator(unreal.MaterialExpressionConstant3Vector)):
    if node.get_outer()==mat:ml.delete_material_expression(mat,node)
normal=ml.create_material_expression(mat,unreal.MaterialExpressionConstant3Vector)
normal.set_editor_property('constant',unreal.LinearColor(0,0,1,1))
ml.connect_material_property(normal,'',unreal.MaterialProperty.MP_NORMAL)
sub=ml.create_material_expression(mat,unreal.MaterialExpressionConstant3Vector)
sub.set_editor_property('constant',unreal.LinearColor(.18,.24,.055,1))
ml.connect_material_property(sub,'',unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
ml.recompile_material(mat);unreal.EditorAssetLibrary.save_loaded_asset(mat)
runpy.run_path(str(root/'scripts/world/TASK-026/ground_rebuild_rocks.py'),run_name='__main__')
unreal.SystemLibrary.execute_console_command(unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world(),'grass.FlushCache')
unreal.EditorLevelLibrary.set_level_viewport_camera_info(unreal.Vector(-100000,-95000,22000),unreal.Rotator(pitch=-10,yaw=55,roll=0))
assert unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True,True)
(root/'Saved/Task026/Rebuild/foliage-refinement.json').write_text(json.dumps({'leaf_bias':-2,'leaf_clip':.2,'grass_nanite':False,'leaf_nodes':changed}))
