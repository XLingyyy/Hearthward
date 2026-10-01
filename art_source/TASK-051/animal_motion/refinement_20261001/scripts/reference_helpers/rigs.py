"""Anatomical rig repair on the original topology and textures.

Mappings below come from measured joint positions and weight centroids. The fox
has reversed numeric limb labels; the bird numbered limbs are legs, not wings.
"""
import bpy,math,json,re
from mathutils import Vector,Matrix

def raw(name):return 'tripo::'+name if not name.startswith('bone_') else name
def limb(prefix,count):return [raw(prefix+str(i)) for i in range(count)]

def chains_for(slug):
    if slug=='red_fox':
        return {'FL':[raw('Spine_5'),'bone_18','bone_19','bone_20','bone_21'],
         'FR':limb('1_Right_Limb_',5),'BL':limb('0_Left_Limb_',5),'BR':limb('0_Right_Limb_',5)}
    if slug=='hen':return {'FL':limb('0_Left_Limb_',3),'FR':limb('0_Right_Limb_',3)}
    if slug=='pheasant':return {'FL':['bone_9']+limb('0_Left_Limb_',2),'FR':['bone_19']+limb('0_Right_Limb_',2)}
    if slug=='ram':return {'FL':['bone_7']+limb('0_Left_Limb_',3),'FR':limb('0_Right_Limb_',4),'BL':limb('1_Left_Limb_',5),'BR':limb('1_Right_Limb_',5)}
    fore=5 if slug in ('pig','black_bear') else 4
    hind=4 if slug in ('hare','goat') else 5
    return {'FL':limb('0_Left_Limb_',fore),'FR':limb('0_Right_Limb_',fore),'BL':limb('1_Left_Limb_',hind),'BR':limb('1_Right_Limb_',hind)}

def descendants(b):
    return [b]+[n for c in b.children for n in descendants(c)]

def normalize(arm,meshes,job):
    slug=job['slug'];fish=job['rig_type'] in ('aquatic','serpentine')
    heads={b.name:arm.matrix_world@b.head_local for b in arm.data.bones}
    if not fish:
        chains=chains_for(slug)
        if len(chains)==4:
            fore=sum((heads[chains[k][0]] for k in ('FL','FR')),Vector())/2
            back=sum((heads[chains[k][0]] for k in ('BL','BR')),Vector())/2
            forward=fore-back
        else:forward=heads[raw('Head_0')]-heads[raw('Root')]
    elif slug in ('carp','crucian_carp'):forward=Vector((0,-1,0))
    elif slug=='catfish':forward=heads[raw('Spine_0')]-heads[raw('Spine_4')]
    else:forward=heads['bone_1']-heads[raw('Tail_1')]
    angle=-math.atan2(forward.y,forward.x)
    rotation=Matrix.Rotation(angle,4,'Z')
    points=[rotation@(o.matrix_world@v.co) for o in meshes for v in o.data.vertices]
    lo=Vector([min(v[i] for v in points) for i in range(3)])
    hi=Vector([max(v[i] for v in points) for i in range(3)])
    target={'hare':.65,'hen':.65,'pheasant':.65,'black_bear':2.2,'carp':.60,'crucian_carp':.30,'catfish':.90,'eel':1.0}.get(slug,1.6)
    scale=target/(hi.x-lo.x)
    center=Vector(((hi.x+lo.x)/2,(hi.y+lo.y)/2,lo.z))
    transform=Matrix.Scale(scale,4)@Matrix.Translation(-center)@rotation
    old_matrices={o.name:o.matrix_world.copy() for o in meshes}
    arm.data.transform(transform@arm.matrix_world)
    arm.matrix_world=Matrix.Identity(4)
    for o in meshes:
        o.parent=None;o.data.transform(transform@old_matrices[o.name]);o.matrix_world=Matrix.Identity(4)
    return {'rotation_z_degrees':math.degrees(angle),'scale':scale,'target_length_m':target,'forward':'+X','up':'+Z','dcc_units':'metres','engine_units':'centimetres'}

def transfer_group(mesh,old,new):
    g=mesh.vertex_groups.get(old)
    if not g:return
    dst=mesh.vertex_groups.get(new) or mesh.vertex_groups.new(name=new)
    for v in mesh.data.vertices:
        w=next((x.weight for x in v.groups if x.group==g.index),0)
        if w:dst.add([v.index],w,'ADD')
    mesh.vertex_groups.remove(g)

def rotate_bone(arm,name,axis,degrees):
    if name not in arm.pose.bones:return
    p=arm.pose.bones[name]
    q=p.bone.matrix_local.to_quaternion()
    from mathutils import Quaternion
    p.rotation_mode='QUATERNION'
    p.rotation_quaternion=q.inverted()@Quaternion(Vector(axis),math.radians(degrees))@q

def make_land(arm,meshes,job):
    chains=chains_for(job['slug'])
    # Sanitize names before writing any UE-targeted skeleton.
    names={b.name:re.sub('[^A-Za-z0-9_]', '_',b.name).replace('tripo__','') for b in arm.data.bones}
    names[raw('Root')]='pelvis'  # UE FName is case-insensitive; Root and root collide.
    for mesh in meshes:
        for g in mesh.vertex_groups:
            if g.name in names:g.name=names[g.name]
    for b in arm.data.bones:b.name=names[b.name]
    chains={k:[names[n] for n in ns] for k,ns in chains.items()}
    body=names[raw('Root')]
    rest={b.name:b.head_local.copy() for b in arm.data.bones}
    repairs=[];extra={'wings':[],'ears':[],'tail':[]}
    bpy.context.view_layer.objects.active=arm;arm.select_set(True)
    bpy.ops.object.mode_set(mode='EDIT')
    e=arm.data.edit_bones
    root=e.new('root');root.head=(0,0,0);root.tail=(0,0,.1)
    body_bone=e[body];body_bone.parent=root
    hip=sum((rest[c[0]] for c in chains.values()),Vector())/len(chains)
    body_bone.head=hip;body_bone.tail=hip+Vector((.1,0,0))
    repairs.append('Added fixed game root; moved deforming body pivot to anatomical hip center')
    # Orient actual limb links toward their next measured joint. This is essential
    # for IK: the original FBX display tails are unrelated to joint connectivity.
    for ns in chains.values():
        for a,b in zip(ns,ns[1:]):
            e[a].tail=e[b].head;e[a].align_roll(Vector((0,1,0)))
            e[b].parent=e[a];e[b].use_connect=False
    if job['slug']=='goat':
        # Head_3's weights are at the tail, 0.8 m away from its named joint.
        wrong=e['Head_3'];wrong.parent=body_bone
        back=(rest[chains['BL'][0]]+rest[chains['BR'][0]])/2
        wrong.head=back+Vector((-.12,0,.05));wrong.tail=wrong.head+Vector((-.12,0,-.04))
        extra['tail']=['Head_3'];repairs.append('Repositioned Head_3 tail weights onto a tail pivot instead of the muzzle')
    if job['slug']=='red_fox':
        # Tail_0 is actually in the neck hierarchy but its vertices form the tail.
        base=(rest[chains['BL'][0]]+rest[chains['BR'][0]])/2
        tip=Vector((min(v.co.x for o in meshes for v in o.data.vertices),0,.15))
        for i in range(3):
            b=e.new('TailRepair_'+str(i));b.head=base.lerp(tip,i/3);b.tail=base.lerp(tip,(i+1)/3)
            b.parent=body_bone if i==0 else e['TailRepair_'+str(i-1)];extra['tail'].append(b.name)
        repairs.append('Added anatomical 3-link tail; removed tail skin from the neck chain')
    if job['slug'] in ('hen','pheasant'):
        # Folded wing geometry needs shoulder/forearm/tip bones independent of legs.
        for sign,side in ((1,'L'),(-1,'R')):
            height=hip.z+(.05 if job['slug']=='hen' else -.01)
            width=max(abs(v.co.y) for o in meshes for v in o.data.vertices)*.65
            pts=[Vector((hip.x+.04,sign*width*.40,height)),Vector((hip.x-.09,sign*width*.90,height-.015)),Vector((hip.x-.20,sign*width,height-.045)),Vector((hip.x-.26,sign*width*.75,height-.065))]
            ns=[]
            for i in range(3):
                b=e.new(f'Wing_{side}_{i}');b.head=pts[i];b.tail=pts[i+1]
                b.parent=body_bone if i==0 else e[ns[-1]];b.align_roll(Vector((0,0,1)));ns.append(b.name)
            extra['wings'].append(ns)
        repairs.append('Added two 3-link folded-wing chains with isolated surface weights')
    bpy.ops.object.mode_set(mode='OBJECT')
    if job['slug']=='red_fox':
        for mesh in meshes:
            g=mesh.vertex_groups.get('Tail_0')
            if g:
                for v in mesh.data.vertices:
                    w=next((x.weight for x in v.groups if x.group==g.index),0)
                    if w:
                        u=max(0,min(2.999,3*(base.x-v.co.x)/max(.001,base.x-tip.x)))
                        a=min(2,int(u));b=min(2,a+1);f=u-a
                        for n,weight in ((extra['tail'][a],w*(1-f)),(extra['tail'][b],w*f)):
                            vg=mesh.vertex_groups.get(n) or mesh.vertex_groups.new(name=n)
                            if weight:vg.add([v.index],weight,'ADD')
                mesh.vertex_groups.remove(g)
            # This ear-labelled branch contains erroneous residual weights across
            # most of the body. Transfer to the remaining local influences, never
            # to a single hind leg (which stretches the ears across the animal).
            wrong=mesh.vertex_groups.get('1_Left_Limb_0')
            if wrong:
                for v in mesh.data.vertices:
                    gs=[g for g in v.groups if g.group!=wrong.index and g.weight>1e-8]
                    total=sum(g.weight for g in gs)
                    if total:
                        for g in gs:mesh.vertex_groups[g.group].add([v.index],g.weight/total,'REPLACE')
                mesh.vertex_groups.remove(wrong)
    if extra['wings']:
        for mesh in meshes:
            groups={g.index:g.name for g in mesh.vertex_groups}
            width=max(abs(p.co.y) for p in mesh.data.vertices)
            for v in mesh.data.vertices:
                x,y,z=v.co
                if not (hip.x-.27<x<hip.x+.07 and hip.z-.14<z<hip.z+.10):continue
                # No weights on legs, belly, neck or midline; fade shoulder edges.
                a=max(0,min(1,(abs(y)-width*.26)/(width*.26)))
                a*=max(0,min(1,(z-(hip.z-.14))/.06))*max(0,min(1,((hip.z+.10)-z)/.04))
                a*=max(0,min(1,(x-(hip.x-.27))/.055))*max(0,min(1,((hip.x+.07)-x)/.055))
                if a<=0:continue
                for g in list(v.groups):mesh.vertex_groups[g.group].add([v.index],g.weight*(1-.92*a),'REPLACE')
                ns=extra['wings'][0 if y>=0 else 1]
                u=max(0,min(1.999,(hip.x+.04-x)/.12));i=int(u);f=u-i
                for n,w in ((ns[i],.92*a*(1-f)),(ns[i+1],.92*a*f)):
                    vg=mesh.vertex_groups.get(n) or mesh.vertex_groups.new(name=n)
                    if w:vg.add([v.index],w,'ADD')
    # Restore total weight to 1; a new root intentionally has no skin weights.
    # Repair cross-foot skin assignments seen in the bird toes and ram's hoof.
    foot_sets={}
    for key,ns in chains.items():
        eff=ns[3] if len(ns)>3 else ns[-1]
        foot_sets[key]={b.name for b in descendants(arm.data.bones[eff])}
    membership={n:k for k,ns in foot_sets.items() for n in ns}
    for mesh in meshes:
        groups={g.index:g.name for g in mesh.vertex_groups}
        for v in mesh.data.vertices:
            footgroups=[g for g in v.groups if groups[g.group] in membership]
            if sum(g.weight for g in footgroups)<.65:continue
            nearest=min(chains,key=lambda k:(v.co-arm.data.bones[chains[k][3] if len(chains[k])>3 else chains[k][-1]].head_local).length_squared)
            if any(membership[groups[g.group]]!=nearest and g.weight>.1 for g in footgroups):
                wrong=sum(g.weight for g in footgroups)
                for g in footgroups:mesh.vertex_groups[g.group].remove([v.index])
                n=min(foot_sets[nearest],key=lambda n:(v.co-arm.data.bones[n].head_local).length_squared)
                vg=mesh.vertex_groups.get(n) or mesh.vertex_groups.new(name=n);vg.add([v.index],wrong,'ADD')
    repairs.append('Corrected spatially mismatched foot/toe weights and normalized skin influences')
    # Planted paws/hooves must follow the actual foot control. Several source
    # meshes attach the paw sole to the shank or pelvis despite an ankle bone.
    # Isolate the lower contact region with a smooth anatomical fade.
    size=max(v.co.x for o in meshes for v in o.data.vertices)-min(v.co.x for o in meshes for v in o.data.vertices)
    for mesh in meshes:
        for v in mesh.data.vertices:
            key=min(chains,key=lambda k:(v.co-arm.data.bones[chains[k][3] if len(chains[k])>3 else chains[k][-1]].head_local).length_squared)
            eff=chains[key][3] if len(chains[key])>3 else chains[key][-1];p=arm.data.bones[eff].head_local
            radial=Vector((v.co.x-p.x,v.co.y-p.y,0)).length
            a=max(0,min(1,(p.z+size*.07-v.co.z)/(size*.055)))
            a*=max(0,min(1,(size*.17-radial)/(size*.04)))
            if a<=0:continue
            for g in list(v.groups):mesh.vertex_groups[g.group].add([v.index],g.weight*(1-a),'REPLACE')
            vg=mesh.vertex_groups.get(eff) or mesh.vertex_groups.new(name=eff);vg.add([v.index],a,'ADD')
    if job['slug']=='black_bear':
        # Source leg groups spill through the torso centre and belly. Those
        # vertices must follow the axial skeleton when the legs fold underneath.
        # Use smooth spatial masks; preserve the outer shoulder/thigh transition.
        axial=[body]+[b.name for b in arm.data.bones if b.name.startswith('Spine_')]
        leg_names={n for ns in chains.values() for n in ns}
        front=sum((arm.data.bones[chains[k][0]].head_local for k in ('FL','FR')),Vector())/2
        back=sum((arm.data.bones[chains[k][0]].head_local for k in ('BL','BR')),Vector())/2
        half_width=sum(abs(arm.data.bones[ns[0]].head_local.y-hip.y) for ns in chains.values())/4
        def eased(x):
            x=max(0,min(1,x));return x*x*(3-2*x)
        for mesh in meshes:
            groups={g.index:g.name for g in mesh.vertex_groups}
            for v in mesh.data.vertices:
                x,y,z=v.co
                inside=eased((x-back.x+size*.07)/(size*.10))*eased((front.x+size*.07-x)/(size*.10))
                height=eased((z-size*.15)/(size*.14))
                core=1-eased((abs(y-hip.y)-half_width*.42)/max(.001,half_width*.60))
                middle=eased((x-back.x)/(size*.15))*eased((front.x-x)/(size*.15))
                above=eased((z-(hip.z-size*.10))/(size*.14))
                suppression=max(core,middle,above)*inside*height
                if suppression<1e-6:continue
                removed=0
                for g in list(v.groups):
                    if groups[g.group] in leg_names:
                        removed+=g.weight*suppression
                        mesh.vertex_groups[g.group].add([v.index],g.weight*(1-suppression),'REPLACE')
                if removed:
                    support=sorted(axial,key=lambda n:(arm.data.bones[n].head_local-v.co).length_squared)[:3]
                    weights=[1/max(size*.025,(arm.data.bones[n].head_local-v.co).length)**3 for n in support]
                    total=sum(weights)
                    for n,w in zip(support,weights):
                        vg=mesh.vertex_groups.get(n) or mesh.vertex_groups.new(name=n)
                        vg.add([v.index],removed*w/total,'ADD')
                    groups={g.index:g.name for g in mesh.vertex_groups}
        repairs.append('Removed leg-bone contamination in torso core and belly; rebound to nearby axial bones with smooth anatomical fades')
    # Average seam duplicates and nearby skin weights in a compact spatial
    # neighbourhood. This removes discontinuous weights responsible for visible
    # hair/shoulder tears while preserving mesh topology and rigid skull regions.
    from mathutils.kdtree import KDTree
    import numpy as np
    for mesh in meshes:
        groups=list(mesh.vertex_groups);weights=np.zeros((len(mesh.data.vertices),len(groups)),dtype=np.float32)
        tree=KDTree(len(mesh.data.vertices))
        for v in mesh.data.vertices:
            tree.insert(v.co,v.index)
            for g in v.groups:weights[v.index,g.group]=g.weight
        tree.balance();near=[];factors=[]
        neighbours=32 if job['slug'] in ('goat','black_bear','pig') else 16
        iterations=80 if job['slug']=='goat' else 48 if job['slug'] in ('black_bear','pig') else 24
        for v in mesh.data.vertices:
            ns=tree.find_n(v.co,neighbours);near.append([i for _,i,d in ns]);factors.append([math.exp(-(d/(size*.012))**2) for _,i,d in ns])
        near=np.array(near);factors=np.array(factors,dtype=np.float32);factors/=factors.sum(axis=1)[:,None]
        for _ in range(iterations):weights=.40*weights+.60*(weights[near]*factors[:,:,None]).sum(axis=1)
        for v,w in zip(mesh.data.vertices,weights):
            keep=np.argsort(w)[-8:];total=float(w[keep][w[keep]>1e-5].sum())
            for g in list(v.groups):mesh.vertex_groups[g.group].remove([v.index])
            for i in keep:
                if w[i]>1e-5:groups[i].add([v.index],float(w[i])/total,'REPLACE')
    repairs.append('Rebound paw soles to contact bones; smoothed seam and shoulder weights with at most eight influences')
    for mesh in meshes:
        for v in mesh.data.vertices:
            total=sum(g.weight for g in v.groups)
            if total>0:
                for g in list(v.groups):mesh.vertex_groups[g.group].add([v.index],g.weight/total,'REPLACE')
    # Neck and tail roles are identified from hierarchy/positions, with explicit
    # overrides for mislabeled generic and fox chains.
    if job['slug']=='stag_a':neck=['Head_0','bone_14','bone_15','bone_16'];head='bone_16';tail=['bone_29','bone_30','bone_31']
    elif job['slug']=='red_fox':neck=['Tail_0','bone_29','bone_30','bone_31'];head='bone_31';tail=extra['tail']
    elif job['slug']=='pheasant':neck=['bone_1','bone_2','bone_3','Head_0'];head='Head_1';tail=['Spine_1','Spine_2','Spine_3','Spine_4']
    elif job['slug']=='hen':neck=['bone_16','bone_17','bone_18','bone_19','Head_0'];head='Head_2';tail=['Spine_0','Spine_1']
    else:
        neck=[b.name for b in arm.data.bones if b.name.startswith('Head_') and b.name not in extra['tail']][:2]
        head=neck[-1];tail=extra['tail'] or [b.name for b in arm.data.bones if b.name.startswith('Tail_')][:3]
    if job['slug']=='hare':extra['ears']=['Head_2','bone_6']
    elif job['slug']=='wolf':extra['ears']=['bone_6','Head_2']
    elif job['slug']=='red_fox':extra['ears']=[]
    spine=[b.name for b in arm.data.bones if b.name.startswith('Spine_')]
    if job['slug'] in ('hen','pheasant'):spine=[]
    # Foot controls are kept out of game exports; every deforming bone is baked.
    targets={};restfeet={}
    for key,ns in chains.items():
        eff=ns[3] if len(ns)>3 else ns[-1]
        solver=ns[2] if len(ns)>3 else ns[-2]
        pos=arm.data.bones[eff].head_local.copy()
        ctl=bpy.data.objects.new('IK_'+key,None);bpy.context.collection.objects.link(ctl);ctl.location=pos
        con=arm.pose.bones[solver].constraints.new('IK');con.target=ctl;con.chain_count=3 if len(ns)>3 else 2;con.use_stretch=False
        start=arm.data.bones[ns[0]].head_local.copy();axis=(pos-start).normalized()
        candidates=[arm.data.bones[n].head_local-start for n in ns[1:-1]]
        bends=[v-axis*v.dot(axis) for v in candidates]
        bend=max(bends,key=lambda v:v.length_squared)
        if bend.length<.001:bend=Vector((-1 if key.startswith('F') else 1,0,0))
        pole=bpy.data.objects.new('IKPole_'+key,None);bpy.context.collection.objects.link(pole)
        pole.location=(start+pos)/2+bend.normalized()*(pos-start).length*3
        con.pole_target=pole
        rootbone=arm.data.bones[ns[0]]
        normal=axis.cross(bend).normalized();projected=normal.cross(rootbone.vector).normalized()
        angle=rootbone.x_axis.angle(projected)
        if rootbone.x_axis.cross(projected).dot(rootbone.vector)<0:angle=-angle
        con.pole_angle=angle
        for n in ns[:3]:arm.pose.bones[n].ik_stretch=0
        targets[key]=(ctl,con,eff);restfeet[key]=list(pos)
    return {'body':body,'root':'root','chains':chains,'spine':spine,'neck':neck,'head':head,**extra,'tail':tail,
            'restfeet':restfeet,'hip':list(hip),'repairs':repairs},targets

def make_fish(arm,meshes,job):
    """Replace incorrect generic head/tail labels with a continuous body chain.

    The eel spine follows the original curved rest centerline. Bind-relative
    rotations preserve that curve. Vertex order, UVs and materials are unchanged.
    """
    slug=job['slug'];old={b.name:b.head_local.copy() for b in arm.data.bones}
    special={};special_points={}
    if slug=='catfish':
        for side,sources in [('R',[raw('Head_0'),raw('Head_1'),raw('Tail_0')]),('L',['bone_17','bone_18','bone_19'])]:
            points=[old[n] for n in sources];points.append(points[-1]+(points[-1]-points[-2])*.65)
            special_points[side]=points
            for i,n in enumerate(sources):special[n]=f'Barbel_{side}_{i}'
    elif slug=='eel':special={raw('Head_0'):'Pectoral_R','bone_10':'Pectoral_L'}
    special_weights={}
    for mesh in meshes:
        names={g.index:g.name for g in mesh.vertex_groups}
        special_weights[mesh.name]={v.index:[(special[names[g.group]],g.weight) for g in v.groups if names[g.group] in special] for v in mesh.data.vertices}
    if slug=='eel':
        # Follow the source's neck, trunk and five tail sections by arclength.
        line=[old['bone_2'],old['bone_1'],old[raw('Tail_0')],*[old[raw('Tail_'+str(i))] for i in range(1,6)]]
    else:
        if slug=='carp':line=[Vector((.28,0,.145)),old[raw('Spine_0')],old[raw('Spine_1')],old['bone_19'],old['bone_20'],old[raw('Tail_0')]]
        elif slug=='crucian_carp':line=[Vector((.145,0,.085)),*[old[raw('Spine_'+str(i))] for i in range(4)],old[raw('Tail_0')]]
        else:line=[Vector((.43,0,.15)),*[old[raw('Spine_'+str(i))] for i in range(5)]]
    # Sample a continuous ordered chain at 12 (fish) / 24 (eel) segments.
    lengths=[0.0]
    for a,b in zip(line,line[1:]):lengths.append(lengths[-1]+(b-a).length)
    count=24 if slug=='eel' else 12
    pts=[]
    for i in range(count+1):
        s=lengths[-1]*i/count;k=next((k for k in range(len(lengths)-1) if lengths[k+1]>=s),len(line)-2)
        f=(s-lengths[k])/max(1e-8,lengths[k+1]-lengths[k]);pts.append(line[k].lerp(line[k+1],f))
    # Avoid old armature data mutation affecting source caches.
    for mesh in meshes:
        for mod in list(mesh.modifiers):
            if mod.type=='ARMATURE':mesh.modifiers.remove(mod)
        for g in list(mesh.vertex_groups):mesh.vertex_groups.remove(g)
    bpy.data.objects.remove(arm,do_unlink=True)
    data=bpy.data.armatures.new('SKEL_'+slug);arm=bpy.data.objects.new('SK_'+slug,data);bpy.context.collection.objects.link(arm)
    bpy.context.view_layer.objects.active=arm;arm.select_set(True);bpy.ops.object.mode_set(mode='EDIT')
    e=data.edit_bones;root=e.new('root');root.head=(0,0,0);root.tail=(0,0,.05)
    body=e.new('body');body.head=pts[0];body.tail=pts[0]+Vector((.05,0,0));body.parent=root
    ns=[]
    for i in range(count):
        b=e.new('Spine_'+f'{i:02}');b.head=pts[i];b.tail=pts[i+1];b.parent=body if i==0 else e[ns[-1]];b.align_roll(Vector((0,0,1)));ns.append(b.name)
    # Independent paired fin controls in the original visible fin regions.
    fins=[];size=max(v.co.x for o in meshes for v in o.data.vertices)-min(v.co.x for o in meshes for v in o.data.vertices)
    for side,sign in (('L',1),('R',-1)):
        b=e.new('Pectoral_'+side);b.head=pts[int(count*.22)]+Vector((0,sign*size*.05,-size*.035));b.tail=b.head+Vector((-size*.09,sign*size*.09,-size*.04));b.parent=e[ns[int(count*.22)]];fins.append(b.name)
    barbels=[]
    for side,points in special_points.items():
        chain=[]
        for i in range(3):
            b=e.new(f'Barbel_{side}_{i}');b.head=points[i];b.tail=points[i+1];b.parent=e[ns[0]] if i==0 else e[chain[-1]];chain.append(b.name)
        barbels.append(chain)
    bpy.ops.object.mode_set(mode='OBJECT')
    # Smooth 3-neighbour segment weights along centerline, preventing cylindrical
    # ribs / gaps when a long eel bends across a coarse original 11-bone rig.
    for mesh in meshes:
        mod=mesh.modifiers.new('Skin','ARMATURE');mod.object=arm
        groups=[mesh.vertex_groups.new(name=n) for n in ns]
        fin_groups=[mesh.vertex_groups.new(name=n) for n in fins]
        special_groups={n:mesh.vertex_groups.get(n) or mesh.vertex_groups.new(name=n) for n in special.values()}
        for v in mesh.data.vertices:
            best=None
            for i,(a,b) in enumerate(zip(pts,pts[1:])):
                ab=b-a;f=max(0,min(1,(v.co-a).dot(ab)/max(1e-9,ab.length_squared)));d=(v.co-a-ab*f).length_squared
                if best is None or d<best[0]:best=(d,i+f)
            u=best[1]
            ws=[(i,math.exp(-((i+.5-u)/.65)**2)) for i in range(max(0,int(u)-1),min(count,int(u)+2))]
            total=sum(w for i,w in ws)
            fin=0.0
            if slug!='eel' and size*.14<v.co.x<size*.32 and abs(v.co.y)>size*.105 and v.co.z<pts[int(count*.22)].z:
                fin=min(.9,(abs(v.co.y)-size*.105)/(size*.055))
            sw=special_weights[mesh.name][v.index];reserved=min(.98,sum(w for n,w in sw))
            for i,w in ws:groups[i].add([v.index],(1-reserved)*(1-fin)*w/total,'REPLACE')
            if fin:fin_groups[0 if v.co.y>0 else 1].add([v.index],(1-reserved)*fin,'REPLACE')
            if sw:
                subtotal=sum(w for n,w in sw)
                for n,w in sw:special_groups[n].add([v.index],reserved*w/subtotal,'REPLACE')
    return arm,{'body':'body','root':'root','spine':ns,'fins':fins,'barbels':barbels,'neck':[],'head':ns[0],'tail':ns[-4:],
             'centerline':[list(p) for p in pts],'hip':list(pts[0]),'repairs':[f'Rebuilt continuous {count}-segment anatomical spine and smooth weights; retained curved bind pose' if slug=='eel' else f'Rebuilt {count}-segment body/tail chain; corrected generic head/fin assignment','Added fixed root and independent paired pectoral fin controls']}
