"""Check the draft's finite content budgets and references; does not test UE gameplay."""
import json
import math
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
d = json.loads((Path(__file__).parent / 'candidate.json').read_text(encoding='utf-8'))
runtime = json.loads((ROOT / 'Resources/Data/gameplay.json').read_text(encoding='utf-8'))
checks = []

def check(name, condition):
    checks.append({'name': name, 'result': 'PASS' if condition else 'FAIL'})

quests = d['quests']
ids = {q['id'] for q in quests}
check('23 distinct quests', len(quests) == len(ids) == 23)
check('8 main and 15 side', sum(q['category'] == 'main' for q in quests) == 8 and sum(q['category'] == 'side' for q in quests) == 15)
check('all predecessor references resolve', all(set(q['requires']) <= ids for q in quests))
parents = {q['id']: q['requires'] for q in quests}
def acyclic(node, stack):
    if node in stack:
        return False
    return all(p in parents and acyclic(p, stack | {node}) for p in parents[node])
check('quest dependency graph acyclic', all(acyclic(q, set()) for q in ids))
locations = {l['id'] for l in d['locations']}
check('location references resolve', all(q['location'] in locations for q in quests))
check('all quests have safe and failure branches', all(q['safe_phase'] and q['failure_or_branch'] and q['steps'] and q['saved_fields'] for q in quests))
items = {i['id'] for i in runtime['items']}
check('material rewards and knowledge IDs exist', all(set(q['reward']['items']) <= items and set(q['reward']['knowledge']) <= items for q in quests))
check('unique reward receipts', len({q['reward']['id'] for q in quests}) == 23)
xp = runtime['progression']['experienceRewards']
check('approved quest experience rates', all(q['reward']['xp'] == xp['main_milestone' if q['category'] == 'main' else 'side_quest'] for q in quests))
people = d['people']
rescued = [p for q in quests for p in q['rescued_people']]
check('exactly 10 distinct assigned rescue people', len(rescued) == len(set(rescued)) == 10 and set(rescued) == {p['id'] for p in people if p['type'] == 'rescuable'})
check('20 initial civilians and 4 protected people', sum(p['type'] == 'initial' for p in people) == 20 and sum(p['type'] == 'protected_enemy_civilian' for p in people) == 4)
check('population budget is 30 excluding brothers', sum(p['type'] == 'initial' for p in people) + len(rescued) == 30)
enemies = d['enemies']
check('80 distinct fixed garrison', len(enemies) == len({e['id'] for e in enemies}) == 80)
check('four distinct zones and flags', len(d['zones']) == len({z['id'] for z in d['zones']}) == len({z['flag']['id'] for z in d['zones']}) == 4)
for z in d['zones']:
    members = [e for e in enemies if e['zone'] == z['id']]
    check(z['id'] + ' troop composition', all(sum(e['kind'] == k for e in members) == v for k,v in z['counts'].items()))
    x1,y1,x2,y2 = z['bounds']
    points = [e['spawn'] for e in members] + [p for route in z['patrols'] for p in route['points']] + [z['flag']['position']]
    check(z['id'] + ' spawns/patrols/flag within bounds', all(x1 <= x < x2 and y1 <= y < y2 for x,y in points))
    patrol_ids = {p['id'] for p in z['patrols']}
    check(z['id'] + ' two 2-person patrols', len(patrol_ids) == 2 and all(sum(e['patrol'] == p for e in members) == 2 for p in patrol_ids))
check('all non-garrison actors excluded from victory', all(not p['counts_for_victory'] for p in people + d['prologue_enemies'] + d['field_encounter']['enemies']))
check('two finite reinforcement groups', len(d['reinforcements']) == 2 and all(len(g['enemies']) == 4 and g['trigger'] and g['cancel'] and g['states'] == ['pending','spawned','cancelled'] for g in d['reinforcements']))
all_enemy_ids = [e['id'] for e in enemies] + [e['id'] for g in d['reinforcements'] for e in g['enemies']] + [e['id'] for e in d['prologue_enemies'] + d['field_encounter']['enemies']]
check('enemy IDs do not overlap across phases/groups', len(all_enemy_ids) == len(set(all_enemy_ids)))
thresholds = {n: next(k for k in range(n+1) if k * 100 > n * 95) for n in (80,84,88)}
check('strict 95 percent threshold', thresholds == {80:77,84:80,88:84})
segments = [math.dist(a,b) for a,b in zip(d['route'],d['route'][1:])]
check('planned walking route exceeds 7 minutes at 3.5 m/s', sum(segments)/3.5 >= 420)
check('mean route discovery budget 90-150 seconds (GDD average)', 90 <= sum(segments)/len(segments)/3.5 <= 150)
check('travel stations reference locations and require interaction', all(s['location'] in locations and 'interaction' in s['activation'] for s in d['travel_stations']))
check('draft not accidentally approved', d['status'] == 'DRAFT_OWNER_REVIEW')
totals = {'route_length_m': round(sum(segments),1), 'walking_minutes_at_3_5_mps': round(sum(segments)/3.5/60,2),
          'segment_seconds_at_3_5_mps': [round(x/3.5,1) for x in segments], 'remaining_enemy_thresholds': thresholds,
          'quest_and_rescue_xp': sum(q['reward']['xp'] for q in quests)+10*xp['first_rescue'],
          'base_garrison_xp':sum(xp[e['kind']] for e in enemies)}
print(json.dumps({'scope':'draft data consistency only; UE/runtime/terrain NOT_RUN','base_sha':d['base_sha'],
                  'passed':sum(c['result']=='PASS' for c in checks),'total':len(checks),'checks':checks,'totals':totals},ensure_ascii=False,indent=2))
raise SystemExit(0 if all(c['result']=='PASS' for c in checks) else 1)
