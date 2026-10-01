"""Rigid connected limb segments with restrained medial knees and hocks.

Repeatable from the immutable R3 snapshot. Authored foot tracks,
contact timing, skeleton names, vertices, UVs and textures stay paired. Limb rest pivots are
measured and all paired actions retargeted; gait curves and shaft weights are edited.
"""
import bpy
import sys
import math
import copy
import numpy as np
from pathlib import Path
from mathutils import Vector, Matrix, Quaternion
sys.path.insert(0, str(Path(__file__).resolve().parent))
from common import ROOT, REV, GAITS, jobs, read, save, sha, backup_folder
from author import reset, curves, export_fbx, write_rig_report, smooth
from refinement_helpers_r1 import snapshot, apply, bake_frame, move, repair_torso
from limb_solver import solve_limb
from rebind_legs import recenter
from body_skin import repair_hare_torso

def rigid_skin(arm, meshes, rig, gain=1.0, chain_gains=None):
    """Rigid shafts and compact, adjacent-joint transitions for linear skinning.

    No DQ-only modifier is used: these weights export to the engine unchanged.
    The old spatial limb membership excludes torso, head, tail and other legs.
    """
    chains=rig['chains']
    stats={'changed_vertices':0, 'limbs':{}, 'strength':gain, 'method':'piecewise rigid shaft; adjacent joints only; compact smoothstep blend; linear skinning'}
    if chain_gains is not None:stats['chain_strengths']=chain_gains
    for mesh in meshes:
        group_names=[g.name for g in mesh.vertex_groups]
        count=len(mesh.data.vertices)
        xyz=np.empty(count*3,np.float32);mesh.data.vertices.foreach_get('co',xyz);xyz=xyz.reshape(-1,3)
        weights=np.zeros((count,len(group_names)),np.float32)
        for v in mesh.data.vertices:
            for g in v.groups:weights[v.index,g.group]=g.weight
        original=weights.copy()
        all_distances=[]
        for ns in chains.values():
            pts=np.array([list(arm.data.bones[n].head_local) for n in ns]+[list(arm.data.bones[ns[-1]].tail_local)])
            distances=[]
            for a,b in zip(pts,pts[1:]):
                ab=b-a;u=np.clip((xyz-a)@ab/(ab@ab),0,1)
                distances.append(np.linalg.norm(xyz-a-u[:,None]*ab,axis=1))
            all_distances.append(np.array(distances).min(axis=0))
        winner=np.array(all_distances).argmin(axis=0)
        reserved=[i for i,n in enumerate(group_names) if n in rig.get('tail',[]) or n in rig.get('neck',[]) or n==rig.get('head')]
        reserved_weight=original[:,reserved].sum(axis=1)
        for chain_index,(key,ns) in enumerate(chains.items()):
            ids=[group_names.index(n) for n in ns]
            total=original[:,ids].sum(axis=1)
            pts=np.array([list(arm.data.bones[n].head_local) for n in ns]+[list(arm.data.bones[ns[-1]].tail_local)])
            lengths=np.linalg.norm(np.diff(pts,axis=0),axis=1)
            cumulative=np.r_[0.,np.cumsum(lengths)]
            distances=[];fractions=[]
            for a,b in zip(pts,pts[1:]):
                v=b-a;u=np.clip((xyz-a)@v/(v@v),0,1)
                distances.append(np.linalg.norm(xyz-a-u[:,None]*v,axis=1));fractions.append(u)
            distances=np.array(distances).T;fractions=np.array(fractions).T
            nearest=distances.argmin(axis=1);arclength=cumulative[nearest]+lengths[nearest]*fractions[np.arange(count),nearest]
            lower=np.array([smooth(v) for v in ((pts[1,2]-xyz[:,2])/(lengths[1]*.20))])
            membership=np.array([smooth(v) for v in ((total-.08)/.42)])
            strength=membership*(1-lower)+lower
            # Leave shoulder/haunch bulk and the proximal attachment untouched.
            # Shaft repair fades in around the first joint, then reaches full
            # rigidity along the distal limb, without reassigning belly skin.
            strength*=np.array([smooth(v) for v in ((pts[1,2]+lengths[1]*.12-xyz[:,2])/(lengths[1]*.38))])
            strength*=np.array([smooth(v) for v in ((lengths.max()*.60-distances.min(axis=1))/(lengths.max()*.15))])
            strength*=np.array([1-smooth(v) for v in ((reserved_weight-.08)/.20)])
            strength*=winner==chain_index
            strength*=chain_gains.get(key,gain) if chain_gains is not None else gain
            selected=np.where(strength>1e-6)[0]
            target=np.zeros_like(weights)
            target[np.arange(count),np.array(ids)[nearest]]=1
            for i in range(1,len(ns)):
                halfwidth=.22*min(lengths[i-1],lengths[i])
                band=np.where(abs(arclength-cumulative[i])<halfwidth)[0]
                t=np.array([smooth(v) for v in ((arclength[band]-cumulative[i]+halfwidth)/(2*halfwidth))])
                target[band,:]=0;target[band,ids[i-1]]=1-t;target[band,ids[i]]=t
            distal=(nearest>=3)|(xyz[:,2]<pts[3,2]+lengths[2]*.12)
            target[distal]=original[distal]
            weights[selected]=original[selected]*(1-strength[selected,None])+target[selected]*strength[selected,None]
            stats['limbs'][key]=stats['limbs'].get(key,0)+len(selected)
        changed=np.where(abs(weights-original).max(axis=1)>1e-6)[0]
        for index in changed:
            v=mesh.data.vertices[int(index)];w=weights[index]
            keep=np.argsort(w)[-8:];subtotal=float(w[keep][w[keep]>1e-7].sum())
            for g in list(v.groups):mesh.vertex_groups[g.group].remove([v.index])
            for i in keep:
                if w[i]>1e-7:mesh.vertex_groups[int(i)].add([v.index],float(w[i])/subtotal,'REPLACE')
        stats['changed_vertices']+=len(changed)
    return stats

def author(job, deliver=False):
    slug=job['slug'];base=backup_folder(job)
    out=Path(job['output']) if deliver else REV/'candidate'/slug
    out.mkdir(parents=True,exist_ok=True);(out/'clips').mkdir(exist_ok=True)
    man=read(base/'animation_manifest.json');rig=man['rig']
    bpy.ops.wm.open_mainfile(filepath=str(base/f'AS_{slug}.blend'))
    arm=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE')
    meshes=[o for o in bpy.context.scene.objects if o.type=='MESH' and not o.name.startswith('BindPoseProxy')]
    scene=bpy.context.scene
    arm.animation_data.action=bpy.data.actions[man['clips'][0]['name']];scene.frame_set(1)
    standing=snapshot(arm)
    sources={};world={};matrices={}
    original_rest={b.name:b.matrix_local.copy() for b in arm.data.bones}
    for c in man['clips']:
        arm.animation_data.action=bpy.data.actions[c['name']]
        sources[c['name']]=[];world[c['name']]=[];matrices[c['name']]=[]
        for f in range(1,c['frames']+1):
            scene.frame_set(f);sources[c['name']].append(snapshot(arm))
            matrices[c['name']].append({p.name:p.matrix.copy() for p in arm.pose.bones})
            world[c['name']].append({k:{'joints':[arm.pose.bones[n].matrix.translation.copy() for n in ns[:4]],'foot_matrix':arm.pose.bones[ns[3]].matrix.copy()} for k,ns in rig['chains'].items()})
    arm.animation_data.action=None;reset(arm);bpy.context.view_layer.update()
    old_signature=rig['skeleton_signature']
    joint_rebind=recenter(arm,meshes,rig,slug)
    new_rest={b.name:b.matrix_local.copy() for b in arm.data.bones}
    conversions={n:original_rest[n].inverted()@new_rest[n] for n in original_rest}
    for c in man['clips']:
        for i,old_mats in enumerate(matrices[c['name']]):
            rebaked={n:old_mats[n]@conversions[n] for n in old_mats}
            values={}
            for p in arm.pose.bones:
                basis=p.bone.convert_local_to_pose(rebaked[p.name],new_rest[p.name],parent_matrix=rebaked[p.parent.name] if p.parent else Matrix.Identity(4),parent_matrix_local=new_rest[p.parent.name] if p.parent else Matrix.Identity(4),invert=True)
                loc,q,_=basis.decompose();values[p.name]=(loc,q)
            sources[c['name']][i]=values
            for key,ns in rig['chains'].items():
                world[c['name']][i][key]['joints']=[rebaked[n].translation.copy() for n in ns[:4]]
                world[c['name']][i][key]['foot_matrix']=rebaked[ns[3]].copy()
    standing=sources[man['clips'][0]['name']][0]
    calibration=REV/'skin_calibration'/f'{slug}.json'
    gain=1.
    chain_gains=None
    if '--calibrated' in sys.argv:
        cal=read(calibration)
        if cal.get('revision')!='recentered_r3' or cal['selected_gain'] is None:raise RuntimeError('No valid recentered skin calibration for '+slug)
        gain=cal['selected_gain']
        chain_gains=cal.get('chain_gains')
    torso_repair=repair_hare_torso(arm,meshes,rig) if slug=='hare' else {'changed_vertices':0}
    skin={'changed_vertices':0,'method':'original weights diagnostic'} if '--original-skin' in sys.argv else rigid_skin(arm,meshes,rig,gain,chain_gains)
    skin['torso_repair']=torso_repair
    lengths={k:[(arm.data.bones[ns[i+1]].head_local-arm.data.bones[ns[i]].head_local).length for i in range(3)] for k,ns in rig['chains'].items()}
    centre_y=sum(arm.data.bones[ns[0]].head_local.y for ns in rig['chains'].values())/4
    changed=[]
    for c in man['clips']:
        old_action=bpy.data.actions[c['name']];bpy.data.actions.remove(old_action)
        action=bpy.data.actions.new(c['name']);action.use_fake_user=True
        for k in ('fps','loop','description'):action[k]=c[k]
        arm.animation_data.action=action
        previous={};continuity={}
        for f in range(1,c['frames']+1):
            t=(f-1)/(c['frames']-1)
            amplitude=smooth(t) if c['kind']=='start' else 1-smooth(t) if c['kind']=='stop' else 1.
            # Advance time first, then write the explicit pose. Reassigning
            # an action after solving would evaluate existing keys again.
            scene.frame_set(f)
            apply(arm,sources[c['name']][f-1]);bpy.context.view_layer.update()
            if c['kind'] in GAITS and amplitude>1e-8:
                body=arm.pose.bones[rig['body']]
                old_loc,old_q=sources[c['name']][f-1][rig['body']];stand_loc,stand_q=standing[rig['body']]
                damping={'hare':.50,'black_bear':.50,'pig':.40}.get(slug,.28)
                blend_amplitude=amplitude if slug in ('stag_a','black_bear','hare') else 1.
                body.location=stand_loc.lerp(old_loc,1+(damping-1)*blend_amplitude)
                body.rotation_quaternion=stand_q.slerp(old_q,1+(.45-1)*blend_amplitude)
                bpy.context.view_layer.update()
                drop=0.
                for key,ns in rig['chains'].items():
                    hip=arm.pose.bones[ns[0]].matrix.translation;foot=world[c['name']][f-1][key]['joints'][3]
                    span=sum(lengths[key])-.003
                    horizontal=(hip.x-foot.x)**2+(hip.y-foot.y)**2
                    allowed=foot.z+math.sqrt(max(.00001,span*span-horizontal))
                    drop=max(drop,hip.z-allowed)
                if drop>0:move(arm,rig['body'],(0,0,-drop));bpy.context.view_layer.update()
                for key,ns in rig['chains'].items():
                    track=world[c['name']][f-1][key];old=[p.copy() for p in track['joints']]
                    old[0]=arm.pose.bones[ns[0]].matrix.translation.copy()
                    limb_amplitude=smooth(min(1,amplitude*1.5)) if slug in ('stag_a','black_bear','hare') else amplitude
                    solve_limb(arm,ns,key,old,lengths[key],centre_y,limb_amplitude,continuity,track['foot_matrix'],slug)
            bake_frame(arm,action,f)
            for p in arm.pose.bones:
                q=p.rotation_quaternion
                if p.name in previous and previous[p.name].dot(q)<0:
                    q.negate();p.keyframe_insert('rotation_quaternion',frame=f,group=p.name)
                previous[p.name]=q.copy()
        for fc in curves(action):
            for k in fc.keyframe_points:k.interpolation='LINEAR'
        arm.animation_data.action=action;scene.render.fps=c['fps'];scene.frame_start=1;scene.frame_end=c['frames'];scene.frame_set(1)
        file=out/'clips'/f'{c["name"]}.fbx'
        export_fbx(arm,meshes,file,False,True);c['file']=str(file);c['sha256']=sha(file)
        c['correction_20261001_r3']={'changes':(['fixed-length analytic connected segments; upright distal shaft and distinct proximal hinge','slight medial joint inset (3 degree design limit); sagittal forward/back swing','reduced excessive torso compression; compact calibrated shaft skinning'] if c['kind'] in GAITS else ['original deformation retargeted to recentered limb bind; paired skin rechecked']),'baseline_fbx_sha256':next(a['sha256'] for a in read(base/'animation_manifest.json')['clips'] if a['name']==c['name']),'foot_tracks_contact_timing_reference_speed':'preserved' if c['kind'] in GAITS else 'not a gait','qa':'PENDING'}
        changed.append(c['name']);print('RIGID_GAIT',slug,c['suffix'],flush=True)
        # SK skin weights change for the whole library. Old clip curve files
        # remain paired to identical rest matrices, but old mesh QA is stale.
        c['qa']={'status':'PENDING_R3_REVALIDATION'}
        c['delivery_review'].update(numeric_qa='PENDING',native_source_verified=False,fbx_source_verified=False)
    arm.animation_data.action=None;reset(arm);bpy.context.view_layer.update()
    rig['repairs'].append('2026-10-01 R3: measured limb rest pivots; retargeted complete library; calibrated shaft skinning; exact-length medial gait solve and restrained torso compression')
    write_rig_report(arm,meshes,rig,out)
    for c in man['clips']:c['skeleton_signature']=rig['skeleton_signature']
    export_fbx(arm,meshes,out/f'SK_{slug}.fbx',True,False)
    revision={'revision':'2026-10-01 R3','baseline':str(base),'changed_actions':changed,'gait_actions':[c['name'] for c in man['clips'] if c['kind'] in GAITS],'skin':skin,'joint_rebind':joint_rebind,'skeleton_rest':'limb pivots recentered on measured native skin; all paired actions rebaked','geometry_uv_materials':'unchanged','foot_tracks_contact_timing':'unchanged','cloud_credits_used':0,'gameplay_integration':'NOT_RUN','qa':'PENDING'}
    man['rig']=rig;man['correction_20261001_r3']=revision;man['ue_import']='PENDING_R3';man['visual_qa']='PENDING_R3'
    save(out/'animation_manifest.json',man);save(out/'correction_20261001_r3.json',revision)
    save(out/'rig_audit.json',{'schema':'animal.rig.audit.r3','rig':rig,'skin_qa':rig['skin_qa'],'source':'current rigid-shaft linear skinning'})
    arm.animation_data.action=bpy.data.actions[man['clips'][0]['name']]
    scene.render.fps=man['clips'][0]['fps'];scene.frame_start=1;scene.frame_end=man['clips'][0]['frames'];scene.frame_set(1)
    bpy.ops.file.pack_all();bpy.ops.wm.save_as_mainfile(filepath=str(out/f'AS_{slug}.blend'))
    print('R3_AUTHORED',slug,len(changed),skin['changed_vertices'],flush=True)

if __name__=='__main__':
    args=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
    for j in jobs(args):author(j,'--deliver' in args)
