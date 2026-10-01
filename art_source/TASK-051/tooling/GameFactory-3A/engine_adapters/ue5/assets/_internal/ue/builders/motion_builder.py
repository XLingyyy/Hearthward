"""Motion import script builder."""

from __future__ import annotations

from engine_adapters.ue5.assets._internal.types import ImportRequest

from ..scripts.import_scripts import _build_motion_import_script, _motion_import_name

from .base import ScriptBuilder


class MotionImportScriptBuilder(ScriptBuilder):
    def build_import_script(self, request: ImportRequest, dest_path: str) -> str:
        return _build_motion_import_script(
            request.src_path,
            dest_path,
            str(request.options.get("skeleton_asset_path", "")).strip(),
            str(request.options.get("avatar_name", "")).strip(),
            sample_rate=int(request.options.get("custom_sample_rate", 0)),
        )


__all__ = ["MotionImportScriptBuilder", "_motion_import_name"]
