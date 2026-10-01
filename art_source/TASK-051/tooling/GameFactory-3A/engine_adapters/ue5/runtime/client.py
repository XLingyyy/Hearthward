"""Stable Unreal Editor process operations for UEClient v1."""

from __future__ import annotations

import os
import subprocess
from pathlib import Path
from typing import Any, Sequence

from ..assets import UEAssetsClient
from ..config import UEClientConfig
from ..contracts import UEOperationResult
from ._internal.bridge import RuntimeUDPBridge
from .sessions import UERuntimeSessionsClient


def _editor_binary(ue_root: Path) -> Path:
    relative = (
        "Engine/Binaries/Win64/UnrealEditor.exe"
        if os.name == "nt"
        else "Engine/Binaries/Linux/UnrealEditor"
    )
    return ue_root / relative


class UERuntimeClient:
    def __init__(
        self,
        config: UEClientConfig,
        assets: UEAssetsClient,
    ) -> None:
        self._config = config
        self._processes: dict[int, subprocess.Popen[Any]] = {}
        self.sessions = UERuntimeSessionsClient(
            assets,
            bridge=RuntimeUDPBridge(
                config.runtime_host,
                config.runtime_port,
            ),
            input_host=config.runtime_host,
            input_port=config.runtime_port,
        )

    def launch_editor(
        self,
        *,
        map_path: str = "",
        extra_args: Sequence[str] = (),
        dry_run: bool = False,
    ) -> dict[str, Any]:
        ue_root = self._config.ue_root
        project_file = self._config.project_file
        if ue_root is None:
            return UEOperationResult.failure(
                "runtime.launch_editor",
                "ue_root is not configured",
            ).to_dict()
        if project_file is None or not project_file.is_file():
            return UEOperationResult.failure(
                "runtime.launch_editor",
                "project_path does not resolve to an existing "
                ".uproject file",
            ).to_dict()
        editor = _editor_binary(ue_root)
        if not editor.is_file():
            return UEOperationResult.failure(
                "runtime.launch_editor",
                f"Unreal Editor was not found: {editor}",
            ).to_dict()

        command = [str(editor), str(project_file)]
        if str(map_path or "").strip():
            command.append(str(map_path).strip())
        command.extend(
            [
                (
                    "-ExecCmds=WebControl.StartServer "
                    f"{self._config.port}"
                ),
                "-NoSplash",
                "-Log",
                (
                    "-A3GameRuntimeInputPort="
                    f"{self._config.runtime_port}"
                ),
            ]
        )
        command.extend(str(item) for item in extra_args)
        payload = {
            "command": command,
            "cwd": str(project_file.parent),
            "map_path": str(map_path or ""),
            "remote_control_url": self._config.remote_url,
            "runtime_input_host": self._config.runtime_host,
            "runtime_input_port": self._config.runtime_port,
            "dry_run": dry_run,
        }
        if dry_run:
            return UEOperationResult.success(
                "runtime.launch_editor",
                payload=payload,
            ).to_dict()

        creationflags = 0
        if os.name == "nt":
            creationflags = subprocess.CREATE_NEW_PROCESS_GROUP
        try:
            process = subprocess.Popen(
                command,
                cwd=project_file.parent,
                creationflags=creationflags,
                start_new_session=os.name != "nt",
            )
        except Exception as exc:
            return UEOperationResult.failure(
                "runtime.launch_editor",
                f"{type(exc).__name__}: {exc}",
                payload=payload,
            ).to_dict()
        self._processes[process.pid] = process
        payload["process_id"] = process.pid
        return UEOperationResult.success(
            "runtime.launch_editor",
            artifacts=[
                {
                    "type": "unreal_editor_process",
                    "path": str(editor),
                    "state": "running",
                    "process_id": process.pid,
                }
            ],
            payload=payload,
        ).to_dict()

    def stop_editor(
        self,
        process_id: int,
    ) -> dict[str, Any]:
        process = self._processes.get(int(process_id))
        if process is None:
            return UEOperationResult.failure(
                "runtime.stop_editor",
                "UEClient can stop only Editor processes that it "
                f"launched; unknown process_id: {process_id}",
            ).to_dict()
        if process.poll() is None:
            process.terminate()
        self._processes.pop(process.pid, None)
        return UEOperationResult.success(
            "runtime.stop_editor",
            payload={"process_id": process.pid},
        ).to_dict()

    def launch_packaged(self, executable: str | Path, *, extra_args: Sequence[str] = (),
                        dry_run: bool = False) -> dict[str, Any]:
        """Launch an archived game and track ownership for stop_editor."""
        executable = Path(executable).resolve()
        if not executable.is_file():
            return UEOperationResult.failure("runtime.launch_packaged", f"Executable not found: {executable}").to_dict()
        command = [str(executable), *map(str, extra_args)]
        payload = {"command": command, "cwd": str(executable.parent), "dry_run": dry_run}
        if not dry_run:
            try:
                process = subprocess.Popen(command, cwd=executable.parent)
            except Exception as exc:
                return UEOperationResult.failure("runtime.launch_packaged", f"{type(exc).__name__}: {exc}", payload=payload).to_dict()
            self._processes[process.pid] = process
            payload["process_id"] = process.pid
        return UEOperationResult.success("runtime.launch_packaged", payload=payload).to_dict()
