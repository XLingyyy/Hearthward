"""Recompute the TASK-046 proposal. This is a design model, not UE validation."""
from collections import Counter
from fractions import Fraction as F
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
DATA = json.loads((ROOT / "docs/planning/TASK-046/economy-candidate.json").read_text(encoding="utf-8"))
RECIPES = {next(iter(r["outputs"])): r for r in DATA["recipes"]}


def raw_cost(cost, visiting=()):
    total = Counter()
    for item, count in cost.items():
        assert item in DATA["resources"] and count > 0
        assert item not in visiting, f"Circular input: {item}"
        recipe = RECIPES.get(item)
        if recipe:
            assert recipe["outputs"][item] == 1
            for raw, qty in raw_cost(recipe["inputs"], visiting + (item,)).items():
                total[raw] += qty * count
        else:
            total[item] += count
    return total


def simulate(workers, patches, minutes, activity=1, meals=True):
    """Shared regional work; reserve finite source at start, output on completion.

    One minute steps are exact for this table's 3/4/5-worker completion times.
    Brothers start at hunger 25, eat whole meals below 25 only when meals=True.
    The explicit meal assumption represents user-supplied meals in camp, not AI.
    """
    labor, ration = DATA["labor"], DATA["rations"]
    stock = [labor["safe_source_units_per_patch"]] * patches
    due = [None] * patches
    cursor = 0
    reserved = produced = regenerated = work = 0
    active = False
    pool = F(ration["initial_points"])
    low_pool = pool
    hunger = [F(25), F(25)]
    ate = 0
    first_failed_meal = None
    for now in range(minutes + 1):
        if now:
            pool = max(F(0), pool - F(ration["daily_base_points"], DATA["units"]["day_W"]))
            if active:
                work += workers
                if work == labor["forage_worker_W_per_unit"]:
                    produced += 1
                    pool += F(str(ration["food_points"]["draft_wild_food"]))
                    active = False
                    work = 0
            if meals:
                for index in range(2):
                    hunger[index] -= F(ration["hunger_per_normal_day_per_brother"] * activity, DATA["units"]["day_W"])
                    if hunger[index] < 25:
                        if pool < ration["points_per_meal"]:
                            first_failed_meal = now
                            break
                        pool -= ration["points_per_meal"]
                        hunger[index] += ration["hunger_per_meal"]
                        ate += 1
            low_pool = min(low_pool, pool)
            if first_failed_meal is not None:
                break
        if now == minutes:
            break  # Stable end boundary, before starting another batch.
        for index in range(patches):
            if due[index] is not None and due[index] <= now:
                assert stock[index] == 0
                stock[index] = labor["safe_source_units_per_patch"]
                regenerated += stock[index]
                due[index] = None
        if not active:
            for offset in range(patches):
                index = (cursor + offset) % patches
                if stock[index]:
                    cursor = index
                    stock[index] -= 1
                    reserved += 1
                    active = True
                    if not stock[index]:
                        due[index] = now + labor["source_refresh_W_after_empty"]
                    break
    assert patches * labor["safe_source_units_per_patch"] + regenerated == sum(stock) + reserved
    assert reserved == produced + int(active)
    return {"workers": workers, "patches": patches, "elapsed_W": now,
            "produced": produced, "reserved": reserved, "in_progress": int(active),
            "remaining_worker_W": labor["forage_worker_W_per_unit"] - work if active else 0,
            "whole_meals": ate, "pool": float(pool), "lowest_pool": float(low_pool),
            "first_failed_meal_W": first_failed_meal}


def main():
    tiers = DATA["camp_tiers"]
    facilities = DATA["facilities"]
    assert [t["tier"] for t in tiers] == list(range(1, 9))
    assert [t["radius_m"] for t in tiers] == list(range(50, 121, 10))
    assert tiers[-1]["cumulative_hp_bonus"] == 100
    assert tiers[-1]["cumulative_stamina_bonus"] == 50
    by_id = {(f["id"], f["level"]): f for f in facilities}
    assert len(by_id) == 12
    assert set(by_id) == {(name, level) for name in ("workbench", "smelter", "forge", "cooking") for level in (1, 2, 3)}
    # Upgrade gates and the recipes needed to pay them must be available first.
    for tier in tiers:
        for condition in tier["conditions"]:
            name, value = condition.split(":")
            if (name, int(value)) in by_id:
                assert by_id[name, int(value)]["minimum_camp_tier"] < tier["tier"]
        for item in tier["cost"]:
            if item in RECIPES:
                recipe = RECIPES[item]
                assert by_id[recipe["facility"], recipe["level"]]["minimum_camp_tier"] < tier["tier"]
    for facility in facilities:
        for item in facility["incremental_cost"]:
            if item in RECIPES:
                recipe = RECIPES[item]
                dependency = by_id[recipe["facility"], recipe["level"]]
                assert dependency["minimum_camp_tier"] <= facility["minimum_camp_tier"]
                assert (recipe["facility"], recipe["level"]) != (facility["id"], facility["level"])

    costs = {}
    for facility in facilities:
        raw = raw_cost(facility["incremental_cost"])
        costs[f'{facility["id"]}_{facility["level"]}'] = {
            "raw": dict(raw), "collection_A_seconds": sum(raw.values()) * F(5, 2)}
    assert costs["workbench_1"]["collection_A_seconds"] == 180
    for name in ("smelter", "forge", "cooking"):
        assert costs[f"{name}_1"]["collection_A_seconds"] == 900
    tier_raw = {str(t["tier"]): dict(raw_cost(t["cost"])) for t in tiers}
    total_raw = Counter()
    for cost in tier_raw.values():
        total_raw.update(cost)
    for cost in costs.values():
        total_raw.update(cost["raw"])
    slice_raw = raw_cost(DATA["slice"]["initial_workbench"]) + raw_cost(DATA["slice"]["first_upgrade"])
    assert slice_raw == {"wood": 120, "stone": 24}
    assert sum(slice_raw.values()) * F(5, 2) == 360

    daily = []
    for workers, activity in ((3, 1), (4, 1), (4, 2), (5, 2)):
        output = F(workers * 1440, DATA["labor"]["forage_worker_W_per_unit"]) * F(5, 2)
        meals = F(2 * 50 * activity * 5, 40)
        daily.append({"workers": workers, "activity": activity, "output_points": float(output),
                      "meal_points": float(meals), "net_points": float(output - 24 - meals)})
    assert [r["net_points"] for r in daily] == [-6.5, 3.5, -9, 1]
    horizon = 60 * DATA["units"]["day_W"]
    normal = simulate(4, 3, horizon)
    high = simulate(5, 4, horizon, activity=2)
    assert normal["produced"] == 60 * 16 and normal["first_failed_meal_W"] is None
    assert high["produced"] == 60 * 20 and high["first_failed_meal_W"] is None
    short = simulate(4, 3, horizon, activity=2)
    assert short["first_failed_meal_W"] is not None
    limited = simulate(5, 3, horizon, meals=False)
    assert limited["produced"] < 60 * 20  # More labor cannot bypass source cooldown.
    sleep = simulate(4, 3, 480, meals=False)
    assert sleep["produced"] == 5 and sleep["reserved"] == 6
    assert sleep["pool"] == 104.5 and sleep["remaining_worker_W"] == 240
    assert [5, 4 + 3, 3 + 6] == [5, 7, 9]
    paid = Counter()
    for level in (1, 2, 3):
        paid.update(by_id["workbench", level]["incremental_cost"])
    refunds = {item: qty * 4 // 5 for item, qty in paid.items()}
    assert all(0 <= refunds[item] < paid[item] for item in paid)
    assert [qty * 4 // 5 for qty in range(6)] == [0, 0, 1, 2, 3, 4]
    result = {"status": "PASS_DESIGN_MODEL_ONLY", "facility_costs": costs, "camp_tier_raw_costs": tier_raw,
              "camp8_plus_all_four_facilities3_raw": dict(total_raw),
              "all_level1_facilities_collection_minutes": float(sum(costs[f"{name}_1"]["collection_A_seconds"] for name in ("workbench", "smelter", "forge", "cooking")) / 60),
              "daily_budget": daily, "normal_60_days": normal, "high_activity_60_days": high,
              "four_workers_high_activity_shortfall": short, "five_workers_only_three_sources": limited,
              "sleep_8h_no_automatic_meals": sleep, "workbench3_paid": dict(paid), "workbench3_refund": refunds}
    print(json.dumps(result, ensure_ascii=False, indent=2, default=float))


if __name__ == "__main__":
    main()
