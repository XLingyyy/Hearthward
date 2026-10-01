"""Stable Unreal build operations for UEClient v1."""

from __future__ import annotations

import os
import re
import subprocess
from pathlib import Path
from typing import Any

from ..config import UEClientConfig
from ..contracts import UEDiagnostic, UEOperationResult


def _build_script(ue_root: Path) -> tuple[list[str], str]:
    if os.name == "nt":
        return (
            [
                str(
                    ue_root
                    / "Engine"
                    / "Build"
                    / "BatchFiles"
                    / "Build.bat"
                )
            ],
            "Win64",
        )
    return (
        [
            "bash",
            str(
                ue_root
                / "Engine"
                / "Build"
                / "BatchFiles"
                / "Linux"
                / "Build.sh"
            ),
        ],
        "Linux",
    )


def _diagnostics(output: str) -> list[UEDiagnostic]:
    pattern = re.compile(
        r"^(?P<file>.*?)(?:\((?P<line>\d+)\))?"
        r":\s*(?P<severity>error|warning)"
        r"(?:\s+(?P<code>[A-Za-z]+\d+))?:\s*"
        r"(?P<message>.+)$",
        flags=re.IGNORECASE,
    )
    result = []
    for line in output.splitlines():
        match = pattern.match(line.strip())
        if not match:
            continue
        result.append(
            UEDiagnostic(
                severity=match.group("severity").lower(),
                message=match.group("message").strip(),
                code=str(match.group("code") or ""),
                file=str(match.group("file") or "").strip(),
                line=(
                    int(match.group("line"))
                    if match.group("line")
                    else None
                ),
                source="unreal_build_tool",
            )
        )
    return result


class UEBuildClient:
    def __init__(self, config: UEClientConfig) -> None:
        self._config = config

    def project(
        self,
        *,
        target: str = "",
        configuration: str = "Development",
        clean: bool = False,
        dry_run: bool = False,
        timeout: float | None = None,
    ) -> dict[str, Any]:
        ue_root = self._config.ue_root
        project_file = self._config.project_file
        if ue_root is None:
            return UEOperationResult.failure(
                "build.project",
                "ue_root is not configured",
            ).to_dict()
        if project_file is None or not project_file.is_file():
            return UEOperationResult.failure(
                "build.project",
                "project_path does not resolve to an existing "
                ".uproject file",
            ).to_dict()

        prefix, platform = _build_script(ue_root)
        script_path = Path(prefix[-1])
        if not script_path.is_file():
            return UEOperationResult.failure(
                "build.project",
                f"Unreal build script was not found: {script_path}",
            ).to_dict()
        resolved_target = (
            str(target).strip()
            or f"{project_file.stem}Editor"
        )
        command = [
            *prefix,
            resolved_target,
            platform,
            str(configuration or "Development"),
            f"-Project={project_file}",
            "-WaitMutex",
        ]
        if clean:
            command.append("-Clean")
        payload = {
            "command": command,
            "cwd": str(project_file.parent),
            "target": resolved_target,
            "platform": platform,
            "configuration": configuration,
            "clean": clean,
            "dry_run": dry_run,
        }
        if dry_run:
            return UEOperationResult.success(
                "build.project",
                payload=payload,
            ).to_dict()

        try:
            completed = subprocess.run(
                command,
                cwd=project_file.parent,
                capture_output=True,
                text=True,
                timeout=timeout,
                check=False,
            )
        except Exception as exc:
            return UEOperationResult.failure(
                "build.project",
                f"{type(exc).__name__}: {exc}",
                payload=payload,
            ).to_dict()
        output = "\n".join(
            item
            for item in (
                completed.stdout.strip(),
                completed.stderr.strip(),
            )
            if item
        )
        diagnostics = _diagnostics(output)
        payload.update(
            {
                "returncode": completed.returncode,
                "stdout": completed.stdout,
                "stderr": completed.stderr,
            }
        )
        if completed.returncode != 0:
            return UEOperationResult.failure(
                "build.project",
                f"Unreal build failed with exit code "
                f"{completed.returncode}",
                diagnostics=diagnostics,
                payload=payload,
            ).to_dict()
        return UEOperationResult.success(
            "build.project",
            diagnostics=diagnostics,
            artifacts=[
                {
                    "type": "unreal_build",
                    "path": str(project_file.parent),
                    "state": "ready",
                }
            ],
            payload=payload,
        ).to_dict()

    def package(self, *, archive_dir: str | Path, configuration: str = "Shipping",
                maps: tuple[str, ...] = (), extra_args: tuple[str, ...] = (),
                log_path: str | Path, timeout: float | None = None,
                dry_run: bool = False) -> dict[str, Any]:
        """Build, cook, stage and archive a Windows distribution through UAT."""
        project = self._config.project_file
        engine = self._config.ue_root
        if os.name != "nt" or engine is None or project is None or not project.is_file():
            return UEOperationResult.failure("build.package", "Windows, ue_root and an existing project are required").to_dict()
        uat = engine / "Engine/Build/BatchFiles/RunUAT.bat"
        if not uat.is_file():
            return UEOperationResult.failure("build.package", f"UAT not found: {uat}").to_dict()
        archive = Path(archive_dir).resolve()
        log = Path(log_path).resolve()
        command = [str(uat), "BuildCookRun", f"-project={project}", "-noP4",
                   "-platform=Win64", f"-clientconfig={configuration}", "-build", "-cook",
                   "-stage", "-pak", "-iostore", "-archive", f"-archivedirectory={archive}",
                   "-prereqs", "-utf8output", "-unattended", "-NoLiveCoding"]
        if maps:
            command.append("-map=" + "+".join(maps))
        command.extend(extra_args)
        payload = {"command": command, "archive_dir": str(archive), "log_path": str(log), "dry_run": dry_run}
        if dry_run:
            return UEOperationResult.success("build.package", payload=payload).to_dict()
        archive.mkdir(parents=True, exist_ok=True)
        log.parent.mkdir(parents=True, exist_ok=True)
        try:
            with log.open("w", encoding="utf-8") as output:
                completed = subprocess.run(command, cwd=project.parent, stdout=output,
                                           stderr=subprocess.STDOUT, timeout=timeout, check=False)
            payload["returncode"] = completed.returncode
            if completed.returncode != 0:
                return UEOperationResult.failure("build.package", f"UAT failed: {completed.returncode}; see {log}", payload=payload).to_dict()
            if not any(archive.rglob("*.exe")):
                return UEOperationResult.failure("build.package", "UAT produced no archived executable", payload=payload).to_dict()
        except Exception as exc:
            return UEOperationResult.failure("build.package", f"{type(exc).__name__}: {exc}", payload=payload).to_dict()
        return UEOperationResult.success("build.package", payload=payload,
            artifacts=[{"type": "unreal_package", "path": str(archive), "state": "ready"}]).to_dict()
