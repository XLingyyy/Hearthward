"""Stable Unreal asset reflection operations for UEClient v1."""

from __future__ import annotations

from typing import Any

from .._internal.transport import PythonRPCTransport
from ..assets import UEAssetsClient
from ..contracts import UEOperationResult


def _inspect_asset_script(asset_path: str, sample_animation: bool = False) -> str:
    return f"""
import unreal

asset_path = {asset_path!r}
asset = unreal.load_asset(asset_path)
if asset is None:
    result = {{
        "ok": False,
        "asset_path": asset_path,
        "error": "asset not found",
    }}
else:
    asset_class = asset.get_class()
    result = {{
        "ok": True,
        "asset_path": asset_path,
        "name": asset.get_name(),
        "class": asset_class.get_name(),
        "class_path": asset_class.get_path_name(),
        "package": asset.get_outermost().get_name(),
    }}
    if isinstance(asset, unreal.SkeletalMesh):
        bounds = asset.get_imported_bounds()
        v = bounds.box_extent
        physics = asset.get_editor_property('physics_asset')
        result['skeletal_mesh'] = {{'imported_size_cm':[2*v.x,2*v.y,2*v.z], 'physics_asset':physics.get_path_name() if physics else None}}
    if {sample_animation!r} and isinstance(asset, unreal.AnimSequence):
        try:
            length = unreal.AnimationLibrary.get_sequence_length(asset)
            count = unreal.AnimationLibrary.get_num_frames(asset)
            evaluation = unreal.AnimPoseEvaluationOptions()
            evaluation.set_editor_property('evaluation_type', unreal.AnimDataEvalType.COMPRESSED)
            evaluation.set_editor_property('should_retarget', False)
            samples = []
            for fraction in (0.0, 0.25, 0.5, 0.75, 1.0):
                pose = unreal.AnimPoseExtensions.get_anim_pose_at_time(asset, length * fraction, evaluation)
                if not unreal.AnimPoseExtensions.is_valid(pose):
                    raise RuntimeError('Animation pose evaluation returned an invalid pose')
                transforms = {{}}
                for bone in unreal.AnimPoseExtensions.get_bone_names(pose):
                    transform = unreal.AnimPoseExtensions.get_bone_pose(pose, bone, unreal.AnimPoseSpaces.WORLD)
                    v = transform.translation
                    q = transform.rotation
                    scale = transform.scale3d
                    transforms[str(bone)] = {{'translation_cm':[v.x,v.y,v.z],'rotation_xyzw':[q.x,q.y,q.z,q.w],'scale':[scale.x,scale.y,scale.z]}}
                samples.append({{'fraction':fraction,'bones':transforms}})
            skeleton = asset.get_editor_property('skeleton')
            result['animation'] = {{'verified':True,'duration_s':float(length),'num_frames':int(count),'skeleton':skeleton.get_path_name(),'evaluation':'compressed','samples':samples}}
        except Exception as exc:
            result['animation'] = {{'verified':False,'error':type(exc).__name__ + ': ' + str(exc)}}
"""


class UEReflectionClient:
    def __init__(
        self,
        transport: PythonRPCTransport,
        assets: UEAssetsClient,
    ) -> None:
        self._transport = transport
        self._assets = assets

    def inspect_artifact(
        self,
        artifact_id: str,
        *,
        live: bool = True,
        sample_animation: bool = False,
    ) -> dict[str, Any]:
        record = self._assets._service.artifacts.get(
            artifact_id
        )
        if record is None:
            return UEOperationResult.failure(
                "reflection.inspect_artifact",
                f"Unknown artifact_id: {artifact_id}",
            ).to_dict()
        payload = {
            "artifact": record.to_dict(),
            "live": live,
        }
        if not live:
            return UEOperationResult.success(
                "reflection.inspect_artifact",
                artifacts=[record.to_dict()],
                payload=payload,
            ).to_dict()
        try:
            inspection = self._transport.execute_json(
                _inspect_asset_script(record.backend_path, sample_animation),
                timeout=60,
            )
        except Exception as exc:
            return UEOperationResult.failure(
                "reflection.inspect_artifact",
                f"{type(exc).__name__}: {exc}",
                payload=payload,
            ).to_dict()
        if not isinstance(inspection, dict):
            return UEOperationResult.failure(
                "reflection.inspect_artifact",
                "Unreal reflection returned an invalid payload",
                payload=payload,
            ).to_dict()
        payload["inspection"] = inspection
        if not inspection.get("ok"):
            return UEOperationResult.failure(
                "reflection.inspect_artifact",
                str(
                    inspection.get("error")
                    or "Unreal asset inspection failed"
                ),
                payload=payload,
            ).to_dict()
        return UEOperationResult.success(
            "reflection.inspect_artifact",
            artifacts=[record.to_dict()],
            payload=payload,
        ).to_dict()
