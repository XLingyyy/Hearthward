import unreal, json
from pathlib import Path
r={}
for name in ['Hero','Brother']:
    m=unreal.load_asset(f'/Game/Characters/{name}/UE5/M_{name}')
    unreal.MaterialEditingLibrary.set_material_usage(m, unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH)
    unreal.MaterialEditingLibrary.recompile_material(m)
    r[name]=unreal.EditorAssetLibrary.save_loaded_asset(m,False)
r['ok']=all(r.values())
(Path(unreal.Paths.project_saved_dir())/'HeroValidation/material-usage.json').write_text(json.dumps(r),encoding='utf-8')
unreal.SystemLibrary.quit_editor()
