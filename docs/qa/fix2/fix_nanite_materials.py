"""Persist Nanite usage for the eight authored camp tree trunk materials in UE Editor."""
import json
import traceback
from pathlib import Path
import unreal

out = Path(unreal.Paths.project_saved_dir()) / 'Fix2/materials.json'
out.parent.mkdir(parents=True, exist_ok=True)
report = {'ok': False, 'materials': []}
try:
    usage = unreal.MaterialUsage.MATUSAGE_NANITE
    for species in ['Fir', 'Pine']:
        for part in ['Bark', 'TrunkA', 'Twig', 'TrunkC']:
            path = f'/Game/Hearthward/Assets/NaturalWorld/Rebuild/Materials/M_Camp{species}_{part}'
            material = unreal.EditorAssetLibrary.load_asset(path)
            assert isinstance(material, unreal.Material), path
            before = unreal.MaterialEditingLibrary.has_material_usage(material, usage)
            if not before:
                unreal.MaterialEditingLibrary.set_base_material_usage(material, usage, True)
                unreal.MaterialEditingLibrary.recompile_material(material)
                assert unreal.EditorAssetLibrary.save_loaded_asset(material), path
            after = unreal.MaterialEditingLibrary.has_material_usage(material, usage)
            assert after, path
            report['materials'].append({'asset': path, 'before': before, 'after': after})
    report['ok'] = True
except Exception:
    report['error'] = traceback.format_exc()
out.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
