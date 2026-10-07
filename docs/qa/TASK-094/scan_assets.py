"""TASK-094 bounded static asset inventory. Reads no Saved/.git or UE binaries."""
from __future__ import annotations

import csv
import json
import re
from collections import Counter, defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / "docs/assets/TASK-094"
QA = ROOT / "docs/qa/TASK-094"
FIELDS = [
    "asset_id", "category", "priority", "player_use", "gameplay_ids", "runtime_package",
    "content_path", "file_state", "file_bytes", "reference_state", "reference_evidence",
    "editable_sources", "source_pairing", "source_file_state", "source_bytes",
    "provenance_records", "existing_manifest_fingerprint", "licence_state", "release_blocker",
    "disposition", "candidate_package", "scale_skeleton", "materials_textures", "collision",
    "lod_nanite", "owner_task", "owner_visual_status", "current_ue_validation", "clearance_status", "notes",
]


def read_json(rel: str):
    return json.loads((ROOT / rel).read_text(encoding="utf-8-sig"))


def compact(value):
    return json.dumps(value, ensure_ascii=False, separators=(",", ":"))


def package(value: str) -> str:
    return value.split(".", 1)[0] if value.startswith(("/Game/", "/Engine/")) else value


def file_fact(rel: str):
    path = ROOT / rel
    if not path.is_file():
        return "MISSING", 0
    with path.open("rb") as stream:
        pointer = stream.read(64).startswith(b"version https://git-lfs.github.com/spec/v1")
    return ("LFS_POINTER" if pointer else "PRESENT"), path.stat().st_size


def strings(value, trail=""):
    if isinstance(value, dict):
        for key, child in value.items():
            yield from strings(child, trail + "/" + str(key))
    elif isinstance(value, list):
        for index, child in enumerate(value):
            yield from strings(child, trail + "/" + str(index))
    elif isinstance(value, str):
        yield trail, value


def family_owner(path: str):
    if path.startswith("/Game/Characters/") or "/Campaign/" in path or "/TASK-055/" in path or "/TASK-070/Equipment/" in path:
        return "TASK-095", "character_motion", "P0"
    if "/TASK-077/" in path:
        return "TASK-096", "stonehold", "P0"
    if "/NaturalWorld/" in path or "/Animals/" in path or "/Nature/" in path:
        return "TASK-097", "natural_route", "P1"
    if "axe" in path.lower() or "bow" in path.lower():
        return "TASK-095", "equipment", "P0"
    if path.startswith("Resources/Audio/"):
        return "TASK-099", "audio", "P1"
    return "TASK-098", "camp_prop_ui", "P0"


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    QA.mkdir(parents=True, exist_ok=True)
    rows = {}
    evidence = defaultdict(set)
    fragments = []
    mappings = {}
    source_only = []

    def add(ref, why, state="STATIC_LITERAL_REFERENCE"):
        ref = package(ref)
        if ref.startswith("/Engine/"):
            return
        if ref not in rows:
            owner, category, priority = family_owner(ref)
            suffix = ".umap" if ref.rsplit("/", 1)[-1].startswith("L_") else ".uasset"
            content = "Content/" + ref.removeprefix("/Game/") + suffix if ref.startswith("/Game/") else ref
            present, size = file_fact(content)
            rows[ref] = dict.fromkeys(FIELDS, "UNKNOWN") | {
                "asset_id": "ue:" + ref if ref.startswith("/Game/") else "resource:" + ref,
                "category": category, "priority": priority, "player_use": category,
                "gameplay_ids": "UNRESOLVED", "runtime_package": ref, "content_path": content,
                "file_state": present, "file_bytes": size, "reference_state": state,
                "editable_sources": "UNRESOLVED", "source_pairing": "UNKNOWN",
                "provenance_records": "UNRESOLVED", "existing_manifest_fingerprint": "NOT_COMPUTED",
                "licence_state": "UNKNOWN", "release_blocker": "SOURCE_OR_LICENCE_UNKNOWN",
                "disposition": "REUSE_PENDING_REVIEW", "candidate_package": "UNRESOLVED",
                "owner_task": owner, "owner_visual_status": "NOT_RUN",
                "current_ue_validation": "NOT_RUN", "clearance_status": "WAITING_084_092",
                "notes": "Static reference or manifest only; does not establish current live actor use or package dependency closure.",
            }
        evidence[ref].add(why)

    # Runtime code only. Tests, Save, and task-only inline test fixtures are excluded.
    source_files = sorted(p for p in (ROOT / "Source/Hearthward").rglob("*")
                          if p.suffix in {".cpp", ".h"} and "Tests" not in p.parts and "Save" not in p.parts)
    for path in source_files:
        rel = path.relative_to(ROOT).as_posix()
        for line_no, line in enumerate(path.read_text(encoding="utf-8-sig").splitlines(), 1):
            if line.lstrip().startswith("//"):
                continue
            for match in re.finditer(r'"(/Game/[^"\s]+)"', line):
                ref = match.group(1)
                if ref.endswith(("/", "_")) or "%" in ref:
                    fragments.append({"fragment": ref, "source": f"{rel}:{line_no}"})
                else:
                    add(ref, f"{rel}:{line_no}")

    data_files = ["Resources/Data/gameplay.json", "Resources/Data/animal_motion.json"]
    for rel in data_files:
        for trail, value in strings(read_json(rel)):
            if value.startswith("/Game/"):
                add(value, rel + "#" + trail, "STATIC_DATA_REFERENCE")

    manifest_rel = "docs/assets/TASK-028/asset-manifest.json"
    manifest = read_json(manifest_rel)
    for asset in manifest["assets"]:
        source_only.append({"asset_id": asset["id"], "editable_source": asset.get("source"),
                            "manifest_runtime_package": asset.get("ue_asset"), "manifest_status": asset.get("status"),
                            "provenance": asset.get("provenance"), "current_live_use": "NOT_RUN"})
        refs = [asset.get("ue_asset")]
        material = asset.get("materials_textures") or {}
        if isinstance(material, dict):
            refs += material.get("materials", []) + material.get("textures", [])
        for ref in filter(None, refs):
            mappings[package(ref)] = asset
            add(ref, manifest_rel + "#" + asset["id"], "HISTORICAL_IMPORT_MANIFEST")

    # The few scoped directories include authored/dynamically built references and dependency candidates.
    # They are explicitly marked, since textual scanning cannot prove a binary map's references.
    scoped_dirs = [
        "Content/Hearthward/Campaign", "Content/Characters/Hero/UE5", "Content/Characters/Brother/UE5",
        "Content/Characters/Hero/AnimationV2", "Content/Characters/Brother/Animation",
        "Content/Hearthward/Assets/NaturalWorld/Rebuild", "Content/Hearthward/Assets/Demo",
        "Content/Hearthward/Assets/TASK-070/Equipment", "Content/Hearthward/Assets/TASK-077",
        "Content/Hearthward/Nature",
    ]
    for rel in scoped_dirs:
        for path in sorted((ROOT / rel).rglob("*.uasset")):
            ref = "/Game/" + path.relative_to(ROOT / "Content").with_suffix("").as_posix()
            add(ref, "bounded directory inventory:" + rel, "SCOPED_PACKAGE_CANDIDATE")

    # Runtime R3 profiles provide an exact mesh/clip-to-source chain, including existing fingerprints.
    for species in read_json("Resources/Data/animal_motion.json")["species"]:
        slug = species["slug"]
        base = f"art_source/TASK-051/animal_motion/Hearthward/animal_motion_20260930/assets/motion/{slug}"
        provenance = ["art_source/TASK-004/Tripo/动物/SOURCE.md", "docs/qa/TASK-051/source_catalog.json",
                      "docs/qa/TASK-051/import_report.json", base + "/animation_manifest.json"]
        refs = [(species["mesh"], base + f"/SK_{slug}.fbx", species.get("skeletal_sha256", "UNKNOWN")),
                (species["skeleton"], base + f"/AS_{slug}.blend", species.get("source_blend_sha256", "UNKNOWN"))]
        refs += [(clip["asset"], base + "/clips/" + clip["name"] + ".fbx", clip.get("source_sha256", "UNKNOWN"))
                 for clip in species["clips"].values()]
        for ref, source, fingerprint in refs:
            add(ref, "Resources/Data/animal_motion.json#" + slug, "STATIC_DATA_REFERENCE")
            row = rows[package(ref)]
            row.update(editable_sources=source, source_pairing="EXACT_PROFILE_AND_ARCHIVED_SOURCE",
                       provenance_records=";".join(provenance), existing_manifest_fingerprint=fingerprint,
                       gameplay_ids=slug, scale_skeleton=species["skeleton"] + ";UE cm;DCC m",
                       licence_state="RECORDED_TRIPO_PAID_ACCOUNT_INPUT_RIGHTS_PENDING",
                       release_blocker="INPUT_RIGHTS_AND_OWNER_VISUAL_PENDING", disposition="REUSE_R3_NO_REBUILD_PLANNED",
                       collision="Preserve existing species hit/capsule contract; current UE inspect NOT_RUN",
                       lod_nanite="Preserve existing; no blanket Nanite change",
                       notes="303 clip set is data-referenced; no replacement is justified by this static inventory.")

    for animal in read_json("docs/qa/TASK-051/import_report.json")["animals"]:
        slug = animal["slug"]
        base = f"art_source/TASK-051/animal_motion/Hearthward/animal_motion_20260930/assets/motion/{slug}"
        for dependency in animal["mesh"]["metadata"]["dependencies"]:
            for ref in dependency["assets"]:
                add(ref, "docs/qa/TASK-051/import_report.json#" + slug + "/" + dependency["type"], "HISTORICAL_PAIRED_MESH_DEPENDENCY")
                if dependency["type"] == "skeleton":
                    continue  # Exact profile source mapping already exists.
                source = base + f"/SK_{slug}.fbx"
                if dependency["type"] == "texture":
                    source = base + f"/SK_{slug}.fbm/" + ref.rsplit("/", 1)[-1] + ".jpg"
                rows[package(ref)].update(editable_sources=source, source_pairing="EXACT_PAIRED_MESH_IMPORT_DEPENDENCY",
                    provenance_records="docs/qa/TASK-051/import_report.json;art_source/TASK-004/Tripo/动物/SOURCE.md",
                    gameplay_ids=slug, licence_state="RECORDED_TRIPO_PAID_ACCOUNT_INPUT_RIGHTS_PENDING",
                    release_blocker="INPUT_RIGHTS_AND_OWNER_VISUAL_PENDING", disposition="REUSE_R3_NO_REBUILD_PLANNED",
                    scale_skeleton=animal["skeleton"], collision="Preserve existing species physics; current UE inspect NOT_RUN",
                    lod_nanite="Preserve existing; no blanket Nanite change",
                    notes="Existing imported paired-mesh dependency; current live dependency closure NOT_RUN.")

    # Attach exact TASK-028 source metadata. Match older Demo basename to the same archived source.
    by_stem = {Path(a["source"]).stem: a for a in manifest["assets"] if a.get("source")}
    animal_sources = {a["id"].rsplit(".", 1)[-1]: a for a in manifest["assets"] if a["category"] == "animal_candidate"}
    for ref, row in rows.items():
        asset = mappings.get(ref)
        if not asset and "/Assets/Demo/" in ref:
            asset = by_stem.get(ref.rsplit("/", 1)[-1])
        if asset:
            row.update(editable_sources=asset.get("source") or "UNRESOLVED", source_pairing="EXACT_EXISTING_MANIFEST" if ref in mappings else "BASENAME_SOURCE_CANDIDATE",
                       provenance_records=asset.get("provenance") or manifest_rel, existing_manifest_fingerprint=asset.get("source_sha256", "UNKNOWN"),
                       gameplay_ids=asset.get("gameplay_id") or "UNRESOLVED", scale_skeleton=compact(asset.get("scale_axis_pivot", "UNKNOWN")),
                       materials_textures=compact(asset.get("materials_textures", "UNKNOWN")), collision=compact(asset.get("collision", "UNKNOWN")),
                       lod_nanite=compact(asset.get("lod_nanite", "UNKNOWN")), licence_state="RECORDED_TRIPO_PAID_ACCOUNT_INPUT_RIGHTS_PENDING",
                       release_blocker="INPUT_RIGHTS_AND_OWNER_VISUAL_PENDING", disposition="TEMP_VISUAL_REUSE_PENDING_REVIEW")
        if ref.startswith("/Game/Hearthward/Nature/SM_"):
            slug = ref.rsplit("SM_", 1)[-1]
            animal = animal_sources.get(slug)
            if animal:
                row.update(editable_sources=animal["source"], source_pairing="EXACT_HISTORICAL_NATURE_MAPPING",
                           provenance_records="docs/assets/TASK-048/MAPPING.md;art_source/TASK-004/Tripo/动物/SOURCE.md",
                           gameplay_ids=slug, licence_state="RECORDED_TRIPO_PAID_ACCOUNT_INPUT_RIGHTS_PENDING",
                           release_blocker="INPUT_RIGHTS_AND_OWNER_VISUAL_PENDING",
                           notes="Static Nature mesh loaded before AnimalMotion component config; skeletal R3 visual takes over supported species. Actual current actor visibility NOT_RUN.")

    # Character archives are the existing source lineage, distinct from later TASK-004 concept outputs.
    for ref, row in rows.items():
        if ref.startswith("/Game/Characters/"):
            who = "主角" if "/Hero/" in ref else "弟弟"
            archive = "medieval+knight+3d+model.zip" if who == "主角" else "dark+fantasy+armor+3d+model.zip"
            row.update(editable_sources=f"art_source/TASK-004/Tripo/{who}/{archive}", source_pairing="HISTORICAL_CHARACTER_LINEAGE",
                       provenance_records="docs/qa/TASK-055/body-weight-scale-audit.md;docs/qa/evidence/TASK-031/REPORT.md",
                       licence_state="UNKNOWN_ACCOUNT_AND_INPUT_RIGHTS", release_blocker="CHARACTER_SOURCE_LICENCE_UNKNOWN",
                       scale_skeleton="61-bone existing rig; original mesh around97.87cm, runtime display scale from code; current UE inspect NOT_RUN",
                       notes="Do not substitute TASK-004 newer generated concept FBX for current archive lineage. Motion source depends on exact clip.")
            if "/Animation" in ref:
                row["editable_sources"] += ";art_source/TASK-055/motion/knight61.fbx"
                row["source_pairing"] = "LINEAGE_CANDIDATE_CLIP_MAPPING_REQUIRES_UE"
        elif "/Campaign/" in ref:
            who = "heavy_armored_soldier" if "/Heavy/" in ref else "short_blade_soldier"
            src = list((ROOT / "art_source/TASK-004/Tripo/敌人/rigged_outputs").rglob(who + "_rigged.fbx"))
            row.update(editable_sources=";".join(p.relative_to(ROOT).as_posix() for p in src) or "UNRESOLVED",
                       source_pairing="HISTORICAL_CAMPAIGN_IMPORT_FAMILY", provenance_records="docs/qa/TASK-049/ASSETS.md;docs/qa/TASK-049/import-assets.json;art_source/TASK-004/Tripo/敌人/SOURCE.md",
                       licence_state="RECORDED_TRIPO_PAID_ACCOUNT_INPUT_RIGHTS_PENDING", release_blocker="INPUT_RIGHTS_AND_TEMP_VISUAL_PENDING",
                       scale_skeleton="Guard/Heavy Runtime mesh normalized to160cm by CampaignActor; animation from Brother via existing IK retargeter",
                       notes="Guard is also temporary archer silhouette. Civilian/rescued/protected actors use Brother mesh. Current UE inspect NOT_RUN.")
        elif "/TASK-077/" in ref:
            row.update(editable_sources="art_source/TASK-077/courtyard-v2-textured.glb;art_source/TASK-077/prepare_facade.py",
                       source_pairing="EXACT_FACADE_DERIVATION_RECORD", provenance_records="art_source/TASK-077/marble-courtyard-v2.json;docs/qa/TASK-077/facade-import.json;docs/qa/TASK-077/facade-material.json",
                       licence_state="RECORDED_GENERATION_CREDIT_AUTHORIZATION_DISTRIBUTION_UNKNOWN", release_blocker="MARBLE_DISTRIBUTION_CLEARANCE_AND_OWNER_VISUAL_PENDING",
                       scale_skeleton="Source metres; positioned by native Fortress centimetre layout", collision="NoCollision; native structure provides collision",
                       lod_nanite="Historic facade binding recorded Nanite; current UE inspect NOT_RUN", materials_textures="Source KHR_materials_unlit; M_StoneholdFacade_PBR with clock Tint; no local-light response",
                       notes="Only extracted103680-triangle facade is current code asset; rejected full worlds are source/evaluation only.")

    # Precise equipment source IDs from the existing import plan; the staging directory itself is not a source deliverable.
    for item in read_json("docs/qa/TASK-070/first-weapon-import-manifest.json")["imports"]:
        prefix = item["destination"] + "/"
        task_id = item["source"]["task_id"]
        src = f"art_source/TASK-004/Tripo/武器/outputs/{task_id}/{task_id}_pbr.fbx"
        for ref, row in rows.items():
            if ref.startswith(prefix):
                row.update(editable_sources=src, source_pairing="EXACT_EXISTING_IMPORT_PLAN", gameplay_ids=item["piece"],
                           provenance_records="docs/qa/TASK-070/first-weapon-import-manifest.json;docs/qa/TASK-070/sample-pbr-runtime-review.md;docs/qa/TASK-055/equipment-source-geometry.json",
                           licence_state="UNKNOWN_PER_ITEM_ACCOUNT_INPUT_RIGHTS", release_blocker="EQUIPMENT_LICENCE_AND_OWNER_GRIP_PENDING",
                           collision="Existing visuals imported with generate_collision=False", lod_nanite="Preserve sample settings; current UE inspect NOT_RUN")

    # Natural world's binary map dependencies cannot be established without live Asset Registry.
    for ref, row in rows.items():
        if "/NaturalWorld/Rebuild/" not in ref:
            continue
        row.update(provenance_records="scripts/world/TASK-026/rebuild_assets.py;scripts/world/TASK-026/rework_s1_tree_assets.py;resourceSummary.md",
                   source_pairing="SOURCE_FAMILY_CANDIDATE", editable_sources="art_source/TASK-026/Rebuild/",
                   notes="Bounded124-package directory inventory; static authoring-source candidate. Actual map/ExternalActor references and dependencies require read-only UE Asset Registry capture.")
        if any(key in ref for key in ["RockScan", "SM_Rock"]):
            row.update(editable_sources="art_source/TASK-004/sketchfab/岩石/中岩石/textures/d_m.jpg",
                       provenance_records="art_source/TASK-004/sketchfab/SOURCE.md;scripts/world/TASK-026/rebuild_assets.py",
                       licence_state="UNKNOWN_ORIGINAL_AUTHOR_URL_PER_ASSET_TERMS", release_blocker="SKETCHFAB_PROVENANCE_UNKNOWN",
                       notes=row["notes"] + " M_RockScan is shared by Fortress; TASK-097 sole package writer, TASK-096 read-only consumer. Mesh source archive mapping remains unresolved.")
        elif any(key in ref for key in ["Tree", "Fir", "Pine", "Island", "Shrub", "Stump"]):
            row.update(editable_sources="art_source/TASK-004/polyhaven/", provenance_records="art_source/TASK-004/polyhaven/SOURCE.md;scripts/world/TASK-026/rebuild_assets.py;scripts/world/TASK-026/rework_s1_tree_assets.py",
                       licence_state="CC0_SOURCE_RECORDED_PACKAGE_PAIRING_PENDING", release_blocker="EXACT_SOURCE_PAIR_AND_OWNER_VISUAL_PENDING")

    # Runtime image set is a scoped inventory, not proof each image is currently presented by a page.
    for folder in ["Resources/UI/Art", "Resources/UI/Atlases"]:
        for path in sorted((ROOT / folder).glob("*.png")):
            ref = path.relative_to(ROOT).as_posix()
            add(ref, "runtime resource folder:" + folder, "SCOPED_RESOURCE_CANDIDATE")
            rows[ref].update(editable_sources="art_source/ui-reference/TASK-020/;Resources/UI/art-provenance.json",
                             source_pairing="GENERATED_UI_PROVENANCE_FAMILY", provenance_records="Resources/UI/art-provenance.json",
                             licence_state="RECORDED_NATIVE_IMAGEGEN_REFERENCE_RIGHTS_PENDING", release_blocker="REFERENCE_RIGHTS_AND_OWNER_VISUAL_PENDING",
                             collision="NOT_APPLICABLE", lod_nanite="NOT_APPLICABLE", clearance_status="NOT_APPLICABLE")

    groups = defaultdict(list)
    for cue in read_json("Resources/Data/experience.json")["fixed_dialogue"]:
        groups[cue["audio_group"]].append(cue)
    for group, cues in sorted(groups.items()):
        ref = "Resources/Audio/Fixed/" + group + ".wav"
        add(ref, "Resources/Data/experience.json#" + group, "DEFINED_UNPRODUCED_AUDIO")
        rows[ref].update(asset_id="audio:" + group, gameplay_ids=";".join(c["cue_id"] for c in cues),
                         editable_sources="UNPRODUCED", source_pairing="UNPRODUCED", licence_state="UNPRODUCED",
                         provenance_records="Resources/Audio/README.md", release_blocker="VOICE_DEFERRED_OWNER_RELEASE_SCOPE_DECISION",
                         disposition="KEEP_SUBTITLES_VOICE_DEFERRED", collision="NOT_APPLICABLE", lod_nanite="NOT_APPLICABLE",
                         clearance_status="NOT_APPLICABLE", notes="Existing fixed human recording work remains deferred. No empty WAV or generated voice substitution.")

    unresolved = [
        ("archer_visual", "archer", "TASK-095", "Current Guard silhouette; dedicated approved silhouette/source unresolved"),
        ("civilian_visual", "civilian_initial_01..20;rescued_01..10;protected_01..04", "TASK-095", "Current Brother silhouette shared by all civilians"),
        ("boar_visual", "boar", "TASK-097", "Current pig source reused; new derivative/source/package unresolved"),
        ("smelter_visual", "smelter", "TASK-098", "Dedicated smelter model unresolved; existing facility presentation reused"),
        ("forge_visual", "forge", "TASK-098", "Dedicated forge model unresolved; existing facility presentation reused"),
        ("cooking_visual", "cooking", "TASK-098", "Dedicated cooking model unresolved; existing facility presentation reused"),
        ("shield_visual", "shield", "TASK-095", "Existing shield gameplay item; independent approved editable mesh source/package unresolved"),
        ("leggings_visual", "leggings", "TASK-095", "Existing leggings gameplay item; dedicated dynamic61-bone skin source unresolved"),
        ("quiver_visual", "quiver", "TASK-095", "Existing quiver gameplay item; separate approved mesh/attachment source unresolved"),
        ("action_environment_sfx", "successful gameplay transactions", "TASK-099", "No selected and cleared SFX source/package in this inventory"),
    ]
    for ident, gameplay, owner, note in unresolved:
        ref = "requirement:" + ident
        add(ref, "docs/qa/TASK-070/read-only-asset-audit.md", "UNRESOLVED_REPLACEMENT_REQUIREMENT")
        rows[ref].update(asset_id=ref, runtime_package="UNRESOLVED", content_path="UNRESOLVED", file_state="UNRESOLVED", file_bytes=0,
                         gameplay_ids=gameplay, owner_task=owner, disposition="WAIT_OWNER_SAMPLE_APPROVAL", notes=note)

    native = "native:fortress_layout"
    add(native, "Source/Hearthward/Building/HearthwardHometownFortress.cpp", "NATIVE_PROCEDURAL_VISUAL")
    rows[native].update(asset_id=native, category="stonehold", owner_task="TASK-096", gameplay_ids="prologue_relic;prologue_exit",
        runtime_package="NOT_APPLICABLE", content_path="NOT_APPLICABLE", file_state="NOT_APPLICABLE", file_bytes=0,
        editable_sources="Source/Hearthward/Building/HearthwardHometownFortress.cpp;art_source/TASK-077/export_blockout.py",
        source_pairing="EXACT_RUNTIME_COMPONENT_SOURCE", provenance_records="art_source/TASK-077/README.md;docs/qa/TASK-077/REPORT.md",
        licence_state="PROJECT_CODE_AND_ENGINE_BUILTIN_DEPENDENCIES", release_blocker="OWNER_VISUAL_AND_CLEARANCE_PENDING",
        candidate_package="UNRESOLVED", scale_skeleton="Native cm; bedroom12x10m, doorway2.8m, gallery/stairs from code",
        collision="BlockAll box components; facade NoCollision; all clearance replacement WAITING_084_092",
        lod_nanite="Individual native components; do not use blanket Nanite",
        notes="Existing Fortress geometry and material references; no synthetic static-mesh package is claimed.")

    for ref, row in rows.items():
        row["reference_evidence"] = ";".join(sorted(evidence[ref]))
        sources = row["editable_sources"].split(";")
        facts = [(s, *file_fact(s)) for s in sources if s not in {"UNRESOLVED", "UNKNOWN", "UNPRODUCED"} and not s.endswith("/")]
        row["source_file_state"] = ";".join(f"{s}={state}" for s, state, _ in facts) or "UNKNOWN_OR_FAMILY_DIRECTORY"
        row["source_bytes"] = ";".join(f"{s}={size}" for s, _, size in facts) or "NOT_APPLICABLE"
    ordered = [rows[key] for key in sorted(rows)]
    with (OUT / "ASSET_REGISTER.csv").open("w", encoding="utf-8-sig", newline="") as stream:
        writer = csv.DictWriter(stream, FIELDS)
        writer.writeheader()
        writer.writerows(ordered)

    packages = {"schema_version": 1, "task": "TASK-094", "status": "CANDIDATE_READ_ONLY_NOT_WRITE_AUTHORIZATION",
                "rules": ["Exact candidate paths still require Owner/task scope and LFS lock before editing.",
                          "No new Content path invented for unresolved replacements; candidate_package stays UNRESOLVED.",
                          "No .umap or ExternalActors permission granted; binary dependency closure NOT_RUN.",
                          "G0 continuous play and TASK-092 clearance are not complete; no collision replacement is frozen."],
                "tasks": {}}
    for task in ["TASK-095", "TASK-096", "TASK-097", "TASK-098", "TASK-099"]:
        own = [row for row in ordered if row["owner_task"] == task]
        exact = sorted({row["content_path"] for row in own if row["content_path"].startswith("Content/")
                        and row["content_path"].endswith(".uasset") and row["file_state"] != "MISSING"})
        packages["tasks"][task] = {"single_package_writer": task, "lock_state": "NOT_ACQUIRED",
                                  "selected_for_change": [], "content_candidates": exact,
                                  "default_read_only_reuse": sorted({row["content_path"] for row in own if row["disposition"] == "REUSE_R3_NO_REBUILD_PLANNED"}),
                                  "resource_candidates": sorted({row["content_path"] for row in own if row["content_path"].startswith("Resources/")}),
                                  "unresolved_requirements": [row["asset_id"] for row in own if row["runtime_package"] == "UNRESOLVED"],
                                  "sources": sorted({source for row in own for source in row["editable_sources"].split(";") if source not in {"UNKNOWN", "UNRESOLVED", "UNPRODUCED"}})}
    packages["shared_consumers"] = [{"package": "Content/Hearthward/Assets/NaturalWorld/Rebuild/Materials/M_RockScan.uasset",
                                      "writer": "TASK-097", "read_only_consumers": ["TASK-096"], "reason": "Fortress and natural rocks share existing material"},
                                     {"package_family": "TASK-028 furniture", "writer": "TASK-098", "read_only_consumers": ["TASK-096"], "reason": "Bedroom reuses camp furniture source; coordinate shared changes"}]
    flat = [path for value in packages["tasks"].values() for path in value["content_candidates"]]
    if len(flat) != len(set(flat)):
        raise ValueError("Candidate package has two proposed writers")
    (OUT / "PACKAGES.json").write_text(json.dumps(packages, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    for task, candidate in packages["tasks"].items():
        (OUT / (task + "-PACKAGES.json")).write_text(json.dumps({"task": task, "status": packages["status"],
            "rules": packages["rules"], **candidate}, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    (OUT / "SOURCE_CANDIDATES.json").write_text(json.dumps({"task": "TASK-094", "source_only_or_prior_import": source_only,
        "dynamic_literal_fragments_requiring_UE_resolution": fragments}, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    result = {"task": "TASK-094", "scope": "bounded static only", "rows": len(ordered),
              "reference_state": dict(Counter(r["reference_state"] for r in ordered)),
              "file_state": dict(Counter(r["file_state"] for r in ordered)),
              "source_pairing": dict(Counter(r["source_pairing"] for r in ordered)),
              "owner_task": dict(Counter(r["owner_task"] for r in ordered)),
              "candidate_packages_unique_single_writer": len(flat) == len(set(flat)),
              "source_cpp_h_files_read": len(source_files), "source_only_manifest_rows": len(source_only),
              "fixed_cues": sum(len(v) for v in groups.values()), "fixed_audio_groups": len(groups),
              "hashes_computed": 0, "ue_current_validation": "NOT_RUN", "binary_map_dependency_closure": "NOT_RUN",
              "excluded_roots": ["Saved", ".git", "Runtime", "Source/Hearthward/Save", "Resources/UI/Fonts"]}
    (QA / "static-scan-results.json").write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(result, ensure_ascii=False))


if __name__ == "__main__":
    main()
