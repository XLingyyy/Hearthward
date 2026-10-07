#!/usr/bin/env python3
"""Read-only static validator for this task-document delivery, not the UE project.

Uses only the standard library. No network, subprocess, copying, or Git writes.
--repo optionally checks conflicts and registered T references against a local repo.
Outputs a JSON report to stdout; does not mutate tasks or their execution status.
"""
from __future__ import annotations
import argparse
import csv
import hashlib
import json
from pathlib import Path
import re
from urllib.parse import unquote, urlsplit

EXPECTED_IDS = [f'TASK-{n:03}' for n in range(84, 104)]
KNOWN_TEST_IDS = {f'T-{n:03}' for n in range(1, 27)}
ARRAY_KEYS = ('source_refs','dependencies','blocked_by','contracts','allowed_paths',
              'forbidden_paths','acceptance','non_goals','required_tests')
PERMISSIONS = ('network','install_dependencies','commit','push','merge')


def safe_path(value: object) -> bool:
    if not isinstance(value, str) or not value or value.strip() != value:
        return False
    if value.startswith('/') or '\\' in value or re.match(r'^[A-Za-z]:', value):
        return False
    if any(c in value for c in '*?[]\x00\n\r'):
        return False
    return all(p not in ('', '.', '..') for p in value.rstrip('/').split('/'))


def matches(path: str, spec: str) -> bool:
    return path.startswith(spec) if spec.endswith('/') else path == spec


def strip_fences(text: str) -> str:
    result = []
    marker = None
    for line in text.splitlines():
        m = re.match(r'^\s*(`{3,}|~{3,})', line)
        if m:
            fence = m.group(1)
            if marker is None:
                marker = fence
            elif fence[0] == marker[0] and len(fence) >= len(marker):
                marker = None
            result.append('')
        else:
            result.append(line if marker is None else '')
    return '\n'.join(result)


def validate(root: Path, repo: Path | None) -> dict:
    errors: list[str] = []
    notes: list[str] = []
    tasks: dict[str, dict] = {}
    matrix_ids = KNOWN_TEST_IDS.copy()
    taskdir = root/'docs/tasks'
    files = sorted(taskdir.glob('TASK-*.json'))
    if [p.stem for p in files] != EXPECTED_IDS:
        errors.append('Task JSON numbering must be exactly TASK-084 through TASK-103.')
    if sorted(p.stem for p in taskdir.glob('TASK-*.md')) != EXPECTED_IDS:
        errors.append('Task Markdown numbering must be exactly TASK-084 through TASK-103.')
    if repo is not None:
        matrix = repo/'docs/qa/TEST_MATRIX.md'
        if not matrix.is_file():
            errors.append('--repo lacks docs/qa/TEST_MATRIX.md; use the actual game repository root.')
        else:
            matrix_ids=set(re.findall(r'(?m)^\| (T-\d{3,}) \|',matrix.read_text(encoding='utf-8')))
    local_case_count = 0
    expected_cases = []
    for path in files:
        tid = path.stem
        try:
            obj = json.loads(path.read_text(encoding='utf-8'))
        except (OSError, UnicodeError, json.JSONDecodeError) as e:
            errors.append(f'{tid}: invalid JSON ({type(e).__name__}).')
            continue
        if not isinstance(obj, dict):
            errors.append(f'{tid}: metadata must be an object.')
            continue
        tasks[tid]=obj
        if obj.get('schema_version') != 1 or obj.get('id') != tid:
            errors.append(f'{tid}: schema_version/id mismatch.')
        if obj.get('status') != 'Backlog':
            errors.append(f'{tid}: this unexecuted delivery must start at Backlog.')
        if not isinstance(obj.get('updated_at'),str) or not re.fullmatch(r'\d{4}-\d{2}-\d{2}',obj['updated_at']):
            errors.append(f'{tid}: invalid date.')
        if not obj.get('title') or not obj.get('owner'):
            errors.append(f'{tid}: missing title or inherited project owner.')
        if obj.get('reviewer') is not None or obj.get('issue_url') is not None:
            errors.append(f'{tid}: do not invent reviewer or Issue.')
        if not obj.get('branch','').startswith(f'codex/{tid}-'):
            errors.append(f'{tid}: proposed branch must match task ID.')
        if not isinstance(obj.get('prototype_only'),bool):
            errors.append(f'{tid}: prototype_only must be boolean.')
        for key in ARRAY_KEYS:
            val=obj.get(key)
            if not isinstance(val,list) or any(not isinstance(v,str) or not v for v in val):
                errors.append(f'{tid}: malformed {key}.')
        for key in ('source_refs','allowed_paths','acceptance','required_tests'):
            if not obj.get(key): errors.append(f'{tid}: empty {key}.')
        for key in ('allowed_paths','forbidden_paths'):
            for spec in obj.get(key,[]):
                if not safe_path(spec): errors.append(f'{tid}: unsafe scope {spec!r}.')
        for allowed in obj.get('allowed_paths',[]):
            for forbidden in obj.get('forbidden_paths',[]):
                if matches(allowed,forbidden) or (allowed.endswith('/') and matches(forbidden,allowed)):
                    errors.append(f'{tid}: conflicting scope {allowed} / {forbidden}.')
        if 'README.md' not in obj.get('allowed_paths',[]):
            errors.append(f'{tid}: mandatory README closeout scope missing.')
        for suffix in ('.md','.json'):
            if f'docs/tasks/{tid}{suffix}' not in obj.get('allowed_paths',[]):
                errors.append(f'{tid}: own task file missing from scope.')
        for dep in obj.get('dependencies',[]):
            if dep not in EXPECTED_IDS or dep==tid:
                errors.append(f'{tid}: invalid/self dependency {dep}.')
        for contract in obj.get('contracts',[]):
            if not safe_path(contract) or not (root/contract).is_file():
                errors.append(f'{tid}: missing/unsafe contract {contract}.')
        for test_id in obj.get('required_tests',[]):
            if test_id not in matrix_ids: errors.append(f'{tid}: unregistered global test {test_id}.')
        if obj.get('blocked_by'):
            errors.append(f'{tid}: unapproved drafting gates must not masquerade as R references.')
        perms=obj.get('permissions',{})
        if not isinstance(perms,dict) or any(not perms.get(k) for k in PERMISSIONS):
            errors.append(f'{tid}: missing explicit permissions.')
        else:
            if perms.get('network') != 'read_only': errors.append(f'{tid}: unexpected network scope.')
            for key in ('install_dependencies','commit','push','merge','github_release','paid_services'):
                if perms.get(key) != 'not_granted': errors.append(f'{tid}: ungranted permission {key} was expanded.')
        md_path=taskdir/f'{tid}.md'
        if not md_path.is_file(): continue
        text=md_path.read_text(encoding='utf-8')
        if not text.startswith(f"# {tid}｜{obj.get('title')}"):
            errors.append(f'{tid}: title not aligned between MD/JSON.')
        heads=re.findall(r'(?m)^## (\d+)\.',text)
        if heads != [str(n) for n in range(1,11)]: errors.append(f'{tid}: ten required sections missing/out of order.')
        cases=re.findall(r'(?m)^\|(T\d{3}-C\d{2})\|',text)
        wanted=[f'T{tid[-3:]}-C{i:02}' for i in range(1,len(obj.get('acceptance',[]))+1)]
        if cases != wanted: errors.append(f'{tid}: local cases not contiguous or acceptance count differs.')
        for i,a in enumerate(obj.get('acceptance',[]),1):
            if not a.startswith(f'T{tid[-3:]}-C{i:02}：'):
                errors.append(f'{tid}: acceptance-case ID mismatch.')
        for line in text.splitlines():
            if re.match(r'^\|T\d{3}-C\d{2}\|',line) and not line.endswith('|NOT_RUN|'):
                errors.append(f'{tid}: fabricated initial case result.')
        expected_cases.extend(cases);local_case_count+=len(cases)
        if not (root/f'docs/handoffs/{tid}.md').is_file():errors.append(f'{tid}: missing handoff template.')

    active:set[str]=set();visited:set[str]=set()
    def visit(tid:str)->None:
        if tid in active:
            errors.append(f'Dependency cycle at {tid}.');return
        if tid in visited:return
        active.add(tid)
        for dep in tasks.get(tid,{}).get('dependencies',[]):
            if dep in tasks:visit(dep)
        active.remove(tid);visited.add(tid)
    for tid in tasks:visit(tid)

    case_file=root/'docs/planning/TASK-084-103/TEST_CASES.csv'
    if not case_file.is_file():errors.append('Missing case CSV.')
    else:
        with case_file.open(encoding='utf-8',newline='') as f:csv_cases=list(csv.DictReader(f))
        if [r.get('local_case_id') for r in csv_cases] != expected_cases:
            errors.append('CSV cases do not match task Markdown order/content IDs.')
        if any(r.get('result') != 'NOT_RUN' or r.get('evidence') for r in csv_cases):
            errors.append('CSV contains fabricated initial test results/evidence.')

    markdowns=sorted(root.rglob('*.md'))
    for path in markdowns:
        try:text=path.read_text(encoding='utf-8')
        except UnicodeError:errors.append(f'Invalid UTF-8: {path.relative_to(root)}');continue
        if '\ufffd' in text:errors.append(f'Replacement character: {path.relative_to(root)}')
        for m in re.finditer(r'!?\[[^\]\n]*\]\((<[^>]+>|[^)\s]+)(?:\s+"[^"]*")?\)',strip_fences(text)):
            target=m.group(1).strip('<>');parsed=urlsplit(target)
            if parsed.scheme or target.startswith(('#','//')) or not parsed.path:continue
            dest=(path.parent/unquote(parsed.path)).resolve()
            if not dest.is_relative_to(root.resolve()):errors.append(f'Link leaves package: {path.relative_to(root)} -> {target}')
            elif not dest.exists():errors.append(f'Missing link: {path.relative_to(root)} -> {target}')
    font_files=[str(p.relative_to(root)) for p in root.rglob('*') if p.is_file() and p.suffix.lower() in {'.ttf','.otf','.woff','.woff2'}]
    if font_files:errors.append('Font binaries must not be included.')
    if repo is not None:
        identical=0
        for p in sorted((root/'docs').rglob('*')):
            if not p.is_file():continue
            rel=p.relative_to(root);q=repo/rel
            if q.exists():
                if q.is_file() and q.read_bytes()==p.read_bytes():identical+=1
                else:errors.append(f'Repository import collision; do not overwrite: {rel}')
        notes.append(f'Optional repository read: {identical} byte-identical existing docs; other collisions listed as errors. No files changed.')
    manifest=root/'MANIFEST.sha256'
    if manifest.is_file():
        listed=set()
        for line in manifest.read_text(encoding='utf-8').splitlines():
            h,sep,name=line.partition('  ')
            if not sep or not re.fullmatch(r'[a-f0-9]{64}',h) or not safe_path(name):
                errors.append('Malformed manifest row.');continue
            listed.add(name);p=root/name
            if not p.is_file() or hashlib.sha256(p.read_bytes()).hexdigest()!=h:
                errors.append(f'Manifest checksum mismatch: {name}')
        actual={str(p.relative_to(root)) for p in root.rglob('*') if p.is_file() and p!=manifest}
        if listed!=actual:errors.append('Manifest file inventory differs from package.')
    notes.extend([
        'Planning baseline: 6fcf5c22e965f0f7409438f19bc7b09e96ffb058.',
        'Global test IDs were checked against the retrieved TEST_MATRIX snapshot, or --repo when supplied.',
        'This checks the document package, not the original full repository validator or UE project.',
        'The GitHub connection was used for source reads; a full container Git clone was unavailable.',
        'UE build, gameplay, real model, LFS locks, human tests, two-machine tests and publishing: NOT_RUN.',
    ])
    return {'status':'PASS' if not errors else 'FAIL','validation_scope':'task_document_package_static_only',
            'task_json_count':len(tasks),'task_markdown_count':len(list(taskdir.glob('TASK-*.md'))),
            'handoff_count':len(list((root/'docs/handoffs').glob('TASK-*.md'))),
            'local_case_count':local_case_count,'markdown_files_checked':len(markdowns),
            'dependency_graph':'acyclic' if not any('cycle' in e.lower() for e in errors) else 'cyclic',
            'errors':errors,'notes':notes,'full_repository_validation':'NOT_RUN',
            'ue_build_and_runtime_validation':'NOT_RUN','git_write_actions':'NOT_PERFORMED'}


def main()->int:
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo',type=Path,help='Optional existing repository root for read-only import conflict checks.')
    args=parser.parse_args()
    try:report=validate(Path(__file__).resolve().parent,args.repo.resolve() if args.repo else None)
    except (OSError,UnicodeError,ValueError,csv.Error) as exc:
        report={'status':'FAIL','validation_scope':'task_document_package_static_only','errors':[f'{type(exc).__name__}: {exc}']}
    print(json.dumps(report,ensure_ascii=False,indent=2))
    return 0 if report['status']=='PASS' else 1

if __name__=='__main__':raise SystemExit(main())
