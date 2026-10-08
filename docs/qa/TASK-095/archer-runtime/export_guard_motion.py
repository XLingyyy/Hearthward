import unreal,json,traceback
from pathlib import Path
out=Path('G:/GameFactory/Hearthward/.agent-local/qa/TASK-095/guard-motion')
out.mkdir(parents=True,exist_ok=True)
report=[]
try:
 for name in ['Idle','Walk','Run']:
  asset=unreal.load_asset('/Game/Hearthward/Campaign/Guard/A_Guard_'+name)
  task=unreal.AssetExportTask();task.object=asset;task.filename=str(out/(name+'.fbx'))
  task.automated=True;task.prompt=False;task.replace_identical=True
  task.exporter=unreal.AnimSequenceExporterFBX();task.options=unreal.FbxExportOption()
  task.options.set_editor_property('export_preview_mesh',False)
  ok=unreal.Exporter.run_asset_export_task(task)
  report.append({'clip':asset.get_path_name(),'ok':ok,'errors':list(task.errors),'bytes':Path(task.filename).stat().st_size if Path(task.filename).exists() else 0})
except Exception:
 report.append({'error':traceback.format_exc()})
finally:
 (out/'results.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
