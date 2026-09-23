"""Diagnose height-source, render target, and Landscape coordinate agreement."""
from pathlib import Path
import json
import traceback
import unreal

out=Path(unreal.Paths.project_dir())/'Saved/Task026/Rebuild'
editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
world=editor.get_editor_world()
report={}
try:
    land=next(a for a in actors.get_all_level_actors() if isinstance(a,unreal.Landscape))
    report['layers']=[{'name':str(layer.get_name_bp()),'visible':layer.get_editor_property('visible'),
                       'heightmap_alpha':layer.get_editor_property('heightmap_alpha')}
                      for layer in land.get_edit_layers_bp()]
    rt=unreal.RenderingLibrary.create_render_target2d(world,2017,2017,unreal.TextureRenderTargetFormat.RTF_RGBA8)
    mat=unreal.load_asset('/Game/Hearthward/Assets/NaturalWorld/Rebuild/Materials/M_HeightImport')
    unreal.RenderingLibrary.draw_material_to_render_target(world,rt,mat)
    unreal.RenderingLibrary.export_render_target(world,rt,str(out),'height-drawn.png')
    land.landscape_export_heightmap_to_render_target(rt,True,True)
    unreal.RenderingLibrary.export_render_target(world,rt,str(out),'height-landscape.png')
    report['proxies']=[{'name':a.get_name(),'position':str(a.get_actor_location()),'scale':str(a.get_actor_scale3d())}
                       for a in actors.get_all_level_actors() if isinstance(a,unreal.LandscapeProxy)]
    report['ok']=True
except Exception:
    report.update(ok=False,error=traceback.format_exc())
(out/'height-diagnostic.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
