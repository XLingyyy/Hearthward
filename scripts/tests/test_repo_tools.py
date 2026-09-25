"""Local stdlib tests; Git scenarios use disposable repositories, never a remote."""
from __future__ import annotations

import copy
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
import validate_repo as checks
import agent_context


def write_json(path: Path, data: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def sample_task() -> dict:
    return {
        "schema_version": 1, "id": "TASK-100", "title": "Example isolated task",
        "status": "Backlog", "updated_at": "2026-09-17", "owner": None,
        "reviewer": None, "issue_url": None, "branch": None,
        "source_refs": ["Q161"], "dependencies": [], "blocked_by": [], "contracts": [],
        "allowed_paths": ["Source/A/", "docs/tasks/TASK-100.json", "docs/handoffs/TASK-100.md"],
        "forbidden_paths": ["Source/A/PrivateLocked/"],
        "acceptance": ["Expected observable result"], "non_goals": ["No unrelated changes"],
        "prototype_only": False, "prototype_approval": None, "required_tests": ["T-002"],
        "permissions": {key: "not_granted" for key in ("network", "install_dependencies", "commit", "push", "merge")},
    }


class ScopeSyntaxTests(unittest.TestCase):
    def test_accepts_exact_and_directory_paths(self):
        for path in ("Source/A/", "docs/tasks/TASK-100.json", "Content/中文 目录/"):
            self.assertTrue(checks.safe_repo_path(path), path)

    def test_rejects_unsafe_paths(self):
        for path in ("", "/Source/A", "../other", "a/../b", "C:/repo", "a\\b", "a//b", "Source/*", "a?", " a", "./a", None):
            self.assertFalse(checks.safe_repo_path(path), repr(path))

    def test_prefix_is_not_sibling(self):
        self.assertTrue(checks.path_allowed("Source/A/file.cpp", ["Source/A/"], []))
        self.assertFalse(checks.path_allowed("Source/AB/file.cpp", ["Source/A/"], []))
        self.assertFalse(checks.path_allowed("README.md.bak", ["README.md"], []))

    def test_forbidden_has_priority(self):
        self.assertFalse(checks.path_allowed("Source/A/PrivateLocked/x.h", ["Source/A/"], ["Source/A/PrivateLocked/"]))


class DocumentTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)

    def test_backlog_task_can_be_unassigned(self):
        self.assertEqual(checks.validate_task(sample_task(), self.root, {"TASK-100"}), [])

    def test_active_task_needs_real_fields_and_prototype_approval(self):
        task = sample_task()
        task.update(status="Active", prototype_only=True)
        errors = checks.validate_task(task, self.root, {"TASK-100"})
        self.assertTrue(any("owner" in e for e in errors))
        self.assertTrue(any("prototype" in e for e in errors))

    def test_active_task_does_not_require_issue_or_reviewer(self):
        task = sample_task()
        task.update(status="Active", owner="developer-a", branch="task/TASK-100")
        self.assertEqual(checks.validate_task(task, self.root, {"TASK-100"}), [])

    def test_review_requires_independent_reviewer(self):
        task = sample_task()
        task.update(status="Review", owner="developer-a", branch="task/TASK-100")
        self.assertTrue(any("requires reviewer" in e for e in checks.validate_task(task, self.root, {"TASK-100"})))
        task["reviewer"] = task["owner"]
        self.assertTrue(any("independent" in e for e in checks.validate_task(task, self.root, {"TASK-100"})))
        task["reviewer"] = "reviewer-b"
        self.assertEqual(checks.validate_task(task, self.root, {"TASK-100"}), [])

    def test_issue_url_is_optional_but_validated_when_present(self):
        task = sample_task()
        task["issue_url"] = "https://github.com/example/test/issues/1"
        self.assertEqual(checks.validate_task(task, self.root, {"TASK-100"}), [])
        task["issue_url"] = "https://github.com/example/test/issues/not-a-number"
        self.assertTrue(any("issue_url" in e for e in checks.validate_task(task, self.root, {"TASK-100"})))

    def test_malformed_fields_are_errors_not_crashes(self):
        task = sample_task()
        task.update(status=["Active"], dependencies=[{"bad": "id"}], allowed_paths=[None], contracts=[12])
        self.assertTrue(checks.validate_task(task, self.root, {"TASK-100"}))

    def test_bad_roles_config_is_reported(self):
        self.assertTrue(checks.launch_errors(self.root, {"roles": ["bad"]}))

    def test_missing_contract_and_dependency_are_reported(self):
        task = sample_task()
        task.update(contracts=["docs/contracts/missing.md"], dependencies=["TASK-999"])
        errors = checks.validate_task(task, self.root, {"TASK-100"})
        self.assertTrue(any("contract" in e for e in errors))
        self.assertTrue(any("dependency" in e for e in errors))

    def test_dependency_cycles_are_reported(self):
        a, b = sample_task(), sample_task()
        a.update(dependencies=["TASK-101"])
        b.update(id="TASK-101", dependencies=["TASK-100"])
        self.assertTrue(checks.dependency_cycles({"TASK-100": a, "TASK-101": b}))

    def test_link_checker_ignores_code_and_external_urls(self):
        path = self.root / "README.md"
        path.write_text('[external](https://example.org/a)\n```md\n[x](not-real.md)\n```\n[local](target.md)', encoding="utf-8")
        (self.root / "target.md").write_text("target", encoding="utf-8")
        self.assertEqual(checks.check_markdown_links(self.root, path), [])

    def test_link_checker_reports_missing_and_outside_paths(self):
        path = self.root / "README.md"
        path.write_text("[missing](missing.md)\n[out](../outside.md)", encoding="utf-8")
        errors = checks.check_markdown_links(self.root, path)
        self.assertEqual(len(errors), 2)

    def test_unconfigured_m0_is_not_passed(self):
        errors = checks.launch_errors(self.root, {"adoption_status": "DRAFT", "roles": {}})
        self.assertTrue(any("adoption_status" in e for e in errors))
        self.assertTrue(any("two_machine" in e for e in errors))
        self.assertTrue(any("CODEOWNERS" in e for e in errors))

    def archive_fixture(self):
        folder = self.root / "docs/design/archive"
        folder.mkdir(parents=True)
        raw, text = b"EXAMPLE ARCHIVE BYTES", b"# Extracted text\n"
        manifest = {"archive_file": "test.docx", "sha256": hashlib.sha256(raw).hexdigest(),
                    "extraction_file": "test.md", "extraction_sha256": hashlib.sha256(text).hexdigest()}
        write_json(folder / "MANIFEST.json", manifest)
        (folder / "test.docx").write_bytes(raw)
        (folder / "test.md").write_bytes(text)
        return folder, manifest, raw

    def test_archive_raw_hash(self):
        folder, manifest, raw = self.archive_fixture()
        self.assertEqual(checks.archive_errors(self.root)[0], [])
        (folder / "test.docx").write_bytes(raw + b"CHANGED")
        self.assertTrue(any("mismatch" in e for e in checks.archive_errors(self.root)[0]))

    def test_archive_pointer_not_claimed_as_downloaded(self):
        folder, manifest, raw = self.archive_fixture()
        pointer = f"version https://git-lfs.github.com/spec/v1\noid sha256:{manifest['sha256']}\nsize {len(raw)}\n"
        (folder / "test.docx").write_bytes(pointer.encode("utf-8"))
        errors, notes = checks.archive_errors(self.root)
        self.assertEqual(errors, [])
        self.assertTrue(any("NOT downloaded" in n for n in notes))

    def test_archive_wrong_pointer_rejected(self):
        folder, _, raw = self.archive_fixture()
        pointer = "version https://git-lfs.github.com/spec/v1\noid sha256:" + "0" * 64 + "\nsize 21\n"
        (folder / "test.docx").write_bytes(pointer.encode("utf-8"))
        self.assertTrue(checks.archive_errors(self.root)[0])

    def test_context_rejects_traversal(self):
        text, code = agent_context.context(self.root, "../TASK-100")
        self.assertEqual(code, 2)
        self.assertIn("invalid", text)

    def test_context_without_git_reports_missing_git(self):
        write_json(self.root / "docs/tasks/TASK-100.json", sample_task())
        text, code = agent_context.context(self.root, "TASK-100")
        self.assertEqual(code, 2)
        self.assertIn("GIT_STATUS_UNAVAILABLE", text)


@unittest.skipUnless(shutil.which("git"), "Local Git executable not available")
class LocalGitTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.run_git("init", "-b", "main")
        self.run_git("config", "user.name", "Local Test")
        self.run_git("config", "user.email", "local-test@example.invalid")
        self.run_git("config", "commit.gpgsign", "false")
        self.task = sample_task()
        write_json(self.root / "docs/tasks/TASK-100.json", self.task)
        (self.root / "Source/A").mkdir(parents=True)
        (self.root / "Source/A/initial.cpp").write_text("original\n", encoding="utf-8")
        (self.root / "outside.txt").write_text("base\n", encoding="utf-8")
        self.run_git("add", ".")
        self.run_git("commit", "-m", "Initial approved scope")
        self.run_git("switch", "-c", "task/TASK-100")

    def run_git(self, *args):
        env = os.environ.copy()
        env.update(GIT_TERMINAL_PROMPT="0", GIT_CONFIG_NOSYSTEM="1")
        result = subprocess.run(["git", "-C", str(self.root), *args], capture_output=True, text=True,
                                timeout=10, env=env, check=False)
        if result.returncode:
            self.fail(f"Local Git fixture failed: {args[0]}: {result.stderr}")
        return result.stdout.strip()

    def test_allowed_untracked_file_passes_scope(self):
        (self.root / "Source/A/new.cpp").write_text("new\n", encoding="utf-8")
        errors, _ = checks.check_scope(self.root, "TASK-100", "main")
        self.assertEqual(errors, [])

    def test_untracked_outside_scope_is_caught(self):
        (self.root / "other.cpp").write_text("new\n", encoding="utf-8")
        errors, _ = checks.check_scope(self.root, "TASK-100", "main")
        self.assertTrue(any("OUT_OF_SCOPE: other.cpp" in e for e in errors))

    def test_current_task_cannot_expand_approved_scope(self):
        changed = copy.deepcopy(self.task)
        changed["allowed_paths"].append("outside.txt")
        write_json(self.root / "docs/tasks/TASK-100.json", changed)
        (self.root / "outside.txt").write_text("modified\n", encoding="utf-8")
        errors, _ = checks.check_scope(self.root, "TASK-100", "main")
        self.assertIn("OUT_OF_SCOPE: outside.txt", errors)

    def test_staged_deletion_is_checked(self):
        (self.root / "outside.txt").unlink()
        self.run_git("add", "-u")
        errors, _ = checks.check_scope(self.root, "TASK-100", "main")
        self.assertIn("OUT_OF_SCOPE: outside.txt", errors)

    def test_committed_changes_are_checked(self):
        (self.root / "outside.txt").write_text("committed change\n", encoding="utf-8")
        self.run_git("add", "outside.txt")
        self.run_git("commit", "-m", "Out of scope change")
        self.assertIn("OUT_OF_SCOPE: outside.txt", checks.check_scope(self.root, "TASK-100", "main")[0])

    def test_rename_checks_both_old_and_new_paths(self):
        self.run_git("mv", "outside.txt", "Source/A/moved.txt")
        errors, _ = checks.check_scope(self.root, "TASK-100", "main")
        self.assertIn("OUT_OF_SCOPE: outside.txt", errors)

    def test_missing_base_task_fails(self):
        errors, _ = checks.check_scope(self.root, "TASK-999", "main")
        self.assertTrue(any("approve task" in e for e in errors))

    def test_main_branch_is_not_allowed(self):
        self.run_git("switch", "main")
        errors, _ = checks.check_scope(self.root, "TASK-100", "main")
        self.assertTrue(any("non-main" in e for e in errors))

    def test_context_keeps_dirty_work_unchanged(self):
        target = self.root / "Source/A/initial.cpp"
        target.write_text("uncommitted work\n", encoding="utf-8")
        before = target.read_bytes()
        status_before = self.run_git("status", "--porcelain")
        head_before = self.run_git("rev-parse", "HEAD")
        text, code = agent_context.context(self.root, "TASK-100")
        self.assertEqual(code, 0)
        self.assertIn("Source/A/initial.cpp", text)
        self.assertEqual(target.read_bytes(), before)
        self.assertEqual(self.run_git("status", "--porcelain"), status_before)
        self.assertEqual(self.run_git("rev-parse", "HEAD"), head_before)

    def test_nested_root_rejected(self):
        with self.assertRaises(ValueError):
            checks.require_git_root(self.root / "Source/A")


if __name__ == "__main__":
    unittest.main()
