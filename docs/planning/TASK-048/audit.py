"""Offline candidate arithmetic. Does not test UE or approve proposed rules."""
from pathlib import Path
from fractions import Fraction
import json
import math

ROOT = Path(__file__).resolve().parents[3]
C = json.loads((Path(__file__).with_name('candidate.json')).read_text(encoding='utf-8'))
GAME = json.loads((ROOT / 'Resources/Data/gameplay.json').read_text(encoding='utf-8-sig'))
checks = {}


def check(name, value):
    checks[name] = bool(value)


def supply(workers, groups, days=20):
    """Reserve real food at batch start; 360 worker-minutes per item."""
    stock = [16] * groups
    due = [None] * groups
    now = Fraction(0)
    end = days * 1440
    completed = 0
    starved = Fraction(0)
    while now < end:
        for i in range(groups):
            if due[i] is not None and due[i] <= now:
                stock[i], due[i] = 16, None
        available = [i for i, count in enumerate(stock) if count]
        slot = min(available, key=lambda i: (stock[i], i)) if available else None
        if slot is None:
            next_due = min(x for x in due if x is not None)
            starved += min(next_due, end) - now
            now = next_due
            continue
        stock[slot] -= 1
        if stock[slot] == 0:
            due[slot] = now + 2880
        now += Fraction(360, workers)
        if now <= end:
            completed += 1
    return {'completed': completed, 'starved_minutes': float(starved)}


existing = {x['id'] for x in GAME['items']}
new = {x['id'] for x in C['new_items']}
check('new IDs do not overwrite 047', not existing & new and len(new) == len(C['new_items']))
check('8 wild 3 domestic 4 fish', [len(C[k]) for k in ['wildlife', 'domestic', 'fish']] == [8, 3, 4])
check('distinct species IDs', len({x['id'] for k in ['wildlife', 'domestic', 'fish'] for x in C[k]}) == 15)
check('one deer species despite two models', sum(x['asset_stem'] and x['asset_stem'].startswith('stag') for x in C['wildlife'] if x['asset_stem']) == 1)
check('missing boar asset is explicit', next(x for x in C['wildlife'] if x['id'] == 'boar')['asset_stem'] is None)
check('new recipes do not override approved recipes', not {r['id'] for r in C['recipes']} & {r['id'] for r in GAME['craftingRecipes']})
check('all recipe items defined', all(set(r['inputs']) | set(r['outputs']) <= existing | new for r in C['recipes']))
check('positive recipe quantities', all(v > 0 for r in C['recipes'] for side in ['inputs', 'outputs'] for v in r[side].values()))
check('resource outputs defined', all(r['item'] in existing | new for r in C['resources']))
reachable = {r['item'] for r in C['resources']} | {'meat', 'hide'} | {f['item'] for f in C['fish']}
reachable |= {c['output'] for c in C['crops'] if c['seed'] in reachable}
reachable |= {r['id'] for r in C['rewards']}
all_recipes = [{'inputs': r['materials'], 'outputs': r['outputs']} for r in GAME['craftingRecipes']] + C['recipes']
for _ in range(len(all_recipes)):
    before = len(reachable)
    for r in all_recipes:
        if set(r['inputs']) <= reachable:
            reachable.update(r['outputs'])
    if len(reachable) == before:
        break
check('recipe and world acquisition reachable', all(set(r['inputs']) | set(r['outputs']) <= reachable for r in C['recipes']))
early = {r['item'] for r in C['resources'] if r['min_grade'] < 2}
for _ in range(len(all_recipes)):
    for r in all_recipes:
        if set(r['inputs']) <= early:
            early.update(r['outputs'])
check('ordinary and metal pick reachable before refined ore', {'pickaxe', 'pickaxe_2', 'metal_ingot'} <= early and 'refined_ore' not in early)
resources = {r['id']: r for r in C['resources']}
check('metal tool threshold retained', resources['rich_ore_vein']['min_grade'] == 2)
check('tree two day rule retained', resources['tree']['refresh_days'] == 2)
wood = resources['fallen_branches']['capacity'] * C['starting_source_counts']['fallen_branches']
stone = resources['loose_stones']['capacity'] * C['starting_source_counts']['loose_stones']
check('bare hand workshop axe and pick recovery', wood >= 72 + 10 + 10 and stone >= 15 + 15)
four = supply(4, 3)
five = supply(5, 4)
insufficient = supply(5, 3)
check('four workers three food nodes sustain 20 days', four['completed'] == 320 and four['starved_minutes'] == 0)
check('five workers four food nodes sustain 20 days', five['completed'] == 400 and five['starved_minutes'] == 0)
check('three nodes not enough for five workers', insufficient['completed'] < 400 and insufficient['starved_minutes'] > 0)
check('four workers cover normal base and brother meals', 16 * 2.5 >= 24 + 2 * 50 / 8)
check('dangerous animals seven hit chest benchmark', all(math.isclose(a['damage'] * .95 * 7, 100, abs_tol=1e-7) for a in C['wildlife'] if a['damage']))
check('headshot deer but not bear with starter bow', 25 * 3 >= next(a['health'] for a in C['wildlife'] if a['id'] == 'deer') and 25 * 3 < next(a['health'] for a in C['wildlife'] if a['id'] == 'black_bear'))
check('flee distances preserve 50 to 80 meters', all(50 <= a['flee_m'] <= 80 for a in C['wildlife']))
check('fish pool normalized', sum(f['pool_weight'] for f in C['fish']) == 100)
check('treasure pool normalized', sum(r['weight'] for r in C['rewards']) == 100)
check('treasure retains two percent', C['fishing']['treasure_chance'] == .02)
check('8 to 12 seconds achievable in green band', min(1 + 2 + f['struggle_seconds'] for f in C['fish']) == 8 and max(1 + 4 + f['struggle_seconds'] for f in C['fish']) == 12)
check('release and hold both controllable during fish force', C['fishing']['hold_rate'] - C['fishing']['fish_rate_amplitude'] > 0 and C['fishing']['release_rate'] + C['fishing']['fish_rate_amplitude'] < 0)
check('treasure drops only existing equipment and materials', all(set(r.get('outputs', {})) <= existing for r in C['rewards']))
check('campaign uniques absent from treasure', all(not {'hearth_blade', 'amulet'} & set(r.get('outputs', {})) for r in C['rewards']))
known = next(r['knowledge'] for r in GAME['craftingRecipes'] if 'bow_rare' in r['outputs'])
check('hunter knowledge ID reused', C['rewards'][-1]['knowledge'] == known)
check('crop two three four days', sorted(c['days'] for c in C['crops']) == [2, 3, 4])
check('crop care capped at fifty percent with integer yield', all(c['base_yield'] % 4 == 0 and c['care_bonus_each'] * len(c['care_flags']) == .5 for c in C['crops']))
check('seed conservation independent of care', all(c['seed_return'] == c['seed_cost'] == 1 for c in C['crops']))
check('seeds have explicit field sources', all(c['seed'] in {r['item'] for r in C['resources']} for c in C['crops']))
check('no automatic crop loops on long skips', not C['rules']['crop_auto_replant'] and not C['rules']['crop_auto_harvest'])
feed_cost = {a['id']: 2 * a['adult_feed_daily'] * a['breeding_days'] + a['young_feed_daily'] * a['maturity_days'] for a in C['domestic']}
check('livestock real feed cost', feed_cost == {'goat': 10, 'pig': 16, 'hen': 6})
check('breeding and maturity both two days', all(a['breeding_days'] == a['maturity_days'] == 2 for a in C['domestic']))
check('pen capacities bounded and gated', [(p['camp_tier'], p['capacity']) for p in C['pens']] == [(2, 6), (4, 10), (6, 14)])
check('no livestock death products weather or offline expansion', not any(C['rules'][k] for k in ['livestock_starvation_death', 'livestock_additional_products', 'weather_affects_catch', 'offline_growth']))

result = {'status': 'PASS' if all(checks.values()) else 'FAIL', 'scope': 'Candidate arithmetic and source relationships only; UE and asset QA NOT_RUN; approval NOT_GRANTED', 'checks': checks, 'metrics': {'food_4_workers_3_nodes_20_days': four, 'food_5_workers_4_nodes_20_days': five, 'food_5_workers_3_nodes_20_days': insufficient, 'treasure_events_per_ideal_hour': [3600 / 12 * .02, 3600 / 8 * .02], 'zero_treasure_in_50_successes': .98 ** 50, 'four_spot_success_capacity': 4 * 24, 'treasure_events_per_four_spot_clear': 96 * .02, 'feed_per_new_adult_with_parent_maintenance': feed_cost}}
target = ROOT / 'docs/qa/TASK-048/calculation.json'
target.parent.mkdir(parents=True, exist_ok=True)
target.write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
print(json.dumps(result, ensure_ascii=False, indent=2))
raise SystemExit(0 if all(checks.values()) else 1)
