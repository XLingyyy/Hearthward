"""Repeatable refinements against the verified 2026-10-01 snapshot.

Works on the existing calibrated skeletons. Preserves UVs and vertices, uses
local skin repairs and isolated new fox ear controls, bakes all IK, exports
only actual revised clips, and invalidates stale QA. No cloud generation.
"""
import bpy
import json
import math
import sys
import hashlib
from pathlib import Path
from mathutils import Vector, Quaternion, Matrix
import numpy as np

ROOT=Path(r'E:\AiAgent\XLingGame\Resource\Tripo\动物\动作\制作成果')
REV=ROOT/'refinement_20261001'
sys.path.insert(0,str(REV/'scripts'))
sys.path.insert(0,str(Path(r'E:\AiAgent\XLingGame\GameFactory-3A\operators\gen_motion\funcs\animal_motion')))
from author import reset, curves, export_fbx, write_rig_report, smooth, evaluated_min_z
from rigs import descendants
TAU=2*math.pi


def save(path, value):
    path.write_text(json.dumps(value,ensure_ascii=False,indent=2),encoding='utf-8')


def snapshot(arm):
    return {p.name:(p.location.copy(),p.rotation_quaternion.copy()) for p in arm.pose.bones}


def apply(arm, values):
    for p in arm.pose.bones:
        loc,q=values.get(p.name,(Vector(),Quaternion()))
        p.location=loc;p.rotation_mode='QUATERNION';p.rotation_quaternion=q;p.scale=(1,1,1)


def rotate(arm, name, axis, degrees):
    p=arm.pose.bones[name];basis=p.bone.matrix_local.to_quaternion()
    p.rotation_quaternion @= basis.inverted() @ Quaternion(axis,math.radians(degrees)) @ basis


def rotate_world(arm,name,axis,degrees):
    bpy.context.view_layer.update()
    p=arm.pose.bones[name];matrix=p.matrix.copy()
    rotation=Quaternion(axis,math.radians(degrees)) @ matrix.to_quaternion()
    mat=rotation.to_matrix().to_4x4();mat.translation=matrix.translation;p.matrix=mat


def move(arm, name, delta):
    p=arm.pose.bones[name]
    p.location += p.bone.matrix_local.to_3x3().inverted() @ Vector(delta)


def blend(a,b,t):
    return {n:(a[n][0].lerp(b[n][0],t),a[n][1].slerp(b[n][1],t)) for n in a}


def envelope(t, enter=.25, leave=.75):
    return smooth(t/enter)*(1-smooth((t-leave)/(1-leave)))


def normalize_weights(mesh):
    for v in mesh.data.vertices:
        gs=sorted([(g.group,g.weight) for g in v.groups if g.weight>1e-7],key=lambda g:-g[1])[:8]
        total=sum(w for _,w in gs)
        for g in list(v.groups):mesh.vertex_groups[g.group].remove([v.index])
        if not total:raise RuntimeError('Unweighted skin vertex')
        for i,w in gs:mesh.vertex_groups[i].add([v.index],w/total,'REPLACE')


def repair_torso(arm,meshes,rig,slug):
    """Remove spatially impossible leg weights in the thorax/belly centre."""
    length=rig['axis']['target_length_m']
    chains=rig['chains'];leg_names={b.name for ns in chains.values() for b in descendants(arm.data.bones[ns[0]])}
    axial=[rig['body']]+[n for n in rig['spine'] if n not in leg_names]
    fore=sum((arm.data.bones[chains[k][0]].head_local for k in ('FL','FR')),Vector())/2
    hind=sum((arm.data.bones[chains[k][0]].head_local for k in ('BL','BR')),Vector())/2
    middle=(fore+hind)/2
    width=sum(abs(arm.data.bones[ns[0]].head_local.y-middle.y) for ns in chains.values())/4
    changed=0
    for mesh in meshes:
        for n in axial:
            if not mesh.vertex_groups.get(n):mesh.vertex_groups.new(name=n)
        groups={g.index:g.name for g in mesh.vertex_groups}
        for v in mesh.data.vertices:
            x,y,z=v.co
            inside=smooth((x-hind.x+length*.09)/(length*.12))*smooth((fore.x+length*.09-x)/(length*.12))
            core=1-smooth((abs(y-middle.y)-width*.50)/max(.001,width*.70))
            above=smooth((z-(middle.z-length*.08))/(length*.16))
            height=smooth((z-length*.065)/(length*.09))
            factor=inside*height*max(core,above)
            if factor<1e-6:continue
            removed=0
            for g in list(v.groups):
                if groups[g.group] in leg_names:
                    removed+=g.weight*factor
                    mesh.vertex_groups[g.group].add([v.index],g.weight*(1-factor),'REPLACE')
            if removed>1e-6:
                near=sorted(axial,key=lambda n:(arm.data.bones[n].head_local-v.co).length_squared)[:3]
                ws=[1/max(length*.055,(arm.data.bones[n].head_local-v.co).length)**2 for n in near]
                for n,w in zip(near,ws):mesh.vertex_groups[n].add([v.index],removed*w/sum(ws),'ADD')
                changed+=1
        normalize_weights(mesh)
    return changed


def smooth_skin(meshes,rig):
    """Smooth weight gradients on the trunk and upper limbs, preserving soles."""
    from mathutils.kdtree import KDTree
    length=rig['axis']['target_length_m'];changed=0
    fore=sum(rig['bone_heads'][rig['chains'][k][0]][0] for k in ('FL','FR'))/2
    for mesh in meshes:
        groups=list(mesh.vertex_groups);count=len(mesh.data.vertices)
        weights=np.zeros((count,len(groups)),dtype=np.float32);tree=KDTree(count)
        for v in mesh.data.vertices:
            tree.insert(v.co,v.index)
            for g in v.groups:weights[v.index,g.group]=g.weight
        tree.balance();near=[];factors=[];mask=[]
        for v in mesh.data.vertices:
            ns=tree.find_n(v.co,32);near.append([i for _,i,d in ns])
            factors.append([math.exp(-(d/(length*.019))**2) for _,i,d in ns])
            mask.append(smooth((v.co.z-length*.045)/(length*.085))*(1-smooth((v.co.x-fore-length*.07)/(length*.12))))
        near=np.array(near);factors=np.array(factors,dtype=np.float32);factors/=factors.sum(axis=1)[:,None]
        original=weights.copy()
        for _ in range(48):weights=.35*weights+.65*(weights[near]*factors[:,:,None]).sum(axis=1)
        weights=original+(weights-original)*np.array(mask)[:,None]
        for v,w in zip(mesh.data.vertices,weights):
            if max(abs(w-original[v.index]))<1e-6:continue
            keep=np.argsort(w)[-8:];total=float(w[keep][w[keep]>1e-6].sum())
            for g in list(v.groups):groups[g.group].remove([v.index])
            for i in keep:
                if w[i]>1e-6:groups[i].add([v.index],float(w[i])/total,'REPLACE')
            changed+=1
    return changed


def fox_controls(arm,meshes,rig):
    head=arm.data.bones[rig['head']].head_local
    zmax=max(v.co.z for m in meshes for v in m.data.vertices)
    center=sum((arm.data.bones[rig['chains'][k][0]].head_local.y for k in ('FL','FR')))/2
    bpy.context.view_layer.objects.active=arm;arm.select_set(True);bpy.ops.object.mode_set(mode='EDIT')
    names=[]
    for side,sign in [('L',1),('R',-1)]:
        selected=[v.co.copy() for m in meshes for v in m.data.vertices if v.co.z>zmax-.105 and v.co.x>head.x-.12 and (v.co.y-center)*sign>0]
        if len(selected)<20:raise RuntimeError('Fox ear mask has no anatomical support')
        mean=sum(selected,Vector())/len(selected)
        b=arm.data.edit_bones.new('EarRepair_'+side)
        b.head=(mean.x,mean.y,zmax-.16);b.tail=(mean.x,mean.y,zmax-.015)
        b.parent=arm.data.edit_bones[rig['head']];names.append(b.name)
    bpy.ops.object.mode_set(mode='OBJECT');rig['ears']=names
    weighted={n:0 for n in names}
    for mesh in meshes:
        for n in names:mesh.vertex_groups.new(name=n)
        for v in mesh.data.vertices:
            amount=smooth((v.co.z-(zmax-.185))/.09)*smooth((v.co.x-head.x+.13)/.06)
            if amount>1e-6:
                n=names[0 if v.co.y>center else 1]
                for g in list(v.groups):mesh.vertex_groups[g.group].add([v.index],g.weight*(1-amount),'REPLACE')
                mesh.vertex_groups[n].add([v.index],amount,'ADD');weighted[n]+=1
            # A tail tip was wrongly blended with the hind paw, dragging it into
            # the floor on fast bounds. Retain a smooth transition at the root.
            tail=[g for g in v.groups if mesh.vertex_groups[g.group].name in rig['tail']]
            total=sum(g.weight for g in tail)
            if total>.10:
                hind=sum(arm.data.bones[rig['chains'][k][0]].head_local.x for k in ('BL','BR'))/2
                factor=smooth((hind-.025-v.co.x)/.20)
                for g in list(v.groups):
                    is_tail=mesh.vertex_groups[g.group].name in rig['tail']
                    weight=g.weight*(1-factor)+(g.weight/total*factor if is_tail else 0)
                    mesh.vertex_groups[g.group].add([v.index],weight,'REPLACE')
        normalize_weights(mesh)
    rig['repairs']+=['2026-10-01: isolated two spatially verified ear bones and ear skin',
                     '2026-10-01: spatially separated tail-tip weights from hind-paw weights, with a smooth haunch fade']
    return weighted


def build_ik(arm,rig,standing):
    apply(arm,standing);bpy.context.view_layer.update();targets={}
    for key,ns in rig['chains'].items():
        eff=ns[3] if len(ns)>3 else ns[-1];solver=ns[2] if len(ns)>3 else ns[-2]
        point=arm.pose.bones[eff].matrix.translation.copy()
        ctl=bpy.data.objects.new('RefineIK_'+key,None);bpy.context.collection.objects.link(ctl);ctl.location=point
        con=arm.pose.bones[solver].constraints.new('IK');con.target=ctl;con.chain_count=3 if len(ns)>3 else 2;con.use_stretch=False
        for n in ns[:3]:arm.pose.bones[n].ik_stretch=0
        targets[key]=(ctl,con,eff,point,arm.pose.bones[eff].matrix.to_quaternion())
        con.mute=True
    return targets


def bake_frame(arm,action,frame,targets=None):
    bpy.context.view_layer.update()
    if targets:
        for ctl,con,eff,point,rotation in targets.values():
            p=arm.pose.bones[eff];mat=rotation.to_matrix().to_4x4();mat.translation=p.matrix.translation;p.matrix=mat
        bpy.context.view_layer.update()
    matrices={p.name:p.matrix.copy() for p in arm.pose.bones}
    for _,con,_,_,_ in (targets or {}).values():con.mute=True
    for p in arm.pose.bones:
        basis=p.bone.convert_local_to_pose(matrices[p.name],p.bone.matrix_local,
                    parent_matrix=matrices[p.parent.name] if p.parent else Matrix.Identity(4),
                    parent_matrix_local=p.parent.bone.matrix_local if p.parent else Matrix.Identity(4),invert=True)
        loc,q,_=basis.decompose()
        p.location=loc;p.rotation_quaternion=q;p.scale=(1,1,1)
        p.keyframe_insert('location',frame=frame,group=p.name)
        p.keyframe_insert('rotation_quaternion',frame=frame,group=p.name)


def pounce_pose(arm,rig,standing,source,t):
    apply(arm,blend(standing,source,.72*envelope(t,.16,.78)))
    move(arm,rig['body'],(0,0,rig['axis']['target_length_m']*.055*math.sin(math.pi*t)**2))


def tail_clearance(arm,meshes,rig,clips,original,standing):
    from audit_refinement import points
    mask=np.array([sum(g.weight for g in v.groups if m.vertex_groups[g.group].name in rig['tail'])>.12 for m in meshes for v in m.data.vertices])
    result={}
    arm.animation_data.action=None
    for c in clips:
        if c['kind'] in ('lie','rest','up','collapse','corpse'):continue
        values=[]
        for i,source in enumerate(original[c['suffix']]):
            t=i/(c['frames']-1)
            def test(degrees):
                apply(arm,source)
                if c['kind']=='pounce':pounce_pose(arm,rig,standing,source,t)
                rotate(arm,rig['tail'][0],(0,1,0),degrees);bpy.context.view_layer.update()
                return float(points(meshes)[mask,2].min())
            if test(0)>=-.001:angle=0
            else:
                low,high=0,35
                for _ in range(9):
                    middle=(low+high)/2
                    if test(middle)<-.001:low=middle
                    else:high=middle
                angle=high
            values.append(angle)
        # Conservative neighbourhood envelope eases raises without letting a
        # nearby compression frame pull the tail through the supporting plane.
        n=len(values)-1 if c['loop'] else len(values)
        values=np.array(values[:n]);smoothed=[]
        for i in range(n):
            indices=[(i+j)%n if c['loop'] else max(0,min(n-1,i+j)) for j in range(-3,4)]
            smoothed.append(max(values[i],sum(values[x] for x in indices)/7+.8))
        if c['loop']:smoothed.append(smoothed[0])
        result[c['suffix']]=smoothed
    return result


def refine(job):
    slug=job['slug'];out=Path(job['output'])
    backup=REV/'backup'/out.relative_to(ROOT.parent)
    man=json.loads((backup/'animation_manifest.json').read_text('utf-8'))
    bpy.ops.wm.open_mainfile(filepath=str(backup/f'AS_{slug}.blend'))
    bpy.context.preferences.filepaths.save_version=0
    arm=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE')
    meshes=[o for o in bpy.context.scene.objects if o.type=='MESH']
    rig=man['rig'];scene=bpy.context.scene;length=rig['axis']['target_length_m']
    original={};original_feet={};by_suffix={c['suffix']:c for c in man['clips']}
    for c in man['clips']:
        arm.animation_data.action=bpy.data.actions[c['name']]
        original[c['suffix']]=[]
        original_feet[c['suffix']]=[]
        for f in range(1,c['frames']+1):
            scene.frame_set(f);original[c['suffix']].append(snapshot(arm))
            original_feet[c['suffix']].append({ns[3] if len(ns)>3 else ns[-1]:arm.pose.bones[ns[3] if len(ns)>3 else ns[-1]].matrix.translation.copy() for ns in rig.get('chains',{}).values()})
    standing=original[man['clips'][0]['suffix']][0]
    arm.animation_data.action=None;reset(arm);bpy.context.view_layer.update()
    repairs={};skin_changed=False
    # Weight changes must improve all affected poses, not just one deep fold.
    if slug in ('black_bear','red_fox','goat'):
        repairs['smoothed_upper_limb_vertices']=smooth_skin(meshes,rig);skin_changed=True
        rig['repairs'].append('2026-10-01: smoothed thorax/upper-limb gradients; retained paw soles and original influence regions')
    if slug=='red_fox':
        for a,b in [('FL','FR'),('BL','BR')]:
            rig['chains'][a],rig['chains'][b]=rig['chains'][b],rig['chains'][a]
            rig['restfeet'][a],rig['restfeet'][b]=rig['restfeet'][b],rig['restfeet'][a]
            for c in man['clips']:
                phases=c.get('contact_offsets',{})
                if a in phases:phases[a],phases[b]=phases[b],phases[a]
        rig['repairs'].append('2026-10-01: corrected anatomical left/right roles from normalized joint positions; existing gait phases remapped by actual bone identity')
        repairs['ear_vertices']=fox_controls(arm,meshes,rig);skin_changed=True
        for values in original.values():
            for p in values:
                for n in rig['ears']:p[n]=(Vector(),Quaternion())
        for n in rig['ears']:standing[n]=(Vector(),Quaternion())
    fish=job['rig_type'] in ('aquatic','serpentine')
    targets=build_ik(arm,rig,standing) if not fish else {}
    tail_angles=tail_clearance(arm,meshes,rig,man['clips'],original,standing) if slug=='red_fox' else {}
    changed=[]
    # Shared, folded terminal rest pose: free ankle positions instead of forcing
    # all four hooves to their standing IK targets during a deep body descent.
    rest_final=None;rest_support_info=None
    if slug in ('stag_a','goat','pig','wolf','black_bear','ram','red_fox'):
        apply(arm,standing)
        move(arm,rig['body'],(0,0,-length*(.27 if slug!='red_fox' else .25)))
        rotate(arm,rig['body'],(0,1,0),3)
        for n in rig['neck']:rotate(arm,n,(0,1,0),8/max(1,len(rig['neck'])))
        for key,ns in rig['chains'].items():
            folds=(30,-115,15) if key.startswith('F') else (-40,145,-15)
            if slug in ('stag_a','goat','ram') and key.startswith('F'):folds=(50,-145,175)
            for n,degrees in zip(ns,folds):rotate(arm,n,(0,1,0),degrees)
        bpy.context.view_layer.update()
        for ctl,con,eff,point,rotation in targets.values():
            p=arm.pose.bones[eff];mat=rotation.to_matrix().to_4x4();mat.translation=p.matrix.translation;p.matrix=mat
        if slug=='red_fox':
            for n in rig['neck']:rotate(arm,n,(0,0,1),12/len(rig['neck']))
            for n in rig['tail']:rotate(arm,n,(0,0,1),18)
            rotate(arm,rig['tail'][0],(0,1,0),50)
        if slug=='wolf':rotate(arm,rig['tail'][0],(0,1,0),50)
        bpy.context.view_layer.update()
        move(arm,rig['body'],(0,0,.0003-evaluated_min_z(meshes)))
        rest_final=snapshot(arm)
        if slug in ('stag_a','goat','pig','wolf','black_bear','ram','red_fox'):
            from audit_refinement import points
            apply(arm,standing);bpy.context.view_layer.update();base=points(meshes)
            fore=sum((arm.pose.bones[rig['chains'][k][0]].matrix.translation for k in ('FL','FR')),Vector())/2
            hind=sum((arm.pose.bones[rig['chains'][k][0]].matrix.translation for k in ('BL','BR')),Vector())/2
            center=(fore.y+hind.y)/2
            width=sum(abs(arm.pose.bones[ns[0]].matrix.translation.y-center) for ns in rig['chains'].values())/4
            core=(base[:,0]>hind.x+length*.08)&(base[:,0]<fore.x-length*.12)&(abs(base[:,1]-center)<width*.65)&(base[:,2]>length*.12)
            move(arm,rig['body'],(0,0,-float(base[core,2].min())+.001))
            if slug in ('wolf','red_fox'):rotate(arm,rig['tail'][0],(0,1,0),50)
            for key,(ctl,con,eff,point,rotation) in targets.items():
                shift=(.12 if key.startswith('F') else .05) if slug in ('red_fox','goat','pig') else (-.12 if key.startswith('F') else .10)
                ctl.location=point+Vector((length*shift,0,0));con.mute=False;con.influence=1
            for _ in range(6):
                bpy.context.view_layer.update()
                for ctl,con,eff,point,rotation in targets.values():
                    p=arm.pose.bones[eff];mat=rotation.to_matrix().to_4x4();mat.translation=p.matrix.translation;p.matrix=mat
                bpy.context.view_layer.update()
                move(arm,rig['body'],(0,0,.001-float(points(meshes)[core,2].min())))
            pose_action=bpy.data.actions.new('SupportedRestTemporary')
            arm.animation_data.action=pose_action
            bake_frame(arm,pose_action,1,targets);arm.animation_data.action=None;bpy.data.actions.remove(pose_action)
            bpy.context.view_layer.update();move(arm,rig['body'],(0,0,.0003-evaluated_min_z(meshes)))
            rest_final=snapshot(arm)
            rest_support_info='Measured ventral support plane; folded paws solved and fully baked'
    for c in man['clips']:
        s=c['suffix'];kind=c['kind'];notes=[]
        rest=rest_final is not None and kind in ('lie','rest','up')
        look=kind=='look'
        scratch=kind=='scratch' or s=='ScratchPeck'
        swipe=kind=='swipe'
        preen=kind=='preen'
        rear=kind=='rear'
        tail_fix=slug=='red_fox' and s in tail_angles or slug=='wolf' and kind in ('run','start','stop','pounce','trot')
        aquatic=fish and (s.startswith('Turn_') or s in ('Burst','Brake','Hooked_In','LiftOut','LiftSupported','StruggleArc','StartWave','StopWave'))
        ear=slug=='red_fox'
        collapse=kind=='collapse'
        settled_corpse=slug=='goat' and kind=='corpse'
        howl=kind=='howl'
        wing_safe=job['rig_type']=='avian' and ('Flutter' in s or kind in ('flight','takeoff','land'))
        if not any((rest,look,scratch,swipe,preen,rear,tail_fix,aquatic,ear,collapse,howl,wing_safe,settled_corpse)):
            continue
        source_action=bpy.data.actions[c['name']];source_action.name=c['name']+'_before_refinement'
        action=bpy.data.actions.new(c['name']);action.use_fake_user=True
        arm.animation_data.action=action
        previous_q={}
        for f in range(1,c['frames']+1):
            t=(f-1)/(c['frames']-1);scene.frame_set(f)
            apply(arm,original[s][f-1])
            active_ik=False
            if slug=='red_fox' and kind=='turn':
                inner='L' if c['direction']=='left' else 'R'
                for key,(ctl,con,eff,point,rotation) in targets.items():
                    desired=original_feet[s][f-1][eff].copy()
                    desired.x=point.x+(desired.x-point.x)*(.72/1.15 if key.endswith(inner) else 1.15/.72)
                    ctl.location=desired;con.mute=False;con.influence=1
                active_ik=True;notes.append('Anatomical inner paws take the shorter turning step; left/right bone roles corrected')
            if rest:
                a=smooth(t) if kind=='lie' else 1-smooth(t) if kind=='up' else 1
                apply(arm,blend(standing,rest_final,a))
                # Breathing stays in the upper axial bones, so the belly/pads
                # keep their support points; endpoints remain exactly matched.
                for n in rig['spine'][:2]:rotate(arm,n,(0,1,0),.12*math.sin(TAU*t)*a)
                notes=['Folded limbs and supported rest; matching lie/rest/rise endpoints']
            if collapse:
                terminal=original['CorpseHold_'+s[-1]][0]
                values=blend(standing,terminal,smooth(t))
                for ns in rig['chains'].values():
                    for n in ns:
                        values[n]=(standing[n][0].lerp(terminal[n][0],smooth(t/.64)),standing[n][1].slerp(terminal[n][1],smooth(t/.64)))
                for n in rig['tail']:
                    values[n]=(standing[n][0].lerp(terminal[n][0],smooth((t-.12)/.88)),standing[n][1].slerp(terminal[n][1],smooth((t-.12)/.88)))
                apply(arm,values);notes=['Legs lose support before side settling; corpse endpoint preserved']
            if look:
                apply(arm,standing)
                keys=[(0,0),(.20,26),(.40,26),(.64,-22),(.82,-22),(1,0)]
                for (ta,va),(tb,vb) in zip(keys,keys[1:]):
                    if ta<=t<=tb:yaw=va+(vb-va)*smooth((t-ta)/(tb-ta));break
                for n in rig['neck']:rotate(arm,n,(0,0,1),yaw*.72/len(rig['neck']))
                rotate(arm,rig['head'],(0,0,1),yaw*.28)
                notes=['Two clear gaze holds and eased neck-leading return']
            if howl:
                apply(arm,standing);a=envelope(t,.24,.72)
                for n in rig['neck']:rotate_world(arm,n,(0,1,0),-44*a/len(rig['neck']))
                rotate_world(arm,rig['head'],(0,1,0),-24*a+.8*math.sin(TAU*t*3)*a)
                notes=['Eased head raise, sustained howl posture and return; mouth/audio gate retained']
            if rear:
                # Axial correction removes the strain; soften extension without
                # losing the existing planted hind paws.
                rotate(arm,rig['head'],(0,1,0),-3*envelope(t) if not c['loop'] else -.4*math.sin(TAU*t))
                notes=['Thorax weighting repaired; stable hind support and smaller neck extension']
            if scratch or swipe or preen:
                apply(arm,standing)
                for key,(ctl,con,eff,point,rotation) in targets.items():ctl.location=point;con.mute=False;con.influence=1
                active_ik=True
                if scratch:
                    u=max(0,min(1,(t-.12)/.40));a=math.sin(math.pi*u)**2
                    ctl=targets['FL'][0]
                    ctl.location+=Vector((length*.12*math.sin(TAU*u)*a,0,length*.045*a))
                    if s=='ScratchPeck':
                        a=envelope(max(0,min(1,(t-.56)/.40)),.36,.70)
                        for n in rig['neck']:rotate(arm,n,(0,1,0),(102 if slug=='hen' else 110)*a/len(rig['neck']))
                        rotate(arm,rig['head'],(0,1,0),18*a)
                    notes=['One planted foot, forward lift/backward scrape; peck follows replanting' if s=='ScratchPeck' else 'One planted foot and forward lift/backward scrape']
                if swipe:
                    side='FL' if s.endswith('_L') else 'FR';sign=1 if side=='FL' else -1
                    a=1-smooth(t) if 'Short' in s else envelope(t,.34,.54)
                    ctl=targets[side][0];ctl.location+=Vector((length*.13*a,sign*length*.08*a,length*.13*a))
                    notes=['Short swipe begins at contact and recovers to support' if 'Short' in s else 'Windup, one-paw swipe and supported recovery']
                if preen:
                    a=envelope(t,.28,.67)
                    for i,n in enumerate(rig['neck']):
                        rotate_world(arm,n,(0,0,1),108*a/len(rig['neck']))
                        yaw=math.radians(108*a*(i+1)/len(rig['neck']))
                        rotate_world(arm,n,(-math.sin(yaw),math.cos(yaw),0),16*a/len(rig['neck']))
                    yaw=math.radians(108*a)
                    rotate_world(arm,rig['head'],(-math.sin(yaw),math.cos(yaw),0),60*a)
                    notes=['Neck turns toward wing before the beak dips, stable two-foot support']
            if tail_fix:
                if slug=='red_fox' and kind=='pounce':
                    pounce_pose(arm,rig,standing,original[s][f-1],t)
                    notes.append('Contained pounce with eased takeoff and supported return to standing')
                factor=1 if c['loop'] else smooth(t) if kind=='start' else 1-smooth(t) if kind=='stop' else envelope(t)
                rotate(arm,rig['tail'][0],(0,1,0),tail_angles[s][f-1] if slug=='red_fox' else 22*factor)
                notes.append('Tail clearance during body compression; planted limb tracks preserved')
            if wing_safe:
                # Existing folded wing topology supports restrained flutters,
                # not an artificial full wingspan made by stretching feathers.
                for ns in rig['wings']:
                    for n in ns:
                        loc,q=original[s][f-1][n];arm.pose.bones[n].rotation_quaternion=Quaternion().slerp(q,.65)
                if slug=='pheasant' and kind in ('takeoff','land'):
                    flight={n:(loc.copy(),q.copy()) for n,(loc,q) in original['FlightShort'][0 if kind=='takeoff' else -1].items()}
                    for ns in rig['wings']:
                        for n in ns:flight[n]=(flight[n][0],Quaternion().slerp(flight[n][1],.65))
                    if kind=='takeoff' and t>.55:apply(arm,blend(snapshot(arm),flight,smooth((t-.55)/.45)))
                    if kind=='land' and t<.45:apply(arm,blend(flight,snapshot(arm),smooth(t/.45)))
                notes.append('Reduced folded-wing flutter strain')
            if aquatic:
                if s.startswith('Turn_') or s=='StruggleArc':
                    sign=1 if s.endswith('_L') or s=='StruggleArc' else -1
                    body=rig['body'];arm.pose.bones[body].rotation_quaternion=Quaternion()
                    rotate(arm,body,(0,0,1),sign*4*envelope(t,.27,.63))
                    for i,n in enumerate(rig['spine']):
                        u=i/max(1,len(rig['spine'])-1)
                        a=envelope(max(0,min(1,(t-u*.18)/(1-u*.18))),.28,.66)
                        rotate(arm,n,(0,0,1),sign*(18 if slug=='eel' else 12)*a/len(rig['spine']))
                    notes=['Turn curvature propagates head-to-tail instead of rotating the whole fish rigidly']
                elif s=='Burst':
                    factor=1+.55*envelope(t,.28,.52)-.70*smooth((t-.72)/.28)
                    for n in rig['spine']:
                        arm.pose.bones[n].rotation_quaternion=Quaternion().slerp(original[s][f-1][n][1],factor/1.7)
                    target=original['Coast' if slug=='carp' else 'PauseGlide' if slug=='crucian_carp' else 'SwimCruise'][0]
                    if t>.78:apply(arm,blend(snapshot(arm),target,smooth((t-.78)/.22)))
                    notes=['Burst amplitude rises and eases down into the low-amplitude glide']
                elif s=='Brake':
                    hover=original['HoverLow' if slug=='catfish' else 'Hover'][0]
                    if t>.65:apply(arm,blend(snapshot(arm),hover,smooth((t-.65)/.35)))
                    notes=['Brake returns to the hover pose rather than a rigid straight tail']
                elif s in ('StartWave','StopWave'):
                    entering=s=='StartWave'
                    begin=original['HoverWave' if entering else 'UndulateCruise'][-1]
                    end=original['UndulateCruise' if entering else 'HoverWave'][0]
                    if t<.25:apply(arm,blend(begin,snapshot(arm),smooth(t/.25)))
                    if t>.65:apply(arm,blend(snapshot(arm),end,smooth((t-.65)/.35)))
                    notes=['Matched hover/cruise wave phases at start and stop boundaries']
                elif s=='Hooked_In':
                    struggle=original['StruggleS' if slug=='eel' else 'Struggle'][0]
                    if t>.62:apply(arm,blend(snapshot(arm),struggle,smooth((t-.62)/.38)))
                    notes=['Hooked endpoint matches the first struggle phase']
                elif s in ('LiftOut','LiftSupported'):
                    land=original['LandWriggle' if slug=='eel' else 'LandFlop'][0]
                    if t>.45:apply(arm,blend(snapshot(arm),land,smooth((t-.45)/.55)))
                    notes=['Supported lift endpoint matches the landing/land-movement pose']
            if ear:
                dead=kind in ('collapse','corpse')
                amp=0 if dead else 10 if kind in ('alert','look') else 3 if kind in ('idle','sniff') else 1.5
                a=1 if c['loop'] else envelope(t)
                for i,n in enumerate(rig['ears']):
                    rotate(arm,n,(0,0,1),amp*math.sin(TAU*t)*(1+.2*math.sin(TAU*t))*a*(1 if i==0 else -.7))
                notes.append('Independent ear skin/control with quiet corpse holds')
            bpy.context.view_layer.update()
            ground=rest or collapse or settled_corpse or (fish and s in ('LiftOut','LiftSupported') and t>.45)
            if settled_corpse:notes.append('Fixed corpse support height recalibrated after weight smoothing; matches collapse terminal pose')
            if ground:
                move(arm,rig['body'],(0,0,.0003-evaluated_min_z(meshes)))
            if slug=='red_fox' and kind=='pounce':
                lowest=evaluated_min_z(meshes)
                if lowest<.0003:move(arm,rig['body'],(0,0,.0003-lowest))
            bake_frame(arm,action,f,targets if active_ik else None)
            # Guarantee continuous quaternion representatives before linear bake.
            for p in arm.pose.bones:
                if p.name in previous_q and previous_q[p.name].dot(p.rotation_quaternion)<0:
                    p.rotation_quaternion.negate();p.keyframe_insert('rotation_quaternion',frame=f,group=p.name)
                previous_q[p.name]=p.rotation_quaternion.copy()
        for fc in curves(action):
            for k in fc.keyframe_points:k.interpolation='LINEAR'
        bpy.data.actions.remove(source_action)
        c['refinement_20261001']={'changes':list(dict.fromkeys(notes)),'qa':'NOT_RUN'}
        changed.append(c['name']);print('REFINED',slug,s,flush=True)
    # Offline phase annotations are metadata, never gameplay/damage notifies.
    for c in man['clips']:
        markers=[]
        if c.get('contact_offsets'):
            for foot,offset in c['contact_offsets'].items():
                for cycle in range(c.get('gait_cycles',1)):
                    for event,phase in [('Contact',0),('Lift',c['contact_duty'])]:
                        normalized=((cycle+phase-offset)%c.get('gait_cycles',1))/c.get('gait_cycles',1)
                        markers.append({'name':foot+'_'+event,'time_s':round(normalized*c['duration'],6),'kind':'visual_contact'})
        c['events']['authored_markers']=sorted(markers,key=lambda a:a['time_s'])
        if c['suffix']=='ScratchPeck':
            c['kind']='scratch_peck'
            c['events']['authored_markers']=[{'name':n,'time_s':round(t*c['duration'],6),'kind':'visual_phase'} for n,t in [('ScratchBegin',.12),('FootReplant',.52),('PeckBegin',.56),('PeckLowest',.76),('ReturnStand',.96)]]
        elif c['kind']=='swipe':
            c['events']['authored_markers']=[{'name':'ContactVisual','time_s':0 if 'Short' in c['suffix'] else round(.34*c['duration'],6),'kind':'visual_phase'},{'name':'SupportRecovered','time_s':c['duration'],'kind':'visual_phase'}]
        c['events']['marker_authority']='offline visual reference; no automatic gameplay trigger'
    for ctl,con,eff,_,_ in targets.values():
        for p in arm.pose.bones:
            if con in list(p.constraints):p.constraints.remove(con)
        bpy.data.objects.remove(ctl,do_unlink=True)
    arm.animation_data.action=None;reset(arm);bpy.context.view_layer.update()
    write_rig_report(arm,meshes,rig,out)
    if skin_changed:export_fbx(arm,meshes,out/f'SK_{slug}.fbx',True,False)
    else:(out/f'SK_{slug}.fbx').write_bytes((backup/f'SK_{slug}.fbx').read_bytes())
    records=[]
    for c in man['clips']:
        if c['name'] in changed or slug=='red_fox':
            scene.render.fps=c['fps'];scene.frame_start=1;scene.frame_end=c['frames']
            arm.animation_data.action=bpy.data.actions[c['name']];scene.frame_set(1)
            before=c['sha256'];export_fbx(arm,meshes,Path(c['file']),False,True)
            c['sha256']=hashlib.sha256(Path(c['file']).read_bytes()).hexdigest()
            records.append({'name':c['name'],'before_sha256':before,'after_sha256':c['sha256']})
        c['skeleton_signature']=rig['skeleton_signature']
    first=man['clips'][0];arm.animation_data.action=bpy.data.actions[first['name']]
    scene.render.fps=first['fps'];scene.frame_start=1;scene.frame_end=first['frames'];scene.frame_set(1)
    bpy.ops.file.pack_all();bpy.ops.wm.save_as_mainfile(filepath=str(out/f'AS_{slug}.blend'))
    man['rig']=rig;man['ue_import']='NOT_RUN';man['visual_qa']='pending refinement review'
    man['refinement_20261001']={'backup':str(backup),'revised_actions':changed,'skin_repair':repairs,
          'baked_constraints_removed':True,'cloud_credits_used':0,'vertex_topology_uv':'preserved',
          'original_guidance_sha256':man['guidance_sha256']}
    if tail_angles:man['refinement_20261001']['tail_clearance_degrees']=tail_angles
    if rest_support_info:man['refinement_20261001']['rest_support']=rest_support_info
    save(out/'animation_manifest.json',man)
    save(out/'refinement_20261001.json',{'slug':slug,'changed_files':records,'skin_repair':repairs,
                                      'backup':str(backup),'checks':'NOT_RUN'})


def main():
    args=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
    jobs=json.loads((ROOT/'jobs.json').read_text('utf-8'))
    if args:jobs=[j for j in jobs if j['slug'] in args]
    for job in jobs:refine(job)


if __name__=='__main__':main()
