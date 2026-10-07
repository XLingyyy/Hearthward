"""Read the four actual locomotion clips; emit pose evidence without changing assets.

Run inside the root-owned Editor through UEClient's public Python operation.
TASK099_FOOT_OUTPUT_DIR may be supplied by the caller; otherwise use a fresh
project .agent-local QA directory. No setters, playback, traces, or asset saves.
"""

from datetime import datetime, timezone
import json
from pathlib import Path
from uuid import uuid4

import unreal


CLIPS = (
    ("/Game/Characters/Hero/AnimationV2/A_Hero_Walk", "/Script/Hearthward.HearthwardCharacter"),
    ("/Game/Characters/Hero/AnimationV2/A_Hero_Run", "/Script/Hearthward.HearthwardCharacter"),
    ("/Game/Characters/Brother/Animation/A_Brother_Walk", "/Script/Hearthward.HearthwardCompanionFixture"),
    ("/Game/Characters/Brother/Animation/A_Brother_Run", "/Script/Hearthward.HearthwardCompanionFixture"),
)
FEET = ("foot_l", "foot_r")


def vector(value):
    return [float(value.x), float(value.y), float(value.z)]


def metadata(value):
    return {"path": value.get_path_name(), "class": value.get_class().get_path_name()}


def inspect_clip(package, actor_class_path):
    row = {"package": package, "status": "NOT_READ", "frames": []}
    try:
        asset = unreal.load_asset(package)
        if not isinstance(asset, unreal.AnimSequence):
            raise RuntimeError("Actual asset is missing or is not AnimSequence")
        actor_class = unreal.load_class(None, actor_class_path)
        if actor_class is None:
            raise RuntimeError("Actual runtime character class is unavailable")
        defaults = unreal.get_default_object(actor_class)
        mesh_component = defaults.get_editor_property("mesh")
        mesh = mesh_component.get_skeletal_mesh_asset()
        capsule = defaults.get_editor_property("capsule_component")
        relative = mesh_component.get_relative_transform()
        half_height = float(capsule.get_unscaled_capsule_half_height())
        options = unreal.AnimPoseEvaluationOptions(
            evaluation_type=unreal.AnimDataEvalType.COMPRESSED,
            should_retarget=True,
            extract_root_motion=False,
            incorporate_root_motion_into_pose=False,
            optional_skeletal_mesh=mesh,
            retrieve_additive_as_full_pose=True,
            evaluate_curves=False,
        )
        library = unreal.AnimationLibrary
        extensions = unreal.AnimPoseExtensions
        last_frame = int(library.get_num_frames(asset))
        row.update({
            "actual_asset": metadata(asset),
            "skeleton": metadata(asset.get_editor_property("skeleton")),
            "runtime_character_class": actor_class_path,
            "runtime_mesh": metadata(mesh),
            "length_seconds": float(library.get_sequence_length(asset)),
            "last_frame_index": last_frame,
            "notify_count": len(library.get_animation_notify_events(asset)),
            "mesh_relative_translation": vector(relative.translation),
            "mesh_relative_scale": vector(relative.scale3d),
            "capsule_half_height_cm": half_height,
            "evaluation": "COMPRESSED, runtime mesh, retargeted, runtime root lock",
            "coordinate_space": "AnimPoseSpaces.WORLD is mesh component space, without actor/world transform",
        })
        for frame in range(last_frame + 1):
            time = float(library.get_time_at_frame(asset, frame))
            # GetTimeAtFrame follows the sequence's sampled frame rate. The
            # AtFrame wrapper instead uses its source DataModel frame rate.
            pose = extensions.get_anim_pose_at_time(asset, time, options)
            if not extensions.is_valid(pose):
                raise RuntimeError("Invalid pose at sampled frame " + str(frame))
            names = {str(name) for name in extensions.get_bone_names(pose)}
            if not set(FEET).issubset(names):
                raise RuntimeError("Missing actual foot bone; identity transform is not evidence")
            points = {}
            for bone in FEET + tuple(name for name in ("ball_l", "ball_r", "root") if name in names):
                transform = extensions.get_bone_pose(pose, bone, unreal.AnimPoseSpaces.WORLD)
                position = transform.translation
                actor_position = relative.transform_location(position)
                points[bone] = {
                    "component_space_cm": vector(position),
                    "actor_space_cm": vector(actor_position),
                    "height_above_default_capsule_bottom_cm": float(actor_position.z + half_height),
                }
            row["frames"].append({"index": frame, "time_seconds": time, "bones": points})
        row["feet"] = {}
        # The final sampled key is retained as loop seam evidence. It is not a
        # second event at time zero and is excluded from cyclic extrema.
        loop_frames = row["frames"][:-1]
        for bone in FEET:
            heights = [item["bones"][bone]["component_space_cm"][2] for item in loop_frames]
            minima = [index for index, height in enumerate(heights)
                      if height <= heights[index - 1] and height <= heights[(index + 1) % len(heights)]
                      and (height < heights[index - 1] or height < heights[(index + 1) % len(heights)])]
            row["feet"][bone] = {
                "component_z_min_cm": min(heights),
                "component_z_max_cm": max(heights),
                "local_minimum_frames": minima,
                "local_minimum_times_seconds": [loop_frames[index]["time_seconds"] for index in minima],
                "contact_status": "CANDIDATE_ONLY; no physical ground/sole evidence sampled",
            }
        row["status"] = "READ"
    except Exception as error:
        row["error"] = str(error)
    return row


def main():
    now = datetime.now(timezone.utc)
    run = "foot-poses-" + now.strftime("%Y%m%dT%H%M%SZ-") + uuid4().hex[:8]
    destination = Path(globals().get("TASK099_FOOT_OUTPUT_DIR") or
                       str(Path(unreal.Paths.project_dir()).resolve() / ".agent-local" / "qa" / "TASK-099" / run))
    rows = [inspect_clip(*clip) for clip in CLIPS]
    result = {
        "task": "TASK-099", "run_id": run, "created_utc": now.isoformat(),
        "engine_version": unreal.SystemLibrary.get_engine_version(),
        "status": "READ" if all(row["status"] == "READ" for row in rows) else "PARTIAL_OR_FAILED",
        "read_only_asset_queries": True, "changed_asset_packages": [], "clips": rows,
        "limits": [
            "Actual pose sampling only; no live actor movement, collision trace, physical material or audio validation.",
            "Default capsule bottom is an authored reference plane, not an observed ground hit.",
            "Local minima are candidates; no Notify timestamp is authorized or written by this script.",
            "The component-space foot joint is an ankle reference, not a sampled shoe sole.",
            "Raw frame evidence and loop seam remain intact; no speed, period or equal-phase event inference.",
        ],
    }
    destination.mkdir(parents=True, exist_ok=False)
    output = destination / "poses.json"
    output.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    unreal.log("TASK099_FOOT_POSE_REPORT=" + str(output))


main()
