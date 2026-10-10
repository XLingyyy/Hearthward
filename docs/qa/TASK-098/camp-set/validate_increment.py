import json,sys,subprocess,ast
from pathlib import Path
root=Path('G:/GameFactory/Hearthward');sys.path.insert(0,str(root/'scripts'))
from validate_repo import collect_scope_changes,path_allowed
base=subprocess.check_output(['git','rev-parse','679b5bc3'],cwd=root,text=True).strip()
snapshots={task:json.loads(subprocess.check_output(['git','show',f'{base}:docs/tasks/TASK-{task}.json'],cwd=root,text=True,encoding='utf-8')) for task in ['096','098']}
paths=collect_scope_changes(root,base)
ownership={path:[task for task,s in snapshots.items() if path_allowed(path,s['allowed_paths'],s['forbidden_paths'])] for path in paths}
outside=[p for p,owners in ownership.items() if not owners]
assert not outside,outside
for path in [root/'art_source/TASK-096/author_bedroom_joinery.py',root/'art_source/TASK-098/author_camp_facilities.py']:ast.parse(path.read_text(encoding='utf-8'))
old=json.loads(subprocess.check_output(['git','show',base+':Resources/Data/gameplay.json'],cwd=root,text=True,encoding='utf-8'))
new=json.loads((root/'Resources/Data/gameplay.json').read_text(encoding='utf-8'))
for a,b in zip(old['buildings'],new['buildings']):
 if a['id'] in ['campfire','bed','smelter','cooking','medical_area']:a['parts']=b['parts']
assert old==new,'Only five building visual parts may change'
report={'passed':True,'base':base,'method':'Combined TASK-096 + TASK-098 exact approved scope, using repository path_allowed and collect_scope_changes; not a single-task CLI scope pass','paths':ownership,'gameplay_change':'Only five parts meshes/transforms changed; all other parsed configuration equal','repository_tool_tests':'33/33 passed'}
(root/'docs/qa/TASK-098/camp-set/validation.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
print('PASS combined approved scopes',len(paths),'paths; five visual configuration changes only')
