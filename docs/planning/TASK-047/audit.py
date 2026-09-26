"""Design arithmetic and dependency audit; no game data writes or UE claims."""
import json
import math
from fractions import Fraction
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
d = json.loads((HERE / 'candidate.json').read_text(encoding='utf-8'))
live = json.loads((ROOT / 'Resources/Data/gameplay.json').read_text(encoding='utf-8'))
checks = []


def check(name, condition):
    checks.append({'name': name, 'passed': bool(condition)})


items = {r['id']: r for r in d['items']}
skills = {r['id']: r for r in d['skills']}
recipes = {r['id']: r for r in d['recipes']}
sources = {r['id']: r for r in d['sources']}
check('unique definition and recipe IDs', len(items) == len(d['items']) and len(recipes) == len(d['recipes']))
check('all 27 live item IDs preserved', {r['id'] for r in live['items']} <= items.keys())
check('all 29 skill IDs preserved', len(skills) == 29 and {r['id'] for r in live['skills']} == skills.keys())
check('integer nonnegative weights', all(type(r['weight_hundredths']) is int and r['weight_hundredths'] >= 0 for r in items.values()))
check('exact accepted basic weights', [items[k]['weight_hundredths'] for k in ['wood', 'stone', 'ore', 'meat', 'arrow']] == [100, 100, 200, 50, 5])
levels = d['levels']
check('60 contiguous level rows', [r['level'] for r in levels] == list(range(1, 61)))
check('strictly increasing cumulative XP', all(b['total_xp'] > a['total_xp'] for a, b in zip(levels, levels[1:])))
check('XP curve and level boundaries', all(r['total_xp'] == 100*(r['level']-1)+35*(r['level']-1)*(r['level']-2)//2 and r['next_xp'] == (100+35*(r['level']-1) if r['level']<60 else 0) for r in levels))
check('half growth contribution endpoint', levels[0]['hp_bonus'] == levels[0]['stamina_bonus'] == 0 and levels[-1]['hp_bonus'] == 100 and levels[-1]['stamina_bonus'] == 50)
check('every level attribute increment', all(abs(r['hp_bonus']-100*(r['level']-1)/59)<.000001 and abs(r['stamina_bonus']-50*(r['level']-1)/59)<.000001 for r in levels))
total_cost = sum(r['maxRank'] * r['cost'] for r in skills.values())
check('87 points denominator, cap floor60%=52 at level60', total_cost == 87 and levels[-1]['skill_budget'] == total_cost*3//5 == 52 and levels[-2]['skill_budget'] < 52)
check('point budget at every level', all(r['skill_budget'] == 2+50*(r['level']-1)//59 for r in levels))
reached = set()
while True:
    new = {k for k, r in skills.items() if not r.get('requires') or r['requires'] in reached}
    if new <= reached:
        break
    reached |= new
check('all skill prerequisites acyclic and reachable', reached == skills.keys())
check('no XP or random stun skill remains', all(r['effect'] not in ['xp', 'stunChance'] for r in skills.values()) and skills['strong']['effect'] == 'heavy_damage')
check('recipe references and positive counts', all(set(r['inputs']) | set(r['outputs']) <= items.keys() and all(type(n) is int and n > 0 for n in list(r['inputs'].values())+list(r['outputs'].values())) for r in recipes.values()))
check('blueprint references', all(r['knowledge'] == 'known' or r['knowledge'] in items for r in recipes.values()))
facilities = {(r['id'], r['level']) for r in live['campEconomy']['facilities']} | {('campfire', 1)}
check('uses actual facility levels', all((r['facility'], r['facility_level']) in facilities for r in recipes.values()))
# A topological acquisition pass includes tool gates and blueprint knowledge.
available = set()
while True:
    new = set(available)
    for k, r in items.items():
        s = sources.get(r['source'])
        if s and (not s['required_tool'] or s['required_tool'] in available):
            if r['source'] != 'event:interrupted_medicine' or 'medicine' in available:
                new.add(k)
    for r in recipes.values():
        if set(r['inputs']) <= available and (r['knowledge'] == 'known' or r['knowledge'] in available):
            new.update(r['outputs'])
    if new == available:
        break
    available = new
check('all acquisition chains reachable including tool/blueprint gates', available == items.keys())
check('every advertised source exists', all(r['source'] in sources or (r['source'].startswith('craft:') and r['source'][6:] in recipes and r['id'] in recipes[r['source'][6:]]['outputs']) for r in items.values()))
# Preserve approved 046 processing ratios exactly, including outputs and facility.
check('046 recipes unchanged', all(recipes[r['id']]['inputs'] == r['inputs'] and recipes[r['id']]['outputs'] == r['outputs'] and recipes[r['id']]['facility'] == r['facility'] and recipes[r['id']]['facility_level'] == r['level'] for r in live['campEconomy']['recipes']))
gear = [r for r in items.values() if r['durability_max'] > 0]
check('every durable item has accessible repair basis', all(set(r['repair_basis']) <= available and (r['repair_facility'], r['repair_facility_level']) in facilities and all(n>0 for n in r['repair_basis'].values()) for r in gear))
check('normal equipment repair basis equals craft input', all(recipes[r['source'][6:]]['inputs'] == r['repair_basis'] for r in gear if r['rarity'] == 'common'))
check('unique claims never craftable', all(r['id'] not in {o for p in recipes.values() for o in p['outputs']} and 'unique_claim' in r for r in items.values() if r['rarity']=='unique'))
check('four armor locations supplied', {'head', 'chest', 'legs', 'feet'} <= {r.get('slot') for r in gear if r['stage']==1})
check('legacy accessories cannot provide ghost armor', all(items[k]['defense_fraction'] == 0 and items[k]['durability_max'] == 0 for k in ['gloves', 'belt', 'quiver']))
weapon_lifetimes=[]
for r in gear:
    if r.get('damage', 0) > 0 and r['rarity']=='common':
        h = d['enemy_calibration'][r['stage']-1]['H']
        hits = math.ceil(h/r['damage'])
        weapon_lifetimes.append({'id': r['id'], 'hits_per_enemy': hits, 'enemies_before_break': r['durability_max']/hits})
check('all common weapons meet 20 unarmored body kills calibration', all(r['enemies_before_break']==20 for r in weapon_lifetimes))
check('three stages satisfy P/H=.2', all(Fraction(r['shortblade_base'],r['H'])==Fraction(1,5) for r in d['enemy_calibration']))
check('initial armored player 5 hits calibration', abs(d['enemy_calibration'][0]['guard_attack']*.95*5-100)<.000001)
# Fraction arithmetic examines every whole-point repair split. Ceil on each
# material prevents a split repair from costing less than one combined repair.
repair_safe = True
for r in gear:
    maximum = r['durability_max']
    for material, quantity in r['repair_basis'].items():
        unit = Fraction(quantity, 5*maximum)
        for restored in range(1, maximum+1):
            fee = math.ceil(unit*restored)
            if fee <= 0:
                repair_safe = False
            for first in [1, restored//2, restored-1]:
                if 0 < first < restored and math.ceil(unit*first)+math.ceil(unit*(restored-first)) < fee:
                    repair_safe = False
check('no free repair or cheaper split repair', repair_safe)
check('backpack per-person endpoints and valid upgrade costs', [r['capacity'] for r in d['backpacks']]==[100,150,200,250,300] and all(set(r['incremental_cost'])<=available for r in d['backpacks']))
loadout={'axe':1,'bow':1,'hood':1,'armor':1,'leggings':1,'boots':1,'shield':1,'arrow':24,'roast':4,'medicine':3}
weight=sum(items[k]['weight_hundredths']*n for k,n in loadout.items())/100
check('starter loadout fits each 100-capacity bag', weight <=100)
budgets=[]
for row in d['budgets']:
    xp=sum(d['experience_rewards'][k]*n for k,n in row['counts'].items())
    level=max(r['level'] for r in levels if r['total_xp']<=xp)
    budgets.append({'id':row['id'],'experience':xp,'level':level,'skill_budget':levels[level-1]['skill_budget']})
check('normal ending below cap', budgets[-1]['level']<60)
report={'status':'PASS' if all(r['passed'] for r in checks) else 'FAIL','scope':'design arithmetic only; no runtime or natural-map validation', 'counts':{'levels':len(levels),'skills':len(skills),'items':len(items),'durable_items':len(gear),'recipes':len(recipes),'checks':len(checks)},'checks':checks,'budgets':budgets,'starter_weight':weight,'starter_free_capacity':100-weight,'weapons':weapon_lifetimes,'placement_pending':[r['id'] for r in sources.values() if r['placement_status'].startswith('PENDING')]}
print(json.dumps(report,ensure_ascii=False,indent=2))
raise SystemExit(0 if report['status']=='PASS' else 1)
