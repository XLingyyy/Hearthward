import unreal,json,math,traceback
from pathlib import Path
out=Path('G:/GameFactory/Hearthward/.agent-local/qa/TASK-100/foundations-02.json')
try:
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/Hearthward/World/Natural/Rebuild/L_HearthwardWilds')
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    data=json.loads(Path('G:/GameFactory/Hearthward/Resources/Data/gameplay.json').read_text(encoding='utf-8'))['campaign']
    cache={}
    def ground(x,y):
        k=(x,y)
        if k not in cache:
            hits=unreal.SystemLibrary.line_trace_multi_for_objects(world,unreal.Vector(x*100,y*100,80000),unreal.Vector(x*100,y*100,-30000),[unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY1],False,[],unreal.DrawDebugTrace.NONE,True)
            z=[]
            for h in hits:
                b=h.to_tuple()
                if b[9] and (b[9].actor_has_tag('Hearthward.NatureGround') or 'Landscape' in b[9].get_class().get_name()) and b[7].z>.65:z.append(b[5].z)
            cache[k]=max(z) if z else None
        return cache[k]
    def distance(p,a,b):
        dx=b[0]-a[0];dy=b[1]-a[1];t=max(0,min(1,((p[0]-a[0])*dx+(p[1]-a[1])*dy)/(dx*dx+dy*dy)))
        return math.hypot(p[0]-a[0]-t*dx,p[1]-a[1]-t*dy)
    rows=[]
    old=json.loads(Path('G:/GameFactory/Hearthward/.agent-local/qa/TASK-100/world-03/result.json').read_text(encoding='utf-8'))['houses']
    for zone in data['zones']:
        for i,h in enumerate(zone['houses']):
            if next(r for r in old if r['id']==f"CampaignHouse:{zone['id']}:{i}")['terrain_span_cm']<35:continue
            candidates=[]
            for dx in range(-60,61,5):
                for dy in range(-60,61,5):
                    p=[h[0]+dx,h[1]+dy];b=zone['bounds']
                    if not (b[0]+5<p[0]<b[2]-5 and b[1]+5<p[1]<b[3]-5):continue
                    if any(math.dist(p,e['spawn'])<6 for e in data['enemies']):continue
                    if any(math.dist(p,e['spawn'])<6 for e in data['people'] if 'spawn' in e):continue
                    if any(math.dist(p,e['xy'])<7 for e in data['locations']):continue
                    if any(math.dist(p,other)<10 for j,other in enumerate(zone['houses']) if j!=i):continue
                    if any(distance(p,a,patrol['points'][(j+1)%4])<6 for patrol in zone['patrols'] for j,a in enumerate(patrol['points'])):continue
                    zs=[ground(p[0]+x,p[1]+y) for x,y in [(0,0),(-3.5,-3.5),(-3.5,3.5),(3.5,-3.5),(3.5,3.5)]]
                    if None in zs:continue
                    span=max(zs)-min(zs)
                    if span<25:candidates.append({'xy':p,'offset':[dx,dy],'span_cm':round(span,2),'z':zs[0],'distance':math.hypot(dx,dy)})
            candidates.sort(key=lambda c:(c['distance'],c['span_cm']))
            rows.append({'zone':zone['id'],'house':i,'candidates':candidates[:4]})
    out.write_text(json.dumps(rows,indent=2),encoding='utf-8')
except Exception:out.write_text(json.dumps({'error':traceback.format_exc()}),encoding='utf-8')
