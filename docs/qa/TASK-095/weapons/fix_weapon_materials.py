import unreal,json,traceback
from pathlib import Path
out=Path('G:/GameFactory/Hearthward/.agent-local/qa/TASK-095/weapons');report=[]
try:
 lib=unreal.MaterialEditingLibrary
 for label in ['Shortblade','Longblade','Spear','Waraxe','Crossbow']:
  mat=unreal.load_asset('/Game/Hearthward/Assets/TASK-095/Weapons/'+label+'/M_'+label+'_Practical')
  body=lib.get_material_property_input_node(mat,unreal.MaterialProperty.MP_BASE_COLOR)
  inputs=lib.get_inputs_for_material_expression(mat,body)
  surface,color=inputs[1],inputs[2]
  assert isinstance(color,unreal.MaterialExpressionVertexColor)
  assert lib.connect_material_expressions(color,'',body,'A')
  assert lib.connect_material_expressions(color,'',surface,'B')
  assert not lib.recompile_material(mat)
  assert lib.get_inputs_for_material_expression(mat,body)[0]==color
  assert lib.get_inputs_for_material_expression(mat,surface)[1]==color
  assert unreal.EditorAssetLibrary.save_loaded_asset(mat)
  report.append(dict(piece=label,connected=True,material=mat.get_path_name()))
except Exception:report.append({'error':traceback.format_exc()})
finally:(out/'material-fix.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
