from pathlib import Path
import unreal
root=Path(unreal.Paths.project_dir()).resolve()
for script in ['docs/assets/TASK-048/import_models.py','docs/qa/TASK-048/verify_nature_pie.py']:
    path=root/script
    exec(compile(path.read_text(encoding='utf-8'),str(path),'exec'),globals())
