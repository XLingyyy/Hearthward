"""Check that the source archive, preview and imported UE assets are complete."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
from urllib.parse import unquote
from paths import GAME, SOURCE, load_jobs, source_path


def digest(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def package_path(object_path):
    return GAME / 'Content' / (object_path.removeprefix('/Game/').split('.', 1)[0]+'.uasset')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    failures = []
    archive = json.loads((GAME/'docs/qa/project-progress-20261002/source_archive.json').read_text('utf-8'))
    for record in archive['files']:
        path = (GAME/record['path']).resolve()
        if not path.is_relative_to(GAME) or not path.is_file():
            failures.append('Missing archived file: '+record['path'])
        elif digest(path) != record['sha256']:
            failures.append('Archive checksum differs: '+record['path'])
    imported = json.loads((GAME/'docs/qa/TASK-051/import_report.json').read_text('utf-8'))
    jobs = {job['slug']:job for job in load_jobs()}
    clip_count = 0
    for animal in imported['animals']:
        folder = Path(jobs[animal['slug']]['output'])
        for path, expected in [(folder/('SK_'+animal['slug']+'.fbx'),animal['skeletal_sha256']),
                               (folder/('AS_'+animal['slug']+'.blend'),animal['source_blend_sha256'])]:
            if not path.is_file() or digest(path)!=expected:
                failures.append('Imported source checksum differs: '+str(path.relative_to(GAME)))
        manifest = json.loads((folder/'animation_manifest.json').read_text('utf-8'))
        native = {clip['name']:clip for clip in animal['clips']}
        for clip in manifest['clips']:
            clip_count += 1
            path = source_path(clip['file'])
            if not path.is_file() or digest(path)!=clip['sha256'] or clip['sha256']!=native[clip['name']]['source_sha256']:
                failures.append('Clip source/import mismatch: '+clip['name'])
            if not package_path(native[clip['name']]['asset']).is_file():
                failures.append('Missing native animation: '+clip['name'])
        for asset in [animal['mesh']['primary_asset']['path'], animal['skeleton']]:
            if not package_path(asset).is_file():
                failures.append('Missing native mesh/skeleton: '+asset)
    preview=(SOURCE/'动物动作预览.html').read_text('utf-8')
    references=set(re.findall(r'(?:Hearthward|corrections_20261001_r3)/[^\s\"\'<>]+',preview))
    for reference in references:
        if not (SOURCE/unquote(reference)).is_file():
            failures.append('Missing preview reference: '+reference)
    if len(jobs)!=14 or len(imported['animals'])!=14 or clip_count!=303:
        failures.append('Expected 14 paired species and 303 clips')
    report={'tested_commit':subprocess.check_output(['git','rev-parse','HEAD'],cwd=GAME,text=True).strip(),
            'archive_files':len(archive['files']), 'species':len(jobs), 'clips':clip_count,
            'preview_references':len(references), 'failures':failures, 'ok':not failures}
    if args.report:
        args.report.parent.mkdir(parents=True,exist_ok=True)
        args.report.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(report,ensure_ascii=False,indent=2))
    raise SystemExit(bool(failures))


if __name__ == '__main__':
    main()
