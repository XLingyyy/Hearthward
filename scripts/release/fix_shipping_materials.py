import json,traceback
from pathlib import Path
import unreal
root=Path(unreal.Paths.project_dir())
out=root/"Saved/ReleaseMaterials/result.json"
out.parent.mkdir(parents=True,exist_ok=True)
result={"ok":False,"materials":[]}
try:
    report=json.loads((root/"docs/releases/demo-20260924/material-fixes.json").read_text(encoding="utf-8"))
    paths=[m["asset"].replace("/Game/","Content/",1)+".uasset" for m in report["materials"]]
    assert paths
    for path in paths:
        asset="/Game/"+path.removeprefix("Content/").removesuffix(".uasset")
        material=unreal.load_asset(asset)
        assert isinstance(material,unreal.Material),asset
        changes=[]
        for node in unreal.ObjectIterator(unreal.MaterialExpressionTextureSample):
            if node.get_outer()!=material: continue
            texture=node.get_editor_property("texture")
            if texture and texture.get_editor_property("compression_settings")==unreal.TextureCompressionSettings.TC_MASKS:
                before=node.get_editor_property("sampler_type")
                if before!=unreal.MaterialSamplerType.SAMPLERTYPE_MASKS:
                    node.set_editor_property("sampler_type",unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
                    changes.append({"node":node.get_name(),"texture":texture.get_path_name(),"before":str(before),"after":"Masks"})
        if not changes: continue
        unreal.MaterialEditingLibrary.recompile_material(material)
        assert unreal.EditorAssetLibrary.save_loaded_asset(material)
        result["materials"].append({"asset":asset,"changes":changes})
    result["ok"]=True
except Exception:
    result["error"]=traceback.format_exc()
out.write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding="utf-8")
unreal.SystemLibrary.quit_editor()
