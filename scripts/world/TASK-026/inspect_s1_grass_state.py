"""Read-only diagnosis of persisted S1 grass settings and rendered weight data."""
from pathlib import Path
import json
import unreal

root=Path(unreal.Paths.project_dir())
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
asset='/Game/Hearthward/Assets/NaturalWorld/Rebuild'
gt=unreal.load_asset(asset+'/Foliage/GT_S1_Meadow')
mat=unreal.load_asset(asset+'/Materials/M_Landscape')
ml=unreal.MaterialEditingLibrary
nodes=[n for n in unreal.ObjectIterator(unreal.MaterialExpression) if n.get_outer()==mat]
output=next(n for n in nodes if isinstance(n,unreal.MaterialExpressionLandscapeGrassOutput))
report={'cvars':{n:unreal.SystemLibrary.get_console_variable_float_value(n) for n in
    ['grass.Enable','grass.densityScale','grass.CullDistanceScale','grass.GrassMap.UseRuntimeGeneration','ShowFlag.InstancedGrass']},
    'varieties':[str(v) for v in gt.get_editor_property('grass_varieties')],
    'outputs':str(output.get_editor_property('grass_types')),
    'output_inputs':[n.get_name() if n else None for n in ml.get_inputs_for_material_expression(mat,output)],
    'landscapes':[]}
for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if isinstance(actor,unreal.LandscapeProxy):
        report['landscapes'].append({'name':actor.get_name(),
            'properties':[x for x in dir(actor) if 'grass' in x.lower()]})
out=root/'docs/qa/evidence/TASK-026/rework-v2/S1-grass-state.json'
out.write_text(json.dumps(report,indent=2),encoding='utf-8')
unreal.SystemLibrary.execute_console_command(world,'grass.DumpGrassData -summary -bygrasstype')
