#!/usr/bin/env python3
"""Validate the documentation starter, optional approved task scope and M0 fields.

Uses only Python's standard library. No fetch/push, builds, network, file writes,
lock operations, or claims that GitHub settings have actually been inspected.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
from urllib.parse import unquote, urlsplit

TASK_ID = re.compile(r"TASK-\d{3,}$")
STATES = {"Backlog", "Ready", "Active", "Review", "Integrated", "Verified", "Done", "Blocked"}
ASSIGNED_STATES = {"Ready", "Active", "Review", "Integrated", "Verified", "Done"}
SKIP_DIRS = {".git", ".venv", "venv", "node_modules", "__pycache__", "Binaries", "Intermediate", "Saved", "DerivedDataCache"}
REQUIRED = (
    "README.md", "WORKFLOW.md", "AGENTS.md", "CONTRIBUTING.md", ".gitattributes", ".gitignore",
    ".github/PULL_REQUEST_TEMPLATE.md", ".github/workflows/repo-checks.yml",
    "config/toolchain.lock.json", "docs/START_HERE.md", "docs/PROJECT_STATE.md",
    "docs/ENVIRONMENT.md", "docs/OWNERSHIP.md", "docs/design/CURRENT.md",
    "docs/design/OPEN_QUESTIONS.md", "docs/design/archive/MANIFEST.json",
    "docs/planning/BOOTSTRAP.md", "docs/qa/BUILD_AND_TEST.md", "docs/qa/TEST_MATRIX.md",
    "docs/qa/STARTER_VALIDATION.md", "docs/templates/handoff.md", "docs/templates/task.json",
    "scripts/agent_context.py", "scripts/validate_repo.py", "scripts/tests/test_repo_tools.py",
)


def git(root: Path, *args: str) -> str:
    """Run a bounded, non-shell, local Git read. No implicit fetch is performed."""
    env = os.environ.copy()
    env.update(GIT_OPTIONAL_LOCKS="0", GIT_TERMINAL_PROMPT="0")
    try:
        result = subprocess.run(
            ["git", "-C", str(root), *args], capture_output=True, text=True,
            encoding="utf-8", errors="replace", timeout=20, env=env, check=False,
        )
    except (OSError, subprocess.TimeoutExpired) as exc:
        raise ValueError(f"Git read failed: {type(exc).__name__}") from exc
    if result.returncode:
        # Do not print arbitrary stderr: it can contain local credentials/URLs.
        raise ValueError(f"Git read failed ({result.returncode}): {args[0]}")
    return result.stdout.rstrip("\n")


def require_git_root(root: Path) -> None:
    actual = Path(git(root, "rev-parse", "--show-toplevel")).resolve()
    if actual != root.resolve():
        raise ValueError("--root must be the repository root, not a nested directory")


def safe_repo_path(path: object) -> bool:
    """Scopes use exact relative paths or directory prefixes ending with '/'."""
    if not isinstance(path, str) or not path or path.strip() != path:
        return False
    if path.startswith("/") or "\\" in path or re.match(r"^[A-Za-z]:", path):
        return False
    if any(c in path for c in "*?[]\x00\n\r"):
        return False
    parts = path.rstrip("/").split("/")
    return all(part not in {"", ".", ".."} for part in parts)


def contained(root: Path, path: Path) -> bool:
    try:
        path.resolve().relative_to(root.resolve())
        return True
    except ValueError:
        return False


def matches_scope(path: str, spec: str) -> bool:
    return path.startswith(spec) if spec.endswith("/") else path == spec


def path_allowed(path: str, allowed: list[str], forbidden: list[str]) -> bool:
    return (
        safe_repo_path(path)
        and not any(matches_scope(path, spec) for spec in forbidden)
        and any(matches_scope(path, spec) for spec in allowed)
    )


def load_json(path: Path) -> dict:
    try:
        obj = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise ValueError(f"Invalid JSON: {path.name}: {type(exc).__name__}") from exc
    if not isinstance(obj, dict):
        raise ValueError(f"Expected JSON object: {path.name}")
    return obj


def substantive(value: object) -> bool:
    if not isinstance(value, str) or not value.strip():
        return False
    return not any(word in value.upper() for word in ("TBD", "TODO", "USER_", "未填写", "待定", "未分配"))


def validate_task(data: dict, root: Path, task_ids: set[str]) -> list[str]:
    errors: list[str] = []
    tid = data.get("id", "<unknown>")
    prefix = str(tid) + ": "
    if not isinstance(tid, str) or not TASK_ID.fullmatch(tid):
        errors.append(prefix + "invalid task id")
    if data.get("schema_version") != 1:
        errors.append(prefix + "schema_version must be 1")
    if not isinstance(data.get("status"), str) or data.get("status") not in STATES:
        errors.append(prefix + "unknown status")
    if not substantive(data.get("title")):
        errors.append(prefix + "missing title")
    if not isinstance(data.get("updated_at"), str) or not re.fullmatch(r"\d{4}-\d{2}-\d{2}", data["updated_at"]):
        errors.append(prefix + "updated_at must use YYYY-MM-DD")
    for key in ("source_refs", "dependencies", "blocked_by", "contracts", "allowed_paths", "forbidden_paths", "acceptance", "non_goals", "required_tests"):
        values = data.get(key)
        if not isinstance(values, list) or any(not isinstance(v, str) or not v for v in values):
            errors.append(prefix + f"{key} must be an array of nonempty strings")
    for key in ("source_refs", "allowed_paths", "acceptance", "required_tests"):
        if not data.get(key):
            errors.append(prefix + f"{key} must not be empty")
    for key in ("allowed_paths", "forbidden_paths"):
        for spec in data.get(key, []) if isinstance(data.get(key), list) else []:
            if not safe_repo_path(spec):
                errors.append(prefix + f"unsafe {key}: {spec!r}; use exact paths or trailing-slash prefixes")
    for dep in data.get("dependencies", []) if isinstance(data.get("dependencies"), list) else []:
        if not isinstance(dep, str) or dep == tid or dep not in task_ids:
            errors.append(prefix + f"unknown/self dependency: {dep}")
    for contract in data.get("contracts", []) if isinstance(data.get("contracts"), list) else []:
        if not safe_repo_path(contract) or not contained(root, root / contract) or not (root / contract).is_file():
            errors.append(prefix + f"missing/unsafe contract: {contract}")
    if not isinstance(data.get("prototype_only"), bool):
        errors.append(prefix + "prototype_only must be a boolean")
    if not isinstance(data.get("permissions"), dict):
        errors.append(prefix + "permissions must be explicit")
    else:
        for permission in ("network", "install_dependencies", "commit", "push", "merge"):
            if not isinstance(data["permissions"].get(permission), str) or not data["permissions"][permission]:
                errors.append(prefix + f"missing permission entry: {permission}")
    if isinstance(data.get("status"), str) and data.get("status") in ASSIGNED_STATES:
        for key in ("owner", "reviewer", "branch"):
            if not substantive(data.get(key)):
                errors.append(prefix + f"{data.get('status')} requires {key}")
        if data.get("owner") == data.get("reviewer"):
            errors.append(prefix + "owner and independent reviewer must differ")
        issue = data.get("issue_url")
        if not isinstance(issue, str) or not re.match(r"https://[^/]+/[^/]+/[^/]+/issues/\d+$", issue):
            errors.append(prefix + "assigned task requires an actual issue URL")
        if isinstance(data.get("branch"), str) and data.get("branch") in {"main", "master"}:
            errors.append(prefix + "task branch cannot be main/master")
        if data.get("prototype_only") and not substantive(data.get("prototype_approval")):
            errors.append(prefix + "prototype task requires explicit approval reference")
    return errors


def dependency_cycles(tasks: dict[str, dict]) -> list[str]:
    errors: list[str] = []
    active: set[str] = set()
    visited: set[str] = set()
    def visit(tid: str) -> None:
        if tid in active:
            errors.append(f"Dependency cycle includes {tid}")
            return
        if tid in visited:
            return
        active.add(tid)
        deps = tasks[tid].get("dependencies", [])
        for dep in deps if isinstance(deps, list) else []:
            if isinstance(dep, str) and dep in tasks:
                visit(dep)
        active.remove(tid)
        visited.add(tid)
    for tid in tasks:
        visit(tid)
    return errors


def strip_fenced_code(text: str) -> str:
    result: list[str] = []
    fence: str | None = None
    for line in text.splitlines():
        match = re.match(r"^\s*(`{3,}|~{3,})", line)
        if match:
            marker = match.group(1)
            if fence is None:
                fence = marker
            elif marker[0] == fence[0] and len(marker) >= len(fence):
                fence = None
            result.append("")
        else:
            result.append(line if fence is None else "")
    return "\n".join(result)


def check_markdown_links(root: Path, path: Path) -> list[str]:
    """Check inline relative file links. Does NOT claim full Markdown/URL validation."""
    errors: list[str] = []
    text = strip_fenced_code(path.read_text(encoding="utf-8"))
    # Repository templates use inline links without nested parentheses in filenames.
    for match in re.finditer(r"!?\[[^\]\n]*\]\((<[^>]+>|[^)\s]+)(?:\s+\"[^\"]*\")?\)", text):
        target = match.group(1).strip("<>")
        parsed = urlsplit(target)
        if parsed.scheme or target.startswith(("#", "//")) or not parsed.path:
            continue
        rel = unquote(parsed.path)
        resolved = path.parent / rel
        if not contained(root, resolved):
            errors.append(f"{path.relative_to(root)}: link leaves repository: {rel}")
        elif not resolved.exists():
            errors.append(f"{path.relative_to(root)}: missing relative link: {rel}")
    return errors


def archive_errors(root: Path) -> tuple[list[str], list[str]]:
    errors: list[str] = []
    notes: list[str] = []
    folder = root / "docs/design/archive"
    try:
        manifest = load_json(folder / "MANIFEST.json")
        for file_key, hash_key in (("archive_file", "sha256"), ("extraction_file", "extraction_sha256")):
            name, expected = manifest.get(file_key), manifest.get(hash_key)
            if not safe_repo_path(name) or "/" in name or not isinstance(expected, str) or not re.fullmatch(r"[a-f0-9]{64}", expected):
                errors.append(f"Archive manifest invalid: {file_key}")
                continue
            path = folder / name
            if not path.is_file() or not contained(root, path):
                errors.append(f"Archive missing: {name}")
                continue
            raw = path.read_bytes()
            if raw.startswith(b"version https://git-lfs.github.com/spec/v1\n"):
                pointer = re.search(rb"(?m)^oid sha256:([a-f0-9]{64})$", raw)
                size = re.search(rb"(?m)^size ([0-9]+)$", raw)
                if not pointer or not size or pointer.group(1).decode() != expected:
                    errors.append(f"Archive LFS pointer mismatch: {name}")
                else:
                    notes.append(f"{name}: LFS pointer OID matches; actual remote object was NOT downloaded or verified")
            elif hashlib.sha256(raw).hexdigest() != expected:
                errors.append(f"Archive SHA256 mismatch: {name}")
    except (ValueError, OSError) as exc:
        errors.append(str(exc))
    return errors, notes


def launch_errors(root: Path, config: dict) -> list[str]:
    errors: list[str] = []
    if config.get("adoption_status") != "ADOPTED":
        errors.append("M0: adoption_status must be ADOPTED after team review")
    for key in ("repository_url", "uproject_path", "ue_version", "ue_distribution", "host_os", "compiler_version", "sdk_version", "git_version", "git_lfs_version", "last_verified_commit"):
        if not substantive(config.get(key)):
            errors.append(f"M0: missing {key}")
    project = config.get("uproject_path")
    if isinstance(project, str) and (not safe_repo_path(project) or not contained(root, root / project) or not (root / project).is_file()):
        errors.append("M0: configured .uproject path must be an existing repository file")
    if config.get("project_identifier_status") != "CONFIRMED":
        errors.append("M0: project identifier is not CONFIRMED")
    roles = config.get("roles") if isinstance(config.get("roles"), dict) else {}
    for role in ("design_owner", "technical_owner", "integration_owner", "verification_owner"):
        if not substantive(roles.get(role)):
            errors.append(f"M0: role not assigned: {role}")
    for flag in ("github_protection_verified", "lfs_two_user_drill_verified", "two_machine_build_verified"):
        if config.get(flag) is not True:
            errors.append(f"M0: verification record not marked true: {flag}")
    owners = root / ".github/CODEOWNERS"
    if not owners.is_file():
        errors.append("M0: active .github/CODEOWNERS is missing (example does not count)")
    else:
        text = owners.read_text(encoding="utf-8")
        active_lines = [line for line in text.splitlines() if line.strip() and not line.lstrip().startswith("#")]
        if not active_lines or any("USER_" in line or "TBD" in line for line in active_lines):
            errors.append("M0: CODEOWNERS still empty or contains placeholders")
    return errors


def collect_scope_changes(root: Path, base_sha: str) -> list[str]:
    paths: set[str] = set()
    commands = (
        ("diff", "--no-renames", "--name-only", "-z", f"{base_sha}...HEAD", "--"),
        ("diff", "--no-renames", "--name-only", "-z", "--cached", "--"),
        ("diff", "--no-renames", "--name-only", "-z", "--"),
        ("ls-files", "--others", "--exclude-standard", "-z"),
    )
    for args in commands:
        paths.update(p for p in git(root, *args).split("\x00") if p)
    return sorted(paths)


def check_scope(root: Path, task_id: str, base: str) -> tuple[list[str], list[str]]:
    errors: list[str] = []
    notes: list[str] = []
    if not TASK_ID.fullmatch(task_id):
        return ["Invalid --task identifier"], notes
    if base.startswith("-") or "\x00" in base:
        return ["Unsafe --base reference"], notes
    try:
        require_git_root(root)
        branch = git(root, "branch", "--show-current")
        if not branch or branch in {"main", "master"}:
            errors.append("Scope check requires a named non-main task branch")
        base_sha = git(root, "rev-parse", "--verify", f"{base}^{{commit}}")
        try:
            baseline = json.loads(git(root, "show", f"{base_sha}:docs/tasks/{task_id}.json"))
        except (ValueError, json.JSONDecodeError):
            return errors + [f"No usable approved task snapshot at base: {task_id}; approve task on baseline first"], notes
        if not isinstance(baseline, dict) or baseline.get("id") != task_id:
            return errors + ["Base task snapshot id mismatch"], notes
        for key in ("allowed_paths", "forbidden_paths"):
            scopes = baseline.get(key)
            if not isinstance(scopes, list) or any(not safe_repo_path(p) for p in scopes):
                return errors + [f"Base task has unsafe {key}"], notes
        if not baseline["allowed_paths"]:
            return errors + ["Base task has no approved paths"], notes
        changed = collect_scope_changes(root, base_sha)
        for path in changed:
            if not path_allowed(path, baseline["allowed_paths"], baseline["forbidden_paths"]):
                errors.append(f"OUT_OF_SCOPE: {path}")
            if (root / path).exists() and not contained(root, root / path):
                errors.append(f"SYMLINK_OUTSIDE_REPO: {path}")
        notes.append(f"Scope from base commit {base_sha}; inspected {len(changed)} changed paths")
        notes.append("Local path check only: current GitHub assignment, approvals and LFS locks are NOT verified")
    except ValueError as exc:
        errors.append(str(exc))
    return errors, notes


def validate(root: Path, launch_ready: bool = False) -> tuple[list[str], list[str]]:
    errors: list[str] = []
    notes: list[str] = []
    for rel in REQUIRED:
        p = root / rel
        if not p.is_file() or not contained(root, p):
            errors.append(f"Missing/unsafe required file: {rel}")
    config: dict = {}
    try:
        config = load_json(root / "config/toolchain.lock.json")
    except ValueError as exc:
        errors.append(str(exc))
    tasks: dict[str, dict] = {}
    for path in sorted((root / "docs/tasks").glob("TASK-*.json")):
        try:
            if not contained(root, path):
                errors.append(f"Task symlink leaves repository: {path.name}")
                continue
            task = load_json(path)
            tid = task.get("id")
            if not isinstance(tid, str) or path.stem != tid or tid in tasks:
                errors.append(f"Task file/id mismatch or duplicate: {path.name}")
                continue
            tasks[tid] = task
        except ValueError as exc:
            errors.append(str(exc))
    if not tasks:
        errors.append("No task snapshots found")
    for task in tasks.values():
        errors.extend(validate_task(task, root, set(tasks)))
    errors.extend(dependency_cycles(tasks))
    for field, document, pattern in (
        ("blocked_by", "docs/design/OPEN_QUESTIONS.md", r"(?m)^\| (R\d{2,}) \|"),
        ("required_tests", "docs/qa/TEST_MATRIX.md", r"(?m)^\| (T-\d{3,}) \|"),
    ):
        source = root / document
        if source.is_file():
            known = set(re.findall(pattern, source.read_text(encoding="utf-8")))
            for tid, task in tasks.items():
                references = task.get(field, [])
                for ref in references if isinstance(references, list) else []:
                    if not isinstance(ref, str) or ref not in known:
                        errors.append(f"{tid}: unknown {field} reference: {ref!r}")
    for current, dirs, files in os.walk(root, followlinks=False):
        dirs[:] = [d for d in dirs if d not in SKIP_DIRS and not (Path(current) / d).is_symlink()]
        for filename in files:
            path = Path(current) / filename
            if path.suffix == ".md":
                if not contained(root, path):
                    errors.append(f"Markdown symlink leaves repository: {path.relative_to(root)}")
                    continue
                try:
                    errors.extend(check_markdown_links(root, path))
                except (OSError, UnicodeError, ValueError) as exc:
                    errors.append(f"Unreadable Markdown: {path.relative_to(root)}: {type(exc).__name__}")
    errs, archive_notes = archive_errors(root)
    errors.extend(errs)
    notes.extend(archive_notes)
    if launch_ready:
        errors.extend(launch_errors(root, config))
        notes.append("M0 checks declared fields only; live GitHub, two-person LFS and UE build evidence require human verification")
    else:
        notes.append("M0/UE/gameplay/model checks were NOT run; use --launch-ready for configuration completeness, not actual game validation")
    notes.append(f"Task snapshots inspected: {len(tasks)}")
    return errors, notes


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--launch-ready", action="store_true")
    parser.add_argument("--task")
    parser.add_argument("--base")
    args = parser.parse_args(argv)
    if bool(args.task) != bool(args.base):
        parser.error("--task and --base must be provided together")
    root = args.root.resolve()
    errors, notes = validate(root, args.launch_ready)
    if args.task:
        scope_errors, scope_notes = check_scope(root, args.task, args.base)
        errors.extend(scope_errors)
        notes.extend(scope_notes)
    for note in notes:
        print("NOTE:", note)
    for error in errors:
        print("ERROR:", error)
    print(f"{'FAIL' if errors else 'PASS'}: repository checks only; {len(errors)} error(s)")
    return 1 if errors else 0


if __name__ == "__main__":
    raise SystemExit(main())
