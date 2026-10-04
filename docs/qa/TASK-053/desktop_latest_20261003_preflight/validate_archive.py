"""Validate the local Shipping candidate against current source and the focused UI evidence."""
import hashlib
import json
from pathlib import Path
import subprocess

GAME = Path(__file__).resolve().parents[4]
OUT = GAME / 'docs/qa/TASK-053/desktop_20261003_latest_ui'
ARCHIVE = GAME / 'Saved/UIShipping/TASK-053/desktop_20261003_latest_ui/Archive/Windows'


def digest(path):
    result = hashlib.sha256()
    with path.open('rb') as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b''):
            result.update(block)
    return result.hexdigest()


def load(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


manifest = load(OUT / 'source-manifest.json')
build = load(OUT / 'build-info.json')
current_sources = {p: digest(GAME / p) == value for p, value in manifest.items()}
resources = {p: (ARCHIVE / 'Hearthward' / p).is_file() and digest(ARCHIVE / 'Hearthward' / p) == value
             for p, value in manifest.items() if p.startswith('Resources/')}
development = load(Path(__file__).with_name('development-validation.json'))
development_matches = {}
for name, entry in development.items():
    record = GAME / entry['evidence']
    hashes = load(record / 'fingerprints.json')
    development_matches[name] = {
        p: digest(GAME / p) == value for p, value in hashes.items()
    }
    development_matches[name]['report_unchanged'] = digest(record / 'report.json') == entry['report_sha256']
lock = load(GAME / 'config/local-ai.lock.json')['model']
model = ARCHIVE / 'Hearthward/Runtime/LocalAI/models' / lock['filename']
checks = {
    'package_passed': build['package_result'] == 'PASS',
    'version': build['version'] == '0.2.0-preview.20261003.2',
    'source_manifest_hash': digest(OUT / 'source-manifest.json') == build['source_manifest_sha256'],
    'all_source_files_current': all(current_sources.values()),
    'all_resources_match_source': all(resources.values()),
    'latest_loading_art_included': resources.get('Resources/UI/Art/loading-ashes.png', False),
    'four_focused_development_runs_passed': len(development) == 4 and all(e['passed'] for e in development.values()),
    'development_runs_still_current': all(all(e.values()) for e in development_matches.values()),
    'executable_matches_build': digest(ARCHIVE / 'Hearthward/Binaries/Win64/Hearthward-Win64-Shipping.exe') == build['executable_sha256'],
    'model_size': model.is_file() and model.stat().st_size == lock['size'],
    'model_sha256': model.is_file() and digest(model) == lock['sha256'],
    'no_developer_saved_data': not (ARCHIVE / 'Hearthward/Saved').exists(),
}
files = [p for p in ARCHIVE.rglob('*') if p.is_file()]
report = {'passed': all(checks.values()), 'checks': checks, 'source_files': current_sources, 'resources': resources,
          'development_fingerprints': development_matches, 'archive_files': len(files),
          'archive_bytes': sum(p.stat().st_size for p in files), 'base_commit': build['base_commit'],
          'branch': build['branch'], 'version': build['version'], 'archive': str(ARCHIVE)}
(OUT / 'archive-validation.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
print(json.dumps({'passed': report['passed'], 'checks': checks, 'source_count': len(manifest),
                  'resource_count': len(resources), 'archive_files': len(files)}, ensure_ascii=False, indent=2))
raise SystemExit(0 if report['passed'] else 1)
