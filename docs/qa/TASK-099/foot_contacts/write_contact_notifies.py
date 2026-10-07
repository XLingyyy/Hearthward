"""Root-owned, one-time writer for exactly four locked locomotion sequences.

Run only after CPU matrix completion and a successful Editor build containing
HearthwardFootContactNotify. The root executes through public UEClient Python.
All four assets must still have zero notifies; no replacement or retry cleanup.
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
TRACK = "HearthwardFootContact"
NOTIFY_CLASS = "/Script/Hearthward.HearthwardFootContactNotify"


def invariant(asset):
    library = unreal.AnimationLibrary
    return {
        "skeleton": asset.get_editor_property("skeleton").get_path_name(),
        "length_seconds": float(library.get_sequence_length(asset)),
        "frames": int(library.get_num_frames(asset)),
        "bone_tracks": [str(name) for name in library.get_animation_track_names(asset)],
        "notify_tracks": [str(name) for name in library.get_animation_notify_track_names(asset)],
    }


def read_events(asset):
    library = unreal.AnimationLibrary
    result = []
    for event in library.get_animation_notify_events(asset):
        notify = event.get_editor_property("notify")
        result.append({
            "class": notify.get_class().get_path_name() if notify else None,
            "foot_bone": str(notify.get_editor_property("foot_bone")) if notify else None,
            "trigger_time_seconds": float(library.get_anim_notify_event_trigger_time(event)),
            "trigger_weight_threshold": float(event.get_editor_property("trigger_weight_threshold")),
            "trigger_on_follower": bool(event.get_editor_property("trigger_on_follower")),
        })
    return result


def main():
    project = Path(unreal.Paths.project_dir()).resolve()
    source = project / "docs/qa/TASK-099/foot_contacts/contact-candidates-20261007-01.json"
    candidates = json.loads(source.read_text(encoding="utf-8-sig"))
    if tuple(row["package"] for row in candidates["clips"]) != PACKAGES:
        raise RuntimeError("Candidate package set differs from the four approved assets")
    notify_type = unreal.HearthwardFootContactNotify
    if notify_type.static_class().get_path_name() != NOTIFY_CLASS:
        raise RuntimeError("New compiled native Notify type is unavailable")
    prepared = []
    library = unreal.AnimationLibrary
    for row in candidates["clips"]:
        asset = unreal.load_asset(row["package"])
        if not isinstance(asset, unreal.AnimSequence):
            raise RuntimeError("Actual sequence missing: " + row["package"])
        if library.get_animation_notify_events(asset):
            raise RuntimeError("Zero-notify baseline changed; preserve existing content: " + row["package"])
        if len(row["events"]) != 4 or sorted(event["foot_bone"] for event in row["events"]) != ["foot_l", "foot_l", "foot_r", "foot_r"]:
            raise RuntimeError("Expected precisely four left/right contact candidates")
        # LFS lock verification belongs to root; this backup is its approved
        # exact local pre-write copy, not a new lock or checksum operation.
        relative = Path("Content") / (row["package"][len("/Game/"):] + ".uasset")
        backup = project / ".agent-local/qa/TASK-099/foot-locks-20261007/backup" / relative
        if not backup.is_file():
            raise RuntimeError("Approved local asset backup is missing: " + str(backup))
        for event in row["events"]:
            actual_time = float(library.get_time_at_frame(asset, event["sampled_frame"]))
            if abs(actual_time - event["time_seconds"]) > .00001:
                raise RuntimeError("Actual sampled frame time changed: " + row["package"])
            if actual_time < 0 or actual_time >= library.get_sequence_length(asset):
                raise RuntimeError("End seam must not duplicate a time-zero contact")
        prepared.append((asset, row, invariant(asset)))
    now = datetime.now(timezone.utc)
    destination = Path(globals().get("TASK099_FOOT_WRITE_OUTPUT_DIR") or
                       str(project / ".agent-local/qa/TASK-099" / ("foot-write-" + now.strftime("%Y%m%dT%H%M%SZ-") + uuid4().hex[:8])))
    # Reserve the exact fresh evidence directory before any asset mutation.
    # An existing parent is permitted; an existing destination is rejected.
    destination.mkdir(parents=True, exist_ok=False)
    output = destination / "write.json"
    # All four zero-notify/identity/frame/backup checks precede any mutation.
    for asset, row, before in prepared:
        tracks = before["notify_tracks"]
        track = tracks[0] if tracks else TRACK
        if not tracks:
            library.add_animation_notify_track(asset, track)
        for candidate in sorted(row["events"], key=lambda event: event["time_seconds"]):
            notify = notify_type(outer=asset)
            notify.set_editor_property("foot_bone", candidate["foot_bone"])
            event = unreal.AnimNotifyEvent()
            event.set_editor_property("notify", notify)
            event.set_editor_property("trigger_weight_threshold", .5)
            event.set_editor_property("trigger_on_follower", False)
            event.set_editor_property("notify_trigger_chance", 1.0)
            added = library.add_animation_notify_event_from_source(asset, candidate["time_seconds"], event, track)
            if added is None:
                raise RuntimeError("Native AnimationLibrary did not create the Notify")
        after = invariant(asset)
        for key in ("skeleton", "length_seconds", "frames", "bone_tracks"):
            if after[key] != before[key]:
                raise RuntimeError("Unrelated animation invariant changed: " + key)
        if tracks and after["notify_tracks"] != tracks:
            raise RuntimeError("Existing notify tracks changed")
        events = read_events(asset)
        if len(events) != 4 or any(event["class"] != NOTIFY_CLASS or event["trigger_on_follower"]
                                  or event["trigger_weight_threshold"] != .5 for event in events):
            raise RuntimeError("Actual notify count/type/leader/weight does not match the approved edit")
    saved = []
    for asset, row, before in prepared:
        if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=True):
            raise RuntimeError("Save failed for approved sequence: " + row["package"])
        saved.append({"package": row["package"], "before": before, "after": invariant(asset),
                      "events": read_events(asset), "status": "SAVED_CANDIDATES_RUNTIME_NOT_RUN"})
    output.write_text(json.dumps({"task": "TASK-099", "created_utc": now.isoformat(),
                                 "status": "SAVED_4_ASSETS_16_CANDIDATES", "changed_asset_packages": list(PACKAGES),
                                 "clips": saved, "runtime_ground_contact": "NOT_RUN", "runtime_audio": "NOT_RUN"},
                                ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    unreal.log("TASK099_FOOT_WRITE_REPORT=" + str(output))


main()
