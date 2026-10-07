"""Read actual referenced locomotion notifies in UE; write QA JSON only.

The root agent runs this through UEClient's public Python execution API.
UE 5.8 AnimationBlueprintLibrary.h exposes ScriptName=AnimationLibrary,
GetAnimationNotifyEvents, GetAnimNotifyEventTriggerTime/Duration, and the
read-only sequence/track queries used below. No asset setters or saves run.
"""

from datetime import datetime, timezone
import json
from pathlib import Path
from uuid import uuid4

import unreal


PACKAGES = (
    "/Game/Characters/Hero/AnimationV2/A_Hero_Walk",
    "/Game/Characters/Hero/AnimationV2/A_Hero_Run",
    "/Game/Characters/Brother/Animation/A_Brother_Walk",
    "/Game/Characters/Brother/Animation/A_Brother_Run",
)


def object_metadata(value):
    if value is None:
        return None
    return {"path": value.get_path_name(), "class": value.get_class().get_path_name()}


def inspect_sequence(package):
    row = {"package": package, "status": "NOT_READ", "notify_count": None, "notifies": None}
    try:
        asset = unreal.load_asset(package)
        if not isinstance(asset, unreal.AnimSequence):
            row["error"] = "Missing asset or actual type is not AnimSequence"
            row["actual_asset"] = object_metadata(asset)
            return row
        library = unreal.AnimationLibrary
        events = library.get_animation_notify_events(asset)
        row["actual_asset"] = object_metadata(asset)
        row["length_seconds"] = library.get_sequence_length(asset)
        row["frames"] = library.get_num_frames(asset)
        row["skeleton"] = object_metadata(asset.get_editor_property("skeleton"))
        tracks = [str(name) for name in library.get_animation_track_names(asset)]
        row["foot_track_names"] = [name for name in tracks if any(part in name.lower() for part in ("foot", "toe"))]
        row["notifies"] = []
        for index, event in enumerate(events):
            notify = event.get_editor_property("notify")
            state = event.get_editor_property("notify_state_class")
            row["notifies"].append({
                "index": index,
                "name": str(event.get_editor_property("notify_name")),
                "event_kind": "NOTIFY_STATE" if state is not None else "NOTIFY" if notify is not None else "NAMED_EVENT",
                "notify": object_metadata(notify),
                "notify_state": object_metadata(state),
                "trigger_time_seconds": library.get_anim_notify_event_trigger_time(event),
                "duration_seconds": library.get_anim_notify_event_duration(event),
                "trigger_weight_threshold": event.get_editor_property("trigger_weight_threshold"),
                "trigger_on_follower": event.get_editor_property("trigger_on_follower"),
                "trigger_chance": event.get_editor_property("notify_trigger_chance"),
            })
        row["notify_count"] = len(events)
        row["status"] = "READ"
    except Exception as error:
        row["error"] = str(error)
    return row


def main():
    now = datetime.now(timezone.utc)
    run = "animation-contacts-" + now.strftime("%Y%m%dT%H%M%SZ-") + uuid4().hex[:8]
    destination = Path(unreal.Paths.project_dir()).resolve() / ".agent-local" / "qa" / "TASK-099" / run
    rows = [inspect_sequence(package) for package in PACKAGES]
    result = {
        "task": "TASK-099",
        "run_id": run,
        "created_utc": now.isoformat(),
        "engine_version": unreal.SystemLibrary.get_engine_version(),
        "status": "READ" if all(row["status"] == "READ" for row in rows) else "PARTIAL_OR_FAILED",
        "read_only_asset_queries": True,
        "changed_asset_packages": [],
        "clips": rows,
        "limits": [
            "Metadata only; no animation playback, foot contact or audio validation.",
            "A failed property/API read is NOT_READ, never interpreted as zero notifies.",
            "No Notify or NotifyState is added, removed, renamed or saved.",
            "Notify names alone do not prove physical contact or material mapping.",
        ],
    }
    destination.mkdir(parents=True, exist_ok=False)
    output = destination / "metadata.json"
    output.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    unreal.log("TASK099_ANIMATION_CONTACT_REPORT=" + str(output))


main()
