"""Reimport corrected top-facing water surfaces without touching terrain."""
from pathlib import Path
import json
import unreal
root=Path(unreal.Paths.project_dir())
source=root/'art_source/TASK-026/Rebuild'
tools=unreal.AssetToolsHelpers.get_asset_tools()
for entry in json.loads((source/'water.json').read_text())+[{'name':'GrassCards'}]:
    task=unreal.AssetImportTask()
    task.set_editor_properties(dict(filename=str(source/'water'/(entry['name']+'.glb')),
        destination_path='/Game/Hearthward/Assets/NaturalWorld/Rebuild/Meshes',
        destination_name='SM_'+entry['name'],automated=True,save=True,replace_existing=True))
    tools.import_asset_tasks([task])
