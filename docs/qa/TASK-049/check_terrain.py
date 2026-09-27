"""Read-only dry-ground audit against the existing TASK026 terrain source."""
import json,math,sys
from pathlib import Path
root=Path(__file__).resolve().parents[3]
sys.path.insert(0,str(root/'scripts/world/TASK-026'))
import prepare_dressing as terrain
d=json.loads((root/'Resources/Data/gameplay.json').read_text(encoding='utf-8'))['campaign']
points=[]
for row in d['locations']:points.append((row['id'],row['xy']))
for row in d['enemies']+d['prologue_enemies']+d['field_enemies']:points.append((row['id'],row['spawn']))
for group in d['reinforcements']:
    for row in group['enemies']:points.append((row['id'],row['spawn']))
for row in d['people']:
    if row['type']=='rescuable':points.append((row['id'],row['spawn']))
for zone in d['zones']:
    for i,p in enumerate(zone['houses']):points.append((zone['id']+':house:'+str(i),p))
    for patrol in zone['patrols']:
        for i,p in enumerate(patrol['points']):points.append((patrol['id']+':'+str(i),p))
checks=[]
for identity,(x,y) in points:
    z=terrain.sample(terrain.h,x,y);slope=terrain.sample(terrain.slope,x,y)
    checks.append({'id':identity,'xy':[x,y],'height_m':z,'slope':slope,'dry':not terrain.wet(x,y,z),'walkable_slope':slope<math.tan(math.radians(32))})
route=json.loads((root/'docs/world/TASK-049/terrain-route.json').read_text(encoding='utf-8'))
wet_route=[]
for index,(x,y,z) in enumerate(route['path']):
    if terrain.wet(x,y,z):wet_route.append(index)
report={'ok':all(c['dry'] and c['walkable_slope'] for c in checks) and not wet_route,
        'scope':'Source-height/water geometry; actual collision, foliage and navigation require PIE',
        'points':checks,'wet_route_indices':wet_route,'route_length_m':route['length_m'],
        'minutes_at_3_5_mps':route['length_m']/210,'mean_discovery_seconds':route['length_m']/3.5/(len(route['waypoints'])-1)}
print(json.dumps(report,ensure_ascii=False,indent=2))
raise SystemExit(0 if report['ok'] else 1)
