"""Snapshot every file that can change; originals are never deleted."""
import shutil
from common import ROOT, REV, jobs, sha, save

def main():
    index = REV / 'backup_index.json'
    if index.exists():
        raise RuntimeError('The immutable R3 backup already exists')
    paths = set()
    for job in jobs():
        out = __import__('pathlib').Path(job['output'])
        paths.add(__import__('pathlib').Path(job['guidance']))
        paths.update(p for p in out.iterdir() if p.is_file() and p.suffix != '.blend1' and p.name != 'source_import.blend')
        paths.update((out / 'clips').glob('*.fbx'))
        for folder in ('review', 'previews', 'loop_preview', 'refinement_preview', 'correction_preview'):
            paths.update(p for p in (out / folder).rglob('*') if p.is_file() and p.suffix in ('.mp4', '.json'))
    paths.update(p for p in ROOT.iterdir() if p.is_file() and p.suffix in ('.json', '.md', '.html'))
    records = []
    for source in sorted(paths):
        destination = REV / 'backup' / source.relative_to(ROOT.parent)
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, destination)
        digest = sha(source)
        if sha(destination) != digest:
            raise RuntimeError('Backup checksum mismatch: '+str(source))
        records.append({'source': str(source), 'backup': str(destination), 'bytes': source.stat().st_size, 'sha256': digest})
    save(index, {'schema': 'animal.motion.backup.r3', 'files': records, 'cloud_credits_used': 0})
    print('BACKUP', len(records), 'files', sum(r['bytes'] for r in records), 'bytes', flush=True)

if __name__ == '__main__':
    main()
