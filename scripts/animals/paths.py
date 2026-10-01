"""Resolve the archived source bundle and local UE/GameFactory installations."""
from pathlib import Path
import json
import os
import subprocess
import sys

GAME = Path(__file__).resolve().parents[2]
SOURCE = GAME / 'art_source/TASK-051/animal_motion'
OUTPUT = GAME / '.agent-local/animal-factory-output'


def factory_root():
    configured = os.environ.get('HEARTHWARD_FACTORY_ROOT')
    candidates = [Path(configured)] if configured else []
    candidates.append(GAME.parent / 'GameFactory-3A')
    common = subprocess.check_output(
        ['git', 'rev-parse', '--git-common-dir'], cwd=GAME, text=True).strip()
    common_path = Path(common)
    if not common_path.is_absolute():
        common_path = GAME / common_path
    candidates.append(common_path.resolve().parent.parent / 'GameFactory-3A')
    for candidate in candidates:
        if (candidate / 'engine_adapters/ue5/__init__.py').is_file():
            return candidate.resolve()
    raise FileNotFoundError('Set HEARTHWARD_FACTORY_ROOT to the prepared GameFactory-3A checkout.')


def ue_root():
    configured = os.environ.get('HEARTHWARD_UE_ROOT')
    candidates = [Path(configured)] if configured else [
        Path(r'E:\UE_5.8'), Path(r'C:\Program Files\Epic Games\UE_5.8')]
    for candidate in candidates:
        if (candidate / 'Engine/Binaries/Win64/UnrealEditor.exe').is_file():
            return candidate.resolve()
    raise FileNotFoundError('Set HEARTHWARD_UE_ROOT to the UE 5.8.2 installation.')


def source_path(value):
    """Relocate original authoring metadata without modifying its provenance."""
    value = str(value).replace('\\', '/')
    marker = '/Resource/Tripo/动物/动作/制作成果/'
    if marker in value:
        return SOURCE / value.split(marker, 1)[1]
    if '/Resource/' in value:
        return GAME / 'art_source/TASK-004' / value.split('/Resource/', 1)[1]
    path = Path(value)
    if path.is_absolute():
        return path
    if value.startswith('art_source/'):
        return GAME / value
    return SOURCE / value


def load_jobs():
    jobs = json.loads((SOURCE / 'jobs.json').read_text(encoding='utf-8'))
    for job in jobs:
        job['output'] = str(SOURCE / 'Hearthward' / job['run_id'] / 'assets/motion' / job['slug'])
        job['source'] = str(source_path(job['source']))
        job['guidance'] = str(SOURCE / 'guides' / Path(job['guidance']).name)
    return jobs


def configure_factory(*, source_metadata=False):
    sys.path.insert(0, str(factory_root()))
    os.environ['AAAGF_OUTPUT_ROOT'] = str(OUTPUT)
    if source_metadata:
        for job in load_jobs():
            folder = Path(job['output'])
            metadata = json.loads((folder / 'meta.json').read_text(encoding='utf-8'))
            for key, value in metadata.items():
                if key.endswith('_path'):
                    metadata[key] = str(source_path(value))
            target = OUTPUT / job['game_id'] / job['run_id'] / 'assets/motion' / job['slug']
            target.mkdir(parents=True, exist_ok=True)
            (target / 'meta.json').write_text(
                json.dumps(metadata, ensure_ascii=False, indent=2), encoding='utf-8')
