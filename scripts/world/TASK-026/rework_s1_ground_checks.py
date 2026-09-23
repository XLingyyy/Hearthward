"""Read-only distributed ground and standing-capsule probes for S1."""
from pathlib import Path
import json
import math
import unreal

root=Path(unreal.Paths.project_dir())
out=root/'docs/qa/evidence/TASK-026/rework-v2/S1-ground-checks-03'
out.mkdir(parents=True,exist_ok=False)
points=json.loads((root/'art_source/TASK-026/Rebuild/ReworkV2/s1-ground-checks.json').read_text(encoding='utf-8'))
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
unreal.WorldPartitionBlueprintLibrary.load_actors([d.guid for d in unreal.WorldPartitionBlueprintLibrary.get_actor_descs()])
result=[]
for point in points:
    x,y,z=point['xyz_m']
    hit=unreal.SystemLibrary.line_trace_single(world,unreal.Vector(x*100,y*100,z*100+1500),
        unreal.Vector(x*100,y*100,z*100-1500),unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,
        False,[],unreal.DrawDebugTrace.NONE,True).to_tuple()
    row={**point,'ground_hit':bool(hit[0])}
    if hit[0]:
        impact=hit[4]
        normal=hit[6]
        row.update(actual_z_m=impact.z/100,height_error_m=impact.z/100-z,
            ground_actor=hit[9].get_actor_label() if hit[9] else None,
            slope_deg=math.degrees(math.acos(max(-1,min(1,normal.z)))))
        start=unreal.Vector(impact.x,impact.y,impact.z+92)
        end=unreal.Vector(impact.x,impact.y,impact.z+93)
        capsule_hit=unreal.SystemLibrary.capsule_trace_single(world,start,end,34,88,
            unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,False,[],unreal.DrawDebugTrace.NONE,True)
        capsule=capsule_hit.to_tuple() if capsule_hit is not None else None
        row['standing_capsule_blocked']=bool(capsule and capsule[0])
        row['capsule_hit_actor']=capsule[9].get_actor_label() if capsule and capsule[9] else None
        row['passed']=abs(row['height_error_m'])<.25 and row['slope_deg']<45 and not row['standing_capsule_blocked']
    else:
        row['passed']=False
    result.append(row)
(out/'report.json').write_text(json.dumps({'scope':'Editor collision queries; does not replace actual walking',
    'capsule_radius_cm':34,'capsule_half_height_cm':88,'count':len(result),
    'passed':all(x['passed'] for x in result),'points':result},indent=2),encoding='utf-8')
