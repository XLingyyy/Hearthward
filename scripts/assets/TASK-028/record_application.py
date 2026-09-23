"""Record only TASK-028 uses proven by final isolated runtime and visual QA."""

import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[3]
EVIDENCE = ROOT / "docs/qa/evidence/TASK-028"
MANIFEST = ROOT / "docs/assets/TASK-028/asset-manifest.json"
runtime = json.loads((EVIDENCE / "runtime-qa-full-route.json").read_text(encoding="utf-8"))
visual = json.loads((EVIDENCE / "visual-capture.json").read_text(encoding="utf-8"))
assert runtime["ok"] and all(runtime["checks"].values()), "Runtime QA must pass first"
assert runtime["checks"]["house_has_29_visual_parts"]
assert visual["ok"] and len(visual["captures"]) == 3, "Final visual capture required"

house_scales = {
    "thatched_roof_ridge": "(1.5, 6.3, 2.3)",
    "thatched_roof_slope": "(3.2, 3.2, 2.3), four panels",
    "wood_door": "(0.5, 2.2, 2.2), fixed open",
    "wood_floor_panel": "(2.4, 2.4, 2.1), four panels",
    "wood_stairs": "(1.5, 1.5, 1.2)",
    "wood_wall_doorway": "(0.5, 3, 3)",
    "wood_wall_solid": "(0.5, 3, 3), four panels",
    "wood_wall_window": "(0.5, 3, 3), three panels",
}
house_furniture_scales = {
    "rope_wood_bed": "(1.4, 1.4, 1.4)",
    "wood_chest": "(0.85, 0.85, 0.85)",
    "wood_chair": "(0.7, 0.7, 0.7)",
    "metal_lantern": "(0.35, 0.35, 0.35)",
}
manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
applied = 0
for row in manifest["assets"]:
    slug = row["id"].rsplit(".", 1)[-1]
    if slug in house_scales and row["category"] == "building_part":
        assert row["status"] in {"imported", "applied"}, row["id"]
        row["status"] = "applied"
        row["application_location"] = (
            "Runtime-spawned TASK-028 camp house at natural-world camp; "
            "all eight module types present, walkable door and steps")
        row["scale_axis_pivot"]["scale"] = house_scales[slug]
        row["scale_axis_pivot"]["axis"] = "UE Z-up, component orientation set in house actor"
        row["scale_axis_pivot"]["pivot"] = "FBX origin retained; bounds origin recorded above"
        row["collision"]["strategy"] = (
            "Visual mesh NoCollision; 11 task-owned box components cover floor, "
            "walls, door frame and steps; roof and open door are nonblocking")
        row["verification"][-1] = (
            "Applied to camp house; final runtime path walks in and out; "
            "three DX12 screenshots capture exterior, entry and interior. "
            "Owner visual acceptance remains open")
        if slug == "wood_floor_panel":
            row["runtime_backing_material"] = (
                "/Game/Hearthward/Assets/TASK-028/house/backing/M_FloorBacking")
        if slug == "thatched_roof_slope":
            row["runtime_backing_material"] = (
                "/Game/Hearthward/Assets/TASK-028/house/backing/M_RoofBacking")
        applied += 1
    elif slug in house_furniture_scales and row["category"] == "furniture":
        assert row["status"] in {"imported", "applied"}, row["id"]
        row["status"] = "applied"
        row["application_location"] = (
            "Runtime-spawned TASK-028 camp house scenery; no new item, "
            "storage or furniture interaction")
        row["scale_axis_pivot"]["scale"] = house_furniture_scales[slug]
        row["scale_axis_pivot"]["axis"] = "UE Z-up; house actor local orientation"
        row["scale_axis_pivot"]["pivot"] = "FBX origin retained; bounds origin recorded above"
        row["collision"]["strategy"] = "Scenic mesh NoCollision; no furniture blocking volume"
        row["verification"][-1] = (
            "Applied as house scenery and included in final DX12 visual capture; "
            "no furniture gameplay or Owner visual acceptance claimed")
        applied += 1
    elif row["id"] == "t028.furniture.wood_table":
        assert row["status"] in {"imported", "applied"}, row["id"]
        row["gameplay_id"] = "workbench"
        row["status"] = "applied"
        row["application_location"] = (
            "Existing workbench building definition; preview, completed instance, "
            "crafting proximity and restored instance")
        row["scale_axis_pivot"]["scale"] = "(1.5, 0.78, 1.6)"
        row["scale_axis_pivot"]["axis"] = "UE Z-up; existing building actor orientation"
        row["scale_axis_pivot"]["pivot"] = "FBX origin retained; bounds origin recorded above"
        row["collision"]["strategy"] = (
            "Visual mesh NoCollision; existing building root box half extent (80,45,45) cm")
        row["verification"][-1] = (
            "Final runtime QA confirmed preview, completion, crafting page, "
            "save/load and Continue Game; normal natural map lacks wood gathering")
        applied += 1
    elif row["id"] == "t028.facility.campfire":
        assert row["status"] in {"imported", "applied"}, row["id"]
        row["status"] = "applied"
        row["application_location"] = (
            "Existing campfire building definition; preview, completed and restored instances")
        row["scale_axis_pivot"]["scale"] = "(0.9, 0.9, 0.65)"
        row["scale_axis_pivot"]["axis"] = "UE Z-up; existing building actor orientation"
        row["scale_axis_pivot"]["pivot"] = "FBX origin retained; bounds origin recorded above"
        row["collision"]["strategy"] = (
            "Visual mesh NoCollision; existing building root box half extent (45,45,16) cm")
        row["verification"][-1] = (
            "Final runtime QA confirmed preview, completion, material debit, "
            "save/load and Continue Game; fire itself remains static")
        applied += 1
    elif row["id"] == "t028.prop.stone_bone_axe":
        assert row["status"] in {"imported", "applied"}, row["id"]
        row["status"] = "applied"
        row["application_location"] = (
            "Provisional HeldAxe on greybox character root while existing "
            "weapon slot contains axe and inventory contains an axe")
        row["scale_axis_pivot"]["scale"] = "(0.7, 0.7, 0.7); offset (30,35,25) cm"
        row["scale_axis_pivot"]["axis"] = "UE Z-up; final hand/socket orientation pending TASK-027"
        row["scale_axis_pivot"]["pivot"] = "FBX origin retained; bounds origin recorded above"
        row["collision"]["strategy"] = "Held visual NoCollision; character capsule unchanged"
        row["verification"][-1] = (
            "Final runtime QA confirmed equip, unequip, re-equip, save/load and "
            "Continue Game; action animation and final skeleton socket not verified")
        applied += 1

assert applied == 15, applied
MANIFEST.write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
print(f"Recorded {applied} applied source models")
