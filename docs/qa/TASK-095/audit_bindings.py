"""Bounded source/config audit for TASK-095--098; does not load or edit UE packages."""
import csv
import json
from pathlib import Path
import re
import struct

ROOT = Path(__file__).resolve().parents[3]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8-sig")


def write(task, data):
    path = ROOT / "docs" / "qa" / task / "TECHNICAL_AUDIT.json"
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def package_file(package):
    return "Content/" + package.removeprefix("/Game/").split(".")[0] + ".uasset"


def source_location(path, needle):
    lines = read(path).splitlines()
    return [{"file": path, "line": i + 1, "text": line.strip()}
            for i, line in enumerate(lines) if needle in line]


def base(task):
    candidate = json.loads(read(f"docs/assets/TASK-094/{task}-PACKAGES.json"))
    # The 094 per-task file wraps the actual candidate group.
    group = candidate.get("candidate", candidate.get("package_scope", candidate))
    if "content_candidates" not in group:
        group = json.loads(read("docs/assets/TASK-094/PACKAGES.json"))["tasks"][task]
    return {
        "schema_version": 1, "task": task, "date": "2026-10-07",
        "evidence_level": "STATIC_SOURCE_AND_CONFIG_ONLY",
        "source_head": "6fcf5c22e965f0f7409438f19bc7b09e96ffb058",
        "working_tree": "shared uncommitted iteration changes; runtime results bind separately",
        "runtime_validation": "NOT_RUN", "owner_visual": "NOT_RUN",
        "source_candidate_manifest": f"docs/assets/TASK-094/{task}-PACKAGES.json",
        "content_candidate_count": len(group.get("content_candidates", [])),
        "resource_candidate_count": len(group.get("resource_candidates", [])),
        "selected_for_change": group.get("selected_for_change", []),
        "lock_state": group.get("lock_state", "NOT_ACQUIRED"),
        "hashes_computed": 0,
    }


def clip_rows(character, directory, names):
    return [{"role": name, "package": f"/Game/Characters/{character}/{directory}/A_{character}_{name}",
             "file": package_file(f"/Game/Characters/{character}/{directory}/A_{character}_{name}"),
             "file_exists": (ROOT / package_file(f"/Game/Characters/{character}/{directory}/A_{character}_{name}")).is_file(),
             "ue_skeleton_pairing": "NOT_RUN"} for name in names]


def main():
    gameplay = json.loads(read("Resources/Data/gameplay.json"))
    interface = json.loads(read("Resources/UI/interface.json"))
    animal = json.loads(read("Resources/Data/animal_motion.json"))
    register = list(csv.DictReader((ROOT / "docs/assets/TASK-094/ASSET_REGISTER.csv").open(encoding="utf-8-sig", newline="")))

    a = base("TASK-095")
    hero_path = "Source/Hearthward/Animation/HearthwardHeroAnimInstance.cpp"
    brother_path = "Source/Hearthward/Animation/HearthwardBrotherAnimInstance.cpp"
    hero_names = re.findall(r'TEXT\("([^\"]+)"\)', re.search(r'const TCHAR\* Names\[\] = \{(.*?)\};', read(hero_path), re.S).group(1))
    brother_names = re.findall(r'TEXT\("([^\"]+)"\)', re.search(r'for \(const TCHAR\* Name : \{(.*?)\}\)', read(brother_path), re.S).group(1))
    a["locomotion_and_action_bindings"] = clip_rows("Hero", "AnimationV2", hero_names) + clip_rows("Brother", "Animation", brother_names)
    a["campaign_enemy_clip_slots"] = {"guard": ["Idle", "Walk", "Run", "Idle", "Attack", "Idle"],
                                      "archer": "Guard mesh and the same clip slots", "heavy": ["Idle", "Walk", "Run", "Idle", "Attack", "Idle"]}
    a["known_representation_gaps"] = ["No dedicated down/revive/draw-bow slot in these two native clip arrays",
        "Hero HeldAxe is visible only for durable equipped axe outside ranged mode; this path does not bind the other twelve weapon item IDs",
        "Campaign civilians use Brother; archer uses Guard; no independently selected approved replacement"]
    a["weapon_item_ids"] = [row["id"] for row in gameplay["items"] if row.get("slot") == "weapon"]
    a["source_evidence"] = source_location("Source/Hearthward/HearthwardCharacter.cpp", "HoldingAxe") + source_location(hero_path, "StoneAxeClipTime") + source_location(brother_path, "SetPlayRate")
    a["scope_constraint"] = "HearthwardCharacter.cpp and CampaignActor.cpp are not TASK-095 allowed write paths; source binding extensions require precise scope coordination"
    a["reuse_filters"] = ["Hearthward.Equipment055.HeldAxeFollowsCurrentInstanceAndMode", "Hearthward.Equipment055.StoneAxeLightUsesClipPhaseAndPhysicalBlade", "Hearthward.Equipment055.StoneAxeHeavyUsesCurrentMoveAndPhysicalBlade"]
    write("TASK-095", a)

    a = base("TASK-096")
    a["geometry_input"] = "docs/planning/TASK-084-103/ROUTE_MANIFEST.json#prologue"
    a["approved_direction"] = "TASK-077 mountain stonehold, Owner direction confirmation 2026-10-05; final near-field material sample unreviewed"
    a["formal_facade"] = [row["runtime_package"] for row in register if row["source_pairing"] == "EXACT_FACADE_DERIVATION_RECORD"]
    a["facade_material"] = "Historic source KHR_materials_unlit; WorldClock tint; not a demonstrated lit/PBR near-field material"
    a["near_field_and_collision"] = "Native HometownFortress blocking parts; facade/furniture NoCollision; exact ground Z and natural following remain NOT_RUN"
    a["lifecycle_source_evidence"] = source_location("Source/Hearthward/Gameplay/HearthwardWorldPresentation.cpp", "LevelAddedToWorld") + source_location("Source/Hearthward/Experience/HearthwardPresentationComponent.cpp", "OnSnapshotRestored")
    a["reuse_filters"] = ["Hearthward.Hometown077.BedroomAndEscapeClearance"]
    a["clearance_status"] = "WAITING_084_092"
    write("TASK-096", a)

    a = base("TASK-097")
    a["profiles"] = [{"species": p["slug"], "mesh": p["mesh"], "skeleton": p.get("skeleton"),
                      "clip_count": len(p["clips"]), "runtime_pairing": "NOT_RUN"} for p in animal["species"]]
    a["clip_count"] = sum(x["clip_count"] for x in a["profiles"])
    a["aliases"] = {"deer": "stag_a", "boar": "pig"}
    a["natural_rebuild_family_only"] = [{"package": x["runtime_package"], "source_candidate": x["editable_sources"],
                                        "licence": x["licence_state"]} for x in register if x["source_pairing"] == "SOURCE_FAMILY_CANDIDATE"]
    a["source_evidence"] = source_location("Source/Hearthward/Animals/HearthwardAnimalMotionComponent.cpp", "GetSkeleton()") + source_location("Source/Hearthward/Nature/HearthwardNatureActor.cpp", 'Def==TEXT("boar")')
    a["reuse_filters"] = ["Hearthward.Animals.FrameBoundaryAndEscape", "Hearthward.Farming062.IndividualGrowthAndProductProgress", "Hearthward.Farming062.CropGeometryConsumesCalendarStage"]
    a["clearance_status"] = "WAITING_084_092"
    a["unverified"] = ["Binary map dependency closure", "Live LOD/Nanite/material settings", "New boar/crop visual sample", "Same-condition frame time and streaming evidence"]
    write("TASK-097", a)

    a = base("TASK-098")
    a["facilities"] = [{"id": b["id"], "name": b["name"], "footprint_half_extent_cm": b["extent"],
                        "visual_parts": b["parts"], "collision": "Native root box; each visual part NoCollision",
                        "runtime_workplace_check": "NOT_RUN"} for b in gameplay["buildings"]]
    assets, errors = [], []
    for key, definition in interface["assets"].items():
        file = definition.get("file")
        if not file:
            continue
        path = ROOT / "Resources/UI" / file
        row = {"key": key, "file": "Resources/UI/" + file, "uv": definition.get("uv"), "file_exists": path.is_file()}
        if path.is_file():
            with path.open("rb") as stream:
                header = stream.read(33)
            if header[:8] == b"\x89PNG\r\n\x1a\n":
                width, height = struct.unpack(">II", header[16:24])
                row.update(width=width, height=height, png_color_type=header[25])
                if row["uv"]:
                    x, y, w, h = row["uv"]
                    row["uv_in_bounds"] = min(x, y) >= 0 and min(w, h) > 0 and x + w <= width and y + h <= height
                    if not row["uv_in_bounds"]:
                        errors.append({"key": key, "error": "UV_OUT_OF_BOUNDS"})
            else:
                errors.append({"key": key, "error": "NOT_PNG_HEADER"})
        else:
            errors.append({"key": key, "error": "FILE_MISSING"})
        assets.append(row)
    a["interface_assets"] = assets
    a["static_file_uv_errors"] = errors
    a["item_icon_keys_missing_from_interface"] = [{"id": row["id"], "icon": row.get("icon")} for row in gameplay["items"] if row.get("icon") and row["icon"] not in interface["assets"]]
    a["shared_axe_icon_weapon_ids"] = [row["id"] for row in gameplay["items"] if row.get("slot") == "weapon" and row.get("icon") == "axe"]
    a["visual_gap"] = "smelter/cooking/campfire share one campfire mesh; forge/warehouse_access/workbench share table; medical_area/bed share bed"
    a["source_evidence"] = source_location("Source/Hearthward/Building/HearthwardBuildingComponent.cpp", "SetCollisionEnabled(ECollisionEnabled::NoCollision)") + source_location("Source/Hearthward/Campaign/HearthwardCampaignWorld.cpp", "PresentLabor")
    a["reuse_filters"] = ["Hearthward.Camp059.WorkerAssignmentPresentation", "Hearthward.Farming062.CropProgressAndHarvestCapacity", "Hearthward.Farming062.PenProductiveAnimalAndPausePresentation"]
    a["runtime_and_owner_unverified"] = ["Working position and actual interaction", "DPI and large text", "Alpha edge/mip and small icon semantic clarity", "Built state and two-camp restoration", "Cook dependency closure"]
    write("TASK-098", a)
    print(json.dumps({"tasks": ["TASK-095", "TASK-096", "TASK-097", "TASK-098"],
                      "ui_asset_rows": len(assets), "file_uv_errors": errors,
                      "missing_item_icon_keys": a["item_icon_keys_missing_from_interface"],
                      "hashes_computed": 0}, ensure_ascii=False))


if __name__ == "__main__":
    main()
