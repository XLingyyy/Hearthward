"""Read-only current campaign coverage; no UE, saves, assets, or economy writes."""
import csv
import json
from pathlib import Path
from collections import Counter

ROOT = Path(__file__).resolve().parents[3]
OUTPUT = ROOT / "docs/qa/TASK-100"
Q_SOURCE = "Source/Hearthward/Campaign/HearthwardCampaignQuests.cpp"
W_SOURCE = "Source/Hearthward/Campaign/HearthwardCampaignWorld.cpp"
I_SOURCE = "Source/Hearthward/Campaign/HearthwardCampaignInteraction.cpp"
S_SOURCE = "Source/Hearthward/Campaign/HearthwardCampaignState.cpp"


def read(path):
    return (ROOT / path).read_text(encoding="utf-8-sig")


def source_lines(path, needle):
    return [{"file": path, "line": i + 1, "text": line.strip()}
            for i, line in enumerate(read(path).splitlines()) if needle in line]


def location(row):
    xy = row.get("xy", row.get("spawn", row.get("center")))
    return {"xy_m": xy, "xy_ue_cm": [value * 100 for value in xy] if xy else None,
            "z": "NOT_FROZEN; actual Campaign Ground/Position must be loaded"}


def main():
    data = json.loads(read("Resources/Data/gameplay.json"))
    c = data["campaign"]
    locations = {r["id"]: r for r in c["locations"]}
    quest_ids = {r["id"] for r in c["quests"]}
    people = {r["id"]: r for r in c["people"]}
    zones = {r["id"]: r for r in c["zones"]}
    items = {r["id"] for r in data["items"]}
    reward_ids = [q["reward"]["id"] for q in c["quests"]]
    rescue_quest_ids = [p for q in c["quests"] for p in q["rescued_people"]]
    errors = []
    for key in ("quests", "zones", "enemies", "people", "locations", "reinforcements"):
        ids = [r["id"] for r in c[key]]
        if len(ids) != len(set(ids)):
            errors.append({"table": key, "error": "DUPLICATE_ID"})
    if len(reward_ids) != len(set(reward_ids)):
        errors.append({"table": "quests.reward.id", "error": "DUPLICATE_REWARD_FACT"})
    if len(rescue_quest_ids) != len(set(rescue_quest_ids)):
        errors.append({"table": "quests.rescued_people", "error": "DUPLICATE_RESCUE_OWNERSHIP"})

    quests = []
    for index, q in enumerate(c["quests"]):
        if q["location"] not in locations:
            errors.append({"id": q["id"], "error": "UNKNOWN_LOCATION"})
        for parent in q["requires"]:
            if parent not in quest_ids:
                errors.append({"id": q["id"], "error": "UNKNOWN_PARENT", "parent": parent})
        for p in q["rescued_people"]:
            if p not in people or people[p]["type"] != "rescuable":
                errors.append({"id": q["id"], "error": "UNKNOWN_RESCUABLE_PERSON", "person": p})
        for item in q["reward"]["items"]:
            if item not in items:
                errors.append({"id": q["id"], "error": "UNKNOWN_REWARD_ITEM", "item": item})
        expected_xp = data["progression"]["experienceRewards"]["main_milestone" if q["category"] == "main" else "side_quest"]
        if q["reward"]["xp"] != expected_xp:
            errors.append({"id": q["id"], "error": "CONFIG_XP_RUNTIME_KIND_DIFFERENCE"})
        quests.append({
            "config_pointer": f"/campaign/quests/{index}", "definition": q,
            "location": location(locations[q["location"]]),
            "conditions_source": source_lines(Q_SOURCE, f'Id==TEXT("{q["id"]}")'),
            "available_rule": "Active + Gameplay; previous main requires Claimed (legacy main_01 exception); prologue permits main_01 only; side quests have no extra clue/discovery gate here",
            "knowledge_state": "SOURCE_GATE_RISK" if q["category"] == "side" else "KNOWN_STORY_ACTION; observe actual phase/target before play",
            "reward_uniqueness": "Claim: Safe/!Busy/current epoch/Available + conditions; reject Claimed or RewardFacts reward:id; storage preflight; XP/storage rollback on failure",
            "runtime": "NOT_RUN_NORMAL_INPUT", "coverage": "STATIC_TRACED",
        })

    base = []
    for index, e in enumerate(c["enemies"]):
        if e["zone"] not in zones:
            errors.append({"id": e["id"], "error": "UNKNOWN_ZONE"})
        if e.get("patrol") and not any(p["id"] == e["patrol"] for p in zones[e["zone"]]["patrols"]):
            errors.append({"id": e["id"], "error": "UNKNOWN_PATROL"})
        base.append({"config_pointer": f"/campaign/enemies/{index}", "definition": e, "location": location(e),
                     "runtime_group": "base", "runtime_generation": 1,
                     "generation_note": "Configuration generation=0 is a declaration; State Enemy constructs Combat.Generation=1",
                     "clear_condition": "Combat.Health<=0, including lethal and nonlethal execution",
                     "reward_fact": f"defeat:{e['id']}:1", "source": S_SOURCE + "#Enemy/Initialize/Total/Cleared", "runtime": "NOT_RUN_NORMAL_INPUT"})
    zone_rows = []
    for index, z in enumerate(c["zones"]):
        enemies = [e for e in c["enemies"] if e["zone"] == z["id"]]
        counts = dict(Counter(e["kind"] for e in enemies))
        if counts != z["counts"]:
            errors.append({"id": z["id"], "error": "DECLARED_COUNTS_DIFFER_FROM_STABLE_ENEMIES", "actual": counts})
        zone_rows.append({"config_pointer": f"/campaign/zones/{index}", "definition": z, "actual_counts": counts,
                          "base_ids": [e["id"] for e in enemies], "location": location(z),
                          "interaction_id": "loc_" + z["id"], "persistent_control_id": z["id"],
                          "visual_tag": "CampaignNode:loc_" + z["id"],
                          "spatial_implementation": "4 AHearthwardTask028CampHouse at configured houses with I*90 rotation; same native cylinder/cube flag presentation",
                          "declared_choices": "2 authored entrances; observe existing clockwise patrols and 3-second first-point pause; existing combat/retreat/escort choices; choose zone order without quest-claim victory gate",
                          "unverified": ["Terrain Z", "Normal walking/retreat paths", "Sightlines and actual cover", "Distinct approved zone sample"],
                          "source": W_SOURCE + "#RefreshActors; " + I_SOURCE + "#Use(loc_)", "runtime": "NOT_RUN_NORMAL_INPUT"})

    groups = []
    for index, group in enumerate(c["reinforcements"]):
        if group["zone"] not in zones:
            errors.append({"id": group["id"], "error": "UNKNOWN_REINFORCEMENT_ZONE"})
        groups.append({"config_pointer": f"/campaign/reinforcements/{index}", "definition": group,
                       "persistent_state_key": group["zone"], "runtime_group": "reinforcement",
                       "terminal_rule": "pending->spawned once on existing successful zone alarm while base survivors remain; pending->cancelled when base zone cleared first; never repeat",
                       "members": [{"definition": e, "location": location(e), "generation": 1,
                                    "reward_fact": f"defeat:{e['id']}:1"} for e in group["enemies"]],
                       "source": S_SOURCE + "#ResolveUntriggered/RegisterReinforcement; " + W_SOURCE + "#Tick", "runtime": "NOT_RUN_NORMAL_INPUT"})
    rescues = []
    for index, p in enumerate(c["people"]):
        if p["type"] != "rescuable":
            continue
        owner = [q for q in c["quests"] if p["id"] in q["rescued_people"]]
        if len(owner) != 1:
            errors.append({"id": p["id"], "error": "RESCUE_QUEST_OWNER_COUNT", "count": len(owner)})
        rescues.append({"config_pointer": f"/campaign/people/{index}", "definition": p, "location": location(p),
                        "quest_id": owner[0]["id"] if owner else None, "location_id": owner[0]["location"] if owner else None,
                        "stages": ["uncontacted", "following", "waiting", "arrived"],
                        "interaction": "Actual player within <260 cm, Safe, !Busy; explicit follow/wait; existing brother escort after contact",
                        "arrival": "Actual rescued actor in CampAt and player not InCombat; RecordRescue single transaction",
                        "reward_fact": "rescue:" + p["id"], "xp_kind": "first_rescue",
                        "population": "20+unique arrived IDs, maximum ten; fifth distinct rescue grants reward:rescue5 only once",
                        "source": S_SOURCE + "#Initialize; " + I_SOURCE + "#Use/AssignEscort; " + W_SOURCE + "#Tick", "runtime": "NOT_RUN_NORMAL_INPUT"})

    counts = {"main": sum(q["category"] == "main" for q in c["quests"]), "side": sum(q["category"] == "side" for q in c["quests"]),
              "zones": len(zones), "base": len(base), "reinforcement_groups": len(groups),
              "reinforcement_members": sum(len(g["members"]) for g in groups), "rescues": len(rescues),
              "initial_people": sum(p["type"] == "initial" for p in c["people"]),
              "protected_people": sum(p["type"] == "protected_enemy_civilian" for p in c["people"]),
              "prologue_enemies_excluded_from_victory": len(c["prologue_enemies"]), "field_enemies_excluded_from_victory": len(c["field_enemies"])}
    result = {"schema_version": 1, "task": "TASK-100", "date": "2026-10-07",
              "reference_head": "6fcf5c22e965f0f7409438f19bc7b09e96ffb058", "implementation": "shared uncommitted iteration sources",
              "evidence_level": "STATIC_SOURCE_AND_CONFIG", "normal_input_route": "NOT_RUN", "owner_zone_sample": "NOT_RUN",
              "selected_for_change": [], "content_edits": "NONE", "rules_economy_save_edits": "NONE", "hashes_computed": 0,
              "counts": counts, "structural_errors": errors, "quests": quests, "zones": zone_rows, "base_enemies": base,
              "reinforcements": groups, "rescues": rescues, "other_people": [p for p in c["people"] if p["type"] != "rescuable"],
              "excluded_enemies": {"prologue": c["prologue_enemies"], "field": c["field_enemies"]},
              "unique_rewards": c["unique_rewards"], "locations": c["locations"], "travel_stations": c["travel_stations"],
              "knowledge_policy": {"discover_radius_cm_base": data["tuning"]["discoverRadius"], "fog_radius_cm": data["tuning"]["fogRadius"],
                                   "journal": "JournalEntryKnown delegates QuestAvailable; all side quests available after prologue",
                                   "map": "Place markers require Discovered; tracked goal requires Discovered+Explored Known; route_trace dots require available main_05+Known",
                                   "remaining_enemies": "Strict >95% cleared; map groups surviving base/reinforcement into 50m cells, not individual dots",
                                   "hud_risk": "QuestGuidance Resolve has no Discovered/Explored gate before Visible=true; Task100 real-controller fixture queued for RED",
                                   "source": [Q_SOURCE + "#Available", "Source/Hearthward/UI/HearthwardScreenContent.cpp#JournalEntryKnown",
                                              "Source/Hearthward/UI/HearthwardScreenWorldMap.cpp#ComposeWorldMap", "Source/Hearthward/UI/HearthwardQuestGuidance.cpp#Resolve"]},
              "victory": {"rules": "K=N from base plus actually spawned reinforcements, Flags count4, both reinforcement states terminal; no main quest claim required",
                          "N_possible": [80, 84, 88], "threshold_counts_show_remaining": [77, 80, 84], "permanent": "No base/reinforcement refresh; field-only refresh retains generation uniqueness",
                          "second_camp": "ReclaimHometown transaction shares storage/tier/ration pool, local building/workplace records; gifts access+2beds+campfire; unique hearth_blade; rollback on gift failure",
                          "postgame_main08": "home_storage + home_work + home_continued; current Save's home_saved integration is not inspected or changed by this audit"},
              "reuse_filters": ["Hearthward.Campaign049.RegistryAndVictory", "Hearthward.Campaign049.ContentContract",
                               "Hearthward.Campaign067.FlagUsesActualActiveSeconds", "Hearthward.Campaign067.VictoryActivatesActualSecondCamp",
                               "Hearthward.Campaign067.FailedSurvivalRejectsAutomaticVictory",
                               "Hearthward.Campaign067.SecondStage.PromptUsesWeightedControlProgress",
                               "Hearthward.Campaign067.SecondStage.OccupiedGiftsStayOnLegalGroundOrRollback"],
              "new_filters": ["Hearthward.Iteration.Task100.Fixture.UnknownSideGoalIsHidden", "Hearthward.Iteration.Task100.Fixture.PrologueActionGoalRemainsVisible"]}
    OUTPUT.mkdir(parents=True, exist_ok=True)
    (OUTPUT / "CONFIG_COVERAGE.json").write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    rows = []
    def add(kind, stable_id, pointer, loc=None, requires=None, trigger=None, success=None, reward=None, source=None, owner=None):
        xy = loc.get("xy_m") if loc else None
        rows.append({"kind": kind, "stable_id": stable_id, "config_pointer": pointer,
                     "xy_m": json.dumps(xy, ensure_ascii=False) if xy else "", "z": "NOT_FROZEN",
                     "requires": json.dumps(requires, ensure_ascii=False) if requires else "",
                     "trigger": trigger or "", "success": success or "", "reward_fact": reward or "",
                     "source": source or "", "gap_owner": owner or "TASK-100 normal input; TASK-092 clearance", "runtime": "NOT_RUN_NORMAL_INPUT"})
    for q in quests:
        d = q["definition"]
        add(d["category"], d["id"], q["config_pointer"], q["location"], d["requires"], d["trigger"], d["success"], d["reward"]["id"], Q_SOURCE,
            "TASK-087/090 knowledge; TASK-100 legal completion; TASK-095--099 presentation")
    for z in zone_rows:
        add("zone", z["definition"]["id"], z["config_pointer"], z["location"], None, "Existing clear/occupied + E at loc_ then 5 active seconds", "Persistent zone ID in Flags", None, z["source"], "TASK-100 zone sample; TASK-092 clearance; TASK-095--099 art")
    for e in base:
        add("base_enemy", e["definition"]["id"], e["config_pointer"], e["location"], None, "Actual perception/patrol", e["clear_condition"], e["reward_fact"], e["source"], "TASK-095 enemy silhouette; TASK-100 legitimate combat")
    for g in groups:
        add("reinforcement_group", g["definition"]["id"], g["config_pointer"], None, None, g["definition"]["trigger"], g["terminal_rule"], None, g["source"])
        for i, e in enumerate(g["members"]):
            add("reinforcement_enemy", e["definition"]["id"], g["config_pointer"] + f"/enemies/{i}", e["location"], [g["definition"]["id"]], "group spawned once", "Health<=0", e["reward_fact"], g["source"])
    for p in rescues:
        add("rescue", p["definition"]["id"], p["config_pointer"], p["location"], None, p["interaction"], p["arrival"], p["reward_fact"], p["source"], "TASK-092 physical escort; TASK-095 civilian; TASK-100 remaining rescues")
    with (OUTPUT / "CONFIG_COVERAGE.csv").open("w", encoding="utf-8-sig", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=list(rows[0]))
        writer.writeheader();writer.writerows(rows)
    print(json.dumps({"counts": counts, "coverage_rows": len(rows), "structural_errors": errors, "hashes_computed": 0}, ensure_ascii=False))
    if errors:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
