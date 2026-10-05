"""One offline .5 local-section + five measured half-extension combination.

Default mode prepares its measured-input posed skin and one surface-aligned Grip. The
optional contact check is separate so Root can reserve CPU timing.
"""
from pathlib import Path
import argparse
import json
import math
import time
import numpy as np
from reconstruct_stone_axe_hand_skin import rotation, transform_matrix, skin_positions

HERE=Path(__file__).resolve().parent
ROOT=Path('G:/GameFactory/Hearthward/.agent-local/task051')
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))


def quat_product(a,b):
    av,bv=np.array(a[:3]),np.array(b[:3])
    return np.r_[a[3]*bv+b[3]*av+np.cross(av,bv),a[3]*b[3]-av@bv]


def unit_quat(q):
    q=np.array(q);return q/np.linalg.norm(q)


def compose(local,parent):
    q=quat_product(parent['rotation_quat_xyzw'],local['rotation_quat_xyzw'])
    t=np.array(parent['translation_cm'])+rotation(parent['rotation_quat_xyzw']) @ (np.array(parent['scale_xyz'])*local['translation_cm'])
    return {'rotation_quat_xyzw':q.tolist(),'translation_cm':t.tolist(),
            'scale_xyz':(np.array(local['scale_xyz'])*parent['scale_xyz']).tolist()}


def apply(p,tf):
    return np.array(p)*tf['scale_xyz'] @ rotation(tf['rotation_quat_xyzw']).T+tf['translation_cm']


def deform(p,grip,axis):
    s=(p-grip)@axis;radial=p-grip-s[...,None]*axis
    smooth=lambda t:(lambda u:u*u*(3-2*u))(np.clip(t,0,1))
    w=smooth((s+20)/8)*smooth((26-s)/10)*smooth(-p[...,2]/12.5)
    return p-.5*w[...,None]*radial


def edge_hits(a,b,faces):
    direction=b-a;e1=faces[:,1]-faces[:,0];e2=faces[:,2]-faces[:,0]
    h=np.cross(direction,e2);det=np.einsum('ij,ij->i',e1,h)
    valid=np.abs(det)>1e-12;inv=np.divide(1.,det,out=np.zeros_like(det),where=valid)
    rel=a-faces[:,0];u=inv*np.einsum('ij,ij->i',rel,h);q=np.cross(rel,e1)
    v=inv*np.sum(q*direction,axis=-1);t=inv*np.einsum('ij,ij->i',e2,q)
    return valid&(u>=-1e-9)&(v>=-1e-9)&(u+v<=1+1e-9)&(t>=-1e-9)&(t<=1+1e-9)


def main():
    parser=argparse.ArgumentParser();parser.add_argument('--contacts',action='store_true');parser.add_argument('--all-contacts',action='store_true');args=parser.parse_args()
    started=time.perf_counter()
    capture=read(HERE/'stone-axe-palm-skin-inputs-actual.json')
    baseline=read(HERE/'stone-axe-hand-skin-reconstruction.json')
    vertices=capture['lod0_skin_vertices'];ids=np.array([v['lod0_vertex_id'] for v in vertices]);index={int(i):j for j,i in enumerate(ids)}
    bones={b['name']:b for b in capture['all_used_influence_bone_skinning_inputs']}
    matrices={b['mesh_bone_index']:np.array(b['actual_ref_to_local_matrix_row_major']).reshape(4,4) for b in bones.values()}
    old_matrices={i:m.copy() for i,m in matrices.items()}
    changed=set();changes=[];compose_errors=[]
    for prefix in ('index','middle','ring','pinky','thumb'):
        one,two,three=[bones[f'{prefix}_{i:02d}_r'] for i in (1,2,3)]
        if two['parent_mesh_bone_index']!=one['mesh_bone_index'] or three['parent_mesh_bone_index']!=two['mesh_bone_index']:
            raise RuntimeError('Actual finger parent chain differs from the measured three-joint chain')
        p1,p2,p3=[np.array(b['actual_component']['translation_cm']) for b in (one,two,three)]
        first=p2-p1;second=p3-p2;first/=np.linalg.norm(first);second/=np.linalg.norm(second)
        full=unit_quat(np.r_[np.cross(second,first),1+first@second]);half=unit_quat(full+np.array([0,0,0,1]))
        pq=np.array(one['actual_component']['rotation_quat_xyzw']);conj=pq*np.array([-1,-1,-1,1])
        delta=unit_quat(quat_product(quat_product(conj,half),pq))
        local=dict(two['actual_local']);local['rotation_quat_xyzw']=unit_quat(quat_product(delta,local['rotation_quat_xyzw'])).tolist()
        new_two=compose(local,one['actual_component']);new_three=compose(three['actual_local'],new_two)
        for bone,parent in ((two,one),(three,two)):
            reconstructed=compose(bone['actual_local'],parent['actual_component'])
            compose_errors.append({'bone':bone['name'],'maximum_original_component_matrix_entry_error':float(np.abs(transform_matrix(reconstructed)-transform_matrix(bone['actual_component'])).max())})
        for bone,new in ((two,new_two),(three,new_three)):
            matrices[bone['mesh_bone_index']]=np.array(bone['inverse_reference_matrix_row_major']).reshape(4,4) @ transform_matrix(new)
            changed.add(bone['mesh_bone_index'])
        changes.append({'bone':two['name'],'extension_degrees':math.degrees(2*math.acos(np.clip(half[3],-1,1))),
                        'parent_frame_delta_quat_xyzw':delta.tolist(),'new_component_02':new_two,'new_component_03':new_three})
    posed=skin_positions(vertices,matrices,np.float64)
    hand=capture['actual_hand_r_world'];hr=rotation(hand['rotation_quat_xyzw']);ht=np.array(hand['translation_cm'])
    to_hand=lambda p:(apply(p,capture['mesh_world'])-ht) @ hr
    posed_rigid=to_hand(posed)
    measured_rigid=to_hand([v['skinned_component_cm'] for v in vertices])
    face_ids=np.array(capture['triangles_fully_inside_selected_vertices'],dtype=int)
    face_indices=np.array([[index[int(i)] for i in face] for face in face_ids])
    fixed_vertex=np.array([not any(w['mesh_bone_index'] in changed for w in v['actual_influences'] if w['raw_uint16_weight']) for v in vertices])
    fixed_faces=np.all(fixed_vertex[face_indices],axis=1)
    bone_rigid={name:to_hand([b['actual_component']['translation_cm']])[0] for name,b in bones.items()}
    reference_normal=np.cross(bone_rigid['index_01_r']-bone_rigid['pinky_01_r'],bone_rigid['thumb_01_r']-bone_rigid['middle_01_r'])
    tips=np.mean([bone_rigid[f'{p}_03_r'] for p in ('index','middle','ring','pinky')],axis=0)
    bases=np.mean([bone_rigid[f'{p}_01_r'] for p in ('index','middle','ring','pinky')],axis=0)
    if reference_normal@(tips-bases)<0:reference_normal=-reference_normal
    reference_normal/=np.linalg.norm(reference_normal)
    old_palm=np.mean([measured_rigid[index[i]] for i in (4265,4268,4258)],axis=0)
    candidates=[]
    for fi,face in enumerate(face_ids):
        if not fixed_faces[fi] or any(vertices[index[int(i)]]['dominant_bone']!='hand_r' for i in face):continue
        points=measured_rigid[face_indices[fi]]
        normal=np.cross(points[2]-points[0],points[1]-points[0]);normal/=np.linalg.norm(normal)
        alignment=float(normal@reference_normal)
        if alignment>=math.cos(math.pi/4):
            candidates.append((float(np.linalg.norm(points.mean(0)-old_palm)),fi,normal,alignment))
    report={'meaning':'One finite offline combination; no UE, asset save or grip acceptance',
            'actual_capture':str(HERE/'stone-axe-palm-skin-inputs-actual.json'),
            'baseline_float32_component_error_cm':baseline['actual_ref_to_local_float32']['maximum_component_position_error_cm'],
            'source_pose_seconds':capture['actual_single_node_time_seconds'],'local_radial_factor':.5,'world_scale':.7,
            'changes':changes,'original_component_compose_checks':compose_errors,
            'unchanged_influence_matrices_preserved':all(np.array_equal(m,old_matrices[i]) for i,m in matrices.items() if i not in changed),
            'fixed_skin_triangle_count':int(fixed_faces.sum()),'fixed_vertex_count':int(fixed_vertex.sum()),
            'stable_hand_dominant_palm_face_candidates':len(candidates),
            'palm_face_selection':'Previously measured/viewed palm-side triangle [4265,4268,4258], reposed through all actual influences; fixed wrist-side triangle is not substituted for palm',
            'reference_palm_normal_hand_r_rigid_unit':reference_normal.tolist(),
            'contact_check':'NOT_RUN','limits':['One source .55 pose only; actual hand surface subset is open.',
            'Five second-joint local rotations change; their third-joint component matrices follow hierarchy. Other actual matrices are retained.',
            'Crossing-free geometry alone does not prove visual or full-animation grip fit.']}
    if candidates:
        _,stable_fi,_,_=min(candidates,key=lambda x:x[0])
        report['unused_stable_surface_diagnostic']={'face_ids':face_ids[stable_fi].tolist(),
            'measured_centroid_hand_r_rigid_cm':measured_rigid[face_indices[stable_fi]].mean(0).tolist(),
            'meaning':'Only stable palm-facing hand-dominant option lies near wrist/pinky side; not treated as palm centre or a separate tested Grip candidate.'}
    fi=next(i for i,face in enumerate(face_ids) if set(face)=={4265,4268,4258})
    palm_points=posed_rigid[face_indices[fi]]
    normal=np.cross(palm_points[2]-palm_points[0],palm_points[1]-palm_points[0]);normal/=np.linalg.norm(normal)
    alignment=float(normal@reference_normal);palm=palm_points.mean(0)
    report['palm_anchor_contains_changed_influences']=not bool(fixed_faces[fi])
    report['palm_anchor_shift_from_actual_original_cm']=float(np.linalg.norm(palm-old_palm))
    if alignment<math.cos(math.pi/4):
        report['result']='Previously measured palm surface no longer faces the measured palm direction after the one half-extension; no substitute pose generated.'
        (HERE/'stone-axe-half-section-finger-grip-candidate.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
        print(json.dumps(report,indent=2));return
    topology=read(ROOT/'Saved/Task055/stone-axe-handle-topology/stone-axe-handle-ue-local-topology.json')
    raw_vertex={int(v[0]):np.array(v[1:]) for v in topology['vertices_id_xyz']}
    rows=np.array(topology['triangles_id_vertex_ids'],dtype=int)
    raw=np.array([[raw_vertex[int(i)] for i in face] for face in rows[:,1:]])
    grip=np.array(topology['actual_socket_relative_locations_cm']['Grip'])
    local=read(HERE/'stone-axe-local-handle-section-candidate.json')
    axis=np.array(local['geometry']['source_axis_unit']);axis/=np.linalg.norm(axis)
    qr=np.array(local['relative_rotation_quat_xyzw']);ar=rotation(qr)
    mesh=deform(raw,grip,axis);longitudinal=(mesh-grip)@axis
    sections=[];section_sources=[]
    for a,b in ((0,1),(1,2),(2,0)):
        sa,sb=longitudinal[:,a],longitudinal[:,b];crossing=(sa*sb<=0)&(np.abs(sb-sa)>1e-12)
        selected=np.flatnonzero(crossing);t=-sa[selected]/(sb[selected]-sa[selected])
        sections.extend(mesh[selected,a]+t[:,None]*(mesh[selected,b]-mesh[selected,a]));section_sources.extend(rows[selected,0].tolist())
    sections=np.array(sections);offset=np.einsum('...j,ij->...i',sections-grip,ar)*.7
    support=int(np.argmin(offset@normal));target=palm-offset[support]
    axe_faces=np.einsum('...j,ij->...i',mesh-grip,ar)*.7+target
    report.update({'palm_anchor_skin_face_ids':face_ids[fi].tolist(),'palm_anchor_hand_r_rigid_cm':palm.tolist(),
                   'palm_anchor_normal_hand_r_rigid_unit':normal.tolist(),'palm_normal_alignment':alignment,
                   'source_axis_plane':'Actual candidate triangles sliced at dot(P-Grip, measured shaft axis)=0',
                   'actual_axis_cross_section_edge_hit_count':len(sections),'surface_support_source_triangle_id':section_sources[support],
                   'surface_support_axe_local_cm':sections[support].tolist(),
                   'Grip_target_hand_r_rigid_cm':target.tolist(),'Grip_target_hand_r_local':(target/hand['scale_xyz']).tolist(),
                   'relative_rotation_quat_xyzw':qr.tolist(),
                   'Grip_alignment_method':'Align one outer actual shaft cross-section support point directly to the reposed measured-input palm-face centroid; no rotation/position search and no invented margin',
                   'target_shift_from_old_palm_cm':float(np.linalg.norm(target-np.array(read(HERE/'stone-axe-palm-actual-surface-contacts.json')['actual_grip_target_in_hand_r_rigid_axes_cm']))),
                   'preparation_seconds':time.perf_counter()-started})
    predicted={'source_pose_seconds':capture['actual_single_node_time_seconds'],'original_vertex_ids':ids.tolist(),
               'predicted_component_cm':posed.tolist(),'predicted_hand_r_rigid_cm':posed_rigid.tolist(),'faces_original_vertex_ids':face_ids.tolist(),
               'fixed_faces_indices':np.flatnonzero(fixed_faces).tolist()}
    (HERE/'stone-axe-half-section-predicted-skin.json').write_text(json.dumps(predicted)+'\n',encoding='utf-8')
    if args.contacts or args.all_contacts:
        contact_started=time.perf_counter();minimum,maximum=axe_faces.min(1),axe_faces.max(1);eps=1e-4;tested=0
        report['contact_tolerance_cm']=eps;report['contact_check']='All 205 existing skin faces; stop at first proper crossing' if args.all_contacts else 'Fixed actual skin faces only; stop at first proper crossing'
        selected_skin_faces=np.arange(len(face_ids)) if args.all_contacts else np.flatnonzero(fixed_faces)
        crossing_key='first_surface_crossing' if args.all_contacts else 'first_fixed_surface_crossing'
        report['skin_triangle_check_limit']=len(selected_skin_faces);report['skin_triangles_checked']=0
        for skin_index in selected_skin_faces:
            report['skin_triangles_checked']+=1
            triangle=posed_rigid[face_indices[skin_index]]
            possible=np.flatnonzero(np.all(maximum>=triangle.min(0)-eps,axis=1)&np.all(minimum<=triangle.max(0)+eps,axis=1))
            faces=axe_faces[possible];tested+=len(possible)
            if not len(possible):continue
            hits=np.zeros(len(possible),dtype=bool)
            for a,b in ((0,1),(1,2),(2,0)):hits|=edge_hits(triangle[a],triangle[b],faces)
            for a,b in ((0,1),(1,2),(2,0)):
                for i in np.flatnonzero(~hits):hits[i]=bool(edge_hits(faces[i,a],faces[i,b],triangle[None])[0])
            hn=np.cross(triangle[1]-triangle[0],triangle[2]-triangle[0]);hn/=np.linalg.norm(hn)
            an=np.cross(faces[:,1]-faces[:,0],faces[:,2]-faces[:,0]);an/=np.linalg.norm(an,axis=1)[:,None]
            ap=(faces-triangle[0])@hn;hp=np.einsum('ijk,ik->ij',triangle[None]-faces[:,0,None],an)
            proper=hits&(ap.min(1)<-eps)&(ap.max(1)>eps)&(hp.min(1)<-eps)&(hp.max(1)>eps)
            if proper.any():
                j=int(np.flatnonzero(proper)[0]);report[crossing_key]={
                    'skin_face_array_index':int(skin_index),'skin_original_vertex_ids':face_ids[skin_index].tolist(),'axe_source_triangle_id':int(rows[possible[j],0]),
                    'axe_source_vertex_ids':rows[possible[j],1:].tolist(),
                    'contains_changed_skin_influences':not bool(fixed_faces[skin_index]),
                    'skin_triangle_hand_r_rigid_cm':triangle.tolist(),'axe_triangle_hand_r_rigid_cm':faces[j].tolist(),
                    'axe_signed_range_to_skin_plane_cm':[float(ap[j].min()),float(ap[j].max())],
                    'skin_signed_range_to_axe_plane_cm':[float(hp[j].min()),float(hp[j].max())],
                    'skin_influence_bones':sorted({w['bone_name'] for i in face_indices[skin_index] for w in vertices[int(i)]['actual_influences'] if w['raw_uint16_weight']})}
                report['result']='Rejected: actual skin still intersects the one combination. No next candidate generated.';break
        if crossing_key not in report:report['result']='No proper crossing found in all 205 existing skin faces; visual/open-boundary/full-animation fit remain unaccepted.' if args.all_contacts else 'No proper fixed-face crossing found; changed finger faces/visual/full-animation fit remain unaccepted.'
        report['surface_aabb_survivor_pair_count' if args.all_contacts else 'fixed_face_aabb_survivor_pair_count']=tested;report['contact_seconds']=time.perf_counter()-contact_started
    (HERE/'stone-axe-half-section-finger-grip-candidate.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(report,indent=2))


if __name__=='__main__':main()
