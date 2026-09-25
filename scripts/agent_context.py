#!/usr/bin/env python3
"""Print a read-only task/Git handoff receipt. Does not fetch or authorize work."""
from __future__ import annotations

import argparse
from pathlib import Path
import sys

sys.dont_write_bytecode = True

from validate_repo import TASK_ID, contained, git, load_json, require_git_root


def context(root: Path, task_id: str) -> tuple[str, int]:
    if not TASK_ID.fullmatch(task_id):
        return "ERROR: invalid task identifier", 2
    task_file = root / f"docs/tasks/{task_id}.json"
    if not contained(root, task_file):
        return "ERROR: task path leaves repository", 2
    try:
        task = load_json(task_file)
    except ValueError as exc:
        return f"ERROR: {exc}", 2
    lines = [
        f"# {task_id} / Agent read-only receipt",
        f"Repository directory: {root.resolve()}",
        "This local snapshot is NOT confirmation of remote ownership, permissions or locks.",
        "No network, installs, writes, checkout, stash, commit or push performed.",
        "",
    ]
    exit_code = 0
    try:
        require_git_root(root)
        branch = git(root, "branch", "--show-current")
        lines += [f"Branch: {branch or 'DETACHED_HEAD'}", f"HEAD: {git(root, 'rev-parse', 'HEAD')}"]
        try:
            upstream = git(root, "rev-parse", "--abbrev-ref", "--symbolic-full-name", "@{u}")
        except ValueError:
            upstream = "NO_UPSTREAM"
        lines += [f"Upstream ref: {upstream} (local record; not fetched)", "", "## Working tree", git(root, "status", "--short", "--branch")]
        lines += ["", "## Recent commits", git(root, "log", "-5", "--oneline")]
        lines += ["", "## Unstaged paths", git(root, "diff", "--name-status", "--") or "none"]
        lines += ["", "## Staged paths", git(root, "diff", "--cached", "--name-status", "--") or "none"]
        if not branch or branch in {"main", "master"}:
            lines += ["WARNING: not on a named task branch; do not start editing."]
    except ValueError as exc:
        lines += [f"GIT_STATUS_UNAVAILABLE: {exc}", "Cannot certify worktree/branch/HEAD; do not start editing."]
        exit_code = 2
    lines += ["", "## Task snapshot"]
    for key in ("title", "status", "updated_at", "owner", "reviewer", "issue_url", "branch", "dependencies", "blocked_by", "source_refs", "contracts", "allowed_paths", "forbidden_paths", "acceptance", "prototype_only", "prototype_approval", "permissions", "required_tests"):
        lines.append(f"{key}: {task.get(key)!r}")
    if task.get("status") != "Active":
        lines.append("WARNING: task is not Active in this snapshot. A printed receipt is not a claim or approval.")
    handoff = root / f"docs/handoffs/{task_id}.md"
    lines += ["", "## Current branch handoff"]
    if handoff.is_file() and contained(root, handoff):
        text = handoff.read_text(encoding="utf-8")
        lines.append(text[:16000])
        if len(text) > 16000:
            lines.append("[truncated; read the remaining handoff before editing]")
    else:
        lines.append("NO_HANDOFF: no task-branch handoff is present. Establish the real state before writing.")
    lines += [
        "", "## Required reads (not automatically considered read)",
        "AGENTS.md -> docs/START_HERE.md -> docs/PROJECT_STATE.md -> task -> task-branch handoff -> contracts/design -> actual code",
        "Confirm task ownership and approved scope with the coordinator; inspect any linked Issue and actual LFS locks using authorized tools.",
        "Do not display .env, credential files or private raw model conversations in the receipt.",
    ]
    return "\n".join(lines), exit_code


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--task", required=True)
    args = parser.parse_args(argv)
    try:
        text, code = context(args.root.resolve(), args.task)
    except (OSError, UnicodeError) as exc:
        text, code = f"ERROR: cannot read local task context: {type(exc).__name__}", 2
    print(text)
    return code


if __name__ == "__main__":
    raise SystemExit(main())
