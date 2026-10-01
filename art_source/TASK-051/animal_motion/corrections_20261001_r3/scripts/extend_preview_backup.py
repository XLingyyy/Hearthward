"""Preserve old thumbnails before rendering current reviews."""
import shutil
from pathlib import Path
from common import ROOT,REV,jobs,read,save,sha

index=read(REV/'backup_index.json');known={r['source'] for r in index['files']};added=0
for job in jobs():
    for source in (Path(job['output'])/'review').glob('*.png'):
        if str(source) in known:continue
        dest=REV/'backup'/source.relative_to(ROOT.parent);dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,dest)
        digest=sha(source);assert sha(dest)==digest
        index['files'].append({'source':str(source),'backup':str(dest),'bytes':source.stat().st_size,'sha256':digest});added+=1
save(REV/'backup_index.json',index)
print('Extended immutable backup with',added,'original pose thumbnails')
