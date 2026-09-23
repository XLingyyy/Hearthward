"""Inventory the checked-in TASK-004 Tripo FBX candidates for TASK-028.

This script only reads source assets. It never imports into Unreal or edits the
TASK-004 source tree. Run from the Hearthward repository root with Python 3.13.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[3]
SOURCE = ROOT / "art_source/TASK-004/Tripo"
MANIFEST = ROOT / "docs/assets/TASK-028/asset-manifest.json"

CATEGORIES = {
    "房屋建筑部件": ("building_part", "house"),
    "妙妙道具": ("prop", "props"),
    "室内家具": ("furniture", "furniture"),
    "篝火": ("facility", "facilities"),
    "敌人": ("enemy_candidate", "enemies"),
    "动物": ("animal_candidate", "animals"),
}

GAMEPLAY = {
    "stone_bone_axe": "axe",
    "wood_table": "workbench",
    "campfire": "campfire",
}

APPLICATION = {
    "building_part": ("apply", "TASK-028-house in the natural camp"),
    "furniture": ("scene_prop", "TASK-028-house; no new furniture interaction"),
    "facility": ("apply", "existing campfire building ID and restore path"),
    "prop": ("defer", "No matching gameplay item/use state; stone_bone_axe is the exception"),
    "enemy_candidate": ("defer", "No compatible enemy animation and behavior integration validated"),
    "animal_candidate": ("defer", "No matching animal behavior and animation integration validated"),
}


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        while block := stream.read(1024 * 1024):
            digest.update(block)
    return digest.hexdigest()


def slug(path: Path) -> str:
    stem = path.stem
    for suffix in ("_model", "_rigged"):
        if stem.endswith(suffix):
            return stem[: -len(suffix)]
    return stem


def source_rows() -> list[dict]:
    rows = []
    for folder, (category, ue_group) in CATEGORIES.items():
        directory = SOURCE / folder
        assert directory.is_dir(), directory
        candidates = sorted(directory.rglob("*.fbx"))
        by_slug: dict[str, list[Path]] = {}
        for path in candidates:
            by_slug.setdefault(slug(path), []).append(path)
        for name, variants in sorted(by_slug.items()):
            primary = next((p for p in variants if p.stem.endswith("_model")), variants[0])
            disposition, location = APPLICATION[category]
            if name == "stone_bone_axe":
                disposition, location = "apply", "equipped weapon in the player's existing axe state"
            target = GAMEPLAY.get(name)
            planned = (
                f"/Game/Hearthward/Assets/TASK-028/{ue_group}/{name}/SM_{name}"
                if category not in {"animal_candidate", "enemy_candidate"} else None
            )
            rows.append({
                "id": f"t028.{category}.{name}",
                "category": category,
                "source": rel(primary),
                "source_sha256": sha256(primary),
                "variants": [
                    {"path": rel(p), "sha256": sha256(p),
                     "kind": "rigged" if p.stem.endswith("_rigged") else "static"}
                    for p in variants
                ],
                "provenance": rel(directory / "SOURCE.md"),
                "rights": "See linked TASK-004 SOURCE.md; input-image rights remain Owner responsibility",
                "gameplay_id": target,
                "planned_ue_asset": planned,
                "ue_asset": None,
                "materials_textures": "FBX embedded/dependent materials to inspect in UE",
                "scale_axis_pivot": "unverified",
                "collision": "unverified",
                "lod_nanite": "unverified; decide from measured geometry and use",
                "disposition": disposition,
                "application_location": location,
                "status": "source_available",
                "verification": [],
            })
    return rows


def missing_equipment_rows() -> list[dict]:
    gameplay = json.loads((ROOT / "Resources/Data/gameplay.json").read_text(encoding="utf-8"))
    rows = []
    for item in gameplay["items"]:
        if "slot" not in item or item["id"] in {"axe"}:
            continue
        rows.append({
            "id": f"t028.missing_equipment.{item['id']}",
            "category": "equipment_gap",
            "source": None,
            "source_sha256": None,
            "variants": [],
            "provenance": None,
            "rights": None,
            "gameplay_id": item["id"],
            "planned_ue_asset": None,
            "ue_asset": None,
            "materials_textures": None,
            "scale_axis_pivot": None,
            "collision": None,
            "lod_nanite": None,
            "disposition": "blocked",
            "application_location": f"equipment slot {item['slot']}; source model required",
            "status": "source_missing",
            "verification": [],
        })
    return rows


def expected() -> dict:
    return {
        "schema_version": 1,
        "task": "TASK-028",
        "baseline_commit": "28e7c52e10e3988a19ca1e4b7e4fcf86ab2f3b93",
        "status_definitions": ["source_missing", "source_available", "imported", "applied", "verified"],
        "assets": source_rows() + missing_equipment_rows(),
    }


def check(manifest: dict) -> None:
    actual = manifest["assets"]
    ids = [row["id"] for row in actual]
    assert len(ids) == len(set(ids)), "duplicate asset IDs"
    expected_rows = {row["id"]: row for row in expected()["assets"]}
    assert set(ids) == set(expected_rows), "source or gameplay coverage changed"
    for row in actual:
        reference = expected_rows[row["id"]]
        assert row["source"] == reference["source"], row["id"]
        assert row["source_sha256"] == reference["source_sha256"], row["id"]
        assert row["gameplay_id"] == reference["gameplay_id"], row["id"]
        assert row["status"] in manifest["status_definitions"], row["id"]
        if row["status"] in {"imported", "applied", "verified"}:
            assert row["ue_asset"], row["id"]
    print(f"TASK-028 inventory OK: {len(actual)} entries, "
          f"{sum(bool(r['source']) for r in actual)} source candidates, "
          f"{sum(r['status'] == 'source_missing' for r in actual)} source gaps")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    if args.check:
        check(json.loads(MANIFEST.read_text(encoding="utf-8")))
    else:
        assert not MANIFEST.exists(), "Refusing to overwrite a reviewed manifest"
        MANIFEST.parent.mkdir(parents=True, exist_ok=True)
        MANIFEST.write_text(json.dumps(expected(), ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        check(json.loads(MANIFEST.read_text(encoding="utf-8")))


if __name__ == "__main__":
    main()
