"""Round-trip every delivered FBX and measure actual skinned/baked poses."""
import bpy,json,sys,math,hashlib,traceback
from pathlib import Path
from mathutils import Vector
HERE=Path(__file__).resolve().parent;sys.path.insert(0,str(HERE))
from author import foot_x,curves
ROOT=Path(r'E:\AiAgent\XLingGame\Resource\Tripo\动物\动作\制作成果')

def points(meshes):
    dg=bpy.context.evaluated_depsgraph_get();out=[]
    for o in meshes:
        ev=o.evaluated_get(dg);mesh=ev.to_mesh()
        out.extend(ev.matrix_world@v.co for v in mesh.vertices);ev.to_mesh_clear()
    return out

def main():
    args=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
    jobs=json.loads((ROOT/'jobs.json').read_text(encoding='utf-8'))
    if args:jobs=[j for j in jobs if j['slug'] in args]
    summaries=[]
    for job in jobs:
        out=Path(job['output']);man=json.loads((out/'animation_manifest.json').read_text(encoding='utf-8'))
        bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath=str(out/('SK_'+job['slug']+'.fbx')),use_image_search=False)
        arm=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE');meshes=[o for o in bpy.context.scene.objects if o.type=='MESH']
        names=set(b.name for b in arm.data.bones);ref={b.name:arm.matrix_world@b.matrix_local for b in arm.data.bones}
        basepoints=points(meshes);edges=[];offset=0
        for m in meshes:
            edges.extend((a+offset,b+offset) for a,b in (e.vertices[:] for e in m.data.edges));offset+=len(m.data.vertices)
        baseLengths=[(basepoints[a]-basepoints[b]).length for a,b in edges]
        rig=man['rig'];report={'slug':job['slug'],'source_sha256':job['source_sha256'],'skeletal_file':str(out/('SK_'+job['slug']+'.fbx')),'skin_vertices':len(basepoints),'bone_count':len(names),'bone_name_match':names==set(rig['bone_heads']),'clips':[]}
        for c in man['clips']:
            bpy.context.scene.render.fps=c['fps']
            before=set(bpy.data.objects);bpy.ops.import_scene.fbx(filepath=c['file'],use_image_search=False)
            created=set(bpy.data.objects)-before;imported=next(o for o in created if o.type=='ARMATURE');act=imported.animation_data.action
            same=set(b.name for b in imported.data.bones)==names
            rest_error=max(max(abs(ref[b.name][i][k]-(imported.matrix_world@b.matrix_local)[i][k]) for i in range(4) for k in range(4)) for b in imported.data.bones) if same else None
            arm.animation_data_create();arm.animation_data.action=act
            if imported.animation_data.action_slot:arm.animation_data.action_slot=imported.animation_data.action_slot
            for o in created:bpy.data.objects.remove(o,do_unlink=True)
            lo,hi=act.frame_range;frames=round(hi-lo)+1
            roots=[];bone_changes=[];previous=None;slips=[];errors=[];worst_delta=0;ground=[]
            start_pose=None;end_pose=None
            for f in range(frames):
                bpy.context.scene.frame_set(round(lo)+f);bpy.context.view_layer.update()
                pose={p.name:arm.matrix_world@p.matrix for p in arm.pose.bones}
                if start_pose is None:start_pose=pose
                end_pose=pose;roots.append(pose['root'].copy())
                if previous:
                    for n in pose:
                        angle=math.degrees(previous[n].to_quaternion().rotation_difference(pose[n].to_quaternion()).angle);angle=min(angle,360-angle)
                        worst_delta=max(worst_delta,angle)
                        if n!='root' and angle>.01:bone_changes.append(n)
                    if c['loop'] and c['kind'] in ('walk','hop','trot','run'):
                        t=f/(frames-1);pt=(f-1)/(frames-1);duty=c['contact_duty'];speed=c['reference_speed_cm_s']/100
                        for key,phase0 in c['contact_offsets'].items():
                            ns=rig['chains'][key];eff=ns[3] if len(ns)>3 else ns[-1]
                            cycles=c.get('gait_cycles',1);ph=(t*cycles+phase0)%1;prevph=(pt*cycles+phase0)%1
                            if ph<duty and prevph<duty and ph>prevph:
                                motion=pose[eff].translation-previous[eff].translation+Vector((speed/c['fps'],0,0));slips.append(motion.length)
                                x,z,contact=foot_x(ph,duty,speed*c['duration']*duty/cycles)
                                target=Vector(rig['restfeet'][key])+Vector((x,0,0));errors.append((pose[eff].translation-target).length)
                if f in (0,round((frames-1)/2),frames-1):
                    pts=points(meshes)
                    if job['rig_type'] not in ('aquatic','serpentine') or any(s in c['suffix'] for s in ('Land','Settle','Display')):ground.append(min(p.z for p in pts))
                previous=pose
            deformation=[]
            for frac in (0,.5,1):
                bpy.context.scene.frame_set(round(lo+(hi-lo)*frac));bpy.context.view_layer.update();ps=points(meshes)
                ratios=[(ps[a]-ps[b]).length/base for (a,b),base in zip(edges,baseLengths) if base>.0003]
                deformation.extend(ratios)
            deformation.sort();p99=deformation[int(len(deformation)*.99)] if deformation else None
            qa={'name':c['name'],'file_sha256':c['sha256'],'imported_bone_names_match':same,'rest_matrix_max_error':rest_error,'curve_count':len(curves(act)),
             'duration_s':(hi-lo)/c['fps'],'expected_duration_s':c['duration'],'pose_animated':bool(bone_changes),'animated_bones':sorted(set(bone_changes)),
             'root_translation_drift_m':max((m.translation-roots[0].translation).length for m in roots),'root_rotation_drift_deg':max(math.degrees(m.to_quaternion().rotation_difference(roots[0].to_quaternion()).angle) for m in roots),
             'max_adjacent_world_rotation_deg':worst_delta,'max_contact_slip_m_per_frame':max(slips) if slips else None,'max_contact_target_error_m':max(errors) if errors else None,
             'ground_min_z_m':min(ground) if ground else None,'skin_edge_stretch_p99':p99,'skin_edge_stretch_max':deformation[-1] if deformation else None}
            if c['loop']:
                qa['loop_pose_error_deg']=max(math.degrees(start_pose[n].to_quaternion().rotation_difference(end_pose[n].to_quaternion()).angle) for n in start_pose)
                qa['loop_position_error_m']=max((start_pose[n].translation-end_pose[n].translation).length for n in start_pose)
            qa['structural_pass']=bool(same and rest_error<.0001 and qa['curve_count']>0 and abs(qa['duration_s']-c['duration'])<.001 and (qa['pose_animated'] or c['hold']) and qa['root_translation_drift_m']<.001)
            qa['review_flags']=[]
            if qa['max_contact_slip_m_per_frame'] is not None and qa['max_contact_slip_m_per_frame']>.02:qa['review_flags'].append('contact slip exceeds 2 cm/frame')
            if qa['ground_min_z_m'] is not None and qa['ground_min_z_m']<-.005:qa['review_flags'].append('visible ground penetration')
            if p99 and p99>1.8:qa['review_flags'].append('p99 skin edge strain exceeds 1.8')
            if worst_delta>60:qa['review_flags'].append('large adjacent world-space joint rotation')
            report['clips'].append(qa)
            (out/'fbx_roundtrip_qa.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
            print('VALIDATE '+job['slug']+' '+c['suffix']+' structural '+str(qa['structural_pass'])+' flags '+str(qa['review_flags']),flush=True)
            arm.animation_data.action=None;bpy.data.actions.remove(act)
        report['structural_pass']=report['bone_name_match'] and all(c['structural_pass'] for c in report['clips'])
        (out/'fbx_roundtrip_qa.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
        summaries.append({'slug':job['slug'],'structural_pass':report['structural_pass'],'clips':len(report['clips']),'flags':{c['name']:c['review_flags'] for c in report['clips'] if c['review_flags']}})
        (ROOT/'roundtrip_summary.json').write_text(json.dumps(summaries,ensure_ascii=False,indent=2),encoding='utf-8')

if __name__=='__main__':main()
