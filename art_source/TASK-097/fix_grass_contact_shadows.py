"""Run in UE Editor after acquiring the two LandscapeGrassType package locks."""
import json
import unreal
from pathlib import Path

root = Path(unreal.Paths.project_dir()).resolve()
out = root / '.agent-local/qa/TASK-097/grass-contact-fix-20261008'
out.mkdir(parents=True, exist_ok=True)
report = {'passed': False, 'assets': []}
for name in ('GT_Meadow', 'GT_S1_Meadow'):
    path = '/Game/Hearthward/Assets/NaturalWorld/Rebuild/Foliage/' + name
    asset = unreal.load_asset(path)
    asset.modify()
    varieties = asset.get_editor_property('grass_varieties')
    before = [v.get_editor_property('cast_contact_shadow') for v in varieties]
    # Alpha-card contact shadows produce solid rectangular patches at ground level.
    for index in range(len(varieties)):
        variety = varieties[index]
        variety.set_editor_property('cast_contact_shadow', False)
        varieties[index] = variety
    asset.set_editor_property('grass_varieties', varieties)
    assert not any(v.get_editor_property('cast_contact_shadow')
                   for v in asset.get_editor_property('grass_varieties')), path
    assert unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False), path
    report['assets'].append({'path': path, 'before': before,
                            'after': [v.get_editor_property('cast_contact_shadow')
                                      for v in asset.get_editor_property('grass_varieties')]})
report['passed'] = True
(out / 'result.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
